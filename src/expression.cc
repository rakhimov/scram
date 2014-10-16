/// @file expression.cc
/// Implementation of various expressions for basic event probability
/// description.
#include "expression.h"

#include <algorithm>

#include "error.h"

typedef boost::shared_ptr<scram::Parameter> ParameterPtr;

namespace scram {

void Parameter::CheckCyclicity() {
  std::vector<std::string> path;
  CheckCyclicity(&path);
}

void Parameter::CheckCyclicity(std::vector<std::string>* path) {
  std::vector<std::string>::iterator it = std::find(path->begin(),
                                                    path->end(),
                                                    name_);
  if (it != path->end()) {
    std::string msg = "Detected a cyclicity through '" + name_ +
                      "' parameter:\n";
    msg += name_;
    for (++it; it != path->end(); ++it) {
      msg += "->" + *it;
    }
    msg += "->" + name_;
    throw ValidationError(msg);
  }
  path->push_back(name_);
  ParameterPtr ptr = boost::dynamic_pointer_cast<Parameter>(expression_);
  if (ptr) ptr->CheckCyclicity(path);
}

}  // namespace scram
