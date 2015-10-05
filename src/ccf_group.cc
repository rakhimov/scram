/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

#include <boost/algorithm/string.hpp>

namespace scram {

CcfGroup::CcfGroup(const std::string& name, const std::string& model,
                   const std::string& base_path, bool is_public)
    : Role::Role(is_public, base_path),
      name_(name),
      model_(model) {
  assert(name != "");
  id_ = is_public ? name : base_path + "." + name;  // Unique combination.
  boost::to_lower(id_);
}

void CcfGroup::AddMember(const BasicEventPtr& basic_event) {
  std::string name = basic_event->name();
  boost::to_lower(name);
  if (distribution_) {
    throw IllegalOperation("No more members accepted. The distribution for " +
                           name_ + " CCF group has already been defined.");
  }
  if (members_.count(name)) {
    throw DuplicateArgumentError("Duplicate member " + basic_event->name() +
                                 " in " + name_ + " CCF group.");
  }
  members_.emplace(name, basic_event);
}

void CcfGroup::AddDistribution(const ExpressionPtr& distr) {
  assert(!distribution_);
  distribution_ = distr;
  // Define probabilities of all basic events.
  for (const std::pair<std::string, BasicEventPtr>& mem : members_) {
    mem.second->expression(distribution_);
  }
}

void CcfGroup::AddFactor(const ExpressionPtr& factor, int level) {
  assert(level > 0);
  if (level != factors_.size() + 1) {
    std::stringstream msg;
    msg << name_ << " " << model_ << " CCF group level expected "
        << factors_.size() + 1 << ". Instead was given " << level;
    throw ValidationError(msg.str());
  }
  factors_.emplace_back(level, factor);
}

void CcfGroup::ValidateDistribution() {
  if (distribution_->Min() < 0 || distribution_->Max() > 1) {
    throw ValidationError("Distribution for " + name_ + " CCF group" +
                          " has illegal values.");
  }
}

void CcfGroup::Validate() {
  if (members_.size() < 2) {
    throw ValidationError(name_ + " CCF group must have at least 2 members.");
  }

  if (factors_.back().first > members_.size()) {
    throw ValidationError("The level of factors for " + name_ + " CCF group" +
                          " cannot be more than # of members.");
  }
  for (const std::pair<int, ExpressionPtr>& f : factors_) {
    if (f.second->Max() > 1 || f.second->Min() < 0) {
      throw ValidationError("Factors for " + name_ + " CCF group" +
                            " have illegal values.");
    }
  }
}

void CcfGroup::ApplyModel() {
  // Construct replacement gates for member basic events.
  std::map<std::string, GatePtr> gates;
  for (const std::pair<std::string, BasicEventPtr>& mem : members_) {
    BasicEventPtr member = mem.second;
    GatePtr new_gate(
        new Gate(member->name(), member->base_path(), member->is_public()));
    assert(member->id() == new_gate->id());
    new_gate->formula(FormulaPtr(new Formula("or")));
    gates.emplace(mem.first, new_gate);
    member->ccf_gate(new_gate);
  }

  int max_level = factors_.back().first;  // Assumes that factors are
                                          // sequential.
  std::map<int, ExpressionPtr> probabilities;  // The level is position + 1.
  this->CalculateProb(max_level, &probabilities);

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
      for (const std::pair<std::string, BasicEventPtr>& mem : members_) {
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

void BetaFactorModel::AddFactor(const ExpressionPtr& factor, int level) {
  if (!factors_.empty()) {
    throw ValidationError("Beta-Factor Model " + CcfGroup::name() +
                          " CCF group must have exactly one factor.");
  }
  if (level != members_.size()) {
    throw ValidationError(
        "Beta-Factor Model " + CcfGroup::name() + " CCF group" +
        " must have the level matching the number of its members.");
  }
  CcfGroup::factors_.emplace_back(level, factor);
}

void BetaFactorModel::ConstructCcfBasicEvents(
    int max_level,
    std::map<BasicEventPtr, std::set<std::string> >* new_events) {
  // This function is not optimized or efficient for beta-factor models.
  assert(CcfGroup::factors_.size() == 1);
  std::map<BasicEventPtr, std::set<std::string>> all_events;
  CcfGroup::ConstructCcfBasicEvents(max_level, &all_events);  // Standard case.

  for (const auto& event : all_events) {
    int level = event.second.size();  // Filter out only relevant levels.
    if (level == 1 || level == max_level) new_events->insert(event);
  }
}

void BetaFactorModel::CalculateProb(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  ExpressionPtr one(new ConstantExpression(1.0));
  ExpressionPtr beta = CcfGroup::factors_.begin()->second;
  ExpressionPtr indep_factor(new Sub({one, beta}));  // (1 - beta)
  probabilities->emplace(  // (1 - beta) * Q
      1,
      ExpressionPtr(new Mul({indep_factor, CcfGroup::distribution_})));

  probabilities->emplace(  // beta * Q
      max_level,
      ExpressionPtr(new Mul({beta, CcfGroup::distribution_})));
}

void MglModel::AddFactor(const ExpressionPtr& factor, int level) {
  assert(level > 0);
  if (level != factors_.size() + 2) {
    std::stringstream msg;
    msg << CcfGroup::name() << " MGL model CCF group level expected "
        << factors_.size() + 2 << ". Instead was given " << level;
    throw ValidationError(msg.str());
  }
  CcfGroup::factors_.emplace_back(level, factor);
}

void MglModel::CalculateProb(int max_level,
                             std::map<int, ExpressionPtr>* probabilities) {
  assert(factors_.size() == max_level - 1);

  ExpressionPtr one(new ConstantExpression(1.0));
  int num_members = members_.size();
  for (int i = 0; i < max_level; ++i) {
    // (n - 1) choose (k - 1) element in the equation.
    int mult = CcfGroup::Factorial(num_members - 1) /
               CcfGroup::Factorial(num_members - i - 1) /
               CcfGroup::Factorial(i);

    std::vector<ExpressionPtr> args;
    args.push_back(ExpressionPtr(new ConstantExpression(1.0 / mult)));
    for (int j = 0; j < i; ++j) {
      args.push_back(factors_[j].second);
    }
    if (i < max_level - 1) {
      args.push_back(ExpressionPtr(new Sub({one, factors_[i].second})));
    }
    args.push_back(CcfGroup::distribution_);
    probabilities->emplace(i + 1, ExpressionPtr(new Mul(args)));
  }
  assert(probabilities->size() == max_level);
}

void AlphaFactorModel::CalculateProb(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  assert(factors_.size() == max_level);
  std::vector<ExpressionPtr> sum_args;
  for (const std::pair<int, ExpressionPtr>& factor : CcfGroup::factors_) {
    sum_args.push_back(factor.second);
  }
  ExpressionPtr sum(new Add(sum_args));
  int num_members = members_.size();

  for (int i = 0; i < max_level; ++i) {
    // (n - 1) choose (k - 1) element in the equation.
    int mult = CcfGroup::Factorial(num_members - 1) /
               CcfGroup::Factorial(num_members - i - 1) /
               CcfGroup::Factorial(i);

    ExpressionPtr k(new ConstantExpression(1.0 / mult));
    ExpressionPtr fraction(new Div({CcfGroup::factors_[i].second, sum}));
    ExpressionPtr prob(new Mul({k, fraction, CcfGroup::distribution_}));
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
  for (const std::pair<int, ExpressionPtr>& factor : CcfGroup::factors_) {
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

void PhiFactorModel::CalculateProb(
    int max_level,
    std::map<int, ExpressionPtr>* probabilities) {
  assert(probabilities->empty());
  for (const std::pair<int, ExpressionPtr>& factor : CcfGroup::factors_) {
    ExpressionPtr prob(new Mul({factor.second, CcfGroup::distribution_}));
    probabilities->emplace(factor.first, prob);
  }
  assert(probabilities->size() == max_level);
}

}  // namespace scram
