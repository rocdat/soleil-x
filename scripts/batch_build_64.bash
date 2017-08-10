#!/bin/bash -l
#SBATCH --job-name=build_64
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --mail-type=END
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --partition=normal
#SBATCH --mem=120GB

ROOT=/users/aheirich
cd $ROOT
source setup.bash
cd PSAAP
source soleil-x/scripts/do.bash 5


OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
mkdir -p ${OUTDIR}
cd src
rm -rf piz_daint_jobs/Job_64
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 64 4,4,4 taylor_green_vortex_1024_1024_1024.lua 32 6 00:10:00 5 ${OUTDIR}

