// Classes for various risk analysis

#ifndef SCRAM_RISK_ANALYISIS_H_
#define SCRAM_RISK_ANALYISIS_H_

#include <string>

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
  std::string analysis_;
  std::string input_file_;

};

}  // namespace scram

#endif  // SCRAM_RISK_ANALYSIS_H_
