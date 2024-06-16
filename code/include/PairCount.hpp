#ifndef MINBPE_PAIRCOUNT_HPP
#define MINBPE_PAIRCOUNT_HPP

#include <tuple>
#include <optional>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <reflex/boostmatcher.h>

using std::tuple;
using std::optional;

struct CountOrder {
  int count;
  int insert_order;
};
struct PairCountOrder {
  tuple<int,int> pair;
  CountOrder countOrder;
};
struct CompareCountOrder {
    bool operator()(const CountOrder& a, const CountOrder& b) const {
        if (a.count == b.count) {
            return a.insert_order < b.insert_order; // lower insert_order is greater
        }
        return a.count > b.count; // higher count is greater
    }
};

typedef boost::multi_index_container<
    PairCountOrder,
    boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<boost::multi_index::member<PairCountOrder, tuple<int,int>, &PairCountOrder::pair>>,
        boost::multi_index::ordered_non_unique<boost::multi_index::member<PairCountOrder, CountOrder, &PairCountOrder::countOrder>, CompareCountOrder>
    > 
  > PairCountStore;

class PairCount {
  private:
    PairCountStore pcs;
    int insertOrder = 0;
  public:
    PairCount() {
    }

    auto get_count() {
      return pcs.size();
    }

    auto get_insert_order() {
      return insertOrder;
    }

    void increment_freq_count(tuple<int,int> mp) {
      auto& index_by_key = pcs.get<0>();
      auto f = index_by_key.find(mp);
      if(f != pcs.end()) {
        index_by_key.modify(f, [](PairCountOrder& pc) { pc.countOrder.count++; });
      } else {
        pcs.insert({mp, CountOrder{1,insertOrder++}});
      }
    }
    void decrement_freq_count(tuple<int,int> mp) {
      auto& index_by_key = pcs.get<0>();
      auto f = index_by_key.find(mp);
      if(f != pcs.end()) {
        index_by_key.modify(f, [](PairCountOrder& pc) { pc.countOrder.count--; });
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

#endif
