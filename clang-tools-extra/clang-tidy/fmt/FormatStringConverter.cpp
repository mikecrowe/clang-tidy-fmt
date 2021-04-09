//===--- FormatStringConverter.cpp - clang-tidy------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FormatStringConverter.h"
#include "clang/AST/Expr.h"
#include "clang/AST/FormatString.h"
#include "clang/Basic/LangOptions.h"
#include "llvm/ADT/StringExtras.h"

namespace clang {
namespace tidy {
namespace fmt {

/// Convert a printf-style format string to a libfmt-style one. This class is
/// expecting to work on the already-cooked format string (i.e. all the escapes
/// have been converted) so we have to convert them back. This means that we
/// might not convert them back using the same form.
class FormatStringConverter
    : public clang::analyze_format_string::FormatStringHandler {
  size_t PrintfFormatStringPos = 0U;
  const StringRef PrintfFormatString;
  const Expr * const *PrintfArgs;
  const unsigned PrintfNumArgs;
  std::string StandardFormatString;
  bool ConversionPossible = true;
  bool NeededRewriting = false;
  std::vector<const Expr *> PointerArgs;

public:
  explicit FormatStringConverter(const StringRef PrintfFormatStringIn,
                                 const Expr *const *PrintfArgsIn,
                                 unsigned PrintfNumArgsIn)
      : PrintfFormatString(PrintfFormatStringIn), PrintfArgs(PrintfArgsIn),
        PrintfNumArgs(PrintfNumArgsIn) {
    // Assume that the output will be approximately the same size as the input,
    // but perhaps with a few escapes expanded.
    StandardFormatString.reserve(PrintfFormatString.size() + 8);
  }

  bool HandlePrintfSpecifier(const analyze_printf::PrintfSpecifier &FS,
                             const char *StartSpecifier,
                             unsigned SpecifierLen) override;
  bool isConversionPossible() const { return ConversionPossible; }
  bool neededRewriting() const { return NeededRewriting; }
  std::string getStandardFormatString();
  std::vector<const Expr *> extractPointerArgs()
  {
    return std::move(PointerArgs);
  }
};

bool FormatStringConverter::HandlePrintfSpecifier(
    const analyze_printf::PrintfSpecifier &FS, const char *StartSpecifier,
    unsigned SpecifierLen) {
  using namespace analyze_printf;

  const size_t StartSpecifierPos = StartSpecifier - PrintfFormatString.data();
  assert(StartSpecifierPos + SpecifierLen <= PrintfFormatString.size());

  // Everything before the specifier needs copying verbatim
  assert(StartSpecifierPos >= PrintfFormatStringPos);

  StandardFormatString.append(PrintfFormatString.begin() +
                                  PrintfFormatStringPos,
                              PrintfFormatString.begin() + StartSpecifierPos);

  if (FS.getConversionSpecifier().getKind() ==
      analyze_format_string::ConversionSpecifier::PercentArg)
    StandardFormatString.push_back('%');
  else {
    StandardFormatString.push_back('{');

    if (FS.usesPositionalArg()) {
      // fmt argument identifiers are zero based, whereas printf ones are
      // one based.
      assert(FS.getPositionalArgIndex() > 0U);
      StandardFormatString.append(llvm::utostr(FS.getPositionalArgIndex() - 1));
    }

    std::string FormatSpec;

    if (FS.hasAlternativeForm()) {
      FormatSpec.push_back('#');
    }

    {
      const OptionalAmount FieldWidth = FS.getFieldWidth();
      switch (FieldWidth.getHowSpecified()) {
      case OptionalAmount::NotSpecified:
        break;
      case OptionalAmount::Constant:
        FormatSpec.append(llvm::utostr(FieldWidth.getConstantAmount()));
        break;
      case OptionalAmount::Arg:
        FormatSpec.push_back('{');
        if (FieldWidth.usesPositionalArg()) {
          // fmt argument identifiers are zero based, whereas printf ones are
          // one based.
          assert(FieldWidth.getPositionalArgIndex() > 0U);
          FormatSpec.append(llvm::utostr(FieldWidth.getPositionalArgIndex() - 1));
        }
        FormatSpec.push_back('}');
        break;
      case OptionalAmount::Invalid:
        break;
      }
    }

    {
      const OptionalAmount FieldPrecision = FS.getPrecision();
      switch (FieldPrecision.getHowSpecified()) {
      case OptionalAmount::NotSpecified:
        break;
      case OptionalAmount::Constant:
        FormatSpec.push_back('.');
        FormatSpec.append(llvm::utostr(FieldPrecision.getConstantAmount()));
        break;
      case OptionalAmount::Arg:
        FormatSpec.push_back('.');
        FormatSpec.push_back('{');
        if (FieldPrecision.usesPositionalArg()) {
          // fmt argument identifiers are zero based, whereas printf ones are
          // one based.
          assert(FieldPrecision.getPositionalArgIndex() > 0U);
          FormatSpec.append(llvm::utostr(FieldPrecision.getPositionalArgIndex() - 1));
        }
        FormatSpec.push_back('}');
        break;
      case OptionalAmount::Invalid:
        break;
      }
    }

    {
      if (FS.getArgIndex() > PrintfNumArgs) {
        // Argument index out of range. Give up.
        ConversionPossible = false;
        return false;
      }

      const Expr *Arg = PrintfArgs[FS.getArgIndex()]->IgnoreImplicitAsWritten();
      using analyze_format_string::ConversionSpecifier;
      const ConversionSpecifier spec = FS.getConversionSpecifier();
      switch (spec.getKind()) {
      case ConversionSpecifier::Kind::nArg:
        // fmt doesn't do the equivalent of %n
        ConversionPossible = false;
        return false;
      case ConversionSpecifier::Kind::sArg:
        // Strings never need to be specified
        break;
      case ConversionSpecifier::Kind::cArg:
        // Only specify char if the argument is of a different type
        if (!Arg->getType()->isCharType())
          FormatSpec.push_back('c');
        llvm::outs() << "c arg " << FS.getArgIndex() << "\n";
        break;
      case ConversionSpecifier::Kind::dArg:
      case ConversionSpecifier::Kind::iArg:
        // Only specify integer if the argument is of a different type
        if (Arg->getType()->isCharType() || !Arg->getType()->isIntegerType())
          FormatSpec.push_back('d');
        break;
      case ConversionSpecifier::Kind::pArg:
        PointerArgs.push_back(Arg);
        break;
      case ConversionSpecifier::Kind::xArg:
        FormatSpec.push_back('x');
        break;
      case ConversionSpecifier::Kind::XArg:
        FormatSpec.push_back('X');
        break;
      case ConversionSpecifier::Kind::oArg:
        FormatSpec.push_back('o');
        break;
      case ConversionSpecifier::Kind::aArg:
        FormatSpec.push_back('a');
        break;
      case ConversionSpecifier::Kind::AArg:
        FormatSpec.push_back('A');
        break;
      case ConversionSpecifier::Kind::eArg:
        FormatSpec.push_back('e');
        break;
      case ConversionSpecifier::Kind::EArg:
        FormatSpec.push_back('E');
        break;
      case ConversionSpecifier::Kind::fArg:
        FormatSpec.push_back('f');
        break;
      case ConversionSpecifier::Kind::FArg:
        FormatSpec.push_back('F');
        break;
      case ConversionSpecifier::Kind::gArg:
        FormatSpec.push_back('g');
        break;
      case ConversionSpecifier::Kind::GArg:
        FormatSpec.push_back('G');
        break;
      default:
        // Something we don't understand
        ConversionPossible = false;
        return false;
      }
    }

    if (!FormatSpec.empty()) {
      StandardFormatString.push_back(':');
      StandardFormatString.append(FormatSpec);
    }

    // Now append the standard version of the printf specifier
    StandardFormatString.push_back('}');
  }

  // Skip over specifier
  PrintfFormatStringPos = StartSpecifierPos + SpecifierLen;
  assert(PrintfFormatStringPos <= PrintfFormatString.size());

  NeededRewriting = true;
  return true;
}

std::string FormatStringConverter::getStandardFormatString() {
  StandardFormatString.append(PrintfFormatString.begin() +
                                  PrintfFormatStringPos,
                              PrintfFormatString.end());
  PrintfFormatStringPos = PrintfFormatString.size();

  std::string result;
  result.push_back('\"');
  for (const char ch : StandardFormatString) {
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
    } else
      result += ch;
  }
  result.push_back('\"');
  return result;
}

FormatStringResult
printfFormatStringToFmtString(const ASTContext *Context,
                              const llvm::StringRef PrintfFormatString,
                              const Expr * const *PrintfArgs,
                              unsigned PrintfNumArgs) {
  FormatStringConverter Handler{PrintfFormatString, PrintfArgs, PrintfNumArgs};
  LangOptions LO;
  const bool IsFreeBsdkPrintf = false;

  using clang::analyze_format_string::ParsePrintfString;
  ParsePrintfString(Handler, PrintfFormatString.data(),
                    PrintfFormatString.data() + PrintfFormatString.size(), LO,
                    Context->getTargetInfo(), IsFreeBsdkPrintf);

  if (!Handler.isConversionPossible())
    return FormatStringResult::Kind::unsuitable;
  if (Handler.neededRewriting())
    return { Handler.getStandardFormatString(), Handler.extractPointerArgs() };
  return FormatStringResult::Kind::unchanged;
}

} // namespace fmt
} // namespace tidy
} // namespace clang
