/// @file superset.cc
/// Implementation of Superset class.
#include "superset.h"

namespace scram {

Superset::Superset() : null_(false), neg_p_events_(false) {}

void Superset::InsertPrimary(int id) {
  if (id > 0) {
    p_events_.insert(p_events_.end(), id);

  } else {
    p_events_.insert(p_events_.begin(), id);
    if (!neg_p_events_) neg_p_events_ = true;
  }
}

void Superset::InsertGate(int id) {
  assert(id > 0);
  gates_.insert(gates_.end(), id);
}

bool Superset::InsertSet(const boost::shared_ptr<Superset>& st) {
  if (null_) return false;
  std::set<int>::iterator it;
  if (neg_p_events_ || st->neg_p_events_) {
    for (it = st->p_events_.begin(); it != st->p_events_.end(); ++it) {
      if (p_events_.count(-*it)) {
        p_events_.clear();
        gates_.clear();
        null_ = true;
        return false;
      }
    }
    if (!neg_p_events_) neg_p_events_ = false;  // New negative was included.
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
  return gate;
}

}  // namespace scram
