#!/bin/bash
cd $SOLEIL_PATH/src
OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
mkdir -p ${OUTDIR}

# run with 2 cpus per node

# nodes mesh testcase numfragments numtreelevels

# 4 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 4 2,2,1 taylor_green_vortex_512_512_256.lua 4 2 00:10:00 ${OUTDIR}

# 8 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 8 2,2,2 taylor_green_vortex_512_512_512.lua 8 3 00:10:00 ${OUTDIR}

# 16 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 16 4,2,2 taylor_green_vortex_1024_512_512.lua 16 4 00:05:00 ${OUTDIR}

# 32 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 32 4,4,2 taylor_green_vortex_1024_1024_512.lua 32 5 00:05:00 ${OUTDIR}

# 64 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 64 4,4,4  taylor_green_vortex_1024_1024_1024.lua 64 6 00:05:00 ${OUTDIR}

# 128 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 128 8,4,4 taylor_green_vortex_2048_1024_1024.lua 128 7 00:05:00 ${OUTDIR}

# 256 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 256 8,8,4   taylor_green_vortex_2048_2048_1024.lua 256 8 00:05:00 ${OUTDIR}

# 512 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 512 8,8,8   taylor_green_vortex_2048_2048_2048.lua 512 9 00:05:00 ${OUTDIR}

# 1024 nodes openmp
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 1024 16,8,8  taylor_green_vortex_4096_2048_2048.lua 1024 10 00:05:00 ${OUTDIR}
