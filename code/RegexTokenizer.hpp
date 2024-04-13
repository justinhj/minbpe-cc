#ifndef MINBPE_REGEX_TOKENIZER_HPP
#define MINBPE_REGEX_TOKENIZER_HPP

#include "Tokenizer.hpp"
#include <iostream>
#include <reflex/boostmatcher.h>

using std::cout;

class RegexTokenizer : public Tokenizer {
  private:
    inline const static std::string GPT2_SPLIT_PATTERN = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|\\s+(?!\\S)|\\s+";
    inline const static std::string GPT4_SPLIT_PATTERN = "'(?i:[sdmt]|ll|ve|re)|[^\\r\\n\\p{L}\\p{N}]?+\\p{L}+|\\p{N}{1,3}| ?[^\\s\\p{L}\\p{N}]++[\\r\\n]*|\\s*[\\r\\n]|\\s+(?!\\S)|\\s+";
  public:
    RegexTokenizer() : Tokenizer() {
      const string text = R"(RCA Republic Big Machine
Website	www.taylorswift.com Edit this at Wikidata
Signature

Taylor Alison Swift (born December 13, 1989) is an American singer-songwriter. Her versatile artistry, songwriting, and entrepreneurship have influenced the music industry, popular culture, and politics, and her life is a subject of widespread media coverage.
)";

      reflex::Input input(text);
      std::string match;

      static const std::string regex = reflex::BoostMatcher::convert(GPT2_SPLIT_PATTERN, reflex::convert_flag::unicode);
      static const reflex::BoostMatcher::Pattern pattern(regex);

      auto matcher = reflex::BoostMatcher(pattern, input);

      for(auto &match : matcher.find) {
        std::cout << "Found " << match << std::endl;
      }
    };

    void train(const string &text, const int vocab_size, const bool verbose) {

    };
    
    vector<int> encode(const string &text, const bool verbose) {
      return vector<int>{};
    };
    string decode(const vector<int> &encoded, const bool verbose) {
      return "nope";
    }
};
#endif
