#!/bin/bash -l
#SBATCH --job-name=PSAAP_taylor_green_vortex
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --mail-type=ALL
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
export LD_LIBRARY_PATH="$SOLEIL_PATH/src/piz_daint_jobs/JOBID/:$LEGION_PATH/bindings/terra/:$OSMESA_PATH/lib"
export REALM_BACKTRACE=1
export LEGION_BACKTRACE=1
export LEGION_FREEZE_ON_ERROR=1
RUNDIR=/scratch/snx3000/aheirich/runX/JOBID

rm -rf ${RUNDIR}
mkdir ${RUNDIR}
pushd ${RUNDIR}
mkdir -p out

echo === run lscpu on all nodes ===
srun -n NODES lscpu

COMMAND=" \
srun -n NODES \
        -N NODES \
        --ntasks-per-node 1 \
        --cpu_bind none \
        /lib64/ld-linux-x86-64.so.2 \
	$SOLEIL_PATH/src/piz_daint_jobs/JOBID/EXEC \
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
        | tee JOBID.log "

echo === run the simulation ===
echo ${COMMAND}
echo ${COMMAND} | /bin/bash

popd

