cmake_minimum_required(VERSION 3.14)
project(swserial_test)
add_compile_options(-Wall -Wextra)
include_directories(../src)
add_executable(swserial_test swserial_test.cpp ../src/bitreader.cpp)
