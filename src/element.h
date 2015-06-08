/// @file element.h
/// Helper classes, structs, and properties common to all other classes.
#ifndef SCRAM_SRC_ELEMENT_H_
#define SCRAM_SRC_ELEMENT_H_

#include <string>
#include <map>

namespace scram {

/// @struct Attribute
/// This struct allows for any attribute.
struct Attribute {
  Attribute();

  std::string name;  ///< The name that identifies this attribute.
  std::string value;  ///< Value of this attributes.
  std::string type;  ///< Optional type of the attribute.
};

/// @class Element
/// This class represents any element of analysis that can have some extra
/// descriptions.
class Element {
 public:
  Element();

  /// @returns The empty or preset label.
  /// @returns Empty string if the label has not been set.
  inline std::string label() { return label_; }

  /// Sets the label.
  /// @param[in] new_label The label to be set.
  /// @throws LogicError if the label is already set or the new label is empty.
  void label(std::string new_label);

  /// Adds an attribute to the attribute map.
  /// @param[in] attr Unique attribute of this element.
  /// @throws LogicError if the attribute already exists.
  void AddAttribute(const Attribute& attr);

  /// Checks if the element has a given attribute.
  /// @param[in] id The identification name of the attribute in lower case.
  bool HasAttribute(const std::string& id);

  /// @return Pointer to the attribute if it exists.
  /// @param[in] id The id name of the attribute in lower case.
  /// @throws LogicError if there is no such attribute.
  const Attribute& GetAttribute(const std::string& id);

  virtual ~Element() {}

 private:
  std::string label_;  ///< The label for the element.
  std::map<std::string, Attribute> attributes_;  ///< Collection of attributes.
};

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

#endif  // SCRAM_SRC_ELEMENT_H_
