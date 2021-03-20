//===---------- FormatStringConverter.cpp - clang-tidy --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FormatStringConverter.h"
#include "clang/AST/FormatString.h"
#include "clang/Basic/TargetInfo.h"

namespace clang {
namespace tidy {
namespace utils {

std::string stdFormatStringFromPrintfFormatString(const std::string &PrintfFormat)
{
#if 0
  using clang::analyze_format_string::ParsePrintfString;

  FormatStringHandler Handler;
  LangOptions LO;
  clang::TargetInfo Target;
  const bool isFreeBSDKPrintf = false;

  if (ParsePrintfString(Handler, PrintfFormat.data() + PrintfFormat.size(),
                        LO, Target, isFreeBSDKPrintf)) {
    printf("Success\n");
  }
  else {
    printf("Failure\n");
  }
#endif
  return {};
}

} // namespace utils
} // namespace tidy
} // namespace clang
