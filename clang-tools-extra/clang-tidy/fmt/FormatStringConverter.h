//===--- FormatStringConverter.h - clang-tidy--------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FMT_FORMATSTRINGCONVERTER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FMT_FORMATSTRINGCONVERTER_H

#include "clang/AST/ASTContext.h"
#include "llvm/ADT/Optional.h"
#include <string>

namespace clang {
namespace tidy {
namespace fmt {

class FormatStringResult {
public:
  enum Kind { unsuitable, unchanged, changed };

private:
  Kind ResultKind;
  std::string ConvertedFormatString;
  std::vector<const Expr *> PointerArgs;

public:
  FormatStringResult(std::string ConvertedFormatStringResult,
                     std::vector<const Expr *> PointerArgsIn)
      : ResultKind(changed),
        ConvertedFormatString(std::move(ConvertedFormatStringResult)),
        PointerArgs(std::move(PointerArgsIn)) {}

  FormatStringResult(Kind ResultKindIn) : ResultKind(ResultKindIn) {}

  bool isSuitable() const { return ResultKind != unsuitable; }

  bool isChanged() const { return ResultKind == changed; }

  std::string getString() && { return std::move(ConvertedFormatString); }

  template <typename Callback>
  void forEachPointerArg(const Callback &callback) {
    for (const auto ArgIndex : PointerArgs)
      callback(ArgIndex);
  }
};

/// If PrintfFormatString would change if converted from printf format to {fmt}
/// format then return a string containing the equivalent {fmt} format.
/// Otherwise return None. Throws
FormatStringResult printfFormatStringToFmtString(
    const ASTContext *Context, const StringRef PrintfFormatString,
    const Expr *const *PrintfArgs, unsigned PrintfNumArgs);

} // namespace fmt
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FMT_FORMATSTRINGCONVERTER_H
