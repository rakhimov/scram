/// @file risk_analysis.h
#ifndef SCRAM_RISK_ANALYISIS_H_
#define SCRAM_RISK_ANALYISIS_H_

#include <string>

namespace scram {

/// @class RiskAnalysis
/// Main system that performs analyses.
class RiskAnalysis {
 public:
  /// This constructor with configurations with the analysis.
  /// @param[in] XML file with configurations for the analysis and output.
  RiskAnalysis(std::string config_file = "guess_yourself");

  /// Initializes the analysis from an input file.
  /// @param[in] input_file The input file.
  /// @todo Must deal with xml input file.
  /// @todo May have default configurations for analysis off all input files.
  virtual void ProcessInput(std::string input_file);

  /// Initializes probabilities relevant to the analysis.
  /// @param[in] prob_file The file with probability instructions.
  /// @todo Must be merged with the processing of the input file.
  virtual void PopulateProbabilities(std::string prob_file);

  /// Graphing or other visual resources for the analysis if applicable.
  /// @todod Must be handled by a separate class.
  virtual void GraphingInstructions();

  /// Perform the main analysis operations.
  /// @todo Must use specific analyzers for this operation.
  virtual void Analyze();

  /// Reports the results of analysis.
  /// @param[out] output The output destination.
  /// @todo Must rely upon another dedicated class.
  virtual void Report(std::string output);

  virtual ~RiskAnalysis() {}

 private:
  /// @todo Containers for fault trees, events, event trees, CCF, and other
  /// analysis entities.
};

}  // namespace scram

#endif  // SCRAM_RISK_ANALYSIS_H_
