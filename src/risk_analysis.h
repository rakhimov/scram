// Classes for various risk analysis

#ifndef SCRAM_RISK_ANALYISIS_H_
#define SCRAM_RISK_ANALYISIS_H_

#include <map>
#include <set>
#include <string>

#include "event.h"

namespace scram {

// Interface for various risk analysis methods
class RiskAnalysis {
 public:
  virtual void process_input(std::string input_file) = 0;
  virtual void populate_probabilities(std::string prob_file) = 0;
  virtual void graphing_instructions() = 0;
  virtual void analyze() = 0;
  virtual void report(std::string output) = 0;

  virtual ~RiskAnalysis() {}
};


// Fault tree analysis
class FaultTree : public RiskAnalysis {
 public:
  FaultTree(std::string input_file, bool rare_event = false);

  // Reads input file with the structure of the Fault tree.
  // Puts all events into their appropriate containers.
  void process_input(std::string input_file);

  // Reads probabiliteis for basic events from formatted input file with values.
  // Attaches probabilities to basic events.
  void populate_probabilities(std::string prob_file);

  // Outputs a file with instructions for graphviz dot to create a fault tree.
  void graphing_instructions();

  // Analyzes the fault tree and performs computations.
    void analyze();

  // Reports the results of analysis to a specified output destination.
  void report(std::string output);

  ~FaultTree() {}

 private:
  // Adds node and updates databases
  void add_node_(std::string parent, std::string id, std::string type);

  // Adds probability to a basic event
  void add_prob_(std::string id, double p);

  // Verifies that there are no intermidiate nodes that are a leaf.
  // Returns empty string if successful and ids of leaves if not.
  std::string inters_no_child_();

  // Returns basic events that do not have probabilities assigned
  std::string basics_no_prob_();

  // Calculates a probability of a set of minimal cut sets, which are in OR
  // relationship with each other. This function is a brute force probability
  // calculation without rare event approximations.
  double prob_or_(const std::set< std::set<std::string> > min_cut_sets);

  // Calculates a probability of a minimal cut set, which members are in AND
  // relationship with each other. This function assumes independence of each
  // member.
  double prob_and_(const std::set<std::string>& min_cut_set);

  // Calculates A(and)( B(or)C ) relationship for sets using set algebra.
  // Returns non-const reference because only intended to be used for
  // brute force probability calculations.
  std::set< std::set<std::string> > combine_el_and_set_(
      const std::set< std::string>& el,
      const std::set< std::set<std::string> >& set);

  // This member is used to provide any warnings about assumptions,
  // calculations, and settings. These warnings must be written into output
  // file.
  std::string warnings_;

  // type of analysis to be performed
  std::string analysis_;

  // rare event approximation
  bool rare_event_;

  // input file path. Needed to create output files.
  std::string input_file_;

  // container of original names of events with capitalizations
  std::map<std::string, std::string> orig_ids_;

  // list of all valid types
  std::set<std::string> types_;

  // id of a top event
  std::string top_event_id_;

  // top event
  scram::TopEvent* top_event_;

  // holder for intermidiate events
  std::map<std::string, scram::InterEvent*> inter_events_;

  // container for basic events
  std::map<std::string, scram::PrimaryEvent*> primary_events_;

  // container for minimal cut sets
  std::set< std::set<std::string> > min_cut_sets_;

  // container for minimal cut sets and their respective probabilities
  std::map< std::set<std::string>, double > prob_of_min_sets_;

  // total probability of the top event
  double p_total_;

};

}  // namespace scram

#endif  // SCRAM_RISK_ANALYSIS_H_
