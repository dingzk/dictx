cmake_minimum_required(VERSION 3.15)
project(dictx)

#使用C++ 11标准
set(CMAKE_CXX_COMPILER "g++" )
set(CMAKE_CXX_STANDARD 11) # cmake 3.1才支持

# C99 标准
set(CMAKE_C_STANDARD 99)

# 启用调试/发版模式
#SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_BUILD_TYPE Debug)
# 显示编译过程
set(CMAKE_VERBOSE_MAKEFILE ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


add_subdirectory(src)


add_executable(tests_mmap tests/mmap.cpp)
add_executable(tests_buffio tests/buffio.cpp)
add_executable(tests_shm_open1 tests/shm_open1.cpp)
add_executable(tests_shm_open2 tests/shm_open2.cpp)

target_link_libraries(tests_shm_open1 rt)
target_link_libraries(tests_shm_open2 rt)


