# minbpe-cc

> What is the real root of suffering? **Tokenization**
>
> _Andrej Karpathy_

## What is this?

A C++ implementation of bpe tokenization, based on Karpathy's [minbpe](https://github.com/karpathy/minbpe).

This is a fairly direct port of the Python code, and is quite a bit faster. Compared to the Python `train.py` example it is roughy 4x faster.

28 seconds in Python train.py
7.1 seconds in C++ (Homebrew clang version 17.0.6)

## Building

The project uses cmake and the vcpkg package manager to manage dependencies. For utf-8 friendly regexes that support negative lookahead (required by GPT tokenizers), I need use the combination of Boost and the Reflex regex library. Reflex is not available in vcpkg, so you will need to install it manually.

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

In general there are three running modes, `train`, `encode`, and `decode`.

In this example we train a model on the Taylor Swift wiki page and save it.

```
minbpe-cc --train --input ./data/taylorswift.txt -m ./models/taylorswift-gpt4.model  --vocab-size 512 --encoder gpt4
```

Then we can encode some text with the model, in this case the same text we trained on.

```
minbpe-cc --encode --input ./data/taylorswift.txt -m ./models/taylorswift-gpt4.model  --vocab-size 512 --encoder gpt4 --output taylorencoded --verbose
```

Finally we can decode the tokens back to the original text.

```
minbpe-cc --decode --input taylorencoded -m ./models/taylorswift-gpt4.model  --vocab-size 512 --encoder gpt4 --output taylororiginal.txt --verbose
```

## Code style

For naming I am using `PascalCase` for class names and constructors. `snake_case` for everything else.

Source file names should use `PascalCase` apart from executables or test which can be `snake-case`.

## References

https://github.com/Genivia/RE-flex
https://github.com/glample/fastBPE/tree/master

## TODO Notes and C++ related

WIP Move towards building as a library with examples
TODO Use a nested namespace called detail to hide non public implementation details
TODO Use zip/tail to simplify the tricky pair iterator logic and see if it impairs performance
TODO add urls as a valid input for the training data
DESIGN look at C++ 23 `flat_map` to make an ordered container for the merges so they can be saved in order

### Optimizations

When calculating the most frequent pair can you track the changes iteratively
