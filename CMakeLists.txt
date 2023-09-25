cmake_minimum_required(VERSION 3.23)
project(my_db C)

set(CMAKE_C_STANDARD 11)

add_executable(my_db my_db.c my_db.h)
add_executable(test test.c)
add_executable(test_sscanf test_sscanf.c)
add_executable(test_strtok test_strtok.c)
