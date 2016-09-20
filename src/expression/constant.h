/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

/// This is for the system mission time.
/// @todo Make this class derive from constant expression.
class MissionTime : public Expression {
 public:
  MissionTime();

  /// Sets the mission time.
  /// This function is expected to be used only once.
  ///
  /// @param[in] time  The mission time.
  ///
  /// @todo Move into constructor.
  void mission_time(double time) {
    assert(time >= 0);
    mission_time_ = time;
  }

  /// @returns The unit of the system mission time.
  Units unit() const { return unit_; }

  /// Sets the unit of this parameter.
  ///
  /// @param[in] unit  A valid unit.
  void unit(Units unit) { unit_ = unit; }

  double Mean() noexcept override { return mission_time_; }
  bool IsConstant() noexcept override { return true; }

 private:
  double GetSample() noexcept override { return mission_time_; }

  double mission_time_;  ///< The system mission time.
  Units unit_;  ///< Units of this parameter.
};

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
  bool IsConstant() noexcept override { return true; }

 private:
  double GetSample() noexcept override { return value_; }
  double value_;  ///< The Constant value.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_CONSTANT_H_
