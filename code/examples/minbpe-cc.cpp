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

expected<void,string> write_file_to_string(const path &path, const string &data) {
  std::ofstream file(path);
  if(!file) {
    std::error_code ec(errno, std::generic_category());
    return unexpected(ec.message());
  }
  file << data;
  return {};
}

// Write out as 16 bit unsigned integers, this could be configurable. Requires thought,
// it is based on the size of the vocabulary, but we could have a larger vocabulary.
expected<void,string> save_encoding(const path &path, const vector<int> encoded) {
  std::ofstream file(path, std::ios::binary);
  if(!file) {
    std::error_code ec(errno, std::generic_category());
      return unexpected(ec.message());
  } else {
    for (const auto& code : encoded) {
      assert(code >= 0);
      assert(code < UINT16_MAX); // TODO oof
      uint16_t c = static_cast<uint16_t>(code);
      file.write(reinterpret_cast<const char *>(&c), sizeof(uint16_t));
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
    int number;
    while(file.read(reinterpret_cast<char *>(&number), sizeof(uint16_t))) {
      data.push_back(number);
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

  bool train;
  app.add_flag("-t,--train", train, "Train on the input");

  bool decode;
  app.add_flag("-d,--decode", decode, "Decode the input");

  bool encode;
  app.add_flag("-e,--encode", encode, "Encode the input");

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
    auto model_fspath = path(model_path);
    cout << "Training using file " << input_fspath << " encoder " << encoder << " vocab size " << vocab_size << " model path " << model_path << "\n";

    auto input = load_file_to_string(input_path);
    if(input.has_value()) {
      rt.train(input.value(), vocab_size, verbose);
      rt.save(model_fspath);
    } else { 
       cerr << "Failed to load training input file: " << input.error() << "\n";
    }
  }
  else if(encode) {
    auto model_fspath = path(model_path);
    auto output_fspath = path(output_path);

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
      auto result = write_file_to_string(output_fspath, decoded);
    } else {
      cerr << "Failed with error: " << input.error() << "\n";
    }
  }

  auto t2 = high_resolution_clock::now(); // Record end time
  auto duration = t2 - t1;
  auto ms_int = duration_cast<milliseconds>(duration).count();

  std::cout << "Execution time: " << ms_int / 1000.0 << " seconds" << std::endl;
}
