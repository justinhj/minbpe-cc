#include "Tokenizer.hpp"

using std::string;
using std::unordered_map;

class BasicTokenizer : Tokenizer {
  BasicTokenizer() :: Tokenizer();
  
  void train(const string &text, const int vocab_size, const bool verbose) { };

}


