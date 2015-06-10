/// @file cycle.cc
/// Implementation of helper facilities to work with cycles.
#include "cycle.h"

namespace scram {
namespace cycle {

std::string PrintCycle(const std::vector<std::string>& cycle) {
  assert(cycle.size() > 1);
  std::vector<std::string>::const_iterator it = cycle.begin();
  std::string cycle_start = *it;
  std::string result = "->" + cycle_start;
  for (++it; *it != cycle_start; ++it) {
    assert(it != cycle.end());
    result = "->" + *it + result;
  }
  result = cycle_start + result;
  return result;
}

}  // namespace cycle
}  // namespace scram
