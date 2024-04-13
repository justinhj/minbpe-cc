#ifndef MINBPE_REGEX_TOKENIZER_HPP
#define MINBPE_REGEX_TOKENIZER_HPP

#include "Tokenizer.hpp"
#include <iostream>
#include <reflex/boostmatcher.h>

using std::cout;

class RegexTokenizer : public Tokenizer {
  public:
    inline const static std::string GPT2_SPLIT_PATTERN = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|\\s+(?!\\S)|\\s+";
    inline const static std::string GPT4_SPLIT_PATTERN = "'(?i:[sdmt]|ll|ve|re)|[^\\r\\n\\p{L}\\p{N}]?+\\p{L}+|\\p{N}{1,3}| ?[^\\s\\p{L}\\p{N}]++[\\r\\n]*|\\s*[\\r\\n]|\\s+(?!\\S)|\\s+";
  private:
    reflex::BoostMatcher::Pattern compiled_pattern;
  public:
    RegexTokenizer(const string &pattern) : Tokenizer() {
      std::string regex = reflex::BoostMatcher::convert(pattern, reflex::convert_flag::unicode);
      compiled_pattern = reflex::BoostMatcher::Pattern(regex);
    };
    RegexTokenizer() : Tokenizer() {
      std::string regex = reflex::BoostMatcher::convert(GPT4_SPLIT_PATTERN, reflex::convert_flag::unicode);
      compiled_pattern = reflex::BoostMatcher::Pattern(regex);
    };

    void train(const string &text, const int vocab_size, const bool verbose) {
      reflex::Input input(text); 
      
      auto matcher = reflex::BoostMatcher(compiled_pattern, input);

      vector<vector<int>> text_chunks;
      for(auto &match : matcher.find) {
        auto text_converted = text_to_vector(match.text());
        text_chunks.push_back(text_converted);
      }

      if(verbose) {
        for(auto chunk : text_chunks) {
          for(int c : chunk) {
            cout << c <<  ", ";
          }
          cout << "\n";
        }
      }

    };
    
    vector<int> encode(const string &text, const bool verbose) {
      return vector<int>{};
    };
    string decode(const vector<int> &encoded, const bool verbose) {
      return "nope";
    }
};
#endif
