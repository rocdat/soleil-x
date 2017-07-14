
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

-- numFragments imageFragments and depthPartitions

local imageFragment0 = regentlib.newsymbol("imageFragment0")
local partition0ByDepth = regentlib.newsymbol("partition0ByDepth")
local imageFragment1 = regentlib.newsymbol("imageFragment1")
local partition1ByDepth = regentlib.newsymbol("partition1ByDepth")
-- etc


-- partitions for fragment0

local partition0LeftRight = regentlib.newsymbol("partition0LeftRight")

local partition0LeftChild0 = regentlib.newsymbol("partition0LeftChild0")
local partition0LeftChild1 = regentlib.newsymbol("partition0LeftChild1")
local partition0LeftChild2 = regentlib.newsymbol("partition0LeftChild2")
local partition0LeftChild3 = regentlib.newsymbol("partition0LeftChild3")
local partition0LeftChild4 = regentlib.newsymbol("partition0LeftChild4")
local partition0LeftChild5 = regentlib.newsymbol("partition0LeftChild5")
local partition0LeftChild6 = regentlib.newsymbol("partition0LeftChild6")
local partition0LeftChild7 = regentlib.newsymbol("partition0LeftChild7")
local partition0LeftChild8 = regentlib.newsymbol("partition0LeftChild8")
local partition0LeftChild9 = regentlib.newsymbol("partition0LeftChild9")
local partition0LeftChild10 = regentlib.newsymbol("partition0LeftChild10")
local partition0LeftChild11 = regentlib.newsymbol("partition0LeftChild11")
local partition0LeftChild12 = regentlib.newsymbol("partition0LeftChild12")
local partition0LeftChild13 = regentlib.newsymbol("partition0LeftChild13")
local partition0LeftChild14 = regentlib.newsymbol("partition0LeftChild14")

local partition0RightChild0 = regentlib.newsymbol("partition0RightChild0")
local partition0RightChild1 = regentlib.newsymbol("partition0RightChild1")
local partition0RightChild2 = regentlib.newsymbol("partition0RightChild2")
local partition0RightChild3 = regentlib.newsymbol("partition0RightChild3")
local partition0RightChild4 = regentlib.newsymbol("partition0RightChild4")
local partition0RightChild5 = regentlib.newsymbol("partition0RightChild5")
local partition0RightChild6 = regentlib.newsymbol("partition0RightChild6")
local partition0RightChild7 = regentlib.newsymbol("partition0RightChild7")
local partition0RightChild8 = regentlib.newsymbol("partition0RightChild8")
local partition0RightChild9 = regentlib.newsymbol("partition0RightChild9")
local partition0RightChild10 = regentlib.newsymbol("partition0RightChild10")
local partition0RightChild11 = regentlib.newsymbol("partition0RightChild11")
local partition0RightChild12 = regentlib.newsymbol("partition0RightChild12")
local partition0RightChild13 = regentlib.newsymbol("partition0RightChild13")
local partition0RightChild14 = regentlib.newsymbol("partition0RightChild14")

-- partitions for fragment1

local partition1LeftRight = regentlib.newsymbol("partition1LeftRight")

local partition1LeftChild0 = regentlib.newsymbol("partition1LeftChild0")
local partition1LeftChild1 = regentlib.newsymbol("partition1LeftChild1")
local partition1LeftChild2 = regentlib.newsymbol("partition1LeftChild2")
local partition1LeftChild3 = regentlib.newsymbol("partition1LeftChild3")
local partition1LeftChild4 = regentlib.newsymbol("partition1LeftChild4")
local partition1LeftChild5 = regentlib.newsymbol("partition1LeftChild5")
local partition1LeftChild6 = regentlib.newsymbol("partition1LeftChild6")
local partition1LeftChild7 = regentlib.newsymbol("partition1LeftChild7")
local partition1LeftChild8 = regentlib.newsymbol("partition1LeftChild8")
local partition1LeftChild9 = regentlib.newsymbol("partition1LeftChild9")
local partition1LeftChild10 = regentlib.newsymbol("partition1LeftChild10")
local partition1LeftChild11 = regentlib.newsymbol("partition1LeftChild11")
local partition1LeftChild12 = regentlib.newsymbol("partition1LeftChild12")
local partition1LeftChild13 = regentlib.newsymbol("partition1LeftChild13")
local partition1LeftChild14 = regentlib.newsymbol("partition1LeftChild14")

local partition1RightChild0 = regentlib.newsymbol("partition1RightChild0")
local partition1RightChild1 = regentlib.newsymbol("partition1RightChild1")
local partition1RightChild2 = regentlib.newsymbol("partition1RightChild2")
local partition1RightChild3 = regentlib.newsymbol("partition1RightChild3")
local partition1RightChild4 = regentlib.newsymbol("partition1RightChild4")
local partition1RightChild5 = regentlib.newsymbol("partition1RightChild5")
local partition1RightChild6 = regentlib.newsymbol("partition1RightChild6")
local partition1RightChild7 = regentlib.newsymbol("partition1RightChild7")
local partition1RightChild8 = regentlib.newsymbol("partition1RightChild8")
local partition1RightChild9 = regentlib.newsymbol("partition1RightChild9")
local partition1RightChild10 = regentlib.newsymbol("partition1RightChild10")
local partition1RightChild11 = regentlib.newsymbol("partition1RightChild11")
local partition1RightChild12 = regentlib.newsymbol("partition1RightChild12")
local partition1RightChild13 = regentlib.newsymbol("partition1RightChild13")
local partition1RightChild14 = regentlib.newsymbol("partition1RightChild14")

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

local task SplitLeftRight(r : region(ispace(int3d), PixelFields))

  var colors = ispace(int3d, int3d{ 2, 2, 2 }) -- 0 = left, 1 = right
  var coloring = regentlib.c.legion_multi_domain_point_coloring_create()

  for i = 0, numLayers do
    var rect = rect3d {
      lo = { 0, 0, i }, hi = { fragmentWidth - 1, fragmentHeight - 1, i }
    }
    var color = zero
    if i % 2 == 1 then
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

  -- regentlib.c.printf("ChildPartition level %d pow2Level %d offset %d\n",
  --   level, pow2Level, offset)

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

    -- regentlib.c.printf("tile %d %d %d   rect %d %d %d  %d %d %d\n",
    --   tile.x, tile.y, tile.z, rect.lo.x, rect.lo.y, rect.lo.z, rect.hi.x, rect.hi.y, rect.hi.z)

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
  regentlib.c.printf("in local task Reduce %d with %d\n",
    leftSubregion.bounds.lo.z, rightSubregion.bounds.lo.z)
--  if leftSubregion.bounds.lo.x < leftSubregion.bounds.hi.x and
--    rightSubregion.bounds.lo.x < rightSubregion.bounds.hi.x then
--    cviz.cxx_reduce(__runtime(), __context(),
--      __physical(leftSubregion), __fields(leftSubregion),
--      __physical(rightSubregion), __fields(rightSubregion),
--      treeLevel, offset)
--    regentlib.c.printf("end Reduce %d with %d\n", leftSubregion.bounds.lo.z, rightSubregion.bounds.lo.z)
--  end
end






-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}

exports.Initialize = rquote

  var indices = ispace(int3d, int3d{ fragmentWidth, fragmentHeight, numLayers })

  -- declare image fragments, up to numFragments

  var [imageFragment0] = region(indices, PixelFields)
  var [partition0ByDepth] = DepthPartition([imageFragment0], fragmentWidth, fragmentHeight, tiles)
  var [imageFragment1] = region(indices, PixelFields)
  var [partition1ByDepth] = DepthPartition([imageFragment1], fragmentWidth, fragmentHeight, tiles)

  -- fragment 0

  var [partition0LeftRight] = SplitLeftRight([imageFragment0])
  var [partition0LeftChild0] = ChildPartition([partition0LeftRight][zero], 0, 1, 0, tiles)
  var [partition0RightChild0] = ChildPartition([partition0LeftRight][one], 0, 1, 1, tiles)
  var [partition0LeftChild1] = ChildPartition([partition0LeftRight][zero], 1, 2, 0, tiles)
  var [partition0RightChild1] = ChildPartition([partition0LeftRight][one], 1, 2, 2, tiles)
  var [partition0LeftChild2] = ChildPartition([partition0LeftRight][zero], 2, 4, 0, tiles)
  var [partition0RightChild2] = ChildPartition([partition0LeftRight][one], 2, 4, 4, tiles)
  var [partition0LeftChild3] = ChildPartition([partition0LeftRight][zero], 3, 8, 0, tiles)
  var [partition0RightChild3] = ChildPartition([partition0LeftRight][one], 3, 8, 8, tiles)
  var [partition0LeftChild4] = ChildPartition([partition0LeftRight][zero], 4, 16, 0, tiles)
  var [partition0RightChild4] = ChildPartition([partition0LeftRight][one], 4, 16, 16, tiles)
  var [partition0LeftChild5] = ChildPartition([partition0LeftRight][zero], 5, 32, 0, tiles)
  var [partition0RightChild5] = ChildPartition([partition0LeftRight][one], 5, 32, 32, tiles)
  var [partition0LeftChild6] = ChildPartition([partition0LeftRight][zero], 6, 64, 0, tiles)
  var [partition0RightChild6] = ChildPartition([partition0LeftRight][one], 6, 64, 64, tiles)
  var [partition0LeftChild7] = ChildPartition([partition0LeftRight][zero], 7, 128, 0, tiles)
  var [partition0RightChild7] = ChildPartition([partition0LeftRight][one], 7, 128, 128, tiles)
  var [partition0LeftChild8] = ChildPartition([partition0LeftRight][zero], 8, 256, 0, tiles)
  var [partition0RightChild8] = ChildPartition([partition0LeftRight][one], 8, 256, 256, tiles)
  var [partition0LeftChild9] = ChildPartition([partition0LeftRight][zero], 9, 512, 0, tiles)
  var [partition0RightChild9] = ChildPartition([partition0LeftRight][one], 9, 512, 512, tiles)
  var [partition0LeftChild10] = ChildPartition([partition0LeftRight][zero], 10, 1024, 0, tiles)
  var [partition0RightChild10] = ChildPartition([partition0LeftRight][one], 10, 1024, 1024, tiles)

  -- fragment 1

  var [partition1LeftRight] = SplitLeftRight([imageFragment1])
  var [partition1LeftChild0] = ChildPartition([partition1LeftRight][zero], 0, 1, 0, tiles)
  var [partition1RightChild0] = ChildPartition([partition1LeftRight][one], 0, 1, 1, tiles)
  var [partition1LeftChild1] = ChildPartition([partition1LeftRight][zero], 1, 2, 0, tiles)
  var [partition1RightChild1] = ChildPartition([partition1LeftRight][one], 1, 2, 2, tiles)
  var [partition1LeftChild2] = ChildPartition([partition1LeftRight][zero], 2, 4, 0, tiles)
  var [partition1RightChild2] = ChildPartition([partition1LeftRight][one], 2, 4, 4, tiles)
  var [partition1LeftChild3] = ChildPartition([partition1LeftRight][zero], 3, 8, 0, tiles)
  var [partition1RightChild3] = ChildPartition([partition1LeftRight][one], 3, 8, 8, tiles)
  var [partition1LeftChild4] = ChildPartition([partition1LeftRight][zero], 4, 16, 0, tiles)
  var [partition1RightChild4] = ChildPartition([partition1LeftRight][one], 4, 16, 16, tiles)
  var [partition1LeftChild5] = ChildPartition([partition1LeftRight][zero], 5, 32, 0, tiles)
  var [partition1RightChild5] = ChildPartition([partition1LeftRight][one], 5, 32, 32, tiles)
  var [partition1LeftChild6] = ChildPartition([partition1LeftRight][zero], 6, 64, 0, tiles)
  var [partition1RightChild6] = ChildPartition([partition1LeftRight][one], 6, 64, 64, tiles)
  var [partition1LeftChild7] = ChildPartition([partition1LeftRight][zero], 7, 128, 0, tiles)
  var [partition1RightChild7] = ChildPartition([partition1LeftRight][one], 7, 128, 128, tiles)
  var [partition1LeftChild8] = ChildPartition([partition1LeftRight][zero], 8, 256, 0, tiles)
  var [partition1RightChild8] = ChildPartition([partition1LeftRight][one], 8, 256, 256, tiles)
  var [partition1LeftChild9] = ChildPartition([partition1LeftRight][zero], 9, 512, 0, tiles)
  var [partition1RightChild9] = ChildPartition([partition1LeftRight][one], 9, 512, 512, tiles)
  var [partition1LeftChild10] = ChildPartition([partition1LeftRight][zero], 10, 1024, 0, tiles)
  var [partition1RightChild10] = ChildPartition([partition1LeftRight][one], 10, 1024, 1024, tiles)

  -- fragment 2 ...

end


exports.Render = rquote
  for tile in tiles do
    Render(p_cells[tile], p_particles[tile],
      [partition0ByDepth][tile],
      [partition1ByDepth][tile]
-- etc to partition1000ByDepth
    )
  end
end


exports.Reduce = rquote
  regentlib.c.printf("Reduce\n")
  var indices = ispace(int3d, int3d{ fragmentWidth, fragmentHeight, numLayers })

  if numLayers > 1 then
    __demand(__spmd) do
      for tile in tiles do
        regentlib.c.printf("calling reduce for tile %d %d %d\n", tile.x, tile.y, tile.z)
        Reduce(0, 1, [partition0LeftChild0][tile], [partition0RightChild0][tile])
        -- Reduce(0, 1, [partition1LeftChild0][tile], [partition1RightChild0][tile])
--- etc for more fragments
      end
    end
  end

end

exports.FUBAR = rquote -- debugging only

  if numLayers > 2 then
    __demand(__spmd) do
      for tile in tiles do
        Reduce(1, 2, [partition0LeftChild1][tile], [partition0RightChild1][tile])
        Reduce(1, 2, [partition1LeftChild1][tile], [partition1RightChild1][tile])
--- etc for more fragments
      end
    end
  end

  if numLayers > 4 then
    __demand(__spmd) do
      for tile in tiles do
        Reduce(2, 4, [partition0LeftChild2][tile], [partition0RightChild2][tile])
        Reduce(2, 4, [partition1LeftChild2][tile], [partition1RightChild2][tile])
--- etc for more fragments
      end
    end
  end

-- etc for more tree levels (numLayers > 2^k)

end



-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
