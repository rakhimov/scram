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

/// @file conditional.h
/// Conditional (if-then-else, switch-case) expressions.

#ifndef SCRAM_SRC_EXPRESSION_CONDITIONAL_H_
#define SCRAM_SRC_EXPRESSION_CONDITIONAL_H_

#include <vector>

#include "src/expression.h"

namespace scram {
namespace mef {

/// If-Then-Else ternary expression.
class Ite : public ExpressionFormula<Ite> {
 public:
  /// @param[in] condition  The Boolean expression to be tested.
  /// @param[in] then_arm  The expression if the Boolean is true.
  /// @param[in] else_arm  The expression if the Boolean is false.
  Ite(Expression* condition, Expression* then_arm, Expression* else_arm)
      : ExpressionFormula<Ite>({condition, then_arm, else_arm}) {}

  Interval interval() noexcept override;

  /// Computes the if-then-else expression with the given evaluator.
  template <typename F>
  double Compute(F&& eval) noexcept {
    assert(args().size() == 3);
    return eval(args()[0]) ? eval(args()[1]) : eval(args()[2]);
  }
};

/// Switch-Case conditional operations.
class Switch : public ExpressionFormula<Switch> {
 public:
  /// Individual cases in the switch-case operation.
  struct Case {
    Expression& condition;  ///< The case condition.
    Expression& value;  ///< The value to evaluated if the condition is true.
  };

  /// @param[in] cases  The collection of cases to evaluate.
  /// @param[in] default_value  The default value if all cases are false.
  Switch(std::vector<Case> cases, Expression* default_value);

  Interval interval() noexcept override;

  /// Computes the switch-case expression with the given evaluator.
  template <typename F>
  double Compute(F&& eval) noexcept {
    for (Case& case_arm : cases_) {
      if (eval(&case_arm.condition))
        return eval(&case_arm.value);
    }
    return eval(&default_value_);
  }

 private:
  std::vector<Case> cases_;  ///< Ordered collection of cases.
  Expression& default_value_;  ///< The default case value.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_CONDITIONAL_H_
