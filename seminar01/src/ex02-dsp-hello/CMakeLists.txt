cmake_minimum_required(VERSION 3.10)

project(dsp-hello-world.elf C)

find_package(ElcoreRuntime REQUIRED)

add_executable(${PROJECT_NAME} dsp-hello-world.c)
target_link_libraries(${PROJECT_NAME} PRIVATE elcore-runtime)
target_compile_options(${PROJECT_NAME} PRIVATE -O3)
