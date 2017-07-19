#!/bin/bash

NUM_FRAGMENTS=$1
NUM_TREE_LEVELS=$2

if [[ "$NUM_FRAGMENTS" == "" ]]
then
  NUM_FRAGMENTS = 1
  NUM_TREE_LEVELS=1
fi

if [[ "$NUM_TREE_LEVELS" == "" ]]
then
  NUM_TREE_LEVELS=1
fi

echo === Generate sources for $NUM_FRAGMENTS fragments, $NUM_TREE_LEVELS tree levels

DIR=tmp_src
mkdir -p ${DIR}

for SOURCE in soleil-x.t viz.rg viz.h viz.cc 
do
  echo === ${SOURCE} ===
  python codegen.py --numFragments ${NUM_FRAGMENTS} --numTreeLevels ${NUM_TREE_LEVELS} < ${SOURCE} > ${DIR}/${SOURCE}
done

