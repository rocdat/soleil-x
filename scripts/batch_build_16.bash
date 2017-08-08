#!/bin/bash -l
#SBATCH --job-name=build_soleil_viz
#SBATCH --mail-user=aheirich@stanford.edu
#SBATCH --time=05:00:00
#SBATCH --nodes=1
#SBATCH --partition=normal

ROOT=/users/aheirich
cd $ROOT/PSAAP
source do.bash 3

cd src
git checkout -- viz.h
rm -f .x1
sed -e "s:GL/glu.h:glu.h:" < viz.h > .x1
cp .x1 viz.h


OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 16 4,4,2 taylor_green_vortex_1024_512_512.lua 16 5 00:10:00 ${OUTDIR}

