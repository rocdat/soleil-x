#!/bin/bash -l
#SBATCH --job-name=build_32
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --mem=120GB

ROOT=/users/aheirich
cd $ROOT
source setup.bash
cd PSAAP
source soleil-x/scripts/do.bash 4


OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
mkdir -p ${OUTDIR}
cd src
rm -rf piz_daint_jobs/Job_32
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 32 4,4,2 taylor_green_vortex_1024_1024_512.lua 32 5 00:10:00 4 ${OUTDIR}

