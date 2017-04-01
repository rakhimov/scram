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

/// @file expression.cc
/// Implementation of the expression base class.

#include "expression.h"

#include "ext/algorithm.h"

namespace scram {
namespace mef {

Expression::Expression(std::vector<Expression*> args)
    : args_(std::move(args)),
      sampled_value_(0),
      sampled_(false) {}

double Expression::Sample() noexcept {
  if (!sampled_) {
    sampled_ = true;
    sampled_value_ = this->DoSample();
  }
  return sampled_value_;
}

void Expression::Reset() noexcept {
  if (!sampled_)
    return;
  sampled_ = false;
  for (Expression* arg : args_)
    arg->Reset();
}

bool Expression::IsDeviate() noexcept {
  return ext::any_of(args_, [](Expression* arg) { return arg->IsDeviate(); });
}

}  // namespace mef
}  // namespace scram
