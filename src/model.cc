/// @file model.cc
/// Implementation of functions in Model class.
#include "model.h"

#include "ccf_group.h"
#include "fault_tree.h"

#include <boost/algorithm/string.hpp>

namespace scram {

Model::Model(std::string name) : name_(name) {}

void Model::AddFaultTree(const FaultTreePtr& fault_tree) {
  std::string name = fault_tree->name();
  boost::to_lower(name);

  if (fault_trees_.count(name)) {
    std::string msg = "Fault tree " + fault_tree->name() + " already exists.";
    throw ValidationError(msg);
  }
  fault_trees_.insert(std::make_pair(name, fault_tree));
}

void Model::AddParameter(const ParameterPtr& parameter) {
  std::string name = parameter->name();
  boost::to_lower(name);

  if (parameters_.count(name)) {
    std::string msg = "Parameter " + parameter->name() + " already exists.";
    throw ValidationError(msg);
  }
  parameters_.insert(std::make_pair(name, parameter));
}

void Model::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->name();
  boost::to_lower(name);

  if (ccf_groups_.count(name)) {
    std::string msg = "CCF group " + ccf_group->name() + " already exists.";
    throw ValidationError(msg);
  }
  ccf_groups_.insert(std::make_pair(name, ccf_group));
}

}  // namespace scram
