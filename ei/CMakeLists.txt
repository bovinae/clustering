cmake_minimum_required (VERSION 2.6) 
project (clustering)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")

find_package(Eigen2 REQUIRED)
include_directories(${Eigen2_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/../lib ${CMAKE_SOURCE_DIR}/../cppjieba/include)

# set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

link_directories(${CMAKE_SOURCE_DIR}/../lib/build/lib/ /usr/local/lib/ /usr/lib64/)

# aux_source_directory(. DIR_SRCS)

add_executable(ei ei.cpp error_code.cpp) # ${DIR_SRCS}
add_definitions(-std=c++11)
target_link_libraries(ei clustering ghttpd profiler pthread)
