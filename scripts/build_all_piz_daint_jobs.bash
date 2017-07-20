#!/bin/bash
cd $SOLEIL_PATH/src
OUTDIR=$SOLEIL_PATH/src/piz_daint_jobs
mkdir -p ${OUTDIR}

# nodes mesh testcase numfragments numtreelevels
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 4 2,2,1 taylor_green_vortex_256_256_256.lua 4 2 00:10:00 ${OUTDIR}
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 8 4,2,1 taylor_green_vortex_512_256_256.lua 8 3 00:10:00 ${OUTDIR}
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 16 4,4,1 taylor_green_vortex_512_512_256.lua 16 4 00:10:00 ${OUTDIR}
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 32 4,4,2 taylor_green_vortex_512_512_512.lua 32 5 00:10:00 ${OUTDIR}
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 64 8,4,2 taylor_green_vortex_1024_512_512.lua 64 6 00:10:00 ${OUTDIR}
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 128 8,8,2 taylor_green_vortex_1024_1024_512.lua 128 7 00:10:00 ${OUTDIR}
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 256 8,8,4 taylor_green_vortex_1024_1024_1024.lua 256 8 00:10:00 ${OUTDIR}
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 512 16,8,4 taylor_green_vortex_2048_1024_1024.lua 512 9 00:10:00 ${OUTDIR}
$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 1024 16,16,4 taylor_green_vortex_2048_2048_1024.lua 1024 10 00:10:00 ${OUTDIR}
#$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 2048 16,16,8 taylor_green_vortex_2048_2048_2048.lua 2048 11 00:10:00 ${OUTDIR}
#$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 4096 32,16,8 taylor_green_vortex_4096_2048_2048.lua 4096 12 00:10:00 ${OUTDIR}
#$SOLEIL_PATH/scripts/build_one_piz_daint_job.bash 8192 32,32,8 taylor_green_vortex_4096_4096_2048.lua 8192 13 00:10:00 ${OUTDIR}
