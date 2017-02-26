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

/// @file expression.h
/// Provides the base class for all expressions
/// and units for expression values.

#ifndef SCRAM_SRC_EXPRESSION_H_
#define SCRAM_SRC_EXPRESSION_H_

#include <cstdint>

#include <memory>
#include <vector>

#include <boost/noncopyable.hpp>

#include "element.h"

namespace scram {
namespace mef {

class Expression;
using ExpressionPtr = std::shared_ptr<Expression>;  ///< Shared expressions.

class Initializer;  // Needs to handle cycles.

/// Abstract base class for all sorts of expressions to describe events.
/// This class also acts like a connector for parameter nodes
/// and may create cycles.
/// Expressions are not expected to be shared
/// except for parameters.
/// In addition, expressions are not expected to be changed
/// after validation phases.
class Expression : private boost::noncopyable {
 public:
  /// Provides access to cycle-destructive functions.
  class Cycle {
    friend class Initializer;  // Only Initializer needs the functionality.
    /// Breaks connections with expression arguments.
    ///
    /// @param[in,out] parameter  A parameter node in possible cycles.
    ///                           The type is not declared ``Parameter``
    ///                           because the inheritance is not
    ///                           forward-declarable.
    ///
    /// @post The parameter is in inconsistent, unusable state.
    ///       Only destruction is guaranteed to succeed.
    ///
    /// @todo Consider moving into Parameter class.
    static void BreakConnections(Expression* parameter) {
      parameter->args_.clear();
    }
  };

  /// Constructor for use by derived classes
  /// to register their arguments.
  ///
  /// @param[in] args  Arguments of this expression.
  explicit Expression(std::vector<ExpressionPtr> args);

  virtual ~Expression() = default;

  /// @returns A set of arguments of the expression.
  const std::vector<ExpressionPtr>& args() const { return args_; }

  /// Validates the expression.
  /// This late validation is due to parameters that are defined late.
  ///
  /// @throws InvalidArgument  The arguments are invalid for setup.
  virtual void Validate() const {}

  /// @returns The mean value of this expression.
  virtual double Mean() noexcept = 0;

  /// @returns A sampled value of this expression.
  double Sample() noexcept;

  /// This routine resets the sampling to get new values.
  /// All the arguments are called to reset themselves.
  /// If this expression was not sampled,
  /// its arguments are not going to get any calls.
  virtual void Reset() noexcept;

  /// Determines if the value of the expression contains deviate expressions.
  /// The default logic is to check arguments with uncertainties for sampling.
  /// Derived expression classes must decide
  /// if they don't have arguments,
  /// or if they are random deviates.
  ///
  /// @returns true if the expression's value deviates from its mean.
  /// @returns false if the expression's value not need sampling.
  ///
  /// @warning Improper registration of arguments
  ///          may yield silent failure.
  virtual bool IsDeviate() noexcept;

  /// @returns Maximum value of this expression.
  virtual double Max() noexcept { return this->Mean(); }

  /// @returns Minimum value of this expression.
  virtual double Min() noexcept { return this->Mean(); }

 protected:
  /// Registers an additional argument expression.
  ///
  /// @param[in] arg  An argument expression used by this expression.
  void AddArg(const ExpressionPtr& arg) { args_.push_back(arg); }

 private:
  /// Runs sampling of the expression.
  /// Derived concrete classes must provide the calculation.
  ///
  /// @returns A sampled value of this expression.
  virtual double DoSample() noexcept = 0;

  std::vector<ExpressionPtr> args_;  ///< Expression's arguments.
  double sampled_value_;  ///< The sampled value.
  bool sampled_;  ///< Indication if the expression is already sampled.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_H_
