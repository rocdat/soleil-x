#!/bin/bash
if [[ -d $LEGION_PATH ]] ; then echo skipping legion.git ; else
#  git clone -b master https://github.com/StanfordLegion/legion.git $LEGION_PATH
  git clone -b soleil_viz https://github.com/StanfordLegion/legion.git $LEGION_PATH
fi
if [[ -d $LISZT_PATH ]] ; then echo skipping liszt-legion.git ; else
  git clone https://github.com/manopapad/liszt-legion.git $LISZT_PATH
fi
if [[ -d $SOLEIL_PATH ]] ; then echo skipping soleil-x; else
  git clone https://github.com/stanfordhpccenter/soleil-x.git $SOLEIL_PATH
fi
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
  else
    ./install.py
  fi
fi
