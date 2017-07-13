
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

return function(cellsRel, particlesRel, xnum, ynum, znum, gridOrigin, gridWidth, imageRegion)

local p_cells   = cellsRel:primPartSymbol()
local cellsType = cellsRel:regionType()
local p_particles   = particlesRel:primPartSymbol()
local particlesType = particlesRel:regionType()
local tiles = A.primColors()

local width = 3840
local height = 2160
local numLayers = 4
local fragmentsX = 1
local fragmentsY = 1
local fragmentsZ = numLayers
local fragmentSizeX = (width / fragmentsX)
local fragmentSizeY = (height / fragmentsY)

local imageRegion = regentlib.newsymbol("imageRegion")
local partitionByDepth = regentlib.newsymbol("partitionByDepth")

local partitionLeftRight0 = regentlib.newsymbol("partitionLeftRight0")
local partitionLeftRight1 = regentlib.newsymbol("partitionLeftRight1")
local partitionLeftRight2 = regentlib.newsymbol("partitionLeftRight2")
local partitionLeftRight3 = regentlib.newsymbol("partitionLeftRight3")
local partitionLeftRight4 = regentlib.newsymbol("partitionLeftRight4")
local partitionLeftRight5 = regentlib.newsymbol("partitionLeftRight5")
local partitionLeftRight6 = regentlib.newsymbol("partitionLeftRight6")
local partitionLeftRight7 = regentlib.newsymbol("partitionLeftRight7")
local partitionLeftRight8 = regentlib.newsymbol("partitionLeftRight8")
local partitionLeftRight9 = regentlib.newsymbol("partitionLeftRight9")
local partitionLeftRight10 = regentlib.newsymbol("partitionLeftRight10")
local partitionLeftRight11 = regentlib.newsymbol("partitionLeftRight11")
local partitionLeftRight12 = regentlib.newsymbol("partitionLeftRight12")
local partitionLeftRight13 = regentlib.newsymbol("partitionLeftRight13")
local partitionLeftRight14 = regentlib.newsymbol("partitionLeftRight14")

local partitionLeftChild0 = regentlib.newsymbol("partitionLeftChild0")
local partitionLeftChild1 = regentlib.newsymbol("partitionLeftChild1")
local partitionLeftChild2 = regentlib.newsymbol("partitionLeftChild2")
local partitionLeftChild3 = regentlib.newsymbol("partitionLeftChild3")
local partitionLeftChild4 = regentlib.newsymbol("partitionLeftChild4")
local partitionLeftChild5 = regentlib.newsymbol("partitionLeftChild5")
local partitionLeftChild6 = regentlib.newsymbol("partitionLeftChild6")
local partitionLeftChild7 = regentlib.newsymbol("partitionLeftChild7")
local partitionLeftChild8 = regentlib.newsymbol("partitionLeftChild8")
local partitionLeftChild9 = regentlib.newsymbol("partitionLeftChild9")
local partitionLeftChild10 = regentlib.newsymbol("partitionLeftChild10")
local partitionLeftChild11 = regentlib.newsymbol("partitionLeftChild11")
local partitionLeftChild12 = regentlib.newsymbol("partitionLeftChild12")
local partitionLeftChild13 = regentlib.newsymbol("partitionLeftChild13")
local partitionLeftChild14 = regentlib.newsymbol("partitionLeftChild14")

local partitionRightChild0 = regentlib.newsymbol("partitionRightChild0")
local partitionRightChild1 = regentlib.newsymbol("partitionRightChild1")
local partitionRightChild2 = regentlib.newsymbol("partitionRightChild2")
local partitionRightChild3 = regentlib.newsymbol("partitionRightChild3")
local partitionRightChild4 = regentlib.newsymbol("partitionRightChild4")
local partitionRightChild5 = regentlib.newsymbol("partitionRightChild5")
local partitionRightChild6 = regentlib.newsymbol("partitionRightChild6")
local partitionRightChild7 = regentlib.newsymbol("partitionRightChild7")
local partitionRightChild8 = regentlib.newsymbol("partitionRightChild8")
local partitionRightChild9 = regentlib.newsymbol("partitionRightChild9")
local partitionRightChild10 = regentlib.newsymbol("partitionRightChild10")
local partitionRightChild11 = regentlib.newsymbol("partitionRightChild11")
local partitionRightChild12 = regentlib.newsymbol("partitionRightChild12")
local partitionRightChild13 = regentlib.newsymbol("partitionRightChild13")
local partitionRightChild14 = regentlib.newsymbol("partitionRightChild14")

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

-- left-right split:
-- lvl2 |LLL|LLL|LLL|LLL|RRR|RRR|RRR|RRR|
-- lvl1 |LLL|LLL|RRR|RRR|LLL|LLL|RRR|RRR|
-- lvl0 |LLL|RRR|LLL|RRR|LLL|RRR|LLL|RRR|

-- left-child coloring (only L's from left-right split): z % (pow2Level * 2) == 0
-- lvl2 |000|___|___|___|___|___|___|___|
-- lvl1 |000|___|___|___|444|___|___|___|
-- lvl0 |000|___|222|___|444|___|666|___|

-- right-child coloring (only R's from left-right split): (z / pow2Level) % 2 == 1.0
-- lvl2 |___|___|___|___|444|___|___|___|  z == 4, 12, 20, 28, 36
-- lvl1 |___|___|222|___|___|___|666|___|  z == 2, 6, 10, 14, 18
-- lvl0 |___|111|___|333|___|555|___|777|  z == 1, 3, 5, 7, 9



--
-- splitLeftRight: assign partition elements
--

local task splitLeftRight(r : region(ispace(int3d), PixelFields),
  level : int,
  pow2Level : int)

  regentlib.c.printf('splitLeftRight level %d pow2Level %d\n', level, pow2Level)
  var colors = ispace(int3d, int3d{ 2, 2, 2 }) -- 0 = left, 1 = right
  var coloring = regentlib.c.legion_multi_domain_point_coloring_create()

  var elementsPerFragmentX = ([int](r.bounds.hi.x) + 1) / fragmentsX
  var elementsPerFragmentY = ([int](r.bounds.hi.y) + 1) / fragmentsY
  var elementsPerFragmentZ = ([int](r.bounds.hi.z) + 1) / fragmentsZ

  for z = 0, fragmentsZ do
    for y = 0, fragmentsY do
      for x = 0, fragmentsX do

        regentlib.c.printf("x y z  %d %d %d\n", x, y, z)
        var rect = rect3d {
          lo = int3d{ x * elementsPerFragmentX, y * elementsPerFragmentY, z * elementsPerFragmentZ },
          hi = int3d{ (x + 1) * elementsPerFragmentX - 1, (y + 1) * elementsPerFragmentY - 1, (z + 1) * elementsPerFragmentZ - 1 }
        }

        if (z / pow2Level) % 2 == 0 then
          regentlib.c.printf('  left: %d %d %d - %d %d %d\n',
            rect.lo.x, rect.lo.y, rect.lo.z, rect.hi.x, rect.hi.y, rect.hi.z)
          regentlib.c.legion_multi_domain_point_coloring_color_domain(coloring, int3d{ 0, 0, 0}, rect)
        else
          regentlib.c.printf('  right: %d %d %d - %d %d %d\n',
            rect.lo.x, rect.lo.y, rect.lo.z, rect.hi.x, rect.hi.y, rect.hi.z)
          regentlib.c.legion_multi_domain_point_coloring_color_domain(coloring, int3d{ 1, 1, 1}, rect)
        end
      end
    end
  end
  var p = partition(disjoint, r, coloring, colors)
  regentlib.c.legion_multi_domain_point_coloring_destroy(coloring)
  return p
end



--
-- leftChildPartition
--

local task leftChildPartition(r : region(ispace(int3d), PixelFields),
  level : int,
  pow2Level : int)

  var elementsPerFragmentX = ([int](r.bounds.hi.x) + 1) / fragmentsX
  var elementsPerFragmentY = ([int](r.bounds.hi.y) + 1) / fragmentsY
  var elementsPerFragmentZ = ([int](r.bounds.hi.z) + 1) / fragmentsZ

  regentlib.c.printf('level %d left children:\n', level)
  var coloring = regentlib.c.legion_domain_point_coloring_create()
  var colors = ispace(int3d, int3d{ fragmentsX, fragmentsY, numLayers })

  for z = 0, fragmentsZ do
    for y = 0, fragmentsY do
      for x = 0, fragmentsX do
        var rect = rect3d{ lo = int3d{ 0, 0, 0 }, hi = int3d{ 0, 0, 0 } }
        if z % (2 * pow2Level) == 0 then
          rect = rect3d {
            lo = int3d{ x * elementsPerFragmentX, y * elementsPerFragmentY, z * elementsPerFragmentZ },
            hi = int3d{ (x + 1) * elementsPerFragmentX - 1, (y + 1) * elementsPerFragmentY - 1, (z + 1) * elementsPerFragmentZ - 1 }
          }
        else
          rect = rect3d {
            lo = int3d{ 1, 1, 1 }, hi = int3d{ 0, 0, 0 }
          }
        end

        regentlib.c.printf("x y z %d %d %d rect %d %d %d   %d %d %d\n",
          x, y, z, rect.lo.x, rect.lo.y, rect.lo.z, rect.hi.x, rect.hi.y, rect.hi.z)

        var color = int3d{ x, y, z }
        regentlib.c.legion_domain_point_coloring_color_domain(coloring, color, rect)
      end
    end
  end
  var p = partition(disjoint, r, coloring, colors)
  regentlib.c.legion_domain_point_coloring_destroy(coloring)
  return p
end




--
-- rightChildPartition
--

local task rightChildPartition(r : region(ispace(int3d), PixelFields),
  level : int,
  pow2Level : int)

  var elementsPerFragmentX = ([int](r.bounds.hi.x) + 1) / fragmentsX
  var elementsPerFragmentY = ([int](r.bounds.hi.y) + 1) / fragmentsY
  var elementsPerFragmentZ = ([int](r.bounds.hi.z) + 1) / fragmentsZ

  regentlib.c.printf('level %d right children:\n', level)
  var coloring = regentlib.c.legion_domain_point_coloring_create()
  var colors = ispace(int3d, int3d{ fragmentsX, fragmentsY, numLayers })

  for z = 0, fragmentsZ do
    for y = 0, fragmentsY do
      for x = 0, fragmentsX do
        var rect = rect3d{ lo = int3d{ 0, 0, 0 }, hi = int3d{ 0, 0, 0 } }
        if z % (2 * pow2Level) == 0 then
          rect = rect3d {
            lo = int3d{ x * elementsPerFragmentX, y * elementsPerFragmentY, z * elementsPerFragmentZ },
            hi = int3d{ (x + 1) * elementsPerFragmentX - 1, (y + 1) * elementsPerFragmentY - 1, (z + 1) * elementsPerFragmentZ - 1 }
          }
        else
          rect = rect3d {
            lo = int3d{ 1, 1, 1 }, hi = int3d{ 0, 0, 0 }
          }
        end

        regentlib.c.printf("x y z %d %d %d rect %d %d %d   %d %d %d\n",
          x, y, z, rect.lo.x, rect.lo.y, rect.lo.z, rect.hi.x, rect.hi.y, rect.hi.z)

        var color = int3d{ x, y, z }
        regentlib.c.legion_domain_point_coloring_color_domain(coloring, color, rect)
      end
    end
  end
  var p = partition(disjoint, r, coloring, colors)
  regentlib.c.legion_domain_point_coloring_destroy(coloring)
  return p
end




--
-- DepthPartition: partition the imageRegion by layers
--

local task DepthPartition(r : region(ispace(int3d), PixelFields),
  width : int,
  height : int)

  var coloring = regentlib.c.legion_domain_point_coloring_create()
  var colors = ispace(int3d, int3d{1, 1, numLayers})

  for c in colors do
    var rect = rect3d {
      lo = { c.x, c.y, c.z },
      hi = { c.x + width - 1, c.y + height - 1, c.z }
    }
    regentlib.c.legion_domain_point_coloring_color_domain(coloring, c, rect)
  end
  var p = partition(disjoint, r, coloring, colors)
  regentlib.c.legion_domain_point_coloring_destroy(coloring)
  return p
end




--
-- Render: produce an RGBA and DEPTH buffer image, write the pixels into the imageRegion
--

local task Render(cells : cellsType, particles : particlesType, imageRegion : region(ispace(int3d), PixelFields))
where
  reads(cells.{centerCoordinates, velocity, temperature}),
  reads(particles.{__valid, cell, position, density, particle_temperature, tracking}),
  writes(imageRegion.{R, G, B, A, Z, UserData})
do
  cviz.cxx_render(__runtime(), __context(),
    __physical(cells), __fields(cells),
    __physical(particles), __fields(particles),
    __physical(imageRegion), __fields(imageRegion),
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
   reads writes (leftSubregion), reads (rightSubregion), leftSubregion * rightSubregion
do
  if leftSubregion.bounds.lo.x < leftSubregion.bounds.hi.x then
    regentlib.c.printf("in reduce treeLevel %d offset %d\n", treeLevel, offset)
    cviz.cxx_reduce(__runtime(), __context(),
      __physical(leftSubregion), __fields(leftSubregion),
      __physical(rightSubregion), __fields(rightSubregion),
      treeLevel, offset)
  end
end






-------------------------------------------------------------------------------
-- Exported quotes
-------------------------------------------------------------------------------

local exports = {}


exports.Initialize = rquote

  var indices = ispace(int3d, int3d{ width, height, numLayers })
  var [imageRegion] = region(indices, PixelFields)
  var [partitionByDepth] = DepthPartition([imageRegion], width, height)
  var zero = int3d{ 0, 0, 0 }
  var one = int3d{ 0, 0, 1 }

  if numLayers > 1 then
    var [partitionLeftRight0] = splitLeftRight([imageRegion], 0, 1)
    var [partitionLeftChild0] = leftChildPartition([partitionLeftRight0][zero], 0, 1)
    var [partitionRightChild0] = rightChildPartition([partitionLeftRight0][one], 0, 1)
  end
  if numLayers > 2 then
    var [partitionLeftRight1] = splitLeftRight([imageRegion], 1, 2)
    var [partitionLeftChild1] = leftChildPartition([partitionLeftRight1][zero], 1, 2)
    var [partitionRightChild1] = rightChildPartition([partitionLeftRight1][one], 1, 2)
  end
  if numLayers > 4 then
    var [partitionLeftRight2] = splitLeftRight([imageRegion], 2, 4)
    var [partitionLeftChild2] = leftChildPartition([partitionLeftRight2][zero], 2, 4)
    var [partitionRightChild2] = rightChildPartition([partitionLeftRight2][one], 2, 4)
  end
  if numLayers > 8 then
    var [partitionLeftRight3] = splitLeftRight([imageRegion], 3, 8)
    var [partitionLeftChild3] = leftChildPartition([partitionLeftRight3][zero], 3, 8)
    var [partitionRightChild3] = rightChildPartition([partitionLeftRight3][one], 3, 8)
  end
  if numLayers > 16 then
    var [partitionLeftRight4] = splitLeftRight([imageRegion], 4, 16)
    var [partitionLeftChild4] = leftChildPartition([partitionLeftRight4][zero], 4, 16)
    var [partitionRightChild4] = rightChildPartition([partitionLeftRight4][one], 4, 16)
  end
  if numLayers > 32 then
    var [partitionLeftRight5] = splitLeftRight([imageRegion], 5, 32)
    var [partitionLeftChild5] = leftChildPartition([partitionLeftRight5][zero], 5, 32)
    var [partitionRightChild5] = rightChildPartition([partitionLeftRight5][one], 5, 32)
  end
  if numLayers > 64 then
    var [partitionLeftRight6] = splitLeftRight([imageRegion], 6, 64)
    var [partitionLeftChild6] = leftChildPartition([partitionLeftRight6][zero], 6, 64)
    var [partitionRightChild6] = rightChildPartition([partitionLeftRight6][one], 6, 64)
  end
  if numLayers > 128 then
    var [partitionLeftRight7] = splitLeftRight([imageRegion], 7, 128)
    var [partitionLeftChild7] = leftChildPartition([partitionLeftRight7][zero], 7, 128)
    var [partitionRightChild7] = rightChildPartition([partitionLeftRight7][one], 7, 128)
  end
  if numLayers > 256 then
    var [partitionLeftRight8] = splitLeftRight([imageRegion], 8, 256)
    var [partitionLeftChild8] = leftChildPartition([partitionLeftRight8][zero], 8, 256)
    var [partitionRightChild8] = rightChildPartition([partitionLeftRight8][one], 8, 256)
  end
  if numLayers > 512 then
    var [partitionLeftRight9] = splitLeftRight([imageRegion], 9, 512)
    var [partitionLeftChild9] = leftChildPartition([partitionLeftRight9][zero], 9, 512)
    var [partitionRightChild9] = rightChildPartition([partitionLeftRight9][one], 9, 512)
  end
  if numLayers > 1024 then
    var [partitionLeftRight10] = splitLeftRight([imageRegion], 10, 1024)
    var [partitionLeftChild10] = leftChildPartition([partitionLeftRight10][zero], 10, 1024)
    var [partitionRightChild10] = rightChildPartition([partitionLeftRight10][one], 10, 1024)
  end


end


exports.Render = rquote
  var depthPartitionNumber = 0
  for tile in tiles do
    var partitionID = int3d{ 0, 0, depthPartitionNumber }
    depthPartitionNumber = depthPartitionNumber + 1
    Render(p_cells[tile], p_particles[tile], [partitionByDepth][partitionID])
  end
end


exports.Reduce = rquote
  regentlib.c.printf("Reduce\n")
  if numLayers > 1 then
    __demand(__spmd) do
      for z = 0, fragmentsZ do
        for y = 0, fragmentsY do
          for x = 0, fragmentsX do
            var color = int3d{ x, y, z }
            Reduce(0, 1, [partitionLeftChild0][color], [partitionRightChild0][color])
          end
        end
      end
    end
  end
end



-------------------------------------------------------------------------------
-- Module exports
-------------------------------------------------------------------------------

return exports
end
