/*
 * Copyright (C) 2016 Olzhas Rakhimov
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

/// @file ext.h
/// Helpful facilities as an extension to the standard library or Boost.

#ifndef SCRAM_SRC_EXT_H_
#define SCRAM_SRC_EXT_H_

#include <algorithm>
#include <memory>
#include <type_traits>

#include <boost/range/algorithm/find_if.hpp>

namespace ext {

/// Iterator adaptor for indication of container ``find`` call results.
/// Conveniently wraps common calls after ``find`` into implicit Boolean value.
///
/// @tparam Iterator  Iterator type belonging to the container.
template <class Iterator>
class find_iterator : public Iterator {
 public:
  /// Initializes the iterator as the result of ``find()``.
  ///
  /// @param[in] it  The result of ``find`` call.
  /// @param[in] it_end  The sentinel iterator indicator ``not-found``.
  find_iterator(Iterator&& it, const Iterator& it_end)
      : Iterator(std::forward<Iterator>(it)),
        found_(it != it_end) {}

  /// @returns true if the iterator indicates that the item is found.
  explicit operator bool() { return found_; }

 private:
  bool found_;  ///< Indicator of the lookup result.
};

/// Wraps ``container::find()`` calls for convenient and efficient lookup
/// with ``find_iterator`` adaptor.
///
/// @tparam T  Container type supporting ``find()`` and ``end()`` calls.
/// @tparam Ts  The argument types to the ``find()`` call.
///
/// @param[in] container  The container to operate upon.
/// @param[in] args  Arguments to the ``find()`` function.
///
/// @returns find_iterator wrapping the resultant iterator.
template <class T, typename... Ts>
auto find(T&& container, Ts&&... args)
    -> find_iterator<decltype(container.end())> {
  auto it = container.find(std::forward<Ts>(args)...);
  return find_iterator<decltype(it)>(std::move(it), container.end());
}

/// C++14 make_unique substitute for non-array types.
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  static_assert(std::is_array<T>::value == false,
                "This extension make_unique doesn't support arrays.");
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

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
  BOOST_RANGE_CONCEPT_ASSERT(
      (boost::SinglePassRangeConcept<const SinglePassRange1>));
  BOOST_RANGE_CONCEPT_ASSERT(
      (boost::SinglePassRangeConcept<const SinglePassRange2>));
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
  BOOST_RANGE_CONCEPT_ASSERT(
      (boost::SinglePassRangeConcept<const SinglePassRange>));
  return boost::end(rng) ==
         std::find_if_not(boost::begin(rng), boost::end(rng), pred);
}
/// @}

}  // namespace ext

#endif  // SCRAM_SRC_EXT_H_
