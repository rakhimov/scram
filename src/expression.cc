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
#include <string>

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
    SCRAM_THROW(ValidityError("Expression requires 2 or more arguments."))
        << errinfo_value(std::to_string(args.size()));
}

}  // namespace detail

namespace {  // Interval to string.

/// Converts an interval into a string.
std::string ToString(const Interval& interval) {
  std::stringstream ss;
  ss << interval;
  return ss.str();
}

}  // namespace

void EnsureProbability(Expression* expression, const char* type) {
  double value = expression->value();
  if (value < 0 || value > 1)
    SCRAM_THROW(DomainError("Invalid " + std::string(type) + " value"))
        << errinfo_value(std::to_string(value));

  Interval interval = expression->interval();
  if (IsProbability(interval) == false)
    SCRAM_THROW(DomainError("Invalid " + std::string(type) + " sample domain"))
        << errinfo_value(ToString(interval));
}

void EnsurePositive(Expression* expression, const char* description) {
  using namespace std::string_literals;  // NOLINT

  double value = expression->value();
  if (value <= 0)
    SCRAM_THROW(DomainError(description + " argument value must be positive."s))
        << errinfo_value(std::to_string(value));

  Interval interval = expression->interval();
  if (IsPositive(interval) == false)
    SCRAM_THROW(
        DomainError(description + " argument sample domain must be positive."s))
        << errinfo_value(ToString(interval));
}

void EnsureNonNegative(Expression* expression, const char* description) {
  using namespace std::string_literals;  // NOLINT

  double value = expression->value();
  if (value < 0)
    SCRAM_THROW(
        DomainError(description + " argument value cannot be negative."s))
        << errinfo_value(std::to_string(value));

  Interval interval = expression->interval();
  if (IsNonNegative(interval) == false)
    SCRAM_THROW(DomainError(description +
                            " argument sample cannot have negative values."s))
        << errinfo_value(ToString(interval));
}

void EnsureWithin(Expression* expression, const Interval& interval,
                  const char* type) {
  double arg_value = expression->value();
  if (!Contains(interval, arg_value)) {
    std::stringstream ss;
    ss << type << " argument value must be in " << interval << ".";
    SCRAM_THROW(DomainError(ss.str()))
        << errinfo_value(std::to_string(arg_value));
  }
  Interval arg_interval = expression->interval();
  if (!boost::icl::within(arg_interval, interval)) {
    std::stringstream ss;
    ss << type << " argument sample domain must be in " << interval << ".";
    SCRAM_THROW(DomainError(ss.str())) << errinfo_value(ToString(arg_interval));
  }
}

}  // namespace scram::mef
