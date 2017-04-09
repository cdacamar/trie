cmake_minimum_required(VERSION 3.1)

project(trie_benchmark)

include_directories(./)
include_directories(../include)

if (POLICY CMP0054)
  cmake_policy(SET CMP0054 OLD)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(DEBUG_FLAGS "/GS /W3 /Gm- /Zi /Od /Oy- /Ob0 /D\"_DEBUG\" /MDd")
  set(RELEASE_FLAGS "/GS /GL /W3 /Gy /Zi /O2 /MD")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
else()
  set(DEBUG_FLAGS "-g -O0 -Wall -Wextra -Werror -std=c++1z")
  set(RELEASE_FLAGS "-O3 -Wall -Wextra -Werror -std=c++1z")
endif()

set(CMAKE_CXX_FLAGS_DEBUG ${DEBUG_FLAGS})
set(CMAKE_CXX_FLAGS_RELEASE ${RELEASE_FLAGS})

set(CMAKE_CONFIGURATION_TYPES Debug Release)

add_definitions(-DCATCH_CONFIG_MAIN)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  message(STATUS "Build type unspecified. Defaulting type: ${CMAKE_BUILD_TYPE}")
endif (NOT CMAKE_BUILD_TYPE)

file(GLOB_RECURSE MAIN RELATIVE_PATH
   main.cpp)
source_group("test" FILES ${MAIN})

set(SOURCE
  ${MAIN})

add_executable(trie_test ${SOURCE})

set_property(TARGET trie_test PROPERTY FOLDER "test")
