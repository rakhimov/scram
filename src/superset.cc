#include "superset.h"

namespace scram {

Superset::Superset() {}

void Superset::AddPrimary(int id) {
  primes_.insert(id);
}

void Superset::AddInter(int id) {
  inters_.insert(id);
}

void Superset::Insert(const boost::shared_ptr<Superset>& st) {
  primes_.insert(st->primes_.begin(), st->primes_.end());
  inters_.insert(st->inters_.begin(), st->inters_.end());
}

int Superset::PopInter() {
  if (inters_.empty()) {
    throw scram::ValueError("No intermediate events to return.");
  }
  std::set<int>::iterator it = inters_.begin();
  int inter = *it;
  inters_.erase(it);
  return inter;
}

int Superset::NumOfPrimeEvents() {
  return primes_.size();
}

int Superset::NumOfInterEvents() {
  return inters_.size();
}

std::set<int>& Superset::primes() {
  return primes_;
}

std::set<int>& Superset::inters() {
  return inters_;
}

}  // namespace scram
