#!/usr/bin/env bash

./create-compilation-database.sh
export CLANG_CHECK=clang-check-4.0
export COMPILE_DB=$(pwd)
grep file compile_commands.json \
    | awk '{ print $2; }' \
    | sed 's/\"//g' \
    | while read FILE; do
        echo "checking $FILE"
        (cd $(dirname $FILE); $CLANG_CHECK -analyze -p $COMPILE_DB $(basename $FILE))
    done
