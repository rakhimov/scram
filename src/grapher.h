/// @file grapher.h
/// Graphing of analysis entities.
#ifndef SCRAM_SRC_GRAPHER_H_
#define SCRAM_SRC_GRAPHER_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

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
  typedef boost::shared_ptr<Event> EventPtr;
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Graphs one formula with arguments.
  /// @param[in] formula_name Unique name for the formula.
  /// @param[in] formula The formula to be graphed.
  /// @param[out] formulas The container with registered nested formulas.
  /// @param[out] node_repeat The number of times a node is repeated.
  /// @param[out] out The output stream.
  /// @note The repetition information is important to avoid clashes.
  void GraphFormula(const std::string& formula_name,
                    const FormulaPtr& formula,
                    std::vector<std::pair<std::string, FormulaPtr> >* formulas,
                    boost::unordered_map<EventPtr, int>* node_repeat,
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
      const boost::unordered_map<EventPtr, int>& node_repeat,
      std::ostream& out);

  /// Provides formatting information for basic events.
  /// @param[in] basic_events The basic events to be formatted.
  /// @param[in] node_repeat The number of times a node is repeated.
  /// @param[in] prob_requested Indication to include probability numbers.
  /// @param[out] out The output stream.
  void FormatBasicEvents(
      const boost::unordered_map<std::string, BasicEventPtr>& basic_events,
      const boost::unordered_map<EventPtr, int>& node_repeat,
      bool prob_requested,
      std::ostream& out);

  /// Provides formatting information for house events.
  /// @param[in] house_events The house events to be formatted.
  /// @param[in] node_repeat The number of times a node is repeated.
  /// @param[in] prob_requested Indication to include probability numbers.
  /// @param[out] out The output stream.
  void FormatHouseEvents(
      const boost::unordered_map<std::string, HouseEventPtr>& house_events,
      const boost::unordered_map<EventPtr, int>& node_repeat,
      bool prob_requested,
      std::ostream& out);

  /// Provides formatting information for each primary event.
  /// @param[in] primary_event The primary event to be formatted.
  /// @param[in] repetition The repetition number of the node.
  /// @param[in] prob_msg Probability information message.
  /// @param[out] out The output stream.
  void FormatPrimaryEvent(const PrimaryEventPtr& primary_event,
                          int repetition,
                          std::string prob_msg,
                          std::ostream& out);

  /// Format formulas gathered from nested formulas of gate descriptions.
  /// The name is empty for these formulas. Formulas are expected to be unique.
  /// @param[in] formulas The container with registered nested formulas.
  /// @param[out] out The output stream.
  void FormatFormulas(
      const std::vector<std::pair<std::string, FormulaPtr> >& formulas,
      std::ostream& out);

  static std::map<std::string, std::string> gate_colors_;  ///< Gate colors.
  static std::map<std::string, std::string> event_colors_;  ///< Event colors.
};

}  // namespace scram

#endif  // SCRAM_SRC_GRAPHER_H_
