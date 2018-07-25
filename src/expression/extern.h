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
/// The MEF facilities to call external functions in expressions.

#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/dll/shared_library.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/system_error.hpp>

#include "src/element.h"
#include "src/error.h"
#include "src/expression.h"

namespace scram::mef {

/// The MEF construct to extend expressions with external libraries.
/// This class dynamically loads and manages libraries.
/// It supports only very basic interface for C function lookup with its symbol.
class ExternLibrary : public Element, public Usage {
 public:
  /// Type string for error messages.
  static constexpr const char* kTypeString = "extern library";

  /// @copydoc Element::Element
  ///
  /// @param[in] lib_path  The library path with its name.
  /// @param[in] reference_dir  The reference directory for relative paths.
  /// @param[in] system  Search for the library in system paths.
  /// @param[in] decorate  Decorate the library name with prefix and suffix.
  ///
  /// @throws ValidityError  The library path is invalid.
  /// @throws DLError  The library cannot be found.
  ExternLibrary(std::string name, std::string lib_path,
                const boost::filesystem::path& reference_dir, bool system,
                bool decorate);

  /// @tparam F  The C free function type.
  ///
  /// @param[in] symbol  The function symbol in the library.
  ///
  /// @returns The function pointer resolved from the symbol.
  ///
  /// @throws DLError  The symbol is not in the library.
  template <typename F>
  std::enable_if_t<std::is_function_v<F>, std::add_pointer_t<F>>
  get(const std::string& symbol) const {
    try {
      return lib_handle_.get<F>(symbol);
    } catch (const boost::system::system_error& err) {
      SCRAM_THROW(DLError(err.what()))
          << errinfo_value(symbol)
          << boost::errinfo_nested_exception(boost::current_exception());
    }
  }

 private:
  boost::dll::shared_library lib_handle_;  ///< Shared Library abstraction.
};

template <typename R, typename... Args>
class ExternFunction;  // Forward declaration to specialize abstract base.

/// Abstract base class for ExternFunction concrete types.
/// This interface hides the return and argument types
/// of generic extern functions and expressions.
///
/// The base acts as a factory for generating expressions with given arguments.
template <>
class ExternFunction<void> : public Element, public Usage {
 public:
  /// Type string for error messages.
  static constexpr const char* kTypeString = "extern function";

  using Element::Element;

  virtual ~ExternFunction() = default;

  /// Applies the function to arguments.
  /// This interface hides the complexity of concrete types of the function.
  ///
  /// @param[in] args  The argument expressions.
  ///
  /// @returns Newly constructed expression as a result of function application.
  ///
  /// @throws ValidityError  The number of arguments is invalid.
  virtual std::unique_ptr<Expression>
  apply(std::vector<Expression*> args) const = 0;
};

using ExternFunctionPtr = std::unique_ptr<ExternFunction<void>>;  ///< Base ptr.
using ExternFunctionBase = ExternFunction<void>;  ///< To help Doxygen.

/// Extern function abstraction to be referenced by expressions.
///
/// @tparam R  Numeric return type.
/// @tparam Args  Numeric argument types.
///
/// @pre The source dynamic library is loaded as long as this function lives.
template <typename R, typename... Args>
class ExternFunction : public ExternFunctionBase {
  static_assert(std::is_arithmetic_v<R>, "Numeric type functions only.");

  using Pointer = R (*)(Args...);  ///< The function pointer type.

 public:
  /// Loads a function from a library for further usage.
  ///
  /// @copydoc Element::Element
  ///
  /// @param[in] symbol  The symbol name for the function in the library.
  /// @param[in] library  The dynamic library to lookup the function.
  ///
  /// @throws DLError  There is no such symbol in the library.
  ExternFunction(std::string name, const std::string& symbol,
                 const ExternLibrary& library)
      : ExternFunctionBase(std::move(name)),
        fptr_(library.get<R(Args...)>(symbol)) {}

  /// Calls the library function with the given numeric arguments.
  R operator()(Args... args) const noexcept { return fptr_(args...); }

  /// @copydoc ExternFunction<void>::apply
  std::unique_ptr<Expression>
  apply(std::vector<Expression*> args) const override;

 private:
  const Pointer fptr_;  ///< The pointer to the extern function in a library.
};

/// Expression evaluating an extern function with expression arguments.
///
/// @tparam R  Numeric return type.
/// @tparam Args  Numeric argument types.
template <typename R, typename... Args>
class ExternExpression
    : public ExpressionFormula<ExternExpression<R, Args...>> {
 public:
  /// @param[in] extern_function  The library function.
  /// @param[in] args  The argument expression for the function.
  ///
  /// @throws ValidityError  The number of arguments is invalid.
  explicit ExternExpression(const ExternFunction<R, Args...>* extern_function,
                            std::vector<Expression*> args)
      : ExpressionFormula<ExternExpression>(std::move(args)),
        extern_function_(*extern_function) {
    if (Expression::args().size() != sizeof...(Args))
      SCRAM_THROW(
          ValidityError("The number of function arguments does not match."));
  }

  /// Computes the extern function with the given evaluator for arguments.
  template <typename F>
  double Compute(F&& eval) noexcept {
    return Marshal(std::forward<F>(eval),
                   std::make_index_sequence<sizeof...(Args)>());
  }

 private:
  /// Evaluates the argument expressions and marshals the result to function.
  /// Marshaller of expressions to extern function calls.
  ///
  /// @tparam F  The expression evaluator type.
  /// @tparam Is  The index sequence for the arguments vector.
  ///
  /// @param[in] eval  The evaluator of the expressions.
  ///
  /// @returns The result of the function call.
  ///
  /// @pre The number of arguments is exactly the same at runtime.
  template <typename F, std::size_t... Is>
  double Marshal(F&& eval, std::index_sequence<Is...>) noexcept {
    return extern_function_(eval(Expression::args()[Is])...);
  }

  const ExternFunction<R, Args...>& extern_function_;  ///< The source function.
};

template <typename R, typename... Args>
std::unique_ptr<Expression>
ExternFunction<R, Args...>::apply(std::vector<Expression*> args) const {
  return std::make_unique<ExternExpression<R, Args...>>(this, std::move(args));
}

}  // namespace scram::mef
