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

#include "expression/arithmetic.h"
#include "expression/constant.h"
#include "ext/algorithm.h"
#include "ext/combination_iterator.h"

namespace scram {
namespace mef {

CcfEvent::CcfEvent(std::string name, const CcfGroup* ccf_group)
    : BasicEvent(std::move(name), ccf_group->base_path(), ccf_group->role()),
      ccf_group_(*ccf_group) {}

void CcfGroup::AddMember(const BasicEventPtr& basic_event) {
  if (distribution_ || factors_.empty() == false) {
    throw IllegalOperation("No more members accepted. The distribution for " +
                           Element::name() +
                           " CCF group has already been defined.");
  }
  if (ext::any_of(members_, [&basic_event](const BasicEventPtr& member) {
        return member->name() == basic_event->name();
      })) {
    throw DuplicateArgumentError("Duplicate member " + basic_event->name() +
                                 " in " + Element::name() + " CCF group.");
  }
  members_.push_back(basic_event);
}

void CcfGroup::AddDistribution(const ExpressionPtr& distr) {
  if (distribution_)
    throw LogicError("CCF distribution is already defined.");
  distribution_ = distr;
  // Define probabilities of all basic events.
  for (const BasicEventPtr& member : members_)
    member->expression(distribution_);
}

void CcfGroup::AddFactor(const ExpressionPtr& factor, int level) {
  if (level <= 0 || members_.empty())
    throw LogicError("Invalid CCF group factor setup.");
  int expected_level = factors_.size() + this->MinLevel();
  if (members_.size() < expected_level)
    throw ValidationError(Element::name() +
                          " CCF group is given more factors than expected.");
  if (level != expected_level)
    throw ValidationError(Element::name() + " CCF group level expected " +
                          std::to_string(expected_level) +
                          ". Instead was given " + std::to_string(level));
  factors_.emplace_back(level, factor);
}

void CcfGroup::Validate() const {
  if (!distribution_ || members_.empty() || factors_.empty())
    throw LogicError("CCF group " + Element::name() + " is not initialized.");

  if (distribution_->Min() < 0 || distribution_->Max() > 1) {
    throw ValidationError("Distribution for " + Element::name() +
                          " CCF group has illegal values.");
  }

  if (members_.size() < 2) {
    throw ValidationError(Element::name() +
                          " CCF group must have at least 2 members.");
  }

  if (factors_.back().first > members_.size()) {
    throw ValidationError("The level of factors for " + Element::name() +
                          " CCF group cannot be more than # of members.");
  }

  for (const std::pair<int, ExpressionPtr>& f : factors_) {
    if (f.second->Max() > 1 || f.second->Min() < 0) {
      throw ValidationError("Factors for " + Element::name() +
                            " CCF group have illegal values.");
    }
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
  for (const BasicEventPtr& member : members_) {
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
    ExpressionPtr prob = entry.second;
    for (auto combination :
         ext::make_combination_generator(level, proxy_gates.begin(),
                                         proxy_gates.end())) {
      auto ccf_event = std::make_shared<CcfEvent>(JoinNames(combination), this);
      ccf_event->expression(prob);
      for (Gate* gate : combination)
        gate->formula().AddArgument(ccf_event);
      ccf_event->members(std::move(combination));  // Move, at last.
    }
  }
}

CcfGroup::ExpressionMap BetaFactorModel::CalculateProbabilities() {
  assert(CcfGroup::factors().size() == 1);
  assert(CcfGroup::members().size() == CcfGroup::factors().front().first);

  ExpressionMap probabilities;

  ExpressionPtr beta = CcfGroup::factors().begin()->second;
  ExpressionPtr indep_factor(new Sub({ConstantExpression::kOne, beta}));
  probabilities.emplace_back(  // (1 - beta) * Q
      1,
      ExpressionPtr(new Mul({indep_factor, CcfGroup::distribution()})));

  probabilities.emplace_back(  // beta * Q
      CcfGroup::factors().front().first,
      ExpressionPtr(new Mul({beta, CcfGroup::distribution()})));
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
    std::vector<ExpressionPtr> args;
    args.emplace_back(new ConstantExpression(mult));
    for (int j = 0; j < i; ++j) {
      args.push_back(CcfGroup::factors()[j].second);
    }
    if (i < max_level - 1) {
      args.emplace_back(
          new Sub({ConstantExpression::kOne, CcfGroup::factors()[i].second}));
    }
    args.push_back(CcfGroup::distribution());
    probabilities.emplace_back(i + 1, ExpressionPtr(new Mul(args)));
  }
  assert(probabilities.size() == max_level);
  return probabilities;
}

CcfGroup::ExpressionMap AlphaFactorModel::CalculateProbabilities() {
  ExpressionMap probabilities;
  int max_level = CcfGroup::factors().back().first;
  assert(CcfGroup::factors().size() == max_level);
  std::vector<ExpressionPtr> sum_args;
  for (const std::pair<int, ExpressionPtr>& factor : CcfGroup::factors()) {
    sum_args.emplace_back(new Mul(
        {ExpressionPtr(new ConstantExpression(factor.first)), factor.second}));
  }
  ExpressionPtr sum(new Add(std::move(sum_args)));
  int num_members = CcfGroup::members().size();

  for (int i = 0; i < max_level; ++i) {
    double mult = CalculateCombinationReciprocal(num_members - 1, i);
    ExpressionPtr level(new ConstantExpression(i + 1));
    ExpressionPtr fraction(new Div({CcfGroup::factors()[i].second, sum}));
    ExpressionPtr prob(
        new Mul({level, ExpressionPtr(new ConstantExpression(mult)), fraction,
                 CcfGroup::distribution()}));
    probabilities.emplace_back(i + 1, prob);
  }
  assert(probabilities.size() == max_level);
  return probabilities;
}

void PhiFactorModel::DoValidate() const {
  double sum = 0;
  double sum_min = 0;
  double sum_max = 0;
  for (const std::pair<int, ExpressionPtr>& factor : CcfGroup::factors()) {
    sum += factor.second->Mean();
    sum_min += factor.second->Min();
    sum_max += factor.second->Max();
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
  for (const std::pair<int, ExpressionPtr>& factor : CcfGroup::factors()) {
    ExpressionPtr prob(new Mul({factor.second, CcfGroup::distribution()}));
    probabilities.emplace_back(factor.first, prob);
  }
  assert(probabilities.size() == max_level);
  return probabilities;
}

}  // namespace mef
}  // namespace scram
