/*
 * Copyright (C) 2017 Olzhas Rakhimov
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

/// @file bits.h
/// Helper constexpr functions to make bit operations explicit.

#ifndef SCRAM_SRC_EXT_BITS_H_
#define SCRAM_SRC_EXT_BITS_H_

namespace ext {

/// Tests the value of a bit.
///
/// @param[in] bits  The array of bits.
/// @param[in] index  The index of the bit to test.
///
/// @returns The bit value in the given index.
///
/// @pre The index is in [0, size(bit-array)).
constexpr bool test_bit(std::uint64_t bits, int index) noexcept {
  return bits & (std::uint64_t(1) << index);
}

/// Counts the trailing zero bits.
///
/// @param[in] bits  The positive number representing a bit array.
///
/// @returns The number of trailing 0 bits
///          (the index of the first 1 bit).
///
/// @pre bits is not 0.
constexpr int count_trailing_zero_bits(std::uint64_t bits) noexcept {
#if defined(__GNUC__)
  return __builtin_ctzl(bits);
#else
  int i = 0;
  while (!test_bit(bits, i))
    ++i;
  return i;
#endif
}

/// Helper function to map single bit integer/enum values into indices.
///
/// @param[in] bits  The value represented as an array of bits.
///
/// @returns The index of the first 1 bit in the positive bit array.
///
/// @pre The bits value is not 0.
constexpr int one_bit_index(std::uint64_t bits) noexcept {
  return count_trailing_zero_bits(bits);
}

}  // namespace ext

#endif  // SCRAM_SRC_EXT_BITS_H_
