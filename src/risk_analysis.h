/// @file risk_analysis.h
#ifndef SCRAM_RISK_ANALYISIS_H_
#define SCRAM_RISK_ANALYISIS_H_

namespace scram {

/// @interface RiskAnalysis
/// Interface for various risk analysis methods.
class RiskAnalysis {
 public:
  /// Initializes the analysis from an input file.
  /// @param[in] input_file The input file.
  virtual void ProcessInput(std::string input_file) = 0;

  /// Initializes probabilities relevant to the analysis.
  /// @param[in] prob_file The file with probability instructions.
  virtual void PopulateProbabilities(std::string prob_file) = 0;

  /// Graphing or other visual resources for the analysis if applicable.
  virtual void GraphingInstructions() = 0;

  /// Perform the main analysis operations.
  virtual void Analyze() = 0;

  /// Reports the results of analysis.
  /// @param[out] output The output destination.
  virtual void Report(std::string output) = 0;

  virtual ~RiskAnalysis() {}
};

}  // namespace scram

#endif  // SCRAM_RISK_ANALYSIS_H_
