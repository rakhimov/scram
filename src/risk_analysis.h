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

  void process_input(std::string input_file);
  void populate_probabilities(std::string prob_file);
  void graphing_instructions();
  void analyze();

  ~FaultTree() {}

 private:
  // Adds node and updates databases
  void add_node_(std::string parent, std::string id, std::string type,
                 int nline);

  // verifies that every intermidiate node is not a leaf
  bool is_leaf_();

  // type of analysis to be performed
  std::string analysis_;

  // input file parth. Needed to create output files.
  std::string input_file_;

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
