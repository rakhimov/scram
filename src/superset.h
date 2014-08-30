/// @file superset.h
/// Superset class for storing various events.
#ifndef SCRAM_SUPERSET_H_
#define SCRAM_SUPERSET_H_

#include <set>
#include <string>

#include <boost/shared_ptr.hpp>

#include "error.h"

namespace scram {

/// @class Superset
/// This class holds sets generated upon traversing a fault tree.
/// Special superset class operates with sets that contain primary events and
/// gates of a fault tree. It is designed to help efficiently find cut sets.
/// This class separates primary events and gates internally
/// and operates with index numbers of these nodes.
///
/// @note The user of this class should take care of whether nodes are
/// primary events or gates upon putting them into this superset.
class Superset {
 public:
  /// Default constructor.
  Superset();

  /// Inserts a primary event into the set for initialization.
  /// @param[in] id The id index number for the event.
  /// @note This function does not check for complements.
  /// It is used for the superset initialization with unique events.
  void InsertPrimary(int id);

  /// Inserts a gate into the set for initialization.
  /// @param[in] id The id index number for the gate.
  /// @note This function does not check for complements.
  /// It is used for the superset initialization with unique events.
  void InsertGate(int id);

  /// Inserts another superset with gates and primary events.
  /// Check if there are complements of events.
  /// @param[in] st A pointer to another superset with events.
  /// @returns false iff the resultant set is null.
  /// @returns true if the addition is successful.
  bool InsertSet(const boost::shared_ptr<Superset>& st);

  /// @returns An id index number of a gate
  /// and deletes it from this set.
  /// @note Undefined behavior if the gates container is empty.
  /// The caller must make sure that that there are gates.
  int PopGate();

  /// @returns The number of primary events in this set.
  int NumOfPrimeEvents();

  /// @returns The number of gates in this set.
  int NumOfGates();

  /// @returns The set of primary events.
  const std::set<int>& primes();

  /// @returns The set of gates.
  const std::set<int>& gates();

  ~Superset() {}

 private:
  /// Container for gates.
  std::set<int> gates_;

  /// Container for primary events.
  std::set<int> primes_;

  /// The number of complement gates in the sets.
  int neg_gates_;

  /// Indicator of complement primary events.
  bool neg_primes_;

  /// Indication that this set contains events that cancel each other.
  /// For example, event A and complement of A will result in a null set.
  bool cancel;
};

}  // namespace scram

#endif  // SCRAM_SUPERSET_H_
