/// @file superset.cc
/// Implementation of Superset class.
#include "superset.h"

namespace scram {

Superset::Superset() : null_(false), neg_gates_(0), neg_p_events_(false) {}

void Superset::InsertPrimary(int id) {
  if (!neg_p_events_ && id < 0) neg_p_events_ = true;
  p_events_.insert(id);
}

void Superset::InsertGate(int id) {
  if (id < 0) ++neg_gates_;
  gates_.insert(id);
}

bool Superset::InsertSet(const boost::shared_ptr<Superset>& st) {
  if (null_) return false;
  if (p_events_.empty() && gates_.empty()) {
    p_events_ = st->p_events_;
    gates_ = st->gates_;
    neg_gates_ = st->neg_gates_;
    neg_p_events_ = st->neg_p_events_;
    return true;
  }
  std::set<int>::iterator it;
  if (neg_p_events_ || st->neg_p_events_) {
    for (it = st->p_events_.begin(); it != st->p_events_.end(); ++it) {
      if (p_events_.count(-1 * (*it))) {
        p_events_.clear();
        gates_.clear();
        null_ = true;
        return false;
      }
    }
    if (!neg_p_events_) neg_p_events_ = false;  // New negative were included.
  }

  if (neg_gates_ || st->neg_gates_) {
    for (it = st->gates_.begin(); it != st->gates_.end(); ++it) {
      if (gates_.count(-1 * (*it))) {
        p_events_.clear();
        gates_.clear();
        null_ = true;
        return false;
      }
      if (*it < 0) ++neg_gates_;
    }
  }

  p_events_.insert(st->p_events_.begin(), st->p_events_.end());
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
