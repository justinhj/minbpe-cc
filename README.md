# minbpe-cc

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

--filename the input file name for training, encoding or decoding
--mode train|encode|decode determines the execution mode
--encoder basic the kind of encoder, only basic is implemented so far
--dictionary-prefix determines the prefix of the output files (defaults to filename)
--dictionary-path allows you to specify where to load the dictionary data from if it is not cwd
--verbose prints extra information during the processing

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
