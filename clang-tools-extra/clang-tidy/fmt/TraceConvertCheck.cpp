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
  // this counts as an argument
  const unsigned FormatArgOffset = 2;
  llvm::outs() << "Operator call\n";
  const auto *Op = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("trace");

  Op->dumpPretty(*Result.Context);
  llvm::outs() << "\n\n";

  const auto *OpArgs = Op->getArgs();
  const auto OpNumArgs = Op->getNumArgs();

  llvm::outs() << "Format string\n";
  const auto *Format = llvm::dyn_cast<clang::StringLiteral>(
      OpArgs[FormatArgOffset - 1]->IgnoreImplicitAsWritten());
  Format->dumpPretty(*Result.Context);
  llvm::outs() << "\n\n";

  using clang::analyze_format_string::ParsePrintfString;

  const StringRef FormatString = Format->getString();

  llvm::outs() << "Format getstring: " << FormatString << "\n";

  auto ReplacementFormat = printfFormatStringToFmtString(
      Result.Context, FormatString, OpArgs + FormatArgOffset,
      OpNumArgs - FormatArgOffset);
  if (ReplacementFormat.isChanged()) {
    DiagnosticBuilder Diag =
        diag(Format->getBeginLoc(), "Replace TRACE format string");
    Diag << FixItHint::CreateReplacement(
        CharSourceRange::getTokenRange(Format->getBeginLoc(),
                                       Format->getEndLoc()),
        std::move(ReplacementFormat).getString());
  }
}

} // namespace fmt
} // namespace tidy
} // namespace clang
