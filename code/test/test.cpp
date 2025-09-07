#include "Tokenizer.h"
#include <catch_amalgamated.hpp>
#include <utility>
#include <boost/intrusive/list.hpp>

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

    auto calculate_freqs_public(const vector<MinBpeCC::Tokenizer::TokenList> &chunks, CONFLICT_RESOLUTION conflict_resolution) {
        return calculate_freqs(chunks, conflict_resolution);
    };

    void merge_public(MinBpeCC::Tokenizer::TokenList &text, pair<MinBpeCC::Tokenizer::Token, MinBpeCC::Tokenizer::Token> mp,
                      MinBpeCC::Tokenizer::Token new_token, PairCount<MinBpeCC::Tokenizer::Token> *freqs) {
        merge(text, mp, new_token, freqs);
    }
};

// Helper to get the length of a TokenList
template<typename T>
size_t getTokenListLength(const T& tlist) {
    return std::distance(tlist.begin(), tlist.end());
}

TEST_CASE("Tokenizer training", "[tokenizer]") {
    TokenizerTest bt;
    vector<vector<MinBpeCC::Tokenizer::Token>> chunks;
    const auto test_string = string("abcbcde");
    chunks.push_back(bt.text_to_vector_public(test_string));

    auto tlists = bt.create_lists_public(chunks);
    REQUIRE( tlists.size() == 1 );
    REQUIRE( getTokenListLength(tlists[0]) == test_string.size());

    // FIX: `freqs` is now a std::unique_ptr, so we use it like a pointer.
    auto freqs = bt.calculate_freqs_public(tlists, Tokenizer::CONFLICT_RESOLUTION::FIRST);

    // FIX: Use the -> operator to access members of the object managed by unique_ptr.
    auto max = freqs->get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair((MinBpeCC::Tokenizer::Token)'b', (MinBpeCC::Tokenizer::Token)'c') ); // 98, 99

    // FIX: Pass the raw pointer using .get() to the merge function.
    bt.merge_public(tlists[0], make_pair((MinBpeCC::Tokenizer::Token)'b', (MinBpeCC::Tokenizer::Token)'c'), 256, freqs.get());

    // Recalculate frequencies and re-assign the unique_ptr.
    freqs = bt.calculate_freqs_public(tlists, Tokenizer::CONFLICT_RESOLUTION::FIRST);
    max = freqs->get_top_pair_count();
    REQUIRE( max.has_value() );
    REQUIRE( max.value() == make_pair((MinBpeCC::Tokenizer::Token)'a', (MinBpeCC::Tokenizer::Token)256) ); // 97, 256

    bt.merge_public(tlists[0], make_pair((MinBpeCC::Tokenizer::Token)'a', (MinBpeCC::Tokenizer::Token)256), 257, freqs.get());

    freqs = bt.calculate_freqs_public(tlists, Tokenizer::CONFLICT_RESOLUTION::FIRST);
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

// Simple test for intrusive list functionality
TEST_CASE("Intrusive list test", "[intrusive]") {
    using namespace MinBpeCC::Tokenizer;
    
    // Create a vector of tokens
    vector<Token> tokens = {1, 2, 3, 4, 5};
    
    // Create nodes and add them to the list
    vector<TokenNode> nodes;
    for (const auto& t : tokens) {
        nodes.emplace_back(t);
    }
    
    TokenList tokenList;
    for (auto& node : nodes) {
        tokenList.push_back(node);
    }
    
    // Compare the contents
    auto it = tokenList.begin();
    for (size_t i = 0; i < tokens.size(); ++i, ++it) {
        REQUIRE(it->value == tokens[i]);
    }
    
    REQUIRE(std::distance(tokenList.begin(), tokenList.end()) == tokens.size());
    
    // Clear the list before nodes go out of scope to avoid destructor issues
    tokenList.clear();
}

// Test for scanning and replacing tokens in intrusive list
TEST_CASE("Intrusive list scan and replace", "[intrusive]") {
    using namespace MinBpeCC::Tokenizer;
    
    // Create a vector of tokens
    vector<Token> tokens = {1, 2, 3, 4, 5};
    
    // Create nodes and add them to the list
    vector<TokenNode> nodes;
    for (const auto& t : tokens) {
        nodes.emplace_back(t);
    }
    
    TokenList tokenList;
    for (auto& node : nodes) {
        tokenList.push_back(node);
    }
    
    // Scan the list for 2 followed by 3
    auto it = tokenList.begin();
    auto found_pair = false;
    while (it != tokenList.end()) {
        auto next_it = std::next(it);
        if (next_it != tokenList.end() && it->value == 2 && next_it->value == 3) {
            // Replace 2 and 3 with 6
            it->value = 6;
            tokenList.erase(next_it);
            found_pair = true;
            break;
        }
        ++it;
    }
    
    REQUIRE(found_pair == true);
    
    // Verify the final list contains {1, 6, 4, 5}
    vector<Token> expected = {1, 6, 4, 5};
    auto list_it = tokenList.begin();
    for (size_t i = 0; i < expected.size(); ++i, ++list_it) {
        REQUIRE(list_it->value == expected[i]);
    }
    
    REQUIRE(std::distance(tokenList.begin(), tokenList.end()) == expected.size());
    
    // Clear the list before nodes go out of scope to avoid destructor issues
    tokenList.clear();
}
