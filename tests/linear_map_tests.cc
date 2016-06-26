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

#include "linear_map.h"

#include <string>

#include <boost/container/vector.hpp>

#include <gtest/gtest.h>

// Explicit instantiations with some common types.
template class scram::LinearMap<int, int>;
template class scram::LinearMap<int, double>;
template class scram::LinearMap<int, std::string>;
template class scram::LinearMap<std::string, std::string>;

namespace {
// The bare minimum class to be a key type for the linear map.
class KeyClass {
  friend bool operator==(const KeyClass& lhs, const KeyClass& rhs) {
    return lhs.a == rhs.a && lhs.b == rhs.b;
  }

  int a;
  std::string b;
};
}  // namespace
template class scram::LinearMap<KeyClass, std::string>;

// Passing another underlying container types.
template class scram::LinearMap<int, int, boost::container::vector>;

// Move erase policy instantiation.
template class scram::LinearMap<KeyClass, std::string, std::vector,
                                scram::MoveEraser>;
template class scram::LinearMap<KeyClass, std::string, boost::container::vector,
                                scram::MoveEraser>;

namespace scram {
namespace test {

using IntMap = LinearMap<int, int>;

TEST(LinearMapTest, Constructors) {
  IntMap m_default;
  EXPECT_EQ(0, m_default.size());
  EXPECT_TRUE(m_default.empty());

  IntMap m_init_list = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(3, m_init_list.size());
  EXPECT_FALSE(m_init_list.empty());

  auto m_copy(m_init_list);
  EXPECT_EQ(3, m_copy.size());
  EXPECT_FALSE(m_copy.empty());
  EXPECT_EQ(m_init_list, m_copy);

  // Copy yourself.
  m_copy = m_copy;
  EXPECT_EQ(3, m_copy.size());
  EXPECT_EQ(m_init_list, m_copy);

  auto construct_extra_copy(m_init_list);
  auto m_move(std::move(construct_extra_copy));
  EXPECT_TRUE(construct_extra_copy.empty());
  EXPECT_EQ(3, m_move.size());
  EXPECT_FALSE(m_move.empty());
  EXPECT_EQ(m_init_list, m_move);

#ifndef NDEBUG
  // Move into itself is undefined behavior!
  EXPECT_DEATH(m_move = std::move(m_move), "");
#endif

  // Assignments.
  IntMap m_assign_copy;
  m_assign_copy = m_init_list;
  EXPECT_EQ(m_init_list, m_assign_copy);

  IntMap m_assign_move;
  auto assign_extra_copy(m_init_list);
  m_assign_move = std::move(assign_extra_copy);
  EXPECT_TRUE(assign_extra_copy.empty());
  EXPECT_EQ(m_init_list, m_assign_move);

  IntMap::container_type m_data = {{1, -1}, {2, -2}, {3, -3}, {3, -4}};
  IntMap m_range(m_data.begin(), m_data.end());
  EXPECT_EQ(m_init_list, m_range);

  IntMap m_repeat_init{{1, -1}, {2, -2}, {3, -3}, {3, -4}};
  EXPECT_EQ(m_init_list, m_repeat_init);
}

TEST(LinearMapTest, Equality) {
  IntMap m1;
  EXPECT_EQ(m1, m1);

  IntMap m2;
  EXPECT_EQ(m1, m2);

  m1 = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(m1, m1);
  EXPECT_NE(m1, m2);
  m2 = m1;
  EXPECT_EQ(m1, m2);
  m2 = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(m1, m2);

  m2 = {{2, -2}, {1, -1}, {3, -3}};  // Different order.
  EXPECT_EQ(m1, m2);

  m2 = {{1, -1}, {2, -2}};
  EXPECT_NE(m1, m2);

  m2 = {{1, 1}, {2, 2}, {3, 3}};  // The same keys, but different values.
  EXPECT_NE(m1, m2);
}

TEST(LinearMapTest, Iterators) {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};
  IntMap::container_type c = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(std::next(m.begin(), m.size()), m.end());
  EXPECT_EQ(std::next(m.rbegin(), m.size()), m.rend());
  EXPECT_EQ(std::next(m.cbegin(), m.size()), m.cend());
  EXPECT_EQ(std::next(m.crbegin(), m.size()), m.crend());

  const auto& m_ref = m;
  EXPECT_EQ(std::next(m_ref.begin(), m_ref.size()), m_ref.end());
  EXPECT_EQ(std::next(m_ref.rbegin(), m_ref.size()), m_ref.rend());
  EXPECT_EQ(std::next(m_ref.cbegin(), m_ref.size()), m_ref.cend());
  EXPECT_EQ(std::next(m_ref.crbegin(), m_ref.size()), m_ref.crend());

  auto it_c = c.begin();
  int num_elements = 0;
  int key_sum = 0;
  int value_sum = 0;
  for (const auto& entry : m) {
    EXPECT_EQ(*it_c++, entry);
    ++num_elements;
    key_sum += entry.first;
    value_sum += entry.second;
  }
  EXPECT_EQ(3, num_elements);
  EXPECT_EQ(6, key_sum);
  EXPECT_EQ(-6, value_sum);
  EXPECT_EQ(c, m.data());
}

TEST(LinearMapTest, Clear) {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_FALSE(m.empty());
  m.clear();
  EXPECT_TRUE(m.empty());
}

TEST(LinearMapTest, Reserve) {
  IntMap m;
  m.reserve(1000);
  EXPECT_LE(1000, m.capacity());
}

TEST(LinearMapTest, Swap) {
  IntMap m1 = {{1, -1}, {2, -2}, {3, -3}};
  IntMap m2 = {{4, -4}, {5, -5}};
  IntMap ms1 = m1;
  IntMap ms2 = m2;
  ms1.swap(ms2);
  EXPECT_EQ(m1, ms2);
  EXPECT_EQ(m2, ms1);

  swap(ms1, ms2);
  EXPECT_EQ(m1, ms1);
  EXPECT_EQ(m2, ms2);
}

TEST(LinearMapTest, DefaultErase) {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};
  m.erase(1);
  IntMap m_expected = {{2, -2}, {3, -3}};
  EXPECT_EQ(m_expected, m);

  m.erase(m.begin());
  m_expected = {{3, -3}};
  EXPECT_EQ(m_expected, m);

  m.erase(m.cbegin());
  EXPECT_TRUE(m.empty());
}

TEST(LinearMapTest, MoveErase) {
  using MoveMap = LinearMap<int, int, std::vector, scram::MoveEraser>;
  MoveMap m = {{1, -1}, {2, -2}, {3, -3}};
  m.erase(1);
  MoveMap m_expected = {{3, -3}, {2, -2}};
  EXPECT_EQ(m_expected, m);

  m.erase(m.begin());
  m_expected = {{2, -2}};
  EXPECT_EQ(m_expected, m);

  m.erase(m.cbegin());
  EXPECT_TRUE(m.empty());
}

TEST(LinearMapTest, Find) {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(1, m.count(1));
  EXPECT_EQ(0, m.count(5));

  EXPECT_EQ(m.begin(), m.find(1));
  EXPECT_EQ(1, m.find(1)->first);
  int key = 2;
  EXPECT_EQ(m.begin() + 1, m.find(key));
  EXPECT_EQ(key, m.find(key)->first);
  EXPECT_EQ(m.begin() + 2, m.find(3));
  EXPECT_EQ(m.end(), m.find(5));
}

TEST(LinearMapTest, OperatorIndex) {
  IntMap m;
  m[1] = -1;
  int k = 2;
  m[k] = -2;
  m[3] = -3;
  IntMap expected = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(expected, m);

  m[3] = -4;
  IntMap changed = {{1, -1}, {2, -2}, {3, -4}};
  EXPECT_EQ(changed, m);
}

TEST(LinearMapTest, At) {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(-1, m.at(1));
  EXPECT_THROW(m.at(5), std::out_of_range);

  const auto& m_ref = m;
  EXPECT_EQ(-2, m_ref.at(2));

  m.at(2) = -4;
  IntMap m_expected = {{1, -1}, {2, -4}, {3, -3}};
  EXPECT_EQ(m_expected, m);
}

TEST(LinearMapTest, InsertSingle) {
  IntMap m;
  auto ret = m.insert({1, -1});
  EXPECT_TRUE(ret.second);
  EXPECT_EQ(m.begin(), ret.first);
  EXPECT_EQ(1, ret.first->first);
  EXPECT_EQ(-1, ret.first->second);

  IntMap::value_type v = {2, -2};
  ret = m.insert(v);
  EXPECT_TRUE(ret.second);
  EXPECT_EQ(m.begin() + 1, ret.first);
  EXPECT_EQ(2, ret.first->first);
  EXPECT_EQ(-2, ret.first->second);

  auto repeat = m.insert({2, -3});
  EXPECT_FALSE(repeat.second);
  EXPECT_EQ(ret.first, repeat.first);

  m.insert({3, -3});
  IntMap expected = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(expected, m);
}

TEST(LinearMapTest, InsertRange) {
  IntMap m;
  std::vector<std::pair<int, int>> data = {{1, -1}, {2, -2}, {3, -3}, {3, -4}};
  IntMap expected = {{1, -1}, {2, -2}, {3, -3}};

  m.insert(data.begin(), data.begin());
  EXPECT_TRUE(m.empty());

  m.insert(data.begin(), data.begin() + 2);
  EXPECT_EQ(2, m.size());

  m.insert(data.begin(), data.end());
  EXPECT_EQ(expected.size(), m.size());
  EXPECT_EQ(expected, m);
}

TEST(LinearMapTest, Emplace) {
  IntMap m;
  auto ret = m.emplace(1, -1);
  EXPECT_TRUE(ret.second);
  EXPECT_EQ(m.begin(), ret.first);
  EXPECT_EQ(1, ret.first->first);
  EXPECT_EQ(-1, ret.first->second);

  int k = 2;
  int v = -2;
  ret = m.emplace(k, v);
  EXPECT_TRUE(ret.second);
  EXPECT_EQ(m.begin() + 1, ret.first);
  EXPECT_EQ(2, ret.first->first);
  EXPECT_EQ(-2, ret.first->second);

  auto repeat = m.emplace(2, -3);
  EXPECT_FALSE(repeat.second);
  EXPECT_EQ(ret.first, repeat.first);

  m.emplace(3, -3);
  IntMap expected = {{1, -1}, {2, -2}, {3, -3}};
  EXPECT_EQ(expected, m);
}

}  // namespace test
}  // namespace scram
