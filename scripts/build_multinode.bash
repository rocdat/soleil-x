#!/bin/bash

if [[ "`pwd`" != "$SOLEIL_PATH/src" ]]
then
  echo === Run this from $SOLEIL_PATH/src ===
fi 

TESTCASE=$1
if [[ "$1" == "" ]]
then
  TESTCASE=cavity/cavity_32x32.lua
fi

echo === Build test case ${TESTCASE} ===

cd $SOLEIL_PATH/src ; git checkout soleil_viz ; USE_HDF=0 DEBUG=1 SAVEOBJ=1 OBJNAME=soleil.exec $LISZT_PATH/liszt-legion.sh $SOLEIL_PATH/src/soleil-x.t -i $SOLEIL_PATH/testcases/${TESTCASE} -fparallelize 1 -fparallelize-dop 2,2,1
