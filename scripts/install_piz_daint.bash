#!/bin/bash
module unload PrgEnv-cray
module load PrgEnv-gnu
module load daint-mc
unset LG_RT_DIR

cd ${LEGION_PATH}/language
rm -rf terra.build
CC=cc CXX=CC HOST_CC=gcc HOST_CXX=g++ scripts/setup_env.py 
rm -rf terra.build.master
mv terra.build terra.build.master
git clone -b luajit2.1 https://github.com/magnatelee/terra.git terra.build
rm -rf terra
ln -s terra.build terra
cd terra.build
CC=gcc CXX=g++ make LLVM_CONFIG=`readlink -f ../llvm/install/bin/llvm-config` CLANG=`readlink -f ../llvm/install/bin/clang` -j
cd ..

mkdir -p $OSMESA_PATH
cd $OSMESA_PATH
curl https://mesa.freedesktop.org/archive/older-versions/8.x/8.0/MesaLib-8.0.tar.gz > MesaLib-8.0.tar.gz
gunzip MesaLib-8.0.tar.gz
tar xvf MesaLib-8.0.tar
cd Mesa-8.0
./configure --enable-osmesa --disable-driglx-direct --disable-dri --with-gallium-drivers=swrast
gmake
rm -f $OSMESA_PATH/lib
ln -s $OSMESA_PATH/Mesa-8.0/lib $OSMESA_PATH/lib

