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

/// @file ccf_group.h
/// Functional containers for basic events
/// grouped by common cause failure.
/// Common cause failure can be modeled
/// with alpha, beta, MGL,
/// or direct parameter assignment in phi model.

#ifndef SCRAM_SRC_CCF_GROUP_H_
#define SCRAM_SRC_CCF_GROUP_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "element.h"
#include "event.h"
#include "expression.h"

namespace scram {
namespace mef {

/// Abstract base class for all common cause failure models.
class CcfGroup : public Element, public Role, public Id {
 public:
  /// Constructor to be used by derived classes.
  ///
  /// @param[in] name  The name of a CCF group.
  /// @param[in] base_path  The series of containers to get this group.
  /// @param[in] role  The role of the CCF group within the model or container.
  ///
  /// @throws LogicError  The name is empty.
  explicit CcfGroup(std::string name, std::string base_path = "",
                    RoleSpecifier role = RoleSpecifier::kPublic);

  CcfGroup(const CcfGroup&) = delete;
  CcfGroup& operator=(const CcfGroup&) = delete;

  virtual ~CcfGroup() = default;

  /// @returns Members of the CCF group with original names as keys.
  const std::map<std::string, BasicEventPtr>& members() const {
    return members_;
  }

  /// Adds a basic event into this CCF group.
  /// This function asserts that each basic event has unique string id.
  ///
  /// @param[in] basic_event  A member basic event.
  ///
  /// @throws DuplicateArgumentError  The basic event is already in the group.
  /// @throws IllegalOperation  The probability distribution
  ///                           for this CCF group is already defined.
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
  /// @throws LogicError  The distribution has already been defined.
  void AddDistribution(const ExpressionPtr& distr);

  /// Adds a CCF factor for the specified model.
  /// The addition of factors must be in ascending level order
  /// and no gaps are allowed between levels.
  /// The default case is to start from 1.
  ///
  /// @param[in] factor  A factor for the CCF model.
  /// @param[in] level  The level of the passed factor.
  ///
  /// @throws ValidationError  Level is not what is expected.
  /// @throws LogicError  The level is not positive.
  void AddFactor(const ExpressionPtr& factor, int level) {
    this->CheckLevel(level);
    factors_.emplace_back(level, factor);
  }

  /// Checks if the provided distribution is between 0 and 1.
  /// This check must be performed before validating basic events
  /// that are members of this CCF group
  /// to give more precise error messages.
  ///
  /// @throws ValidationError  There is an issue with the distribution.
  void ValidateDistribution();

  /// Validates the setup for the CCF model and group.
  /// The passed expressions must be checked for circular logic
  /// before initiating the CCF validation.
  ///
  /// @throws ValidationError  There is an issue with the setup.
  virtual void Validate() const;

  /// Processes the given factors and members
  /// to create common cause failure probabilities and new events
  /// that can replace the members in a fault tree.
  void ApplyModel();

 protected:
  /// Mapping expressions and their application levels.
  using ExpressionMap = std::vector<std::pair<int, ExpressionPtr>>;

  /// @returns The probability distribution of the events.
  const ExpressionPtr& distribution() const { return distribution_; }

  /// @returns CCF factors of the model.
  const ExpressionMap& factors() const { return factors_; }

 private:
  /// Checks the level of factors
  /// before the addition of factors.
  /// By default,
  /// the addition of factors must be in ascending level order
  /// and no gaps are allowed between levels.
  ///
  /// @param[in] level  The level of the passed factor.
  ///
  /// @throws ValidationError  Level is not what is expected.
  /// @throws LogicError  The level is not positive.
  virtual void CheckLevel(int level);

  /// Calculates probabilities for new basic events
  /// representing failures due to common cause.
  /// Each derived common cause failure model
  /// must implement this function
  /// with its own specific formulas and assumptions.
  ///
  /// @returns  Expressions representing probabilities
  ///           for each level of groupings for CCF events.
  virtual ExpressionMap CalculateProbabilities() = 0;

  /// Members of CCF groups.
  /// @todo Consider other cross-platform stable data structures or approaches.
  std::map<std::string, BasicEventPtr> members_;
  ExpressionPtr distribution_;  ///< The probability distribution of the group.
  ExpressionMap factors_;  ///< CCF factors for models to get CCF probabilities.
};

using CcfGroupPtr = std::shared_ptr<CcfGroup>;  ///< Shared CCF groups.

/// Common cause failure model that assumes,
/// if common cause failure occurs,
/// then all components or members fail simultaneously or within short time.
class BetaFactorModel : public CcfGroup {
 public:
  using CcfGroup::CcfGroup;  ///< Standard group constructor with a group name.

 private:
  /// Checks a CCF factor level for the beta model.
  /// Only one factor is expected.
  ///
  /// @param[in] level  The level of the passed factor.
  ///
  /// @throws ValidationError  Level is not what is expected.
  /// @throws LogicError  The level is not positive.
  void CheckLevel(int level) override;

  ExpressionMap CalculateProbabilities() override;
};

/// Multiple Greek Letters model characterizes failure of
/// sub-groups of the group due to common cause.
/// The factor for k-component group defines
/// fraction of failure k or more members
/// given that (k-1) members failed.
class MglModel : public CcfGroup {
 public:
  using CcfGroup::CcfGroup;  ///< Standard group constructor with a group name.

 private:
  /// Checks a CCF factor level for the MGL model.
  /// The factor level must start from 2.
  ///
  /// @param[in] level  The level of the passed factor.
  ///
  /// @throws ValidationError  Level is not what is expected.
  /// @throws LogicError  The level is not positive.
  void CheckLevel(int level) override;

  ExpressionMap CalculateProbabilities() override;
};

/// Alpha factor model characterizes
/// failure of exactly k members of
/// the group due to common cause.
class AlphaFactorModel : public CcfGroup {
 public:
  using CcfGroup::CcfGroup;  ///< Standard group constructor with a group name.

 private:
  ExpressionMap CalculateProbabilities() override;
};

/// Phi factor model is a simplification,
/// where fractions of k-member group failure is given directly.
/// Thus, Q_k = phi_k * Q_total.
/// This model is described in the OpenPSA Model Exchange Format.
class PhiFactorModel : public CcfGroup {
 public:
  using CcfGroup::CcfGroup;  ///< Standard group constructor with a group name.

  /// In addition to the default validation of CcfGroup,
  /// checks if the given factors' sum is 1.
  ///
  /// @throws ValidationError  There is an issue with the setup.
  ///
  /// @todo Problem with sampling the factors and not getting exactly 1.
  ///       Currently only accepts constant expressions.
  void Validate() const override;

 private:
  ExpressionMap CalculateProbabilities() override;
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_CCF_GROUP_H_
