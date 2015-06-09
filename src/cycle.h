/// @file cycle.h
/// Facilities to detect and print cycles.
#ifndef SCRAM_SRC_CYCLE_H_
#define SCRAM_SRC_CYCLE_H_

#include <assert.h>

#include <string>
#include <vector>

namespace scram {
namespace cycle {

/// @param[in] cycle Cycle containing names in reverse order.
/// @returns String representation of the cycle.
std::string PrintCycle(const std::vector<std::string>& cycle);

}  // namespace cycle
}  // namespace scram

#endif  // SCRAM_SRC_CYCLE_H_
