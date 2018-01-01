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

/// @file
/// Helper workarounds or additions to Qt Test.

#ifndef SCRAM_GUI_TEST_HELP_H
#define SCRAM_GUI_TEST_HELP_H

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <QObject>
#include <QtTest>

namespace QTest {

/// Specialization to print standard strings.
///
/// @warning This is a C string API in Qt C++ code!!!
///          The caller must deallocate the returned string.
///          This is only specialized for QCOMPARE to print better messages.
/// @{
template <>
inline char *toString(const std::string &value)
{
    return qstrdup(value.c_str());
}

template <>
inline char *toString(const std::vector<std::string> &value)
{
    std::string result = "{";
    for (const auto &item : value) {
        result += item;
        result += ", ";
    }
    result += " }";
    return toString(result);
}
/// @}

} // namespace QTest

/// Helper macro to workaround the limitation of QCOMPARE,
/// not being able to deal with "comparable" values of different types.
#define TEST_EQ(actual, expected)                                              \
    QCOMPARE(actual, std::decay_t<decltype(actual)>(expected))

namespace ext {

/// Signal spy that preserves the concrete types of signal arguments.
/// The interface is similar to QSignalSpy,
/// but instead of a list of QVariant, a tuple of argument values are returned.
///
/// @tparam Ts  The signal parameter types.
///             Must be copy constructible.
template <typename... Ts>
class SignalSpy : public QObject,
                  public std::vector<std::tuple<std::decay_t<Ts>...>>
{
public:
    /// @tparam T  The type deriving from QObject.
    /// @tparam U  The base type of T or itself.
    ///
    /// @param[in] sender  The sender object to be spied on.
    /// @param[in] sig  The member signal of the object.
    template <class T, class U>
    SignalSpy(const T *sender, void (U::*sig)(Ts...))
    {
        connect(sender, sig, this, &SignalSpy::accept, Qt::DirectConnection);
    }

    SignalSpy(SignalSpy &&); ///< Only to enable RVO. Undefined.

private:
    /// Stores the signal arguments.
    void accept(Ts... args) { this->emplace_back(args...); }
};

/// Convenience function to construct a SignalSpy with deduced types.
template <class T, class U, typename... Ts>
auto make_spy(const T *sender, void (U::*sig)(Ts...))
{
    return SignalSpy<Ts...>(sender, sig);
}

} // namespace ext

#endif // SCRAM_GUI_TEST_HELP_H
