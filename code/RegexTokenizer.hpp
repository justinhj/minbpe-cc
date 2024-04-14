#ifndef MINBPE_REGEX_TOKENIZER_HPP
#define MINBPE_REGEX_TOKENIZER_HPP

#include "Tokenizer.hpp"
#include <iostream>
/* #include <ranges> */
#include <optional>
#include <reflex/boostmatcher.h>

using std::cout;
using std::optional;
using std::get;

class RegexTokenizer : public Tokenizer {
  public:
    inline const static std::string GPT2_SPLIT_PATTERN = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|\\s+(?!\\S)|\\s+";
    inline const static std::string GPT4_SPLIT_PATTERN = "'(?i:[sdmt]|ll|ve|re)|[^\\r\\n\\p{L}\\p{N}]?+\\p{L}+|\\p{N}{1,3}| ?[^\\s\\p{L}\\p{N}]++[\\r\\n]*|\\s*[\\r\\n]|\\s+(?!\\S)|\\s+";
  protected:
    reflex::BoostMatcher::Pattern compiled_pattern;

    optional<tuple<int,int,int>> most_frequent_pair(const vector<vector<int>> &chunks) {
      assert(chunks.size() > 1);
      // TODO for fun use std::ranges::zip_view<input_range Views> to zip over the head and next elements as pairs
      unordered_map<merge_key_t, tuple<int,int>, decltype(key_hash_lambda)> freqs(10, key_hash_lambda);
      // Track insert order for predictable vocab output
      auto step = 0;
      for(auto const &chunk: chunks) {
        /* cout << "chunk "; */
        /* for(int c : chunk) { */
        /*   cout << c <<  ", "; */
        /* } */
        /* cout << "  => " << string(chunk.begin(), chunk.end()) << "\n"; */

        if(chunk.size() < 2) {
          continue; // Skip processing chunk with no pairs
        } 
        auto i1 = chunk.begin();
        auto i2 = ++chunk.begin();
        while(i1 != chunk.end() && i2 != chunk.end()) {
          auto p = make_tuple(*i1, *i2);
          int f = 1;
          auto pair = freqs.find(p);
          if(pair == freqs.end()) {
            freqs[p] = make_tuple(f,step);
            /* cout << "adding pair " << get<0>(p) << ", " << get<1>(p) << "\n"; */
          } else {
            auto [f,s] = get<1>(*pair);
            f++;
            freqs[p] = make_tuple(f,s);
            /* cout << "increment pair " << get<0>(p) << ", " << get<1>(p) << " count " << f << " seq " << s << "\n"; */
          }
          ++i1;
          ++i2;
          ++step;
        }
      }

      auto maxElementIt = std::max_element(freqs.begin(), freqs.end(),
          [](const std::pair<const tuple<int,int>, tuple<int,int>>& a, const std::pair<const tuple<int,int>, tuple<int,int>>& b) -> bool {
              auto av = get<0>(a.second);  // TODO use structured binding here to simplify
              auto as = get<1>(a.second); 
              auto bv = get<0>(b.second); 
              auto bs = get<1>(b.second); 
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
      if(maxElementIt != freqs.end()) {
        auto &max_pair = *maxElementIt;
        auto return_pair = max_pair.first;
        auto max = get<0>(max_pair.second);
        return make_tuple(get<0>(return_pair), get<1>(return_pair), max);
      } else {
        return std::nullopt;
      }
    }
    // replace all consecutive occurences of pair with the new token idx
    void merge_chunks(std::vector<vector<int>> &chunks, tuple<int,int> mp, int idx) {
      for(auto &chunk: chunks) {
        merge(chunk, mp, idx);
      }
    }
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
      assert(vocab_size >= 256);
      reflex::Input input(text); 
      auto matcher = reflex::BoostMatcher(compiled_pattern, input);

      // GPT2(+) tokenizers first chunk the input text
      // to keep semantically related pairs together.
      // This means the input to the merging stage is a vector 
      // of vectors...
      vector<vector<int>> ids;
      for(auto &match : matcher.find) {
        auto text_converted = text_to_vector(match.text());
        ids.push_back(text_converted);
      }

      initialize_vocab();

      for(auto i=UCHAR_MAX + 1; i < vocab_size; i++) {
        // TODO Maybe set verbose as enum levels and add this as trace
        // Just to debug
        /* if(verbose) { */
        /*   for(auto chunk : ids) { */
        /*     for(int c : chunk) { */
        /*       cout << c <<  ", "; */
        /*     } */
        /*     cout << "  => " << string(chunk.begin(), chunk.end()) << "\n"; */
        /*   } */
        /* } */
        assert(ids.size() > 0);
        auto mp3 = most_frequent_pair(ids);
        if(mp3.has_value()) {
          if(verbose) {
            cout << "merge pair " << get<0>(mp3.value()) << ", " << get<1>(mp3.value()) << " with new token " << i << " count " << get<2>(mp3.value()) << "\n";
          }
          auto mp = make_tuple(get<0>(mp3.value()), get<1>(mp3.value()));
          merge_chunks(ids, mp, i);
          merges[mp] = i;

          vector<int> new_vocab { vocab[get<0>(mp)] };
          const vector<int> &v2 = vocab[get<1>(mp)];
          new_vocab.insert(new_vocab.end(), v2.begin(), v2.end());
          vocab[i] = new_vocab;
        }
      }
      if(verbose) {
        int size = 0;
        for(auto &chunk: ids) {
          size += chunk.size();
        }
        cout << "length of text " << text.size() << " after merges " << size << "\n";
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
