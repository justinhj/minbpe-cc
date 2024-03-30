#include "Tokenizer.h"
#include <iostream>

using std::string;
using std::unordered_map;
using std::cout;
using std::endl;

class BasicTokenizer : public Tokenizer {
  public:
  BasicTokenizer() : Tokenizer() {};
  
  void train(const string &text, const int vocab_size, const bool verbose) { 

    cout << text << endl;
  };

};


