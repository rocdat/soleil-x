#!/bin/bash -l
#SBATCH --job-name=PSAAP_cavity32x32_local
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=00:10:00
#SBATCH --nodes=2
#SBATCH --ntasks-per-core=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --partition=normal
#SBATCH --constraint=mc

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export GASNET_BACKTRACE=1
export LD_LIBRARY_PATH="$PWD:$PWD/../../legion/bindings/terra/"
export REALM_BACKTRACE=1
root_dir="$PWD"

if [[ ! -d runX ]]; then mkdir runX; fi
pushd runX

for n in 1 2; do
        echo "Running $n""x10..."
        srun -n $n -N $n --ntasks-per-node 1 --cpu_bind none /lib64/ld-linux-x86-64.so.2 ~/PSAAP/soleil-x/src/soleil.exec -seq_init 0 -par_init 1 -print_ts 1 -prune 5 -ll:cpu 10 -ll:io 1 -ll:util 2 -ll:dma 2 -ll:csize 30000 -ll:rsize 0 -ll:gsize 0 -hl:sched -1 -hl:prof 4 -level legion_prof=2 -lg:prof_logfile prof_"$n"x10_%.log | tee out_"$n"x10.log
done

popd


