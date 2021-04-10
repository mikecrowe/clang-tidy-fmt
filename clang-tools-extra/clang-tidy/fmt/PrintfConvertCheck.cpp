//===--- PrintfConvertCheck.cpp - clang-tidy---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PrintfConvertCheck.h"
#include "FormatStringConverter.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace fmt {

void PrintfConvertCheck::registerMatchers(MatchFinder *Finder) {
  StatementMatcher PrintfMatcher =
      traverse(TK_AsIs, callExpr(callee(functionDecl(hasName("printf"))),
                                 hasArgument(0, stringLiteral()))
               .bind("printf"));
  Finder->addMatcher(PrintfMatcher, this);

  StatementMatcher FprintfMatcher =
      traverse(TK_AsIs, callExpr(callee(functionDecl(hasName("fprintf"))),
                                 hasArgument(1, stringLiteral()))
               .bind("fprintf"));
  Finder->addMatcher(FprintfMatcher, this);
}

void PrintfConvertCheck::check(const MatchFinder::MatchResult &Result) {
  unsigned FormatArgOffset = 1;
  const auto *Printf = Result.Nodes.getNodeAs<CallExpr>("printf");
  if (!Printf) {
    Printf = Result.Nodes.getNodeAs<CallExpr>("fprintf");
    FormatArgOffset = 2;
  }
  const auto *PrintfCall = Printf->getCallee();
  const auto *PrintfArgs = Printf->getArgs();
  const auto PrintfNumArgs = Printf->getNumArgs();
  const auto *Format = llvm::dyn_cast<clang::StringLiteral>(PrintfArgs[FormatArgOffset - 1]->IgnoreImplicitAsWritten());
  const StringRef FormatString = Format->getString();

  auto ReplacementFormat = printfFormatStringToFmtString(
      Result.Context, FormatString, PrintfArgs + FormatArgOffset,
      PrintfNumArgs - FormatArgOffset);

  if (ReplacementFormat.isSuitable()) {
    DiagnosticBuilder Diag =
        diag(PrintfCall->getBeginLoc(), "Replace printf with fmt::print");
    Diag << FixItHint::CreateReplacement(
        CharSourceRange::getTokenRange(PrintfCall->getBeginLoc(),
                                       PrintfCall->getEndLoc()),
        "fmt::print");

    if (ReplacementFormat.isChanged()) {
      Diag << FixItHint::CreateReplacement(
          CharSourceRange::getTokenRange(Format->getBeginLoc(),
                                         Format->getEndLoc()),
          std::move(ReplacementFormat).getString());
    }

    ReplacementFormat.forEachPointerArg(
        [&Diag, &Result, this](const Expr *Arg) {
          llvm::outs() << "Adding hint\n";
          SourceLocation AfterOtherSide =
              Lexer::findNextToken(Arg->getEndLoc(), *Result.SourceManager,
                                   getLangOpts())
                  ->getLocation();

          Diag << FixItHint::CreateInsertion(Arg->getBeginLoc(), "fmt::ptr(")
               << FixItHint::CreateInsertion(AfterOtherSide, ")");
          //           << FixItHint::CreateInsertion(
          //                  Arg->getEndLoc().getLocWithOffset(1), ")");
        });
  }
}

} // namespace fmt
} // namespace tidy
} // namespace clang
