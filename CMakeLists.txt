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

include_directories(code)
include_directories(/opt/CLI11)
include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

# ./installed/x64-osx/share/re2/re2Config.cmake
# ./installed/x64-osx/share/re2/re2ConfigVersion.cmake

set(re2_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/re2")
set(absl_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/absl")
find_package(Boost COMPONENTS regex REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(re2 CONFIG REQUIRED)

add_executable(minbpe-cc code/main.cpp code/Tokenizer.cpp)
target_link_libraries(minbpe-cc PRIVATE Boost::regex re2::re2)
