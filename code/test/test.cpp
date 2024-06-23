#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "Tokenizer.hpp"

void debug_pair_count(PairCount &pc) {
    // Iterate over the freqs in order
    // pair: (65, 32) count: 8
    auto &index_by_count = pc.get_index_by_count();
    for(auto &f: index_by_count) {
      auto [p1, p2] = f.pair;
      cout << "pair: (" << p1 << ", " << p2 << ") count: " << f.count << "   insert " << f.count << "\n";
    }
}

// Tests for the PairCount helper class
TEST_CASE("PairCount add and count", "[paircount]") {
    PairCount pc;
    REQUIRE( pc.get_count() == 0 );
    int insert_order = 0;
    pc.increment_freq_count(make_tuple(1,2));
    REQUIRE( pc.get_count() == 1 );
    pc.increment_freq_count(make_tuple(1,2));
    REQUIRE( pc.get_count() == 1 );
    pc.increment_freq_count(make_tuple(2,3));
    REQUIRE( pc.get_count() == 2 );
}

TEST_CASE("PairCount get most frequent", "[paircount]") {
    PairCount pc;
    auto max = pc.get_top_pair_count_order();
    REQUIRE( !max.has_value() );
    pc.increment_freq_count(make_tuple(1,2));
    max = pc.get_top_pair_count_order();
    REQUIRE( max.has_value() );
    REQUIRE( max.value().pair == make_tuple(1,2) );

    pc.increment_freq_count(make_tuple(1,2));
    pc.increment_freq_count(make_tuple(2,3));
    max = pc.get_top_pair_count_order();
    REQUIRE( max.has_value() );
    REQUIRE( max.value().pair == make_tuple(1,2) );

    pc.increment_freq_count(make_tuple(2,3));
    pc.increment_freq_count(make_tuple(2,3));
    max = pc.get_top_pair_count_order();
    REQUIRE( max.has_value() );
    REQUIRE( max.value().pair == make_tuple(2,3) );

    pc.increment_freq_count(make_tuple(1,2));

    max = pc.get_top_pair_count_order();
    REQUIRE( max.has_value() );
    REQUIRE( (max.value().pair == make_tuple(1,2) || max.value().pair == make_tuple(2,3)) );
}

// Expose private and protected methods for testing
class TokenizerTest : public Tokenizer {
  public:
    auto create_lists_public(const vector<vector<int>> &chunks) {
        return create_lists(chunks);
    };

    auto text_to_vector_public(const string &text) {
        return text_to_vector(text);
    };

    auto calculate_freqs_public(const vector<std::forward_list<int>> &chunks) {
        return calculate_freqs(chunks);
    };

    auto merge_public(std::forward_list<int> &text, tuple<int,int> mp, int new_token, int insert_order, PairCount &freqs) {
      return merge(text, mp, new_token, insert_order, freqs);
    }
};

template<typename T>
size_t getForwardListLength(const std::forward_list<T>& flist) {
    size_t count = 0;
    for (auto it = flist.begin(); it != flist.end(); ++it) {
        ++count;
    }
    return count;
}

// Tests for the Tokenizer class
TEST_CASE("Tokenizer training tests", "[tokenizer]") {
    TokenizerTest bt;
    vector<vector<int>> chunks;
    const auto test_string = string("abcbcde");
    chunks.push_back(bt.text_to_vector_public(test_string));
    auto flists = bt.create_lists_public(chunks);
    REQUIRE( flists.size() == 1 );
    REQUIRE( getForwardListLength(flists[0]) == test_string.size());

    auto freqs = bt.calculate_freqs_public(flists);

    // 97,98,99,98,99,100,101

    auto max = freqs.get_top_pair_count_order();
    REQUIRE( max.has_value() ); 
    REQUIRE( max.value().pair == make_tuple(98,99) );

    bt.merge_public(flists[0], make_tuple(98,99), 256, 1, freqs);
    
    // 97,256,256,100,101

    max = freqs.get_top_pair_count_order();

    REQUIRE( max.has_value() ); 
    /* REQUIRE( max.value().pair == make_tuple(97,256) ); */

    bt.merge_public(flists[0], make_tuple(97,256), 257, 1, freqs);

    max = freqs.get_top_pair_count_order();

    REQUIRE( max.has_value() ); 

    auto expected_values = std::vector{
          std::make_tuple(257,256),
          std::make_tuple(256,100),
          std::make_tuple(0,257),
          std::make_tuple(100,101)
      };

   auto it = std::find(expected_values.begin(), expected_values.end(), max.value().pair);
   REQUIRE(it != expected_values.end());
}
