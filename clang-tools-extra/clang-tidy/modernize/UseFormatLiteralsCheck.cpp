//===--- UseFormatLiteralsCheck.cpp - clang-tidy --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UseFormatLiteralsCheck.h"
#include "../utils/Matchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/FixIt.h"

using namespace clang::ast_matchers;

namespace clang::tidy::modernize {

void UseFormatLiteralsCheck::registerMatchers(MatchFinder *Finder) {
  std::vector<StringRef> FormatFunctions = {"std::format", "std::print"};

  Finder->addMatcher(
      callExpr(
          argumentCountAtLeast(1),
          hasArgument(0,
                      hasType(qualType(hasCanonicalType(hasDeclaration(
                          recordDecl(hasName("std::basic_format_string"))))))),
          callee(functionDecl(matchers::matchesAnyListedName(FormatFunctions))))
          .bind("func_call"),
      this);
}

void UseFormatLiteralsCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *FormatCall = Result.Nodes.getNodeAs<CallExpr>("func_call");
  const StringLiteral *FormatExpr = llvm::dyn_cast<StringLiteral>(
      FormatCall->getArg(0)->IgnoreImplicitAsWritten());

  assert(FormatExpr);
  assert(FormatExpr->isOrdinary());

  std::string RewrittenFormatString{"\""};
  StringRef InputFormatString = FormatExpr->getString();

  // This can probably be made more efficient by using Twine
  size_t ArgIndex = 1;
  for (unsigned I = 0, E = InputFormatString.size(); I != E; ++I) {
    const char C = InputFormatString[I];
    if (C == '{') {
      if (I + 1 < E && FormatExpr->getString()[I + 1] == '{') {
        // Escaped '{'.
        RewrittenFormatString += "{{";
        ++I;
        continue;
      }

      RewrittenFormatString += "{";
      RewrittenFormatString +=
          tooling::fixit::getText(*FormatCall->getArg(ArgIndex++),
                                  *Result.Context)
              .str();
    } else
      RewrittenFormatString += C;
  }

  RewrittenFormatString += "\"";

  llvm::dbgs() << "Rewritten format string: " << RewrittenFormatString << "\n";

  const SourceLocation ArgsBeginLoc = FormatCall->getArg(0)->getBeginLoc();
  const SourceLocation ArgsEndLoc =
      FormatCall->getArg(FormatCall->getNumArgs() - 1)->getEndLoc();

  // Create a CharSourceRange covering the entire argument list, including
  // parentheses.
  const CharSourceRange ArgsRange =
      CharSourceRange::getTokenRange(ArgsBeginLoc, ArgsEndLoc);

  diag(ArgsRange.getBegin(), "use format literals")
      << FixItHint::CreateReplacement(ArgsRange, RewrittenFormatString);
}

} // namespace clang::tidy::modernize
