#ifndef MINBPE_PAIRCOUNT_HPP
#define MINBPE_PAIRCOUNT_HPP

#include <boost/multi_index/identity.hpp>
#include <utility>
#include <optional>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <reflex/boostmatcher.h>

using std::pair;
using std::optional;

namespace MinBpeCC::Util {

struct PairCountOrder {
  pair<int,int> pair;
  int count;
};

struct CompareCountOrder {
    bool operator()(const PairCountOrder& a, const PairCountOrder& b) const {
      if(a.count == b.count) {
        return a.pair < b.pair; // lower pair is greater
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

    void increment_freq_count(pair<int,int> mp) {
      auto& index_by_key = pcs.get<0>();
      auto f = index_by_key.find(mp);
      if(f != pcs.end()) {
        index_by_key.modify(f, [](PairCountOrder& pc) { pc.count++; });
      } else {
        pcs.insert({mp, 1});
      }
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
