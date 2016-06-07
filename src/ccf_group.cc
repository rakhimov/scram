/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

#include <sstream>

namespace scram {
namespace mef {

CcfGroup::CcfGroup(std::string name, std::string base_path, RoleSpecifier role)
    : Element(std::move(name)),
      Role(role, std::move(base_path)),
      Id(*this, *this) {}

void CcfGroup::AddMember(const BasicEventPtr& basic_event) {
  if (distribution_) {
    throw IllegalOperation("No more members accepted. The distribution for " +
                           Element::name() +
                           " CCF group has already been defined.");
  }
  if (members_.count(basic_event->name())) {
    throw DuplicateArgumentError("Duplicate member " + basic_event->name() +
                                 " in " + Element::name() + " CCF group.");
  }
  members_.emplace(basic_event->name(), basic_event);
}

void CcfGroup::AddDistribution(const ExpressionPtr& distr) {
  if (distribution_) throw LogicError("CCF distribution is already defined.");
  distribution_ = distr;
  // Define probabilities of all basic events.
  for (const std::pair<const std::string, BasicEventPtr>& mem : members_) {
    mem.second->expression(distribution_);
  }
}

void CcfGroup::CheckLevel(int level) {
  if (level <= 0) throw LogicError("CCF group level is not positive.");
  if (level != factors_.size() + 1) {
    std::stringstream msg;
    msg << Element::name() << " CCF group level expected "
        << factors_.size() + 1 << ". Instead was given " << level;
    throw ValidationError(msg.str());
  }
}

void CcfGroup::ValidateDistribution() {
  if (distribution_->Min() < 0 || distribution_->Max() > 1) {
    throw ValidationError("Distribution for " + Element::name() + " CCF group" +
                          " has illegal values.");
  }
}

void CcfGroup::Validate() {
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
      throw ValidationError("Factors for " + Element::name() + " CCF group" +
                            " have illegal values.");
    }
  }
}

void CcfGroup::ApplyModel() {
  // Construct replacement gates for member basic events.
  std::map<std::string, GatePtr> gates;
  for (const std::pair<const std::string, BasicEventPtr>& mem : members_) {
    BasicEventPtr member = mem.second;
    GatePtr new_gate(
        new Gate(member->name(), member->base_path(), member->role()));
    assert(member->id() == new_gate->id());
    new_gate->formula(FormulaPtr(new Formula("or")));
    gates.emplace(mem.first, new_gate);
    member->ccf_gate(new_gate);
  }

  int max_level = factors_.back().first;  // Assumes that factors are
                                          // sequential.
  std::map<int, ExpressionPtr> probabilities;  // The level is position + 1.
  this->CalculateProbabilities(max_level, &probabilities);

  // Mapping of new basic events and their parents.
  std::map<BasicEventPtr, std::set<std::string>> new_events;
  this->ConstructCcfBasicEvents(max_level, &new_events);

  assert(!new_events.empty());
  for (const auto& mem : new_events) {
    int level = mem.second.size();
    ExpressionPtr prob = probabilities.find(level)->second;
    BasicEventPtr new_event = mem.first;
    new_event->expression(prob);
    // Add this basic event to the parent gates.
    for (const std::string& parent : mem.second) {
      gates.at(parent)->formula()->AddArgument(new_event);
    }
  }
}

void CcfGroup::ConstructCcfBasicEvents(
    int max_level,
    std::map<BasicEventPtr, std::set<std::string> >* new_events) {
  assert(max_level > 1);
  assert(members_.size() > 1);
  assert(max_level <= members_.size());
  assert(new_events->empty());

  std::set<std::set<std::string>> combinations = {{}};
  for (int i = 0; i < max_level; ++i) {
    std::set<std::set<std::string>> next_level;
    for (const std::set<std::string>& combination : combinations) {
      for (const std::pair<const std::string, BasicEventPtr>& mem : members_) {
        if (!combination.count(mem.first)) {
          std::set<std::string> comb(combination);
          comb.insert(mem.first);
          next_level.insert(comb);
        }
      }
    }
    for (const std::set<std::string>& combination : next_level) {
      std::string name = "[";
      std::vector<std::string> names;
      for (auto it_s = combination.begin(); it_s != combination.end();) {
        std::string member_name = members_.at(*it_s)->name();
        name += member_name;
        names.push_back(member_name);
        ++it_s;
        if (it_s != combination.end()) name += " ";
      }
      name += "]";
      new_events->emplace(std::make_shared<CcfEvent>(name, this, names),
                          combination);
    }
    combinations = next_level;
  }
}

void BetaFactorModel::CheckLevel(int level) {
  if (level <= 0) throw LogicError("CCF group level is not positive.");
  if (!CcfGroup::factors().empty()) {
    throw ValidationError("Beta-Factor Model " + CcfGroup::name() +
                          " CCF group must have exactly one factor.");
  }
  if (level != CcfGroup::members().size()) {
    throw ValidationError(
        "Beta-Factor Model " + CcfGroup::name() + " CCF group" +
        " must have the level matching the number of its members.");
  }
}

void BetaFactorModel::ConstructCcfBasicEvents(
    int max_level,
    std::map<BasicEventPtr, std::set<std::string> >* new_events) {
  // This function is not optimized or efficient for beta-factor models.
  assert(CcfGroup::factors().size() == 1);
  std::map<BasicEventPtr, std::set<std::string>> all_events;
  CcfGroup::ConstructCcfBasicEvents(max_level, &all_events);  // Standard case.

  for (const auto& event : all_events) {
    int level = event.second.size();  // Filter out only relevant levels.
    if (level == 1 || level == max_level) new_events->insert(event);
  }
}

void BetaFactorModel::CalculateProbabilities(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  ExpressionPtr one(new ConstantExpression(1.0));
  ExpressionPtr beta = CcfGroup::factors().begin()->second;
  ExpressionPtr indep_factor(new Sub({one, beta}));  // (1 - beta)
  probabilities->emplace(  // (1 - beta) * Q
      1,
      ExpressionPtr(new Mul({indep_factor, CcfGroup::distribution()})));

  probabilities->emplace(  // beta * Q
      max_level,
      ExpressionPtr(new Mul({beta, CcfGroup::distribution()})));
}

void MglModel::CheckLevel(int level) {
  if (level <= 0) throw LogicError("CCF group level is not positive.");
  if (level != CcfGroup::factors().size() + 2) {
    std::stringstream msg;
    msg << CcfGroup::name() << " MGL model CCF group level expected "
        << CcfGroup::factors().size() + 2 << ". Instead was given " << level;
    throw ValidationError(msg.str());
  }
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
  if (n - k > k) k = n - k;
  double result = 1;
  for (int i = 1; i <= n - k; ++i) {
    result *= static_cast<double>(i) / static_cast<double>(k + i);
  }
  return result;
}

}  // namespace

void MglModel::CalculateProbabilities(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(CcfGroup::factors().size() == max_level - 1);

  ExpressionPtr one(new ConstantExpression(1.0));
  int num_members = CcfGroup::members().size();
  for (int i = 0; i < max_level; ++i) {
    double mult = CalculateCombinationReciprocal(num_members - 1, i);
    std::vector<ExpressionPtr> args;
    args.push_back(ExpressionPtr(new ConstantExpression(mult)));
    for (int j = 0; j < i; ++j) {
      args.push_back(CcfGroup::factors()[j].second);
    }
    if (i < max_level - 1) {
      args.push_back(
          ExpressionPtr(new Sub({one, CcfGroup::factors()[i].second})));
    }
    args.push_back(CcfGroup::distribution());
    probabilities->emplace(i + 1, ExpressionPtr(new Mul(args)));
  }
  assert(probabilities->size() == max_level);
}

void AlphaFactorModel::CalculateProbabilities(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  assert(CcfGroup::factors().size() == max_level);
  std::vector<ExpressionPtr> sum_args;
  for (const std::pair<int, ExpressionPtr>& factor : CcfGroup::factors()) {
    sum_args.push_back(factor.second);
  }
  ExpressionPtr sum(new Add(sum_args));
  int num_members = CcfGroup::members().size();

  for (int i = 0; i < max_level; ++i) {
    double mult = CalculateCombinationReciprocal(num_members - 1, i);
    ExpressionPtr k(new ConstantExpression(mult));
    ExpressionPtr fraction(new Div({CcfGroup::factors()[i].second, sum}));
    ExpressionPtr prob(new Mul({k, fraction, CcfGroup::distribution()}));
    probabilities->emplace(i + 1, prob);
  }
  assert(probabilities->size() == max_level);
}

void PhiFactorModel::Validate() {
  CcfGroup::Validate();
  double sum = 0;
  double sum_min = 0;
  double sum_max = 0;
  /// @todo How to assure that the sum will be 1 in sampling.
  ///       Is it allowed to have a factor sampling for Uncertainty analysis.
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

void PhiFactorModel::CalculateProbabilities(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  for (const std::pair<int, ExpressionPtr>& factor : CcfGroup::factors()) {
    ExpressionPtr prob(new Mul({factor.second, CcfGroup::distribution()}));
    probabilities->emplace(factor.first, prob);
  }
  assert(probabilities->size() == max_level);
}

}  // namespace mef
}  // namespace scram
