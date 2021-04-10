//===--- TraceConverterCheck.cpp -clang-tidy---------------------*- C++ -*-===//
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
  const auto DerivedTraceClassExpr =
      expr(hasType(cxxRecordDecl(isDerivedFrom("::BaseTrace"))));
  const auto TraceClassExpr =
      expr(hasType(cxxRecordDecl(hasName("::BaseTrace"))));

  StatementMatcher TraceMatcher = traverse(
      TK_AsIs, cxxOperatorCallExpr(
                   hasOverloadedOperatorName("()"),
                   hasArgument(0, anyOf(TraceClassExpr, DerivedTraceClassExpr)),
                   hasArgument(1, stringLiteral()))
                   .bind("trace"));

  Finder->addMatcher(TraceMatcher, this);
}

void TraceConverterCheck::check(const MatchFinder::MatchResult &Result) {
  const unsigned FormatArgOffset = 1;
  const auto *Op = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("trace");

  FormatStringConverter Converter(Result.Context, Op, FormatArgOffset,
                                  getLangOpts());
  if (Converter.canApply()) {
    DiagnosticBuilder Diag =
        diag(Op->getBeginLoc(), "Replace TRACE with fmt equivalent");
    Converter.applyFixes(Diag, *Result.SourceManager);
  }
}

} // namespace fmt
} // namespace tidy
} // namespace clang
