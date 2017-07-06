#!/bin/bash
module unload PrgEnv-cray
module load PrgEnv-gnu
module load daint-mc
unset LG_RT_DIR

cd ${LEGION_PATH}/language
CC=cc CXX=CC HOST_CC=gcc HOST_CXX=g++ scripts/setup_env.py 
mv terra.build terra.build.master
git clone -b luajit2.1 https://github.com/magnatelee/terra.git terra.build
cd terra.build
CC=gcc CXX=g++ make LLVM_CONFIG=`readlink -f ../llvm/install/bin/llvm-config` CLANG=`readlink -f ../llvm/install/bin/clang` -j
cd ..

