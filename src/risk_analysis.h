/// @file risk_analysis.h
#ifndef SCRAM_RISK_ANALYISIS_H_
#define SCRAM_RISK_ANALYISIS_H_

#include <map>
#include <set>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "env.h"
#include "error.h"
#include "event.h"
#include "fault_tree.h"
#include "fault_tree_analysis.h"
#include "grapher.h"
#include "reporter.h"
#include "xml_parser.h"

class RiskAnalysisTest;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;
typedef boost::shared_ptr<scram::BasicEvent> BasicEventPtr;
typedef boost::shared_ptr<scram::HouseEvent> HouseEventPtr;

typedef boost::shared_ptr<scram::FaultTree> FaultTreePtr;

namespace scram {

/// @class RiskAnalysis
/// Main system that performs analyses.
class RiskAnalysis {
  friend class ::RiskAnalysisTest;

 public:
  /// This constructor with configurations with the analysis.
  /// @param[in] XML file with configurations for the analysis and output.
  /// @todo Should be able to accept configurations from XML files.
  RiskAnalysis(std::string config_file = "guess_yourself");

  /// Set the fault tree analysis.
  void fta(FaultTreeAnalysis* fta) { fta_ = fta; }

  /// Reads input file with the structure of analysis entities.
  /// Initializes the analysis from the given input file.
  /// Puts all events into their appropriate containers.
  /// @param[in] xml_file The formatted xml input file.
  /// @throws ValidationError if input contains errors.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if the input file is not accessable.
  /// @todo May have default configurations for analysis off all input files.
  /// @todo Should be able to deal with multiple files.
  void ProcessInput(std::string xml_file);

  /// Graphing or other visual resources for the analysis if applicable.
  /// @todod Must be handled by a separate class.
  ///
  /// Descriptions from original Fault Tree Class
  ///
  /// Outputs a file with instructions for graphviz dot to create a fault tree.
  /// @note This function must be called only after initializing the tree.
  /// @note The name of the output file is the same as the input file, but
  /// the extensions are different.
  /// @throws Error if called before tree initialization from an input file.
  /// @throws IOError if the output file is not accessable.
  void GraphingInstructions();

  /// Perform the main analysis operations.
  /// @todo Must use specific analyzers for this operation.
  ///
  /// Descriptions from original Fault Tree Class
  ///
  /// Analyzes the fault tree and performs computations.
  /// This function must be called only after initilizing the tree with or
  /// without its probabilities.
  /// @throws Error if called before tree initialization from an input file.
  /// @note Cut set generator: O_avg(N) O_max(N)
  void Analyze();

  /// Reports the results of analysis.
  /// @param[out] output The output destination.
  /// @todo Must rely upon another dedicated class.
  ///
  /// Descriptions from original Fault Tree Class
  ///
  /// Reports the results of analysis to a specified output destination.
  /// @note This function must be called only after Analyze() function.
  /// param[out] output The output destination.
  /// @throws Error if called before the tree analysis.
  /// @throws IOError if the output file is not accessable.
  void Report(std::string output);

  ~RiskAnalysis() {
    delete fta_;
    delete env_;
  }

 private:
  void DefineFaultTree(const xmlpp::Element* ft_node);

  void DefineGate(const xmlpp::Element* gate_node, FaultTreePtr& ft);
  void DefineBasicEvent(const xmlpp::Element* event_node, FaultTreePtr& ft);
  void DefineHouseEvent(const xmlpp::Element* event_node, FaultTreePtr& ft);

  void ProcessModelData(const xmlpp::Element* model_data);

  /// Verifies if gates are initialized correctly.
  /// @returns A warning message with a list of all bad gates with problems.
  /// @note An empty string for no problems detected.
  std::string CheckAllGates();

  /// Checks if a gate is initialized correctly.
  /// @returns A warning message with the problem description.
  /// @note An empty string for no problems detected.
  std::string CheckGate(const GatePtr& event);

  /// @returns Primary events that do not have probabilities assigned.
  /// @note An empty string for no problems detected.
  std::string PrimariesNoProb();

  /// @todo Containers for fault trees, events, event trees, CCF, and other
  /// analysis entities.
  /// @deprecated l-model analysis

  /// Container of original names of events with capitalizations.
  std::map<std::string, std::string> orig_ids_;

  /// List of all valid gates.
  std::set<std::string> gate_types_;

  /// List of all valid types of primary events.
  std::set<std::string> types_;

  /// Container for gates.
  boost::unordered_map<std::string, GatePtr> gates_;

  /// Container for primary events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  /// Events to be defined with their parents saved for later.
  boost::unordered_map<std::string, std::vector<GatePtr> > tbd_events_;

  /// Gates to be defined.
  boost::unordered_map<std::string, GatePtr> tbd_gates_;

  /// Basic events to be defined.
  boost::unordered_map<std::string, BasicEventPtr> tbd_basic_events_;

  /// House events to be defined.
  boost::unordered_map<std::string, HouseEventPtr> tbd_house_events_;

  /// Container for all events.
  boost::unordered_map<std::string, EventPtr> all_events_;

  /// A fault tree.
  FaultTreePtr fault_tree_;

  /// A fault tree analysis;
  FaultTreeAnalysis* fta_;

  // Specific variables that are shared for initialization of tree nodes.
  std::string parent_;  ///< The parent id.
  std::string id_;  ///< The id of the node.
  std::string type_;  ///< The type of the node.
  int vote_number_;  ///< The vote number for the VOTE gate.

  /// Input file path.
  std::string input_file_;

  /// Indicator if probability calculations are requested.
  bool prob_requested_;

  /// Environment information provider.
  Env* env_;
};

}  // namespace scram

#endif  // SCRAM_RISK_ANALYSIS_H_
