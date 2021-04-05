//===--- TraceConverterCheck.cpp - clang-tidy--------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TraceConvertCheck.h"
#include "FormatStringConverter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/FormatString.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace fmt {

void TraceConverterCheck::registerMatchers(MatchFinder *Finder) {
  const auto DerivedTraceClassExpr = expr(hasType(cxxRecordDecl(isDerivedFrom("::BaseTrace"))));
  const auto TraceClassExpr = expr(hasType(cxxRecordDecl(hasName("::BaseTrace"))));

  StatementMatcher TraceMatcher =
      traverse(TK_AsIs,
               cxxOperatorCallExpr(hasOverloadedOperatorName("()"),
                                   hasArgument(0, anyOf(TraceClassExpr, DerivedTraceClassExpr)),
                                   hasArgument(1, stringLiteral().bind("format"))
                                   ).bind("trace"));

  Finder->addMatcher(TraceMatcher, this);
}

void TraceConverterCheck::check(const MatchFinder::MatchResult &Result) {
  llvm::outs() << "Operator call\n";
  const auto *OpCall = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("trace");
  OpCall->dumpPretty(*Result.Context);
  llvm::outs() << "\n\n";

  llvm::outs() << "Format string\n";
  const auto *Format = Result.Nodes.getNodeAs<clang::StringLiteral>("format");
  Format->dumpPretty(*Result.Context);
  llvm::outs() << "\n\n";

  using clang::analyze_format_string::ParsePrintfString;

  const StringRef FormatString = Format->getString();

  llvm::outs() << "Format getstring: " << FormatString << "\n";

  const auto MaybeReplacementFormatString =
      printfFormatStringToFmtString(Result.Context, FormatString);
  if (MaybeReplacementFormatString) {
      DiagnosticBuilder Diag =
          diag(Format->getBeginLoc(), "Replace TRACE format string");
      Diag << FixItHint::CreateReplacement(
          CharSourceRange::getTokenRange(Format->getBeginLoc(),
                                         Format->getEndLoc()),
          *MaybeReplacementFormatString);
  }
}

}  // namespace fmt
}  // namespace tidy
}  // namespace clang
