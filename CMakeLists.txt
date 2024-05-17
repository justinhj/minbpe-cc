cmake_minimum_required(VERSION 3.12)
project(minbpe-cc)

# TODO Switch to using the re-flex library from vcpkg if they add it, for now it should be added manually using your package manager

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_ROOT})
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "CMake toolchain file")
else()
  message(FATAL_ERROR "VCPKG_ROOT is not defined!")
endif()

if(DEFINED ENV{VCPKG_TARGET_TRIPLET})
  set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET})
else()
  message(FATAL_ERROR "VCPKG_TARGET_TRIPLET not defined!")
endif()

set(Boost_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
find_path(Boost_INCLUDE_DIR boost)

include_directories("/usr/local/include")
include_directories("/opt/homebrew/include")
include_directories("code/include")
include_directories("thirdparty")
include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

find_package(Boost COMPONENTS regex REQUIRED)

# TODO path to libs should be a variable not hard coded
# TODO add instructions to readme to install libreflex
# e.g., on Mac is brew install re-flex
add_executable(minbpe-cc code/examples/minbpe-cc.cpp code/lib/Tokenizer.cpp)
target_link_libraries(minbpe-cc PRIVATE Boost::regex "/opt/homebrew/lib/libreflex.a")

add_executable(train code/examples/train.cpp code/lib/Tokenizer.cpp)
target_link_libraries(train PRIVATE Boost::regex "/opt/homebrew/lib/libreflex.a")
