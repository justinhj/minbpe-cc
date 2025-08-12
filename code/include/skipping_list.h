#ifndef MINBPE_SKIPPING_LIST_H
#define MINBPE_SKIPPING_LIST_H

#include <vector>
#include <iterator>

namespace MinBpeCC::Util {

template<typename T>
class skipping_list {
public:
    class iterator;
    class const_iterator;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator(typename std::vector<T>::iterator it) : current_(it) {}

        reference operator*() const { return *current_; }
        pointer operator->() { return &*current_; }
        iterator& operator++() { ++current_; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++current_; return tmp; }

        friend bool operator==(const iterator& a, const iterator& b) { return a.current_ == b.current_; }
        friend bool operator!=(const iterator& a, const iterator& b) { return a.current_ != b.current_; }

    private:
        friend class skipping_list<T>;
        friend class skipping_list<T>::const_iterator;
        typename std::vector<T>::iterator current_;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator(typename std::vector<T>::const_iterator it) : current_(it) {}
        const_iterator(const iterator& it) : current_(it.current_) {}

        reference operator*() const { return *current_; }
        pointer operator->() { return &*current_; }
        const_iterator& operator++() { ++current_; return *this; }
        const_iterator operator++(int) { const_iterator tmp = *this; ++current_; return tmp; }

        friend bool operator==(const const_iterator& a, const const_iterator& b) { return a.current_ == b.current_; }
        friend bool operator!=(const const_iterator& a, const const_iterator& b) { return a.current_ != b.current_; }

    private:
        friend class skipping_list<T>;
        typename std::vector<T>::const_iterator current_;
    };

    explicit skipping_list(const std::vector<T>& values, int max_skips) : list_(values), max_skips_(max_skips) {
    }

    // Iterator access
    iterator begin() { return iterator(list_.begin()); }
    const_iterator begin() const { return const_iterator(list_.cbegin()); }
    iterator end() { return iterator(list_.end()); }
    const_iterator end() const { return const_iterator(list_.cend()); }

    // Deletion
    iterator erase_after(const_iterator position) {
        auto it_to_erase = std::next(position.current_);
        if (it_to_erase != list_.cend()) {
            auto next_it = list_.erase(it_to_erase);
            return iterator(next_it);
        }
        return end();
    }

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
