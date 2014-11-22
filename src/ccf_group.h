/// @file ccf_group.h
/// Functional containers for basic events grouped by common cause failure.
/// Common cause failure can be modeled using alpha, beta, MGL, or direct
/// parameter assigment in phi model.
#ifndef SCRAM_SRC_COMMON_CAUSE_GROUP_H_
#define SCRAM_SRC_COMMON_CAUSE_GROUP_H_

#include <map>
#include <string>
#include <utility>

#include <boost/shared_ptr.hpp>

#include "element.h"
#include "event.h"
#include "expression.h"

namespace scram {

/// @class CcfGroup
/// Base class for all common cause failure models.
class CcfGroup : public Element {
 public:
  typedef boost::shared_ptr<Expression> ExpressionPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;

  virtual ~CcfGroup() {}

  /// @returns The name of this CCF group.
  inline std::string name() { return name_; }

  /// @returns The CCF model applied to this group.
  inline std::string model() { return model_; }

  /// Adds a basic event into this CCF group.
  /// This function asserts that each basic event has unique string id.
  /// @param[in] basic_event A member basic event.
  /// @throws LogicError if the passed basic event already in the group.
  /// @throws IllegalOperation if the probability distribution for this
  ///                          CCF group is already defined. No more members
  ///                          are accepted.
  void AddMember(const BasicEventPtr& basic_event);

  /// Adds the distribution that describes the probability of basic events
  /// in this CCF group. All basic events should be added as members
  /// before defining their probabilities. No more basic events can be
  /// added after this function.
  /// @param[in] distr The probability distribution of this group.
  void AddDistribution(const ExpressionPtr& distr);

  /// Adds a CCF factor for the specified model.
  /// @param[in] factor A factor for the CCF model.
  /// @param[in] level The level of the passed factor.
  /// @todo Verify the level and the factor. Define the default level.
  void AddFactor(const ExpressionPtr& factor, int level);

  /// Validates the setup for the CCF model and group.
  /// The passed expressions must be checked for circular logic before
  /// initiating the CCF validation.
  /// @throws ValidationError if there is an issue with the setup.
  virtual void Validate();

 protected:
  /// Constructor to be used by derived classes.
  /// @param[in] name The name of a CCF group.
  /// @param[in] model CCF model of this group.
  CcfGroup(std::string name, std::string model);

  std::map<std::string, BasicEventPtr> members_;  ///< Members of CCF groups.
  ExpressionPtr distribution_;  ///< The probability distribution of the group.
  /// CCF factors for models to get CCF probabilities.
  std::vector<std::pair<int, ExpressionPtr> > factors_;

 private:
  /// Default constructor should not be used.
  /// All CCF models should be instantiated explicitly.
  CcfGroup();
  /// Restrict copy construction.
  CcfGroup(const CcfGroup&);
  /// Restrict copy assignment.
  CcfGroup& operator=(const CcfGroup&);

  std::string name_;  ///< The name of CCF group.
  std::string model_;  ///< Common cause model type.
};

/// @class BetaFactorModel
/// Common cause failure model that assumes, if common cause failure occurs,
/// then all components or members fail simultaneously or within short time.
class BetaFactorModel : public CcfGroup {
  /// Constructs the group and sets the model.
  /// @param[in] name The name for the group.
  BetaFactorModel(std::string name) : CcfGroup(name, "beta-factor") {}
};

/// @class MglModel
/// Multiple Greek Letters model characterizes failure of sub-groups of
/// the group due to common cause. The factor for k-component group defines
/// fraction of failure k or more members given that (k-1) members failed.
class MglModel : public CcfGroup {
  /// Constructs the group and sets the model.
  /// @param[in] name The name for the group.
  MglModel(std::string name) : CcfGroup(name, "MGL") {}
};

/// @class AlphaFactorModel
/// Alpha factor model characterizes failure of exactly k members of
/// the group due to common cause.
class AlphaFactorModel : public CcfGroup {
  /// Constructs the group and sets the model.
  /// @param[in] name The name for the group.
  AlphaFactorModel(std::string name) : CcfGroup(name, "alpha-factor") {}
};

/// @class PhiFactorModel
/// Phi factor model is a simplification, where fractions of k-member group
/// failure is given directly. Thus, Q_k = phi_k * Q_total.
/// This model is described in OpenPSA Model Exchange Format.
class PhiFactorModel : public CcfGroup {
 public:
  /// Constructs the group and sets the model.
  /// @param[in] name The name for the group.
  PhiFactorModel(std::string name) : CcfGroup(name, "phi-factor") {}

  /// In addition to the default validation of CcfGroup, checks if
  /// the given factors' sum is 1.
  /// @todo Problem with sampling the factors and not getting exactly 1.
  ///       Currently only accepts constant expressions.
  void Validate();
};

}  // namespace scram

#endif  // SCRAM_SRC_COMMON_CAUSE_GROUP_H_
