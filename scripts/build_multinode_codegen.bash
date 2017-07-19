#!/bin/bash

if [[ "`pwd`" != "$SOLEIL_PATH/src" ]]
then
  echo === Run this from $SOLEIL_PATH/src ===
fi 

TESTCASE=$1
if [[ "$1" == "" ]]
then
  TESTCASE=${SOLEIL_PATH}/testcases/taylor_with_smaller_particles/taylor_green_vortex_256_256_256_modified.lua
  #cat ${SOLEIL_PATH}/testcases/taylor_with_smaller_particles/taylor_green_vortex_256_256_256_modified.lua| sed -e "s:max_iter =.*:max_iter = 25,:" > testcase.lua
  #TESTCASE=testcase.lua
fi

echo === Build test case ${TESTCASE} ===

START=`date`
echo start ${START}
cd $SOLEIL_PATH/src 
git checkout soleil_viz 

EXEC=soleil_2_2.exec

source $SOLEIL_PATH/scripts/codegen.bash 2 2

for f in algebraic.rg particles_init_uniform.rg soleil_mapper.*
do
  ln $f tmp_src/$f
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

