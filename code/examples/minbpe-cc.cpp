#include "Tokenizer.hpp"
#include "BasicTokenizer.hpp"
#include "RegexTokenizer.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <expected>
#include "CLI11/CLI11.hpp"

using std::string;
using std::optional;
using std::expected;
using std::unexpected;
using std::filesystem::path;
using std::filesystem::exists;

expected<string,string> load_file_to_string(const path &path) {
  std::ifstream file(path);
  if (!file) {
      std::error_code ec(errno, std::generic_category());
      return unexpected(ec.message());
  }

  std::stringstream buffer;
  // TODO error handling
  buffer << file.rdbuf();
  return buffer.str();
}

// Command line training, encoding and decoding
int main(int argc, char *argv[]) {
  CLI::App app{"Training, encoding and decoding of tokens"};
  argv = app.ensure_utf8(argv);

  optional<string> input_path;
  app.add_option("-i,--input", input_path, "Input file path");

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

  bool verbose;
  app.add_flag("-v,--verbose", verbose, "Print more things");

  CLI11_PARSE(app, argc, argv);

  if(input_path.has_value()) {
    cout << "Input file " << input_path.value() << "\n";
  } else {
    cout << "Input file <stdin>\n";
  }

  using std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  auto t1 = high_resolution_clock::now(); // Record start time

  if(train && input_path.has_value()) {
    auto input_fspath = path(input_path.value());
    if(!exists(input_fspath)) {
      cout << "Could not find " << input_path.value();
      return -1;
    }

    std::unique_ptr<Tokenizer> rt;
    if(encoder == "gpt2") {
      rt =  std::make_unique<RegexTokenizer>(RegexTokenizer::GPT2_SPLIT_PATTERN);
    } else if(encoder == "gpt4") {
      rt =  std::make_unique<RegexTokenizer>(RegexTokenizer::GPT4_SPLIT_PATTERN);
    } else if(encoder == "basic") {
      rt =  std::make_unique<BasicTokenizer>();
    } else {
      cout << "Encoder should be one of: basic, gpt2 or gpt4\n";
      return -1;
    }

    auto model_fspath = path(model_path);
    cout << "Training using file " << input_fspath << " encoder " << encoder << " vocab size " << vocab_size << " model path " << model_path << "\n";

    auto input = load_file_to_string(input_path.value());
    if(input.has_value()) {
      rt->train(input.value(), vocab_size, verbose);
    } else { 
       cout << "Failed to load training input file: " << input.error() << "\n";
    }
    rt->train(input.value(), vocab_size, verbose);
    rt->save(model_fspath);
  }

  auto t2 = high_resolution_clock::now(); // Record end time
  auto duration = t2 - t1;
  auto ms_int = duration_cast<milliseconds>(duration).count();

  std::cout << "Execution time: " << ms_int / 1000.0 << " seconds" << std::endl;
}
