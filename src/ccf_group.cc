/// @file ccf_group.cc
/// Implementation of various common cause failure models.
#include "ccf_group.h"

namespace scram {

CcfGroup::CcfGroup(std::string name, std::string model)
    : name_(name),
      model_(model) {}

void CcfGroup::AddMember(const BasicEventPtr& basic_event) {
  if (distribution_) {
    throw IllegalOperation("No more members accepted. The distribution for " +
                           name_ + " CCF group has already been defined.");

  } else if (members_.count(basic_event->id())) {
    throw LogicError("Basic event " + basic_event->orig_id() + " is already" +
                     " in " + name_ + " CCF group.");
  }
  members_.insert(std::make_pair(basic_event->id(), basic_event));
}

void CcfGroup::AddDistribution(const ExpressionPtr& distr) {
  assert(!distribution_);
  distribution_ = distr;
  // Define probabilities of all basic events.
  std::map<std::string, BasicEventPtr>::iterator it;
  for (it = members_.begin(); it != members_.end(); ++it) {
    it->second->expression(distribution_);
  }
}

void CcfGroup::AddFactor(const ExpressionPtr& factor, int level) {
  assert(level > 1);
  if (factors_.empty()) assert(level == 2);
  if (!factors_.empty()) assert(level == factors_.back().first + 1);
  factors_.push_back(std::make_pair(level, factor));
}

void CcfGroup::Validate() {
  if (members_.size() < 2) {
    throw ValidationError(name_ + " CCF group must have at least 2 members.");
  }

  if (distribution_->Min() < 0 || distribution_->Max() > 1) {
    throw ValidationError("Distribution for " + name_ + " CCF group" +
                          " has illegal values.");
  }
  if (factors_.size() > members_.size() - 1) {
    throw ValidationError("The number of factors for " + name_ + " CCF group" +
                          " cannot be more than (# of members - 1).");
  }
  std::vector<std::pair<int, ExpressionPtr> >::iterator it;
  for (it = factors_.begin(); it != factors_.end(); ++it) {
    if (it->second->Max() > 1 || it->second->Min() < 0) {
      throw ValidationError("Factors for " + name_ + " CCF group" +
                            " have illegal values.");
    }
  }
}

void BetaFactorModel::Validate() {
  if (factors_.size() != 1) {
    throw ValidationError("Beta-Factor Model " + this->name() + " CCF group" +
                          " must have exactly one factor.");
  }
  CcfGroup::Validate();
}

void BetaFactorModel::ApplyModel() {
  std::string common_name = "[";  // Event name for common failure group.
  std::string common_id = "[";  // Event id for common failure group.

  std::vector<ExpressionPtr> args;  // For expression arguments.

  // Getting the probability equation for independent events.
  assert(CcfGroup::factors_.size() == 1);
  ExpressionPtr one(new ConstantExpression(1.0));
  args.push_back(one);
  ExpressionPtr beta = CcfGroup::factors_.begin()->second;
  args.push_back(beta);
  ExpressionPtr indep_factor(new Sub(args));  // (1 - beta)
  args.clear();
  args.push_back(indep_factor);
  args.push_back(CcfGroup::distribution_);
  ExpressionPtr indep_prob(new Mul(args));  // (1 - beta) * Q

  std::map<std::string, BasicEventPtr>::const_iterator it;
  for (it = CcfGroup::members_.begin(); it != CcfGroup::members_.end();) {
    // Create indipendent events.
    std::string independent_orig_id = "[" + it->second->orig_id() + "]";
    std::string independent_id = "[" + it->second->id() + "]";

    BasicEventPtr independent(new BasicEvent(independent_id));
    independent->orig_id(independent_orig_id);
    independent->expression(indep_prob);
    CcfGroup::new_events_.push_back(independent);

    GatePtr replacement(new Gate(it->first, "or"));
    CcfGroup::gates_.insert(std::make_pair(it->first, replacement));
    it->second->ccf_gate(replacement);
    replacement->AddChild(independent);

    common_name += it->second->orig_id();
    common_id += it->second->id();
    ++it;
    if (it != CcfGroup::members_.end()) {
      common_name += " ";
      common_id += " ";
    }
  }
  common_id += "]";
  common_name += "]";
  BasicEventPtr common_failure(new BasicEvent(common_id));
  common_failure->orig_id(common_name);
  args.clear();
  args.push_back(beta);
  args.push_back(CcfGroup::distribution_);
  common_failure->expression(ExpressionPtr(new Mul(args)));
  CcfGroup::new_events_.push_back(common_failure);

  std::map<std::string, GatePtr>::iterator it_g;
  for (it_g = CcfGroup::gates_.begin(); it_g != CcfGroup::gates_.end();
       ++it_g) {
    it_g->second->AddChild(common_failure);
  }
}

void MglModel::ApplyModel() {

}

void AlphaFactorModel::ApplyModel() {

}

void PhiFactorModel::Validate() {
  CcfGroup::Validate();
  int sum = 0;
  std::vector<std::pair<int, ExpressionPtr> >::iterator it;
  for (it = CcfGroup::factors_.begin(); it != CcfGroup::factors_.end(); ++it) {
    if (!it->second->IsConstant()) {
      throw ValidationError("Phi Factor Model " + CcfGroup::name() +
                            " CCF group accepts only constant expressions.");

    }
    sum += it->second->Mean();
  }
  /// @todo Problems with floating point number comparison.
  if (sum != 1.0) {
    throw ValidationError("The factors for Phi model " + CcfGroup::name() +
                          " CCF group must sum to 1.");
  }
}

void PhiFactorModel::ApplyModel() {

}

}  // namespace scram
