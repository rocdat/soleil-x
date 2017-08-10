#!/bin/bash -l
#SBATCH --job-name=build_256
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --mem=120GB

ROOT=/users/aheirich
cd $ROOT
source setup.bash
cd PSAAP
source soleil-x/scripts/do.bash 7


OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
mkdir -p ${OUTDIR}
cd src
rm -rf piz_daint_jobs/Job_256
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 256 8,8,4 taylor_green_vortex_2048_2048_1024.lua 20 8 00:10:00 7 ${OUTDIR}

