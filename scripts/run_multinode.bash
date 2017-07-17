#!/bin/bash
rm -f ~/soleil.exec ~/librender.so ~/libsoleil_mapper.so
ln -s $SOLEIL_PATH/src/soleil.exec ~
ln -s $SOLEIL_PATH/src/librender.so ~
ln -s $SOLEIL_PATH/src/libsoleil_mapper.so ~
rm -rf ~/out
mkdir -p ~/out

GASNET_BACKTRACE=1 LD_LIBRARY_PATH=.:$LEGION_PATH/bindings/terra/:/usr/lib/x86_64-linux-gnu/ \
	mpirun \
		-bind-to none \
		-npernode 1 \
		-H n0000,n0001,n0002,n0003 \
		-x LD_LIBRARY_PATH \
		-x GASNET_BACKTRACE \
	./soleil.exec \
		-ll:cpu 4 \
		-ll:util 1 \
		-ll:dma 2 \
		-ll:csize 50000 \
		-lg:prof 4 \
		-lg:prof_logfile soleil_prof_log
