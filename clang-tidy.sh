#!/usr/bin/env bash

./create-compilation-database.sh
CLANG_TIDY=run-clang-tidy-4.0.py
CHECKS="-*,bugprone-*,cert-*,cppcoreguidelines-*,clang-analyzer-*,misc-*,modernize-*,performance-*,readability-*"

if [[ -n $(which $CLANG_TIDY) ]]; then
    $CLANG_TIDY -checks=$CHECKS
else
    export CLANG_TIDY=clang-tidy-4.0
    export COMPILE_DB=$(pwd)
    grep file compile_commands.json \
        | awk '{ print $2; }' \
        | sed 's/\"//g' \
        | while read FILE; do
            echo "checking $FILE"
            (cd $(dirname $FILE); $CLANG_TIDY -checks=$CHECKS -p $COMPILE_DB $(basename $FILE))
        done
fi
