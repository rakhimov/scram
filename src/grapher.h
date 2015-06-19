/// @file grapher.h
/// Graphing of analysis entities.
#ifndef SCRAM_SRC_GRAPHER_H_
#define SCRAM_SRC_GRAPHER_H_

#include <iostream>
#include <map>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "event.h"

namespace scram {

/// @class Grapher
/// Provides graphing instruction output to other tools.
/// Currently operate with Fault Trees only.
class Grapher {
 public:
  typedef boost::shared_ptr<Gate> GatePtr;

  /// Outputs instructions for graphviz dot to create a fault tree.
  /// This function must be called only with fully initialized fault tree.
  /// @param[in] top_event The root node for the fault tree to draw.
  /// @param[in] prob_requested Should probabilities be included.
  /// @param[out] out The output stream.
  void GraphFaultTree(const GatePtr& top_event, bool prob_requested,
                      std::ostream& out);

 private:
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<Event> EventPtr;

  /// Graphs one gate with children.
  /// @param[in] gate The gate to be graphed.
  /// @param[out] node_repeat The number of times a node is repeated.
  /// @param[out] out The output stream.
  /// @note The repetition information is important to avoid clashes.
  void GraphGate(const GatePtr& gate,
                 boost::unordered_map<std::string, int>* node_repeat,
                 std::ostream& out);

  /// Provides formatting information for top gate.
  /// @param[in] top_event The top event to be formatted.
  /// @param[out] out The output stream.
  void FormatTopEvent(const GatePtr& top_event, std::ostream& out);

  /// Provides formatting information for each gate intermediate event.
  /// @param[in] inter_events The intermediate events to be formatted.
  /// @param[in] node_repeat The number of times a node is repeated.
  /// @param[out] out The output stream.
  void FormatIntermediateEvents(
      const boost::unordered_map<std::string, GatePtr>& inter_events,
      const boost::unordered_map<std::string, int>& node_repeat,
      std::ostream& out);

  /// Provides formatting information for each primary event.
  /// @param[in] primary_events The primary events to be formatted.
  /// @param[in] node_repeat The number of times a node is repeated.
  /// @param[in] prob_requested Indication to include probability numbers.
  /// @param[out] out The output stream.
  void FormatPrimaryEvents(
      const boost::unordered_map<std::string, PrimaryEventPtr>& primary_events,
      const boost::unordered_map<std::string, int>& node_repeat,
      bool prob_requested,
      std::ostream& out);

  static std::map<std::string, std::string> gate_colors_;  ///< Gate colors.
  static std::map<std::string, std::string> event_colors_;  ///< Event colors.
};

}  // namespace scram

#endif  // SCRAM_SRC_GRAPHER_H_
