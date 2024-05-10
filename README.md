# minbpe-cc

> What is the real root of suffering? **Tokenization**
>
> _Andrej Karpathy_

## What is this?

A C++ implementation of bpe tokenization, based on Karpathy's [minbpe](https://github.com/karpathy/minbpe).

This is a fairly direct port of the Python code, and is quite a bit faster. Compared to the Python `train.py` example it is roughy 5x faster.

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

https://github.com/karpathy/minbpe
https://github.com/Genivia/RE-flex
https://github.com/glample/fastBPE/tree/master

## Optimization notes

May 3rd

bible.txt (4.2Mb) 512 tokens took 63 seconds in release
8.5 seconds per iteration

wikitext (514Mb) about 45 seconds for first iteration (count 15622355)
6 hours for full training?
then  11137719, 9702736, 9322647

May 5th 2024

After some optimization on the C++ side ran a comparative test of Karpathy's train.py with shakespeare.txt:

- minbpe-cc/train 32.859 seconds
- minbpe/train/.py 157.96 seconds

## TODO Notes and C++ related

* WIP Remove oop
* TODO Use a nested namespace called detail to hide non public implementation details
* TODO Use zip/tail to simplify the tricky pair iterator logic and see if it impairs performance
* TODO add urls as a valid input for the training data
* DONE optimize sorted output by using a vector containing each pair and a pointer to the map key/value
* DONE Move towards building as a library with examples

### Optimizations

#### Training

May 6th timings (no verbose, release build)

Train shakespeare 20s
Train taylorswift 3.3s


* TODO When calculating the most frequent pair can you track the changes iteratively in a map instead of doing the actual swaps in a big vector
