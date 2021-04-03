//===--- PrintfConvertCheck.cpp - clang-tidy--------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PrintfConvertCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "FormatStringConverter.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace fmt {

void PrintfConvertCheck::registerMatchers(MatchFinder *Finder) {
    StatementMatcher PrintfMatcher =
        traverse(TK_AsIs,
                 callExpr(callee(functionDecl(hasName("printf"))),
                          hasArgument(0, stringLiteral().bind("format"))).bind("printf"));
    Finder->addMatcher(PrintfMatcher, this);

    StatementMatcher FprintfMatcher =
        traverse(TK_AsIs,
                 callExpr(callee(functionDecl(hasName("fprintf"))),
                          hasArgument(1, stringLiteral().bind("format"))).bind("printf"));
    Finder->addMatcher(FprintfMatcher, this);
}

void PrintfConvertCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *PrintfCall = Result.Nodes.getNodeAs<CallExpr>("printf")->getCallee();
  const auto *Format = Result.Nodes.getNodeAs<clang::StringLiteral>("format");
  const StringRef FormatString = Format->getString();

  DiagnosticBuilder Diag = diag(PrintfCall->getBeginLoc(), "Replace printf with fmt::print");
  Diag << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(PrintfCall->getBeginLoc(),
                                                                      PrintfCall->getEndLoc()),
                                       "fmt::print");

  const auto MaybeReplacementFormatString = printfFormatStringToFmtString(Result.Context, FormatString);
  if (MaybeReplacementFormatString)
    Diag << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(Format->getBeginLoc(),
                                                                        Format->getEndLoc()),
                                         *MaybeReplacementFormatString);
}

}  // namespace fmt
}  // namespace tidy
}  // namespace clang
