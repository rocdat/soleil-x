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
source do.bash 1


OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
mkdir -p ${OUTDIR}
cd src
rm -rf piz_daint_jobs/Job_4
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 4 2,2,2 taylor_green_vortex_512_512_256.lua 4 3 00:10:00 ${OUTDIR}