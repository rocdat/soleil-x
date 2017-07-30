#!/bin/bash

if [[ "`pwd`" != "$SOLEIL_PATH/src" ]]
then
  echo === Run this from $SOLEIL_PATH/src ===
  exit -1
fi

COMPILE_COMMAND=\
"CC=gcc CXX=g++ USE_HDF=0 DEBUG=0 SAVEOBJ=1 SAVE_MAPPER_ONLY=1 \
	$LISZT_PATH/liszt-legion.sh $SOLEIL_PATH/src/soleil-x.t "
echo $COMPILE_COMMAND
echo $COMPILE_COMMAND | /bin/bash
