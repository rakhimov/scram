/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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
/// Implementation of the expression base class.

#include "expression.h"

#include <sstream>

#include "error.h"
#include "ext/algorithm.h"

namespace scram::mef {

Expression::Expression(std::vector<Expression*> args)
    : args_(std::move(args)), sampled_value_(0), sampled_(false) {}

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

namespace detail {

void EnsureMultivariateArgs(std::vector<Expression*> args) {
  if (args.size() < 2)
    SCRAM_THROW(ValidityError("Expression requires 2 or more arguments."));
}

}  // namespace detail

void EnsureProbability(Expression* expression, const std::string& description,
                       const char* type) {
  double value = expression->value();
  if (value < 0 || value > 1)
    SCRAM_THROW(DomainError("Invalid " + std::string(type) + " value for " +
                            description));

  if (IsProbability(expression->interval()) == false)
    SCRAM_THROW(DomainError("Invalid " + std::string(type) +
                            " sample domain for " + description));
}

void EnsurePositive(Expression* expression, const std::string& description) {
  if (expression->value() <= 0)
    SCRAM_THROW(DomainError(description + " argument value must be positive."));
  if (IsPositive(expression->interval()) == false)
    SCRAM_THROW(
        DomainError(description + " argument sample domain must be positive."));
}

void EnsureNonNegative(Expression* expression, const std::string& description) {
  if (expression->value() < 0)
    SCRAM_THROW(
        DomainError(description + " argument value cannot be negative."));
  if (IsNonNegative(expression->interval()) == false)
    SCRAM_THROW(DomainError(description +
                            " argument sample cannot have negative values."));
}

void EnsureWithin(Expression* expression, const Interval& interval,
                  const char* type) {
  double arg_value = expression->value();
  if (!Contains(interval, arg_value)) {
    std::stringstream ss;
    ss << type << " argument value [" << arg_value << "] must be in "
       << interval << ".";
    SCRAM_THROW(DomainError(ss.str()));
  }
  Interval arg_interval = expression->interval();
  if (!boost::icl::within(arg_interval, interval)) {
    std::stringstream ss;
    ss << type << " argument sample domain " << arg_interval << " must be in "
       << interval << ".";
    SCRAM_THROW(DomainError(ss.str()));
  }
}

}  // namespace scram::mef
