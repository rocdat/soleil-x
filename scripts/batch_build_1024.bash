#!/bin/bash -l
#SBATCH --job-name=1024build
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=03:00:00
#SBATCH --nodes=1
#SBATCH --partition=normal

ROOT=/users/aheirich
cd $ROOT
source setup.bash
cd PSAAP
source soleil-x/scripts/do.bash 9


OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
mkdir -p ${OUTDIR}
cd src
rm -rf piz_daint_jobs/Job_1024
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 1024 16,8,8 taylor_green_vortex_4096_2048_2048.lua 2 11 01:00:00 9 ${OUTDIR}

