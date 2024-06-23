#ifndef MINBPE_TOKENIZER_HPP
#define MINBPE_TOKENIZER_HPP

#include <algorithm>
#include <climits>
#include <unordered_map>
#include <tuple>
#include <string>
#include <functional>
#include <forward_list>
#include <vector>
#include <cassert>
#include <tuple>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <reflex/boostmatcher.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include "PairCount.h"

using std::string;
using std::unordered_map;
using std::vector;
using std::tuple;
using std::optional;
using std::cout;
using std::filesystem::path;
using std::make_tuple;
using std::ios;

using namespace MinBpeCC::Util;

namespace MinBpeCC::Tokenizer {
  inline std::function<std::size_t(const tuple<int,int>&)> tuple_int_int_hash = 
      [](const tuple<int,int>& k) -> std::size_t {
        std::size_t seed = 0;
        std::hash<int> hasher;
        seed ^= hasher(std::get<0>(k)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hasher(std::get<1>(k)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
  };

  class Tokenizer {
    public:
      inline const static std::string GPT2_SPLIT_PATTERN = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|\\s+(?!\\S)|\\s+";
      inline const static std::string GPT4_SPLIT_PATTERN = "'(?i:[sdmt]|ll|ve|re)|[^\\r\\n\\p{L}\\p{N}]?+\\p{L}+|\\p{N}{1,3}| ?[^\\s\\p{L}\\p{N}]++[\\r\\n]*|\\s*[\\r\\n]|\\s+(?!\\S)|\\s+";
    protected:
      static const auto bucket_size = 10;
      optional<reflex::BoostMatcher::Pattern> compiled_pattern;
      unordered_map<tuple<int,int>, int, decltype(tuple_int_int_hash)> merges_lookup;
      vector<tuple<int,int>> merges;
      unordered_map<int, vector<int>> vocab;   
      string pattern;
      int char_to_int(char c) {
        return c < 0 ? c + 256 : c; 
      }
      std::vector<int> text_to_vector(const string &text) {
        std::vector<int> text_converted;
        for(auto c : text) {
          text_converted.push_back(static_cast<int>(char_to_int(c)));
        }
        return text_converted;
      }
      void initialize_vocab() {
        vocab.clear();
        for(auto i=0; i<(UCHAR_MAX + 1); i++) {
          vector<int> s;
          s.push_back(i);
          vocab[i] = s;
        }
      }
      auto create_lists(const vector<vector<int>> &chunks) {
        vector<std::forward_list<int>> flists;
        for(auto const &chunk: chunks) {
          std::forward_list<int> flist;
          copy(chunk.rbegin(), chunk.rend(), std::front_inserter(flist));
          flists.push_back(flist);
        }
        return flists;
      }
      auto calculate_freqs(const vector<std::forward_list<int>> &chunks) {
        PairCount freqs;
        int insert_order = 0;
        for(auto const &chunk: chunks) {
          auto p1 = chunk.begin();
          auto p2 = next(p1);
          while(p1 != chunk.end() && p2 != chunk.end()) {
            auto p = make_tuple(*p1, *p2);
            freqs.increment_freq_count(p);
            ++p1;
            ++p2;
          }
        }
        return freqs;
    }
    void merge_chunks(vector<std::forward_list<int>> &chunks, tuple<int,int> mp, int idx, int insert_order, PairCount &freqs) {
      /* cout << "start merge_chunks " << chunks.size() <<  "\n"; */
      for(auto &chunk: chunks) {
        /* cout << "  chunk" << "\n"; */
        merge(chunk, mp, idx, insert_order, freqs);
      }
      /* cout << "merge_chunks\n"; */
    }
    void merge(std::forward_list<int> &text, tuple<int,int> mp, int new_token, int insert_order, PairCount &freqs) {
      // can remove verbose setting when it all works lol
      auto verbose = 0;
      // display the text 
      if(verbose >= 2) {
        cout << "before merge\n";
        for(auto c: text) {
          cout << c << " ";
        }
        cout << "\n";
      }
      auto [p1,p2] = mp;
      auto i0 = text.before_begin();
      auto i1 = text.begin();
      auto i2 = next(i1);
      if(i2 == text.end()) {
        // No pairs
        return;
      }
      auto i3 = next(i2);
      while(i1 != text.end() && i2 != text.end()) {
        verbose >= 1 && cout << "i0 " << *i0 << " i1 " << *i1 << " i2 " << *i2 << " i3 " << (i3 == text.end() ? '?' : *i3) << "\n";
        if(*i1 == p1 && *i2 == p2) {
          verbose >= 1 && cout << "found pair " << p1 << ", " << p2 << " replace with " << new_token << "\n";
          *i1 = new_token;
          i2 = text.erase_after(i1);

          // update freqs
          auto& index_by_key = freqs.get_index_by_key();
          auto f = index_by_key.find(mp);
          if(f != freqs.end()) {
            verbose >= 1 && cout << "decrement replaced pair " << get<0>(f->pair) << ", " << get<1>(f->pair) << "\n";
            freqs.decrement_freq_count(f->pair);
          }
          if(i0 != text.end()) {
            auto prev = index_by_key.find(make_tuple(*i0, p1));
            if(prev != freqs.end()) {
              verbose >= 1 && cout << "decrement previous pair " << get<0>(prev->pair) << ", " << get<1>(prev->pair) << "\n";
              freqs.decrement_freq_count(prev->pair);
            }
            verbose >= 1 && cout << "increment new previous pair " << *i0 << ", " << new_token << "\n";
            freqs.increment_freq_count(make_tuple(*i0, new_token));
          }
          if(i3 != text.end()) {
            auto next = index_by_key.find(make_tuple(p2, *i3));
            if(next != freqs.end()) {
              verbose >= 1 && cout << "decrement next pair " << get<0>(next->pair) << ", " << get<1>(next->pair) << "\n";
              freqs.decrement_freq_count(next->pair);
            } else {
              verbose >= 1 && cout << "next pair not found " << p2 << ", " << *i3 << "\n";
            }
            verbose >= 1 && cout << "increment new next pair " << new_token << ", " << *i3 << "\n";
            freqs.increment_freq_count(make_tuple(new_token, *i3));
          }

          // Adjust iterators
          if(i2 == text.end()) {
            verbose >= 1 && cout << "i2 end\n";
          } else {
            i3  = next(i2);
          }

          if(verbose >= 1 && i3 == text.end()) {
            cout << "i3 end\n";
          }

        }
        i0++;
        i1++;
        if(i2 != text.end()) {
          i2++;
        }
        if(i3 != text.end()) {
          i3++;
        }
      }
      if(verbose >= 2) {
        cout << "after merge\n";
        for(auto c: text) {
          cout << c << " ";
        }
        cout << "\n";
      }
    }
    // replace all consecutive occurences of pair with the new token idx
    void merge_chunks(std::vector<vector<int>> &chunks, tuple<int,int> mp, int idx) {
      for(auto &chunk: chunks) {
        merge(chunk, mp, idx);
      }
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
        // A B C and p1 = AB   
        // i1 = a, i2 = b
        // output p1 and set skip 
        // i1 B i2 C skip true
        // i1 C i2 end skip false
        // loop ends
        // so check for i1 not end, i2 end, output i1
        if(skip) {
          skip = false;
          i++;
          i1++;
          i2++;
          if(i1 != text.end() && i2 == text.end()) {
            new_text.push_back(*i1);
            break;
          } else {
            continue;
          }
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
    vector<int> internal_internal_encode(const vector<int> &text) {
        vector<int> out;
        auto i = 0;
        auto merge_count = 0;
        auto len = text.size();
        while(i < len) {
          auto pair = make_tuple(text[i], text[i + 1]);
          if(i < len - 1 && merges_lookup.find(pair) != merges_lookup.end()) {
            out.push_back(merges_lookup[pair]);
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
          return internal_internal_encode(out);
        }
    }
    vector<vector<int>> internal_encode(const vector<vector<int>> &text) {
      vector<vector<int>> outer;
      for(auto &t: text) {
        auto encoded = internal_internal_encode(t);
        outer.push_back(encoded);
      }
      return outer;
    }
    void build_vocab(bool verbose) {
      vocab.clear();
      auto i=0;
      for(; i<256; i++) {
        vocab[i] = vector<int>{i};
      }
      for(auto mio: merges) {
        vector<int> appended{vocab[get<0>(mio)]};
        appended.insert(appended.end(),vocab[get<1>(mio)].begin(), vocab[get<1>(mio)].end());
        vocab[i] = appended;
        i++;
      }
      if(verbose) {
        cout << "Loaded vocab with " << 256 + merges.size() << " merges, vocab size is " << vocab.size() << "\n";
      }
      // TODO special token handling
    }
    public:
      Tokenizer() : merges_lookup(bucket_size, tuple_int_int_hash),
                    pattern{},
                    compiled_pattern{} {
      };
      Tokenizer(const string &pattern) : merges_lookup(bucket_size, tuple_int_int_hash) {
        this->pattern = pattern;
        if(pattern.length() > 0) {
          std::string regex = reflex::BoostMatcher::convert(pattern, reflex::convert_flag::unicode);
          compiled_pattern = reflex::BoostMatcher::Pattern(regex);
        }
      };
      void train(const string &text, const int vocab_size, const bool verbose) {
        assert(vocab_size >= 256);
        reflex::Input input(text); 

        merges.clear();
        merges.reserve(vocab_size - 256);

        vector<vector<int>> chunks;

        if(compiled_pattern.has_value()) {
          // GPT-2, GPT-4 tokenizers first chunk the input text
          // to keep semantically related pairs together.
          // This means the input to the merging stage is a vector 
          // of vectors...
          auto matcher = reflex::BoostMatcher(compiled_pattern.value(), input);
          for(auto &match : matcher.find) {
            auto text_converted = text_to_vector(match.text());
            chunks.push_back(text_converted);
          }
        } else {
          // When no split pattern just treat the whole text as
          // a single chunk
          chunks.push_back(text_to_vector(text));
        }

        initialize_vocab();

        auto flists = create_lists(chunks);
        auto freqs = calculate_freqs(flists);
        for(auto i=0; i < vocab_size - 256; i++) {
          const auto& index_by_count = freqs.get_index_by_count();
          if(!index_by_count.empty()) {
          /* if(max != freqs.end()) { */
            auto max = *index_by_count.begin();
            auto [p1,p2] = max.pair;
            if(verbose) {
              cout << "merge pair " << p1 << ", " << p2 << " with new token " << i << " count " << max.count << "\n";
            }

            merges.push_back(max.pair);
            merge_chunks(flists, max.pair, i, max.count, freqs);
          } else {
            break;
          }
        }
        if(verbose) {
          int size = 0;
          for(auto &fl: flists) {
            size += std::distance(fl.begin(), fl.end());
          }
          cout << "Length of training text " << text.size() << ". After merges " << size << ".\n";
        }
      };

      vector<int> encode(const string &text, const bool verbose) {
          vector<vector<int>> text_chunks;

          if(compiled_pattern.has_value()) {
            reflex::Input input(text); 
            // TODO should be able to run without a compiled pattern
            auto matcher = reflex::BoostMatcher(compiled_pattern.value(), input);

            // Note if you add special token support it would do a pass
            // here first to take care of those, then continue as normal

            // GPT2(+) tokenizers first chunk the input text
            // to keep semantically related pairs together.
            // This means the input to the merging stage is a vector 
            // of vectors...
            for(auto &match : matcher.find) {
              auto text_converted = text_to_vector(match.text());
              text_chunks.push_back(text_converted);
            }
          } else {
            // When no split pattern just treat the whole text as
            // a single chunk
            text_chunks.push_back(text_to_vector(text));
          }

          auto encoded_chunks = internal_encode(text_chunks);
          vector<int> out;
          for(auto &chunk: encoded_chunks) {
            out.insert(out.end(), chunk.begin(), chunk.end());
          }
          if(verbose) {
            cout << "Encoded input text (length " << text.length() << ") to " << out.size() << " tokens\n"; 
          }
          return out;

      };
      string decode(const vector<int> &tokens, const bool verbose) {
        if(verbose) {
          cout << "Decoding " << tokens.size() << " tokens\n";
        }
        string text  = "";
        for(auto tkn : tokens) {
          for(auto c : vocab[tkn]) {
            text.push_back(c);
          }
        }
        return text;
      };
      bool load(const path &path, const bool verbose) {
        std::ifstream input_file(path, ios::in);
        if(input_file.is_open()) {
          string version;
          std::getline(input_file, version);
          if(version != "minbpe v1") {
            std::cerr << "Unexpected version: " << version << "\n";
            return false;
          }
          merges.clear(); 
          vocab.clear();
          // TODO init special tokens
          int index = 256;
          std::getline(input_file, pattern);
          int num_special;
          input_file >> num_special;
          for(int i=0; i<num_special; i++) {
            // TODO read special tokens
          }
          int idx = 256, idx1, idx2;
          while(input_file >> idx1 >> idx2) {
            merges_lookup[make_tuple(idx1, idx2)] = idx;
            ++idx;
          }
          if(verbose) {
            cout << "Read input\n";
          }
          build_vocab(verbose);
        } else {
          std::cerr << "Failed to open file " << path << "\n";
          return false;
        }
        
        return true;
      }
      bool save(const path &path) {
        std::ofstream output_file(path, ios::out);
        if (output_file.is_open()) {
          cout << "Writing model...\n";
          output_file << "minbpe v1" << std::endl;
          output_file << pattern << std::endl;
          output_file << 0 << std::endl; // special token count
          // write special tokens
          for(auto &m: merges) {
            output_file << get<0>(m) << ' ' << get<1>(m) << "\n";
          }
          output_file.close();
          cout << "Complete.\n";
          return true;
        } else {
          std::cerr << "Unable to open file: " << path << std::endl;
          return false;
        }
      }
  };
}
#endif
