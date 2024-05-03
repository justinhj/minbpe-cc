#ifndef MINBPE_BASIC_TOKENIZER_HPP
#define MINBPE_BASIC_TOKENIZER_HPP

#include "Tokenizer.hpp"
#include <climits>
#include <iostream>
/* #include <ranges> */
#include <tuple>
#include <vector>
/* #include <algorithm> */

using std::string;
using std::unordered_map;
using std::cout;
using std::endl;
using std::tuple;
using std::make_tuple;

class BasicTokenizer : public Tokenizer {
  public:
  BasicTokenizer() : Tokenizer() {};
  protected:
  public:
  // Warning for compatibility with common tokenizers it is assumed the input is in 
  // utf-8 encoding.
  void train(const string &text, const int vocab_size, const bool verbose) { 
    assert(vocab_size >= 256);
    merges.clear();
    merges_insert_order.clear();
    merges_insert_order.reserve(vocab_size);  
    // Show input text, a bit too verbose unless you're debugging
    /* if(verbose) { */
    /*   for(int c : text) { */
    /*     cout << char_to_int(c) <<  ' '; */
    /*   } */
    /*   cout << "\n"; */
    /* } */
    auto text_converted = text_to_vector(text);
    initialize_vocab();
    for(auto i=UCHAR_MAX + 1; i < vocab_size; i++) {
      if(text_converted.size() < 2) {
        break;
      }
      // TODO Maybe set verbose as enum levels and add this as trace
      // Just to debug
      /* for(int c : text_converted) { */
      /*   cout << c <<  ' '; */
      /* } */
      /* cout << "\n"; */
      // TODO make this return and handle an option like the regex one does
      auto mp3 = most_frequent_pair(text_converted);
      if(verbose) {
        cout << "merge pair " << std::get<0>(mp3) << ", " << std::get<1>(mp3) << " with new token " << i << " count " << std::get<2>(mp3) << "\n";
      }
      auto mp = make_tuple(std::get<0>(mp3), std::get<1>(mp3));
      merge(text_converted, mp, i);
      merges[mp] = i;
      merges_insert_order.push_back(mp);

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
    // TODO move to parent
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
