#ifndef SCRAM_SUPERSET_H_
#define SCRAM_SUPERSET_H_

#include <set>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>

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
  void AddPrimary(const std::string& id);

  // Add a name of an intermediate event into the set.
  void AddInter(const std::string& id);

  // Inserts another superset.
  void Insert(const boost::shared_ptr<Superset>& st);

  // Returns an intermidiate event and deletes it from the set.
  std::string PopInter();

  // Returns the number of primary events.
  int NumOfPrimeEvents();

  // Returns the number of intermediate events.
  int NumOfInterEvents();

  // Returns the set of primary events.
  std::set<std::string>& primes();

  // Returns the set of intermediate events. NOTE: Implemented for testing.
  std::set<std::string>& inters();

  ~Superset() {}

 private:
  boost::unordered_set<std::string> inters_;
  boost::unordered_set<std::string> primes_;
  std::set<std::string> std_primes_;
  std::set<std::string> std_inters_;
};

}  // namespace scram

#endif  // SCRAM_SUPERSET_H_
