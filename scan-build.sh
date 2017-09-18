#!/usr/bin/env bash

VER=-4.0
SCAN_BUILD=scan-build$VER
SCAN_VIEW=scan-view$VER
REPORT_DIR=$(mktemp -d --suffix=-beehive-scan)

make clean
$SCAN_BUILD -o $REPORT_DIR make -j $(($(nproc) * 2))

# specify PYTHONPATH to get around 'Use of uninitialized value $ScanView' bug
PYTHONPATH=/usr/share/clang/$SCAN_VIEW/share:$PYTHONPATH $SCAN_VIEW $REPORT_DIR/$(ls $REPORT_DIR)
