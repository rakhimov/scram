/// @file superset.cc
/// Implementation of Superset class.
#include "superset.h"

namespace scram {

Superset::Superset() : cancel(false), neg_gates_(0), neg_primes_(false) {}

bool Superset::InsertPrimary(int id) {
  if (cancel) return false;
  if (id < 0 || neg_primes_) {
    if (primes_.count(-1 * id)) {
      primes_.clear();
      gates_.clear();
      cancel = true;
      return false;
    }
  }
  if (!neg_primes_ && id < 0) neg_primes_ = true;
  primes_.insert(id);
  return true;
}

bool Superset::InsertGate(int id) {
  if (cancel) return false;
  if (id < 0 || neg_gates_) {
    if (gates_.count(-1 * id)) {
      primes_.clear();
      gates_.clear();
      cancel = true;
      return false;
    }
  }
  if (id < 0) ++neg_gates_;
  gates_.insert(id);
  return true;
}

bool Superset::InsertSet(const boost::shared_ptr<Superset>& st) {
  if (cancel) return false;
  if (primes_.empty() && gates_.empty()) {
    primes_ = st->primes_;
    gates_ = st->gates_;
    neg_gates_ = st->neg_gates_;
    neg_primes_ = st->neg_primes_;
    return true;
  }
  std::set<int>::iterator it;
  for (it = st->primes_.begin(); it != st->primes_.end(); ++it) {
    if (*it < 0 || neg_primes_) {
      if (primes_.count(-1 * (*it))) {
        primes_.clear();
        gates_.clear();
        cancel = true;
        return false;
      }
    }
    if (!neg_primes_ && *it < 0) neg_primes_ = true;
    primes_.insert(*it);
  }

  for (it = st->gates_.begin(); it != st->gates_.end(); ++it) {
    if (*it < 0 || neg_gates_) {
      if (gates_.count(-1 * (*it))) {
        primes_.clear();
        gates_.clear();
        cancel = true;
        return false;
      }
    }
    if (*it < 0) ++neg_gates_;
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
  if (gate < 0) --neg_gates_;
  assert(neg_gates_ >= 0);
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
