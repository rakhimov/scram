/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/* This file is added in order to prevent the "too many sections &
 * file too big" error triggered on mingw-w64 32bit platform, debug build
 *
 * The reason for this error is the giant recursive template funtion
 * Initializer::DefineExternFunction.
 * Moving it to an independent file largely reduces the size of the
 * produced obj file
 */


#include "initializer.h"

#include <functional>  // std::mem_fn
#include <sstream>
#include <type_traits>

#include <boost/exception/errinfo_at_line.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/algorithm.hpp>

#include "cycle.h"
#include "env.h"
#include "error.h"
#include "expression/boolean.h"
#include "expression/conditional.h"
#include "expression/exponential.h"
#include "expression/extern.h"
#include "expression/numerical.h"
#include "expression/random_deviate.h"
#include "expression/test_event.h"
#include "ext/algorithm.h"
#include "ext/find_iterator.h"
#include "logger.h"

namespace scram::mef {

namespace {  // Extern function initialization helpers.

/// All the allowed extern function parameter types.
///
/// @note Template code may be optimized for these types only.
enum class ExternParamType { kInt = 1, kDouble };
const int kExternTypeBase = 3;  ///< The information base for encoding.
const int kMaxNumParam = 5;  ///< The max number of args (excludes the return).
const int kNumInterfaces = 126;  ///< All possible interfaces.

/// Encodes parameter types kExternTypeBase base number.
///
/// @tparam SinglePassRange  The forward range type
///
/// @param[in] args  The non-empty XML elements encoding parameter types.
///
/// @returns Unique integer encoding parameter types.
///
/// @pre The number of parameters is less than log_base(max int).
template <class SinglePassRange>
int Encode(const SinglePassRange& args) noexcept {
  assert(!args.empty());
  auto to_digit = [](const xml::Element& node) -> int {
    std::string_view name = node.name();
    return static_cast<int>([&name] {
      if (name == "int")
        return ExternParamType::kInt;
      assert(name == "double");
      return ExternParamType::kDouble;
    }());
  };

  int ret = 0;
  int base_power = 1;  // Base ^ (pos - 1).
  for (const xml::Element& node : args) {
    ret += base_power * to_digit(node);
    base_power *= kExternTypeBase;
  }

  return ret;
}

/// Encodes function parameter types at compile-time.
template <typename T, typename... Ts>
constexpr int Encode(int base_power = 1) noexcept {
  if constexpr (sizeof...(Ts)) {
    return Encode<T>(base_power) + Encode<Ts...>(base_power * kExternTypeBase);

  } else if constexpr (std::is_same_v<T, int>) {
    return base_power * static_cast<int>(ExternParamType::kInt);

  } else {
    static_assert(std::is_same_v<T, double>);
    return base_power * static_cast<int>(ExternParamType::kDouble);
  }
}

using ExternFunctionExtractor = ExternFunctionPtr (*)(std::string,
                                                      const std::string&,
                                                      const ExternLibrary&);
using ExternFunctionExtractorMap =
    std::unordered_map<int, ExternFunctionExtractor>;

/// Generates all extractors for extern functions.
///
/// @tparam N  The number of parameters.
/// @tparam Ts  The return and parameter types of the extern function.
///
/// @param[in,out] function_map  The destination container for extractor.
template <int N, typename... Ts>
void GenerateExternFunctionExtractor(ExternFunctionExtractorMap* function_map) {
  static_assert(N >= 0);
  static_assert(sizeof...(Ts));

  if constexpr (N == 0) {
    function_map->emplace(
        Encode<Ts...>(),
        [](std::string name, const std::string& symbol,
           const ExternLibrary& library) -> ExternFunctionPtr {
          return std::make_unique<ExternFunction<Ts...>>(std::move(name),
                                                         symbol, library);
        });
  } else {
    GenerateExternFunctionExtractor<0, Ts...>(function_map);
    GenerateExternFunctionExtractor<N - 1, Ts..., int>(function_map);
    GenerateExternFunctionExtractor<N - 1, Ts..., double>(function_map);
  }
}

}  // namespace

void Initializer::DefineExternFunction(const xml::Element& xml_element) {
  static const ExternFunctionExtractorMap function_extractors = [] {
    ExternFunctionExtractorMap function_map;
    function_map.reserve(kNumInterfaces);
    GenerateExternFunctionExtractor<kMaxNumParam, int>(&function_map);
    GenerateExternFunctionExtractor<kMaxNumParam, double>(&function_map);
    assert(function_map.size() == kNumInterfaces);
    return function_map;
  }();

  const ExternLibrary& library = [this, &xml_element]() -> decltype(auto) {
    try {
      auto& lib = model_->Get<ExternLibrary>(xml_element.attribute("library"));
      lib.usage(true);
      return lib;
    } catch (UndefinedElement& err) {
      err << boost::errinfo_at_line(xml_element.line());
      throw;
    }
  }();

  ExternFunctionPtr extern_function = [&xml_element, &library] {
    auto args = GetNonAttributeElements(xml_element);
    assert(!args.empty());
    /// @todo Optimize extern-function num args violation detection.
    int num_args = std::distance(args.begin(), args.end()) - /*return*/ 1;
    if (num_args > kMaxNumParam) {
      SCRAM_THROW(ValidityError("The number of function parameters '" +
                                std::to_string(num_args) +
                                "' exceeds the number of allowed parameters '" +
                                std::to_string(kMaxNumParam) + "'"))
          << boost::errinfo_at_line(xml_element.line());
    }
    int encoding = Encode(args);
    try {
      return function_extractors.at(encoding)(
          std::string(xml_element.attribute("name")),
          std::string(xml_element.attribute("symbol")), library);
    } catch (Error& err) {
      err << boost::errinfo_at_line(xml_element.line());
      throw;
    }
  }();

  Register(std::move(extern_function), xml_element);
}


}
