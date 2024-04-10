#ifndef MINBPE_REGEX_TOKENIZER_HPP
#define MINBPE_REGEX_TOKENIZER_HPP

#include "Tokenizer.hpp"
#include <iostream>
#include <re2/re2.h>

/* GPT2_SPLIT_PATTERN = r"""'(?:[sdmt]|ll|ve|re)| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+""" */
/* GPT4_SPLIT_PATTERN = r"""'(?i:[sdmt]|ll|ve|re)|[^\r\n\p{L}\p{N}]?+\p{L}+|\p{N}{1,3}| ?[^\s\p{L}\p{N}]++[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+""" */

class RegexTokenizer : public Tokenizer {
  private:
    /* inline const static std::string GPT2_SPLIT_PATTERN = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|\\s+(?!\\S)|\\s+"; */
    inline const static std::string GPT2_SPLIT_PATTERN = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|(?:(?<=\\s)\\s+)|(?:\\s+\\s)|\\s+";
    /* re2::RE2 gpt2_split_regex; */
  public:
    RegexTokenizer() : Tokenizer() {
      /* gpt2_split_regex = re2::RE2(GPT2_SPLIT_PATTERN); */

      // TEMP
      const string text = R"(RCA Republic Big Machine
Website	www.taylorswift.com Edit this at Wikidata
Signature

Taylor Alison Swift (born December 13, 1989) is an American singer-songwriter. Her versatile artistry, songwriting, and entrepreneurship have influenced the music industry, popular culture, and politics, and her life is a subject of widespread media coverage.
)";

    re2::RE2 re(GPT2_SPLIT_PATTERN);
    re2::StringPiece input_piece(text);
    string match;
    
    while (re2::RE2::FindAndConsume(&input_piece, re, &match)) {
        std::cout << "Matched: " << match << std::endl;
    }

    // Iterator to iterate through the matches
    /* boost::sregex_iterator iter(text.begin(), text.end(), gpt2_split_regex); */
    /* boost::sregex_iterator end; */

    /* // Iterate over all matches */
    /* for( ; iter != end; ++iter ) { */
    /*     // iter->text() gets the matched texting */
    /*     std::cout << "Found match: " << iter->str() << std::endl; */
    /* } */

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
