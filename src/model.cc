/// @file model.cc
/// Implementation of functions in Model class.
#include "model.h"

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

}  // namespace scram
