#include "Tokenizer.hpp"
#include <climits>
#include <iostream>
#include <ranges>
#include <vector>

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
  std::tuple<int,int> most_frequent_pair(const std::vector<int> &text) {
    assert(text.size() > 1);
    if(text.size() == 2) {
      return std::make_tuple(text[0], text[1]);
    }
    /* for(std::tuple<int,int> elem: std::views::zip(text, &text[1])) { } */
    auto i1 = text.begin();
    auto i2 = ++text.begin();
    unordered_map<merge_key_t, int, decltype(key_hash_lambda)> freqs(10, key_hash_lambda);
    int max = 0;
    std::tuple<int,int> max_pair;
    while(i1 != text.end() && i2 != text.end()) {
      auto p = std::make_tuple(*i1, *i2);
      int f = 1;
      if(freqs.find(p) == freqs.end()) {
        freqs[p] = f;
      } else {
        freqs[p]++;
        f = freqs[p];
      }
      if(f >= max) {
        max = f;
        max_pair = p;
        /* cout << "new max " << max << "pair " << std::get<0>(p) << " " << std::get<1>(p) << "\n"; */
      }
      ++i1;
      ++i2;
    }
    return max_pair;
  }

  // replace all consecutive occurences of pair with the new token idx
  void merge(std::vector<int> &text, std::tuple<int,int> mp, int idx) {
    if(text.size() < 2) {
      return;
    }
    auto new_text = std::vector<int>();
    auto i1 = text.begin();
    auto i2 = ++text.begin();
    auto i = 0;
    auto skip = false;
    while(i1 != text.end() && i2 != text.end()) {
      if(skip) {
        skip = false;
        i++;
        i1++;
        i2++;
        continue;
      }
      auto p = std::make_tuple(*i1,*i2);
      if(p == mp) {
        skip = true;
        new_text.push_back(idx);
      } else {
        new_text.push_back(*i1);
        if(i == text.size() - 2) {
          new_text.push_back(*i2);
        }
      }
      i++;
      i1++;
      i2++;
    }
    text = new_text;
  }

  public:
  // Warning for compatibility with common tokenizers it is assumed the input is in 
  // utf-8 encoding.
  void train(const u8string &text, const int vocab_size, const bool verbose) { 
    // Show input text, a bit too verbose unless you're debugging
    /* if(verbose) { */
    /*   for(int c : text) { */
    /*     cout << char_to_int(c) <<  ' '; */
    /*   } */
    /*   cout << "\n"; */
    /* } */
    std::vector<int> text_converted;
    for(auto c : text) {
      text_converted.push_back(static_cast<int>(char_to_int(c)));
    }
    // TODO
    // 7. build the vocab which just requires the merges 

    for(auto i=UCHAR_MAX + 1; i < vocab_size; i++) {
      if(text_converted.size() < 2) {
        break;
      }
      // Just to debug
      /* for(int c : text_converted) { */
      /*   cout << c <<  ' '; */
      /* } */
      /* cout << "\n"; */
      auto mp = most_frequent_pair(text_converted);
      if(verbose) {
        cout << "merge pair " << std::get<0>(mp) << ", " << std::get<1>(mp) << " with new token " << i << "\n";
      }
      merge(text_converted, mp, i);
      merges[mp] = i;
    }
    if(verbose) {
      cout << "length of text " << text.size() << " after merges " << text_converted.size() << "\n";
    }
  };
};


