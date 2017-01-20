/*
 * Copyright (C) 2016-2017 Olzhas Rakhimov
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

/// @file combination_iterator.h
/// n-choose-k combination generation facilities.

#ifndef SCRAM_SRC_EXT_COMBINATION_ITERATOR_H_
#define SCRAM_SRC_EXT_COMBINATION_ITERATOR_H_

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

namespace ext {

/// Read forward-iterator for combination generation.
/// The combination generator guarantees the element order
/// to be the same as in the original source collection.
///
/// @tparam Iterator  A random access iterator type.
/// @tparam value_type  The container type to generate combinations into.
template <class Iterator,
          class value_type = std::vector<typename Iterator::value_type>>
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
      : first1_(first1), bitmask_(std::distance(first1, last1)) {
    assert(k > 0 && "The choice must be positive.");
    assert(k <= std::distance(first1, last1) && "The choice can't exceed N.");
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
    value_type combination;
    for (int i = 0; i < bitmask_.size(); ++i) {
      if (bitmask_[i])
        combination.push_back(*std::next(first1_, i));
    }
    return combination;
  }
  /// @}
  Iterator first1_;           ///< The first element in the collection.
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

#endif  // SCRAM_SRC_EXT_COMBINATION_ITERATOR_H_
