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

/// @file ccf_group.cc
/// Implementation of various common cause failure models.

#include "ccf_group.h"

#include <cmath>

#include "error.h"
#include "expression/constant.h"
#include "expression/numerical.h"
#include "ext/algorithm.h"
#include "ext/combination_iterator.h"

namespace scram {
namespace mef {

CcfEvent::CcfEvent(std::string name, const CcfGroup* ccf_group)
    : BasicEvent(std::move(name), ccf_group->base_path(), ccf_group->role()),
      ccf_group_(*ccf_group) {}

void CcfGroup::AddMember(BasicEvent* basic_event) {
  if (distribution_ || factors_.empty() == false) {
    throw IllegalOperation("No more members accepted. The distribution for " +
                           Element::name() +
                           " CCF group has already been defined.");
  }
  if (ext::any_of(members_, [&basic_event](BasicEvent* member) {
        return member->name() == basic_event->name();
      })) {
    throw DuplicateArgumentError("Duplicate member " + basic_event->name() +
                                 " in " + Element::name() + " CCF group.");
  }
  members_.push_back(basic_event);
}

void CcfGroup::AddDistribution(Expression* distr) {
  if (distribution_)
    throw LogicError("CCF distribution is already defined.");
  if (members_.size() < 2) {
    throw ValidationError(Element::name() +
                          " CCF group must have at least 2 members.");
  }
  distribution_ = distr;
  // Define probabilities of all basic events.
  for (BasicEvent* member : members_)
    member->expression(distribution_);
}

void CcfGroup::AddFactor(Expression* factor, boost::optional<int> level) {
  int min_level = this->min_level();
  if (!level)
    level = prev_level_ ? (prev_level_ + 1) : min_level;

  if (*level <= 0 || members_.empty())
    throw LogicError("Invalid CCF group factor setup.");

  if (*level < min_level) {
    throw ValidationError("The CCF factor level (" + std::to_string(*level) +
                          ") is less than the minimum level (" +
                          std::to_string(min_level) + ") required by " +
                          Element::name() + " CCF group.");
  }
  if (members_.size() < *level) {
    throw ValidationError("The CCF factor level " + std::to_string(*level) +
                          " is more than the number of members (" +
                          std::to_string(members_.size()) + ") in " +
                          Element::name() + " CCF group.");
  }

  int index = *level - min_level;
  if (index < factors_.size() && factors_[index].second != nullptr) {
    throw RedefinitionError("Redefinition of CCF factor for level " +
                            std::to_string(*level) + " in " + Element::name() +
                            " CCF group.");
  }
  if (index >= factors_.size())
    factors_.resize(index + 1);

  factors_[index] = {*level, factor};
  prev_level_ = *level;
}

void CcfGroup::Validate() const {
  if (!distribution_ || members_.empty() || factors_.empty())
    throw LogicError("CCF group " + Element::name() + " is not initialized.");

  EnsureProbability<ValidationError>(
      distribution_, Element::name() + " CCF group distribution.");

  for (const std::pair<int, Expression*>& f : factors_) {
    if (!f.second) {
      throw ValidationError("Missing some CCF factors for " + Element::name() +
                            " CCF group.");
    }
    EnsureProbability<ValidationError>(
        f.second, Element::name() + " CCF group factors.", "fraction");
  }
  this->DoValidate();
}

namespace {

/// Joins CCF combination proxy gate names
/// to create a distinct name for a new CCF event.
///
/// @param[in] combination  The combination of events.
///
/// @returns A uniquely mangled string for the combination.
std::string JoinNames(const std::vector<Gate*>& combination) {
  std::string name = "[";
  for (auto it = combination.begin(), it_end = std::prev(combination.end());
       it != it_end; ++it) {
    name += (*it)->name() + " ";
  }
  name += combination.back()->name() + "]";
  return name;
}

}  // namespace

void CcfGroup::ApplyModel() {
  // Construct replacement proxy gates for member basic events.
  std::vector<Gate*> proxy_gates;
  for (BasicEvent* member : members_) {
    auto new_gate = std::make_unique<Gate>(member->name(), member->base_path(),
                                           member->role());
    assert(member->id() == new_gate->id());
    new_gate->formula(std::make_unique<Formula>(kOr));

    proxy_gates.push_back(new_gate.get());
    member->ccf_gate(std::move(new_gate));
  }

  ExpressionMap probabilities = this->CalculateProbabilities();
  assert(probabilities.size() > 1);

  for (auto& entry : probabilities) {
    int level = entry.first;
    Expression* prob = entry.second;
    for (auto combination :
         ext::make_combination_generator(level, proxy_gates.begin(),
                                         proxy_gates.end())) {
      auto ccf_event = std::make_unique<CcfEvent>(JoinNames(combination), this);
      ccf_event->expression(prob);
      for (Gate* gate : combination)
        gate->formula().AddArgument(ccf_event.get());
      ccf_event->members(std::move(combination));  // Move, at last.
      ccf_events_.emplace_back(std::move(ccf_event));
    }
  }
}

CcfGroup::ExpressionMap BetaFactorModel::CalculateProbabilities() {
  assert(CcfGroup::factors().size() == 1);
  assert(CcfGroup::members().size() == CcfGroup::factors().front().first);

  ExpressionMap probabilities;

  Expression* beta = CcfGroup::factors().begin()->second;
  probabilities.emplace_back(  // (1 - beta) * Q
      1, CcfGroup::Register<Mul>(
             {CcfGroup::Register<Sub>({&ConstantExpression::kOne, beta}),
              CcfGroup::distribution()}));

  probabilities.emplace_back(  // beta * Q
      CcfGroup::factors().front().first,
      CcfGroup::Register<Mul>({beta, CcfGroup::distribution()}));
  return probabilities;
}

namespace {

/// Helper function to calculate reciprocal of
/// nCk (n-choose-k) combination.
///
/// @param[in] n  The total number elements.
/// @param[in] k  Subset size.
///
/// @returns 1 / nCk
double CalculateCombinationReciprocal(int n, int k) {
  assert(n >= 0);
  assert(k >= 0);
  assert(n >= k);
  if (n - k > k)
    k = n - k;
  double result = 1;
  for (int i = 1; i <= n - k; ++i) {
    result *= static_cast<double>(i) / static_cast<double>(k + i);
  }
  return result;
}

}  // namespace

CcfGroup::ExpressionMap MglModel::CalculateProbabilities() {
  ExpressionMap probabilities;
  int max_level = CcfGroup::factors().back().first;
  assert(CcfGroup::factors().size() == max_level - 1);

  int num_members = CcfGroup::members().size();
  for (int i = 0; i < max_level; ++i) {
    double mult = CalculateCombinationReciprocal(num_members - 1, i);
    std::vector<Expression*> args;
    args.push_back(CcfGroup::Register<ConstantExpression>(mult));
    for (int j = 0; j < i; ++j) {
      args.push_back(CcfGroup::factors()[j].second);
    }
    if (i < max_level - 1) {
      args.push_back(CcfGroup::Register<Sub>(
          {&ConstantExpression::kOne, CcfGroup::factors()[i].second}));
    }
    args.push_back(CcfGroup::distribution());
    probabilities.emplace_back(i + 1, CcfGroup::Register<Mul>(std::move(args)));
  }
  assert(probabilities.size() == max_level);
  return probabilities;
}

CcfGroup::ExpressionMap AlphaFactorModel::CalculateProbabilities() {
  ExpressionMap probabilities;
  int max_level = CcfGroup::factors().back().first;
  assert(CcfGroup::factors().size() == max_level);
  std::vector<Expression*> sum_args;
  for (const std::pair<int, Expression*>& factor : CcfGroup::factors()) {
    sum_args.emplace_back(CcfGroup::Register<Mul>(
        {CcfGroup::Register<ConstantExpression>(factor.first), factor.second}));
  }
  Expression* sum = CcfGroup::Register<Add>(std::move(sum_args));
  int num_members = CcfGroup::members().size();

  for (int i = 0; i < max_level; ++i) {
    double mult = CalculateCombinationReciprocal(num_members - 1, i);
    Expression* level = CcfGroup::Register<ConstantExpression>(i + 1);
    Expression* fraction =
        CcfGroup::Register<Div>({CcfGroup::factors()[i].second, sum});
    Expression* prob = CcfGroup::Register<Mul>(
        {level, CcfGroup::Register<ConstantExpression>(mult), fraction,
         CcfGroup::distribution()});
    probabilities.emplace_back(i + 1, prob);
  }
  assert(probabilities.size() == max_level);
  return probabilities;
}

void PhiFactorModel::DoValidate() const {
  double sum = 0;
  double sum_min = 0;
  double sum_max = 0;
  for (const std::pair<int, Expression*>& factor : CcfGroup::factors()) {
    sum += factor.second->value();
    Interval interval = factor.second->interval();
    sum_min += interval.lower();
    sum_max += interval.upper();
  }
  /// @todo Problems with floating point number comparison.
  double epsilon = 1e-4;
  double diff = std::abs(sum - 1);
  double diff_min = std::abs(sum_min - 1);
  double diff_max = std::abs(sum_max - 1);
  if (diff_min > epsilon || diff > epsilon || diff_max > epsilon) {
    throw ValidationError("The factors for Phi model " + CcfGroup::name() +
                          " CCF group must sum to 1.");
  }
}

CcfGroup::ExpressionMap PhiFactorModel::CalculateProbabilities() {
  ExpressionMap probabilities;
  int max_level = CcfGroup::factors().back().first;
  for (const std::pair<int, Expression*>& factor : CcfGroup::factors()) {
    Expression* prob =
        CcfGroup::Register<Mul>({factor.second, CcfGroup::distribution()});
    probabilities.emplace_back(factor.first, prob);
  }
  assert(probabilities.size() == max_level);
  return probabilities;
}

}  // namespace mef
}  // namespace scram
