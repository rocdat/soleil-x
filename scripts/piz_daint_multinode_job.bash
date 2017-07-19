#!/bin/bash -l
#SBATCH --job-name=PSAAP_taylor_green_vortex
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=TIME_LIMIT
#SBATCH --nodes=NODES
#SBATCH --ntasks-per-core=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --partition=normal
#SBATCH --constraint=mc

echo OSMESA_PATH $OSMESA_PATH LEGION_PATH $LEGION_PATH
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export GASNET_BACKTRACE=1
export LD_LIBRARY_PATH="$PWD:$PWD/../../legion/bindings/terra/:$OSMESA_PATH/lib"
export REALM_BACKTRACE=1
root_dir="$PWD"
RUNDIR=/scratch/snx3000/aheirich/runX

if [[ ! -d ${RUNDIR} ]]; then mkdir ${RUNDIR}; fi
pushd ${RUNDIR}
mkdir out

n=0
while [[ $n -lt NODES ]]
do
  srun -n $n -N $n --ntasks-per-node 1 --cpu_bind none /lib64/ld-linux-x86-64.so.2 ~/PSAAP/soleil-x/src/soleil.exec -ll:cpu 8 -ll:util 1 -ll:dma 2 -ll:csize 50000 -hl:sched -1 -level legion_prof=2,5 -lg:prof_logfile prof_%_logfile -hl:prof 1 | tee JOBID.log

  n=$(( $n+1 ))
done

popd


