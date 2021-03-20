//===--- TraceConverterCheck.cpp - clang-tidy--------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TraceConverterCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/FormatString.h"
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

class FormatStringConverterHandler : public clang::analyze_format_string::FormatStringHandler {
  size_t PrintfFormatStringPos = 0U;
  const StringRef PrintfFormatString;
  std::string StandardFormatString;

  public:
  explicit FormatStringConverterHandler(const StringRef PrintfFormatStringIn)
      : PrintfFormatString(PrintfFormatStringIn)
  {
    StandardFormatString.reserve(PrintfFormatString.size());
  }

  bool HandlePrintfSpecifier(const analyze_printf::PrintfSpecifier &FS,
                                     const char *startSpecifier,
                             unsigned specifierLen) override;

  std::string getStandardFormatString() && {
    StandardFormatString.append(PrintfFormatString.begin() + PrintfFormatStringPos, PrintfFormatString.end());

    std::string result;
    result.push_back('\"');
    for(const char ch : StandardFormatString) {
        if (ch == '\n')
            result += "\\n";
        else if (ch == '\r')
            result += "\\r";
        else if (ch == '\b')
            result += "\\b";
        else if (ch == '\v')
            result += "\\v";
        else if (ch == '\"')
            result += "\\\"";
        else if (ch == '\\')
            result += "\\\\";
        else
            result += ch;
    }
    result.push_back('\"');

    llvm::outs() << "getStandardFormatString is \"" << StandardFormatString << "\"\n";
    return result;
  }
};

bool FormatStringConverterHandler::HandlePrintfSpecifier(const analyze_printf::PrintfSpecifier &FS,
                                                         const char *startSpecifier,
                                                         unsigned specifierLen) {
  llvm::outs() << "Specifier at " << startSpecifier - PrintfFormatString.data() << " for " << specifierLen << "\n";
  llvm::outs() << "PrintfFormatStringPos is " << PrintfFormatStringPos << "\n";
  llvm::outs() << "StandardFormatString is \"" << StandardFormatString << "\"\n";

  const size_t StartSpecifierPos = startSpecifier - PrintfFormatString.data();
  assert(StartSpecifierPos + specifierLen <= PrintfFormatString.size());

  // Everything before the specifier needs copying verbatim
  assert(StartSpecifierPos >= PrintfFormatStringPos);

  StandardFormatString.append(PrintfFormatString.begin() + PrintfFormatStringPos, PrintfFormatString.begin() + StartSpecifierPos);

  // Now append the standard version of the printf specifier
  StandardFormatString.append("{}");

  // Skip over specifier
  PrintfFormatStringPos = StartSpecifierPos + specifierLen;
  assert(PrintfFormatStringPos <= PrintfFormatString.size());
  return true;
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
  //  using clang::analyze_format_string::FormatStringHandler;

  const StringRef FormatString = Format->getString();

  llvm::outs() << "Format getstring: " << FormatString << "\n";

  FormatStringConverterHandler Handler(FormatString);
  LangOptions LO;
  const bool isFreeBSDKPrintf = false;

  if (ParsePrintfString(Handler,
                        FormatString.data(), FormatString.data() + FormatString.size(),
                        LO, Result.Context->getTargetInfo(), isFreeBSDKPrintf)) {
    printf("Success\n");
  }
  else {
    printf("Failure\n");
  }

#if 0
  auto ReplacementFormatString = clang::StringLiteral::Create(Result.Context,
                                                              FormatString.getKind(),
                                                              FormatString.isPascal(), ...);
#endif


  diag(Format->getBeginLoc(),
       "replace format string")
      << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(Format->getBeginLoc(),
                                                                     Format->getEndLoc()),
                                      std::move(Handler).getStandardFormatString());

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
