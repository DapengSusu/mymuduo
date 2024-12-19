#!/bin/bash

set -e

RELEASE=0

# Set the build directory
BUILD_DIR=$(pwd)/build

# Check if the build directory exists
if [ -d ${BUILD_DIR}  ]; then
    # Clean the build directory
    rm -rf ${BUILD_DIR}/*
else
    # Create the build directory
    mkdir ${BUILD_DIR}
fi

# Build
cd ${BUILD_DIR} && cmake .. && cmake --build .

# Check if the build is successful
if [ $? -ne 0 ]; then
    exit $?
fi

cd -

# Install
if [ "${RELEASE}" = "0" ]; then
    echo "-- Debug mode"

    MYMUDUO_INCLUDE_INSTALL_DIR=$(pwd)/../libmymuduo/include
    MYMUDUO_LIB_INSTALL_DIR=$(pwd)/../libmymuduo/lib
    if [ ! -d ${MYMUDUO_INCLUDE_INSTALL_DIR} ]; then
        mkdir -p ${MYMUDUO_INCLUDE_INSTALL_DIR}
    else
        echo "Existed dir:${MYMUDUO_INCLUDE_INSTALL_DIR}, clean it"
        rm -rf ${MYMUDUO_INCLUDE_INSTALL_DIR}/*
    fi

    for header in $(ls *.h); do
        cp ${header} ${MYMUDUO_INCLUDE_INSTALL_DIR}
    done

    if [ ! -d ${MYMUDUO_LIB_INSTALL_DIR} ]; then
        mkdir -p ${MYMUDUO_LIB_INSTALL_DIR}
    fi
    cp lib/libmymuduo.so ${MYMUDUO_LIB_INSTALL_DIR}
else
    echo "-- Release mode"

    MYMUDUO_INCLUDE_INSTALL_DIR=/usr/include/mymuduo
    MYMUDUO_LIB_INSTALL_DIR=/usr/lib

    if [ ! -d ${MYMUDUO_INCLUDE_INSTALL_DIR} ]; then
        sudo mkdir -p ${MYMUDUO_INCLUDE_INSTALL_DIR}
    else
        echo "Existed dir:${MYMUDUO_INCLUDE_INSTALL_DIR}, clean it"
        sudo rm -rf ${MYMUDUO_INCLUDE_INSTALL_DIR}/*
    fi

    for header in $(ls *.h); do
        sudo cp ${header} ${MYMUDUO_INCLUDE_INSTALL_DIR}
    done

    sudo cp lib/libmymuduo.so ${MYMUDUO_LIB_INSTALL_DIR}
fi

# Update the shared library cache
sudo ldconfig
