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

using std::pair;
using std::optional;

namespace MinBpeCC::Util {

struct PairCountOrder {
  ::pair<int,int> pair;
  int count;
  size_t first_occurrence;
  PairCountOrder(::pair<int,int> p, int c, size_t fo) : pair(p), count(c), first_occurrence(fo) {}
  PairCountOrder(::pair<int,int> p, int c) : pair(p), count(c), first_occurrence(std::numeric_limits<size_t>::max()) {}
};

struct CompareCountOrder {
    bool operator()(const PairCountOrder& a, const PairCountOrder& b) const {
      if(a.count == b.count) {
        return a.first_occurrence < b.first_occurrence;
      } else {
        return a.count > b.count; // higher count is greater
      }
    }
};

using boost::multi_index::hashed_unique;
using boost::multi_index::ordered_unique;
using boost::multi_index::indexed_by;
using boost::multi_index::member;
using boost::multi_index::identity;

typedef boost::multi_index_container<
    PairCountOrder,
    indexed_by<
        hashed_unique<member<PairCountOrder, pair<int,int>, &PairCountOrder::pair>>,
        ordered_unique<identity<PairCountOrder>, CompareCountOrder>
    > 
> PairCountStore;

class PairCount {
  private:
    PairCountStore pcs;
  public:
    PairCount() {}

    auto get_count() {
      return pcs.size();
    }

    optional<int> get_pair(pair<int,int> mp) {
      auto& index_by_key = pcs.get<0>();
      auto f = index_by_key.find(mp);
      if(f != pcs.end()) {
        return (*f).count;
      } else {
        return {};
      }
    }

    // Add or increment/decrement a pair, tracking first occurrence
    void add_pair(int a, int b, int freq, size_t first_occurrence) {
      pair<int,int> mp = {a, b};
      auto& index_by_key = pcs.get<0>();
      auto f = index_by_key.find(mp);
      if(f != pcs.end()) {
        index_by_key.modify(f, [freq](PairCountOrder& pc) { pc.count += freq; });
      } else {
        pcs.insert(PairCountOrder(mp, freq, first_occurrence));
      }
    }

    void increment_freq_count(pair<int,int> mp, size_t first_occurrence) {
      auto& index_by_key = pcs.get<0>();
      auto f = index_by_key.find(mp);
      if(f != pcs.end()) {
        index_by_key.modify(f, [](PairCountOrder& pc) { pc.count++; });
      } else {
        pcs.insert(PairCountOrder(mp, 1, first_occurrence));
      }
    }
    void increment_freq_count(pair<int,int> mp) {
      increment_freq_count(mp, std::numeric_limits<size_t>::max());
    }

    void decrement_freq_count(pair<int,int> mp) {
      auto& index_by_key = pcs.get<0>();
      auto f = index_by_key.find(mp);
      if(f != pcs.end()) {
        index_by_key.modify(f, [](PairCountOrder& pc) { pc.count--; });
      }
    }

    const auto &get_index_by_count() {
      return pcs.get<1>();
    }

    auto get_top_pair_count_order() {
      const auto& index_by_count = pcs.get<1>();
      if(!index_by_count.empty()) {
        return optional<PairCountOrder>(*index_by_count.begin());
      } else {
        return optional<PairCountOrder>();
      }
    }

    const auto &get_index_by_key() {
      return pcs.get<0>();
    }

    const auto end() {
      return pcs.end();
    }
};

}

#endif
