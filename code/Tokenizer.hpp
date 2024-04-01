// Base class for tokenizers to extend
#include <unordered_map>
#include <tuple>
#include <string>
#include <functional>
#include <vector>

using std::u8string;
using std::unordered_map;
using std::vector;

using merge_key_t = std::tuple<int, int>;

extern std::function<std::size_t(const merge_key_t&)> key_hash_lambda;

class Tokenizer {

  protected:
     unordered_map<merge_key_t, int, decltype(key_hash_lambda)> merges;
     std::vector<u8string> vocab;   
  public:

    Tokenizer() : merges(10, key_hash_lambda) {};
    virtual void train(const u8string &text, const int vocab_size, const bool verbose) = 0;
    virtual vector<int> encode(const u8string &text, const bool verbose) = 0;
    virtual u8string decode(const vector<int> &encoded, const bool verbose) = 0;
};
