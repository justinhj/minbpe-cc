#ifndef MINBPE_SKIPPING_LIST_H
#define MINBPE_SKIPPING_LIST_H

#include <vector>
#include <forward_list>
#include <iterator>
#include <algorithm>

namespace MinBpeCC::Util {

template<typename T>
class skipping_list {
public:
    using iterator = typename std::forward_list<T>::iterator;
    using const_iterator = typename std::forward_list<T>::const_iterator;

    explicit skipping_list(const std::vector<T>& values) {
        // To mimic forward_list creation from a vector in existing code
        std::copy(values.rbegin(), values.rend(), std::front_inserter(list_));
    }

    // Iterator access
    iterator before_begin() { return list_.before_begin(); }
    const_iterator before_begin() const { return list_.before_begin(); }
    iterator begin() { return list_.begin(); }
    const_iterator begin() const { return list_.begin(); }
    iterator end() { return list_.end(); }
    const_iterator end() const { return list_.end(); }

    // Deletion
    iterator erase_after(const_iterator position) {
        return list_.erase_after(position);
    }

    // Size
    size_t size() const {
        return std::distance(begin(), end());
    }

private:
    std::forward_list<T> list_;
};

} // namespace MinBpeCC::Util

#endif // MINBPE_SKIPPING_LIST_H
