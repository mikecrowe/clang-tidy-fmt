//===--- StrPrintfConvertCheck.cpp - clang-tidy------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "StrPrintfConvertCheck.h"
#include "FormatStringConverter.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace fmt {

void StrPrintfConvertCheck::registerMatchers(MatchFinder *Finder) {
  StatementMatcher StrPrintfMatcher =
      traverse(TK_AsIs, callExpr(callee(functionDecl(hasName("strprintf"))),
                                 hasArgument(0, stringLiteral()))
                            .bind("strprintf"));
  Finder->addMatcher(StrPrintfMatcher, this);
}

void StrPrintfConvertCheck::check(const MatchFinder::MatchResult &Result) {
  const unsigned FormatArgOffset = 0;
  const auto *StrPrintf = Result.Nodes.getNodeAs<CallExpr>("strprintf");

  FormatStringConverter Converter(Result.Context, StrPrintf, FormatArgOffset,
                                  getLangOpts());
  if (Converter.canApply()) {
    const auto *StrPrintfCall = StrPrintf->getCallee();
    DiagnosticBuilder Diag = diag(StrPrintfCall->getBeginLoc(),
                                  "Replace strprintf with fmt::format");
    Diag << FixItHint::CreateReplacement(
        CharSourceRange::getTokenRange(StrPrintfCall->getBeginLoc(),
                                       StrPrintfCall->getEndLoc()),
        "fmt::format");
    Converter.applyFixes(Diag, *Result.SourceManager);
  }
}

} // namespace fmt
} // namespace tidy
} // namespace clang
