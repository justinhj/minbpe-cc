// Base class for tokenizers to extend
#include <unordered_map>
#include <tuple>
#include <string>
#include <functional>

using std::u8string;
using std::unordered_map;

using merge_key_t = std::tuple<int, int>;

extern std::function<std::size_t(const merge_key_t&)> key_hash_lambda;

class Tokenizer {

  protected:
     unordered_map<merge_key_t, int, decltype(key_hash_lambda)> merges;
     unordered_map<int, u8string> vocab;   

    // TODO 
    /* self.pattern = "" # str */
    /* self.special_tokens = {} # str -> int, e.g. {'<|endoftext|>': 100257} */
    /* self.vocab = self._build_vocab() # int -> bytes */

    // usage of map
    // adding to merges   myMap[std::make_tuple(1, 2, 3)] = 42;
    /* if (myMap.find(key) != myMap.end()) { */
    /*     return myMap[key]; */
    /* } */

  public:

    Tokenizer() : merges(10, key_hash_lambda) {
    };

    virtual void train(const u8string &text, const int vocab_size, const bool verbose) = 0;
};
