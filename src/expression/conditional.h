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

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_CONDITIONAL_H_
