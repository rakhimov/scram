/// @file fault_tree.h
/// Fault Tree Analysis.
#ifndef SCRAM_FAULT_TREE_H_
#define SCRAM_FAULT_TREE_H_

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <queue>

#include <boost/serialization/map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "error.h"
#include "event.h"
#include "risk_analysis.h"
#include "superset.h"

class FaultTreeTest;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::TopEvent> TopEventPtr;
typedef boost::shared_ptr<scram::InterEvent> InterEventPtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

typedef boost::shared_ptr<scram::Superset> SupersetPtr;

namespace scram {

/// @class FaultTree
/// Fault tree analysis functionality.
class FaultTree : public RiskAnalysis {
  friend class ::FaultTreeTest;

 public:
  /// The main constructor of the Fault Tree Analysis.
  /// @param[in] analysis The type of fault tree analysis.
  /// @param[in] graph_only Indicates if only graph is requested.
  /// @param[in] approx The kind of approximation for probability calculations.
  /// @param[in] limit_order The maximum limit on minimal cut sets' order.
  /// @param[in] nsums The number of sums in the probability series.
  /// @throws ValueError if any of the parameters are invalid.
  FaultTree(std::string analysis, bool graph_only, std::string approx = "no",
            int limit_order = 20, int nsums = 1000000);

  /// Reads input file with the structure of the Fault tree.
  /// Puts all events into their appropriate containers.
  /// @param[in] input_file The formatted input file.
  /// @throws ValidationError if input contains errors.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if the input file is not accessable.
  void ProcessInput(std::string input_file);

  /// Reads probabilities for primary events from a formatted input file.
  /// Attaches probabilities to primary events.
  /// @param[in] prob_file The file with probability information.
  /// @throws Error if called before tree initialization from an input file.
  /// @throws ValidationError if input contains errors.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if the input file is not accessable.
  void PopulateProbabilities(std::string prob_file);

  /// Outputs a file with instructions for graphviz dot to create a fault tree.
  /// @note This function must be called only after initializing the tree.
  /// @note The name of the output file is the same as the input file, but
  /// the extensions are different.
  /// @throws Error if called before tree initialization from an input file.
  /// @throws IOError if the output file is not accessable.
  void GraphingInstructions();

  /// Analyzes the fault tree and performs computations.
  /// This function must be called only after initilizing the tree with or
  /// without its probabilities.
  /// @throws Error if called before tree initialization from an input file.
  void Analyze();

  /// Reports the results of analysis to a specified output destination.
  /// @note This function must be called only after Analyze() function.
  /// param[out] output The output destination.
  /// @throws Error if called before the tree analysis.
  /// @throws IOError if the output file is not accessable.
  void Report(std::string output);

  virtual ~FaultTree() {}

 private:
  /// Gets arguments from a line in an input file formatted accordingly.
  /// Arguments vector will be flashed, and new contents will be inserted.
  /// @param[in] line The line containing arguments in lower case.
  /// @param[out] orig_line The original line with cases preserved.
  /// @param[out] args The arguments from the line.
  /// @returns false if there are no arguments from the line.
  /// @returns true if there are one or more arguments from the line.
  bool GetArgs_(std::string& line, std::string& orig_line,
                std::vector<std::string>& args);

  /// Interpret arguments and perform specific actions on the tree.
  /// This should be performed only after GetArgs_ function has been called,
  /// and the arguments and original line have been processed.
  /// @param[in] nline The line number of input file.
  /// @param[in] msg The start for the error messages.
  /// @param[in] args The arguments to be interpreted.
  /// @param[in] orig_line The original line preserving cases.
  /// @param[in] tr_parent The parent of the TransferIn for sub-trees.
  /// @param[in] tr_id The TransferIn/Out id name.
  /// @param[in] suffix The suffix that is attached to the new node id.
  /// @throws ValidationError if the input or arguments are invalid.
  void InterpretArgs_(int nline, std::stringstream& msg,
                      std::vector<std::string>& args,
                      std::string& orig_line,
                      std::string tr_parent = "",
                      std::string tr_id = "",
                      std::string suffix = "");

  /// Adds node and updates databases of intermediate and primary events.
  /// @param[in] parent The id of the parent node.
  /// @param[in] id The id name of the node to be created.
  /// @param[in] type The symbol, type, or gate of the event to be added.
  /// @param[in] vote_number The vote number for the VOTE gate initialization.
  /// @throws ValidationError for invalid or incorrect inputs.
  void AddNode_(std::string parent, std::string id, std::string type,
                int vote_number = -1);

  /// Adds probability to a primary event for p-model.
  /// @param[in] id The id name of the primary event.
  /// @param[in] p The probability for the primary event.
  /// @note If id is not in the tree, the probability is ignored.
  void AddProb_(std::string id, double p);

  /// Adds probability to a primary event for l-model.
  /// @param[in] id The id name of the primary event.
  /// @param[in] p The probability for the primary event.
  /// @param[in] time The time to failure for this event.
  /// @note If id is not in the tree, the probability is ignored.
  void AddProb_(std::string id, double p, double time);

  /// Includes external transfer in subtrees to this current main tree.
  void IncludeTransfers_();

  /// Verifies if gates are initialized correctly.
  /// @returns A warning message with a list of all bad gates with problems.
  /// @note An empty string for no problems detected.
  std::string CheckAllGates_();

  /// Checks if a gate is initialized correctly.
  /// @returns A warning message with the problem description.
  /// @note An empty string for no problems detected.
  std::string CheckGate_(const TopEventPtr& event);

  /// @returns Primary events that do not have probabilities assigned.
  /// @note An empty string for no problems detected.
  std::string PrimariesNoProb_();

  /// Graphs one top or intermediate event with children.
  /// @param[in] t The top or intermediate event.
  /// @param[in] pr_repeat The number of times a primary event is repeated.
  /// @param[in] out The output stream.
  /// @note The repetition information is important to avoid clashes.
  void GraphNode_(TopEventPtr t, std::map<std::string, int>& pr_repeat,
                  std::ofstream& out);

  /// Expands the children of a top or intermediate event to Supersets.
  /// @param[in] inter_index The index number of the parent node.
  /// @param[out] sets The final Supersets from the children.
  /// @throws ValueError if the parent's gate is not recognized.
  /// @note The final sets are dependent on the gate of the parent.
  void ExpandSets_(int inter_index, std::vector<SupersetPtr>& sets);

  /// Expands sets for OR operator.
  /// @param[in] events_children The indices of the children of the event.
  /// @param[out] sets The final Supersets generated for OR operator.
  /// @param[in] mult The positive or negative event indicator.
  void SetOr_(std::vector<int>& events_children,
              std::vector<SupersetPtr>& sets, int mult = 1);

  /// Expands sets for AND operator.
  /// @param[in] events_children The indices of the children of the event.
  /// @param[out] sets The final Supersets generated for OR operator.
  /// @param[in] mult The positive or negative event indicator.
  void SetAnd_(std::vector<int>& events_children,
               std::vector<SupersetPtr>& sets, int mult = 1);

  // -------------------- Algorithm for Cut Sets and Probabilities -----------
  /// Assigns an index to each primary event, and then populates with this
  /// indices new databases of minimal cut sets and primary to integer
  /// converting maps.
  void AssignIndexes_();

  /// Converts minimal cut sets from indices to strings.
  void SetsToString_();

  /// Calculates a probability of a set of minimal cut sets, which are in OR
  /// relationship with each other. This function is a brute force probability
  /// calculation without approximations.
  /// @param[in] min_cut_sets Sets of indices of primary events.
  /// @param[in] nsums The number of sums in the series.
  /// @returns The total probability.
  /// @note This function drastically modifies min_cut_sets by deleting
  /// sets inside it. This is for better performance.
  double ProbOr_(std::set< std::set<int> >& min_cut_sets,
                 int nsums = 1000000);

  /// Calculates a probability of a minimal cut set, whose members are in AND
  /// relationship with each other. This function assumes independence of each
  /// member.
  /// @param[in] min_cut_set A set of indices of primary events.
  /// @returns The total probability.
  double ProbAnd_(const std::set<int>& min_cut_set);

  /// Calculates A(and)( B(or)C ) relationship for sets using set algebra.
  /// @param[in] el A set of indices of primary events.
  /// @param[in] set Sets of indices of primary events.
  /// @param[out] combo_set A final set resulting from joining el and sets.
  void CombineElAndSet_(const std::set<int>& el,
                        const std::set< std::set<int> >& set,
                        std::set< std::set<int> >& combo_set);

  std::set< std::set<int> > imcs_;  ///< Min cut sets with indices of events.
  /// Indices min cut sets to strings min cut sets mapping.
  std::map< std::set<int>, std::set<std::string> > imcs_to_smcs_;

  std::vector<PrimaryEventPtr> int_to_prime_;  ///< Primary events from indices.
  /// Indices of primary events.
  boost::unordered_map<std::string, int> prime_to_int_;
  std::vector<double> iprobs_;  ///< Holds probabilities of primary events.

  int top_event_index_;  ///< The index of the top event.
  /// Intermediate events from indices.
  boost::unordered_map<int, TopEventPtr> int_to_inter_;
  /// Indices of intermediate events.
  boost::unordered_map<std::string, int> inter_to_int_;
  // -----------------------------------------------------------------
  // ---- Algorithm for Equation Construction for Monte Carlo Sim -------
  /// Generates positive and negative terms of probability equation expansion.
  /// @param[in] min_cut_sets Sets of indices of primary events.
  /// @param[in] sign The sign of the series.
  /// @param[in] nsums The number of sums in the series.
  void MProbOr_(std::set< std::set<int> >& min_cut_sets, int sign = 1,
                int nsums = 1000000);

  /// Performs Monte Carlo Simulation.
  /// @todo Implement the simulation.
  void MSample_();

  std::vector< std::set<int> > pos_terms_;  ///< Plus terms of the equation.
  std::vector< std::set<int> > neg_terms_;  ///< Minus terms of the equation.
  std::vector<double> sampled_results_;  ///< Storage for sampled values.
  // -----------------------------------------------------------------
  // ----------------------- Member Variables of this Class -----------------
  // Specific variables that are shared for initialization of tree nodes.
  std::string parent_;  ///< The parent id.
  std::string id_;  ///< The id of the node.
  std::string type_;  ///< The type of the node.
  int vote_number_;  ///< The vote number for the VOTE gate.
  bool block_started_;  ///< Indicator of a start of a new block of a node.

  /// This member is used to provide any warnings about assumptions,
  /// calculations, and settings. These warnings must be written into output
  /// file.
  std::string warnings_;

  /// Type of analysis to be performed.
  std::string analysis_;

  /// Indicator of request for graphing instructions only.
  bool graph_only_;

  /// Approximations for probability calculations.
  std::string approx_;

  /// Input file path.
  std::string input_file_;

  /// Keep track of currently opened file with sub-trees.
  std::string current_file_;

  /// Indicator if probability calculations are requested.
  bool prob_requested_;

  /// Indicate if analysis of the tree is done.
  bool analysis_done_;

  /// Number of sums in series expansion for probability calculations.
  int nsums_;

  /// Container of original names of events with capitalizations.
  std::map<std::string, std::string> orig_ids_;

  /// List of all valid gates.
  std::set<std::string> gates_;

  /// List of all valid types of primary events.
  std::set<std::string> types_;

  /// Id of a top event.
  std::string top_event_id_;

  /// Top event.
  TopEventPtr top_event_;

  /// Indicator of detection of a top event described by a transfer sub-tree.
  bool top_detected_;

  /// Indicates that reading the main tree file as opposed to a transfer tree.
  bool is_main_;

  /// Holder for intermediate events.
  boost::unordered_map<std::string, InterEventPtr> inter_events_;

  /// Container for primary events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  /// Container for transfer symbols as requested in tree initialization.
  /// A queue contains a tuple of the parent and id of transferIn.
  std::queue< std::pair<std::string, std::string> > transfers_;

  /// For graphing purposes the same transferIn.
  std::multimap<std::string, std::string> transfer_map_;

  /// Container for storing all transfer sub-trees' names and number of calls.
  std::map<std::string, int> trans_calls_;

  /// Container to track transfer calls to prevent cyclic calls/inclusions.
  std::map< std::string, std::vector<std::string> > trans_tree_;

  /// Indicate if TransferOut is initiated correctly.
  bool transfer_correct_;

  /// Indication of the first intermediate event of the transfer.
  bool transfer_first_inter_;

  /// Sub-tree analysis only.
  bool sub_tree_analysis_;

  /// Container for minimal cut sets.
  std::set< std::set<std::string> > min_cut_sets_;

  /// Container for minimal cut sets and their respective probabilities.
  std::map< std::set<std::string>, double > prob_of_min_sets_;

  /// Container for minimal cut sets ordered by their probabilities.
  std::multimap< double, std::set<std::string> > ordered_min_sets_;

  /// Container for primary events and their contribution.
  std::map< std::string, double > imp_of_primaries_;

  /// Container for primary events ordered by their contribution.
  std::multimap< double, std::string > ordered_primaries_;

  /// Maximum order of the minimal cut sets.
  int max_order_;

  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;

  /// Total probability of the top event.
  double p_total_;
};

}  // namespace scram

#endif  // SCRAM_FAULT_TREE_H_
