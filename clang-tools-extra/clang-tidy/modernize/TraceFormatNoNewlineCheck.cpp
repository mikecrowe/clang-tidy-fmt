//===--- TraceNoNewlineCheck.cpp -clang-tidy---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TraceFormatNoNewlineCheck.h"
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

void TraceFormatNoNewlineCheck::registerMatchers(MatchFinder *Finder) {
  const auto DerivedTraceClassExpr =
      expr(hasType(cxxRecordDecl(isDerivedFrom("::BaseTrace"))));
  const auto TraceClassExpr =
      expr(hasType(cxxRecordDecl(hasName("::BaseTrace"))));

  Finder->addMatcher(
      cxxMemberCallExpr(
          on(anyOf(TraceClassExpr, DerivedTraceClassExpr)),
          callee(cxxMethodDecl(hasName("F"))),
          hasArgument(0, stringLiteral(isOrdinary()).bind("FormatString"))),
      this);
}

void TraceFormatNoNewlineCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *FormatStringExpr =
      Result.Nodes.getNodeAs<StringLiteral>("FormatString");

  SourceLocation FormatStringLoc = FormatStringExpr->getExprLoc();
  StringRef FormatString =
      Lexer::getSourceText(CharSourceRange::getTokenRange(FormatStringLoc),
                           *Result.SourceManager, LangOptions());

  if (FormatString.ends_with("\\n\"") && !FormatString.ends_with("\\\\n\"")) {
    std::string FixedFormatString = FormatString.str();
    FixedFormatString.erase(FixedFormatString.end() - 3,
                            FixedFormatString.end() - 1);
    diag(FormatStringLoc, "unwanted newline in TRACE.F format string")
        << FixItHint::CreateReplacement(FormatStringLoc, FixedFormatString);
  }
}

} // namespace clang::tidy::modernize
