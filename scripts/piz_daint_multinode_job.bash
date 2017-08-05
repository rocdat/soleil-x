#!/bin/bash -l
#SBATCH --job-name=TESTCASE
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=TIME_LIMIT
#SBATCH --nodes=NODES
#SBATCH --ntasks-per-core=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=16
#SBATCH --partition=normal
#SBATCH --constraint=mc

ROOT=/users/aheirich
cd $ROOT/PSAAP
source soleil-x/scripts/setup.bash

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export GASNET_BACKTRACE=1
export LD_LIBRARY_PATH="$SOLEIL_PATH/src/piz_daint_jobs/JOB_ID/:$LEGION_PATH/bindings/terra/:$OSMESA_PATH/lib"
export REALM_BACKTRACE=1
export LEGION_BACKTRACE=1
export LEGION_FREEZE_ON_ERROR=1
RUNDIR=/scratch/snx3000/aheirich/runX/JOB_ID

rm -rf ${RUNDIR}
mkdir -p ${RUNDIR}
pushd ${RUNDIR}
mkdir out

echo === run lscpu on all nodes ===
srun -n NODES lscpu

echo === copy this script to run dir ===
cp $SOLEIL_PATH/src/piz_daint_jobs/JOB_ID/JOB_ID_piz_daint.bash ${RUNDIR}/

COMMAND=" \
srun -n NODES \
        -N NODES \
        --ntasks-per-node 1 \
        --cpu_bind none \
        /lib64/ld-linux-x86-64.so.2 \
	$SOLEIL_PATH/src/piz_daint_jobs/JOB_ID/EXEC \
        -ll:cpu 2 \
        -ll:ocpu 1 \
        -ll:othr 8 \
        -ll:util 2 \
        -ll:dma 2 \
	-ll:io 1 \
        -ll:csize 60000 \
        -level barriers=2 \
        -logfile barriers_%.log \
        -hl:sched -1 \
        -level legion_prof=2,5 \
        -lg:prof_logfile soleil_prof_%.log \
        -hl:prof 4 \
        | tee JOB_ID.log "

echo === run the simulation ===
echo ${COMMAND}
echo ${COMMAND} | /bin/bash

echo === elapsed time ===
sacct -j ${SLURM_JOBID} -o elapsed

echo === execution complete, saving slurm log ===
cp slurm-${SLURM_JOBID}.out ${RUNDIR}/

popd

