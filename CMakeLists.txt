cmake_minimum_required(VERSION 3.12)
project(minbpe-cc)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Note I have conifgured vcpkg but it can be removed if I don't use any libraries by the time that the code is complete
if(DEFINED ENV{VCPKG_ROOT})
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "CMake toolchain file")
else()
  message(FATAL_ERROR "VCPKG_ROOT is not defined!")
endif()

include_directories(code)
include_directories(/opt/CLI11)
include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
add_executable(minbpe-cc code/main.cpp code/Tokenizer.cpp)
