# Notes on building the project

## Building with zig build
Note that Zig is not required to build the project if you use cmake, but the file build.zig is offered as a an alternative build system. It is not as feature complete as cmake, but it is simpler and faster to use. It is also a good way to test the zig compiler and its features.

The C++ build makes assumptions about the library locations, but with the zig build you need to explicitly specify each. 

The libraries required are:

Boost (regex component) - Used for regular expression functionality.
RE-flex - A regex library (linked as libreflex.a).
CLI11 - A command-line parsing library, used by the minbpe-cc executable.
Catch2 - A testing framework, used by the test executable (specifically Catch2WithMain for test entry point).

Recommended configuration:

Create a `.env` file in the root of the project with the following content, or adjust for you configuration:

```bash
DEFAULT_LIB_PATH=../../../../usr/local/lib
DEFAULT_INCLUDE_PATH=../../../../usr/local/include
```

Then you can build the project with the following command:

```bash
zig build
```

The executables will be in `zig-out/bin/` and you can run the tests with `zig build test`

## Buiding with cmake
Note my experience with cmake is not extensive, so if you run into issues with the cmake build being too specific for my setup I would recommend the zig build instead which is much simpler to configure.

Since I use cmake you need to first install it and your build system of choice. I use ninja, but you can use make or other build systems that cmake supports.

You run cmake to create the build files for you preferred build system in a folder, then build it from there. Some examples I have used during development on Mac OS are below for convenience.

The project uses cmake and the vcpkg package manager to manage dependencies. For utf-8 friendly regexes that support negative lookahead (required by GPT tokenizers), I need use the combination of Boost and the Reflex regex library. Reflex is not available in vcpkg, so you will need to install it manually.

The `compile_commands.json` output is to help the ccls lsp server and other tools, you may omit it otherwise.

Default toolchain
```
cmake -S . -B ninjabuildrelease -G Ninja -DCMAKE_BUILD_TYPE=release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DVCPKG_TARGET_TRIPLET=arm64-osx

cmake -S . -B ninjabuilddebug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DVCPKG_TARGET_TRIPLET=arm64-osx

ln -fs ninjabuilddebug/compile_commands.json compile_commands.json
```

For the release build with clang from llvm
```
cmake -S . -B ninjabuildrelease -G Ninja -DCMAKE_BUILD_TYPE=release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ \
    -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang \
    -DVCPKG_TARGET_TRIPLET=arm64-osx
```

```
cmake -S . -B ninjabuilddebugclang -G Ninja -DCMAKE_BUILD_TYPE=debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang -DVCPKG_TARGET_TRIPLET=arm64-osx
```
gnu C++ from brew
```
cmake -S . -B ninjabuilddebug -G Ninja  \
	-DCMAKE_BUILD_TYPE=Debug  \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON  \
	-DVCPKG_TARGET_TRIPLET=arm64-osx  \
	-DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-14  \
	-DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-14 \
    -DCMAKE_LIBRARY_PATH=/opt/homebrew/lib \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++ -I/opt/homebrew/include/c++/14"

cmake -S . -B ninjabuilddebuggcc -G Ninja  \
	-DCMAKE_BUILD_TYPE=Debug  \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON  \
	-DVCPKG_TARGET_TRIPLET=arm64-osx-gcc  \
	-DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-14  \
	-DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-14 \
    -DCMAKE_LIBRARY_PATH=/opt/homebrew/lib \
    -DVCPKG_OVERLAY_TRIPLETS=../justinhj-triplets \
    -DCMAKE_LIBRARY_PATH=/opt/homebrew/lib \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++ -I/opt/homebrew/include/c++/14"

ln -fs ninjabuilddebug/compile_commands.json compile_commands.json
 
```

```
cmake -S . -B ninjabuildrelease -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ln -fs ninjabuildrelease/compile_commands.json compile_commands.json
```

```
cmake -S . -B ninjabuilddebug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DVCPKG_TARGET_TRIPLET=arm64-osx
ln -fs ninjabuilddebug/compile_commands.json compile_commands.json
```

Example build config with Clang 

```
cmake -S . -B ninjabuilddebug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang
ln -fs ninjabuilddebug/compile_commands.json compile_commands.json
```

```
cmake -S . -B ninjabuildrelease -G Ninja -DCMAKE_BUILD_TYPE=release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang
ln -fs ninjabuildrelease/compile_commands.json compile_commands.json
```

```
cmake -S . -B ninjabuildreleasegcc -G Ninja \
    -DCMAKE_BUILD_TYPE=release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DVCPKG_TARGET_TRIPLET=arm64-osx \
    -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-14 \
    -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-14

cmake -S . -B ninjabuilddebuggcc -G Ninja \
    -DCMAKE_BUILD_TYPE=debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DVCPKG_TARGET_TRIPLET=arm64-osx \
    -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-14 \
    -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-14
```

Build it

```
ninja -C ninjabuild[release|debug]
```
