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
class CcfGroup : Element {
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
  std::map<std::string, BasicEventPtr> members_;  ///< Members of CCF groups.
  ExpressionPtr distribution_;  ///< The probability distribution of the group.
  /// CCF factors for models to get CCF probabilities.
  std::vector<std::pair<int, ExpressionPtr> > factors_;
};

}  // namespace scram

#endif  // SCRAM_SRC_COMMON_CAUSE_GROUP_H_
