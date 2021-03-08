//===--- TraceConverterCheck.cpp - clang-tidy--------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TraceConverterCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace fmt {

//    |-CXXOperatorCallExpr 0x55dda8d48be8 <line:24:5, col:20> 'void' '()'
//    | |-ImplicitCastExpr 0x55dda8d48bb8 <col:10, col:20> 'void (*)(const char *)' <FunctionToPointerDecay>
//    | | `-DeclRefExpr 0x55dda8d48b68 <col:10, col:20> 'void (const char *)' lvalue CXXMethod 0x55dda8d48a68 'operator()' 'void (const char *)'
//    | |-DeclRefExpr 0x55dda8d48848 <col:5> 'NullTrace' lvalue Var 0x55dda8d1ad88 'TRACE' 'NullTrace'
//    | `-ImplicitCastExpr 0x55dda8d48bd0 <col:11> 'const char *' <ArrayToPointerDecay>
//    |   `-StringLiteral 0x55dda8d488e8 <col:11> 'const char [7]' lvalue "Hello\n"

void TraceConverterCheck::registerMatchers(MatchFinder *Finder) {
  const auto DerivedTraceClassExpr = expr(hasType(cxxRecordDecl(isDerivedFrom("::NullTrace"))));
  const auto TraceClassExpr = expr(hasType(cxxRecordDecl(hasName("::NullTrace"))));

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

  diag(Format->getBeginLoc(),
       "replace format string")
      << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(Format->getBeginLoc(),
                                                                     Format->getEndLoc()),
                                      "wibble");
#if 0
  const auto *Op = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("Op");
  const auto *Call = Result.Nodes.getNodeAs<CallExpr>("Call");
  assert(Op != nullptr && Call != nullptr && "Matcher does not work as expected");

  // Handles the case 'x = absl::StrCat(x)', which has no effect.
  if (Call->getNumArgs() == 1) {
    diag(Op->getBeginLoc(), "call to 'absl::StrCat' has no effect");
    return;
  }

  // Emit a warning and emit fixits to go from
  //   x = absl::StrCat(x, ...)
  // to
  //   absl::StrAppend(&x, ...)
  diag(Op->getBeginLoc(),
       "call 'absl::StrAppend' instead of 'absl::StrCat' when appending to a "
       "string to avoid a performance penalty")
      << FixItHint::CreateReplacement(
             CharSourceRange::getTokenRange(Op->getBeginLoc(),
                                            Call->getCallee()->getEndLoc()),
             "absl::StrAppend")
      << FixItHint::CreateInsertion(Call->getArg(0)->getBeginLoc(), "&");
#endif
}

}  // namespace fmt
}  // namespace tidy
}  // namespace clang
