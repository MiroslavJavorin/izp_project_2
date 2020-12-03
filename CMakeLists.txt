cmake_minimum_required(VERSION 3.15)
project(project_2)
set(CMAKE_C_STANDARD 99)

set(GCC_COVERAGE_COMPILE_FLAGS " -Wall -Wextra -pedantic -D SHOWALL ")
#set(GCC_COVERAGE_COMPILE_FLAGS " -Wall -Wextra -pedantic -D CMDS -D SELECT -D SHOWTAB ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

add_executable(untitled spsWOUTREFRACTORING.c neWdef.c sps.c)