/// @file superset.cc
/// Implementation of Superset class.
#include "superset.h"

namespace scram {

Superset::Superset() : null_(false), neg_gates_(0), neg_primes_(false) {}

void Superset::InsertPrimary(int id) {
  if (!neg_primes_ && id < 0) neg_primes_ = true;
  primes_.insert(id);
}

void Superset::InsertGate(int id) {
  if (id < 0) ++neg_gates_;
  gates_.insert(id);
}

bool Superset::InsertSet(const boost::shared_ptr<Superset>& st) {
  if (null_) return false;
  if (primes_.empty() && gates_.empty()) {
    primes_ = st->primes_;
    gates_ = st->gates_;
    neg_gates_ = st->neg_gates_;
    neg_primes_ = st->neg_primes_;
    return true;
  }
  std::set<int>::iterator it;
  if (neg_primes_ || st->neg_primes_) {
    for (it = st->primes_.begin(); it != st->primes_.end(); ++it) {
      if (primes_.count(-1 * (*it))) {
        primes_.clear();
        gates_.clear();
        null_ = true;
        return false;
      }
    }
    if (!neg_primes_) neg_primes_ = false;  // New negative were included.
  }

  if (neg_gates_ || st->neg_gates_) {
    for (it = st->gates_.begin(); it != st->gates_.end(); ++it) {
      if (gates_.count(-1 * (*it))) {
        primes_.clear();
        gates_.clear();
        null_ = true;
        return false;
      }
      if (*it < 0) ++neg_gates_;
    }
  }

  primes_.insert(st->primes_.begin(), st->primes_.end());
  gates_.insert(st->gates_.begin(), st->gates_.end());
  return true;
}

int Superset::PopGate() {
  assert(!gates_.empty());
  std::set<int>::iterator it = gates_.begin();
  int gate = *it;
  gates_.erase(it);
  if (gate < 0) --neg_gates_;
  assert(neg_gates_ >= 0);
  return gate;
}

}  // namespace scram
