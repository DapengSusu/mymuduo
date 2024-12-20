#!/bin/bash

set -e

# Check if the script is running as root
if [ -z "$SUDO_USER" ]; then
    echo "please run this script with sudo"
    exit 1
fi

# Set the build directory
BUILD_DIR=$(pwd)/build

# Build
cd ${BUILD_DIR} && make

# Check if the build is successful
if [ $? -ne 0 ]; then
    exit $?
fi

cd -

# Install
MYMUDUO_INCLUDE_INSTALL_DIR=/usr/include/mymuduo
MYMUDUO_LIB_INSTALL_DIR=/usr/lib

if [ ! -d ${MYMUDUO_INCLUDE_INSTALL_DIR} ]; then
    mkdir -p ${MYMUDUO_INCLUDE_INSTALL_DIR}
# else
    # echo "Existed dir:${MYMUDUO_INCLUDE_INSTALL_DIR}, clean it"
    # rm -rf ${MYMUDUO_INCLUDE_INSTALL_DIR}/*
fi

# Copy the header files
# for header in $(ls *.h); do
#     cp ${header} ${MYMUDUO_INCLUDE_INSTALL_DIR}
# done
# Copy the shared library
cp lib/libmymuduo.so ${MYMUDUO_LIB_INSTALL_DIR}

# Update the shared library cache
ldconfig

ulimit -c unlimited

if [ "$1" == "r" ]; then
    cd example && ./testserver
else
    cd example && make clean && make && ./testserver
fi
