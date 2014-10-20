/// @file grapher.h
/// Graphing of analysis entities.
#ifndef SCRAM_SRC_GRAPHER_H_
#define SCRAM_SRC_GRAPHER_H_

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "fault_tree.h"

typedef boost::shared_ptr<scram::FaultTree> FaultTreePtr;

namespace scram {

/// @class Grapher
/// Provides graphing instruction output to other tools.
/// Currently operate with Fault Trees only.
class Grapher {
 public:
  /// Initializes gate and primary event colors.
  Grapher();

  /// Outputs a file with instructions for graphviz dot to create a fault tree.
  /// This function must be called only after initializing the tree.
  /// @param[in] fault_tree Fault Tree to draw.
  /// @param[in] prob_requested Should probabilities be included.
  /// @param[out] output Output destination.
  /// @throws IOError if the output file is not accessable.
  void GraphFaultTree(const FaultTreePtr& fault_tree,
                      bool prob_requested = false,
                      std::string output = "");

 private:
  /// Graphs one top or intermediate event with children.
  /// @param[in] t The top or intermediate event.
  /// @param[in] primary_events The container of primary events of the tree.
  /// @param[out] pr_repeat The number of times a primary event is repeated.
  /// @param[out] in_repeat The number of times an inter event is repeated.
  /// @param[out] out The output stream.
  /// @note The repetition information is important to avoid clashes.
  void GraphNode(
      const GatePtr& t,
      const boost::unordered_map<std::string, PrimaryEventPtr>& primary_events,
      std::map<std::string, int>* pr_repeat,
      std::map<std::string, int>* in_repeat,
      std::ofstream& out);

  /// Provides formatting information for top gate.
  /// @param[in] top_event The top event to be formatted.
  /// @param[out] out The output stream.
  void FormatTopEvent(const GatePtr& top_event, std::ofstream& out);

  /// Provides formatting information for each gate intermediate event.
  /// @param[in] inter_events The intermediate events to be formatted.
  /// @param[in] in_repeat The number of repetitions of intermediate gates.
  /// @param[out] out The output stream.
  void FormatIntermediateEvents(
      const boost::unordered_map<std::string, GatePtr>& inter_events,
      const std::map<std::string, int>& in_repeat,
      std::ofstream& out);

  /// Provides formatting information for each primary event.
  /// @param[in] primary_events The primary events to be formatted.
  /// @param[in] pr_repeat The number of repetitions of primary events.
  /// @param[in] prob_requested Indication to include probability numbers.
  /// @param[out] out The output stream.
  void FormatPrimaryEvents(
      const boost::unordered_map<std::string, PrimaryEventPtr>& primary_events,
      const std::map<std::string, int>& pr_repeat,
      bool prob_requested,
      std::ofstream& out);

  /// Gate colors.
  std::map<std::string, std::string> gate_colors_;
  /// Primary event colors.
  std::map<std::string, std::string> event_colors_;
};

}  // namespace scram

#endif  // SCRAM_SRC_GRAPHER_H_
