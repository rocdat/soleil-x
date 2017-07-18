#!/bin/bash

if [[ "`pwd`" != "$SOLEIL_PATH/src" ]]
then
  echo === Run this from $SOLEIL_PATH/src ===
fi 

TESTCASE=$1
if [[ "$1" == "" ]]
then
  cat ${SOLEIL_PATH}/testcases/taylor_with_smaller_particles/taylor_green_vortex_256_256_256_modified.lua| sed -e "s:max_iter =.*:max_iter = 25,:" > testcase.lua
  TESTCASE=testcase.lua
fi

echo === Build test case ${TESTCASE} ===

START=`date`
echo start ${START}
cd $SOLEIL_PATH/src 
git checkout soleil_viz 
USE_HDF=0 DEBUG=0 SAVEOBJ=1 OBJNAME=soleil.exec \
	$LISZT_PATH/liszt-legion.sh $SOLEIL_PATH/src/soleil-x.t \
	-i ${TESTCASE} \
	-fparallelize 1 \
	-fparallelize-dop 2,2,1 \
#	-fflow-spmd 1 \
#-fflow-spmd-shardsize 1

echo start ${START}
echo end `date`
