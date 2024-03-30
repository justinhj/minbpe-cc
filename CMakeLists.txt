cmake_minimum_required(VERSION 3.12)
project(minbpe-cc)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_ROOT})
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "CMake toolchain file")
else()
  message(FATAL_ERROR "VCPKG_ROOT is not defined!")
endif()

# set(Boost_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
# find_path(Boost_INCLUDE_DIR boost)

# find_package(Boost 1.82.0 REQUIRED COMPONENTS filesystem system thread)
# find_package(zstd CONFIG REQUIRED)
# find_package(Snappy CONFIG REQUIRED)

include_directories(code)
include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
   
add_executable(basic_tokenizer code/basic-tokenizer.cpp code/Tokenizer.cpp)
# target_link_libraries(ReadSparkeyData PRIVATE Boost::boost Boost::thread Boost::system Boost::filesystem Sparkey zstd::libzstd_static Snappy::snappy)

