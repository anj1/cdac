#!/bin/sh

EXPECTED_ARGS=1
if [ $# -ne $EXPECTED_ARGS ]
then
  echo "Please specify output directory."
  exit 1
fi

autc2 robie.c $1/robie
autc2 extract2.c $1/extract2
autc2 robie-present.c $1/robie-present
autc2 cellcmp.c $1/cellcmp
