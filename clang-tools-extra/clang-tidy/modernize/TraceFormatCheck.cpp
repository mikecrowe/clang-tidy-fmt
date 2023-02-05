//===--- TraceConverterCheck.cpp -clang-tidy---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TraceFormatCheck.h"
#include "../utils/FormatStringConverter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/FormatString.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang::tidy::modernize {

namespace {
AST_MATCHER(StringLiteral, isOrdinary) { return Node.isOrdinary(); }
} // namespace

void TraceFormatCheck::registerMatchers(MatchFinder *Finder) {
  const auto DerivedTraceClassExpr =
      expr(hasType(cxxRecordDecl(isDerivedFrom("::BaseTrace"))));
  const auto TraceClassExpr =
      expr(hasType(cxxRecordDecl(hasName("::BaseTrace"))));
  const auto DerivedNullTraceClassExpr =
      expr(hasType(cxxRecordDecl(isDerivedFrom("::NullTrace"))));
  const auto NullTraceClassExpr =
      expr(hasType(cxxRecordDecl(hasName("::NullTrace"))));

  Finder->addMatcher(
      cxxOperatorCallExpr(
          hasOverloadedOperatorName("()"),
          hasArgument(0, anyOf(TraceClassExpr, DerivedTraceClassExpr,
                               NullTraceClassExpr, DerivedNullTraceClassExpr)),
          hasArgument(1, stringLiteral(isOrdinary())))
          .bind("trace"),
      this);
}

void TraceFormatCheck::check(const MatchFinder::MatchResult &Result) {
  const unsigned FormatArgOffset = 1;
  const auto *Op = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("trace");

  const LangOptions &LangOpts = getLangOpts();
  utils::FormatStringConverter Converter(Result.Context, Op, FormatArgOffset,
                                         {}, LangOpts);
  if (!Converter.canApply()) {
    DiagnosticBuilder Diag = diag(Op->getBeginLoc(),
                                  "unable to use std::format syntax in TRACE because %0")
                             << Converter.conversionNotPossibleReason();
    return;
  }

  DiagnosticBuilder Diag =
    diag(Op->getBeginLoc(), "Replace printf-style TRACE with std::format equivalent");
#if 1
  SourceLocation InsertionPoint = Op->getCallee()->getExprLoc();
  Diag << FixItHint::CreateInsertion(InsertionPoint, ".FMT_STYLE");
#endif
  Converter.applyFixes(Diag, *Result.SourceManager);
}

} // namespace clang::tidy::modernize
