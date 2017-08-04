#!/bin/bash

NODES=$1
MESH=$2
TESTCASE=$3
NUM_FRAGMENTS=$4
NUM_TREE_LEVELS=$5
TIME_LIMIT=$6
OUTDIR=$7

if [[ "$TESTCASE" == "" ]]
then
  TESTCASE=taylor_green_vortex_256_256_256.lua
fi

cd $SOLEIL_PATH/src

$SOLEIL_PATH/scripts/build_multinode_openmp.bash $NUM_FRAGMENTS $NUM_TREE_LEVELS $SOLEIL_PATH/testcases/taylor_with_smaller_particles/$TESTCASE $MESH

JOBID=Job_$NODES
mkdir -p ${OUTDIR}/${JOBID}
EXEC=soleil_${NUM_FRAGMENTS}_${NUM_TREE_LEVELS}.exec
mv ./${EXEC} ${OUTDIR}/${JOBID}
mv ./*.so ${OUTDIR}/${JOBID}
SCRIPT="${OUTDIR}/${JOBID}/${JOBID}_piz_daint.bash"
cat $SOLEIL_PATH/scripts/piz_daint_multinode_job.bash | \
  sed -e "s/TIME_LIMIT/${TIME_LIMIT}/g" | \
  sed -e "s/TESTCASE/${TESTCASE}/g" | \
  sed -e "s/NODES/${NODES}/g" | \
  sed -e "s/JOBID/${JOBID}/g" | \
  sed -e "s/EXEC/${EXEC}/g" | \
  sed -e "s/SCRIPT/${SCRIPT}/g" \
  > ${SCRIPT}

