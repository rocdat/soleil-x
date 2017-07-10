#!/bin/bash
TESTCASE=$SOLEIL_PATH/src/movie.lua
rm -f ${TESTCASE}
cat $SOLEIL_PATH/testcases/taylor_with_smaller_particles/taylor_green_vortex_256_256_256.lua | sed -e "s/max_iter =.*/max_iter = 500/" > ${TESTCASE}
unset SAVE_RENDER_ONLY
$SOLEIL_PATH/scripts/build_multinode.bash ${TESTCASE}
$SOLEIL_PATH/scripts/run_multinode.bash 
