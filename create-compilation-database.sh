#!/usr/bin/env bash

make clean
bear make -j $(($(nproc) * 2))
