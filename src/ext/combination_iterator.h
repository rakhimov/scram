/*
 * Copyright (C) 2016-2018 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file
/// n-choose-k combination generation facilities.

#pragma once

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

namespace ext {

namespace detail {

/// Iterator range to filter according to a bit-mask.
template <class Iterator>
class bitmask_range {
 public:
  /// Filtering iterator.
  class iterator
      : public boost::iterator_facade<iterator, typename Iterator::value_type,
                                      boost::forward_traversal_tag,
                                      typename Iterator::reference> {
    friend class boost::iterator_core_access;

   public:
    /// @param[in] it  The starting/current iterator in the range.
    /// @param[in] it_end  The end iterator for sentinel.
    /// @param[in] it_bit  The bit/filter value provider.
    iterator(Iterator it, Iterator it_end,
             std::vector<bool>::const_iterator it_bit)
        : it_(it), it_end_(it_end), it_bit_(it_bit) {
      apply_filter();
    }

   private:
    /// Standard iterator functionality required by the facade facilities.
    /// @{
    void increment() {
      assert(it_ != it_end_);
      ++it_;
      ++it_bit_;
      apply_filter();
    }
    bool equal(const iterator& other) const {
      return it_ == other.it_ && it_bit_ == other.it_bit_;
    }
    typename Iterator::reference dereference() const { return *it_; }
    /// @}

    /// Applies the bit filter to the current till a positive state reached.
    void apply_filter() {
      while (it_ != it_end_ && !*it_bit_) {
        ++it_;
        ++it_bit_;
      }
    }

    Iterator it_;  ///< The data source.
    Iterator it_end_;  ///< The end sentinel iterator.
    std::vector<bool>::const_iterator it_bit_;  ///< The bit-mask iterator.
  };

  /// @param[in] first1  The start of the range.
  /// @param[in] last1  The sentinel end of the range.
  /// @param[in] bitmask  The bit-mask to filter the range.
  ///
  /// @pre The bit-mask is sufficient for the source range,
  ///      and it is alive for at least as long as this filtering range.
  bitmask_range(Iterator first1, Iterator last1,
                const std::vector<bool>& bitmask)
      : first1_(first1), last1_(last1), bitmask_(bitmask) {}

  /// @returns The iterator to the first positive element.
  iterator begin() { return iterator(first1_, last1_, bitmask_.begin()); }

  /// @returns The sentinel end iterator.
  iterator end() { return iterator(last1_, last1_, bitmask_.end()); }

 private:
  Iterator first1_;  ///< The first element in the collection.
  Iterator last1_;  ///< The sentinel end iterator.
  const std::vector<bool>& bitmask_;  ///< bit-mask for N elements.
};

}  // namespace detail

/// Read forward-iterator for combination generation.
/// The combination generator guarantees the element order
/// to be the same as in the original source collection.
/// It doesn't require values be comparable for lexicographic order.
///
/// @tparam Iterator  A forward iterator type.
/// @tparam value_type  The range type to generate combinations into.
template <class Iterator, class value_type = detail::bitmask_range<Iterator>>
class combination_iterator
    : public boost::iterator_facade<combination_iterator<Iterator>, value_type,
                                    boost::forward_traversal_tag, value_type> {
  friend class boost::iterator_core_access;

 public:
  /// Constructor for a range with N elements to choose from.
  ///
  /// @param[in] k  The number of elements to choose.
  /// @param[in] first1  The start of the range.
  /// @param[in] last1  The sentinel end of the range.
  combination_iterator(int k, Iterator first1, Iterator last1)
      : first1_(first1), last1_(last1), bitmask_(std::distance(first1, last1)) {
    assert(k > 0 && "The choice must be positive.");
    assert(k <= bitmask_.size() && "The choice can't exceed N.");
    std::fill_n(bitmask_.begin(), k, true);
  }

  /// Constructs a special iterator to signal the end of generation.
  explicit combination_iterator(Iterator first1) : first1_(first1) {}

 private:
  /// Standard iterator functionality required by the facade facilities.
  /// @{
  void increment() {
    if (boost::prev_permutation(bitmask_) == false)
      bitmask_.clear();
  }
  bool equal(const combination_iterator& other) const {
    return first1_ == other.first1_ && bitmask_ == other.bitmask_;
  }
  value_type dereference() const {
    assert(!bitmask_.empty() && "Calling on the sentinel iterator.");
    return detail::bitmask_range(first1_, last1_, bitmask_);
  }
  /// @}
  Iterator first1_;  ///< The first element in the collection.
  Iterator last1_;  ///< The sentinel end iterator.
  std::vector<bool> bitmask_;  ///< bit-mask for N elements.
};

/// Helper for N-choose-K combination generator range construction.
template <typename Iterator>
auto make_combination_generator(int k, Iterator first1, Iterator last1) {
  return boost::make_iterator_range(
      combination_iterator<Iterator>(k, first1, last1),
      combination_iterator<Iterator>(first1));
}

}  // namespace ext
