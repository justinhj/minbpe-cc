#include "Tokenizer.h"
#include <fstream>
#include <iostream>
#include <CLI/CLI.hpp>

using std::string;
using std::cout;

using MinBpeCC::Tokenizer::Tokenizer;

// Originally a port of https://github.com/karpathy/minbpe/blob/master/train.py
// then gradually optimized.

// Note the default is to train on the taylorswift sample, but if you pass in an integer less
// than the number of test strings I will run that one.
// Each entry is either text to train on or if you prefix with FILE: it will load the file instead.
const string test_strings[] = {
  "FILE:data/taylorswift.txt",
  "FILE:data/sample.txt",
  "FILE:data/shakespeare.txt",
  "FILE:data/bible.txt",
  "FILE:data/small.txt",
  "abcdebce",
  "Ｕｎｉｃｏｄｅ! 🅤🅝🅘🅒🅞🅓🅔‽ 🇺‌🇳‌🇮‌🇨‌🇴‌🇩‌🇪! 😄 The very name strikes fear and awe into the hearts of programmers worldwide. We all know we ought to “support Unicode” in our software (whatever that means—like using wchar_t for all the strings, right?). But Unicode can be abstruse, and diving into the thousand-page Unicode Standard plus its dozens of supplementary annexes, reports, and notes can be more than a little intimidating. I don’t blame programmers for still finding the whole thing mysterious, even 30 years after Unicode’s inception.",
  "", // empty string
  "?", // single character
  "안 👋 hello", // also from the youtube video
  "hello world!!!? (안녕하세요!) lol123 😉", // fun small string
  "But Unicode can be abstruse plus we know we are still finding the whole thing mysterious",
};

string getTestString(int index) {
  if (index < 0 || index >= sizeof(test_strings) / sizeof(test_strings[0])) {
    std::cerr << "Index out of bounds." << std::endl;
    return "";
  }
  string str = test_strings[index];
  const string fileIndicator = "FILE:";
  if (str.find(fileIndicator) != str.npos) {
    std::string file_path = str.substr(fileIndicator.length());
    std::ifstream file(file_path);
    std::string file_content;

    if (file) {
      cout << "Loading input file: " << file_path << "\n";
      file_content.assign((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
      file.close();
      return file_content;
    } else {
      std::cerr << "Could not open file: " << file_path << std::endl;
      return "";
    }
  }
  cout << str << " does not begin with " << fileIndicator << "\n";
  return str;
}

int main(int argc, char *argv[]) {
  using std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  CLI::App app{"Train a tokenizer on a test string"};
  argv = app.ensure_utf8(argv);

  long test_index = 0;
  app.add_option("-i,--test-string-index", test_index, "Index of the test string to use")
     ->required();

  std::string conflict_resolution_str = "first"; // Default value
  app.add_option("-c,--conflict-resolution", conflict_resolution_str, "Conflict resolution strategy: 'first' or 'lexical'")
     ->check(CLI::IsMember({"first", "lexical"}));

  CLI11_PARSE(app, argc, argv);

  long num_elements = sizeof(test_strings) / sizeof(test_strings[0]);
  if (test_index < 0 || test_index >= num_elements) {
    std::cerr << "Error: test-string-index out of bounds. Must be between 0 and " << num_elements - 1 << std::endl;
    return 1;
  }

  auto t1 = high_resolution_clock::now(); // Record start time
  auto verbose = true;
  auto input = getTestString(test_index);

  int num_tokens = 512;

  MinBpeCC::Tokenizer::Tokenizer::CONFLICT_RESOLUTION conflict_resolution;
  if (conflict_resolution_str == "first") {
    conflict_resolution = MinBpeCC::Tokenizer::Tokenizer::CONFLICT_RESOLUTION::FIRST;
  } else {
    conflict_resolution = MinBpeCC::Tokenizer::Tokenizer::CONFLICT_RESOLUTION::LEXICAL;
  }

  Tokenizer bt;
  bt.train(input, num_tokens, conflict_resolution, verbose);

  Tokenizer rt(Tokenizer::GPT4_SPLIT_PATTERN);
  rt.train(input, num_tokens, conflict_resolution, verbose);

  auto t2 = high_resolution_clock::now(); // Record end time
  auto duration = t2 - t1;
  auto ms_int = duration_cast<milliseconds>(duration).count();

  std::cout << "Execution time: " << ms_int / 1000.0 << " seconds" << std::endl;
}