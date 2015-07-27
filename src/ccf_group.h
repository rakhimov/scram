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
/// @file ccf_group.h
/// Functional containers for basic events grouped by common cause failure.
/// Common cause failure can be modeled using alpha, beta, MGL, or direct
/// parameter assignment in phi model.
#ifndef SCRAM_SRC_CCF_GROUP_H_
#define SCRAM_SRC_CCF_GROUP_H_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "element.h"
#include "event.h"
#include "expression.h"

namespace scram {

/// @class CcfGroup
/// Abstract base class for all common cause failure models.
class CcfGroup : public Element, public Role {
 public:
  typedef boost::shared_ptr<Expression> ExpressionPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;

  /// Constructor to be used by derived classes.
  ///
  /// @param[in] name The name of a CCF group.
  /// @param[in] model CCF model of this group.
  /// @param[in] base_path The series of containers to get this group.
  /// @param[in] is_public Whether or not the group is public.
  CcfGroup(const std::string& name, const std::string& model,
           const std::string& base_path = "", bool is_public = true);

  virtual ~CcfGroup() {}

  /// @returns The name of this CCF group.
  inline const std::string& name() const { return name_; }

  /// @returns The identifier of this CCF group.
  inline const std::string& id() const { return id_; }

  /// @returns The CCF model applied to this group.
  inline const std::string& model() const { return model_; }

  /// @returns Members of the CCF group with lower-case names as keys.
  inline const std::map<std::string, BasicEventPtr>& members() const {
    return members_;
  }

  /// Adds a basic event into this CCF group.
  /// This function asserts that each basic event has unique string id.
  ///
  /// @param[in] basic_event A member basic event.
  ///
  /// @throws DuplicateArgumentError The basic event is already in the group.
  /// @throws IllegalOperation The probability distribution for this
  ///                          CCF group is already defined. No more members
  ///                          are accepted.
  void AddMember(const BasicEventPtr& basic_event);

  /// Adds the distribution that describes the probability of basic events
  /// in this CCF group. All basic events should be added as members
  /// before defining their probabilities. No more basic events can be
  /// added after this function.
  ///
  /// @param[in] distr The probability distribution of this group.
  void AddDistribution(const ExpressionPtr& distr);

  /// Adds a CCF factor for the specified model. The addition of factors
  /// must be in ascending level order and no gaps are allowed between levels.
  /// The default case is to start from 1.
  ///
  /// @param[in] factor A factor for the CCF model.
  /// @param[in] level The level of the passed factor.
  ///
  /// @throws ValidationError Level is not what is expected.
  virtual void AddFactor(const ExpressionPtr& factor, int level);

  /// Checks if the provided distribution is between 0 and 1.
  /// This check must be performed before validating basic events that are
  /// members of this CCF group to give more precise error messages.
  ///
  /// @throws ValidationError There is an issue with the distribution.
  void ValidateDistribution();

  /// Validates the setup for the CCF model and group.
  /// The passed expressions must be checked for circular logic before
  /// initiating the CCF validation.
  ///
  /// @throws ValidationError There is an issue with the setup.
  virtual void Validate();

  /// Processes the given factors and members to create common cause failure
  /// probabilities and new events that can replace the members in a fault
  /// tree.
  void ApplyModel();

 protected:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Creates new basic events from members. The new basic events
  /// are included in the database of new events.
  ///
  /// @param[in] max_level The max level for grouping.
  /// @param[out] new_events New basic events and their parents.
  virtual void ConstructCcfBasicEvents(
      int max_level,
      std::map<BasicEventPtr, std::set<std::string> >* new_events);

  /// Calculates probabilities for new basic events representing failures
  /// due to common cause. Each derived common cause failure model must
  /// implement this function with its own specific formulas and assumptions.
  ///
  /// @param[in] max_level The max level of grouping.
  /// @param[out] probabilities Expressions representing probabilities for
  ///                           each level of groupings for CCF events.
  virtual void CalculateProb(int max_level,
                             std::map<int, ExpressionPtr>* probabilities) = 0;

  /// Simple factorial calculation.
  ///
  /// @param[in] n Positive number for factorial calculation.
  ///
  /// @returns n factorial.
  int Factorial(int n) { return n ? n * Factorial(n - 1) : 1; }

  std::string name_;  ///< The name of the CCF group.
  std::string id_;  ///< The unique identifier of the CCF group.
  std::map<std::string, BasicEventPtr> members_;  ///< Members of CCF groups.
  ExpressionPtr distribution_;  ///< The probability distribution of the group.
  /// CCF factors for models to get CCF probabilities.
  std::vector<std::pair<int, ExpressionPtr> > factors_;

 private:
  std::string model_;  ///< Common cause model type.
};

/// @class BetaFactorModel
/// Common cause failure model that assumes, if common cause failure occurs,
/// then all components or members fail simultaneously or within short time.
class BetaFactorModel : public CcfGroup {
 public:
  /// Constructs the group and sets the model.
  ///
  /// @param[in] name The name for the group.
  /// @param[in] base_path The series of containers to get this group.
  /// @param[in] is_public Whether or not the group is public.
  explicit BetaFactorModel(const std::string& name,
                           const std::string& base_path = "",
                           bool is_public = true)
      : CcfGroup(name, "beta-factor", base_path, is_public) {}

  /// Adds a CCF factor for the beta model. Only one factor is expected.
  ///
  /// @param[in] factor A factor for the CCF model.
  /// @param[in] level The level of the passed factor.
  ///
  /// @throws ValidationError Level is not what is expected.
  void AddFactor(const ExpressionPtr& factor, int level);

 private:
  void ConstructCcfBasicEvents(
      int max_level,
      std::map<BasicEventPtr, std::set<std::string> >* new_events);

  void CalculateProb(int max_level,
                     std::map<int, ExpressionPtr>* probabilities);
};

/// @class MglModel
/// Multiple Greek Letters model characterizes failure of sub-groups of
/// the group due to common cause. The factor for k-component group defines
/// fraction of failure k or more members given that (k-1) members failed.
class MglModel : public CcfGroup {
 public:
  /// Constructs the group and sets the model.
  ///
  /// @param[in] name The name for the group.
  /// @param[in] base_path The series of containers to get this group.
  /// @param[in] is_public Whether or not the group is public.
  explicit MglModel(const std::string& name,
                    const std::string& base_path = "",
                    bool is_public = true)
      : CcfGroup(name, "MGL", base_path, is_public) {}

  /// Adds a CCF factor for the MGL model. The factor level must start
  /// from 2.
  ///
  /// @param[in] factor A factor for the CCF model.
  /// @param[in] level The level of the passed factor.
  ///
  /// @throws ValidationError Level is not what is expected.
  void AddFactor(const ExpressionPtr& factor, int level);

 private:
  void CalculateProb(int max_level,
                     std::map<int, ExpressionPtr>* probabilities);
};

/// @class AlphaFactorModel
/// Alpha factor model characterizes failure of exactly k members of
/// the group due to common cause.
class AlphaFactorModel : public CcfGroup {
 public:
  /// Constructs the group and sets the model.
  ///
  /// @param[in] name The name for the group.
  /// @param[in] base_path The series of containers to get this group.
  /// @param[in] is_public Whether or not the group is public.
  explicit AlphaFactorModel(const std::string& name,
                            const std::string& base_path = "",
                            bool is_public = true)
      : CcfGroup(name, "alpha-factor", base_path, is_public) {}

 private:
  void CalculateProb(int max_level,
                     std::map<int, ExpressionPtr>* probabilities);
};

/// @class PhiFactorModel
/// Phi factor model is a simplification, where fractions of k-member group
/// failure is given directly. Thus, Q_k = phi_k * Q_total.
/// This model is described in OpenPSA Model Exchange Format.
class PhiFactorModel : public CcfGroup {
 public:
  /// Constructs the group and sets the model.
  ///
  /// @param[in] name The name for the group.
  /// @param[in] base_path The series of containers to get this group.
  /// @param[in] is_public Whether or not the group is public.
  explicit PhiFactorModel(const std::string& name,
                          const std::string& base_path = "",
                          bool is_public = true)
      : CcfGroup(name, "phi-factor", base_path, is_public) {}

  /// In addition to the default validation of CcfGroup, checks if
  /// the given factors' sum is 1.
  ///
  /// @throws ValidationError There is an issue with the setup.
  ///
  /// @todo Problem with sampling the factors and not getting exactly 1.
  ///       Currently only accepts constant expressions.
  void Validate();

 private:
  void CalculateProb(int max_level,
                     std::map<int, ExpressionPtr>* probabilities);
};

}  // namespace scram

#endif  // SCRAM_SRC_CCF_GROUP_H_
