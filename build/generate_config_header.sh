#!/usr/bin/env bash

if [ $# -lt 2 ]; then
    echo "usage: $(basename $0) <filename> <name=value> [name=value]..."
    exit 1
fi

FILENAME=$1
shift

mkdir -p $(dirname $FILENAME)

cat >$FILENAME <<EOF
#pragma once

EOF

IFS='='
while [ $# -ne 0 ]; do
    read -r key value <<< "${1}"
    echo "static const char * const ${key} = \"${value}\";" >> $FILENAME
    shift
done
unset IFS

echo >> $FILENAME
echo >> $FILENAME
