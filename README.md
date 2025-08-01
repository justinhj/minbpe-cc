# minbpe-cc

> What is the real root of suffering? **Tokenization**
>
> _Andrej Karpathy_

## What is this?

A C++ implementation of bpe tokenization, based on Karpathy's [minbpe](https://github.com/karpathy/minbpe).

This was originally a direct port of the Python code, which was quite a bit faster. Compared to the Python `train.py` example it is roughy 5x faster.

However, after making some data structure changes and other optimizations, it is now faster by a factor of 50x, or almost 2 orders of magniture.

## References on BPE for NLP
### Paper referenced in Kaparthy's video
[Neural Machine Translation of Rare Words with Subword Units](https://arxiv.org/pdf/1508.07909)

Mentions this optimization:
"In practice, we increase efficiency by indexing all ppairs, and updating data structures incrementally".

Implementation from the authors:
<https://github.com/rsennrich/subword-nmt>

## Chat gpt2 encoder
<https://github.com/openai/gpt-2/blob/master/src/encoder.py>

## On choosing the next pair to merge

To decide which pair to merge next when multiple pairs have the same frequency in BPE (Byte Pair Encoding) tokenization, there are a few common approaches:

1. Lexicographic order: Choose the pair that comes first alphabetically (numerically really).
2. First occurrence: Select the pair that appears earliest in the text.
3. Length of subwords: Prefer merging shorter subwords over longer ones.
4. Random selection: Randomly choose among the tied pairs.
5. Tie-breaking heuristics: Use additional criteria like the frequency of individual tokens in the pair.

The choice often depends on the specific implementation and the goals of the tokenization process.

In my case I have two implementations for this conflict resolution which you can choose from when running the bpe process.

- **First Occurence**: This is the default and matches Karpathy's original implementation. It chooses the conflicting pairs based which was first added to the Python dictionary. This is simple to implement because it is the default behaviour of Python dictionaries since Raymond Hettinger's implementation in Python 3.6+.
- **Lexicographic Order**: By using lexicographic ordering I can optimize the training process by incrementally updating the frequency counts and still give deterministic results. This is the greatest speedup in this implementation.

## Building

See [BUILDING.md](BUILDING.md) for more detailed instructions.

## Running

After building, the executable will be in the build or release folder named `minbpe-cc`.

For help run `minbpe-cc --help`.

In general there are three running modes, `train`, `encode`, and `decode`.

In this example we train a model on the Taylor Swift wiki page and save it.

```
mkdir ./models
minbpe-cc --train --input ./data/taylorswift.txt --model-path ./models/taylorswift-gpt4.model  --vocab-size 512 --encoder gpt4
```

Then we can encode some text with the model, in this case the same text we trained on.

```
minbpe-cc --encode --input ./data/taylorswift.txt --model-path ./models/taylorswift-gpt4.model  --vocab-size 512 --encoder gpt4 --output taylorencoded --verbose
```

Finally we can decode the tokens back to the original text.

```
minbpe-cc --decode --input taylorencoded --model-path ./models/taylorswift-gpt4.model  --vocab-size 512 --encoder gpt4 --output taylororiginal.txt --verbose
```

## Code style

The implementation is C++23 and follows a modern C++ style with a focus on readability and maintainability, avoiding new and delete where possible, and using smart pointers for memory management.

For naming I am using `PascalCase` for class names and constructors. `snake_case` for everything else.

Source file names should use `PascalCase` apart from executables or test which can be `snake-case`.

## References

<https://github.com/karpathy/minbpe>
<https://github.com/glample/fastBPE>
<https://github.com/rsennrich/subword-nmt>

## Optimization and development notes

Jul 23 2025

Completed refactor so we can choose between completely different implementations of frequency counting based on the conflict resolution chosen.

__(Taken on a Macbook Pro with 16Gb and M1 Pro 10-core CPU)__
The results are based on several runs and the average is rounded to the nearest 0.5 seconds.

### Timings for train.cpp on shakespeare.txt

- Python train.py (77.5s)
- C++ (first occurrence) (36.5s)
- C++ (lexicographical) (1.50s)

### Timings for train.cpp on bible.txt

The results are based on several runs and the average is rounded to the nearest 0.5 seconds.

- Python train.py (298.5s)
- C++ (first occurrence) (107.0s)
- C++ (lexicographical) (5.5s)

Jul 22 2025

Reworked the code so it's fully compatible with Karpathy's example models for training (the model files are exactly the same). Working on a refactor so you can run the slower compatible version or the optimized version with a command line flag.

Time for taylorswitch train. 5.8s


Next steps:
Code cleanup. Rename PairCount. Code tidying.
Add the flash

Jul 21 2025

Revisiting the data structures used. I would like to restore the original algorithm of picking "first" inserted as the criteria for tie breaking, instead of the 


Jun 9th 2025

Realized in Debug mode I was hitting an assert when using the GPT4 regex. Whilst tracking it down I found that internationalization (ICU) was not working even though boost was built with it.

Turned out the best solution was to use the pcre2 regex library instead of boost. This matches up with the synatx used in Karpathy's original code, since he used the regex module as used in Tiktoken, which is Perl compatible.

Jun 6th 2025

After migrating to zig build ran train on shakespeare.txt with latest optimizations and ReleaseFast mode.
Switched test device to Macbook Pro with M1 Pro 10-core CPU

- minbpe-cc/train 1.8 seconds
- minbpe/train.py 11 seconds

May 3rd 2024

bible.txt (4.2Mb) 512 tokens took 63 seconds in release
8.5 seconds per iteration

wikitext (514Mb) about 45 seconds for first iteration (count 15622355)
6 hours for full training?
then  11137719, 9702736, 9322647

May 5th 2024

After some optimization on the C++ side ran a comparative test of Karpathy's train.py with shakespeare.txt:

- minbpe-cc/train 32.859 seconds
- minbpe/train.py 97.96 seconds

July 1st 2024

Incremental optimization on frequency count and added lexicographical ordering for tie breaks

shakespeare.txt (train 2)

- minbpe-cc/train 1.9 seconds 
- minbpe/train.py 97.96 seconds


## TODO Notes and C++ related

* TODO :Performance: check performance of going back to vector instead of forward_list when training in each mode
* TODO :Ergonomics: Warnings when specifying unused arguments. vocab size and encoder only matter for training
* TODO :Fun: Use zip/tail to simplify the tricky pair iterator logic and see if it impairs performance
* TODO :Verification: Add end to end test script comparing to Karpathy's train.py
* DONE Add special token support for encoding and decoding
* DONE Make build files more portable (Converting to zig build)
* DONE replace tuple with pair
* DONE Add lexicographic ordering for tie breaking pairs (note this is a bit slower)
* DONE option to save the vocab
* DONE Use namespaces
* DONE optimize sorted output by using a vector containing each pair and a pointer to the map key/value
* DONE Move towards building as a library with examples

## Conversion to Zig notes
Currently I have add a zig build file to allow the project to be built with that tool instead of cmake.The reasoning is I find it a lot simpler to work with.

This opens up the question of whether to reimplement in Zig.

* It helps me with my ongoing Zig learning.
* Curious how the performance compares.
* Possible that some of the data structures don't have mature implementations in the Zig library or ecosystem yet.

There is the option of a hybrid approach where the C++ code is used for the core functionality and Zig is used as the CLI. The rewrite can then be incremental.

### Optimizations

#### Training

May 6th timings (no verbose, release build)

Train shakespeare 20s
Train taylorswift 3.3s

May 31st 

Train shakespeare 1.52s
Train taylorswift 0.264s
Train bible 5.7s

* DONE When calculating the most frequent pair can you track the changes iteratively in a map instead of doing the actual swaps in a big vector
* IDEA Make a forward list of the whole training text in the regex versions. Mark the split points as special nodes so they are not counted as pairs or replaced during training. This avoids having to iterate over a vector of lists.
