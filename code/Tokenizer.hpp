// Base class for tokenizers to extend
#ifndef MINBPE_TOKENIZER_HPP
#define MINBPE_TOKENIZER_HPP

#include <unordered_map>
#include <tuple>
#include <string>
#include <functional>
#include <vector>

using std::string;
using std::unordered_map;
using std::vector;

using merge_key_t = std::tuple<int, int>;

extern std::function<std::size_t(const merge_key_t&)> key_hash_lambda;

class Tokenizer {

  protected:
     unordered_map<merge_key_t, int, decltype(key_hash_lambda)> merges;
     unordered_map<int, vector<int>> vocab;   
  public:

    Tokenizer() : merges(10, key_hash_lambda) {};
    virtual void train(const string &text, const int vocab_size, const bool verbose) = 0;
    virtual vector<int> encode(const string &text, const bool verbose) = 0;
    virtual string decode(const vector<int> &encoded, const bool verbose) = 0;
};
#endif
