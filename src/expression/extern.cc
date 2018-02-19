/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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

/// @file
/// Implementation of foreign function calls with MEF expressions.

#include "extern.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace scram::mef {

ExternLibrary::ExternLibrary(std::string name, std::string lib_path,
                             const fs::path& reference_dir, bool system,
                             bool decorate)
    : Element(std::move(name)) {
  fs::path fs_path(lib_path);
  std::string filename = fs_path.filename().string();
  // clang-format off
  if (fs_path.empty() ||
      filename == "." ||
      filename == ".." ||
      lib_path.back() == ':' ||
      lib_path.back() == '/' ||
      lib_path.back() == '\\') {
    SCRAM_THROW(ValidityError("Invalid library path format"))
        << errinfo_value(lib_path)
        << errinfo_element(Element::name(), kTypeString);
  }
  // clang-format on

  boost::dll::load_mode::type load_type = boost::dll::load_mode::default_mode;
  if (decorate)
    load_type |= boost::dll::load_mode::append_decorations;
  if (system)
    load_type |= boost::dll::load_mode::search_system_folders;

  fs::path ref_path = lib_path;
  if (!system || ref_path.has_parent_path())
    ref_path = fs::absolute(ref_path, reference_dir);

  try {
    lib_handle_.load(ref_path, load_type);
  } catch (const boost::system::system_error& err) {
    SCRAM_THROW(DLError(err.what()))
        << boost::errinfo_nested_exception(boost::current_exception())
        << errinfo_element(Element::name(), kTypeString);
  }
}

}  // namespace scram::mef
