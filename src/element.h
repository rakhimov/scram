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

  /// The name that identifies this attribute.
  std::string name;

  /// Value of this attributes.
  std::string value;

  /// Optional type of the attribute.
  std::string type;
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
  /// The label for the element.
  std::string label_;

  /// Collection of attributes.
  std::map<std::string, Attribute> attributes_;
};

}  // namespace scram

#endif  // SCRAM_SRC_ELEMENT_H_
