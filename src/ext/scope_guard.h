//                     Copyright Yuri Kilochek 2017.
//        Distributed under the Boost Software License, Version 1.0.
//           (See accompanying file LICENSE_1_0.txt or copy at
//                 http://www.boost.org/LICENSE_1_0.txt)

/// @file
/// Scope guard functionality with C++17.
/// Adapted from Yuri Kilochek's Boost.scope_guard.

#pragma once

#include <exception>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/config.hpp>
#include <boost/preprocessor/cat.hpp>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace ext {

namespace detail::scope_guard {

template <typename Fn, typename Args, std::size_t... I>
auto apply_impl(Fn&& fn, Args&& args, std::index_sequence<I...>) noexcept(
    noexcept(std::invoke(std::forward<Fn>(fn),
                         std::get<I>(std::forward<Args>(args))...)))
    -> decltype(std::invoke(std::forward<Fn>(fn),
                            std::get<I>(std::forward<Args>(args))...)) {
  return std::invoke(std::forward<Fn>(fn),
                     std::get<I>(std::forward<Args>(args))...);
}

// Like `std::apply` but SFINAE friendly and propagates `noexcept`ness.
template <class Fn, typename Args>
auto apply(Fn&& fn, Args&& args) noexcept(noexcept(apply_impl(
    std::forward<Fn>(fn), std::forward<Args>(args),
    std::make_index_sequence<std::tuple_size_v<std::decay_t<Args>>>())))
    -> decltype(apply_impl(
        std::forward<Fn>(fn), std::forward<Args>(args),
        std::make_index_sequence<std::tuple_size_v<std::decay_t<Args>>>())) {
  return apply_impl(
      std::forward<Fn>(fn), std::forward<Args>(args),
      std::make_index_sequence<std::tuple_size_v<std::decay_t<Args>>>());
}

template <typename Fn, typename... Args>
class callback {
  Fn m_fn;
  std::tuple<Args...> m_args;

 public:
  template <typename Fn_, typename... Args_,
            std::enable_if_t<(std::is_constructible_v<Fn, Fn_> && ... &&
                              std::is_constructible_v<Args, Args_>)>*...>
  explicit callback(Fn_&& fn, Args_&&... args) noexcept(
      (std::is_nothrow_constructible_v<Fn, Fn_> && ... &&
       std::is_nothrow_constructible_v<Args, Args_>))
      : m_fn(std::forward<Fn_>(fn)), m_args(std::forward<Args_>(args)...) {}

  template <typename Fn_ = Fn>
  auto operator()() noexcept(noexcept(
      (void)scope_guard::apply(std::forward<Fn_>(m_fn), std::move(m_args))))
      -> decltype((void)scope_guard::apply(std::forward<Fn_>(m_fn),
                                           std::move(m_args))) {
    return (void)scope_guard::apply(std::forward<Fn_>(m_fn), std::move(m_args));
  }
};

template <typename... Params>
class base {
 protected:
  using callback_type = detail::scope_guard::callback<Params...>;

  callback_type callback;

 public:
  static_assert(std::is_invocable_v<callback_type>, "callback not invocable");

  template <
      typename... Params_,
      std::enable_if_t<std::is_constructible_v<callback_type, Params...>>*...>
  explicit base(Params_&&... params) noexcept(
      std::is_nothrow_constructible_v<callback_type, Params...>)
      : callback(std::forward<Params_>(params)...) {}

  base(base const&) = delete;
  auto operator=(base const&) -> base& = delete;

  base(base&&) = delete;
  auto operator=(base &&) -> base& = delete;
};

template <typename T>
struct unwrap {
  using type = T;
};

template <typename T>
struct unwrap<std::reference_wrapper<T>> {
  using type = T&;
};

template <typename T>
using unwrap_decay_t = typename unwrap<std::decay_t<T>>::type;
}  // namespace detail::scope_guard

template <typename... Params>
struct scope_guard : detail::scope_guard::base<Params...> {
  using base_type = detail::scope_guard::base<Params...>;
  using this_type = scope_guard;

 public:
  using base_type::base_type;

  ~scope_guard() noexcept(noexcept(this_type::callback()) &&
                          std::is_nothrow_destructible_v<base_type>) {
    this_type::callback();
  }
};

template <typename... Params>
scope_guard(Params&&...)
    ->scope_guard<detail::scope_guard::unwrap_decay_t<Params>...>;

template <typename... Params>
struct scope_guard_failure : detail::scope_guard::base<Params...> {
  using base_type = detail::scope_guard::base<Params...>;
  using this_type = scope_guard_failure;

  int in = std::uncaught_exceptions();

 public:
  using detail::scope_guard::base<Params...>::base;

  ~scope_guard_failure() noexcept(noexcept(this_type::callback()) &&
                                  std::is_nothrow_destructible_v<base_type>) {
    int out = std::uncaught_exceptions();
    if (BOOST_UNLIKELY(out > in)) {
      this_type::callback();
    }
  }
};

template <typename... Params>
scope_guard_failure(Params&&...)
    ->scope_guard_failure<detail::scope_guard::unwrap_decay_t<Params>...>;

template <typename... Params>
struct scope_guard_success : detail::scope_guard::base<Params...> {
  using base_type = detail::scope_guard::base<Params...>;
  using this_type = scope_guard_success;

  int in = std::uncaught_exceptions();

 public:
  using detail::scope_guard::base<Params...>::base;

  ~scope_guard_success() noexcept(noexcept(this_type::callback()) &&
                                  std::is_nothrow_destructible_v<base_type>) {
    int out = std::uncaught_exceptions();
    if (BOOST_LIKELY(out == in)) {
      this_type::callback();
    }
  }
};

template <typename... Params>
scope_guard_success(Params&&...)
    ->scope_guard_success<detail::scope_guard::unwrap_decay_t<Params>...>;

}  // namespace ext

#define SCOPE_EXIT(fn) ext::scope_guard BOOST_PP_CAT(scope_guard_, __LINE__)(fn)

#define SCOPE_FAIL(fn) \
  ext::scope_guard_failure BOOST_PP_CAT(scope_fail_, __LINE__)(fn)

#define SCOPE_SUCCESS(fn) \
  ext::scope_guard_success BOOST_PP_CAT(scope_success_, __LINE__)(fn)

#endif  // DOXYGEN_SHOULD_SKIP_THIS
