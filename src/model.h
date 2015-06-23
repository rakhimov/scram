/// @file model.h
/// Representation for a model container for risk analysis.
#ifndef SCRAM_SRC_MODEL_H_
#define SCRAM_SRC_MODEL_H_

#include "element.h"

namespace scram {

/// @class Model
/// This class represents a model that is defined in one input file.
class Model : public Element {
 public:
  /// Creates a model from with the input file and name as identifiers.
  /// This information can be used to detect duplications.
  /// @param[in] file The file where this model is defined.
  /// @param[in] name The optional name for the model.
  explicit Model(std::string file, std::string name = "");

  /// @returns The file of this model.
  inline std::string file() const { return file_; }

  /// @returns The name of the model.
  inline std::string name() const { return name_; }

 private:
  std::string file_;  ///< The file where this model is retrieved from.
  std::string name_;  ///< The name of the model.
};

}  // namespace scram

#endif  // SCRAM_SRC_MODEL_H_
