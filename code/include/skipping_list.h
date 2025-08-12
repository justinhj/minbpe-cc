#ifndef MINBPE_SKIPPING_LIST_H
#define MINBPE_SKIPPING_LIST_H

#include <vector>

namespace MinBpeCC::Util {

template<typename T>
class skipping_list {
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    explicit skipping_list(const std::vector<T>& values, int max_skips) : list_(values), max_skips_(max_skips) {
    }

    // Iterator access
    iterator begin() { return list_.begin(); }
    const_iterator begin() const { return list_.begin(); }
    iterator end() { return list_.end(); }
    const_iterator end() const { return list_.end(); }

    // Size
    size_t size() const {
        return list_.size();
    }

private:
    std::vector<T> list_;
    int max_skips_;
};

} // namespace MinBpeCC::Util

#endif // MINBPE_SKIPPING_LIST_H