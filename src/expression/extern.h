/*
 * Copyright (C) 2017 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file extern.h
/// The MEF facilities to call external functions in expressions.

#ifndef SCRAM_SRC_EXPRESSION_EXTERN_H_
#define SCRAM_SRC_EXPRESSION_EXTERN_H_

#include <string>
#include <type_traits>

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include "src/element.h"
#include "src/expression.h"

namespace scram {
namespace mef {

/// The MEF construct to extend expressions with external libraries.
/// This class dynamically loads and manages libraries.
/// It supports only very basic interface for C function lookup with its symbol.
class ExternLibrary : public Element, private boost::noncopyable {
 public:
  /// @copydoc Element::Element
  ///
  /// @param[in] lib_path  The library path with its name.
  /// @param[in] reference_dir  The reference directory for relative paths.
  /// @param[in] system  Search for the library in system paths.
  /// @param[in] decorate  Decorate the library name with prefix and suffix.
  ///
  /// @throws IOError  The library cannot be found.
  ///
  /// @pre The library path is canonical file path.
  ExternLibrary(std::string name, std::string lib_path,
                const boost::filesystem::path& reference_dir, bool system,
                bool decorate);

  ~ExternLibrary();  ///< Closes the loaded library.

  /// @tparam F  The C free function pointer type.
  ///
  /// @param[in] symbol  The function symbol in the library.
  ///
  /// @returns The function pointer resolved from the symbol.
  ///
  /// @throws UndefinedElement  The symbol is not in the library.
  ///
  /// @note The functionality should fail at compile time (UB in C)
  ///       if the platform doesn't support
  ///       object pointer to function pointer casts.
  template <typename F>
  std::enable_if_t<std::is_pointer<F>::value &&
                       std::is_function<std::remove_pointer_t<F>>::value,
                   F>
  get(const std::string& symbol) const {
    return reinterpret_cast<F>(get(symbol.c_str()));
  }

 private:
  /// Hides all the cross-platform shared library faculties.
  /// @todo Remove/refactor after switching to Boost 1.61 on Linux.
  class Pimpl;

  /// @param[in] symbol  The function symbol in the library.
  ///
  /// @returns The function loaded from the library symbol.
  ///
  /// @throws UndefinedElement  The symbol is not in the library.
  void* get(const char* symbol) const;

  Pimpl* pimpl_;  ///< Provides basic implementation for function discovery.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_EXTERN_H_
