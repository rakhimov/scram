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

/// @file ccf_group.h
/// Functional containers for basic events
/// grouped by common cause failure.
/// Common cause failure can be modeled
/// with alpha, beta, MGL,
/// or direct parameter assignment in phi model.

#ifndef SCRAM_SRC_CCF_GROUP_H_
#define SCRAM_SRC_CCF_GROUP_H_

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "element.h"
#include "event.h"
#include "expression.h"

namespace scram {
namespace mef {

class CcfGroup;  // CCF Events know their own groups.

/// A basic event that represents a multiple failure of
/// a group of events due to a common cause.
/// This event is generated out of a common cause group.
/// This class is a helper to report correctly the CCF events.
class CcfEvent : public BasicEvent {
 public:
  /// Constructs CCF event with specific name
  /// that is used for internal purposes.
  /// This name is formatted by the CcfGroup.
  /// The creator CCF group
  /// and names of the member events of this specific CCF event
  /// are saved for reporting.
  ///
  /// @param[in] name  The identifying name of this CCF event.
  /// @param[in] ccf_group  The CCF group that created this event.
  CcfEvent(std::string name, const CcfGroup* ccf_group);

  /// @returns The CCF group that created this CCF event.
  const CcfGroup& ccf_group() const { return ccf_group_; }

  /// @returns Members of this CCF event.
  ///          The members also own this CCF event through parentship.
  const std::vector<Gate*>& members() const { return members_; }

  /// Sets the member parents.
  ///
  /// @param[in] members  The members that this CCF event
  ///                     represents as multiple failure.
  ///
  /// @note The reason for late setting of members
  ///       instead of in the constructor is moveability.
  ///       The container of member gates can only move
  ///       after the creation of the event.
  void members(std::vector<Gate*> members) {
    assert(members_.empty() && "Resetting members.");
    members_ = std::move(members);
  }

 private:
  const CcfGroup& ccf_group_;  ///< The originating CCF group.
  std::vector<Gate*> members_;  ///< Member parent gates of this CCF event.
};

/// Abstract base class for all common cause failure models.
class CcfGroup : public Id, private boost::noncopyable {
 public:
  using Id::Id;

  virtual ~CcfGroup() = default;

  /// @returns Members of the CCF group with original names as keys.
  const std::vector<BasicEventPtr>& members() const { return members_; }

  /// Adds a basic event into this CCF group.
  /// This function asserts that each basic event has unique string id.
  ///
  /// @param[in] basic_event  A member basic event.
  ///
  /// @throws DuplicateArgumentError  The basic event is already in the group.
  /// @throws IllegalOperation  The probability distribution or factors
  ///                           for this CCF group are already defined.
  ///                           No more members are accepted.
  void AddMember(const BasicEventPtr& basic_event);

  /// Adds the distribution that describes the probability of
  /// basic events in this CCF group.
  /// All basic events should be added as members
  /// before defining their probabilities.
  /// No more basic events can be added after this function.
  ///
  /// @param[in] distr  The probability distribution of this group.
  ///
  /// @throws ValidationError  Not enough members.
  /// @throws LogicError  The distribution has already been defined.
  void AddDistribution(Expression* distr);

  /// Adds a CCF factor for the specified model.
  /// All basic events should be added as members
  /// before defining the CCF factors.
  /// No more basic events can be added after this function.
  ///
  /// @param[in] factor  A factor for the CCF model.
  /// @param[in] level  The level of the passed factor.
  ///
  /// @throws ValidationError  The level is invalid.
  /// @throws RedefinitionError  The factor for the level already exists.
  /// @throws LogicError  The level is not positive,
  ///                     or the CCF group members are undefined.
  void AddFactor(Expression* factor, boost::optional<int> level = {});

  /// Validates the setup for the CCF model and group.
  /// Checks if the provided distribution is between 0 and 1.
  ///
  /// This check must be performed before validating basic events
  /// that are members of this CCF group
  /// to give more precise error messages.
  ///
  /// @throws ValidationError  There is an issue with the setup.
  /// @throws LogicError  The primary distribution, event, factors are not set.
  void Validate() const;

  /// Processes the given factors and members
  /// to create common cause failure probabilities and new events
  /// that can replace the members in a fault tree.
  ///
  /// @pre The CCF is validated.
  void ApplyModel();

 protected:
  /// Mapping expressions and their application levels.
  using ExpressionMap = std::vector<std::pair<int, Expression*>>;

  /// @returns The probability distribution of the events.
  Expression* distribution() const { return distribution_; }

  /// @returns CCF factors of the model.
  const ExpressionMap& factors() const { return factors_; }

  /// Registers a new expression for ownership by the group.
  /// @{
  template <class T, typename... Ts>
  Expression* Register(Ts&&... args) {
    expressions_.emplace_back(std::make_unique<T>(std::forward<Ts>(args)...));
    return expressions_.back().get();
  }
  template <class T>
  Expression* Register(std::initializer_list<Expression*> args) {
    expressions_.emplace_back(std::make_unique<T>(args));
    return expressions_.back().get();
  }
  /// @}

 private:
  /// @returns The minimum level for CCF factors for the specific model.
  virtual int min_level() const { return 1; }

  /// Runs any additional validation specific to the CCF models.
  /// All the general validation is done in the base class Validate function.
  /// The derived classes should only provided additional logic if any.
  ///
  /// @throws ValidationError  The model is invalid.
  virtual void DoValidate() const {}

  /// Calculates probabilities for new basic events
  /// representing failures due to common cause.
  /// Each derived common cause failure model
  /// must implement this function
  /// with its own specific formulas and assumptions.
  ///
  /// @returns  Expressions representing probabilities
  ///           for each level of groupings for CCF events.
  virtual ExpressionMap CalculateProbabilities() = 0;

  int prev_level_ = 0;  ///< To deduce optional levels from the previous level.
  Expression* distribution_ = nullptr;  ///< The group probability distribution.
  std::vector<BasicEventPtr> members_;  ///< Members of CCF groups.
  ExpressionMap factors_;  ///< CCF factors for models to get CCF probabilities.
  /// Collection of expressions created specifically for this group.
  std::vector<std::unique_ptr<Expression>> expressions_;
  /// CCF events created by the group.
  std::vector<std::unique_ptr<CcfEvent>> ccf_events_;
};

using CcfGroupPtr = std::shared_ptr<CcfGroup>;  ///< Shared CCF groups.

/// Common cause failure model that assumes,
/// if common cause failure occurs,
/// then all components or members fail simultaneously or within short time.
class BetaFactorModel : public CcfGroup {
 public:
  using CcfGroup::CcfGroup;

 private:
  int min_level() const override { return CcfGroup::members().size(); }

  ExpressionMap CalculateProbabilities() override;
};

/// Multiple Greek Letters model characterizes failure of
/// sub-groups of the group due to common cause.
/// The factor for k-component group defines
/// fraction of failure k or more members
/// given that (k-1) members failed.
class MglModel : public CcfGroup {
 public:
  using CcfGroup::CcfGroup;

 private:
  int min_level() const override { return 2; }

  ExpressionMap CalculateProbabilities() override;
};

/// Alpha factor model characterizes
/// failure of exactly k members of
/// the group due to common cause.
class AlphaFactorModel : public CcfGroup {
 public:
  using CcfGroup::CcfGroup;

 private:
  ExpressionMap CalculateProbabilities() override;
};

/// Phi factor model is a simplification,
/// where fractions of k-member group failure is given directly.
/// Thus, Q_k = phi_k * Q_total.
/// This model is described in the Open-PSA Model Exchange Format.
class PhiFactorModel : public CcfGroup {
 public:
  using CcfGroup::CcfGroup;

 private:
  /// In addition to the default validation of CcfGroup,
  /// checks if the given factors' sum is 1.
  ///
  /// @throws ValidationError  There is an issue with the setup.
  void DoValidate() const override;

  ExpressionMap CalculateProbabilities() override;
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_CCF_GROUP_H_
