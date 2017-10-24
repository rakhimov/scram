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

/// @file extern.cc
/// Implementation of foreign function calls with MEF expressions.

#include "extern.h"

#include <boost/filesystem.hpp>
#include <boost/predef.h>
#include <boost/version.hpp>

#if BOOST_VERSION < 106100

#if !BOOST_OS_LINUX
#error "Dynamic library loading w/o Boost 1.61 is supported only on Linux."
#endif

#include <dlfcn.h>

/// Use POSIX directly on Linux only.
#define DSO_LINUX 1

#else

#include <boost/dll/shared_library.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/system/system_error.hpp>

#endif

#include "src/error.h"

namespace fs = boost::filesystem;

namespace scram {
namespace mef {

#if DSO_LINUX
/// Implementation of external library load facilities.
class ExternLibrary::Pimpl {
 public:
  /// Loads the library for ExternLibrary.
  Pimpl(std::string lib_path, const fs::path& reference_dir, bool system,
        bool decorate) : lib_handle_(nullptr) {
    if (decorate) {
      lib_path += ".so";
      auto pos = lib_path.rfind('/');
      lib_path.insert(pos == std::string::npos ? 0 : (pos + 1), "lib");
    }
    if (!system || lib_path.find('/') != std::string::npos) {
      fs::path abs_path = fs::absolute(lib_path, reference_dir);
      lib_handle_ = dlopen(abs_path.c_str(), RTLD_LAZY);
    } else {
      lib_handle_ = dlopen(lib_path.c_str(), RTLD_LAZY);
    }

    if (!lib_handle_)
      SCRAM_THROW(DLError(dlerror()));
  }

  /// @copydoc ExternLibrary::~ExternLibrary
  ~Pimpl() {
    int err = dlclose(lib_handle_);
    assert(!err && "Failed to close dynamic library.");
  }

  /// Retrieves the symbol from the loaded library.
  void* get(const char* symbol) const {
    dlerror();  // Clear the error message.
    void* fptr = dlsym(lib_handle_, symbol);
    const char* err = dlerror();
    if (!fptr && err)
      SCRAM_THROW(UndefinedElement(err));

    return fptr;
  }

 private:
  void* lib_handle_;  ///< Handle to the library for reference.
};
#else
/// Implementation of external library load facilities.
class ExternLibrary::Pimpl {
 public:
  /// Loads the library for ExternLibrary.
  Pimpl(std::string lib_path, const fs::path& reference_dir, bool system,
        bool decorate) {
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
          << boost::errinfo_nested_exception(boost::current_exception());
    }
  }

  /// Retrieves the symbol from the loaded library.
  void* get(const char* symbol) const {
    try {
      return reinterpret_cast<void*>(lib_handle_.get<void()>(symbol));
    } catch (const boost::system::system_error& err) {
      SCRAM_THROW(UndefinedElement(err.what()))
          << boost::errinfo_nested_exception(boost::current_exception());
    }
  }

 private:
  boost::dll::shared_library lib_handle_;  ///< Shared Library abstraction.
};
#endif

ExternLibrary::ExternLibrary(std::string name, std::string lib_path,
                             const fs::path& reference_dir, bool system,
                             bool decorate)
    : Element(std::move(name)) {
  fs::path fs_path(lib_path);
  std::string filename = fs_path.filename().string();
  if (fs_path.empty() ||
      filename == "." ||
      filename == ".." ||
      lib_path.back() == ':' ||
      lib_path.back() == '/' ||
      lib_path.back() == '\\') {
    SCRAM_THROW(ValidityError("Invalid library path: " + lib_path));
  }

  pimpl_ = new Pimpl(std::move(lib_path), reference_dir, system, decorate);
}

ExternLibrary::~ExternLibrary() { delete pimpl_; }

void* ExternLibrary::get(const char* symbol) const {
  return pimpl_->get(symbol);
}

}  // namespace mef
}  // namespace scram
