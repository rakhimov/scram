/// @file risk_analysis.h
#ifndef SCRAM_RISK_ANALYISIS_H_
#define SCRAM_RISK_ANALYISIS_H_

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <queue>

#include <boost/serialization/map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "error.h"

#include "fault_tree.h"
#include "fault_tree_analysis.h"
#include "event.h"
#include "grapher.h"
#include "reporter.h"
#include "xml_parser.h"

class FaultTreeAnalysisTest;

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
  friend class ::FaultTreeAnalysisTest;

 public:
  /// This constructor with configurations with the analysis.
  /// @param[in] XML file with configurations for the analysis and output.
  RiskAnalysis(std::string config_file = "guess_yourself");

  /// Set the fault tree analysis.
  void fta(FaultTreeAnalysis* fta) { fta_ = fta; }

  /// Work with XML input file.
  void ProcessXml(std::string xml_file);

  /// Initializes the analysis from an input file.
  /// @param[in] input_file The input file.
  /// @todo Must deal with xml input file.
  /// @todo May have default configurations for analysis off all input files.
  ///
  /// Descriptions from original Fault Tree Class
  ///
  /// Reads input file with the structure of the Fault tree.
  /// Puts all events into their appropriate containers.
  /// @param[in] input_file The formatted input file.
  /// @throws ValidationError if input contains errors.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if the input file is not accessable.
  void ProcessInput(std::string input_file);

  /// Initializes probabilities relevant to the analysis.
  /// @param[in] prob_file The file with probability instructions.
  /// @todo Must be merged with the processing of the input file.
  ///
  /// Descriptions from original Fault Tree Class
  ///
  /// Reads probabilities for primary events from a formatted input file.
  /// Attaches probabilities to primary events.
  /// @param[in] prob_file The file with probability information.
  /// @throws Error if called before tree initialization from an input file.
  /// @throws ValidationError if input contains errors.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if the input file is not accessable.
  void PopulateProbabilities(std::string prob_file);

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

  ~RiskAnalysis() { delete fta_; }

 private:
  /// Gets arguments from a line in an input file formatted accordingly.
  /// Arguments vector will be flashed, and new contents will be inserted.
  /// @param[in] line The line containing arguments in lower case.
  /// @param[out] orig_line The original line with cases preserved.
  /// @param[out] args The arguments from the line.
  /// @returns false if there are no arguments from the line.
  /// @returns true if there are one or more arguments from the line.
  bool GetArgs(std::string& line, std::string& orig_line,
                std::vector<std::string>& args);

  /// Interpret arguments and perform specific actions on the tree.
  /// This should be performed only after GetArgs_ function has been called,
  /// and the arguments and original line have been processed.
  /// @param[in] nline The line number of input file.
  /// @param[in] msg The start for the error messages.
  /// @param[in] args The arguments to be interpreted.
  /// @param[in] orig_line The original line preserving cases.
  /// @throws ValidationError if the input or arguments are invalid.
  void InterpretArgs(int nline, std::stringstream& msg,
                      std::vector<std::string>& args,
                      std::string& orig_line);

  void DefineFaultTree(const xmlpp::Element* ft_node);

  void ProcessModelData(const xmlpp::Element* model_data);

  /// Adds node and updates databases of intermediate and primary events.
  /// @param[in] parent The id of the parent node.
  /// @param[in] id The id name of the node to be created.
  /// @param[in] type The symbol, type, or gate of the event to be added.
  /// @param[in] vote_number The vote number for the VOTE gate initialization.
  /// @throws ValidationError for invalid or incorrect inputs.
  void AddNode(std::string parent, std::string id, std::string type,
               int vote_number = -1);

  /// Adds probability to a primary event for p-model.
  /// @param[in] id The id name of the primary event.
  /// @param[in] p The probability for the primary event.
  /// @note If id is not in the tree, the probability is ignored.
  void AddProb(std::string id, double p);

  /// Adds probability to a primary event for l-model.
  /// @param[in] id The id name of the primary event.
  /// @param[in] p The probability for the primary event.
  /// @param[in] time The time to failure for this event.
  /// @note If id is not in the tree, the probability is ignored.
  void AddProb(std::string id, double p, double time);

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

  /// Container of original names of events with capitalizations.
  std::map<std::string, std::string> orig_ids_;

  /// List of all valid gates.
  std::set<std::string> gate_types_;

  /// List of all valid types of primary events.
  std::set<std::string> types_;

  /// Container for gates.
  boost::unordered_map<std::string, GatePtr> gates_;

  /// Container for primary events.
  /// @todo Consider deprecating this container for house and basic events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  /// Events to be defined.
  // boost::unordered_map<std::string, EventPtr> tbd_events_;

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
};

}  // namespace scram

#endif  // SCRAM_RISK_ANALYSIS_H_
