/// @file node.h
/// Contains node classes for fault trees.
#ifndef SCRAM_NODE_H_
#define SCRAM_NODE_H_

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "error.h"

namespace scram {

/// @class Node
/// General fault tree node base class.
class Node {
 public:
  /// Constructs a fault tree node with a specific id.
  /// @param[in] id The identifying name for the node.
  Node(std::string id) : id_(id) {}

  /// @returns The id that is set upon the construction of this node.
  virtual const std::string& id() { return id_; }

  virtual ~Node() {}

 private:
  /// Id name of a node.
  std::string id_;
};

/// @class Gate
/// A representation of a gate in a fault tree.
class Gate : public scram::Node {
 public:
  /// Constructs with an id and a gate.
  /// @param[in] id The identifying name for this node.
  /// @param[in] type The type for this gate.
  Gate(std::string id, std::string type = "NONE");

  /// @returns The gate type.
  /// @throws ValueError if the gate is not yet assigned.
  const std::string& type();

  /// Sets the gate type.
  /// @param[in] gate The gate type for this node.
  /// @throws ValueError if the gate type is being re-assigned.
  void type(std::string type);

  /// @returns The vote number iff the gate is vote.
  int vote_number();

  /// Sets the vote number only for a vote gate.
  /// @param[in] vnumber The vote number.
  /// @throws ValueError if the vote number is invalid or being re-assigned.
  void vote_number(int vnumber);

  /// Adds a parent into the parent map.
  /// @param[in] parent One of the parents of this gate node.
  /// @throws ValueError if the parent is being re-inserted.
  void AddParent(const boost::shared_ptr<scram::Gate>& parent);

  /// @returns All the parents of this gate node.
  /// @throws ValueError if there are no parents for this gate node.
  const std::map<std::string, boost::shared_ptr<scram::Gate> >& parents();

  /// Adds a child node into the children list.
  /// @param[in] child A pointer to a child node.
  /// @throws ValueError if the child is being re-inserted.
  virtual void AddChild(const boost::shared_ptr<scram::Node>& child);

  /// @returns The children of this gate.
  /// @throws ValueError if there are no children.
  const std::map<std::string, boost::shared_ptr<scram::Node> >& children();

  virtual ~Gate() {}

 private:
  /// Gate type.
  std::string type_;

  /// Vote number for the vote gate.
  int vote_number_;

  /// The parents of this gate.
  std::map<std::string, boost::shared_ptr<scram::Gate> > parents_;

  /// The children of this gate.
  std::map<std::string, boost::shared_ptr<scram::Node> > children_;
};

/// @class PrimaryEvent
/// This is a base class for events that can cause faults.
/// This class represents Base, House, Undeveloped, and other events.
class PrimaryEvent : public scram::Node {
 public:
  /// Constructs with id name and probability.
  /// @param[in] id The identifying name of this primary event.
  /// @param[in] type The type of the event.
  /// @param[in] p The failure probability.
  PrimaryEvent(std::string id, std::string type = "", double p = -1);

  /// @returns The type of the primary event.
  /// @throws ValueError if the type is not yet set.
  const std::string& type();

  /// Sets the type.
  /// @param[in] new_type The type for this event.
  /// @throws ValueError if type is not valid or being re-assigned.
  void type(std::string new_type);

  /// @returns The probability of failure of this event.
  /// @throws ValueError if probability is not yet set.
  double p();

  /// Sets the total probability for P-model.
  /// @param[in] p The total failure probability.
  /// @throws ValueError if probability is not a valid value or re-assigned.
  void p(double p);

  /// Sets the total probability for L-model.
  /// @param[in] freq The failure frequency for L-model.
  /// @param[in] time The time to failure for L-model.
  /// @throws ValueError if probability is not a valid value or re-assigned.
  void p(double freq, double time);

  /// Adds a parent into the parent map.
  /// @param[in] parent One of the parents of this primary event.
  /// @throws ValueError if the parent is being re-inserted.
  void AddParent(const boost::shared_ptr<scram::Gate>& parent);

  /// @returns All the parents of this primary event.
  /// @throws ValueError if there are no parents for this primary event.
  const std::map<std::string, boost::shared_ptr<scram::Gate> >& parents();

  ~PrimaryEvent() {}

 private:
  /// The type of the primary event.
  std::string type_;

  /// The total failure probability of the primary event.
  double p_;

  /// The parents of this primary event.
  std::map<std::string, boost::shared_ptr<scram::Gate> > parents_;
};

}  // namespace scram

#endif  // SCRAM_NODE_H_
