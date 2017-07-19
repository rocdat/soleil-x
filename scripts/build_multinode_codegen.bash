#!/bin/bash

if [[ "`pwd`" != "$SOLEIL_PATH/src" ]]
then
  echo === Run this from $SOLEIL_PATH/src ===
  exit -1
fi 

NUM_FRAGMENTS=$1
if [[ "$NUM_FRAGMENTS" == "" ]]
then
  NUM_FRAGMENTS=2
fi

NUM_TREE_LEVELS=$2
if [[ "$NUM_TREE_LEVELS" == "" ]]
then
  NUM_TREE_LEVELS=2
fi

TESTCASE=$3
if [[ "$TESTCASE" == "" ]]
then
  TESTCASE=${SOLEIL_PATH}/testcases/taylor_with_smaller_particles/taylor_green_vortex_256_256_256.lua
  #cat ${SOLEIL_PATH}/testcases/taylor_with_smaller_particles/taylor_green_vortex_256_256_256_modified.lua| sed -e "s:max_iter =.*:max_iter = 25,:" > testcase.lua
  #TESTCASE=testcase.lua
fi


echo === Build ${NUM_FRAGMENTS} fragments, ${NUM_TREE_LEVELS} tree levels, test case ${TESTCASE} ===

START=`date`
echo start ${START}
cd $SOLEIL_PATH/src 
git checkout soleil_viz 

EXEC=soleil_${NUM_FRAGMENTS}_${NUM_TREE_LEVELS}.exec

source $SOLEIL_PATH/scripts/codegen.bash ${NUM_FRAGMENTS} ${NUM_TREE_LEVELS}

for f in algebraic.rg particles_init_uniform.rg soleil_mapper.*
do
  if [[ ! -e tmp_src/$f ]]
  then
    ln $f tmp_src/$f
  fi
done

cd tmp_src

USE_HDF=0 DEBUG=0 SAVEOBJ=1 OBJNAME=${EXEC} \
	$LISZT_PATH/liszt-legion.sh $SOLEIL_PATH/src/tmp_src/soleil-x.t \
	-i ${TESTCASE} \
	-fparallelize 1 \
	-fparallelize-dop 2,2,1 \
#	-fflow-spmd 1 \
#-fflow-spmd-shardsize 1

echo start ${START}
echo end `date`

