/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file constant.cc
/// Implementation of various constant expressions.

#include "constant.h"

#include <boost/math/constants/constants.hpp>

namespace scram {
namespace mef {

ConstantExpression ConstantExpression::kOne(1);
ConstantExpression ConstantExpression::kZero(0);
ConstantExpression ConstantExpression::kPi(
    boost::math::constants::pi<double>());

}  // namespace mef
}  // namespace scram
