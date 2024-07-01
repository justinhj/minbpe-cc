pair minbpe-cc

> What is the real root of suffering? **Tokenization**
>
> _Andrej Karpathy_

## What is this?

A C++ implementation of bpe tokenization, based on Karpathy's [minbpe](https://github.com/karpathy/minbpe).

This is a fairly direct port of the Python code, and is quite a bit faster. Compared to the Python `train.py` example it is roughy 5x faster.

## References on BPE for NLP
[Neural Machine Translation of Rare Words with Subword Units](https://arxiv.org/pdf/1508.07909)

"In practice, we increase efficiency by indexing all ppairs, and updating data structures incrementally".

## On choosing the next pair to merge

To decide which pair to merge next when multiple pairs have the same frequency in BPE (Byte Pair Encoding) tokenization, there are a few common approaches:

1. Lexicographic order: Choose the pair that comes first alphabetically.
2. First occurrence: Select the pair that appears earliest in the text.
3. Length of subwords: Prefer merging shorter subwords over longer ones.
4. Random selection: Randomly choose among the tied pairs.
5. Tie-breaking heuristics: Use additional criteria like the frequency of individual tokens in the pair.

The choice often depends on the specific implementation and the goals of the tokenization process.

## Building

See [BUILDING.md](BUILDING.md) for more detailed instructions.

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
- minbpe/train.py 97.96 seconds

July 1st 2024

Incremental optimization on frequency count and added lexicographical ordering for tie breaks

shakespeare.txt (train 2)

- minbpe-cc/train 2.8 seconds 
- minbpe/train.py 


## TODO Notes and C++ related

* TODO Add special token support for encoding and decoding
* TODO Make build files more portable
* TODO check performance of going back to vector instead of forward_list when training in each mode
* TODO Warnings when specifying unused arguments. vocab size and encoder only matter for training
* TODO Use zip/tail to simplify the tricky pair iterator logic and see if it impairs performance
* TODO Add end to end test script
* DONE replace tuple with pair
* DONE Add lexicographic ordering for tie breaking pairs (note this is a bit slower)
* DONE option to save the vocab
* DONE Use namespaces
* DONE optimize sorted output by using a vector containing each pair and a pointer to the map key/value
* DONE Move towards building as a library with examples

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
