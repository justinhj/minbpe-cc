#ifndef MINBPE_BASIC_TOKENIZER_HPP
#define MINBPE_BASIC_TOKENIZER_HPP

#include "Tokenizer.hpp"
#include <climits>
#include <iostream>
#include <ranges>
#include <tuple>
#include <vector>
#include <algorithm>
#include <cassert>

using std::string;
using std::unordered_map;
using std::cout;
using std::endl;
using std::tuple;
using std::make_tuple;

class BasicTokenizer : public Tokenizer {
  public:
  BasicTokenizer() : Tokenizer() {};
  
  private:
  int char_to_int(char8_t c) {
    return c < 0 ? c + 256 : c; 
  }

  protected:
  // Count the frequencies of all pairs and return the most frequently occuring
  tuple<int,int,int> most_frequent_pair(const std::vector<int> &text) {
    assert(text.size() > 1);
    if(text.size() == 2) {
      return make_tuple(text[0], text[1], 1);
    }
    // Would like to use a zip iterator here but doesn't seem to be support in Apple Clang yet
    /* for(tuple<int,int> elem: std::views::zip(text, &text[1])) { } */
    auto i1 = text.begin();
    auto i2 = ++text.begin();
    auto step = 0; // count the steps so we can pick the first added to the map
    // this is to work around unordered map not having insertion order maintained
    unordered_map<merge_key_t, tuple<int,int>, decltype(key_hash_lambda)> freqs(10, key_hash_lambda);
    while(i1 != text.end() && i2 != text.end()) {
      auto p = make_tuple(*i1, *i2);
      int f = 1;
      if(freqs.find(p) == freqs.end()) {
        freqs[p] = make_tuple(f,step);
      } else {
        auto tf = freqs[p];
        f = std::get<0>(freqs[p]);
        auto s = std::get<1>(freqs[p]);
        freqs[p] = make_tuple(++f,s);
      }
      ++i1;
      ++i2;
      ++step;
    }
    auto maxElementIt = std::max_element(freqs.begin(), freqs.end(),
        [](const std::pair<const tuple<int,int>, tuple<int,int>>& a, const std::pair<const tuple<int,int>, tuple<int,int>>& b) -> bool {
            auto av = std::get<0>(a.second); 
            auto as = std::get<1>(a.second); 
            auto bv = std::get<0>(b.second); 
            auto bs = std::get<1>(b.second); 
            if(av < bv) {
              return true;
            } else if (av == bv) {
              if(bs < as) {
                return true;
              } else {
                return false;
              }
            } else {
              return false;
            }
        });
    auto max_pair = *maxElementIt;
    auto return_pair = max_pair.first;
    auto max = std::get<0>(max_pair.second);
    return make_tuple(std::get<0>(return_pair), std::get<1>(return_pair), max);
  }
  // replace all consecutive occurences of pair with the new token idx
  void merge(std::vector<int> &text, tuple<int,int> mp, int idx) {
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
      auto p = make_tuple(*i1,*i2);
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
  std::vector<int> text_to_vector(const string &text) {
    std::vector<int> text_converted;
    for(auto c : text) {
      text_converted.push_back(static_cast<int>(char_to_int(c)));
    }
    return text_converted;
  }
  // given a string text, return the token ids
  vector<int> internal_encode(const vector<int> &text, const bool verbose) {
    vector<int> out;
    auto i = 0;
    auto merge_count = 0;
    auto len = text.size();
    while(i < len) {
      auto pair = make_tuple(text[i], text[i + 1]);
      if(i < len - 1 && merges.find(pair) != merges.end()) {
        out.push_back(merges[pair]);
        i += 2;
        merge_count ++;
      } else {
        out.push_back(text[i]);
        i ++;
      }
    }
    if(merge_count == 0) {
      return out;
    } else {
      return internal_encode(out, verbose);
    }
  }
  public:
  // Warning for compatibility with common tokenizers it is assumed the input is in 
  // utf-8 encoding.
  void train(const string &text, const int vocab_size, const bool verbose) { 
    // Show input text, a bit too verbose unless you're debugging
    /* if(verbose) { */
    /*   for(int c : text) { */
    /*     cout << char_to_int(c) <<  ' '; */
    /*   } */
    /*   cout << "\n"; */
    /* } */
    auto text_converted = text_to_vector(text);
    vocab.clear();
    for(auto i=0; i<(UCHAR_MAX + 1); i++) {
      vector<int> s;
      s.push_back(i);
      vocab[i] = s;
    }
    for(auto i=UCHAR_MAX + 1; i < vocab_size; i++) {
      if(text_converted.size() < 2) {
        break;
      }
      // Just to debug
      /* for(int c : text_converted) { */
      /*   cout << c <<  ' '; */
      /* } */
      /* cout << "\n"; */
      auto mp3 = most_frequent_pair(text_converted);
      if(verbose) {
        cout << "merge pair " << std::get<0>(mp3) << ", " << std::get<1>(mp3) << " with new token " << i << " count " << std::get<2>(mp3) << "\n";
      }
      auto mp = make_tuple(std::get<0>(mp3), std::get<1>(mp3));
      merge(text_converted, mp, i);
      merges[mp] = i;

      vector<int> new_vocab { vocab[std::get<0>(mp)] };
      const vector<int> &v2 = vocab[std::get<1>(mp)];
      new_vocab.insert(new_vocab.end(), v2.begin(), v2.end());
      vocab[i] = new_vocab;
    }
    if(verbose) {
      cout << "length of text " << text.size() << " after merges " << text_converted.size() << "\n";
    }
    /* if(verbose) { */
    /*   int token = 0; */
    /*   for(auto v : vocab) { */
    /*     std::cout << token; */
    /*     cout << ": \t "; */
    /*     for(auto c : vocab[token]) { */
    /*       cout << c << " "; */
    /*     } */
    /*     cout << "\n"; */
    /*     token ++; */
    /*   } */
    /* } */
  };
  vector<int> encode(const string &text, const bool verbose) {
    auto text_converted = text_to_vector(text);
    return internal_encode(text_converted, verbose);
  }
  // given ids (list of integers), return Python string
  string decode(const vector<int> &tokens, const bool verbose) {
    /* tokens = b"".join(vocab[idx] for idx in ids) */
    /* text = tokens.decode("utf-8", errors="replace") */
    string text  = "";
    for(auto tkn : tokens) {
      for(auto c : vocab[tkn]) {
        text.push_back(c);
      }
    }
    return text;
  }
};
#endif
