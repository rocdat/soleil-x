#!/bin/bash
cd $SOLEIL_PATH/src ; git checkout soleil_viz ; USE_HDF=0 DEBUG=1 SAVEOBJ=1 OBJNAME=soleil.exec $LISZT_PATH/liszt-legion.sh $SOLEIL_PATH/src/soleil-x.t render.cc -i $SOLEIL_PATH/testcases/cavity/cavity_32x32.lua -fparallelize 1
