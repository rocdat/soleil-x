
import 'regent'

local A = require 'admiral'

local crender
local link_flags
do
  local root_dir = arg[0]:match(".*/") or "./"
  assert(os.getenv('LG_RT_DIR'), "LG_RT_DIR should be set!")
  local runtime_dir = os.getenv('LG_RT_DIR') .. "/"
  local legion_dir = runtime_dir .. "legion/"
  local mapper_dir = runtime_dir .. "mappers/"
  local realm_dir = runtime_dir .. "realm/"
  local render_cc = root_dir .. "render.cc"
  if os.getenv('SAVEOBJ') == '1' then
    render_so = root_dir .. "librender.so"
    link_flags = terralib.newlist({"-L" .. root_dir, "-lrender"})
  else
    render_so = os.tmpname() .. ".so"
  end
  local cxx = os.getenv('CXX') or 'c++'

  local cxx_flags = "-O2 -Wall -Werror"
  if os.execute('test "$(uname)" = Darwin') == 0 then
    cxx_flags =
      (cxx_flags ..
         " -dynamiclib -single_module -undefined dynamic_lookup -fPIC")
  else
    cxx_flags = cxx_flags .. " -shared -fPIC"
  end

  local cmd = (cxx .. " " .. cxx_flags .. " -I " .. runtime_dir .. " " ..
                 " -I " .. mapper_dir .. " " .. " -I " .. legion_dir .. " " ..
                 " -I " .. realm_dir .. " " .. render_cc .. " -o " .. render_so)
  if os.execute(cmd) ~= 0 then
    print("Error: failed to compile " .. render_cc)
    assert(false)
  end
  terralib.linklibrary(render_so)
  crender = terralib.includec("render.h", {"-I", root_dir, "-I", runtime_dir,
                                                  "-I", mapper_dir, "-I", legion_dir,
                                                  "-I", realm_dir})
end


-------------------------------------------------------------------------------
-- Module parameters
-------------------------------------------------------------------------------

return function(cellsRel, particlesRel, xnum, ynum, znum, origin, width)

local p_cells   = cellsRel:primPartSymbol()
local cellsType = cellsRel:regionType()
local p_particles   = particlesRel:primPartSymbol()
local particlesType = particlesRel:regionType()
local tiles = A.primColors()
local imageRegion

-------------------------------------------------------------------------------
-- Local tasks
-------------------------------------------------------------------------------

local task Render(cells : cellsType, particles : particlesType)
where
  reads(cells.{centerCoordinates, velocity, temperature}),
  reads(particles.{__valid, cell, position, density, particle_temperature, tracking})
do
  -- var pr_cells : regentlib.c.legion_physical_region_t[3] = __physical(cells)
  crender.cxx_render(__runtime(), __context(), __physical(cells), __fields(cells),
                        __physical(particles), __fields(particles), xnum, ynum, znum)
end


local task debug_particles(particles : particlesType)
where
  reads(particles.{cell, position, density, particle_temperature})
do
  regentlib.c.printf("=== particles in tile ===\n")
  for p in particles do
    -- if p.cell[0] > 0 then
      regentlib.c.printf("(%d) %lf %lf %lf  %lf  %lf\n",
        p.cell,
        p.position[0], p.position[1], p.position[2],
        p.density, p.particle_temperature)
    -- end
  end
end

local task debug_cells(cells : cellsType)
where
  reads(cells.{ centerCoordinates, velocity })
do
  regentlib.c.printf("=== cells centerCoordinates in tile ===\n")
  for c in cells do
    regentlib.c.printf("%lf %lf %lf   %lf %lf %lf\n",
      c.centerCoordinates[0], c.centerCoordinates[1], c.centerCoordinates[2],
      c.velocity[0], c.velocity[1], c.velocity[2])
  end
end



fspace PixelFields {
  R : float,
  G : float,
  B : float,
  A : float,
  Z : float
}



local task MakePartition(r : region(ispace(int3d), PixelFields),
                         colors : ispace(int3d),
                         level : int,
                         pow2level : int)

  regentlib.c.printf('level %d:\n', level)

  regentlib.assert([int](colors.bounds.lo.x) == 0, '')
  regentlib.assert([int](colors.bounds.lo.y) == 0, '')
  regentlib.assert([int](colors.bounds.lo.z) == 0, '')

  regentlib.assert([int](r.bounds.lo.x) == 0, '')
  regentlib.assert([int](r.bounds.lo.y) == 0, '')
  regentlib.assert([int](r.bounds.lo.z) == 0, '')

  regentlib.assert(([int](r.bounds.hi.x) + 1) % ([int](colors.bounds.hi.x) + 1) == 0, '')
  regentlib.assert(([int](r.bounds.hi.y) + 1) % ([int](colors.bounds.hi.y) + 1) == 0, '')
  regentlib.assert(([int](r.bounds.hi.z) + 1) % ([int](colors.bounds.hi.z) + 1) == 0, '')

  var elemsPerTileX = ([int](r.bounds.hi.x) + 1) / ([int](colors.bounds.hi.x) + 1)
  var elemsPerTileY = ([int](r.bounds.hi.y) + 1) / ([int](colors.bounds.hi.y) + 1)
  var elemsPerTileZ = ([int](r.bounds.hi.z) + 1) / ([int](colors.bounds.hi.z) + 1)

  var coloring = regentlib.c.legion_domain_point_coloring_create()

  for c in colors do
    var rect : rect3d
    if [int](c.z) % pow2level == 0 then
      rect = rect3d {
        lo = c * elemsPerTile,
        hi = (c + pow2level) * elemsPerTile - 1
      }
      regentlib.c.printf('  color %d: %d - %d\n', c, rect.lo, rect.hi)
    else
      -- create an empty rectangle
      rect = rect3d {
        lo = 1,
        hi = 0
      }
    end
    regentlib.c.legion_domain_point_coloring_color_domain(coloring, c, rect)
  end
  var p = partition(disjoint, r, coloring, colors)
  regentlib.c.legion_domain_point_coloring_destroy(coloring)
  return p
end





local task AllocateImage()
do
  var width = 3840
  var height = 2160
  var numLayers = 4
  var indices = ispace(int3d, (width, height, numLayers))
  imageRegion = region(indices, PixelFields)
end


local task PartitionImage(imageRegion : region(ispace(int3d), PixelFields))
do
  var fragmentsX = 1
  var fragmentsY = 1
  var colors = ispace(int3d, (fragmentsX, fragmentsY, numLayers))
  var partitionLevel1 = MakePartition(imageRegion, colors, 1, 2)
end


-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}

exports.Render = rquote
  for tile in tiles do
    Render(p_cells[tile], p_particles[tile])
  end
end

exports.Initialize = rquote
  AllocateImage()
  PartitionImage()
end

-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
