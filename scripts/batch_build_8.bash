#!/bin/bash -l
#SBATCH --job-name=build_soleil_viz
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --partition=normal

ROOT=/users/aheirich
cd $ROOT
source setup.bash
cd PSAAP
source do.bash 2


OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
mkdir -p ${OUTDIR}
cd src
rm -rf piz_daint_jobs/Job_8
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 8 4,2,2 taylor_green_vortex_512_512_512.lua 8 4 00:10:00 ${OUTDIR}

