#!/bin/bash
rm -f ~/soleil.exec ~/librender.so ~/libsoleil_mapper.so
ln -s $SOLEIL_PATH/src/soleil.exec ~
ln -s $SOLEIL_PATH/src/librender.so ~
ln -s $SOLEIL_PATH/src/libsoleil_mapper.so ~

rm -rf ~/out
mkdir -p ~/out

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

LEGION_BACKTRACE=1 LEGION_FREEZE_ON_ERROR=1 GASNET_BACKTRACE=1 LD_LIBRARY_PATH=.:$LEGION_PATH/bindings/terra/:/usr/lib/x86_64-linux-gnu/:$SOLEIL_PATH/src/ \
	mpirun \
		-bind-to none \
		-npernode 1 \
		-H n0000,n0001,n0002,n0003 \
		-x LD_LIBRARY_PATH \
		-x GASNET_BACKTRACE \
		-x LEGION_BACKTRACE \
		-x LEGION_FREEZE_ON_ERROR \
	./soleil_${NUM_FRAGMENTS}_${NUM_TREE_LEVELS}.exec \
		-ll:cpu 2 \
		-ll:ocpu 1 \
		-ll:othr 8 \
		-ll:util 1 \
		-ll:dma 2 \
		-ll:csize 50000 \
		-lg:prof 4 \
		-lg:prof_logfile soleil_prof_log
