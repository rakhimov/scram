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
/// Implementation of various common cause failure models.

#include "ccf_group.h"

#include <cmath>

#include <utility>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "error.h"
#include "expression/constant.h"
#include "expression/numerical.h"
#include "ext/algorithm.h"
#include "ext/combination.h"
#include "ext/float_compare.h"

namespace scram::mef {

CcfEvent::CcfEvent(std::vector<Gate*> members, const CcfGroup* ccf_group)
    : BasicEvent(MakeName(members), ccf_group->base_path(), ccf_group->role()),
      ccf_group_(*ccf_group),
      members_(std::move(members)) {}

std::string CcfEvent::MakeName(const std::vector<Gate*>& members) {
  return "[" +
         boost::join(members | boost::adaptors::transformed(
                                   [](const Gate* gate) -> decltype(auto) {
                                     return gate->name();
                                   }),
                     " ") +
         "]";
}

void CcfGroup::AddMember(BasicEvent* basic_event) {
  if (distribution_ || factors_.empty() == false) {
    SCRAM_THROW(LogicError("No more members accepted. The distribution for " +
                           Element::name() +
                           " CCF group has already been defined."));
  }
  if (ext::any_of(members_, [&basic_event](BasicEvent* member) {
        return member->name() == basic_event->name();
      })) {
    SCRAM_THROW(DuplicateArgumentError("Duplicate member " +
                                       basic_event->name() + " in " +
                                       Element::name() + " CCF group."));
  }
  members_.push_back(basic_event);
}

void CcfGroup::AddDistribution(Expression* distr) {
  if (distribution_)
    SCRAM_THROW(LogicError("CCF distribution is already defined."));
  if (members_.size() < 2) {
    SCRAM_THROW(ValidityError(Element::name() +
                              " CCF group must have at least 2 members."));
  }
  distribution_ = distr;
  // Define probabilities of all basic events.
  for (BasicEvent* member : members_)
    member->expression(distribution_);
}

void CcfGroup::AddFactor(Expression* factor, std::optional<int> level) {
  int min_level = this->min_level();
  if (!level)
    level = prev_level_ ? (prev_level_ + 1) : min_level;

  if (*level <= 0 || members_.empty())
    SCRAM_THROW(LogicError("Invalid CCF group factor setup."));

  if (*level < min_level) {
    SCRAM_THROW(ValidityError(
        "The CCF factor level (" + std::to_string(*level) +
        ") is less than the minimum level (" + std::to_string(min_level) +
        ") required by " + Element::name() + " CCF group."));
  }
  if (members_.size() < *level) {
    SCRAM_THROW(ValidityError("The CCF factor level " + std::to_string(*level) +
                              " is more than the number of members (" +
                              std::to_string(members_.size()) + ") in " +
                              Element::name() + " CCF group."));
  }

  int index = *level - min_level;
  if (index < factors_.size() && factors_[index].second != nullptr) {
    SCRAM_THROW(RedefinitionError("Redefinition of CCF factor for level " +
                                  std::to_string(*level) + " in " +
                                  Element::name() + " CCF group."));
  }
  if (index >= factors_.size())
    factors_.resize(index + 1);

  factors_[index] = {*level, factor};
  prev_level_ = *level;
}

void CcfGroup::Validate() const {
  if (!distribution_ || members_.empty() || factors_.empty())
    SCRAM_THROW(
        LogicError("CCF group " + Element::name() + " is not initialized."));

  EnsureProbability(distribution_,
                    Element::name() + " CCF group distribution.");

  for (const std::pair<int, Expression*>& f : factors_) {
    if (!f.second) {
      SCRAM_THROW(ValidityError("Missing some CCF factors for " +
                                Element::name() + " CCF group."));
    }
    EnsureProbability(f.second, Element::name() + " CCF group factors.",
                      "fraction");
  }
  this->DoValidate();
}

void CcfGroup::ApplyModel() {
  // Construct replacement proxy gates for member basic events.
  std::vector<std::pair<Gate*, Formula::ArgSet>> proxy_gates;
  for (BasicEvent* member : members_) {
    auto new_gate = std::make_unique<Gate>(member->name(), member->base_path(),
                                           member->role());
    assert(member->id() == new_gate->id());
    proxy_gates.push_back({new_gate.get(), {}});
    member->ccf_gate(std::move(new_gate));
  }

  ExpressionMap probabilities = this->CalculateProbabilities();
  assert(probabilities.size() > 1);

  // Generate CCF events.
  for (auto& [level, prob] : probabilities) {
    using Iterator = decltype(proxy_gates)::iterator;
    auto combination_visitor = [this, prob](Iterator it_begin,
                                            Iterator it_end) {
      std::vector<Gate*> combination;
      for (auto it = it_begin; it != it_end; ++it)
        combination.push_back(it->first);

      auto ccf_event = std::make_unique<CcfEvent>(std::move(combination), this);

      for (auto it = it_begin; it != it_end; ++it)
        it->second.Add(ccf_event.get());

      ccf_event->expression(prob);
      ccf_events_.emplace_back(std::move(ccf_event));

      return false;
    };
    ext::for_each_combination(proxy_gates.begin(),
                              std::next(proxy_gates.begin(), level),
                              proxy_gates.end(), combination_visitor);
  }

  // Assign formulas to the proxy gates.
  for (std::pair<Gate*, Formula::ArgSet>& gate : proxy_gates) {
    assert(gate.second.size() >= 2);
    gate.first->formula(std::make_unique<Formula>(kOr, std::move(gate.second)));
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
  if (!ext::is_close(1, sum, 1e-4) || !ext::is_close(1, sum_min, 1e-4) ||
      !ext::is_close(1, sum_max, 1e-4)) {
    SCRAM_THROW(ValidityError("The factors for Phi model " + CcfGroup::name() +
                              " CCF group must sum to 1."));
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

}  // namespace scram::mef
