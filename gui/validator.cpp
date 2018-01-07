/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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
/// Definition and initialization of validators.

#include "validator.h"

#include <limits>

#include <QDoubleValidator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace scram::gui {

const QValidator *Validator::name()
{
    static const QRegularExpressionValidator nameValidator(
        QRegularExpression(QStringLiteral(R"([[:alpha:]]\w*(-\w+)*)"),
                           QRegularExpression::UseUnicodePropertiesOption));
    return &nameValidator;
}

const QValidator *Validator::percent()
{
    static const QRegularExpressionValidator percentValidator(
        QRegularExpression(QStringLiteral(R"([1-9]\d*%?)")));
    return &percentValidator;
}

const QValidator *Validator::probability()
{
    static const QDoubleValidator probabilityValidator(0, 1, 1000);
    return &probabilityValidator;
}

const QValidator *Validator::nonNegative()
{
    static const QDoubleValidator nonNegativeValidator(
        0, std::numeric_limits<double>::max(), 1000);
    return &nonNegativeValidator;
}

} // namespace scram::gui
