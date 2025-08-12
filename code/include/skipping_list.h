#ifndef MINBPE_SKIPPING_LIST_H
#define MINBPE_SKIPPING_LIST_H

#include <vector>
#include <iterator>
#include <concepts>
#include <algorithm> // For std::min
#include <cassert>   // For assert

namespace MinBpeCC::Util {

template<std::integral T>
class skipping_list {
private:
    // The number of high bits in T to use for the skip count.
    int max_skips_;
    // The maximum value the skip count can hold (2^max_skips - 1).
    T max_skip_val_;
    // Bitmasks for separating the value and the skip count.
    T value_mask_;
    T skip_mask_;
    int skip_shift_;

    // Sets up the bitmasks based on max_skips_.
    void setup_masks() {
        assert(max_skips_ > 0 && max_skips_ < sizeof(T) * 8);
        skip_shift_ = sizeof(T) * 8 - max_skips_;
        max_skip_val_ = (T(1) << max_skips_) - 1;
        skip_mask_ = max_skip_val_ << skip_shift_;
        value_mask_ = ~skip_mask_;
    }

    // Gets the user-facing value from an element, excluding skip bits.
    T get_value(const T& element) const {
        return element & value_mask_;
    }

    // Gets the skip count from an element's high bits.
    T get_skip(const T& element) const {
        return (element & skip_mask_) >> skip_shift_;
    }

    // Sets the skip count on an element, preserving its value.
    void set_skip(T& element, T count) {
        element = (element & value_mask_) | (count << skip_shift_);
    }

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

        iterator(skipping_list* parent, typename std::vector<T>::iterator it) : parent_(parent), current_(it) {}

        // Dereferencing returns the masked value, not the raw stored value.
        value_type operator*() const { return parent_->get_value(*current_); }

        // Prefix increment - this is where the skipping happens.
        iterator& operator++() {
            if (current_ != parent_->list_.end()) {
                T skip_count = parent_->get_skip(*current_);
                current_ += (1 + skip_count);
            }
            return *this;
        }

        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const iterator& a, const iterator& b) { return a.current_ == b.current_; }
        friend bool operator!=(const iterator& a, const iterator& b) { return a.current_ != b.current_; }

    private:
        friend class skipping_list<T>;
        friend class skipping_list<T>::const_iterator;
        skipping_list* parent_;
        typename std::vector<T>::iterator current_;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator(const skipping_list* parent, typename std::vector<T>::const_iterator it) : parent_(parent), current_(it) {}
        const_iterator(const iterator& it) : parent_(it.parent_), current_(it.current_) {}

        value_type operator*() const { return parent_->get_value(*current_); }

        const_iterator& operator++() {
            if (current_ != parent_->list_.end()) {
                T skip_count = parent_->get_skip(*current_);
                current_ += (1 + skip_count);
            }
            return *this;
        }

        const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const const_iterator& a, const const_iterator& b) { return a.current_ == b.current_; }
        friend bool operator!=(const const_iterator& a, const const_iterator& b) { return a.current_ != b.current_; }

    private:
        friend class skipping_list<T>;
        const skipping_list* parent_;
        typename std::vector<T>::const_iterator current_;
    };

    explicit skipping_list(const std::vector<T>& values, int max_skips) : max_skips_(max_skips) {
        setup_masks();
        list_.reserve(values.size());
        for (const auto& val : values) {
            // Ensure skip bits are zeroed out on initial insertion.
            list_.push_back(val & value_mask_);
        }
    }

    iterator begin() { return iterator(this, list_.begin()); }
    const_iterator begin() const { return const_iterator(this, list_.cbegin()); }
    iterator end() { return iterator(this, list_.end()); }
    const_iterator end() const { return const_iterator(this, list_.cend()); }

    // Efficiently "deletes" an element by incrementing the skip count of the preceding element.
    iterator erase_after(const_iterator position) {
        // We need a non-const iterator to modify the list.
        auto dist = std::distance(list_.cbegin(), position.current_);
        auto current_node_it = list_.begin() + dist;

        auto element_to_delete_it = std::next(current_node_it);

        if (element_to_delete_it == list_.end()) {
            return end(); // Nothing to erase after the given position.
        }

        // The total number of skips to add is 1 (for the deleted element) plus any skips
        // the deleted element was already responsible for.
        T skips_to_add = 1 + get_skip(*element_to_delete_it);
        
        // The deleted element's skips are now absorbed, so clear them.
        set_skip(*element_to_delete_it, 0);

        // This loop implements the "daisy-chaining". If the current node can't hold all
        // the new skips, it's filled to capacity, and the remainder is passed to the
        // next node in the chain.
        while (skips_to_add > 0) {
            assert(current_node_it < list_.end() && "Daisy chain ran off the end of the list");

            T current_skips = get_skip(*current_node_it);
            T available_capacity = max_skip_val_ - current_skips;
            T can_add = std::min(skips_to_add, available_capacity);

            set_skip(*current_node_it, current_skips + can_add);
            skips_to_add -= can_add;

            if (skips_to_add > 0) {
                // We maxed out the current node, so we find the next node in the chain
                // to offload the remaining skips to.
                current_node_it += (1 + current_skips); // Jump over the original skip block.
            }
        }
        
        // Return an iterator to the element that logically follows the erased one.
        auto next_valid_it = position;
        ++next_valid_it; // This uses the new skipping logic.
        
        // We have a const_iterator but need to return a non-const one.
        auto final_dist = std::distance(list_.cbegin(), next_valid_it.current_);
        return iterator(this, list_.begin() + final_dist);
    }

    // Returns the number of "active" (not deleted) elements. This is slow (O(N)).
    size_t size() const {
        size_t count = 0;
        for (auto it = begin(); it != end(); ++it) {
            count++;
        }
        return count;
    }

private:
    std::vector<T> list_;
};

} // namespace MinBpeCC::Util

#endif // MINBPE_SKIPPING_LIST_H