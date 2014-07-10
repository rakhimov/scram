#include "superset.h"

namespace scram {

Superset::Superset() {}

void Superset::AddPrimary(const std::string& id) {
  primes_.insert(id);
}

void Superset::AddInter(const std::string& id) {
  inters_.insert(id);
}

void Superset::Insert(const boost::shared_ptr<Superset>& st) {
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

std::set<std::string>& Superset::inters() {
  if (!std_inters_.empty()) std_inters_.clear();  // Inters may be deleted.
  std_inters_.insert(inters_.begin(), inters_.end());
  return std_inters_;
}

}  // namespace scram
