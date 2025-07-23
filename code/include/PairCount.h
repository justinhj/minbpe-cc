#ifndef MINBPE_PAIRCOUNT_HPP
#define MINBPE_PAIRCOUNT_HPP

#include <boost/multi_index/identity.hpp>
#include <utility>
#include <optional>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <limits>
#include <vector>

using std::pair;
using std::optional;

namespace MinBpeCC::Util {

/**
 * @class PairCount
 * @brief An abstract base class defining the interface for a pair counting container.
 *
 * This class provides a contract for different implementations of pair frequency counters.
 * It allows for querying counts, modifying pairs, and retrieving the most frequent pair.
 */
class PairCount {
public:
    // Virtual destructor to ensure proper cleanup of derived classes.
    virtual ~PairCount() = default;

    // Gets the total number of unique pairs stored.
    virtual size_t get_count() = 0;

    // Retrieves the count for a specific pair.
    virtual optional<int> get_pair(pair<int,int> mp) = 0;

    // Creates a new pair or modifies the frequency of an existing one.
    virtual bool create_or_modify_pair(int a, int b, int freq) = 0;

    // Gets the pair with the highest count.
    virtual optional<pair<int,int>> get_top_pair_count() = 0;

    // Retrieves all pairs and their counts.
    virtual std::vector<std::vector<int>> get_all() = 0;
};


// --- Implementation: PairCountInsertOrder ---
// This implementation breaks ties based on the insertion order.

// Struct to hold the pair, its count, and its insertion order.
struct PairCountOrder {
    ::pair<int,int> pair;
    int count;
    size_t insert_order;

    PairCountOrder(::pair<int,int> p, int c, size_t fo) : pair(p), count(c), insert_order(fo) {}
    PairCountOrder(::pair<int,int> p, int c) : pair(p), count(c), insert_order(std::numeric_limits<size_t>::max()) {}
};

// Comparison struct for sorting. Sorts by count (descending), then by insertion order (ascending).
struct CompareCountOrder {
    bool operator()(const PairCountOrder& a, const PairCountOrder& b) const {
        if(a.count == b.count) {
            return a.insert_order < b.insert_order;
        } else {
            return a.count > b.count; // higher count is greater
        }
    }
};

using boost::multi_index::hashed_unique;
using boost::multi_index::ordered_non_unique;
using boost::multi_index::indexed_by;
using boost::multi_index::member;
using boost::multi_index::identity;

// The underlying data store using Boost.MultiIndex
using PairCountStore = boost::multi_index_container<
    PairCountOrder,
    indexed_by<
        // Index 0: Hashed unique index on the 'pair' member for fast lookups.
        hashed_unique<member<PairCountOrder, pair<int,int>, &PairCountOrder::pair>>,
        // Index 1: Ordered non-unique index for sorting by count and insertion order.
        ordered_non_unique<identity<PairCountOrder>, CompareCountOrder>
    >
>;

/**
 * @class PairCountInsertOrder
 * @brief An implementation of PairCount that tracks frequency and breaks ties with insertion order.
 *
 * This class uses a Boost.MultiIndex container to efficiently store and retrieve
 * pairs, sorted primarily by their frequency count and secondarily by when they were first added.
 */
class PairCountInsertOrder : public PairCount {
private:
    PairCountStore pcs;
    size_t next_insert = 0;

public:
    PairCountInsertOrder() {}

    /**
     * @brief Gets the total number of unique pairs.
     * @return The number of pairs.
     */
    size_t get_count() override {
        return pcs.size();
    }

    /**
     * @brief Gets the frequency count of a given pair.
     * @param mp The pair to look up.
     * @return An optional containing the count if the pair exists, otherwise an empty optional.
     */
    [[nodiscard]] optional<int> get_pair(pair<int,int> mp) override {
        auto& index_by_key = pcs.get<0>();
        auto f = index_by_key.find(mp);
        if(f != pcs.end()) {
            return (*f).count;
        } else {
            return {};
        }
    }

    /**
     * @brief Adds a new pair or modifies the count of an existing pair.
     * If a pair is new, its insertion order is recorded.
     * @param a The first element of the pair.
     * @param b The second element of the pair.
     * @param freq The value to add to the pair's count (can be negative).
     * @return True if the pair was newly created, false if it already existed.
     */
    bool create_or_modify_pair(int a, int b, int freq) override {
        pair<int,int> mp = {a, b};
        auto& index_by_key = pcs.get<0>();
        auto f = index_by_key.find(mp);
        if(f != pcs.end()) {
            index_by_key.modify(f, [freq](PairCountOrder& pc) { pc.count += freq; });
            return false;
        } else {
            pcs.insert(PairCountOrder(mp, freq, next_insert++));
            return true;
        }
    }

    /**
     * @brief Retrieves the pair with the highest frequency count.
     * Ties are broken by the earliest insertion order.
     * @return An optional containing the top pair if the container is not empty, otherwise an empty optional.
     */
    optional<pair<int,int>> get_top_pair_count() override {
        const auto& index_by_count = pcs.get<1>();
        if(!index_by_count.empty()) {
            return optional<pair<int,int>>((*index_by_count.begin()).pair);
        } else {
            return optional<pair<int,int>>();
        }
    }

    /**
     * @brief Retrieves all stored pairs along with their counts.
     * The order is not guaranteed.
     * @return A vector of vectors, where each inner vector is {pair.first, pair.second, count}.
     */
    std::vector<std::vector<int>> get_all() override {
        std::vector<std::vector<int>> result;
        result.reserve(pcs.size());
        for (const auto& pco : pcs) {
            result.push_back({pco.pair.first, pco.pair.second, pco.count});
        }
        return result;
    }
};

/**
 * @class PairCountLexicalOrder
 * @brief A stubbed implementation of PairCount that will track frequency and break ties with lexical order.
 *
 * NOTE: This is a stub implementation and is not yet functional.
 */
class PairCountLexicalOrder : public PairCount {
public:
    PairCountLexicalOrder() {}

    size_t get_count() override {
        // Not implemented
        throw std::logic_error("Function not implemented.");
        return 0;
    }

    [[nodiscard]] optional<int> get_pair(pair<int,int> mp) override {
        // Not implemented
        throw std::logic_error("Function not implemented.");
        return {};
    }

    bool create_or_modify_pair(int a, int b, int freq) override {
        // Not implemented
        throw std::logic_error("Function not implemented.");
        return false;
    }

    optional<pair<int,int>> get_top_pair_count() override {
        // Not implemented
        throw std::logic_error("Function not implemented.");
        return {};
    }

    std::vector<std::vector<int>> get_all() override {
        // Not implemented
        throw std::logic_error("Function not implemented.");
        return {};
    }
};

} // namespace MinBpeCC::Util

#endif // MINBPE_PAIRCOUNT_HPP
