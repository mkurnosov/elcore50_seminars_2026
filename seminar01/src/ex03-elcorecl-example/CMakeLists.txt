cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ecl-example)

set(ELCORE50_TOOLCHAIN_FILE
    ${HOST_DIR}/opt/toolchain-elcore50/share/cmake/elcore50-toolchain.cmake)
set(TOOLCHAIN_FILE ${HOST_DIR}/share/buildroot/toolchainfile.cmake)
set(ELCORERUNTIME_DIR ${STAGING_DIR}/usr/lib/cmake/ElcoreRuntime)
set(ELCORECL_DIR ${STAGING_DIR}/usr/lib/cmake/ElcoreCL)


find_package(ElcoreRuntime REQUIRED)
find_package(ElcoreCL REQUIRED)

elcoreproject_add(DSP_KERNELS dsp-kernels CMAKE_ARGS -DElcoreRuntime_DIR=${ElcoreRuntime_DIR})

add_executable(ecl-example ecl-example.cc)
target_link_libraries(ecl-example PRIVATE elcorecl)
