#!/bin/bash
git clone -b master https://github.com/StanfordLegion/legion.git $LEGION_PATH
git clone https://github.com/manopapad/liszt-legion.git $LISZT_PATH
git clone https://github.com/stanfordhpccenter/soleil-x.git $SOLEIL_PATH
cd $LEGION_PATH/language
git clone -b luajit2.1 https://github.com/magnatelee/terra.git

if [[ "`hostname`" == "sapling" ]]
then
  CONDUIT=ibv ./install.py --gasnet
else
  HOSTNAME=`hostname`
  MYHOST=`echo ${HOSTNAME} | sed -e 's/daint.*/daint/'`
  if [[ "${MYHOST}" == "daint" ]]
  then
    ${SOLEIL_PATH}/scripts/install_piz_daint.bash
    mv terra.build terra.build.master
    CC=cc CXX=CC HOST_CC=gcc HOST_CXX=g++ scripts/setup_env.py
    git clone -b luajit2.1 https://github.com/magnatelee/terra.git terra.build

    cd terra.build
    CC=gcc CXX=g++ make LLVM_CONFIG=`readlink -f ../llvm/install/bin/llvm-config` CLANG=`readlink -f ../llvm/install/bin/clang` -j
    cd ..

  else
    ./install.py
  fi
fi
