//===--- PrintfConvertCheck.cpp - clang-tidy--------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PrintfConvertCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/FormatString.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace fmt {

// `-FunctionDecl 0x559a988524e0 <../clang-tools-extra/test/clang-tidy/checkers/fmt-printf-convert.cpp:5:1, line:9:1> line:5:6 f 'void ()'
//   `-CompoundStmt 0x559a98852768 <col:10, line:9:1>
//     `-CallExpr 0x559a98852720 <line:6:3, col:26> 'int'
//       |-ImplicitCastExpr 0x559a98852708 <col:3> 'int (*)(const char *__restrict, ...)' <FunctionToPointerDecay>
//       | `-DeclRefExpr 0x559a98852690 <col:3> 'int (const char *__restrict, ...)' lvalue Function 0x559a98837ef8 'printf' 'int (const char *__restrict, ...)'
//       |-ImplicitCastExpr 0x559a98852750 <col:10> 'const char *' <ArrayToPointerDecay>
//       | `-StringLiteral 0x559a98852648 <col:10> 'const char [10]' lvalue "Hello %d\n"
//       `-IntegerLiteral 0x559a98852670 <col:24> 'int' 42

//    |-CXXOperatorCallExpr 0x55dda8d48be8 <line:24:5, col:20> 'void' '()'
//    | |-ImplicitCastExpr 0x55dda8d48bb8 <col:10, col:20> 'void (*)(const char *)' <FunctionToPointerDecay>
//    | | `-DeclRefExpr 0x55dda8d48b68 <col:10, col:20> 'void (const char *)' lvalue CXXMethod 0x55dda8d48a68 'operator()' 'void (const char *)'
//    | |-DeclRefExpr 0x55dda8d48848 <col:5> 'NullTrace' lvalue Var 0x55dda8d1ad88 'TRACE' 'NullTrace'
//    | `-ImplicitCastExpr 0x55dda8d48bd0 <col:11> 'const char *' <ArrayToPointerDecay>
//    |   `-StringLiteral 0x55dda8d488e8 <col:11> 'const char [7]' lvalue "Hello\n"

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

class FormatStringConverter : public clang::analyze_format_string::FormatStringHandler {
  size_t PrintfFormatStringPos = 0U;
  const StringRef PrintfFormatString;
  std::string StandardFormatString;

  public:
  explicit FormatStringConverter(const StringRef PrintfFormatStringIn)
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
        if (ch == '\a')
            result += "\\a";
        else if (ch == '\b')
            result += "\\b";
        else if (ch == '\f')
            result += "\\f";
        else if (ch == '\n')
            result += "\\n";
        else if (ch == '\r')
            result += "\\r";
        else if (ch == '\t')
            result += "\\t";
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

bool FormatStringConverter::HandlePrintfSpecifier(const analyze_printf::PrintfSpecifier &FS,
                                                         const char *startSpecifier,
                                                         unsigned specifierLen) {
  using namespace analyze_printf;

  llvm::outs() << "Specifier at " << startSpecifier - PrintfFormatString.data() << " for " << specifierLen << "\n";
  llvm::outs() << "PrintfFormatStringPos is " << PrintfFormatStringPos << "\n";
  llvm::outs() << "StandardFormatString is \"" << StandardFormatString << "\"\n";

  const size_t StartSpecifierPos = startSpecifier - PrintfFormatString.data();
  assert(StartSpecifierPos + specifierLen <= PrintfFormatString.size());

  // Everything before the specifier needs copying verbatim
  assert(StartSpecifierPos >= PrintfFormatStringPos);

  StandardFormatString.append(PrintfFormatString.begin() + PrintfFormatStringPos, PrintfFormatString.begin() + StartSpecifierPos);

  if (FS.getConversionSpecifier().getKind() == analyze_format_string::ConversionSpecifier::PercentArg)
    StandardFormatString.push_back('%');
  else {
    StandardFormatString.push_back('{');

    if (FS.usesPositionalArg())
      StandardFormatString.append(llvm::utostr(FS.getPositionalArgIndex()));

    std::string FormatSpec;

    if (FS.hasAlternativeForm()) {
      FormatSpec.push_back('#');
    }

    const OptionalAmount FieldWidth = FS.getFieldWidth();
    switch (FieldWidth.getHowSpecified()) {
    case OptionalAmount::NotSpecified:
      break;
    case OptionalAmount::Constant:
      FormatSpec.append(llvm::utostr(FieldWidth.getConstantAmount()));
      break;
    case OptionalAmount::Arg:
      FormatSpec.push_back('{');
      if (FieldWidth.usesPositionalArg())
        FormatSpec.append(llvm::utostr(FieldWidth.getPositionalArgIndex()));
      FormatSpec.push_back('}');
      break;
    case OptionalAmount::Invalid:
      break;
    }

    if (!FormatSpec.empty()) {
      StandardFormatString.push_back(':');
      StandardFormatString.append(FormatSpec);
    }

    // Now append the standard version of the printf specifier
    StandardFormatString.push_back('}');
  }

  // Skip over specifier
  PrintfFormatStringPos = StartSpecifierPos + specifierLen;
  assert(PrintfFormatStringPos <= PrintfFormatString.size());
  return true;
}

void PrintfConvertCheck::check(const MatchFinder::MatchResult &Result) {
  llvm::outs() << "printf call\n";
  const auto *PrintfCall = Result.Nodes.getNodeAs<CallExpr>("printf");
  assert(PrintfCall);
  PrintfCall->dumpPretty(*Result.Context);
  llvm::outs() << "\n\n";

  llvm::outs() << "Format string\n";
  const auto *Format = Result.Nodes.getNodeAs<clang::StringLiteral>("format");
  Format->dumpPretty(*Result.Context);
  llvm::outs() << "\n\n";

  using clang::analyze_format_string::ParsePrintfString;
  //  using clang::analyze_format_string::FormatStringHandler;

  const StringRef FormatString = Format->getString();

  llvm::outs() << "Format getstring: " << FormatString << "\n";

  FormatStringConverter Handler(FormatString);
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
