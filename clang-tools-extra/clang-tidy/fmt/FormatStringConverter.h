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
#include "clang/AST/FormatString.h"
#include "llvm/ADT/Optional.h"
#include <string>

namespace clang {
namespace tidy {
namespace fmt {

/// Convert a printf-style format string to a libfmt-style one. This class is
/// expecting to work on the already-cooked format string (i.e. all the escapes
/// have been converted) so we have to convert them back. This means that we
/// might not convert them back using the same form.
class FormatStringConverter
    : public clang::analyze_format_string::FormatStringHandler {
  const ASTContext *Context;
  bool ConversionPossible = true;
  bool FormatStringNeededRewriting = false;
  size_t PrintfFormatStringPos = 0U;
  StringRef PrintfFormatString;

  const Expr *const *Args;
  const unsigned NumArgs;
  unsigned ArgsOffset;

  const StringLiteral *FormatExpr;
  std::string StandardFormatString;
  std::vector<const Expr *> PointerArgs;
  const LangOptions &LangOpts;

  bool HandlePrintfSpecifier(const analyze_printf::PrintfSpecifier &FS,
                             const char *StartSpecifier,
                             unsigned SpecifierLen) override;

  std::string getStandardFormatString();

public:
  FormatStringConverter(const ASTContext *Context, const CallExpr *Call,
                        unsigned FormatArgOffset, const LangOptions &LO);

  bool canApply() const { return ConversionPossible; }
  void applyFixes(DiagnosticBuilder &Diag, SourceManager &SM);
};

} // namespace fmt
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FMT_FORMATSTRINGCONVERTER_H
