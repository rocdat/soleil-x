#!/usr/bin/env python

# Copyright 2017 Stanford University, NVIDIA Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import sys

  
def generateCode(keyword, numFragments, numTreeLevels):
    
  if(keyword == 'legion_physical_region_t_imageFragmentX'):
    for i in range(numFragments):
      print '                   legion_physical_region_t *imageFragment' + str(i) + ','
      if( i < numFragments - 1):
        print '                   legion_field_id_t *imageFragment' + str(i) + '_fields,'
      else:
        print '                   legion_field_id_t *imageFragment' + str(i) + '_fields'

  
  if(keyword == 'legion_physical_region_t_imageFragmentXComma'):
    for i in range(numFragments):
      print '                   legion_physical_region_t *imageFragment' + str(i) + ','
      print '                   legion_field_id_t *imageFragment' + str(i) + '_fields,'
  
  ###viz.cc:                // CODEGEN: legion_physical_region_t_imageFragmentX
  #  legion_physical_region_t *imageFragment0,
  #  legion_field_id_t *imageFragment0_fields,
  #    legion_physical_region_t *imageFragment1,
  #      legion_field_id_t *imageFragment1_fields,
  
  if(keyword == 'legion_physical_region_t_imageFragment_arrays'):
    print '  legion_physical_region_t* imageFragment[] = {'
    for i in range(numFragments):
      if(i < numFragments - 1):
        print '    imageFragment' + str(i) + ','
      else:
        print '    imageFragment' + str(i)


    print '  };'
    print '  legion_field_id_t* imageFragment_fields[] = {'
    for i in range(numFragments):
      if(i < numFragments - 1):
        print '    imageFragment' + str(i) + '_fields,'
      else:
        print '    imageFragment' + str(i) + '_fields'

    print '  };'


###viz.cc:  // CODEGEN: legion_physical_region_t_imageFragment_arrays
#legion_physical_region_t* imageFragment[] = {
#  imageFragment0,
#    imageFragment1
#    /***extend here for more regions***///////////////////
#  };
#
#  legion_field_id_t* imageFragment_fields[] = {
#    imageFragment0_fields,
#    imageFragment1_fields
#    /***extend here for more regions***///////////////////
#};

##viz.cc:                   // CODEGEN: legion_physical_region_t_imageFragmentX
#legion_physical_region_t *imageFragment0,
#  legion_field_id_t *imageFragment0_fields,
#    legion_physical_region_t *imageFragment1,
#      legion_field_id_t *imageFragment1_fields)

##viz.cc:  // CODEGEN: legion_physical_region_t_imageFragment_arrays
#legion_physical_region_t* imageFragment[] = {
#  imageFragment0,
#    imageFragment1
#    /***extend here for more regions***///////////////////
#  };
#
#  legion_field_id_t* imageFragment_fields[] = {
#    imageFragment0_fields,
#    imageFragment1_fields
#    /***extend here for more regions***///////////////////
#};


#viz.h:                  // CODEGEN: legion_physical_region_t_imageFragmentX
#legion_physical_region_t *imageFragment0,
#  legion_field_id_t *imageFragment0_fields,
#    legion_physical_region_t *imageFragment1,
#      legion_field_id_t *imageFragment1_fields,

#viz.h:                     // CODEGEN: legion_physical_region_t_imageFragmentX
#legion_physical_region_t *imageFragment0,
#  legion_field_id_t *imageFragment0_fields,
#    legion_physical_region_t *imageFragment1,
#      legion_field_id_t *imageFragment1_fields,


  if(keyword == 'local_imageFragmentX'):
    print 'local indices = regentlib.newsymbol("indices")'
    print 'local timeStep = regentlib.newsymbol("timeStep")'
    for i in range(numFragments):
      print 'local imageFragment' + str(i) + ' = regentlib.newsymbol("imageFragment' + str(i) + '")'

#viz.rg:-- CODEGEN: local_imageFragmentX
#local indices = regentlib.newsymbol("indices")
#local imageFragment0 = regentlib.newsymbol("imageFragment0")
#local imageFragment1 = regentlib.newsymbol("imageFragment1")

  if(keyword == 'local_partitionFragmentXByDepth'):
    for i in range(numFragments):
      print 'local partitionFragment' + str(i) + 'ByDepth = regentlib.newsymbol("partitionFragment' + str(i) + 'ByDepth")'
    
    
    #viz.rg:-- CODEGEN: local_partitionFragmentXByDepth
    #local partitionFragment0ByDepth = regentlib.newsymbol("partitionFragment0ByDepth")
    #local partitionFragment1ByDepth = regentlib.newsymbol("partitionFragment1ByDepth")
    
  if(keyword == 'local_partitionFragmentXLeftRightChildren'):
    for i in range(numFragments):
      print '-- partitions for fragment ' + str(i) + ''
      for j in range(numTreeLevels):
        print 'local partitionFragment' + str(i) + 'LeftRightLevel' + str(j) + ' = regentlib.newsymbol("partitionFragment' + str(i) + 'LeftRightLevel' + str(j) + '")'
        print 'local partitionFragment' + str(i) + 'LeftChildLevel' + str(j) + ' = regentlib.newsymbol("partitionFragment' + str(i) + 'LeftChildLevel' + str(j) + '")'
        print 'local partitionFragment' + str(i) + 'RightChildLevel' + str(j) + ' = regentlib.newsymbol("partitionFragment' + str(i) + 'RightChildLevel' + str(j) + '")'

  #viz.rg:-- CODEGEN: local_partitionFragmentXLeftRightChildren
  #repeat this for num fragments
  #-- partitions for fragment0

  #local partitionFragment0LeftRightLevel0 = regentlib.newsymbol("partitionFragment0LeftRightLevel0")
  #local partitionFragment0LeftRightLevel1 = regentlib.newsymbol("partitionFragment0LeftRightLevel1")
  #local partitionFragment0LeftRightLevel2 = regentlib.newsymbol("partitionFragment0LeftRightLevel2")
  #local partitionFragment0LeftRightLevel3 = regentlib.newsymbol("partitionFragment0LeftRightLevel3")
  #local partitionFragment0LeftRightLevel4 = regentlib.newsymbol("partitionFragment0LeftRightLevel4")
  #local partitionFragment0LeftRightLevel5 = regentlib.newsymbol("partitionFragment0LeftRightLevel5")
  #local partitionFragment0LeftRightLevel6 = regentlib.newsymbol("partitionFragment0LeftRightLevel6")
  #local partitionFragment0LeftRightLevel7 = regentlib.newsymbol("partitionFragment0LeftRightLevel7")
  #local partitionFragment0LeftRightLevel8 = regentlib.newsymbol("partitionFragment0LeftRightLevel8")
  #local partitionFragment0LeftRightLevel9 = regentlib.newsymbol("partitionFragment0LeftRightLevel9")
  #local partitionFragment0LeftRightLevel10 = regentlib.newsymbol("partitionFragment0LeftRightLevel10")

  #local partitionFragment0LeftChildLevel0 = regentlib.newsymbol("partitionFragment0LeftChildLevel0")
  #local partitionFragment0LeftChildLevel1 = regentlib.newsymbol("partitionFragment0LeftChildLevel1")
  #local partitionFragment0LeftChildLevel2 = regentlib.newsymbol("partitionFragment0LeftChildLevel2")
  #local partitionFragment0LeftChildLevel3 = regentlib.newsymbol("partitionFragment0LeftChildLevel3")
  #local partitionFragment0LeftChildLevel4 = regentlib.newsymbol("partitionFragment0LeftChildLevel4")
  #local partitionFragment0LeftChildLevel5 = regentlib.newsymbol("partitionFragment0LeftChildLevel5")
  #local partitionFragment0LeftChildLevel6 = regentlib.newsymbol("partitionFragment0LeftChildLevel6")
  #local partitionFragment0LeftChildLevel7 = regentlib.newsymbol("partitionFragment0LeftChildLevel7")
  #local partitionFragment0LeftChildLevel8 = regentlib.newsymbol("partitionFragment0LeftChildLevel8")
  #local partitionFragment0LeftChildLevel9 = regentlib.newsymbol("partitionFragment0LeftChildLevel9")
  #local partitionFragment0LeftChildLevel10 = regentlib.newsymbol("partitionFragment0LeftChildLevel10")
  #local partitionFragment0LeftChildLevel11 = regentlib.newsymbol("partitionFragment0LeftChildLevel11")
  #local partitionFragment0LeftChildLevel12 = regentlib.newsymbol("partitionFragment0LeftChildLevel12")
  #local partitionFragment0LeftChildLevel13 = regentlib.newsymbol("partitionFragment0LeftChildLevel13")
  #local partitionFragment0LeftChildLevel14 = regentlib.newsymbol("partitionFragment0LeftChildLevel14")

  #local partitionFragment0RightChildLevel0 = regentlib.newsymbol("partitionFragment0RightChildLevel0")
  #local partitionFragment0RightChildLevel1 = regentlib.newsymbol("partitionFragment0RightChildLevel1")
  #local partitionFragment0RightChildLevel2 = regentlib.newsymbol("partitionFragment0RightChildLevel2")
  #local partitionFragment0RightChildLevel3 = regentlib.newsymbol("partitionFragment0RightChildLevel3")
  #local partitionFragment0RightChildLevel4 = regentlib.newsymbol("partitionFragment0RightChildLevel4")
  #local partitionFragment0RightChildLevel5 = regentlib.newsymbol("partitionFragment0RightChildLevel5")
  #local partitionFragment0RightChildLevel6 = regentlib.newsymbol("partitionFragment0RightChildLevel6")
  #local partitionFragment0RightChildLevel7 = regentlib.newsymbol("partitionFragment0RightChildLevel7")
  #local partitionFragment0RightChildLevel8 = regentlib.newsymbol("partitionFragment0RightChildLevel8")
  #local partitionFragment0RightChildLevel9 = regentlib.newsymbol("partitionFragment0RightChildLevel9")
  #local partitionFragment0RightChildLevel10 = regentlib.newsymbol("partitionFragment0RightChildLevel10")
  #local partitionFragment0RightChildLevel11 = regentlib.newsymbol("partitionFragment0RightChildLevel11")
  #local partitionFragment0RightChildLevel12 = regentlib.newsymbol("partitionFragment0RightChildLevel12")
  #local partitionFragment0RightChildLevel13 = regentlib.newsymbol("partitionFragment0RightChildLevel13")
  #local partitionFragment0RightChildLevel14 = regentlib.newsymbol("partitionFragment0RightChildLevel14")

  if(keyword == 'imageFragmentX_arglist'):
    for i in range(numFragments):
      if(i < numFragments - 1):
        print '  imageFragment' + str(i) + ' : region(ispace(int3d), PixelFields),'
      else:
        print '  imageFragment' + str(i) + ' : region(ispace(int3d), PixelFields)'


#viz.rg:-- CODEGEN: imageFragmentX_arglist
#imageFragment0 : region(ispace(int3d), PixelFields),
#  imageFragment1 : region(ispace(int3d), PixelFields)

  if(keyword == 'writes_imageFragmentX'):
    for i in range(numFragments):
      if(i < numFragments - 1):
        print '  writes(imageFragment' + str(i) + '),'
      else:
        print '  writes(imageFragment' + str(i) + ')'



#viz.rg:-- CODEGEN: writes_imageFragmentX
#writes(imageFragment0),
#  writes(imageFragment1)

  if(keyword == '__physical_imageFragmentX__fields'):
    for i in range(numFragments):
      if i < numFragments - 1:
        print '    __physical(imageFragment' + str(i) + '), __fields(imageFragment' + str(i) + '),'
      else:
        print '    __physical(imageFragment' + str(i) + '), __fields(imageFragment' + str(i) + ')'


  if(keyword == '__physical_imageFragmentX__fieldsComma'):
    for i in range(numFragments):
      print '    __physical(imageFragment' + str(i) + '), __fields(imageFragment' + str(i) + '),'


  #viz.rg:-- CODEGEN: __physical_imageFragmentX__fields
  #__physical(imageFragment0), __fields(imageFragment0),
  #  __physical(imageFragment1), __fields(imageFragment1),

  #viz.rg:-- CODEGEN: imageFragmentX_region_paramList
  #imageFragment0 : region(ispace(int3d), PixelFields),
  #  imageFragment1 : region(ispace(int3d), PixelFields)

  if(keyword == 'reads_imageFragmentX'):
    for i in range(numFragments):
      if(i < numFragments - 1):
        print '  reads(imageFragment' + str(i) + '),'
      else:
        print '  reads(imageFragment' + str(i) + ')'



#viz.rg:-- CODEGEN: reads_imageFragmentX
#reads(imageFragment0),
#  reads(imageFragment1)

  if(keyword == 'viz.InitializeFragmentX'):
    for i in range(numFragments):
      print '  M.INLINE(viz.InitializeFragment' + str(i) + ')'

#viz.rg:-- CODEGEN: __physical_imageFragmentX__fields
#__physical(imageFragment0), __fields(imageFragment0),
#  __physical(imageFragment1), __fields(imageFragment1)


  if(keyword == 'InitializeFragmentX'):
    for i in range(numFragments):
      print '\nexports.InitializeFragment' + str(i) + ' = rquote\n'
      if(i == 0):
        print '  var numLayers = tiles.volume'
        print '  var [timeStep] = 0'
        print '  var [indices] = ispace(int3d, int3d{ fragmentWidth, fragmentHeight, numLayers })'
      print '  var [imageFragment' + str(i) + '] = region([indices], PixelFields)'
      print '  var [partitionFragment' + str(i) + 'ByDepth] = DepthPartition([imageFragment' + str(i) + '], fragmentWidth, fragmentHeight, tiles)'
      pow2Level = 1
      for j in range(numTreeLevels):
        print '  var [partitionFragment' + str(i) + 'LeftRightLevel' + str(j) + '] = SplitLeftRight([imageFragment' + str(i) + '], ' + str(i) + ', ' + str(pow2Level) + ', tiles)'
        print '  var [partitionFragment' + str(i) + 'LeftChildLevel' + str(j) + '] = ChildPartition([partitionFragment' + str(i) + 'LeftRightLevel' + str(j) + '][zero], ' + str(j) + ', ' + str(pow2Level) + ', 0, tiles)'
        print '  var [partitionFragment' + str(i) + 'RightChildLevel' + str(j) + '] = ChildPartition([partitionFragment' + str(i) + 'LeftRightLevel' + str(j) + '][one], ' + str(j) + ', ' + str(pow2Level) + ', ' + str(pow2Level) + ', tiles)'
        pow2Level = pow2Level * 2
      print '\nend'



#viz.rg:-- CODEGEN: InitializeFragmentX
#repeat for numfragments
#exports.InitializeFragment0 = rquote
#  var [indices] = ispace(int3d, int3d{ fragmentWidth, fragmentHeight, numLayers })
#  var [imageFragment0] = region([indices], PixelFields)
#  var [partitionFragment0ByDepth] = DepthPartition([imageFragment0], fragmentWidth, fragmentHeight, tiles)

#  var [partitionFragment0LeftRightLevel0] = SplitLeftRight([imageFragment0], 0, 1)
#  var [partitionFragment0LeftRightLevel1] = SplitLeftRight([imageFragment0], 1, 2)
#  var [partitionFragment0LeftRightLevel2] = SplitLeftRight([imageFragment0], 2, 4)
#  var [partitionFragment0LeftRightLevel3] = SplitLeftRight([imageFragment0], 3, 8)
#  var [partitionFragment0LeftRightLevel4] = SplitLeftRight([imageFragment0], 4, 16)
#  var [partitionFragment0LeftRightLevel5] = SplitLeftRight([imageFragment0], 5, 32)
#  var [partitionFragment0LeftRightLevel6] = SplitLeftRight([imageFragment0], 6, 64)
#  var [partitionFragment0LeftRightLevel7] = SplitLeftRight([imageFragment0], 7, 128)
#  var [partitionFragment0LeftRightLevel8] = SplitLeftRight([imageFragment0], 8, 256)
#  var [partitionFragment0LeftRightLevel9] = SplitLeftRight([imageFragment0], 9, 512)
#  var [partitionFragment0LeftRightLevel10] = SplitLeftRight([imageFragment0], 10, 1024)

#  var [partitionFragment0LeftChildLevel0] = ChildPartition([partitionFragment0LeftRightLevel0][zero], 0, 1, 0, tiles)
#  var [partitionFragment0RightChildLevel0] = ChildPartition([partitionFragment0LeftRightLevel0][one], 0, 1, 1, tiles)
#  var [partitionFragment0LeftChildLevel1] = ChildPartition([partitionFragment0LeftRightLevel1][zero], 1, 2, 0, tiles)
#  var [partitionFragment0RightChildLevel1] = ChildPartition([partitionFragment0LeftRightLevel1][one], 1, 2, 2, tiles)
#  var [partitionFragment0LeftChildLevel2] = ChildPartition([partitionFragment0LeftRightLevel2][zero], 2, 4, 0, tiles)
#  var [partitionFragment0RightChildLevel2] = ChildPartition([partitionFragment0LeftRightLevel2][one], 2, 4, 4, tiles)
#  var [partitionFragment0LeftChildLevel3] = ChildPartition([partitionFragment0LeftRightLevel3][zero], 3, 8, 0, tiles)
#  var [partitionFragment0RightChildLevel3] = ChildPartition([partitionFragment0LeftRightLevel3][one], 3, 8, 8, tiles)
#  var [partitionFragment0LeftChildLevel4] = ChildPartition([partitionFragment0LeftRightLevel4][zero], 4, 16, 0, tiles)
#  var [partitionFragment0RightChildLevel4] = ChildPartition([partitionFragment0LeftRightLevel4][one], 4, 16, 16, tiles)
#  var [partitionFragment0LeftChildLevel5] = ChildPartition([partitionFragment0LeftRightLevel5][zero], 5, 32, 0, tiles)
#  var [partitionFragment0RightChildLevel5] = ChildPartition([partitionFragment0LeftRightLevel5][one], 5, 32, 32, tiles)
#  var [partitionFragment0LeftChildLevel6] = ChildPartition([partitionFragment0LeftRightLevel6][zero], 6, 64, 0, tiles)
#  var [partitionFragment0RightChildLevel6] = ChildPartition([partitionFragment0LeftRightLevel6][one], 6, 64, 64, tiles)
#  var [partitionFragment0LeftChildLevel7] = ChildPartition([partitionFragment0LeftRightLevel7][zero], 7, 128, 0, tiles)
#  var [partitionFragment0RightChildLevel7] = ChildPartition([partitionFragment0LeftRightLevel7][one], 7, 128, 128, tiles)
#  var [partitionFragment0LeftChildLevel8] = ChildPartition([partitionFragment0LeftRightLevel8][zero], 8, 256, 0, tiles)
#  var [partitionFragment0RightChildLevel8] = ChildPartition([partitionFragment0LeftRightLevel8][one], 8, 256, 256, tiles)
#  var [partitionFragment0LeftChildLevel9] = ChildPartition([partitionFragment0LeftRightLevel9][zero], 9, 512, 0, tiles)
#  var [partitionFragment0RightChildLevel9] = ChildPartition([partitionFragment0LeftRightLevel9][one], 9, 512, 512, tiles)
#  var [partitionFragment0LeftChildLevel10] = ChildPartition([partitionFragment0LeftRightLevel10][zero], 10, 1024, 0, tiles)
#  var [partitionFragment0RightChildLevel10] = ChildPartition([partitionFragment0LeftRightLevel10][one], 10, 1024, 1024, tiles)

#end

  if(keyword == 'partitionFragmentXByDepth_argList'):
    for i in range(numFragments):
      if(i < numFragments - 1):
        print '        [partitionFragment' + str(i) + 'ByDepth][tile],'
      else:
        print '        [partitionFragment' + str(i) + 'ByDepth][tile]'


  #viz.rg:-- CODEGEN: partitionFragmentXByDepth_argList
  #[partitionFragment0ByDepth][tile],
  #  [partitionFragment1ByDepth][tile]

  if(keyword == 'tree_reductions'):
    pow2Level = 1
    for j in range(numTreeLevels):
      print '\n-- tree level ' + str(j) + '\n'
      print '      for tile in tiles do'
      for i in range(numFragments):
        print '        Reduce(' + str(j) + ', ' + str(pow2Level) + ', [partitionFragment' + str(i) + 'LeftChildLevel' + str(j) + '][tile], [partitionFragment' + str(i) + 'RightChildLevel' + str(j) + '][tile])'
      print '      end'
      pow2Level = pow2Level * 2

#viz.rg:-- CODEGEN: tree_reductions
#repeat for numTreeLevels
  #  -- tree level 0
  #    for tile in tiles do
  #      Reduce(0, 1, [partitionFragment0LeftChildLevel0][tile], [partitionFragment0RightChildLevel0][tile])
  #        Reduce(0, 1, [partitionFragment1LeftChildLevel0][tile], [partitionFragment1RightChildLevel0][tile])
  #--- etc for more fragments
  #  end
  #
  #    -- tree level 1
  #      for tile in tiles do
  #        Reduce(1, 2, [partitionFragment0LeftChildLevel1][tile], [partitionFragment0RightChildLevel1][tile])
  #        Reduce(1, 2, [partitionFragment1LeftChildLevel1][tile], [partitionFragment1RightChildLevel1][tile])
  #--- etc for more fragments
  #  end



#viz.rg:-- CODEGEN: partitionFragmentXByDepth_argList
#partitionFragment0ByDepth[tile],
#  partitionFragment1ByDepth[tile])






def doCodegen(numFragments, numTreeLevels):
  for inputLine in sys.stdin:
    sourceLine = inputLine[:-1]
    print sourceLine
    words = sourceLine.strip().split(' ')
    for i in range(len(words)):
      if(words[i] == 'CODEGEN:'):
        generateCode(words[i + 1], numFragments, numTreeLevels)




class MyParser(argparse.ArgumentParser):
  def error(self, message):
    self.print_usage(sys.stderr)
    print 'error', message
    sys.exit(2)

parser = MyParser(description = 'codegen: autogenerate fragment code for soleil-x visualization')
parser.add_argument('--numFragments', dest='numFragments', action='store', type=int, help='number of fragments (1...2160)', default=1)
parser.add_argument('--numTreeLevels', dest='numTreeLevels', action='store', type=int, help='number of tree levels (1..15)', default=1)
args = parser.parse_args()
doCodegen(args.numFragments, args.numTreeLevels)
