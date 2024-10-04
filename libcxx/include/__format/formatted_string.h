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

_LIBCPP_BEGIN_NAMESPACE_STD

template <typename CharT> struct __select_format_context {};
template <> struct __select_format_context<char> {
  using type = std::format_context;
};
template <> struct __select_format_context<wchar_t> {
  using type = std::wformat_context;
};

template <typename CharT> using __select_format_context_t = __select_format_context<CharT>::type;

template <typename CharT, typename... Args>
struct basic_formatted_string {
  basic_formatted_string(std::basic_format_string<CharT, Args...> fmt, const Args&... as)
      : args(std::make_format_args<__select_format_context_t<CharT>>(as...)), literal(fmt.get()) {}

  operator std::string() const { return std::vformat(literal, args); }

  decltype(std::make_format_args<__select_format_context_t<CharT>>(std::declval<const Args&>()...)) args;
  std::basic_string_view<CharT> literal;
};

template <typename... Args>
auto make_formatted_string(std::format_string<Args...> fmt, Args&&... as) {
  return basic_formatted_string<char, Args...>(fmt, as...);
}
template <typename... Args>
auto make_formatted_string(std::wformat_string<Args...> fmt, Args&&... as) {
  return basic_formatted_string<wchar_t, Args...>(fmt, as...);
}

template <typename CharT, typename... Args>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const basic_formatted_string<CharT, Args...>& fs) {
  std::vformat_to(std::ostream_iterator<CharT, CharT>(os), fs.literal, fs.args);
  return os;
}

template <typename CharT, typename... Args>
void print(std::basic_ostream<CharT>& os, const basic_formatted_string<CharT, Args...>& fs) {
  os << fs;
}
template <typename CharT, typename... Args>
void println(std::basic_ostream<CharT>& os, const basic_formatted_string<CharT, Args...>& fs) {
  os << fs << std::endl; // Or just \n?
}

template <typename... Args>
void print(const basic_formatted_string<char, Args...>& fs) {
  print(std::cout, fs);
}

template <typename... Args>
void print(const basic_formatted_string<wchar_t, Args...>& fs) {
  print(std::wcout, fs);
}

template <typename... Args>
void println(const basic_formatted_string<char, Args...>& fs) {
  println(std::cout, fs);
}

template <typename... Args>
void println(const basic_formatted_string<wchar_t, Args...>& fs) {
  println(std::wcout, fs);
}

_LIBCPP_END_NAMESPACE_STD

#error here

#endif
