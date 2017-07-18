
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
local numLayers = 4
local fragmentsX = 1
local fragmentsY = 2
local fragmentsZ = numLayers
local fragmentWidth = (width / fragmentsX)
local fragmentHeight = (height / fragmentsY)
local numTilesX = 2
local numTilesY = 2
local numTilesZ = 1
local zero = terralib.constant(`int3d { __ptr = regentlib.__int3d { 0, 0, 0 } })
local one = terralib.constant(`int3d { __ptr = regentlib.__int3d { 1, 1, 1 } })


-- image fragments

local indices = regentlib.newsymbol("indices")
local imageFragment0 = regentlib.newsymbol("imageFragment0")
local imageFragment1 = regentlib.newsymbol("imageFragment1")
--etc

-- depth partitions

local partitionFragment0ByDepth = regentlib.newsymbol("partitionFragment0ByDepth")
local partitionFragment1ByDepth = regentlib.newsymbol("partitionFragment1ByDepth")
-- etc


-- partitions for fragment0

local partitionFragment0LeftRightLevel0 = regentlib.newsymbol("partitionFragment0LeftRightLevel0")
local partitionFragment0LeftRightLevel1 = regentlib.newsymbol("partitionFragment0LeftRightLevel1")
local partitionFragment0LeftRightLevel2 = regentlib.newsymbol("partitionFragment0LeftRightLevel2")
local partitionFragment0LeftRightLevel3 = regentlib.newsymbol("partitionFragment0LeftRightLevel3")
local partitionFragment0LeftRightLevel4 = regentlib.newsymbol("partitionFragment0LeftRightLevel4")
local partitionFragment0LeftRightLevel5 = regentlib.newsymbol("partitionFragment0LeftRightLevel5")
local partitionFragment0LeftRightLevel6 = regentlib.newsymbol("partitionFragment0LeftRightLevel6")
local partitionFragment0LeftRightLevel7 = regentlib.newsymbol("partitionFragment0LeftRightLevel7")
local partitionFragment0LeftRightLevel8 = regentlib.newsymbol("partitionFragment0LeftRightLevel8")
local partitionFragment0LeftRightLevel9 = regentlib.newsymbol("partitionFragment0LeftRightLevel9")
local partitionFragment0LeftRightLevel10 = regentlib.newsymbol("partitionFragment0LeftRightLevel10")

local partitionFragment0LeftChildLevel0 = regentlib.newsymbol("partitionFragment0LeftChildLevel0")
local partitionFragment0LeftChildLevel1 = regentlib.newsymbol("partitionFragment0LeftChildLevel1")
local partitionFragment0LeftChildLevel2 = regentlib.newsymbol("partitionFragment0LeftChildLevel2")
local partitionFragment0LeftChildLevel3 = regentlib.newsymbol("partitionFragment0LeftChildLevel3")
local partitionFragment0LeftChildLevel4 = regentlib.newsymbol("partitionFragment0LeftChildLevel4")
local partitionFragment0LeftChildLevel5 = regentlib.newsymbol("partitionFragment0LeftChildLevel5")
local partitionFragment0LeftChildLevel6 = regentlib.newsymbol("partitionFragment0LeftChildLevel6")
local partitionFragment0LeftChildLevel7 = regentlib.newsymbol("partitionFragment0LeftChildLevel7")
local partitionFragment0LeftChildLevel8 = regentlib.newsymbol("partitionFragment0LeftChildLevel8")
local partitionFragment0LeftChildLevel9 = regentlib.newsymbol("partitionFragment0LeftChildLevel9")
local partitionFragment0LeftChildLevel10 = regentlib.newsymbol("partitionFragment0LeftChildLevel10")
local partitionFragment0LeftChildLevel11 = regentlib.newsymbol("partitionFragment0LeftChildLevel11")
local partitionFragment0LeftChildLevel12 = regentlib.newsymbol("partitionFragment0LeftChildLevel12")
local partitionFragment0LeftChildLevel13 = regentlib.newsymbol("partitionFragment0LeftChildLevel13")
local partitionFragment0LeftChildLevel14 = regentlib.newsymbol("partitionFragment0LeftChildLevel14")

local partitionFragment0RightChildLevel0 = regentlib.newsymbol("partitionFragment0RightChildLevel0")
local partitionFragment0RightChildLevel1 = regentlib.newsymbol("partitionFragment0RightChildLevel1")
local partitionFragment0RightChildLevel2 = regentlib.newsymbol("partitionFragment0RightChildLevel2")
local partitionFragment0RightChildLevel3 = regentlib.newsymbol("partitionFragment0RightChildLevel3")
local partitionFragment0RightChildLevel4 = regentlib.newsymbol("partitionFragment0RightChildLevel4")
local partitionFragment0RightChildLevel5 = regentlib.newsymbol("partitionFragment0RightChildLevel5")
local partitionFragment0RightChildLevel6 = regentlib.newsymbol("partitionFragment0RightChildLevel6")
local partitionFragment0RightChildLevel7 = regentlib.newsymbol("partitionFragment0RightChildLevel7")
local partitionFragment0RightChildLevel8 = regentlib.newsymbol("partitionFragment0RightChildLevel8")
local partitionFragment0RightChildLevel9 = regentlib.newsymbol("partitionFragment0RightChildLevel9")
local partitionFragment0RightChildLevel10 = regentlib.newsymbol("partitionFragment0RightChildLevel10")
local partitionFragment0RightChildLevel11 = regentlib.newsymbol("partitionFragment0RightChildLevel11")
local partitionFragment0RightChildLevel12 = regentlib.newsymbol("partitionFragment0RightChildLevel12")
local partitionFragment0RightChildLevel13 = regentlib.newsymbol("partitionFragment0RightChildLevel13")
local partitionFragment0RightChildLevel14 = regentlib.newsymbol("partitionFragment0RightChildLevel14")


-- partitions for fragment1

local partitionFragment1LeftRightLevel0 = regentlib.newsymbol("partitionFragment1LeftRightLevel0")
local partitionFragment1LeftRightLevel1 = regentlib.newsymbol("partitionFragment1LeftRightLevel1")
local partitionFragment1LeftRightLevel2 = regentlib.newsymbol("partitionFragment1LeftRightLevel2")
local partitionFragment1LeftRightLevel3 = regentlib.newsymbol("partitionFragment1LeftRightLevel3")
local partitionFragment1LeftRightLevel4 = regentlib.newsymbol("partitionFragment1LeftRightLevel4")
local partitionFragment1LeftRightLevel5 = regentlib.newsymbol("partitionFragment1LeftRightLevel5")
local partitionFragment1LeftRightLevel6 = regentlib.newsymbol("partitionFragment1LeftRightLevel6")
local partitionFragment1LeftRightLevel7 = regentlib.newsymbol("partitionFragment1LeftRightLevel7")
local partitionFragment1LeftRightLevel8 = regentlib.newsymbol("partitionFragment1LeftRightLevel8")
local partitionFragment1LeftRightLevel9 = regentlib.newsymbol("partitionFragment1LeftRightLevel9")
local partitionFragment1LeftRightLevel10 = regentlib.newsymbol("partitionFragment1LeftRightLevel10")

local partitionFragment1LeftChildLevel0 = regentlib.newsymbol("partitionFragment1LeftChildLevel0")
local partitionFragment1LeftChildLevel1 = regentlib.newsymbol("partitionFragment1LeftChildLevel1")
local partitionFragment1LeftChildLevel2 = regentlib.newsymbol("partitionFragment1LeftChildLevel2")
local partitionFragment1LeftChildLevel3 = regentlib.newsymbol("partitionFragment1LeftChildLevel3")
local partitionFragment1LeftChildLevel4 = regentlib.newsymbol("partitionFragment1LeftChildLevel4")
local partitionFragment1LeftChildLevel5 = regentlib.newsymbol("partitionFragment1LeftChildLevel5")
local partitionFragment1LeftChildLevel6 = regentlib.newsymbol("partitionFragment1LeftChildLevel6")
local partitionFragment1LeftChildLevel7 = regentlib.newsymbol("partitionFragment1LeftChildLevel7")
local partitionFragment1LeftChildLevel8 = regentlib.newsymbol("partitionFragment1LeftChildLevel8")
local partitionFragment1LeftChildLevel9 = regentlib.newsymbol("partitionFragment1LeftChildLevel9")
local partitionFragment1LeftChildLevel10 = regentlib.newsymbol("partitionFragment1LeftChildLevel10")
local partitionFragment1LeftChildLevel11 = regentlib.newsymbol("partitionFragment1LeftChildLevel11")
local partitionFragment1LeftChildLevel12 = regentlib.newsymbol("partitionFragment1LeftChildLevel12")
local partitionFragment1LeftChildLevel13 = regentlib.newsymbol("partitionFragment1LeftChildLevel13")
local partitionFragment1LeftChildLevel14 = regentlib.newsymbol("partitionFragment1LeftChildLevel14")

local partitionFragment1RightChildLevel0 = regentlib.newsymbol("partitionFragment1RightChildLevel0")
local partitionFragment1RightChildLevel1 = regentlib.newsymbol("partitionFragment1RightChildLevel1")
local partitionFragment1RightChildLevel2 = regentlib.newsymbol("partitionFragment1RightChildLevel2")
local partitionFragment1RightChildLevel3 = regentlib.newsymbol("partitionFragment1RightChildLevel3")
local partitionFragment1RightChildLevel4 = regentlib.newsymbol("partitionFragment1RightChildLevel4")
local partitionFragment1RightChildLevel5 = regentlib.newsymbol("partitionFragment1RightChildLevel5")
local partitionFragment1RightChildLevel6 = regentlib.newsymbol("partitionFragment1RightChildLevel6")
local partitionFragment1RightChildLevel7 = regentlib.newsymbol("partitionFragment1RightChildLevel7")
local partitionFragment1RightChildLevel8 = regentlib.newsymbol("partitionFragment1RightChildLevel8")
local partitionFragment1RightChildLevel9 = regentlib.newsymbol("partitionFragment1RightChildLevel9")
local partitionFragment1RightChildLevel10 = regentlib.newsymbol("partitionFragment1RightChildLevel10")
local partitionFragment1RightChildLevel11 = regentlib.newsymbol("partitionFragment1RightChildLevel11")
local partitionFragment1RightChildLevel12 = regentlib.newsymbol("partitionFragment1RightChildLevel12")
local partitionFragment1RightChildLevel13 = regentlib.newsymbol("partitionFragment1RightChildLevel13")
local partitionFragment1RightChildLevel14 = regentlib.newsymbol("partitionFragment1RightChildLevel14")

-- etc for more fragments

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
  imageFragment0 : region(ispace(int3d), PixelFields),
  imageFragment1 : region(ispace(int3d), PixelFields)
-- etc more fragments up to 1000
  )
where
  reads(cells.{centerCoordinates, velocity, temperature}),
  reads(particles.{__valid, cell, position, density, particle_temperature, tracking}),
  writes(imageFragment0),
  writes(imageFragment1)
-- etc more fragments here
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
    __physical(imageFragment0), __fields(imageFragment0),
    __physical(imageFragment1), __fields(imageFragment1),
-- etc here for more fragments
    xnum, ynum, znum)
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
  imageFragment0 : region(ispace(int3d), PixelFields),
  imageFragment1 : region(ispace(int3d), PixelFields)
)
-- etc for more fragments
where
  reads(imageFragment0),
  reads(imageFragment1)
-- etc for more fragments
do
  if tile.z == 0 and tile.y == 0 and tile.x == 0 then
    cviz.cxx_saveImage(__runtime(), __context(),
      width, height,
      __physical(imageFragment0), __fields(imageFragment0),
      __physical(imageFragment1), __fields(imageFragment1)
    )
-- etc for more fragments
  end
end





-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}

exports.InitializeFragment0 = rquote
  var [indices] = ispace(int3d, int3d{ fragmentWidth, fragmentHeight, numLayers })
  var [imageFragment0] = region([indices], PixelFields)
  var [partitionFragment0ByDepth] = DepthPartition([imageFragment0], fragmentWidth, fragmentHeight, tiles)

  var [partitionFragment0LeftRightLevel0] = SplitLeftRight([imageFragment0], 0, 1)
  var [partitionFragment0LeftRightLevel1] = SplitLeftRight([imageFragment0], 1, 2)
  var [partitionFragment0LeftRightLevel2] = SplitLeftRight([imageFragment0], 2, 4)
  var [partitionFragment0LeftRightLevel3] = SplitLeftRight([imageFragment0], 3, 8)
  var [partitionFragment0LeftRightLevel4] = SplitLeftRight([imageFragment0], 4, 16)
  var [partitionFragment0LeftRightLevel5] = SplitLeftRight([imageFragment0], 5, 32)
  var [partitionFragment0LeftRightLevel6] = SplitLeftRight([imageFragment0], 6, 64)
  var [partitionFragment0LeftRightLevel7] = SplitLeftRight([imageFragment0], 7, 128)
  var [partitionFragment0LeftRightLevel8] = SplitLeftRight([imageFragment0], 8, 256)
  var [partitionFragment0LeftRightLevel9] = SplitLeftRight([imageFragment0], 9, 512)
  var [partitionFragment0LeftRightLevel10] = SplitLeftRight([imageFragment0], 10, 1024)

  var [partitionFragment0LeftChildLevel0] = ChildPartition([partitionFragment0LeftRightLevel0][zero], 0, 1, 0, tiles)
  var [partitionFragment0RightChildLevel0] = ChildPartition([partitionFragment0LeftRightLevel0][one], 0, 1, 1, tiles)
  var [partitionFragment0LeftChildLevel1] = ChildPartition([partitionFragment0LeftRightLevel1][zero], 1, 2, 0, tiles)
  var [partitionFragment0RightChildLevel1] = ChildPartition([partitionFragment0LeftRightLevel1][one], 1, 2, 2, tiles)
  var [partitionFragment0LeftChildLevel2] = ChildPartition([partitionFragment0LeftRightLevel2][zero], 2, 4, 0, tiles)
  var [partitionFragment0RightChildLevel2] = ChildPartition([partitionFragment0LeftRightLevel2][one], 2, 4, 4, tiles)
  var [partitionFragment0LeftChildLevel3] = ChildPartition([partitionFragment0LeftRightLevel3][zero], 3, 8, 0, tiles)
  var [partitionFragment0RightChildLevel3] = ChildPartition([partitionFragment0LeftRightLevel3][one], 3, 8, 8, tiles)
  var [partitionFragment0LeftChildLevel4] = ChildPartition([partitionFragment0LeftRightLevel4][zero], 4, 16, 0, tiles)
  var [partitionFragment0RightChildLevel4] = ChildPartition([partitionFragment0LeftRightLevel4][one], 4, 16, 16, tiles)
  var [partitionFragment0LeftChildLevel5] = ChildPartition([partitionFragment0LeftRightLevel5][zero], 5, 32, 0, tiles)
  var [partitionFragment0RightChildLevel5] = ChildPartition([partitionFragment0LeftRightLevel5][one], 5, 32, 32, tiles)
  var [partitionFragment0LeftChildLevel6] = ChildPartition([partitionFragment0LeftRightLevel6][zero], 6, 64, 0, tiles)
  var [partitionFragment0RightChildLevel6] = ChildPartition([partitionFragment0LeftRightLevel6][one], 6, 64, 64, tiles)
  var [partitionFragment0LeftChildLevel7] = ChildPartition([partitionFragment0LeftRightLevel7][zero], 7, 128, 0, tiles)
  var [partitionFragment0RightChildLevel7] = ChildPartition([partitionFragment0LeftRightLevel7][one], 7, 128, 128, tiles)
  var [partitionFragment0LeftChildLevel8] = ChildPartition([partitionFragment0LeftRightLevel8][zero], 8, 256, 0, tiles)
  var [partitionFragment0RightChildLevel8] = ChildPartition([partitionFragment0LeftRightLevel8][one], 8, 256, 256, tiles)
  var [partitionFragment0LeftChildLevel9] = ChildPartition([partitionFragment0LeftRightLevel9][zero], 9, 512, 0, tiles)
  var [partitionFragment0RightChildLevel9] = ChildPartition([partitionFragment0LeftRightLevel9][one], 9, 512, 512, tiles)
  var [partitionFragment0LeftChildLevel10] = ChildPartition([partitionFragment0LeftRightLevel10][zero], 10, 1024, 0, tiles)
  var [partitionFragment0RightChildLevel10] = ChildPartition([partitionFragment0LeftRightLevel10][one], 10, 1024, 1024, tiles)

end


exports.InitializeFragment1 = rquote

  var [imageFragment1] = region([indices], PixelFields)
  var [partitionFragment1ByDepth] = DepthPartition([imageFragment1], fragmentWidth, fragmentHeight, tiles)

  var [partitionFragment1LeftRightLevel0] = SplitLeftRight([imageFragment1], 0, 1)
  var [partitionFragment1LeftRightLevel1] = SplitLeftRight([imageFragment1], 1, 2)
  var [partitionFragment1LeftRightLevel2] = SplitLeftRight([imageFragment1], 2, 4)
  var [partitionFragment1LeftRightLevel3] = SplitLeftRight([imageFragment1], 3, 8)
  var [partitionFragment1LeftRightLevel4] = SplitLeftRight([imageFragment1], 4, 16)
  var [partitionFragment1LeftRightLevel5] = SplitLeftRight([imageFragment1], 5, 32)
  var [partitionFragment1LeftRightLevel6] = SplitLeftRight([imageFragment1], 6, 64)
  var [partitionFragment1LeftRightLevel7] = SplitLeftRight([imageFragment1], 7, 128)
  var [partitionFragment1LeftRightLevel8] = SplitLeftRight([imageFragment1], 8, 256)
  var [partitionFragment1LeftRightLevel9] = SplitLeftRight([imageFragment1], 9, 512)
  var [partitionFragment1LeftRightLevel10] = SplitLeftRight([imageFragment1], 10, 1024)

  var [partitionFragment1LeftChildLevel0] = ChildPartition([partitionFragment1LeftRightLevel0][zero], 0, 1, 0, tiles)
  var [partitionFragment1RightChildLevel0] = ChildPartition([partitionFragment1LeftRightLevel0][one], 0, 1, 1, tiles)
  var [partitionFragment1LeftChildLevel1] = ChildPartition([partitionFragment1LeftRightLevel1][zero], 1, 2, 0, tiles)
  var [partitionFragment1RightChildLevel1] = ChildPartition([partitionFragment1LeftRightLevel1][one], 1, 2, 2, tiles)
  var [partitionFragment1LeftChildLevel2] = ChildPartition([partitionFragment1LeftRightLevel2][zero], 2, 4, 0, tiles)
  var [partitionFragment1RightChildLevel2] = ChildPartition([partitionFragment1LeftRightLevel2][one], 2, 4, 4, tiles)
  var [partitionFragment1LeftChildLevel3] = ChildPartition([partitionFragment1LeftRightLevel3][zero], 3, 8, 0, tiles)
  var [partitionFragment1RightChildLevel3] = ChildPartition([partitionFragment1LeftRightLevel3][one], 3, 8, 8, tiles)
  var [partitionFragment1LeftChildLevel4] = ChildPartition([partitionFragment1LeftRightLevel4][zero], 4, 16, 0, tiles)
  var [partitionFragment1RightChildLevel4] = ChildPartition([partitionFragment1LeftRightLevel4][one], 4, 16, 16, tiles)
  var [partitionFragment1LeftChildLevel5] = ChildPartition([partitionFragment1LeftRightLevel5][zero], 5, 32, 0, tiles)
  var [partitionFragment1RightChildLevel5] = ChildPartition([partitionFragment1LeftRightLevel5][one], 5, 32, 32, tiles)
  var [partitionFragment1LeftChildLevel6] = ChildPartition([partitionFragment1LeftRightLevel6][zero], 6, 64, 0, tiles)
  var [partitionFragment1RightChildLevel6] = ChildPartition([partitionFragment1LeftRightLevel6][one], 6, 64, 64, tiles)
  var [partitionFragment1LeftChildLevel7] = ChildPartition([partitionFragment1LeftRightLevel7][zero], 7, 128, 0, tiles)
  var [partitionFragment1RightChildLevel7] = ChildPartition([partitionFragment1LeftRightLevel7][one], 7, 128, 128, tiles)
  var [partitionFragment1LeftChildLevel8] = ChildPartition([partitionFragment1LeftRightLevel8][zero], 8, 256, 0, tiles)
  var [partitionFragment1RightChildLevel8] = ChildPartition([partitionFragment1LeftRightLevel8][one], 8, 256, 256, tiles)
  var [partitionFragment1LeftChildLevel9] = ChildPartition([partitionFragment1LeftRightLevel9][zero], 9, 512, 0, tiles)
  var [partitionFragment1RightChildLevel9] = ChildPartition([partitionFragment1LeftRightLevel9][one], 9, 512, 512, tiles)
  var [partitionFragment1LeftChildLevel10] = ChildPartition([partitionFragment1LeftRightLevel10][zero], 10, 1024, 0, tiles)
  var [partitionFragment1RightChildLevel10] = ChildPartition([partitionFragment1LeftRightLevel10][one], 10, 1024, 1024, tiles)

end

-- fragment 2 ...




exports.Render = function(dummy)
  return rquote
    for tile in tiles do
      Render(p_cells[tile], p_particles[tile],
        [partitionFragment0ByDepth][tile],
        [partitionFragment1ByDepth][tile]
      )
-- etc for more fragments
    end
  end
end




exports.Reduce = rquote

  __demand(__spmd) do

  -- tree level 0
    for tile in tiles do
      Reduce(0, 1, [partitionFragment0LeftChildLevel0][tile], [partitionFragment0RightChildLevel0][tile])
      Reduce(0, 1, [partitionFragment1LeftChildLevel0][tile], [partitionFragment1RightChildLevel0][tile])
--- etc for more fragments
    end

  -- tree level 1
    for tile in tiles do
      Reduce(1, 2, [partitionFragment0LeftChildLevel1][tile], [partitionFragment0RightChildLevel1][tile])
      Reduce(1, 2, [partitionFragment1LeftChildLevel1][tile], [partitionFragment1RightChildLevel1][tile])
--- etc for more fragments
    end

  -- save result to disk
    for tile in tiles do
      SaveImage(tile,
        partitionFragment0ByDepth[tile],
        partitionFragment1ByDepth[tile])
-- etc for more fragments
    end

end -- demand spmd

end



-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
