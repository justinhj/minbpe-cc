cmake_minimum_required(VERSION 3.12)
project(minbpe-cc)

# To improve portability of this build:
# - re-flex library location should not be hard coded
# - homebrew shouldn't be mentioned
# - vcpkg should be optional
# - boost location should also be optional

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

set(Catch2_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/Catch2")
set(CLI11_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cli11")

include_directories("/opt/homebrew/include")
include_directories("../RE-flex/include")
include_directories("code/include")
include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

find_package(CLI11 CONFIG REQUIRED)
find_package(Catch2 3 REQUIRED)
find_package(Boost COMPONENTS regex REQUIRED)
find_library(REFLEX libreflex.a PATHS ../RE-flex/lib REQUIRED)

add_executable(minbpe-cc code/examples/minbpe-cc.cpp)
target_link_libraries(minbpe-cc PRIVATE Boost::regex ${REFLEX} CLI11::CLI11)

add_executable(train code/examples/train.cpp)
target_link_libraries(train PRIVATE Boost::regex ${REFLEX})

add_executable(test code/test/test.cpp)
target_link_libraries(test PRIVATE Boost::regex ${REFLEX} Catch2::Catch2WithMain)
