#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "Tokenizer.h"

using MinBpeCC::Tokenizer::Tokenizer;

TEST_CASE("PairCount allows multiple pairs with the same rank", "[paircount]") {
    PairCount pc;

    // Add a new pair. It will be inserted with count = 1 and first_occurrence = 1.
    pc.create_or_modify_pair(10, 20, 1);
    REQUIRE(pc.get_count() == 1);

    // Now, add a DIFFERENT pair but with the same initial count and first_occurrence value.
    // With the ordered_unique bug, this insert will fail silently.
    pc.create_or_modify_pair(30, 40, 1);

    // This assertion will fail with the buggy code, as the count will be 1 instead of 2.
    REQUIRE(pc.get_count() == 2);

    // Verify both pairs are actually in the container.
    auto p1 = pc.get_pair({10, 20});
    auto p2 = pc.get_pair({30, 40});

    REQUIRE(p1.has_value());
    REQUIRE(p1.value() == 1);
    REQUIRE(p2.has_value());
    REQUIRE(p2.value() == 1);
}

TEST_CASE("PairCount add and count", "[paircount]") {
    PairCount pc;
    REQUIRE( pc.get_count() == 0 );
    int insert_order = 0;
    pc.create_or_modify_pair(1, 2, 1); 
    REQUIRE( pc.get_count() == 1 );
    pc.create_or_modify_pair(1, 2, 1); 
    REQUIRE( pc.get_count() == 1 );
    pc.create_or_modify_pair(2, 3, 1); 
    REQUIRE( pc.get_count() == 2 );
}

TEST_CASE("PairCount get most frequent", "[paircount]") {
    PairCount pc;
    auto max = pc.get_top_pair_count_order();
    REQUIRE( !max.has_value() );
    pc.create_or_modify_pair(1,2,1);
    max = pc.get_top_pair_count_order();
    REQUIRE( max.has_value() );
    REQUIRE( max.value().pair == make_pair(1,2) );

    pc.create_or_modify_pair(1,2,1);
    pc.create_or_modify_pair(2,3,1);
    max = pc.get_top_pair_count_order();
    REQUIRE( max.has_value() );
    REQUIRE( max.value().pair == make_pair(1,2) );

    pc.create_or_modify_pair(2,3,1);
    pc.create_or_modify_pair(2,3,1);
    max = pc.get_top_pair_count_order();
    REQUIRE( max.has_value() );
    REQUIRE( max.value().pair == make_pair(2,3) );

    pc.create_or_modify_pair(1,2,1);

    max = pc.get_top_pair_count_order();
    REQUIRE( max.has_value() );
    REQUIRE( (max.value().pair == make_pair(1,2) ) );
}

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

  void merge_public(std::forward_list<int> &text, pair<int, int> mp,
                    int new_token, PairCount &freqs) {
    merge(text, mp, new_token, freqs);
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

void print_flist(const char* msg, const std::forward_list<int>& flist) {
    cout << msg;
    for (const auto& token : flist) {
        cout << token << " ";
    }
    cout << std::endl;
}

TEST_CASE("Tokenizer training", "[tokenizer]") {
    TokenizerTest bt;
    vector<vector<int>> chunks;
    const auto test_string = string("abcbcde");
    chunks.push_back(bt.text_to_vector_public(test_string));
    auto flists = bt.create_lists_public(chunks);
    REQUIRE( flists.size() == 1 );
    REQUIRE( getForwardListLength(flists[0]) == test_string.size());

    auto freqs = bt.calculate_freqs_public(flists);

    auto max = freqs.get_top_pair_count_order();
    REQUIRE( max.has_value() ); 
    REQUIRE( max.value().pair == make_pair(98,99) );

    // print_flist("Before merge 1: ", flists[0]);
    bt.merge_public(flists[0], make_pair(98,99), 256, freqs);
    // print_flist("After merge 1:  ", flists[0]);
    
    freqs = bt.calculate_freqs_public(flists);
    max = freqs.get_top_pair_count_order();

    REQUIRE( max.has_value() ); 
    REQUIRE( max.value().pair == make_pair(97,256) );

    // print_flist("Before merge 2: ", flists[0]);
    bt.merge_public(flists[0], make_pair(97,256), 257, freqs);
    // print_flist("After merge 2:  ", flists[0]);

    freqs = bt.calculate_freqs_public(flists);
    max = freqs.get_top_pair_count_order();

    REQUIRE( max.has_value() ); 
    REQUIRE( max.value().pair == make_pair(257,256) );

    const auto& freqs_index = freqs.get_index_by_key();

    // Verify the contents of freqs
    auto p1 = freqs.get_pair({256, 100});
    REQUIRE(p1.has_value());
    REQUIRE(p1.value() == 1);

    auto p2 = freqs.get_pair({257, 256});
    REQUIRE(p2.has_value());
    REQUIRE(p2.value() == 1);

    auto p3 = freqs.get_pair({100, 101});
    REQUIRE(p3.has_value());
    REQUIRE(p3.value() == 1);

    // Also check that the total number of pairs is 3
    REQUIRE(freqs.get_count() == 3);
}
