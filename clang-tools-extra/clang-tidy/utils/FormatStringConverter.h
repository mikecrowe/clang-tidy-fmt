//===---------- FormatStringConverter.h - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FORMATSTRINGCONVERTER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FORMATSTRINGCONVERTER_H

#include <string>

namespace clang {
namespace tidy {
namespace utils {

// Returns a fmt::format/std::format format string corresponding to a printf
// format string.
std::string stdFormatStringFromPrintfFormatString(const std::string &PrintfFormat);

} // namespace utils
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FORMATSTRINGCONVERTER_H
