#ifndef SCRAM_SUPERSET_H_
#define SCRAM_SUPERSET_H_

#include <map>
#include <set>
#include <string>

#include "error.h"
#include "risk_analysis.h"

namespace scram {

// This class holds sets generated upon traversing the fault tree.
// This class is a friend of FaultTree class, so it accesses private members.
class Superset {
 public:
  Superset();

  // Add an event into the set
  void AddMember(std::string id, FaultTree* ft);

  // Inserts another superset
  void Insert(Superset* st);

  // Returns an intermidiate event and deletes it from the set
  std::string PopInter();

  // Returns primary events
  int nprimes();

  // Returns intermediate events
  int ninters();

  // Returns the total set
  std::set<std::string>& all();

  ~Superset() {}

 private:
  std::set<std::string> inters_;
  std::set<std::string> primes_;
};

}  // namespace scram

#endif  // SCRAM_SUPERSET_H_
