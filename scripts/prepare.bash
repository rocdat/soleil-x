#!/bin/bash
source $SOLEIL_PATH/scripts/setup.bash
git clone -b master https://github.com/StanfordLegion/legion.git $LEGION_PATH
git clone https://github.com/manopapad/liszt-legion.git $LISZT_PATH
git clone https://github.com/stanfordhpccenter/soleil-x.git $SOLEIL_PATH
cd $LEGION_PATH/language
git clone -b luajit2.1 https://github.com/magnatelee/terra.git

if [[ "`hostname`" == "sapling" ]]
then
  CONDUIT=ibv ./install.py --gasnet
else
  ./install.py
fi
