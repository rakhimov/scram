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

#ifndef SCRAM_GUI_TEST_DATA_H
#define SCRAM_GUI_TEST_DATA_H

#include <initializer_list>

#include <QtTest>

namespace detail {

/// Tuple with implicit constructor for convenience sake.
///
/// @todo Not necessary in C++17.
template <typename... Ts>
struct tuple : public std::tuple<Ts...>
{
    template <typename... Us>
    constexpr tuple(Us &&... us) : std::tuple<Ts...>(std::forward<Us>(us)...)
    {
    }
};

/// Initializes columns in the test data table.
///
/// @tparam Ts  void terminated type list.
template <typename T, typename... Ts>
void initializeColumns(const char *const *it)
{
    QTest::addColumn<T>(*it);
    initializeColumns<Ts...>(++it);
}

/// Terminal case for test column initialization.
template <>
inline void initializeColumns<void>(const char *const *)
{
}

/// @tparam N  The number of columns to initialize in the row.
template <int N>
struct RowInitializer
{
    template <typename... Ts>
    void operator()(QTestData &data, const std::tuple<const char *, Ts...> &row)
    {
        RowInitializer<N - 1>{}(data, row);
        data << std::get<N>(row);
    }
};

/// Terminal case for row initialization.
template <>
struct RowInitializer<0>
{
    template <typename... Ts>
    void operator()(QTestData &, const std::tuple<const char *, Ts...> &)
    {
    }
};

} // namespace detail

/// Convenience function to populate QTest data table.
///
/// @tparam Ts  The column data types.
///
/// @param[in] columns  The column names.
/// @param[in] rows  The row name and data.
template <typename... Ts>
void populateData(
    const char *const (&columns)[sizeof...(Ts)],
    std::initializer_list<detail::tuple<const char *, Ts...>> rows)
{
    detail::initializeColumns<Ts..., void>(columns);

    for (const auto &row : rows)
        detail::RowInitializer<sizeof...(Ts)>{}(QTest::newRow(std::get<0>(row)),
                                                row);
}

#endif // SCRAM_GUI_TEST_DATA_H
