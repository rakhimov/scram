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
/// Special superset class operates with sets that contain primary and
/// intermediate events. It is designed to help efficiently find cut sets.
/// This class separates primary events and intermediate events internally
/// and operates with index numbers of events.
///
/// @note The user of this class should take care of whether the events are
/// primary or intermediate upon putting them into this superset.
class Superset {
 public:
  /// Default constructor.
  Superset();

  /// Adds a primary event into the set.
  /// @param[in] id The id index number for the event.
  /// @returns false iff the resultant set is null.
  /// @returns true if the addition is successful.
  bool AddPrimary(int id);

  /// Add an intermediate event into the set.
  /// @param[in] id The id index number for the event.
  /// @returns false iff the resultant set is null.
  /// @returns true if the addition is successful.
  bool AddInter(int id);

  /// Inserts another superset of intermediate and primary events.
  /// @param[in] st A pointer to another superset with events.
  /// @returns false iff the resultant set is null.
  /// @returns true if the addition is successful.
  bool Insert(const boost::shared_ptr<Superset>& st);

  /// @returns An id index number of an intermidiate event
  /// and deletes it from this set.
  /// @throws ValueError if there are no events to pop.
  int PopInter();

  /// @returns The number of primary events in this set.
  int NumOfPrimeEvents();

  /// @returns The number of intermediate events in this set.
  int NumOfInterEvents();

  /// @returns The set of primary events.
  const std::set<int>& primes();

  /// @returns The set of intermediate events.
  /// @note This is for testing only.
  const std::set<int>& inters();

  ~Superset() {}

 private:
  /// Container for intermediate events.
  std::set<int> inters_;

  /// Container for primary events.
  std::set<int> primes_;

  /// Indication that this set contains events that cancel each other.
  /// For example, event A and complement of A will result in a null set.
  bool cancel;
};

}  // namespace scram

#endif  // SCRAM_SUPERSET_H_
