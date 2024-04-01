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
  
  private:
  int char_to_int(char8_t c) {
    return c < 0 ? c + 256 : c; 
  }
  protected:
  // Count the frequencies of all pairs and return the most frequently occuring
  std::tuple<int,int> most_frequent_pair(const u8string &text) {
    assert(text.length() >= 2);
    /* for(std::tuple<int,int> elem: std::views::zip(text, &text[1])) { } */
    auto i1 = text.begin();
    auto i2 = ++text.begin();
    unordered_map<merge_key_t, int, decltype(key_hash_lambda)> freqs(10, key_hash_lambda);
    int max = 0;
    std::tuple<int,int> max_pair;
    while(i1 != text.end() && i2 != text.end()) {
      auto p1 = char_to_int(*i1);
      auto p2 = char_to_int(*i2);
      auto p = std::make_tuple(p1, p2);
      int f = 0;
      if(freqs.find(p) == freqs.end()) {
        freqs[p] = f;
      } else {
        freqs[p]++;
        f = freqs[p];
      }
      if(f > max) {
        max = f;
        max_pair = p;
      }
      ++i1;
      ++i2;
    }
    return max_pair;
  }

  public:
  // Warning for compatibility with common tokenizers it is assumed the input is in 
  // utf-8 encoding.
  void train(const u8string &text, const int vocab_size, const bool verbose) { 
    if(verbose) {
      for(int c : text) {
        cout << char_to_int(c) <<  ' ';
      }
      cout << "\n";
    }
    auto mp = most_frequent_pair(text);
    cout << "max pair " << std::get<0>(mp) << " " << std::get<1>(mp) << "\n";
  };

};


