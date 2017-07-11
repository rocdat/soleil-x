#!/bin/bash
LD_LIBRARY_PATH=.:$LEGION_PATH/bindings/terra/:/usr/lib/x86_64-linux-gnu ./soleil.exec -ll:csize 3000

