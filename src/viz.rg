
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

  local cxx_flags = "-O2 -Wall -Werror -ftree-vectorize"
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

-- CODEGEN: local_module_declarations
local my = {}

return function(cellsRel, particlesRel, xnum, ynum, znum, gridOrigin, gridWidth)

local p_cells   = cellsRel:primPartSymbol()
local cellsType = cellsRel:regionType()
local p_particles   = particlesRel:primPartSymbol()
local particlesType = particlesRel:regionType()
local tiles = A.primColors()

local width = 3840
local height = 2160
local numFragments = 2
local fragmentWidth = width 
local fragmentHeight = (height / numFragments)
local zero = terralib.constant(`int3d { __ptr = regentlib.__int3d { 0, 0, 0 } })
local one = terralib.constant(`int3d { __ptr = regentlib.__int3d { 1, 1, 1 } })


-- CODEGEN: local_imageFragmentX
my.indices = regentlib.newsymbol("indices")
my.timeStep = regentlib.newsymbol("timeStep")
my.imageFragment0 = regentlib.newsymbol("imageFragment0")
my.imageFragment1 = regentlib.newsymbol("imageFragment1")
my.imageFragment2 = regentlib.newsymbol("imageFragment2")
my.imageFragment3 = regentlib.newsymbol("imageFragment3")

-- CODEGEN: local_partitionFragmentXByDepth
my.partitionFragment0ByDepth = regentlib.newsymbol("partitionFragment0ByDepth")
my.partitionFragment1ByDepth = regentlib.newsymbol("partitionFragment1ByDepth")
my.partitionFragment2ByDepth = regentlib.newsymbol("partitionFragment2ByDepth")
my.partitionFragment3ByDepth = regentlib.newsymbol("partitionFragment3ByDepth")

-- CODEGEN: local_partitionFragmentXLeftRightChildren
-- partitions for fragment 0
my.partitionFragment0LeftRightLevel0 = regentlib.newsymbol("partitionFragment0LeftRightLevel0")
my.partitionFragment0LeftChildLevel0 = regentlib.newsymbol("partitionFragment0LeftChildLevel0")
my.partitionFragment0RightChildLevel0 = regentlib.newsymbol("partitionFragment0RightChildLevel0")
my.partitionFragment0LeftRightLevel1 = regentlib.newsymbol("partitionFragment0LeftRightLevel1")
my.partitionFragment0LeftChildLevel1 = regentlib.newsymbol("partitionFragment0LeftChildLevel1")
my.partitionFragment0RightChildLevel1 = regentlib.newsymbol("partitionFragment0RightChildLevel1")
my.partitionFragment0LeftRightLevel2 = regentlib.newsymbol("partitionFragment0LeftRightLevel2")
my.partitionFragment0LeftChildLevel2 = regentlib.newsymbol("partitionFragment0LeftChildLevel2")
my.partitionFragment0RightChildLevel2 = regentlib.newsymbol("partitionFragment0RightChildLevel2")
-- partitions for fragment 1
my.partitionFragment1LeftRightLevel0 = regentlib.newsymbol("partitionFragment1LeftRightLevel0")
my.partitionFragment1LeftChildLevel0 = regentlib.newsymbol("partitionFragment1LeftChildLevel0")
my.partitionFragment1RightChildLevel0 = regentlib.newsymbol("partitionFragment1RightChildLevel0")
my.partitionFragment1LeftRightLevel1 = regentlib.newsymbol("partitionFragment1LeftRightLevel1")
my.partitionFragment1LeftChildLevel1 = regentlib.newsymbol("partitionFragment1LeftChildLevel1")
my.partitionFragment1RightChildLevel1 = regentlib.newsymbol("partitionFragment1RightChildLevel1")
my.partitionFragment1LeftRightLevel2 = regentlib.newsymbol("partitionFragment1LeftRightLevel2")
my.partitionFragment1LeftChildLevel2 = regentlib.newsymbol("partitionFragment1LeftChildLevel2")
my.partitionFragment1RightChildLevel2 = regentlib.newsymbol("partitionFragment1RightChildLevel2")
-- partitions for fragment 2
my.partitionFragment2LeftRightLevel0 = regentlib.newsymbol("partitionFragment2LeftRightLevel0")
my.partitionFragment2LeftChildLevel0 = regentlib.newsymbol("partitionFragment2LeftChildLevel0")
my.partitionFragment2RightChildLevel0 = regentlib.newsymbol("partitionFragment2RightChildLevel0")
my.partitionFragment2LeftRightLevel1 = regentlib.newsymbol("partitionFragment2LeftRightLevel1")
my.partitionFragment2LeftChildLevel1 = regentlib.newsymbol("partitionFragment2LeftChildLevel1")
my.partitionFragment2RightChildLevel1 = regentlib.newsymbol("partitionFragment2RightChildLevel1")
my.partitionFragment2LeftRightLevel2 = regentlib.newsymbol("partitionFragment2LeftRightLevel2")
my.partitionFragment2LeftChildLevel2 = regentlib.newsymbol("partitionFragment2LeftChildLevel2")
my.partitionFragment2RightChildLevel2 = regentlib.newsymbol("partitionFragment2RightChildLevel2")
-- partitions for fragment 3
my.partitionFragment3LeftRightLevel0 = regentlib.newsymbol("partitionFragment3LeftRightLevel0")
my.partitionFragment3LeftChildLevel0 = regentlib.newsymbol("partitionFragment3LeftChildLevel0")
my.partitionFragment3RightChildLevel0 = regentlib.newsymbol("partitionFragment3RightChildLevel0")
my.partitionFragment3LeftRightLevel1 = regentlib.newsymbol("partitionFragment3LeftRightLevel1")
my.partitionFragment3LeftChildLevel1 = regentlib.newsymbol("partitionFragment3LeftChildLevel1")
my.partitionFragment3RightChildLevel1 = regentlib.newsymbol("partitionFragment3RightChildLevel1")
my.partitionFragment3LeftRightLevel2 = regentlib.newsymbol("partitionFragment3LeftRightLevel2")
my.partitionFragment3LeftChildLevel2 = regentlib.newsymbol("partitionFragment3LeftChildLevel2")
my.partitionFragment3RightChildLevel2 = regentlib.newsymbol("partitionFragment3RightChildLevel2")


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
  pow2Level : int,
  tiles : ispace(int3d))

  var colors = ispace(int1d, int1d{ 2 }) -- 0 = left, 1 = right
  var coloring = regentlib.c.legion_multi_domain_point_coloring_create()
  var numNodes = tiles.volume

  for i = 0, numNodes do
    var rect = rect3d {
      lo = { 0, 0, i }, hi = { fragmentWidth - 1, fragmentHeight - 1, i }
    }
    var color = int1d{ 0 }
    if (i / pow2Level) % 2 == 1 then
      color = int1d{ 1 }
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
  var numNodes = tiles.volume
  var numTilesX = tiles.bounds.hi.x - tiles.bounds.lo.x + 1
  var numTilesY = tiles.bounds.hi.y - tiles.bounds.lo.y + 1

  for tile in tiles do
    var z = tile.x + (tile.y * numTilesX) + (tile.z * numTilesX * numTilesY)
    var rect = rect3d{ lo = one, hi = zero }
    if z % (2 * pow2Level) == 0 then
      var layer = z + offset
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
  var numTilesX = 2
  var numTilesY = 2
  var numTilesZ = 1

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
  timeStep : int,
-- CODEGEN: imageFragmentX_arglist
  imageFragment0 : region(ispace(int3d), PixelFields),
  imageFragment1 : region(ispace(int3d), PixelFields),
  imageFragment2 : region(ispace(int3d), PixelFields),
  imageFragment3 : region(ispace(int3d), PixelFields)
  )
where
  reads(cells.{centerCoordinates, velocity, temperature}),
  reads(particles.{__valid, cell, position, density, particle_temperature, tracking}),
-- CODEGEN: writes_imageFragmentX
  writes(imageFragment0),
  writes(imageFragment1),
  writes(imageFragment2),
  writes(imageFragment3)
do
  regentlib.c.printf("in local task Render\n")

  var numValid = 0
  for p in particles do
    if p.__valid then
      numValid = numValid + 1
    end
  end
  regentlib.c.printf("number of valid particles = %d\n", numValid)

  cviz.cxx_render(__runtime(),
    __physical(cells), __fields(cells),
    __physical(particles), __fields(particles),
-- CODEGEN: __physical_imageFragmentX__fieldsComma
    __physical(imageFragment0), __fields(imageFragment0),
    __physical(imageFragment1), __fields(imageFragment1),
    __physical(imageFragment2), __fields(imageFragment2),
    __physical(imageFragment3), __fields(imageFragment3),
    xnum, ynum, znum, timeStep)
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
  cviz.cxx_reduce(__runtime(),
    __physical(leftSubregion), __fields(leftSubregion),
    __physical(rightSubregion), __fields(rightSubregion),
    treeLevel, offset)
end


--
-- Empty: empty task
--

local task NullTask(
  leftSubregion : region(ispace(int3d), PixelFields)
)
where
  reads (leftSubregion)
do
  cviz.cxx_empty()
end



--
-- SaveImage
--

local task SaveImage(tile : int3d,
  timeStep : int,
-- CODEGEN: imageFragmentX_arglist
  imageFragment0 : region(ispace(int3d), PixelFields),
  imageFragment1 : region(ispace(int3d), PixelFields),
  imageFragment2 : region(ispace(int3d), PixelFields),
  imageFragment3 : region(ispace(int3d), PixelFields)
)
where
-- CODEGEN: reads_imageFragmentX
  reads(imageFragment0),
  reads(imageFragment1),
  reads(imageFragment2),
  reads(imageFragment3)
do
  if tile.z == 0 and tile.y == 0 and tile.x == 0 then
    regentlib.c.printf("Save Image timeStep %d\n", timeStep)
    cviz.cxx_saveImage(__runtime(),
      width, height, timeStep,
-- CODEGEN: __physical_imageFragmentX__fields
    __physical(imageFragment0), __fields(imageFragment0),
    __physical(imageFragment1), __fields(imageFragment1),
    __physical(imageFragment2), __fields(imageFragment2),
    __physical(imageFragment3), __fields(imageFragment3)
    )
  end
end




-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}

-- CODEGEN: InitializeFragmentX

exports.InitializeFragment0 = rquote

  var numLayers = tiles.volume
  var [my.timeStep] = 0
  var [my.indices] = ispace(int3d, int3d{ fragmentWidth, fragmentHeight, numLayers })
  var [my.imageFragment0] = region([my.indices], PixelFields)
  var [my.partitionFragment0ByDepth] = DepthPartition([my.imageFragment0], fragmentWidth, fragmentHeight, tiles)
  var [my.partitionFragment0LeftRightLevel0] = SplitLeftRight([my.imageFragment0], 0, 1, tiles)
  var [my.partitionFragment0LeftChildLevel0] = ChildPartition([my.partitionFragment0LeftRightLevel0][0], 0, 1, 0, tiles)
  var [my.partitionFragment0RightChildLevel0] = ChildPartition([my.partitionFragment0LeftRightLevel0][1], 0, 1, 1, tiles)
  var [my.partitionFragment0LeftRightLevel1] = SplitLeftRight([my.imageFragment0], 0, 2, tiles)
  var [my.partitionFragment0LeftChildLevel1] = ChildPartition([my.partitionFragment0LeftRightLevel1][0], 1, 2, 0, tiles)
  var [my.partitionFragment0RightChildLevel1] = ChildPartition([my.partitionFragment0LeftRightLevel1][1], 1, 2, 2, tiles)
  var [my.partitionFragment0LeftRightLevel2] = SplitLeftRight([my.imageFragment0], 0, 4, tiles)
  var [my.partitionFragment0LeftChildLevel2] = ChildPartition([my.partitionFragment0LeftRightLevel2][0], 2, 4, 0, tiles)
  var [my.partitionFragment0RightChildLevel2] = ChildPartition([my.partitionFragment0LeftRightLevel2][1], 2, 4, 4, tiles)

end

exports.InitializeFragment1 = rquote

  var [my.imageFragment1] = region([my.indices], PixelFields)
  var [my.partitionFragment1ByDepth] = DepthPartition([my.imageFragment1], fragmentWidth, fragmentHeight, tiles)
  var [my.partitionFragment1LeftRightLevel0] = SplitLeftRight([my.imageFragment1], 1, 1, tiles)
  var [my.partitionFragment1LeftChildLevel0] = ChildPartition([my.partitionFragment1LeftRightLevel0][0], 0, 1, 0, tiles)
  var [my.partitionFragment1RightChildLevel0] = ChildPartition([my.partitionFragment1LeftRightLevel0][1], 0, 1, 1, tiles)
  var [my.partitionFragment1LeftRightLevel1] = SplitLeftRight([my.imageFragment1], 1, 2, tiles)
  var [my.partitionFragment1LeftChildLevel1] = ChildPartition([my.partitionFragment1LeftRightLevel1][0], 1, 2, 0, tiles)
  var [my.partitionFragment1RightChildLevel1] = ChildPartition([my.partitionFragment1LeftRightLevel1][1], 1, 2, 2, tiles)
  var [my.partitionFragment1LeftRightLevel2] = SplitLeftRight([my.imageFragment1], 1, 4, tiles)
  var [my.partitionFragment1LeftChildLevel2] = ChildPartition([my.partitionFragment1LeftRightLevel2][0], 2, 4, 0, tiles)
  var [my.partitionFragment1RightChildLevel2] = ChildPartition([my.partitionFragment1LeftRightLevel2][1], 2, 4, 4, tiles)

end

exports.InitializeFragment2 = rquote

  var [my.imageFragment2] = region([my.indices], PixelFields)
  var [my.partitionFragment2ByDepth] = DepthPartition([my.imageFragment2], fragmentWidth, fragmentHeight, tiles)
  var [my.partitionFragment2LeftRightLevel0] = SplitLeftRight([my.imageFragment2], 2, 1, tiles)
  var [my.partitionFragment2LeftChildLevel0] = ChildPartition([my.partitionFragment2LeftRightLevel0][0], 0, 1, 0, tiles)
  var [my.partitionFragment2RightChildLevel0] = ChildPartition([my.partitionFragment2LeftRightLevel0][1], 0, 1, 1, tiles)
  var [my.partitionFragment2LeftRightLevel1] = SplitLeftRight([my.imageFragment2], 2, 2, tiles)
  var [my.partitionFragment2LeftChildLevel1] = ChildPartition([my.partitionFragment2LeftRightLevel1][0], 1, 2, 0, tiles)
  var [my.partitionFragment2RightChildLevel1] = ChildPartition([my.partitionFragment2LeftRightLevel1][1], 1, 2, 2, tiles)
  var [my.partitionFragment2LeftRightLevel2] = SplitLeftRight([my.imageFragment2], 2, 4, tiles)
  var [my.partitionFragment2LeftChildLevel2] = ChildPartition([my.partitionFragment2LeftRightLevel2][0], 2, 4, 0, tiles)
  var [my.partitionFragment2RightChildLevel2] = ChildPartition([my.partitionFragment2LeftRightLevel2][1], 2, 4, 4, tiles)

end

exports.InitializeFragment3 = rquote

  var [my.imageFragment3] = region([my.indices], PixelFields)
  var [my.partitionFragment3ByDepth] = DepthPartition([my.imageFragment3], fragmentWidth, fragmentHeight, tiles)
  var [my.partitionFragment3LeftRightLevel0] = SplitLeftRight([my.imageFragment3], 3, 1, tiles)
  var [my.partitionFragment3LeftChildLevel0] = ChildPartition([my.partitionFragment3LeftRightLevel0][0], 0, 1, 0, tiles)
  var [my.partitionFragment3RightChildLevel0] = ChildPartition([my.partitionFragment3LeftRightLevel0][1], 0, 1, 1, tiles)
  var [my.partitionFragment3LeftRightLevel1] = SplitLeftRight([my.imageFragment3], 3, 2, tiles)
  var [my.partitionFragment3LeftChildLevel1] = ChildPartition([my.partitionFragment3LeftRightLevel1][0], 1, 2, 0, tiles)
  var [my.partitionFragment3RightChildLevel1] = ChildPartition([my.partitionFragment3LeftRightLevel1][1], 1, 2, 2, tiles)
  var [my.partitionFragment3LeftRightLevel2] = SplitLeftRight([my.imageFragment3], 3, 4, tiles)
  var [my.partitionFragment3LeftChildLevel2] = ChildPartition([my.partitionFragment3LeftRightLevel2][0], 2, 4, 0, tiles)
  var [my.partitionFragment3RightChildLevel2] = ChildPartition([my.partitionFragment3LeftRightLevel2][1], 2, 4, 4, tiles)

end



exports.Render = function()
  return rquote
    for tile in tiles do
      Render(p_cells[tile], p_particles[tile], [my.timeStep],
-- CODEGEN: partitionFragmentXByDepth_argList
        [my.partitionFragment0ByDepth][tile],
        [my.partitionFragment1ByDepth][tile],
        [my.partitionFragment2ByDepth][tile],
        [my.partitionFragment3ByDepth][tile]
      )
    end
  end
end




exports.Reduce = function()
  return rquote

-- CODEGEN: tree_reductions

-- tree level 0

    for tile in tiles do
      Reduce(0, 1, [my.partitionFragment0LeftChildLevel0][tile], [my.partitionFragment0RightChildLevel0][tile])
    end
    for tile in tiles do
      Reduce(0, 1, [my.partitionFragment1LeftChildLevel0][tile], [my.partitionFragment1RightChildLevel0][tile])
    end
    for tile in tiles do
      Reduce(0, 1, [my.partitionFragment2LeftChildLevel0][tile], [my.partitionFragment2RightChildLevel0][tile])
    end
    for tile in tiles do
      Reduce(0, 1, [my.partitionFragment3LeftChildLevel0][tile], [my.partitionFragment3RightChildLevel0][tile])
    end

-- tree level 1

    for tile in tiles do
      Reduce(1, 2, [my.partitionFragment0LeftChildLevel1][tile], [my.partitionFragment0RightChildLevel1][tile])
    end
    for tile in tiles do
      Reduce(1, 2, [my.partitionFragment1LeftChildLevel1][tile], [my.partitionFragment1RightChildLevel1][tile])
    end
    for tile in tiles do
      Reduce(1, 2, [my.partitionFragment2LeftChildLevel1][tile], [my.partitionFragment2RightChildLevel1][tile])
    end
    for tile in tiles do
      Reduce(1, 2, [my.partitionFragment3LeftChildLevel1][tile], [my.partitionFragment3RightChildLevel1][tile])
    end

-- tree level 2

    for tile in tiles do
      Reduce(2, 4, [my.partitionFragment0LeftChildLevel2][tile], [my.partitionFragment0RightChildLevel2][tile])
    end
    for tile in tiles do
      Reduce(2, 4, [my.partitionFragment1LeftChildLevel2][tile], [my.partitionFragment1RightChildLevel2][tile])
    end
    for tile in tiles do
      Reduce(2, 4, [my.partitionFragment2LeftChildLevel2][tile], [my.partitionFragment2RightChildLevel2][tile])
    end
    for tile in tiles do
      Reduce(2, 4, [my.partitionFragment3LeftChildLevel2][tile], [my.partitionFragment3RightChildLevel2][tile])
    end

    -- save result to disk
    for tile in tiles do
      SaveImage(tile, [my.timeStep],
-- CODEGEN: partitionFragmentXByDepth_argList
        [my.partitionFragment0ByDepth][tile],
        [my.partitionFragment1ByDepth][tile],
        [my.partitionFragment2ByDepth][tile],
        [my.partitionFragment3ByDepth][tile]
      )
    end
    [my.timeStep] = [my.timeStep] + 1

  end
end



-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
