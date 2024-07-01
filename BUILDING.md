# Notes on building the project

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