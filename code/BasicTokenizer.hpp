#include "Tokenizer.hpp"
#include <iostream>
#include <ranges>

using std::u8string;
using std::unordered_map;
using std::cout;
using std::endl;

class BasicTokenizer : public Tokenizer {
  public:
  BasicTokenizer() : Tokenizer() {};
  
  protected:
  // Count the frequencies of all pairs and return the most frequently occuring
  std::tuple<int,int> most_frequent_pair(const u8string &text) {
    /* for(std::tuple<int,int> elem: std::views::zip(text, &text[1])) { */
    /* } */
    return std::make_tuple(1,1);
  }

  public:
  // Warning for compatibility with common tokenizers it is assumed the input is in 
  // utf-8 encoding.
  void train(const u8string &text, const int vocab_size, const bool verbose) { 
    if(verbose) {
      for(int c : text) {
        c = c < 0 ? c + 256 : c; 
        cout << c <<  ' ';
      }
      cout << "\n";
    }
  };

};


