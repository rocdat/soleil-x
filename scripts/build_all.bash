#!/bin/bash

if [[ "`pwd`" != "$SOLEIL_PATH/src" ]]
then
  echo === Run this from $SOLEIL_PATH/src ===
fi

rm .tmp_testcases
find $SOLEIL_PATH/testcases -name *.lua | sed -e "s:${SOLEIL_PATH}/testcases/::" > .tmp_testcases
#
while read TESTCASE
do
  DIR=`echo ${TESTCASE} | sed -e "s:/.*::"`
  echo DIR is $DIR TESTCASE is $TESTCASE
  TESTNAME=`echo ${TESTCASE} | sed -e "s:$DIR/::" | sed -e "s:.lua::"`
  DESTINATION=$SOLEIL_PATH/bin/multinode/
  DESTINATION_DIR=$DESTINATION/$DIR/$TESTNAME
  mkdir -p $DESTINATION_DIR
  $SOLEIL_PATH/scripts/build_multinode.bash $TESTCASE
  cp soleil.exec $DESTINATION_DIR/
  cp lib*.so $DESTINATION_DIR
done < .tmp_testcases

