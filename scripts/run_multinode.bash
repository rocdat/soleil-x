#!/bin/bash
LD_LIBRARY_PATH=.:$LEGION_PATH/bindings/terra/ mpirun -H n0000,n0001,n0002,n0003 -x LD_LIBRARY_PATH ./soleil.exec -ll:cpu 8 -ll:util 1 -ll:dma 1 -ll:csize 30000

