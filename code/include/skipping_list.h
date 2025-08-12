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

        // Prefix increment - this is where the daisy-chain skipping happens.
        iterator& operator++() {
            if (current_ != parent_->list_.end()) {
                T skip_val = parent_->get_skip(*current_);
                current_ += (1 + skip_val); // Perform the initial jump

                // Continue jumping only if we land on a node that is ALSO a max-skip node
                while (current_ < parent_->list_.end() && parent_->get_skip(*current_) == parent_->max_skip_val_) {
                    skip_val = parent_->get_skip(*current_);
                    current_ += (1 + skip_val);
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
              T skip_val = parent_->get_skip(*current_);
              current_ += (1 + skip_val); // Perform the initial jump

              // Continue jumping only if we land on a node that is ALSO a max-skip node
              while (current_ < parent_->list_.end() && parent_->get_skip(*current_) == parent_->max_skip_val_) {
                  skip_val = parent_->get_skip(*current_);
                  current_ += (1 + skip_val);
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

    iterator erase_after(const_iterator position) {
      // 1. Find the physical node 'p_it' (at the given position) and 'e_it' (the logical node to be erased).
      auto it_to_erase = position;
      ++it_to_erase;
      if (it_to_erase == end()) {
          return end(); // Nothing to erase.
      }

      auto p_it = list_.begin() + std::distance(list_.cbegin(), position.current_);
      auto e_it = list_.begin() + std::distance(list_.cbegin(), it_to_erase.current_);

      // 2. Calculate the total number of physical nodes that 'p_it' must now skip.
      // This is the physical distance from 'p_it' to 'e_it', plus any nodes 'e_it' was already skipping.
      T total_skips_needed = std::distance(p_it, e_it) + get_skip(*e_it);
      
      // 3. The skips from the deleted node are now accounted for, so clear them from the original location.
      set_skip(*e_it, 0);

      // 4. Starting from 'p_it', "pave" a new, correct daisy-chain by overwriting skip values
      //    until all 'total_skips_needed' are distributed.
      auto current_link_it = p_it;
      T remaining_skips = total_skips_needed;

      while (remaining_skips > 0) {
          assert(current_link_it < list_.end() && "Daisy chain ran off the end of the list");

          // Determine how many skips this current node can hold.
          T skips_for_this_node = std::min(remaining_skips, max_skip_val_);
          set_skip(*current_link_it, skips_for_this_node);
          
          remaining_skips -= skips_for_this_node;
          
          if (remaining_skips > 0) {
              // We maxed out this node. Jump to the node *after* the physical block
              // we just created to place the next link of the daisy chain.
              current_link_it += (1 + skips_for_this_node);
          }
      }
      
      // 5. Return an iterator to the element that now logically follows the original position.
      auto next_valid_it = position;
      ++next_valid_it; // This uses the newly created skipping logic.
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
