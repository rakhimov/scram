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

#ifndef GUIASSERT_H
#define GUIASSERT_H

#include <QMessageBox>
#include <QString>

/// Assertion that avoids crashing the application.
/// To simulate the standard assert (i.e., crash on error),
/// define the environmental variable QT_FATAL_CRITICALS to non-empty value.
///
/// @param cond  The condition under test.
/// @param ret  The return value if assertion fails.
#define GUI_ASSERT(cond, ret)                                                  \
    do {                                                                       \
        if (cond)                                                              \
            break;                                                             \
        qCritical("Assertion failure: %s in %s line %d", #cond, __FILE__,      \
                  __LINE__);                                                   \
        QMessageBox::critical(                                                 \
            nullptr, QString::fromLatin1("Assertion Failure"),                 \
            QString::fromLatin1("%1 in %2 line %3")                            \
                .arg(QString::fromLatin1(#cond), QString::fromUtf8(__FILE__),  \
                     QString::number(__LINE__)));                              \
        return ret;                                                            \
    } while (false)

#endif // GUIASSERT_H
