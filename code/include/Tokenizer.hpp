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

using std::string;
using std::unordered_map;
using std::vector;
using std::tuple;
using std::cout;
using std::filesystem::path;
using std::make_tuple;
using std::ios;

extern std::function<std::size_t(const tuple<int,int>&)> key_hash_lambda;

class Tokenizer {
  protected:
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
    unordered_map<tuple<int,int>, tuple<int,int>, decltype(key_hash_lambda)> freqs(10, key_hash_lambda);
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
    Tokenizer() : merges(10, key_hash_lambda) {}; // 10 is the bucket size
    virtual ~Tokenizer() {};
    virtual void train(const string &text, const int vocab_size, const bool verbose) = 0;
    virtual vector<int> encode(const string &text, const bool verbose) = 0;
    virtual string decode(const vector<int> &encoded, const bool verbose) = 0;
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
