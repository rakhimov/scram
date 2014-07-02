#ifndef SCRAM_RISK_ANALYISIS_H_
#define SCRAM_RISK_ANALYISIS_H_

namespace scram {

// Interface for various risk analysis methods.
class RiskAnalysis {
 public:
  virtual void ProcessInput(std::string input_file) = 0;
  virtual void PopulateProbabilities(std::string prob_file) = 0;
  virtual void GraphingInstructions() = 0;
  virtual void Analyze() = 0;
  virtual void Report(std::string output) = 0;

  virtual ~RiskAnalysis() {}
};

}  // namespace scram

#endif  // SCRAM_RISK_ANALYSIS_H_
