
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

-------------------------------------------------------------------------------
-- Local tasks
-------------------------------------------------------------------------------

local task Render(cells : cellsType, particles : particlesType)
where
  reads(cells.{centerCoordinates, velocity, temperature}),
  reads(particles.{cell, position, density, particle_temperature})
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

-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}

exports.Render = rquote
  for tile in tiles do
    -- debug_particles(p_particles[tile])
    -- debug_cells(p_cells[tile])
    Render(p_cells[tile], p_particles[tile])
  end
end

-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
