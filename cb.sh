#!/bin/bash

if [ -d "build"  ]; then
    rm -rf build
    echo "Deleted 'build' directory."
fi

mkdir build && cd build && cmake .. && cmake --build .
