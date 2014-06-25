#include "superset.h"

namespace scram {

Superset::Superset() {}

void Superset::AddMember(std::string id, FaultTree* ft) {
  if (ft->inter_events_.count(id)) {
    inters_.insert(id);
  } else if (ft->primary_events_.count(id)) {
    primes_.insert(id);
  } else {
    throw scram::ValueError(id + " event does not exists in the fault tree.");
  }
}

void Superset::Insert(Superset* st) {
  primes_.insert(st->primes_.begin(), st->primes_.end());
  inters_.insert(st->inters_.begin(), st->inters_.end());
}

std::string Superset::PopInter() {
  if (inters_.empty()) {
    throw scram::ValueError("No intermediate events to return.");
  }
  boost::unordered_set<std::string>::iterator it = inters_.begin();
  std::string inter = *it;
  inters_.erase(it);
  return inter;
}

int Superset::NumOfPrimeEvents() {
  return primes_.size();
}

int Superset::NumOfInterEvents() {
  return inters_.size();
}

std::set<std::string>& Superset::primes() {
  std_primes_.insert(primes_.begin(), primes_.end());
  return std_primes_;
}

}  // namespace scram
