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

/// @file constant.h
/// Constant expressions that cannot have uncertainties.

#ifndef SCRAM_SRC_EXPRESSION_CONSTANT_H_
#define SCRAM_SRC_EXPRESSION_CONSTANT_H_

#include "src/expression.h"

namespace scram {
namespace mef {

/// Indicates a constant value.
class ConstantExpression : public Expression {
 public:
  static const ExpressionPtr kOne;  ///< Constant 1 or True.
  static const ExpressionPtr kZero;  ///< Constant 0 or False.
  static const ExpressionPtr kPi;  ///< Constant PI value.

  /// Constructor for numerical values.
  ///
  /// @param[in] value  Float numerical value.
  explicit ConstantExpression(double value);

  /// Constructor for numerical values.
  ///
  /// @param[in] value  Integer numerical value.
  explicit ConstantExpression(int value);

  /// Constructor for boolean values.
  ///
  /// @param[in] value  true for 1 and false for 0 value of this constant.
  explicit ConstantExpression(bool value);

  double Mean() noexcept override { return value_; }
  bool IsDeviate() noexcept override { return false; }

 private:
  double DoSample() noexcept override { return value_; }

  const double value_;  ///< The universal value to represent int, bool, double.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_CONSTANT_H_
