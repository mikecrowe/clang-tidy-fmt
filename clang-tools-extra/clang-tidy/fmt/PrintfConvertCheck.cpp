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

/// Convert a printf-style format string to a libfmt-style one. This class is
/// suboptimal because it works on the already-cooked format string (i.e. all
/// the escapes have been converted) so we have to convert them back. This means
/// that we might not convert them back using the same form.
class FormatStringConverter : public clang::analyze_format_string::FormatStringHandler {
  size_t PrintfFormatStringPos = 0U;
  const StringRef PrintfFormatString;
  std::string StandardFormatString;
  bool NeededRewriting = false;

  public:
  explicit FormatStringConverter(const StringRef PrintfFormatStringIn)
      : PrintfFormatString(PrintfFormatStringIn)
  {
    // Assume that the output will be approximately the same size as the input,
    // but perhaps with a few escapes expanded.
    StandardFormatString.reserve(PrintfFormatString.size() + 8);
  }

  bool HandlePrintfSpecifier(const analyze_printf::PrintfSpecifier &FS,
                                     const char *startSpecifier,
                             unsigned specifierLen) override;

  bool neededRewriting() const {
    return NeededRewriting;
  }

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
        else if (ch < 32) {
          result += "\\x";
          result += llvm::hexdigit(ch >> 4, true);
          result += llvm::hexdigit(ch & 0xf, true);
        }
        else
            result += ch;
    }
    result.push_back('\"');
    return result;
  }
};

bool FormatStringConverter::HandlePrintfSpecifier(const analyze_printf::PrintfSpecifier &FS,
                                                         const char *startSpecifier,
                                                         unsigned specifierLen) {
  using namespace analyze_printf;

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

  NeededRewriting = true;
  return true;
}

void PrintfConvertCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *PrintfCall = Result.Nodes.getNodeAs<CallExpr>("printf")->getCallee();
  const auto *Format = Result.Nodes.getNodeAs<clang::StringLiteral>("format");
  const StringRef FormatString = Format->getString();

  FormatStringConverter Handler(FormatString);
  LangOptions LO;
  const bool IsFreeBsdkPrintf = false;

  using clang::analyze_format_string::ParsePrintfString;
  ParsePrintfString(Handler,
                    FormatString.data(), FormatString.data() + FormatString.size(),
                    LO, Result.Context->getTargetInfo(), IsFreeBsdkPrintf);

  DiagnosticBuilder Diag = diag(PrintfCall->getBeginLoc(), "Replace printf with fmt::print");
  Diag << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(PrintfCall->getBeginLoc(),
                                                                      PrintfCall->getEndLoc()),
                                       "fmt::print");

  if (Handler.neededRewriting()) {
    const auto StandardFormatString = std::move(Handler).getStandardFormatString();
    Diag << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(Format->getBeginLoc(),
                                                                        Format->getEndLoc()),
                                         StandardFormatString);
  }
}

}  // namespace fmt
}  // namespace tidy
}  // namespace clang
