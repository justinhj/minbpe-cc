#include "Tokenizer.h"
#include <catch_amalgamated.hpp>
#include <utility>

using MinBpeCC::Tokenizer::Tokenizer;
using MinBpeCC::Util::PairCount;
using MinBpeCC::Util::PairCountInsertOrder;
using MinBpeCC::Util::PairCountLexicalOrder;
using std::vector;
using std::string;
using std::pair;
using std::make_pair;

// Test cases for the PairCountInsertOrder concrete class
TEST_CASE("PairCountInsertOrder allows multiple pairs with the same rank", "[paircount]") {
    // Instantiate the concrete class, not the abstract base class.
    PairCountInsertOrder<int> pc;

    // Add a new pair.
    pc.create_or_modify_pair(10, 20, 1);
    REQUIRE(pc.get_count() == 1);

    // Add a different pair with the same initial count.
    pc.create_or_modify_pair(30, 40, 1);

    // Verify that both pairs were added successfully.
    REQUIRE(pc.get_count() == 2);

    // Verify both pairs are actually in the container.
    auto p1 = pc.get_pair({10, 20});
    auto p2 = pc.get_pair({30, 40});

    REQUIRE(p1.has_value());
    REQUIRE(p1.value() == 1);
    REQUIRE(p2.has_value());
    REQUIRE(p2.value() == 1);
}

TEST_CASE("PairCountInsertOrder add and count", "[paircount]") {
    PairCountInsertOrder<int> pc;
    REQUIRE( pc.get_count() == 0 );
    pc.create_or_modify_pair(1, 2, 1);
    REQUIRE( pc.get_count() == 1 );
    pc.create_or_modify_pair(1, 2, 1); // Increment existing pair
    REQUIRE( pc.get_count() == 1 );
    pc.create_or_modify_pair(2, 3, 1); // Add new pair
    REQUIRE( pc.get_count() == 2 );
}

TEST_CASE("PairCountInsertOrder get most frequent", "[paircount]") {
    PairCountInsertOrder<int> pc;
    auto max = pc.get_top_pair_count();
    REQUIRE( !max.has_value() );

    pc.create_or_modify_pair(1,2,1);
    max = pc.get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair(1,2) );

    pc.create_or_modify_pair(1,2,1); // count(1,2) is 2
    pc.create_or_modify_pair(2,3,1); // count(2,3) is 1
    max = pc.get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair(1,2) );

    pc.create_or_modify_pair(2,3,1); // count(1,2) is 2, count(2,3) is 2. (1,2) was inserted first.
    pc.create_or_modify_pair(2,3,1); // count(1,2) is 2, count(2,3) is 3.
    max = pc.get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair(2,3) );

    pc.create_or_modify_pair(1,2,1); // count(1,2) is 3, count(2,3) is 3. (1,2) was still inserted first.
    max = pc.get_top_pair_count();
    REQUIRE( max.has_value() );
    // Tie-breaking rule: the one inserted first wins. (1,2) was inserted before (2,3).
    // Let's re-add to (1,2) to make it win again.
    pc.create_or_modify_pair(1,2,1); // count(1,2) is 4, count(2,3) is 3.
    max = pc.get_top_pair_count();
    REQUIRE( max.value() == make_pair(1,2) );
}

TEST_CASE("PairCountLexicalOrder get most frequent", "[paircount]") {
    PairCountLexicalOrder<int> pc;
    auto max = pc.get_top_pair_count();
    REQUIRE( !max.has_value() );

    pc.create_or_modify_pair(1,2,1);
    max = pc.get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair(1,2) );

    pc.create_or_modify_pair(1,2,1); // count(1,2) is 2
    pc.create_or_modify_pair(2,3,1); // count(2,3) is 1
    max = pc.get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair(1,2) );

    pc.create_or_modify_pair(2,3,1); // count(1,2) is 2, count(2,3) is 2. (1,2) is smaller
    max = pc.get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair(1,2) );

    pc.create_or_modify_pair(0,1,3); // count(1,2) is 2, count(2,3) is 3, count(0,1) is 3.
    max = pc.get_top_pair_count();
    REQUIRE( max.value() == make_pair(0,1) );
}


// Test helper class to expose protected members of Tokenizer
class TokenizerTest : public Tokenizer {
public:
    auto create_lists_public(const vector<vector<MinBpeCC::Tokenizer::Token>> &chunks) {
        return create_lists(chunks);
    };

    auto text_to_vector_public(const string &text) {
        return text_to_vector(text);
    };

    auto calculate_freqs_public(const vector<std::forward_list<MinBpeCC::Tokenizer::Token>> &chunks, CONFLICT_RESOLUTION conflict_resolution) {
        return calculate_freqs(chunks, conflict_resolution);
    };

    void merge_public(std::forward_list<MinBpeCC::Tokenizer::Token> &text, pair<MinBpeCC::Tokenizer::Token, MinBpeCC::Tokenizer::Token> mp,
                      MinBpeCC::Tokenizer::Token new_token, PairCount<MinBpeCC::Tokenizer::Token> *freqs) {
        merge(text, mp, new_token, freqs);
    }
};

// Helper to get the length of a forward_list
template<typename T>
size_t getForwardListLength(const std::forward_list<T>& flist) {
    return std::distance(flist.begin(), flist.end());
}

TEST_CASE("Tokenizer training", "[tokenizer]") {
    TokenizerTest bt;
    vector<vector<MinBpeCC::Tokenizer::Token>> chunks;
    const auto test_string = string("abcbcde");
    chunks.push_back(bt.text_to_vector_public(test_string));

    auto flists = bt.create_lists_public(chunks);
    REQUIRE( flists.size() == 1 );
    REQUIRE( getForwardListLength(flists[0]) == test_string.size());

    // FIX: `freqs` is now a std::unique_ptr, so we use it like a pointer.
    auto freqs = bt.calculate_freqs_public(flists, Tokenizer::CONFLICT_RESOLUTION::FIRST);

    // FIX: Use the -> operator to access members of the object managed by unique_ptr.
    auto max = freqs->get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair((MinBpeCC::Tokenizer::Token)'b', (MinBpeCC::Tokenizer::Token)'c') ); // 98, 99

    // FIX: Pass the raw pointer using .get() to the merge function.
    bt.merge_public(flists[0], make_pair((MinBpeCC::Tokenizer::Token)'b', (MinBpeCC::Tokenizer::Token)'c'), 256, freqs.get());

    // Recalculate frequencies and re-assign the unique_ptr.
    freqs = bt.calculate_freqs_public(flists, Tokenizer::CONFLICT_RESOLUTION::FIRST);
    max = freqs->get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair((MinBpeCC::Tokenizer::Token)'a', (MinBpeCC::Tokenizer::Token)256) ); // 97, 256

    bt.merge_public(flists[0], make_pair((MinBpeCC::Tokenizer::Token)'a', (MinBpeCC::Tokenizer::Token)256), 257, freqs.get());

    freqs = bt.calculate_freqs_public(flists, Tokenizer::CONFLICT_RESOLUTION::FIRST);
    max = freqs->get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair((MinBpeCC::Tokenizer::Token)257, (MinBpeCC::Tokenizer::Token)256) );

    // FIX: The get_index_by_key() method is no longer public.
    // We can verify the state using the public get_pair() and get_count() methods.
    auto p1 = freqs->get_pair({256, (MinBpeCC::Tokenizer::Token)'d'}); // 256, 100
    REQUIRE(p1.has_value());
    REQUIRE(p1.value() == 1);

    auto p2 = freqs->get_pair({257, 256});
    REQUIRE(p2.has_value());
    REQUIRE(p2.value() == 1);

    auto p3 = freqs->get_pair({(MinBpeCC::Tokenizer::Token)'d', (MinBpeCC::Tokenizer::Token)'e'}); // 100, 101
    REQUIRE(p3.has_value());
    REQUIRE(p3.value() == 1);

    // Also check that the total number of pairs is 3
    REQUIRE(freqs->get_count() == 3);
}
