/*
 * Copyright (C) 2018 Olzhas Rakhimov
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

#include "ext/linear_set.h"

#include <string>
#include <type_traits>

#include <boost/container/vector.hpp>

#include <catch.hpp>

// Explicit instantiations with some common types.
template class ext::linear_set<int>;
template class ext::linear_set<double>;
template class ext::linear_set<std::string>;

namespace {

// Dummy implicit bool conversion based extractor.
struct KeyExtractor {
  template <typename T>
  bool operator()(const T& arg) {
    return arg;
  }
};

// The bare minimum class to be a value type for the linear set.
struct ValueClass {
  friend bool operator==(const ValueClass& lhs, const ValueClass& rhs) {
    return lhs.a == rhs.a && lhs.b == rhs.b;
  }

  int a;
  std::string b;
};

struct KeyByA {
  int operator()(const ValueClass& v) const { return v.a; }
};

}  // namespace

template class ext::linear_set<double, KeyExtractor>;
template class ext::linear_set<int, KeyExtractor>;
template class ext::linear_set<bool, KeyExtractor>;
template class ext::linear_set<ValueClass>;
template class ext::linear_set<ValueClass, KeyByA>;

#ifndef __INTEL_COMPILER
// Passing another underlying container types.
template class ext::linear_set<int, ext::identity, boost::container::vector>;
template class ext::linear_set<ValueClass, ext::identity,
                               boost::container::vector>;
#endif

namespace scram::test {

using IntSet = ext::linear_set<int>;

static_assert(std::is_move_constructible_v<IntSet>);
static_assert(std::is_move_assignable_v<IntSet>);
static_assert(std::is_copy_constructible_v<IntSet>);
static_assert(std::is_copy_assignable_v<IntSet>);

TEST_CASE("linear set ctors", "[linear_set]") {
  SECTION("default ctor") {
    IntSet m_default;
    CHECK(m_default.size() == 0);
    CHECK(m_default.empty());
  }

  IntSet m_init_list = {1, 2, 3};
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
    IntSet m_assign_copy;
    m_assign_copy = m_init_list;
    CHECK(m_assign_copy == m_init_list);
  }

  SECTION("move assignment") {
    IntSet m_assign_move;
    auto assign_extra_copy(m_init_list);
    m_assign_move = std::move(assign_extra_copy);
    CHECK(assign_extra_copy.empty());
    CHECK(m_assign_move == m_init_list);
  }

  SECTION("range ctor") {
    IntSet::container_type m_data = {1, 2, 3, 3};
    IntSet m_range(m_data.begin(), m_data.end());
    CHECK(m_range == m_init_list);
  }

  SECTION("repeat") {
    IntSet m_repeat_init{1, 2, 2, 3, 3, 3};
    CHECK(m_repeat_init == m_init_list);
  }
}

TEST_CASE("linear set equality", "[linear_set]") {
  IntSet m1;
  SECTION("compare to itself") { CHECK(m1 == m1); }

  IntSet m2;
  SECTION("compare to new") { CHECK(m2 == m1); }

  m1 = {1, 2, 3};
  SECTION("with equal values") {
    CHECK(m1 == m1);
    CHECK_FALSE(m2 == m1);
    SECTION("copy") {
      m2 = m1;
      CHECK(m2 == m1);
    }
    SECTION("new with the same values") {
      m2 = {1, 2, 3};
      CHECK(m2 == m1);
    }
    SECTION("new with different order") {
      m2 = {2, 1, 3};
      CHECK(m2 == m1);
    }
  }
  SECTION("with unequal values") {
    SECTION("one less") {
      m2 = {1, 2};
      CHECK_FALSE(m2 == m1);
    }
    SECTION("value diff") {
      m2 = {1, 2, 4};
      CHECK_FALSE(m2 == m1);
    }
    SECTION("one more") {
      m2 = {1, 2, 3, 4};
      CHECK_FALSE(m2 == m1);
    }
  }
}

TEST_CASE("linear set iterators", "[linear_set]") {
  IntSet m = {1, 2, 3};
  IntSet::container_type c = {1, 2, 3};
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
  int value_sum = 0;
  for (const auto& entry : m) {
    INFO(std::distance(c.begin(), it_c));
    CHECK(*it_c++ == entry);
    ++num_elements;
    value_sum += entry;
  }
  CHECK(num_elements == 3);
  CHECK(value_sum == 6);
  CHECK(m.data() == c);
}

SCENARIO("linear set clear", "[linear_set]") {
  GIVEN("empty linear set") {
    IntSet m;
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

  GIVEN("non empty linear set") {
    IntSet m = {1, 2, 3};
    CHECK_FALSE(m.empty());
    REQUIRE(m.capacity() >= m.size());
    auto init_capacity = m.capacity();

    WHEN("clear") {
      m.clear();
      THEN("the set is empty but capacity is not") {
        CHECK(m.empty());
        CHECK(m.capacity() == init_capacity);
      }
    }
  }
}

SCENARIO("linear set capacity reserve", "[linear_set]") {
  GIVEN("A linear set with some items") {
    IntSet m = {1, 2, 3};

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

TEST_CASE("linear set swap", "[linear_set]") {
  const IntSet m1 = {1, 2, 3};
  const IntSet m2 = {4, 5};
  IntSet ms1 = m1;
  IntSet ms2 = m2;

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

TEST_CASE("linear set default erase", "[linear_set]") {
  IntSet m = {1, 2, 3};
  IntSet m_expected = {2, 3};

  SECTION("erase w/ key") {
    int result = m.erase(1);
    CHECK(result == 1);
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }

  SECTION("erase w/ iterator") {
    auto it = m.erase(m.begin());
    CHECK(it == m.begin());
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }

  SECTION("erase w/ const iterator") {
    auto it = m.erase(m.cbegin());
    CHECK(it == m.cbegin());
    CHECK(m == m_expected);
    CHECK(m.data() == m_expected.data());
  }
}

TEST_CASE("linear set find", "[linear_set]") {
  IntSet m = {1, 2, 3};

  CHECK(m.count(1) == 1);
  CHECK(m.count(5) == 0);

  CHECK(m.find(1) == m.begin());
  CHECK(*m.find(1) == 1);
  int key = 2;
  CHECK(m.find(key) == std::next(m.begin(), 1));
  CHECK(*m.find(key) == key);
  CHECK(m.find(3) == std::next(m.begin(), 2));
  CHECK(m.find(5) == m.end());
}

TEST_CASE("linear set insert single", "[linear_set]") {
  IntSet m;
  auto ret = m.insert(1);
  CHECK(ret.second);
  REQUIRE_FALSE(ret.first == m.end());
  CHECK(ret.first == m.begin());
  CHECK(*ret.first == 1);

  IntSet::value_type v = 2;
  ret = m.insert(v);
  CHECK(ret.second);
  CHECK(ret.first == std::next(m.begin(), 1));
  CHECK(*ret.first == 2);

  auto repeat = m.insert(2);
  CHECK_FALSE(repeat.second);
  CHECK(ret.first == repeat.first);

  m.insert(3);
  IntSet expected = {1, 2, 3};
  CHECK(m == expected);
}

TEST_CASE("linear set insert range", "[linear_set]") {
  IntSet m;
  std::vector<int> data = {1, 2, 3, 3};
  IntSet expected = {1, 2, 3};

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

TEST_CASE("linear set emplace", "[linear_set]") {
  using PairSet = ext::linear_set<std::pair<int, int>>;
  PairSet m;
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
  CHECK(repeat.second);
  CHECK(repeat.first != ret.first);
  CHECK(repeat.first == std::prev(m.end()));
  CHECK(repeat.first->first == 2);
  CHECK(repeat.first->second == -3);

  m.emplace(3, -3);
  PairSet expected = {{1, -1}, {2, -2}, {2, -3}, {3, -3}};
  CHECK(m == expected);
}

}  // namespace scram::test
