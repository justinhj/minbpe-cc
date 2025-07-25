#!/bin/bash
set -e

mkdir -p tests

# Run the basic encoding test
./zig-out/bin/minbpe-cc --train --input data/shakespeare.txt --model-path tests/basic-model --vocab-size 512 --encoder basic --conflict-resolution lexical
./zig-out/bin/minbpe-cc --encode --input data/sample.txt --model-path tests/basic-model --output tests/sample-encoded-basic
./zig-out/bin/minbpe-cc --decode --input tests/sample-encoded-basic --model-path tests/basic-model --output tests/sample-decoded-basic
diff tests/sample-decoded-basic data/sample.txt

# Run the gpt4 regex with special tokens test
./zig-out/bin/minbpe-cc --train --input data/taylorswift.txt --special-tokens-path ./data/special1.txt --model-path tests/gpt4-model --vocab-size 512 --encoder gpt4 --conflict-resolution first
./zig-out/bin/minbpe-cc --encode --input data/specialtokensample.txt --model-path tests/gpt4-model --output tests/sample-encoded-gpt4
./zig-out/bin/minbpe-cc --decode --input tests/sample-encoded-gpt4 --model-path tests/gpt4-model --output tests/sample-decoded-gpt4
diff tests/sample-decoded-gpt4 data/specialtokensample.txt

echo Tests succeeded
