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

/// @file algorithm.h
/// Helpful algorithm facilities as an extension to the STL or Boost.

#ifndef SCRAM_SRC_EXT_ALGORITHM_H_
#define SCRAM_SRC_EXT_ALGORITHM_H_

#include <algorithm>

#include <boost/range/algorithm.hpp>

namespace ext {

/// Determines if two sorted ranges intersect.
/// This function is complementary to std::set_intersection
/// when the actual intersection container is not needed.
///
/// @tparam Iterator1  Forward iterator type of the first range.
/// @tparam Iterator2  Forward iterator type of the second range.
///
/// @param[in] first1  Start of the first range.
/// @param[in] last1  End of the first range.
/// @param[in] first2  Start of the second range.
/// @param[in] last2  End of the second range.
///
/// @returns true if the [first1, last1) and [first2, last2) ranges intersect.
template <typename Iterator1, typename Iterator2>
bool intersects(Iterator1 first1, Iterator1 last1,
                Iterator2 first2, Iterator2 last2) noexcept {
  while (first1 != last1 && first2 != last2) {
    if (*first1 < *first2) {
      ++first1;
    } else if (*first2 < *first1) {
      ++first2;
    } else {
      return true;
    }
  }
  return false;
}

/// Range-based version of ``intersects``.
template <class SinglePassRange1, class SinglePassRange2>
bool intersects(const SinglePassRange1& rng1, const SinglePassRange2& rng2) {
  return intersects(boost::begin(rng1), boost::end(rng1),
                    boost::begin(rng2), boost::end(rng2));
}

/// Range-based versions of std algorithms missing in Boost.
/// @{
template <class SinglePassRange, class UnaryPredicate>
bool none_of(const SinglePassRange& rng, UnaryPredicate pred) {
  return boost::end(rng) == boost::find_if(rng, pred);
}
template <class SinglePassRange, class UnaryPredicate>
bool any_of(const SinglePassRange& rng, UnaryPredicate pred) {
  return !none_of(rng, pred);
}
template <class SinglePassRange, class UnaryPredicate>
bool all_of(const SinglePassRange& rng, UnaryPredicate pred) {
  return boost::end(rng) ==
         std::find_if_not(boost::begin(rng), boost::end(rng), pred);
}
/// @}

}  // namespace ext

#endif  // SCRAM_SRC_EXT_ALGORITHM_H_
