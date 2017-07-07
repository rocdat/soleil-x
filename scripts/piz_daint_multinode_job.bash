#!/bin/bash -l
#SBATCH --job-name=PSAAP_taylor_green_vortex
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=00:10:00
#SBATCH --nodes=4
#SBATCH --ntasks-per-core=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --partition=normal
#SBATCH --constraint=mc

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export GASNET_BACKTRACE=1
export LD_LIBRARY_PATH="$PWD:$PWD/../../legion/bindings/terra/:$OSMESA_PATH/lib"
export REALM_BACKTRACE=1
root_dir="$PWD"
OUTDIR=/scratch/snx3000/aheirich/runX

if [[ ! -d ${OUTDIR} ]]; then mkdir ${OUTDIR}; fi
pushd ${OUTDIR}
mkdir out

for n in 1 2 3 4; do
  CASE="case$n"
  echo Running $CASE
  srun -n $n -N $n --ntasks-per-node 1 --cpu_bind none /lib64/ld-linux-x86-64.so.2 ~/PSAAP/soleil-x/src/soleil.exec -ll:cpu 8 -ll:util 1 -ll:dma 2 -ll:csize 50000 -hl:sched -1 -level legion_prof=2,5 -lg:prof_logfile prof_%_logfile -hl:prof 1 | tee out_${CASE}.log
done

popd


