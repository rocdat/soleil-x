
import 'regent'

local A = require 'admiral'

local cviz
local link_flags
do
  local root_dir = arg[0]:match(".*/") or "./"
  assert(os.getenv('LG_RT_DIR'), "LG_RT_DIR should be set!")
  local runtime_dir = os.getenv('LG_RT_DIR') .. "/"
  local legion_dir = runtime_dir .. "legion/"
  local mapper_dir = runtime_dir .. "mappers/"
  local realm_dir = runtime_dir .. "realm/"
  local viz_cc = root_dir .. "viz.cc"
  if os.getenv('SAVEOBJ') == '1' then
    viz_so = root_dir .. "libviz.so"
    link_flags = terralib.newlist({"-L" .. root_dir, "-lviz"})
  else
    viz_so = os.tmpname() .. ".so"
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
                 " -I " .. realm_dir .. " " .. viz_cc .. " -o " .. viz_so)
  if os.execute(cmd) ~= 0 then
    print("Error: failed to compile " .. viz_cc)
    assert(false)
  end
  terralib.linklibrary(viz_so)
  cviz = terralib.includec("viz.h", {"-I", root_dir, "-I", runtime_dir,
                            "-I", mapper_dir, "-I", legion_dir,
                            "-I", realm_dir})
end


-------------------------------------------------------------------------------
-- Module parameters
-------------------------------------------------------------------------------

return function(cellsRel, particlesRel, xnum, ynum, znum, gridOrigin, gridWidth)

local p_cells   = cellsRel:primPartSymbol()
local cellsType = cellsRel:regionType()
local p_particles   = particlesRel:primPartSymbol()
local particlesType = particlesRel:regionType()
local tiles = A.primColors()

local width = 3840
local height = 2160
local numLayers = tiles.volume()
local fragmentsX = 1
local fragmentsY = 2
local fragmentsZ = numLayers
local fragmentWidth = (width / fragmentsX)
local fragmentHeight = (height / fragmentsY)
local numTilesX = tiles.hi.x - tiles.lo.x + 1
local numTilesY = tiles.hi.y - tiles.lo.y + 1
local numTilesZ = tiles.hi.z - tiles.lo.z + 1
local zero = terralib.constant(`int3d { __ptr = regentlib.__int3d { 0, 0, 0 } })
local one = terralib.constant(`int3d { __ptr = regentlib.__int3d { 1, 1, 1 } })


-- CODEGEN: local_imageFragmentX

-- CODEGEN: local_partitionFragmentXByDepth

-- CODEGEN: local_partitionFragmentXLeftRightChildren




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



--
-- SplitLeftRight:
--

local task SplitLeftRight(r : region(ispace(int3d), PixelFields),
  level : int,
  pow2Level : int)

  var colors = ispace(int3d, int3d{ 2, 2, 2 }) -- 0 = left, 1 = right
  var coloring = regentlib.c.legion_multi_domain_point_coloring_create()

  for i = 0, numLayers do
    var rect = rect3d {
      lo = { 0, 0, i }, hi = { fragmentWidth - 1, fragmentHeight - 1, i }
    }
    var color = zero
    if (i / pow2Level) % 2 == 1 then
      color = one
    end
    regentlib.c.legion_multi_domain_point_coloring_color_domain(coloring, color, rect)
  end

  var p = partition(disjoint, r, coloring, colors)
  regentlib.c.legion_multi_domain_point_coloring_destroy(coloring)
  return p
end




-- subsumption of partitions:
-- lvl3     0
--          |\
--          ...
--          |      \
-- lvl2     0       4
--          |\      |\
--          | \     | \
--          |  \    |  \
-- lvl1     0   2   4   6
--          |\  |\  |\  |\
-- lvl0     0 1 2 3 4 5 6 7



--
-- ChildPartition
--

local task ChildPartition(r : region(ispace(int3d), PixelFields),
  level : int,
  pow2Level : int,
  offset : int,
  tiles : ispace(int3d))

  var coloring = regentlib.c.legion_domain_point_coloring_create()

  for tile in tiles do
    var rect = rect3d{ lo = one, hi = zero }
    var z = tile.x + (tile.y * numTilesX) + (tile.z * numTilesX * numTilesY)
    if z < numLayers / (2 * pow2Level) then
      var layer = 2 * pow2Level * z + offset
      rect = rect3d{
        lo = int3d{ 0, 0, layer },
        hi = int3d{ fragmentWidth - 1, fragmentHeight - 1, layer }
      }
    end
    regentlib.c.legion_domain_point_coloring_color_domain(coloring, tile, rect)
  end

  var p = partition(disjoint, r, coloring, tiles)
  regentlib.c.legion_domain_point_coloring_destroy(coloring)
  return p
end







--
-- DepthPartition: partition the imageFragment by layers
--

local task DepthPartition(r : region(ispace(int3d), PixelFields),
  width : int,
  height : int,
  tiles : ispace(int3d))

  var coloring = regentlib.c.legion_domain_point_coloring_create()

  for tile in tiles do
    var z = tile.x + (tile.y * numTilesX) + (tile.z * numTilesX * numTilesY)
    var rect = rect3d {
      lo = { 0, 0, z },
      hi = { width - 1, height - 1, z }
    }
    regentlib.c.legion_domain_point_coloring_color_domain(coloring, tile, rect)
  end

  var p = partition(disjoint, r, coloring, tiles)
  regentlib.c.legion_domain_point_coloring_destroy(coloring)
  return p
end




--
-- Render: produce an RGBA and DEPTH buffer image, write the pixels into the imageFragment
--

local task Render(cells : cellsType,
  particles : particlesType,
  timeStepNumber : int,
-- CODEGEN: imageFragmentX_arglist
  )
where
  reads(cells.{centerCoordinates, velocity, temperature}),
  reads(particles.{__valid, cell, position, density, particle_temperature, tracking}),
-- CODEGEN: writes_imageFragmentX
do
  regentlib.c.printf("in local task Render\n")

  var numValid = 0
  for p in particles do
    if p.__valid then
      numValid = numValid + 1
    end
  end
  regentlib.c.printf("number of valid particles = %d\n", numValid)

  cviz.cxx_render(__runtime(), __context(),
    __physical(cells), __fields(cells),
    __physical(particles), __fields(particles),
-- CODEGEN: __physical_imageFragmentX__fieldsComma
    xnum, ynum, znum, timeStepNumber)
end




--
-- Reduce: reduce images to one
--

local task Reduce(treeLevel : int,
  offset : int,
  leftSubregion : region(ispace(int3d), PixelFields),
  rightSubregion : region(ispace(int3d), PixelFields))
where
   reads writes (leftSubregion), reads (rightSubregion)
do
  if leftSubregion.bounds.lo.x < leftSubregion.bounds.hi.x and
    rightSubregion.bounds.lo.x < rightSubregion.bounds.hi.x then
    cviz.cxx_reduce(__runtime(), __context(),
      __physical(leftSubregion), __fields(leftSubregion),
      __physical(rightSubregion), __fields(rightSubregion),
      treeLevel, offset)
  end
end



--
-- SaveImage
--

local task SaveImage(tile : int3d,
  timeStepNumber : int,
-- CODEGEN: imageFragmentX_arglist
)
where
-- CODEGEN: reads_imageFragmentX
do
  if tile.z == 0 and tile.y == 0 and tile.x == 0 then
    cviz.cxx_saveImage(__runtime(), __context(),
      width, height, timeStepNumber,
-- CODEGEN: __physical_imageFragmentX__fields
    )
  end
end





-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}

-- CODEGEN: InitializeFragmentX





exports.Render = function(timeStepNumber)
  return rquote
    for tile in tiles do
      Render(p_cells[tile], p_particles[tile], timeStepNumber,
-- CODEGEN: partitionFragmentXByDepth_argList
      )
    end
  end
end




exports.Reduce = function(timeStepNumber)
  return rquote

    __demand(__spmd) do

-- CODEGEN: tree_reductions


      -- save result to disk
      for tile in tiles do
        SaveImage(tile, timeStepNumber,
-- CODEGEN: partitionFragmentXByDepth_argList
      end

    end -- demand spmd
  end
end



-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
