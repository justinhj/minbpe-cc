// Base class for tokenizers to extend
#include <unordered_map>
#include <tuple>

using std::string;
using std::unordered_map;

class Tokenizer {

  private:
     unordered_map<std::tuple<int, int, int>, int> merges;   
     unordered_map<std::tuple<int, int, int>, string> vocab;   

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

    Tokenizer() {
    };

    virtual void train(const string &text, const int vocab_size, const bool verbose) = 0;
};
