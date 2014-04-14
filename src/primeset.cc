#include "primeset.h"

namespace scram {

Primeset::Primeset() : prob_(1) {
}

void Primeset::AddPrime(std::string id, FaultTree* ft) {
  if (!primes_.count(id)) {
    primes_.insert(id);
    // prob_ *= ft->primary_events_[id]->p();
  }
}

void Primeset::Insert(Primeset* st, FaultTree* ft) {
  std::set<std::string>::iterator it;
  for (it = st->primes_.begin(); it != st->primes_.end(); ++it) {
    AddPrime(*it, ft);
  }
}

void Primeset::Insert(const std::set<std::string>& set, FaultTree* ft) {
  std::set<std::string>::iterator it;
  for (it = set.begin(); it != set.end(); ++it) {
    AddPrime(*it, ft);
  }
}

bool Primeset::empty() {
  return primes_.empty();
}

double Primeset::prob() {
  if (empty()) {
    throw scram::ValueError("The set is empty for probability calculations.");
  }
  return prob_;
}

}  // namespace scram
