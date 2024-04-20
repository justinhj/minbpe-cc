# minbpe-cc

> What is the real root of suffering? **Tokenization**
>
> _Andrej Karpathy_

## What is this?

A C++ implementation of bpe tokenization, based on Karpathy's [minbpe](https://github.com/karpathy/minbpe).

This is a fairly direct port of the Python code, and is quite a bit faster. Compared to the Python `train.py` example it is roughy 4x faster.

28 seconds in Python train.py
7.1 seconds in C++

## Building

Examples to build with the ninja build system as release or debug. The `compile_commands.json` output is to help the ccls lsp server and other tools, you may omit it otherwise.

```
cmake -S . -B ninjabuildrelease -G Ninja -DCMAKE_BUILD_TYPE=release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang -DVCPKG_TARGET_TRIPLET=x64-osx
```


```
cmake -S . -B ninjabuildrelease -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ln -fs ninjabuildrelease/compile_commands.json compile_commands.json
```

```
cmake -S . -B ninjabuilddebug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
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

Build it

```
ninja -C ninjabuild[release|debug]
```

## Running

After building, the executable will be in the build or release folder named `minbpe-cc`.

For help run `minbpe-cc --help`.

## Code style

For naming I am using `PascalCase` for class names and constructors. `snake_case` for everything else.

Source file names should use `PascalCase` apart from executables or test which can be `snake-case`.

## Usage

Can run in three modes, train, encode and decode, given by the mode parameter.

Parameters

--input The input file name for training, encoding or decoding
--train Train the input file and output the model data
--encode Encode the input file using provided model data
--decode Decode the input file using provided model data
--model-path Where save and load the models
--model-prefix Optional prefix to distinguish models
--verbose Prints extra information during the processing

Train

Input: filename to train on.
Output: A csv file of merges and vocabulary. By default the files will be named [filename]-merges.csv and [filename]-vocab.csv.
Merges would be three columns: `10,20,256` where the first two columns are the pair and the last column is the new token.
Vocabulary would be `256,'ab'` Where the first column is the token index and the second column is the bytes to emit encoded in some suitable form. TBD.

Encode

Input: filename to encode.
Output: loads the merges and dictionary, uses it to encode the input and writes it to an output file.

Decode 

Input: filename to decode.
Output: loads the merges and dictionary, uses it to decode the input and writes the original data to the output file.

## References


https://github.com/Genivia/RE-flex
https://github.com/glample/fastBPE/tree/master

## TODO Notes and C++ related

TODO Move towards building as a library with examples
TODO Use a nested namespace called detail to hide non public implementation details
TODO Use zip/tail to simplify the tricky pair iterator logic and see if it impairs performance

