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
/// Collection of validators.

#pragma once

#include <QValidator>

namespace scram::gui {

/// Provider of common validators.
class Validator
{
public:
    /// @returns The validator suitable for MEF element names.
    static const QValidator *name();

    /// @returns The validator for integer percent with '%' optional.
    static const QValidator *percent();

    /// @returns The floating point probability value validator.
    static const QValidator *probability();

    /// @returns The validator for non-negative floating-point numbers.
    static const QValidator *nonNegative();
};

} // namespace scram::gui
