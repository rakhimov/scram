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

// Fault tree analysis.
class FaultTree : public RiskAnalysis {
  friend class ::FaultTreeTest;

 public:
  FaultTree(std::string analysis, bool graph_only, std::string approx = "no",
            int limit_order = 20, int nsums = 1000000);

  // Reads input file with the structure of the Fault tree.
  // Puts all events into their appropriate containers.
  void ProcessInput(std::string input_file);

  // Reads probabilities for primary events from a formatted input file.
  // Attaches probabilities to primary events.
  void PopulateProbabilities(std::string prob_file);

  // Outputs a file with instructions for graphviz dot to create a fault tree.
  // This function must be called only after initializing the tree.
  void GraphingInstructions();

  // Analyzes the fault tree and performs computations.
  // This function must be called only after initilizing the tree with or
  // without its probabilities.
  void Analyze();

  // Reports the results of analysis to a specified output destination.
  // This function must be called only after Analyze() function.
  void Report(std::string output);

  virtual ~FaultTree() {}

 private:
  // Gets arguments from a line in an input file formatted accordingly.
  // Arguments vector will be flashed, and new contents will be inserted.
  bool GetArgs_(std::vector<std::string>& args, std::string& line,
                std::string& orig_line);

  // Interpret arguments and perform specific actions on the tree.
  // This should be performed only after GetArgs_ function has been called,
  // and the arguments and original line have been processed.
  void InterpretArgs_(int nline, std::stringstream& msg,
                      std::vector<std::string>& args,
                      std::string& orig_line,
                      std::string tr_parent = "",
                      std::string tr_id = "",
                      std::string suffix = "");

  // Adds node and updates databases.
  void AddNode_(std::string parent, std::string id, std::string type,
                int vote_number = -1);

  // Adds probability to a primary event for p-model.
  void AddProb_(std::string id, double p);

  // Adds probability to a primary event for l-model.
  void AddProb_(std::string id, double p, double time);

  // Includes external transfer in subtrees to this current main tree.
  void IncludeTransfers_();

  // Graphs one top or intermediate event
  void GraphNode_(TopEventPtr t, std::map<std::string, int>& pr_repeat,
                  std::ofstream& out);

  // Adds children of top or intermediate event into a specified vector of sets.
  void ExpandSets_(int inter_index, std::vector<SupersetPtr>& sets);

  // Verifies if gates are initialized correctly with right number of children.
  // Returns a warning message string with the list of bad gates and their
  // problems.
  // Returns an empty string if no problems are detected.
  std::string CheckAllGates_();

  // Checks if a gate is initialized correctly.
  std::string CheckGate_(const TopEventPtr& event);

  // Returns primary events that do not have probabilities assigned.
  std::string PrimariesNoProb_();

  // -------------------- Algorithm for Cut Sets and Probabilities -----------
  // Expands sets for OR operator.
  void SetOr_(std::vector<int>& events_children,
              std::vector<SupersetPtr>& sets, int mult = 1);

  // Expands sets for AND operator.
  void SetAnd_(std::vector<int>& events_children,
               std::vector<SupersetPtr>& sets, int mult = 1);

  // Calculates a probability of a set of minimal cut sets, which are in OR
  // relationship with each other. This function is a brute force probability
  // calculation without rare event approximations.
  // nsums parameter specifies number of sums in the series.
  // Note: This function drastically modifies min_cut_sets by deleting
  // sets inside it. This is for better performance.
  double ProbOr_(std::set< std::set<int> >& min_cut_sets,
                 int nsums = 1000000);

  // Calculates a probability of a minimal cut set, which members are in AND
  // relationship with each other. This function assumes independence of each
  // member.
  double ProbAnd_(const std::set<int>& min_cut_set);

  // Calculates A(and)( B(or)C ) relationship for sets using set algebra.
  // Returns non-const reference because only intended to be used for
  // brute force probability calculations.
  void CombineElAndSet_(const std::set<int>& el,
                        const std::set< std::set<int> >& set,
                        std::set< std::set<int> >& combo_set);

  // Assign an index to each primary event, and then populates with this
  // indexes new databases of minimal cut sets and primary to integer
  // converting maps.
  void AssignIndexes_();

  // Updates minimal cut sets from indexes to strings.
  void SetsToString_();

  std::set< std::set<int> > imcs_;
  std::map< std::set<int>, std::set<std::string> > imcs_to_smcs_;

  std::vector<PrimaryEventPtr> int_to_prime_;
  boost::unordered_map<std::string, int> prime_to_int_;
  std::vector<double> iprobs_;  // Holds probabilities of basic events.

  int top_event_index_;
  boost::unordered_map<int, TopEventPtr> int_to_inter_;
  boost::unordered_map<std::string, int> inter_to_int_;
  // -----------------------------------------------------------------
  // ---- Algorithm for Equation Construction for Monte Carlo Sim -------
  void MProbOr_(std::set< std::set<int> >& min_cut_sets, int sign = 1,
                int nsums = 1000000);

  void MCombineElAndSet_(const std::set<int>& el,
                         const std::set< std::set<int> >& set,
                         std::set< std::set<int> >& combo_set);

  void MSample_();  // Perform simulation.
  std::vector< std::set<int> > pos_terms_;  // Plus terms of the equation.
  std::vector< std::set<int> > neg_terms_;  // Minus terms of the equation.
  std::vector<double> sampled_results_;  // Storage for sampled values.
  // -----------------------------------------------------------------
  // This member is used to provide any warnings about assumptions,
  // calculations, and settings. These warnings must be written into output
  // file.
  std::string warnings_;

  // Type of analysis to be performed.
  std::string analysis_;

  // Request for graphing instructions only.
  bool graph_only_;

  // Approximations for probability calculations.
  std::string approx_;

  // Input file path.
  std::string input_file_;

  // Keep track of currently opened file with sub-trees.
  std::string current_file_;

  // Indicator if probability calculations are requested.
  bool prob_requested_;

  // Indicate if analysis of the tree is done.
  bool analysis_done_;

  // Number of sums in series expansion for probability calculations.
  int nsums_;

  // Container of original names of events with capitalizations.
  std::map<std::string, std::string> orig_ids_;

  // List of all valid gates.
  std::set<std::string> gates_;

  // List of all valid types of primary events.
  std::set<std::string> types_;

  // Id of a top event.
  std::string top_event_id_;

  // Top event.
  TopEventPtr top_event_;

  // Indicator of detection of a top event described by a transfer sub-tree.
  bool top_detected_;

  // Indicates that reading the main tree file as opposed to a transfer tree.
  bool is_main_;

  // Holder for intermediate events.
  boost::unordered_map<std::string, InterEventPtr> inter_events_;

  // Container for primary events.
  boost::unordered_map<std::string, PrimaryEventPtr> primary_events_;

  // Container for transfer symbols as requested in tree initialization.
  // A queue contains a tuple of the parent and id of transferIn.
  std::queue< std::pair<std::string, std::string> > transfers_;

  // For graphing purposes the same transferIn.
  std::multimap<std::string, std::string> transfer_map_;

  // Container for storing all transfer sub-trees' names and number of calls.
  std::map<std::string, int> trans_calls_;

  // Container to track transfer calls to prevent cyclic calls/inclusions.
  std::map< std::string, std::vector<std::string> > trans_tree_;

  // Container for minimal cut sets.
  std::set< std::set<std::string> > min_cut_sets_;

  // Container for minimal cut sets and their respective probabilities.
  std::map< std::set<std::string>, double > prob_of_min_sets_;

  // Container for minimal cut sets ordered by their probabilities.
  std::multimap< double, std::set<std::string> > ordered_min_sets_;

  // Container for primary events and their contribution.
  std::map< std::string, double > imp_of_primaries_;

  // Container for primary events ordered by their contribution.
  std::multimap< double, std::string > ordered_primaries_;

  // Maximum order of the minimal cut sets.
  int max_order_;

  // Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;

  // Total probability of the top event.
  double p_total_;

  // Specific variables that are shared for initialization of tree nodes.
  std::string parent_;
  std::string id_;
  std::string type_;
  int vote_number_;
  bool block_started_;

  // Indicate if TransferOut is initiated correctly.
  bool transfer_correct_;

  // Indication of the first intermediate event of the transfer.
  bool transfer_first_inter_;

  // Sub-tree analysis only.
  bool sub_tree_analysis_;
};

}  // namespace scram

#endif  // SCRAM_FAULT_TREE_H_
