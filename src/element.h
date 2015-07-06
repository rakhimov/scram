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
/// This abstract class represents any element of analysis that can have some
/// extra descriptions, such as attributes and a label.
class Element {
 public:
  Element();

  virtual ~Element() = 0;  ///< Abstract class.

  /// @returns The empty or preset label.
  /// @returns Empty string if the label has not been set.
  inline const std::string& label() const { return label_; }

  /// Sets the label.
  /// @param[in] new_label The label to be set.
  /// @throws LogicError if the label is already set or the new label is empty.
  void label(const std::string& new_label);

  /// Adds an attribute to the attribute map.
  /// @param[in] attr Unique attribute of this element.
  /// @throws LogicError if the attribute already exists.
  void AddAttribute(const Attribute& attr);

  /// Checks if the element has a given attribute.
  /// @param[in] id The identification name of the attribute in lower case.
  bool HasAttribute(const std::string& id) const;

  /// @returns Pointer to the attribute if it exists.
  /// @param[in] id The id name of the attribute in lower case.
  /// @throws LogicError if there is no such attribute.
  const Attribute& GetAttribute(const std::string& id) const;

 private:
  std::string label_;  ///< The label for the element.
  std::map<std::string, Attribute> attributes_;  ///< Collection of attributes.
};

/// @class Role
/// Abstract class that manages private or public roles for elements as needed.
/// Public is the default assumption. It is expected to set only once and never
/// changes.
class Role {
 public:
  /// Sets the role of an element upon creation.
  /// @param[in] is_public A flag to define public or private role.
  /// @param[in] base_path The series of containers to get this event.
  explicit Role(bool is_public = true, const std::string& base_path = "")
      : is_public_(is_public),
        base_path_(base_path) {}

  virtual ~Role() = 0;  ///< Abstract class;

  /// @returns True for public roles, or False for private roles.
  inline bool is_public() const { return is_public_; }

  /// @returns The base path containing ancestor container names.
  inline const std::string& base_path() const { return base_path_; }

 private:
  bool is_public_;  ///< A flag for public and private roles.
  std::string base_path_;  ///< A series of containers leading to this event.
};

}  // namespace scram

#endif  // SCRAM_SRC_ELEMENT_H_
