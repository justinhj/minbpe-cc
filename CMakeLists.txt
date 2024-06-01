cmake_minimum_required(VERSION 3.12)
project(minbpe-cc)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_ROOT})
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "CMake toolchain file")
else()
  message(FATAL_ERROR "VCPKG_ROOT is not defined!")
endif()

set(Boost_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
find_path(Boost_INCLUDE_DIR boost)

include_directories("/opt/homebrew/include")
include_directories("code/include")
include_directories("thirdparty")
include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

find_package(Boost COMPONENTS regex REQUIRED)

find_library(REFLEX libreflex_static_lib.a PATHS ../RE-flex/ninjabuildreleasegcc REQUIRED)

add_executable(minbpe-cc code/examples/minbpe-cc.cpp)
target_link_libraries(minbpe-cc PRIVATE Boost::regex ${REFLEX})

add_executable(train code/examples/train.cpp)
target_link_libraries(train PRIVATE Boost::regex ${REFLEX})

add_executable(test code/test/test.cpp)
target_link_libraries(train PRIVATE Boost::regex ${REFLEX})
