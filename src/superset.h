#ifndef SCRAM_SUPERSET_H_
#define SCRAM_SUPERSET_H_

#include <set>
#include <string>

#include <boost/shared_ptr.hpp>

#include "error.h"

namespace scram {

// This class holds sets generated upon traversing a fault tree.
// Special superset class operates with sets that contain primary and
// intermediate events. It is designed to help efficiently find cut sets.
// This class separates primary events and intermediate events internally.
class Superset {
 public:
  Superset();

  // Add a name of a primary event into the set.
  bool AddPrimary(int id);

  // Add a name of an intermediate event into the set.
  bool AddInter(int id);

  // Inserts another superset.
  bool Insert(const boost::shared_ptr<Superset>& st);

  // Returns an intermidiate event and deletes it from the set.
  int PopInter();

  // Returns the number of primary events.
  int NumOfPrimeEvents();

  // Returns the number of intermediate events.
  int NumOfInterEvents();

  // Returns the set of primary events.
  std::set<int>& primes();

  // Returns the set of intermediate events. NOTE: Implemented for testing.
  std::set<int>& inters();

  ~Superset() {}

 private:
  std::set<int> inters_;
  std::set<int> primes_;
  bool cancel;
};

}  // namespace scram

#endif  // SCRAM_SUPERSET_H_
