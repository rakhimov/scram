/// @file superset.cc
/// Implementation of Superset class.
#include "superset.h"

namespace scram {

Superset::Superset() : cancel(false) {}

bool Superset::InsertPrimary(int id) {
  if (cancel) return false;
  if (primes_.count(-1 * id)) {
    primes_.clear();
    gates_.clear();
    cancel = true;
    return false;
  }
  primes_.insert(id);
  return true;
}

bool Superset::InsertGate(int id) {
  if (cancel) return false;
  if (gates_.count(-1 * id)) {
    primes_.clear();
    gates_.clear();
    cancel = true;
    return false;
  }
  gates_.insert(id);
  return true;
}

bool Superset::InsertSet(const boost::shared_ptr<Superset>& st) {
  if (cancel) return false;
  std::set<int>::iterator it;
  for (it = st->primes_.begin(); it != st->primes_.end(); ++it) {
    if (primes_.count(-1 * (*it))) {
      primes_.clear();
      gates_.clear();
      cancel = true;
      return false;
    }
    primes_.insert(*it);
  }

  for (it = st->gates_.begin(); it != st->gates_.end(); ++it) {
    if (gates_.count(-1 * (*it))) {
      primes_.clear();
      gates_.clear();
      cancel = true;
      return false;
    }
    gates_.insert(*it);
  }

  return true;
}

int Superset::PopGate() {
  if (gates_.empty()) {
    throw scram::ValueError("No gates to return.");
  }
  std::set<int>::iterator it = gates_.begin();
  int gate = *it;
  gates_.erase(it);
  return gate;
}

int Superset::NumOfPrimeEvents() {
  return primes_.size();
}

int Superset::NumOfGates() {
  return gates_.size();
}

const std::set<int>& Superset::primes() {
  return primes_;
}

const std::set<int>& Superset::gates() {
  return gates_;
}

}  // namespace scram
