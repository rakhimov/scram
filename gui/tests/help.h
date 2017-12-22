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

#ifndef SCRAM_GUI_TEST_HELP_H
#define SCRAM_GUI_TEST_HELP_H

#include <string>

#include <QtTest>

namespace QTest {

/// Specialization to print standard strings.
///
/// @warning This is a C string API in Qt C++ code!!!
///          The caller must deallocate the returned string.
///          This is only specialized for QCOMPARE to print better messages.
template <>
inline char *toString(const std::string &value)
{
    return qstrdup(value.c_str());
}

} // namespace QTest

/// Helper macro to workaround the limitation of QCOMPARE,
/// not being able to deal with "comparable" values of different types.
#define TEST_EQ(actual, expected) QCOMPARE(actual, decltype(actual)(expected))

#endif // SCRAM_GUI_TEST_HELP_H
