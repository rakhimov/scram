/// @file model.h
/// Representation for a model container for risk analysis.
#ifndef SCRAM_SRC_MODEL_H_
#define SCRAM_SRC_MODEL_H_

#include <map>
#include <string>

#include <boost/unordered_map.hpp>

#include "element.h"
#include "event.h"

namespace scram {

class CcfGroup;
class Parameter;
class FaultTree;

/// @class Model
/// This class represents a model that is defined in one input file.
class Model : public Element {
 public:
  typedef boost::shared_ptr<FaultTree> FaultTreePtr;

  /// Creates a model container.
  /// @param[in] name The optional name for the model.
  explicit Model(std::string name = "");

  /// @returns The name of the model.
  inline std::string name() const { return name_; }

  /// Adds a fault tree into the model container.
  /// @param[in] fault_tree A fault tree defined in this model.
  /// @throws ValidationError if a container with the same name already exists.
  void AddFaultTree(const FaultTreePtr& fault_tree);

  /// @returns Defined fault trees in the model.
  inline const std::map<std::string, FaultTreePtr>& fault_trees() const {
    return fault_trees_;
  }

 private:
  typedef boost::shared_ptr<Parameter> ParameterPtr;
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<CcfGroup> CcfGroupPtr;

  std::string name_;  ///< The name of the model.

  /// Container for fully defined gates.
  boost::unordered_map<std::string, GatePtr> gates_;

  /// Container for fully defined house events.
  boost::unordered_map<std::string, HouseEventPtr> house_events_;

  /// Container for fully defined basic events.
  boost::unordered_map<std::string, BasicEventPtr> basic_events_;

  /// Container for defined parameters or variables.
  boost::unordered_map<std::string, ParameterPtr> parameters_;

  /// A collection of fault trees.
  std::map<std::string, FaultTreePtr> fault_trees_;

  /// A collection of common cause failure groups.
  std::map<std::string, CcfGroupPtr> ccf_groups_;
};

}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
