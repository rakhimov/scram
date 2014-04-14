#ifndef SCRAM_PRIMESET_H_
#define SCRAM_PRIMESET_H_

#include <set>
#include <string>

#include <boost/unordered_set.hpp>

#include "error.h"
#include "risk_analysis.h"

namespace scram {

// This class holds sets only primary events. It is purpose to speed up
// calculations.
// This class is a friend of FaultTree class, so it accesses private members.
class Primeset {
 public:
  Primeset();

  // Add an event into the set
  void AddPrime(std::string id, FaultTree* ft);

  // Inserts another Primeset
  void Insert(Primeset* st, FaultTree* ft);

  // Inserts primary events from a standard set
  void Insert(const std::set<std::string>& set, FaultTree* ft);

  // Check for emptiness
  bool empty();

  // Probability of this set
  double prob();

    ~Primeset() {}

 private:
  std::set<std::string> primes_;
  // total probability of the set
  double prob_;
};

}  // namespace scram

#endif  // SCRAM_PRIMESET_H_
