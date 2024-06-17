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

void debug_pair_count(PairCount &pc) {
    // Iterate over the freqs in order
    // pair: (65, 32) count: 8
    auto &index_by_count = pc.get_index_by_count();
    for(auto &f: index_by_count) {
      auto [p1, p2] = f.pair;
      cout << "pair: (" << p1 << ", " << p2 << ") count: " << f.countOrder.count << "   insert " << f.countOrder.insert_order << "\n";
    }
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

    auto merge_public(std::forward_list<int> &text, tuple<int,int> mp, int new_token, PairCount &freqs) {
      return merge(text, mp, new_token, freqs);
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

    debug_pair_count(freqs);
    cout << "\n";

    bt.merge_public(flists[0], make_tuple(98,99), 256, freqs);
    
    // 97,256,256,100,101

    // 99,98 1 should be 0


    max = freqs.get_top_pair_count_order();

    debug_pair_count(freqs);
    REQUIRE( max.has_value() ); 
    REQUIRE( max.value().pair == make_tuple(256,256) );
}
