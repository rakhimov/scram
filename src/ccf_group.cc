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
  assert(level >= 0);
  factors_.push_back(std::make_pair(level, factor));
}

void CcfGroup::Validate() {
  if (distribution_->Min() < 0 || distribution_->Max() > 1) {
    throw ValidationError("Distribution for " + name_ + " CCF group" +
                          " has illegal values.");
  }
  std::vector<std::pair<int, ExpressionPtr> >::iterator it;
  for (it = factors_.begin(); it != factors_.end(); ++it) {
    if (it->second->Max() > 1 || it->second->Min() < 0) {
      throw ValidationError("Factors for " + name_ + " CCF group" +
                            " have illegal values.");
    }
  }
}

}  // namespace scram
