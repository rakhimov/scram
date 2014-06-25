#ifndef SCRAM_SUPERSET_H_
#define SCRAM_SUPERSET_H_

#include <set>
#include <string>

#include <boost/unordered_set.hpp>

#include "error.h"
#include "risk_analysis.h"

namespace scram {

// This class holds sets generated upon traversing the fault tree.
// This class is a friend of FaultTree class, so it accesses private members.
class Superset {
 public:
  Superset();

  // Add an event into the set.
  void AddMember(std::string id, FaultTree* ft);

  // Inserts another superset.
  void Insert(Superset* st);

  // Returns an intermidiate event and deletes it from the set.
  std::string PopInter();

  // Returns the number of primary events.
  int NumOfPrimeEvents();

  // Returns the number of intermediate events.
  int NumOfInterEvents();

  // Returns the set of primary events.
  std::set<std::string>& primes();

  ~Superset() {}

 private:
  boost::unordered_set<std::string> inters_;
  boost::unordered_set<std::string> primes_;
  std::set<std::string> std_primes_;
};

}  // namespace scram

#endif  // SCRAM_SUPERSET_H_
