#!/bin/bash

export BUILDROOT_SRC_DIR=/opt/eltay-sc-sdk-2025.09-linux66-20260928

cmake . -B build \
        -DCMAKE_TOOLCHAIN_FILE=$BUILDROOT_SRC_DIR/buildroot/output/host/share/buildroot/toolchainfile.cmake \
        -DELCORE_CMAKE_TOOLCHAIN_FILE=$BUILDROOT_SRC_DIR/buildroot/output/host/opt/toolchain-elcore50/share/cmake/elcore50-toolchain.cmake \
        -DElcoreRuntime_DIR=$BUILDROOT_SRC_DIR/buildroot/output/staging/usr/lib/cmake/ElcoreRuntime \
        -DElcoreCL_DIR=$BUILDROOT_SRC_DIR/buildroot/output/staging/usr/lib/cmake/ElcoreCL \

cmake --build build
