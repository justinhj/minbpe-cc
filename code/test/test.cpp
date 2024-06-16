#define CATCH_CONFIG_MAIN
#include "Catch2/catch2.hpp"

#include "Tokenizer.hpp"

// Tests for the PairCount helper class
TEST_CASE("PairCount add and count", "[paircount]") {
    PairCount pc;
    REQUIRE( pc.get_count() == 0 );
    pc.increment_freq_count(make_tuple(1,2));
    REQUIRE( pc.get_count() == 1 );
    REQUIRE( pc.get_insert_order() == 1 );
    pc.increment_freq_count(make_tuple(1,2));
    REQUIRE( pc.get_count() == 1 );
    pc.increment_freq_count(make_tuple(2,3));
    REQUIRE( pc.get_count() == 2 );
}

TEST_CASE("PairCount order and get most frequent", "[paircount]") {
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
    REQUIRE( max.value().pair == make_tuple(1,2) );
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

    /* bt.train(input, num_tokens, verbose); */
    /* REQUIRE( bt.get_token_count() == 4 ); */
    /* REQUIRE( bt.get_token("This") == 0 ); */
    /* REQUIRE( bt.get_token("is") == 1 ); */
    /* REQUIRE( bt.get_token("a") == 2 ); */
    /* REQUIRE( bt.get_token("test.") == 3 ); */
}
