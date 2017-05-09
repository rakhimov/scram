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

/// @file parameter.h
/// Parameter expressions that act like a shareable variable.

#ifndef SCRAM_SRC_PARAMETER_H_
#define SCRAM_SRC_PARAMETER_H_

#include <cstdint>

#include <memory>
#include <string>

#include "element.h"
#include "expression.h"

namespace scram {
namespace mef {

/// Provides units for parameters.
enum Units : std::uint8_t {
  kUnitless = 0,
  kBool,
  kInt,
  kFloat,
  kHours,
  kInverseHours,
  kYears,
  kInverseYears,
  kFit,
  kDemands
};

const int kNumUnits = 10;  ///< The number of elements in the Units enum.

/// String representations of the Units in the same order as the enum.
const char* const kUnitsToString[] = {"unitless", "bool",    "int",   "float",
                                      "hours",    "hours-1", "years", "years-1",
                                      "fit",      "demands"};

/// The special parameter for system mission time.
class MissionTime : public Expression {
 public:
  /// @param[in] time  The mission time.
  /// @param[in] unit  The unit of the given ``time`` argument.
  ///
  /// @throws LogicError  The time value is negative.
  explicit MissionTime(double time = 0, Units unit = kHours);

  /// @returns The unit of the system mission time.
  Units unit() const { return unit_; }

  /// Changes the mission time value.
  ///
  /// @param[in] time  The mission time in hours.
  ///
  /// @throws LogicError  The time value is negative.
  void value(double time);

  double value() noexcept override { return value_; }
  Interval interval() noexcept override { return Interval::closed(0, value_); }
  bool IsDeviate() noexcept override { return false; }

 private:
  double DoSample() noexcept override { return value_; }

  Units unit_;  ///< Units of this parameter.
  double value_;  ///< The universal value to represent int, bool, double.
};

/// This class provides a representation of a variable
/// in basic event description.
/// It is both expression and element description.
class Parameter : public Expression, public Id, public NodeMark, public Usage {
 public:
  using Id::Id;

  /// Sets the expression of this parameter.
  ///
  /// @param[in] expression  The expression to describe this parameter.
  ///
  /// @throws LogicError  The parameter expression is already set.
  void expression(Expression* expression);

  /// @returns The unit of this parameter.
  Units unit() const { return unit_; }

  /// Sets the unit of this parameter.
  ///
  /// @param[in] unit  A valid unit.
  void unit(Units unit) { unit_ = unit; }

  double value() noexcept override { return expression_->value(); }
  Interval interval() noexcept override { return expression_->interval(); }

 private:
  double DoSample() noexcept override { return expression_->Sample(); }

  Units unit_ = kUnitless;  ///< Units of this parameter.
  Expression* expression_ = nullptr;  ///< Expression for this parameter.
};

using ParameterPtr = std::shared_ptr<Parameter>;  ///< Shared parameters.

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_PARAMETER_H_
