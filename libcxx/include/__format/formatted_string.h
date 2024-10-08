// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FORMAT_FORMATTED_STRING_H
#define _LIBCPP___FORMAT_FORMATTED_STRING_H

#if _LIBCPP_STD_VER >= 23       // Should be 26

_LIBCPP_BEGIN_NAMESPACE_STD

template <typename CharT> struct __select_format_context {};
template <> struct __select_format_context<char> {
  using type = std::format_context;
};
template <> struct __select_format_context<wchar_t> {
  using type = std::wformat_context;
};

template <typename CharT> using __select_format_context_t = __select_format_context<CharT>::type;

template <typename CharT, typename... _Args>
struct basic_formatted_string {
  basic_formatted_string(std::basic_format_string<CharT, _Args...> fmt, const _Args&... as)
      : args(std::make_format_args<__select_format_context_t<CharT>>(as...)), literal(fmt.get()) {}

  operator std::string() const { return std::vformat(literal, args); }

  decltype(std::make_format_args<__select_format_context_t<CharT>>(std::declval<const _Args&>()...)) args;
  std::basic_string_view<CharT> literal;
};

template<typename... _Args>
using formatted_string = basic_formatted_string<char, _Args...>;

template<typename... _Args>
using wformatted_string = basic_formatted_string<wchar_t, _Args...>;

template <typename... _Args>
auto make_formatted_string(std::format_string<_Args...> fmt, _Args&&... as) {
  return basic_formatted_string<char, _Args...>(fmt, as...);
}
template <typename... _Args>
auto make_formatted_string(std::wformat_string<_Args...> fmt, _Args&&... as) {
  return basic_formatted_string<wchar_t, _Args...>(fmt, as...);
}

_LIBCPP_END_NAMESPACE_STD

#endif

#endif
