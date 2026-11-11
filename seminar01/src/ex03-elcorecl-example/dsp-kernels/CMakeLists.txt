cmake_minimum_required(VERSION 3.10)

project(dsp-kernel.elf C)

find_package(ElcoreRuntime REQUIRED)

add_executable(${PROJECT_NAME} dsp-kernel.c)
target_link_libraries(${PROJECT_NAME} PRIVATE elcore-runtime)
target_compile_options(${PROJECT_NAME} PRIVATE -O3)

install(TARGETS ${PROJECT_NAME} DESTINATION .)
