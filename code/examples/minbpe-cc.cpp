#include "BasicTokenizer.hpp"
#include "RegexTokenizer.hpp"
#include <fstream>
#include <iostream>
#include "CLI11/CLI11.hpp"

using std::string;
using std::optional;

// Command line training, encoding and decoding
int main(int argc, char *argv[]) {
  CLI::App app{"Training, encoding and decoding of tokens"};
  argv = app.ensure_utf8(argv);

  optional<string> input_path;
  app.add_option("-i,--input", input_path, "Input file path");

  bool train;
  app.add_flag("-t,--train", train, "Train on the input");

  CLI11_PARSE(app, argc, argv);

  if(input_path.has_value()) {
    cout << "Input file " << input_path.value() << "\n";
  } else {
    cout << "Input file <stdin>\n";
  }

  if(train) {
    cout << "Train!\n";
  } else {
    cout << "No Train!\n";
  }

  using std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  auto t1 = high_resolution_clock::now(); // Record start time
  auto verbose = true; // TODO this should be an option

  /* if(true) { */
  /*   BasicTokenizer bt; */
  /*   bt.train(input, 256 + 256, verbose); */
  /* } */

  /* if(true) { */
  /*   RegexTokenizer rt = RegexTokenizer(RegexTokenizer::GPT4_SPLIT_PATTERN); */
  /*   rt.train(input, 256 + 256, verbose); */
  /* } */

  auto t2 = high_resolution_clock::now(); // Record end time
  auto duration = t2 - t1;
  auto ms_int = duration_cast<milliseconds>(duration).count();

  std::cout << "Execution time: " << ms_int / 1000.0 << " seconds" << std::endl;

  if(false) {
    /* auto encoded = rt.encode(input, verbose); */
    /* cout << "Original string length " << input.size() << "\n"; */
    /* cout << "Encoded string length " << encoded.size() << "\n"; */
    /* auto decoded = rt.decode(encoded, false); */
    /* cout << "Decoded string length " << decoded.size() << "\n"; */

    /* cout << "Original == decoded: " << (decoded == input ? "Yes" : "No") << "\n"; */ 
  }
}
