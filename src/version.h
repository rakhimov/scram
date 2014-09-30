/// @file version.h
/// Set of functions with version information of the core and
/// dependencies.
#ifndef SCRAM_VERSION_H_
#define SCRAM_VERSION_H_

#define SCRAM_VERSION_MAJOR 0
#define SCRAM_VERSION_MINOR 3
#define SCRAM_VERSION_MICRO 1

namespace scram {
namespace version {

/// @returns Git generated tag recent version.
const char* describe();

/// @returns The core version.
const char* core();

/// @returns The version of the boost.
const char* boost();

/// @returns The version of XML libraries.
const char* xml2();

}  // namespace version
}  // namespace scram

#endif  // SCRAM_VERSION_H_
