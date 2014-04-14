#include "superset.h"

namespace scram {

Superset::Superset() {
}

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
  std::set<std::string>::iterator it = inters_.begin();
  std::string inter = *it;
  inters_.erase(it);
  return inter;
}

int Superset::nprimes() {
  return primes_.size();
}

int Superset::ninters() {
  return inters_.size();
}

std::set<std::string>& Superset::all() {
  if (inters_.size() == 0) {
    return primes_;
  }
  std::set<std::string> temp = primes_;
  temp.insert(inters_.begin(), inters_.end());
  return temp;
}

}  // namespace scram
