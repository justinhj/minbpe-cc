#ifndef MINBPE_SKIPPING_LIST_H
#define MINBPE_SKIPPING_LIST_H

#include <vector>
#include <iterator>
#include <concepts>
#include <cassert> // For assert

namespace MinBpeCC::Util {

template<std::integral T>
class skipping_list {
private:
    // We use the most significant bit (MSB) as a "deleted" flag.
    // 1 means deleted/skipped, 0 means active.
    static constexpr int skip_shift_ = sizeof(T) * 8 - 1;
    static constexpr T skip_mask_ = T(1) << skip_shift_;
    static constexpr T value_mask_ = ~skip_mask_;

    // Gets the user-facing value from an element, excluding the deleted flag.
    T get_value(const T& element) const {
        return element & value_mask_;
    }

    // Checks if an element is marked as deleted.
    bool is_deleted(const T& element) const {
        return (element & skip_mask_) != 0;
    }

    // Marks an element as deleted by setting its MSB.
    void set_deleted(T& element) {
        element |= skip_mask_;
    }

public:
    class iterator;
    class const_iterator;

    // ## Iterator
    // The iterator is responsible for transparently skipping over deleted elements.
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator(skipping_list* parent, typename std::vector<T>::iterator it) : parent_(parent), current_(it) {
            // Ensure the iterator starts on a valid (non-deleted) element, or at the end.
            while (current_ != parent_->list_.end() && parent_->is_deleted(*current_)) {
                ++current_;
            }
        }

        // Dereferencing returns the masked value, not the raw stored value.
        value_type operator*() const { return parent_->get_value(*current_); }

        // Prefix increment: advances to the next non-deleted element.
        iterator& operator++() {
            if (current_ != parent_->list_.end()) {
                ++current_; // Move to the next physical element
                // Scan forward to find the next valid (non-deleted) element.
                while (current_ != parent_->list_.end() && parent_->is_deleted(*current_)) {
                    ++current_;
                }
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
    
    // ## Const Iterator
    // A read-only version of the iterator.
    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator(const skipping_list* parent, typename std::vector<T>::const_iterator it) : parent_(parent), current_(it) {
            // Ensure the iterator starts on a valid element.
            while (current_ != parent_->list_.end() && parent_->is_deleted(*current_)) {
                ++current_;
            }
        }
        const_iterator(const iterator& it) : parent_(it.parent_), current_(it.current_) {}

        value_type operator*() const { return parent_->get_value(*current_); }

        // Prefix increment: advances to the next non-deleted element.
        const_iterator& operator++() {
            if (current_ != parent_->list_.end()) {
                ++current_; // Move to the next physical element
                // Scan forward to find the next valid element.
                while (current_ != parent_->list_.end() && parent_->is_deleted(*current_)) {
                    ++current_;
                }
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

    // Constructor no longer needs max_skips.
    explicit skipping_list(const std::vector<T>& values) {
        list_.reserve(values.size());
        for (const auto& val : values) {
            // The user-provided value should not have the MSB set, as it's reserved for our flag.
            assert((val & skip_mask_) == 0 && "Initial values must not have the top bit set.");
            list_.push_back(val); // The value is already clean.
        }
    }

    iterator begin() { return iterator(this, list_.begin()); }
    const_iterator begin() const { return const_iterator(this, list_.cbegin()); }
    iterator end() { return iterator(this, list_.end()); }
    const_iterator end() const { return const_iterator(this, list_.cend()); }

    // ## Erase After
    // Deletes the logical element *after* the one at `position`.
    iterator erase_after(const_iterator position) {
        // Find the logical next element to delete using the iterator's skipping logic.
        auto it_to_erase_const = position;
        ++it_to_erase_const;

        if (it_to_erase_const == end()) {
            return end(); // Nothing to erase.
        }

        // Get a non-const iterator to the physical element we need to modify.
        auto dist_erase = std::distance(list_.cbegin(), it_to_erase_const.current_);
        auto element_to_delete_it = list_.begin() + dist_erase;

        // Mark the element as deleted by setting its top bit. No complex chain logic needed.
        set_deleted(*element_to_delete_it);

        // To find the new next valid iterator, we simply increment the original position's iterator again.
        // `operator++` will now correctly skip over the element we just marked as deleted.
        auto next_valid_it = position;
        ++next_valid_it;
        
        // We have a const_iterator but need to return a non-const one.
        // We can construct it from the underlying physical iterator.
        auto final_dist = std::distance(list_.cbegin(), next_valid_it.current_);
        return iterator(this, list_.begin() + final_dist);
    }

    // Returns the number of "active" (not deleted) elements. This is slow (O(N)).
    size_t size() const {
        size_t count = 0;
        // The iterators correctly handle skipping, so this logic remains correct.
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
