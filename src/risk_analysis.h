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

  virtual ~RiskAnalysis() {}
};


// Fault tree analysis
class FaultTree : public RiskAnalysis {
 public:
  explicit FaultTree(std::string input_file);

  // Reads input file with the structure of the Fault tree.
  // Puts all events into their appropriate containers.
  void process_input(std::string input_file);

  // Reads probabiliteis for basic events from formatted input file with values.
  // Attaches probabilities to basic events.
  void populate_probabilities(std::string prob_file);

  // Outputs a file with instructions for graphviz dot to create a fault tree.
  void graphing_instructions();

  // Analyzes the fault tree and performs computations.
  // Writes the results into output file that is named like the provided input
  // file.
  void analyze();

  ~FaultTree() {}

 private:
  // Adds node and updates databases
  void add_node_(std::string parent, std::string id, std::string type,
                 int nline);

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
  bool rare_event;

  // input file path. Needed to create output files.
  std::string input_file_;

  // container of original names of events with capitalizations
  std::map<std::string, std::string> orig_ids_;

  // list of all valid gates
  std::set<std::string> gates_;

  // id of a top event
  std::string top_event_id_;

  // top event
  scram::TopEvent* top_event_;

  // holder for intermidiate events
  std::map<std::string, scram::InterEvent*> inter_events_;

  // container for basic events
  std::map<std::string, scram::BasicEvent*> basic_events_;

};

}  // namespace scram

#endif  // SCRAM_RISK_ANALYSIS_H_
