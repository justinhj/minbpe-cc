// Base class for tokenizers to extend
#ifndef MINBPE_TOKENIZER_HPP
#define MINBPE_TOKENIZER_HPP

#include <unordered_map>
#include <tuple>
#include <string>
#include <functional>
#include <vector>
#include <cassert>
#include <tuple>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <reflex/boostmatcher.h>

using std::string;
using std::unordered_map;
using std::vector;
using std::tuple;
using std::optional;
using std::cout;
using std::filesystem::path;
using std::make_tuple;
using std::ios;

inline std::function<std::size_t(const tuple<int,int>&)> key_hash_lambda = 
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
    struct MergeOrder {
      int p1, p2, *idx;
    };
    unordered_map<tuple<int,int>, int, decltype(key_hash_lambda)> merges;
    vector<MergeOrder> merges_insert_order;
    unordered_map<int, vector<int>> vocab;   
    string pattern;
    int char_to_int(char8_t c) {
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

  // Count the frequencies of all pairs and return the most frequently occuring
  optional<tuple<int,int,int>> most_frequent_pair(const vector<vector<int>> &chunks) {
    // TODO for fun use std::ranges::zip_view<input_range Views> to zip over the head and next elements as pairs
    unordered_map<tuple<int,int>, tuple<int,int>, decltype(key_hash_lambda)> freqs(10, key_hash_lambda);
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
            auto [av,as] = a.second;
            auto [bv,bs] = b.second; 
            return (av < bv) || (av == bv && bs < as);
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
    /* cout << "text " << string(text.begin(), text.end()) << " new_text " << string(new_text.begin(), new_text.end()) << "\n"; */
    text = new_text;
  }
  vector<int> internal_internal_encode(const vector<int> &text) {
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
    for(auto i=0; i<256; i++) {
      vocab[i] = vector<int>{i};
    }
    for(auto mio: merges_insert_order) {
      vector<int> appended{vocab[mio.p1]};
      appended.insert(appended.end(),vocab[mio.p2].begin(), vocab[mio.p2].end());
      vocab[*mio.idx] = appended;
    }
    if(verbose) {
      cout << "Loaded vocab with " << 256 + merges.size() << " merges, vocab size is " << vocab.size() << "\n";
    }
    // TODO special token handling
  }
  public:
    Tokenizer() : merges(bucket_size, key_hash_lambda),
                  pattern{},
                  compiled_pattern{} {
    };
    Tokenizer(const string &pattern) : merges(bucket_size, key_hash_lambda) {
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
      merges_insert_order.clear();
      merges_insert_order.reserve(vocab_size);  

      vector<vector<int>> ids;

      if(compiled_pattern.has_value()) {
        // GPT-2, GPT-4 tokenizers first chunk the input text
        // to keep semantically related pairs together.
        // This means the input to the merging stage is a vector 
        // of vectors...
        auto matcher = reflex::BoostMatcher(compiled_pattern.value(), input);
        for(auto &match : matcher.find) {
          auto text_converted = text_to_vector(match.text());
          ids.push_back(text_converted);
        }
      } else {
        // When no split pattern just treat the whole text as
        // a single chunk
        ids.push_back(text_to_vector(text));
      }

      initialize_vocab();

      for(auto i=UCHAR_MAX + 1; i < vocab_size; i++) {
        // TODO Maybe set verbose as enum levels and add this as trace
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
          auto mp = make_tuple(get<0>(mp3.value()), get<1>(mp3.value()));
          merge_chunks(ids, mp, i);
          merges[mp] = i;
          merges_insert_order.push_back({get<0>(mp),get<1>(mp),&merges[mp]});

          vector<int> new_vocab { vocab[get<0>(mp)] };
          const vector<int> &v2 = vocab[get<1>(mp)];
          new_vocab.insert(new_vocab.end(), v2.begin(), v2.end());
          vocab[i] = new_vocab;
          if(verbose) {
            cout << "merge pair " << get<0>(mp3.value()) << ", " << get<1>(mp3.value()) << " with new token " << i << " count " << get<2>(mp3.value()) <<  " new vocab " << string(new_vocab.begin(), new_vocab.end()) << "\n";
          }
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
          merges[make_tuple(idx1, idx2)] = idx;
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
        for(auto &mio: merges_insert_order) {
          output_file << mio.p1 << ' ' << mio.p2 << "\n";
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
#endif
