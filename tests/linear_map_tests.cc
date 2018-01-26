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

#include "ext/linear_map.h"

#include <string>
#include <type_traits>

#include <boost/container/vector.hpp>

#include <catch.hpp>

// Explicit instantiations with some common types.
template class ext::linear_map<int, int>;
template class ext::linear_map<int, double>;
template class ext::linear_map<int, std::string>;
template class ext::linear_map<std::string, std::string>;

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
template class ext::linear_map<KeyClass, std::string>;

// Move erase policy instantiation.
template class ext::linear_map<KeyClass, std::string, ext::MoveEraser>;

#ifndef __INTEL_COMPILER
// Passing another underlying container types.
template class ext::linear_map<int, int, ext::DefaultEraser,
                               boost::container::vector>;
template class ext::linear_map<KeyClass, std::string, ext::MoveEraser,
                               boost::container::vector>;
#endif

namespace std {

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& out, const std::pair<T1, T2>& value) {
  out << "{" << Catch::StringMaker<T1>::convert(value.first) << ", "
      << Catch::StringMaker<T2>::convert(value.second) << "}";
  return out;
}

}  // namespace std

namespace scram::test {

using IntMap = ext::linear_map<int, int>;

static_assert(std::is_move_constructible_v<IntMap>);
static_assert(std::is_move_assignable_v<IntMap>);
static_assert(std::is_copy_constructible_v<IntMap>);
static_assert(std::is_copy_assignable_v<IntMap>);

TEST_CASE("linear map ctors", "[linear_map]") {
  SECTION("default ctor") {
    IntMap m_default;
    CHECK(m_default.size() == 0);
    CHECK(m_default.empty());
  }

  IntMap m_init_list = {{1, -1}, {2, -2}, {3, -3}};
  SECTION("initializer list") {
    CHECK(m_init_list.size() == 3);
    CHECK_FALSE(m_init_list.empty());
  }

  SECTION("copy ctor") {
    auto m_copy(m_init_list);
    CHECK(m_copy.size() == 3);
    CHECK_FALSE(m_copy.empty());
    CHECK(m_copy == m_init_list);

    SECTION("copy itself") {
      m_copy = m_copy;
      CHECK(m_copy.size() == 3);
      CHECK(m_copy == m_init_list);
    }
  }

  SECTION("move ctor") {
    auto construct_extra_copy(m_init_list);
    auto m_move(std::move(construct_extra_copy));
    CHECK(construct_extra_copy.empty());
    CHECK(m_move.size() == 3);
    CHECK_FALSE(m_move.empty());
    CHECK(m_move == m_init_list);
  }

  SECTION("copy assignment") {
    IntMap m_assign_copy;
    m_assign_copy = m_init_list;
    CHECK(m_assign_copy == m_init_list);
  }

  SECTION("move assignment") {
    IntMap m_assign_move;
    auto assign_extra_copy(m_init_list);
    m_assign_move = std::move(assign_extra_copy);
    CHECK(assign_extra_copy.empty());
    CHECK(m_assign_move == m_init_list);
  }

  SECTION("range ctor") {
    IntMap::container_type m_data = {{1, -1}, {2, -2}, {3, -3}, {3, -4}};
    IntMap m_range(m_data.begin(), m_data.end());
    CHECK(m_range == m_init_list);
  }

  SECTION("repeat") {
    IntMap m_repeat_init{{1, -1}, {2, -2}, {3, -3}, {3, -4}};
    CHECK(m_repeat_init == m_init_list);
  }
}

TEST_CASE("linear map equality", "[linear_map]") {
  IntMap m1;
  SECTION("compare to itself") { CHECK(m1 == m1); }

  IntMap m2;
  SECTION("compare to new") { CHECK(m2 == m1); }

  m1 = {{1, -1}, {2, -2}, {3, -3}};
  SECTION("with equal values") {
    CHECK(m1 == m1);
    CHECK_FALSE(m2 == m1);
    SECTION("copy") {
      m2 = m1;
      CHECK(m2 == m1);
    }
    SECTION("new with the same values") {
      m2 = {{1, -1}, {2, -2}, {3, -3}};
      CHECK(m2 == m1);
    }
    SECTION("new with different order") {
      m2 = {{2, -2}, {1, -1}, {3, -3}};
      CHECK(m2 == m1);
    }
  }
  SECTION("with unequal values") {
    SECTION("one less") {
      m2 = {{1, -1}, {2, -2}};
      CHECK_FALSE(m2 == m1);
    }
    SECTION("same keys but different values") {
      m2 = {{1, 1}, {2, 2}, {3, 3}};
      CHECK_FALSE(m2 == m1);
    }
  }
}

TEST_CASE("linear map iterators", "[linear_map]") {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};
  IntMap::container_type c = {{1, -1}, {2, -2}, {3, -3}};
  CHECK(std::next(m.begin(), m.size()) == m.end());
  CHECK(std::next(m.rbegin(), m.size()) == m.rend());
  CHECK(std::next(m.cbegin(), m.size()) == m.cend());
  CHECK(std::next(m.crbegin(), m.size()) == m.crend());

  const auto& m_ref = m;
  CHECK(std::next(m_ref.begin(), m_ref.size()) == m_ref.end());
  CHECK(std::next(m_ref.rbegin(), m_ref.size()) == m_ref.rend());
  CHECK(std::next(m_ref.cbegin(), m_ref.size()) == m_ref.cend());
  CHECK(std::next(m_ref.crbegin(), m_ref.size()) == m_ref.crend());

  auto it_c = c.begin();
  int num_elements = 0;
  int key_sum = 0;
  int value_sum = 0;
  for (const auto& entry : m) {
    INFO(std::distance(c.begin(), it_c));
    CHECK(*it_c++ == entry);
    ++num_elements;
    key_sum += entry.first;
    value_sum += entry.second;
  }
  CHECK(num_elements == 3);
  CHECK(key_sum == 6);
  CHECK(value_sum == -6);
  CHECK(m.data() == c);
}

SCENARIO("linear map clear", "[linear_map]") {
  GIVEN("empty linear map") {
    IntMap m;
    REQUIRE(m.empty());
    REQUIRE(m.capacity() >= m.size());
    auto init_capacity = m.capacity();

    WHEN("clear") {
      m.clear();
      THEN("no change") {
        CHECK(m.empty());
        CHECK(m.capacity() == init_capacity);
      }
    }
  }

  GIVEN("non empty linear map") {
    IntMap m = {{1, -1}, {2, -2}, {3, -3}};
    CHECK_FALSE(m.empty());
    REQUIRE(m.capacity() >= m.size());
    auto init_capacity = m.capacity();

    WHEN("clear") {
      m.clear();
      THEN("the map is empty but capacity is not") {
        CHECK(m.empty());
        CHECK(m.capacity() == init_capacity);
      }
    }
  }
}

SCENARIO("linear map capacity reserve", "[linear_map]") {
  GIVEN("A linear map with some items") {
    IntMap m = {{1, -1}, {2, -2}, {3, -3}};

    REQUIRE(m.size() == 3);
    REQUIRE(m.capacity() >= 3);

    WHEN("the capacity is increased") {
      m.reserve(10);

      THEN("the capacity change but not size") {
        REQUIRE(m.size() == 3);
        REQUIRE(m.capacity() >= 10);
      }
    }
    WHEN("the capacity is reduced") {
      m.reserve(0);

      THEN("the size and capacity do not change") {
        REQUIRE(m.size() == 3);
        REQUIRE(m.capacity() >= 3);
      }
    }
  }
}

TEST_CASE("linear map swap", "[linear_map]") {
  const IntMap m1 = {{1, -1}, {2, -2}, {3, -3}};
  const IntMap m2 = {{4, -4}, {5, -5}};
  IntMap ms1 = m1;
  IntMap ms2 = m2;

  SECTION("member swap") {
    ms1.swap(ms2);
    CHECK(ms2 == m1);
    CHECK(ms1 == m2);
  }

  SECTION("ADL swap") {
    swap(ms1, ms2);
    CHECK(ms2 == m1);
    CHECK(ms1 == m2);
  }

  SECTION("STD swap") {
    std::swap(ms1, ms2);
    CHECK(ms2 == m1);
    CHECK(ms1 == m2);
  }
}

TEST_CASE("linear map default erase", "[linear_map]") {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};
  IntMap m_expected = {{2, -2}, {3, -3}};

  SECTION("erase w/ key") {
    m.erase(1);
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }

  SECTION("erase w/ iterator") {
    m.erase(m.begin());
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }

  SECTION("erase w/ const iterator") {
    m.erase(m.cbegin());
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }
}

TEST_CASE("linear map move erase", "[linear_map]") {
  using MoveMap = ext::linear_map<int, int, ext::MoveEraser>;
  MoveMap m = {{1, -1}, {2, -2}, {3, -3}};
  MoveMap m_expected = {{3, -3}, {2, -2}};

  SECTION("erase w/ key") {
    m.erase(1);
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }

  SECTION("erase w/ iterator") {
    m.erase(m.begin());
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }

  SECTION("erase w/ const iterator") {
    m.erase(m.cbegin());
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }
}

TEST_CASE("linear map find", "[linear_map]") {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};

  CHECK(m.count(1) == 1);
  CHECK(m.count(5) == 0);

  CHECK(m.find(1) == m.begin());
  CHECK(m.find(1)->first == 1);
  int key = 2;
  CHECK(m.find(key) == std::next(m.begin(), 1));
  CHECK(m.find(key)->first == key);
  CHECK(m.find(3) == std::next(m.begin(), 2));
  CHECK(m.find(5) == m.end());
}

TEST_CASE("linear map operator index", "[linear_map]") {
  IntMap m;
  m[1] = -1;
  int k = 2;
  m[k] = -2;
  m[3] = -3;
  IntMap expected = {{1, -1}, {2, -2}, {3, -3}};
  REQUIRE(m == expected);

  m[3] = -4;
  IntMap changed = {{1, -1}, {2, -2}, {3, -4}};
  REQUIRE(m == changed);
}

TEST_CASE("linear map at", "[linear_map]") {
  IntMap m = {{1, -1}, {2, -2}, {3, -3}};
  CHECK(m.at(1) == -1);
  CHECK_THROWS_AS(m.at(5), std::out_of_range);

  const auto& m_ref = m;
  CHECK(m_ref.at(2) == -2);

  m.at(2) = -4;
  IntMap m_expected = {{1, -1}, {2, -4}, {3, -3}};
  CHECK(m == m_expected);
}

TEST_CASE("linear map insert single", "[linear_map]") {
  IntMap m;
  auto ret = m.insert({1, -1});
  CHECK(ret.second);
  REQUIRE_FALSE(ret.first == m.end());
  CHECK(ret.first == m.begin());
  CHECK(ret.first->first == 1);
  CHECK(ret.first->second == -1);

  IntMap::value_type v = {2, -2};
  ret = m.insert(v);
  CHECK(ret.second);
  CHECK(ret.first == std::next(m.begin(), 1));
  CHECK(ret.first->first == 2);
  CHECK(ret.first->second == -2);

  auto repeat = m.insert({2, -3});
  CHECK_FALSE(repeat.second);
  CHECK(ret.first == repeat.first);

  m.insert({3, -3});
  IntMap expected = {{1, -1}, {2, -2}, {3, -3}};
  CHECK(m == expected);
}

TEST_CASE("linear map insert range", "[linear_map]") {
  IntMap m;
  std::vector<std::pair<int, int>> data = {{1, -1}, {2, -2}, {3, -3}, {3, -4}};
  IntMap expected = {{1, -1}, {2, -2}, {3, -3}};

  SECTION("equal iterators") {
    m.insert(data.begin(), data.begin());
    CHECK(m.empty());
  }

  SECTION("shift two") {
    m.insert(data.begin(), data.begin() + 2);
    CHECK(m.size() == 2);
  }

  SECTION("begin to end") {
    m.insert(data.begin(), data.end());
    CHECK(m.size() == expected.size());
    CHECK(m == expected);
  }
}

TEST_CASE("linear map emplace", "[linear_map]") {
  IntMap m;
  auto ret = m.emplace(1, -1);
  CHECK(ret.second);
  CHECK(ret.first == m.begin());
  CHECK(ret.first->first == 1);
  CHECK(ret.first->second == -1);

  int k = 2;
  int v = -2;
  ret = m.emplace(k, v);
  CHECK(ret.second);
  CHECK(ret.first == std::next(m.begin()));
  CHECK(ret.first->first == 2);
  CHECK(ret.first->second == -2);

  auto repeat = m.emplace(2, -3);
  CHECK_FALSE(repeat.second);
  CHECK(repeat.first == ret.first);

  m.emplace(3, -3);
  IntMap expected = {{1, -1}, {2, -2}, {3, -3}};
  CHECK(m == expected);
}

}  // namespace scram::test
