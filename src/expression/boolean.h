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
/// Boolean expressions.

#pragma once

#include <functional>

#include "src/expression.h"

namespace scram::mef {

using Not = NaryExpression<std::logical_not<>, 1>;  ///< Logical negation.
using And = NaryExpression<std::logical_and<>, -1>;  ///< Logical conjunction.
using Or = NaryExpression<std::logical_or<>, -1>;  ///< Logical disjunction.
using Eq = NaryExpression<std::equal_to<>, 2>;  ///< Equality test.
using Df = NaryExpression<std::not_equal_to<>, 2>;  ///< Inequality test.
using Lt = NaryExpression<std::less<>, 2>;  ///< (<) test.
using Gt = NaryExpression<std::greater<>, 2>;  ///< (>) test.
using Leq = NaryExpression<std::less_equal<>, 2>;  ///< (<=) test.
using Geq = NaryExpression<std::greater_equal<>, 2>;  ///< (>=) test.

}  // namespace scram::mef
