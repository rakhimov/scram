/// @file grapher.h
/// Graphing of analysis entities.
#ifndef SCRAM_GRAPHER_H_
#define SCRAM_GRAPHER_H_

#include <boost/shared_ptr.hpp>

#include "fault_tree.h"

typedef boost::shared_ptr<scram::FaultTree> FaultTreePtr;

namespace scram {

/// @class Grapher
/// Provides graphing instruction output to other tools.
/// Currently operate with Fault Trees only.
class Grapher {
 public:
  Grapher();

  /// Outputs a file with instructions for graphviz dot to create a fault tree.
  /// @note This function must be called only after initializing the tree.
  /// @note The name of the output file is the same as the input file, but
  /// the extensions are different.
  /// @throws Error if called before tree initialization from an input file.
  /// @throws IOError if the output file is not accessable.
  void GraphFaultTree(const FaultTreePtr& fault_tree,
                      const std::map<std::string, std::string>& orig_ids,
                      bool prob_requested = false,
                      std::string output = "");


 private:
  /// Graphs one top or intermediate event with children.
  /// @param[in] t The top or intermediate event.
  /// @param[in] pr_repeat The number of times a primary event is repeated.
  /// @param[in] out The output stream.
  /// @note The repetition information is important to avoid clashes.
  void GraphNode(GatePtr t, std::map<std::string, int>& pr_repeat,
                 std::map<std::string, int>& in_repeat, std::ofstream& out);

  /// @todo optimize and delete the following.
  /// Container of original names of events with capitalizations.
  std::map<std::string, std::string> orig_ids_;

  /// Top event.
  GatePtr top_event_;

  /// Holder for intermediate events.
  boost::unordered_map<std::string, GatePtr> inter_events_;

  /// Container for primary events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  /// Identify if the probability should be included.
  bool prob_requested_;
};

}  // namespace scram

#endif  // SCRAM_GRAPHER_H_
