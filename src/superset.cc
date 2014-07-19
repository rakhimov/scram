#include "superset.h"

namespace scram {

Superset::Superset() : cancel(false) {}

bool Superset::AddPrimary(int id) {
  if (cancel) return false;
  if (primes_.count(-1 * id)) {
    primes_.clear();
    inters_.clear();
    cancel = true;
    return false;
  }
  primes_.insert(id);
  return true;
}

bool Superset::AddInter(int id) {
  if (cancel) return false;
  if (inters_.count(-1 * id)) {
    primes_.clear();
    inters_.clear();
    cancel = true;
    return false;
  }
  inters_.insert(id);
  return true;
}

bool Superset::Insert(const boost::shared_ptr<Superset>& st) {
  if (cancel) return false;
  std::set<int>::iterator it;
  for (it = st->primes_.begin(); it != st->primes_.end(); ++it) {
    if (primes_.count(-1 * (*it))) {
      primes_.clear();
      inters_.clear();
      cancel = true;
      return false;
    }
    primes_.insert(*it);
  }

  for (it = st->inters_.begin(); it != st->inters_.end(); ++it) {
    if (inters_.count(-1 * (*it))) {
      primes_.clear();
      inters_.clear();
      cancel = true;
      return false;
    }
    inters_.insert(*it);
  }

  return true;
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
