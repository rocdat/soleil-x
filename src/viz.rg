
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

return function(cellsRel, particlesRel, xnum, ynum, znum, gridOrigin, gridWidth, imageRegion)

local p_cells   = cellsRel:primPartSymbol()
local cellsType = cellsRel:regionType()
local p_particles   = particlesRel:primPartSymbol()
local particlesType = particlesRel:regionType()
local tiles = A.primColors()

local width = 3840
local height = 2160
local numLayers = 4
local imageRegion = regentlib.newsymbol("imageRegion")
local partitionByDepth = regentlib.newsymbol("partitionByDepth")
local partitionLevel0 = regentlib.newsymbol("partitionLevel0")
local partitionLevel1 = regentlib.newsymbol("partitionLevel1")
local partitionLevel2 = regentlib.newsymbol("partitionLevel2")
local partitionLevel3 = regentlib.newsymbol("partitionLevel3")
local partitionLevel4 = regentlib.newsymbol("partitionLevel4")
local partitionLevel5 = regentlib.newsymbol("partitionLevel5")
local partitionLevel6 = regentlib.newsymbol("partitionLevel6")
local partitionLevel7 = regentlib.newsymbol("partitionLevel7")
local partitionLevel8 = regentlib.newsymbol("partitionLevel8")
local partitionLevel9 = regentlib.newsymbol("partitionLevel9")
local partitionLevel10 = regentlib.newsymbol("partitionLevel10")
local partitionLevel11 = regentlib.newsymbol("partitionLevel11")
local partitionLevel12 = regentlib.newsymbol("partitionLevel12")
local partitionLevel13 = regentlib.newsymbol("partitionLevel13")
local partitionLevel14 = regentlib.newsymbol("partitionLevel14")

local fspace PixelFields {
  R : float,
  G : float,
  B : float,
  A : float,
  Z : float,
  UserData : float
}


-------------------------------------------------------------------------------
-- Local tasks
-------------------------------------------------------------------------------

local task Render(cells : cellsType, particles : particlesType, imageRegion : region(ispace(int3d), PixelFields))
where
  reads(cells.{centerCoordinates, velocity, temperature}),
  reads(particles.{__valid, cell, position, density, particle_temperature, tracking}),
  writes(imageRegion.{R, G, B, A, Z, UserData})
do
  regentlib.c.printf('in task Render\n')
  crender.cxx_render(__runtime(), __context(),
                     __physical(cells), __fields(cells),
                     __physical(particles), __fields(particles),
                     __physical(imageRegion), __fields(imageRegion),
                     xnum, ynum, znum)
end

local task Composite()
end


local task MakePartition(r : region(ispace(int3d), PixelFields),
  colors : ispace(int3d),
  level : int,
  pow2level : int)

  regentlib.assert([int](colors.bounds.lo.x) == 0, '')
  regentlib.assert([int](colors.bounds.lo.y) == 0, '')
  regentlib.assert([int](colors.bounds.lo.z) == 0, '')

  regentlib.assert([int](r.bounds.lo.x) == 0, '')
  regentlib.assert([int](r.bounds.lo.y) == 0, '')
  regentlib.assert([int](r.bounds.lo.z) == 0, '')

  regentlib.assert(([int](r.bounds.hi.x) + 1) % ([int](colors.bounds.hi.x) + 1) == 0, '')
  regentlib.assert(([int](r.bounds.hi.y) + 1) % ([int](colors.bounds.hi.y) + 1) == 0, '')
  regentlib.assert(([int](r.bounds.hi.z) + 1) % ([int](colors.bounds.hi.z) + 1) == 0, '')

  var elementsPerTileX = ([int](r.bounds.hi.x) + 1) / ([int](colors.bounds.hi.x) + 1)
  var elementsPerTileY = ([int](r.bounds.hi.y) + 1) / ([int](colors.bounds.hi.y) + 1)
  var elementsPerTileZ = ([int](r.bounds.hi.z) + 1) / ([int](colors.bounds.hi.z) + 1)

  var coloring = regentlib.c.legion_domain_point_coloring_create()

  for c in colors do
    var rect : rect3d
    if [int](c.z) % pow2level == 0 then
      rect = rect3d {
        lo = int3d{c.x * elementsPerTileX, c.y * elementsPerTileY, c.z * elementsPerTileZ},
        hi = {(c.x + pow2level) * elementsPerTileX - 1,
          (c.y * pow2level) * elementsPerTileY - 1,
          (c.z * pow2level) * elementsPerTileZ - 1}
      }
    else
      -- create an empty rectangle
      rect = rect3d {
        lo = int3d{1, 1, 1},
        hi = int3d{0, 0, 0}
      }
    end
    regentlib.c.legion_domain_point_coloring_color_domain(coloring, c, rect)
  end
  var p = partition(disjoint, r, coloring, colors)
  regentlib.c.legion_domain_point_coloring_destroy(coloring)
  return p
end


local task DepthPartition(r : region(ispace(int3d), PixelFields),
  width : int,
  height : int)

  var coloring = regentlib.c.legion_domain_point_coloring_create()
  var colors = ispace(int3d, int3d{1, 1, 1})

  for c in colors do
    regentlib.c.printf("%f %f %f\n", c.x, c.y, c.z)
    var rect = rect3d {
      lo = { 0, 0, c.z },
      hi = { width - 1, height - 1, c.z }
    }
    regentlib.c.legion_domain_point_coloring_color_domain(coloring, c, rect)
  end
  var p = partition(disjoint, r, coloring, colors)
  regentlib.c.legion_domain_point_coloring_destroy(coloring)
  return p
end


-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}


exports.Initialize = rquote

  var indices = ispace(int3d, int3d{width, height, numLayers})
  var [imageRegion] = region(indices, PixelFields)
  regentlib.c.printf("width %d height %d\n", width, height)
  var [partitionByDepth] = DepthPartition([imageRegion], width, height)
  -- var fragmentsX = 1
  -- var fragmentsY = 1
  -- var colors = ispace(int3d, int3d{fragmentsX, fragmentsY, numLayers})
  -- var [partitionLevel0] = MakePartition([imageRegion], colors, 0, 1)
  -- var [partitionLevel1] = MakePartition([imageRegion], colors, 1, 2)
  -- var [partitionLevel2] = MakePartition([imageRegion], colors, 2, 4)
  -- var [partitionLevel3] = MakePartition([imageRegion], colors, 3, 8)
  -- var [partitionLevel4] = MakePartition([imageRegion], colors, 4, 16)
  -- var [partitionLevel5] = MakePartition([imageRegion], colors, 5, 32)
  -- var [partitionLevel6] = MakePartition([imageRegion], colors, 6, 64)
  -- var [partitionLevel7] = MakePartition([imageRegion], colors, 7, 128)
  -- var [partitionLevel8] = MakePartition([imageRegion], colors, 8, 256)
  -- var [partitionLevel9] = MakePartition([imageRegion], colors, 9, 512)
  -- var [partitionLevel10] = MakePartition([imageRegion], colors, 10, 1024)
  -- var [partitionLevel11] = MakePartition([imageRegion], colors, 11, 2048)
  -- var [partitionLevel12] = MakePartition([imageRegion], colors, 12, 4096)
  -- var [partitionLevel13] = MakePartition([imageRegion], colors, 13, 8192)
  -- var [partitionLevel14] = MakePartition([imageRegion], colors, 14, 16384)
end


exports.Render = rquote
  regentlib.c.printf('Render\n')
  for tile in tiles do
    -- Render(p_cells[tile], p_particles[tile], [imageRegion])
    --Render(p_cells[tile], p_particles[tile])
    Render(p_cells[tile], p_particles[tile], [partitionByDepth][tile])
  end
  regentlib.c.printf('end Render\n')
end


exports.Composite = rquote
  for tile in tiles do
    -- Composite(partitionLevel0[tile])
  end
end

-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
