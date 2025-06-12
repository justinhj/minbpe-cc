#include <fstream>
#include <filesystem>
#include <ios>
#include <iostream>
#include <expected>

#include <CLI/CLI.hpp>

#include "Tokenizer.h"

using std::string;
using std::expected;
using std::unexpected;
using std::cerr;
using std::filesystem::path;
using std::filesystem::exists;

using namespace MinBpeCC::Tokenizer;

#include <vector>

// Utility function to split a string by a delimiter
std::vector<std::string> split_string(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    // Add the last token (or the whole string if delimiter not found)
    tokens.push_back(str.substr(start));
    return tokens;
}

expected<string,string> load_file_to_string(const path &path) {
  std::ifstream file(path);
  if(!file) {
      std::error_code ec(errno, std::generic_category());
      return unexpected(ec.message());
  }
  std::stringstream buffer;
  // TODO error handling
  buffer << file.rdbuf();
  return buffer.str();
}

expected<void,string> write_string_to_file(const path &path, const string &data) {
  std::ofstream file(path);
  if(!file) {
    std::error_code ec(errno, std::generic_category());
    return unexpected(ec.message());
  }
  file << data;
  return {};
}

// Write out as 32 bit unsigned integers, this could be configurable. Requires thought,
// it is based on the size of the vocabulary, but we could have a larger vocabulary.
// Maybe a variable length encoding would be better.
// I guess there is always variable length encoding
expected<void,string> save_encoding(const path &path, const vector<int> encoded) {
  std::ofstream file(path, std::ios::binary);
  if(!file) {
    std::error_code ec(errno, std::generic_category());
    return unexpected(ec.message());
  } else {
    for (const auto& code : encoded) {
      assert(code >= 0);
      assert(code <= UINT32_MAX);
      uint32_t c = static_cast<uint32_t>(code);
      cout << "writing " << c << "\n";
      file.write(reinterpret_cast<const char *>(&c), sizeof(uint32_t));
    }
    return {};
  }
}

expected<vector<int>,string> load_encoding(const path &path) {
    std::ifstream file(path, ios::binary);
    if (!file.is_open()) {
      std::error_code ec(errno, std::generic_category());
      return unexpected(ec.message());
    }
    vector<int> data;
    uint32_t number;
    while(file.read(reinterpret_cast<char *>(&number), sizeof(uint32_t))) {
      data.push_back(number);
      cout << "Read " << number << "\n";
    }

    if (file.fail() && !file.eof()) {
      std::error_code ec(errno, std::generic_category());
      return unexpected(ec.message());
    }
    cout << "Loaded encoding with " << data.size() << " tokens\n";
    return data;
}

// Command line training, encoding and decoding
int main(int argc, char *argv[]) {
  CLI::App app{"Training, encoding and decoding of tokens"};
  argv = app.ensure_utf8(argv);

  string input_path;
  app.add_option("-i,--input", input_path, "Path to the input to be trained on, encoded or decoded");

  string output_path;
  app.add_option("-o,--output", output_path, "Path for the output of the encoding or decoding");

  string special_token_path;
  app.add_option("-s,--special-tokens-path", special_token_path, "Path to the special tokens file");

  bool train = false;
  app.add_flag("-t,--train", train, "Train on the input");

  bool decode = false;
  app.add_flag("-d,--decode", decode, "Decode the input");

  bool encode = false;
  app.add_flag("-e,--encode", encode, "Encode the input");

  bool write_vocab = false;
  app.add_flag("-w,--write-vocab", write_vocab, "When training, write the vocabulary to a file");

  int vocab_size = 512;
  app.add_option("--vocab-size", vocab_size, "Vocabulary size");

  string encoder = "gpt4";
  app.add_option("--encoder", encoder, "Encoder to use from basic,gpt2,gpt4");

  string model_path = "./output.model";
  app.add_option("-m,--model-path", model_path, "Path to load or save the model");

  bool verbose = false;
  app.add_flag("-v,--verbose", verbose, "Print more things");

  CLI11_PARSE(app, argc, argv);

  auto input_fspath = path(input_path);
  if(!input_path.empty()) {
    if(!exists(input_fspath)) {
      cerr << "Input file " << input_path << " does not exist\n";
      return -1;
    }
  } else {
    cerr << "Input file not specified\n";
    return -1;
  }

  // Make a vector of strings to hold the special tokens
  auto special_token_fspath = path(special_token_path);
  expected <string,string> special_tokens_data;
  if(!special_token_path.empty()) {
    if(exists(special_token_fspath)) {
      special_tokens_data = load_file_to_string(special_token_path);
      // print success if loaded
      if(special_tokens_data.has_value()) {
        cout << "Loaded special tokens from " << special_token_path << "\n";
      } else {
        cerr << "Failed to load special tokens from " << special_token_path << ": " << special_tokens_data.error() << "\n";
      }
    }
  }

  using std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  auto t1 = high_resolution_clock::now(); // Record start time of operation

  string split_pattern;
  if(encoder == "gpt2") {
    split_pattern = Tokenizer::GPT2_SPLIT_PATTERN;
  } else if(encoder == "gpt4") {
    split_pattern = Tokenizer::GPT4_SPLIT_PATTERN;
  } else if(encoder == "basic") {
    split_pattern = "";
  } else {
    cout << "Encoder should be one of: basic, gpt2 or gpt4\n";
    return -1;
  }

  auto rt = Tokenizer(split_pattern);

  if(train) {
    if(special_tokens_data.has_value()) {
      rt.set_special_tokens_from_file(special_tokens_data.value());
    } else {
      cout << "No special tokens file provided\n";
    }

    auto model_fspath = path(model_path);
    cout << "Training using file " << input_fspath << " encoder " << encoder << " vocab size " << vocab_size << " model path " << model_path << "\n";


    if(verbose) {
        cout << "Loading file " << input_path << "\n";
    }
    auto input = load_file_to_string(input_path);
    if(input.has_value()) {
      if(verbose) {
          cout << "Starting training...\n";
      }
      rt.train(input.value(), vocab_size, verbose);
      rt.save(model_fspath, write_vocab);
    } else { 
       cerr << "Failed to load training input file: " << input.error() << "\n";
    }
  }
  else if(encode) {
    auto model_fspath = path(model_path);
    auto output_fspath = path(output_path);

    if(output_path.empty()) {
      cerr << "Output file not specified\n";
      return -1;
    }

    if(!exists(model_fspath)) {
      cerr << "Model file " << model_path << " does not exist\n";
      return -1;
    }

    cout << "Encoding input file " << input_fspath << " encoder " << encoder << " model path " << model_path << " output to " << output_path << "\n";
    rt.load(model_fspath, verbose);
    auto input = load_file_to_string(input_fspath);
    if(input.has_value()) {
      auto encoded = rt.encode(input.value(), verbose);

      cout << "Writing " << encoded.size() << " encoded tokens\n";
      auto result = save_encoding(output_fspath, encoded);
      if(result.has_value()) {
        cout << "Success\n";
      } else {
        cerr << "Failed with error: " << result.error() << "\n";
      }
    } else {
      cerr << "Failed with error: " << input.error() << "\n";
    }
  }
  else if(decode) {
    auto model_fspath = path(model_path);
    auto output_fspath = path(output_path);

    cout << "Decoding input file " << input_fspath << " encoder " << encoder << " model path " << model_path << " output to " << output_path << "\n";
    rt.load(model_fspath, verbose);
    auto input = load_encoding(input_fspath);
    if(input.has_value()) {
      auto decoded = rt.decode(input.value(), verbose);

      cout << "Writing " << decoded.size() << " decoded tokens to " << output_path << "\n";
      auto result = write_string_to_file(output_fspath, decoded);
    } else {
      cerr << "Failed with error: " << input.error() << "\n";
    }
  }

  auto t2 = high_resolution_clock::now(); // Record end time
  auto duration = t2 - t1;
  auto ms_int = duration_cast<milliseconds>(duration).count();

  std::cout << "Execution time: " << ms_int / 1000.0 << " (s)" << std::endl;
}
