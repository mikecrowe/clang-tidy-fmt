//===--- FormatStringConverter.cpp - clang-tidy------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FormatStringConverter.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/StringExtras.h"

namespace clang {
namespace tidy {
namespace fmt {

FormatStringConverter::FormatStringConverter(const ASTContext *ContextIn,
                                             const CallExpr *Call,
                                             unsigned FormatArgOffset,
                                             const LangOptions &LO)
    : Context(ContextIn), Args(Call->getArgs()), NumArgs(Call->getNumArgs()),
      ArgsOffset(FormatArgOffset + 1), LangOpts(LO) {
  assert(ArgsOffset <= NumArgs);
  FormatExpr = llvm::dyn_cast<StringLiteral>(
      Args[FormatArgOffset]->IgnoreImplicitAsWritten());
  assert(FormatExpr);
  PrintfFormatString = FormatExpr->getString();

  // Assume that the output will be approximately the same size as the input,
  // but perhaps with a few escapes expanded.
  StandardFormatString.reserve(PrintfFormatString.size() + 8);

  const bool IsFreeBsdkPrintf = false;

  using clang::analyze_format_string::ParsePrintfString;
  ParsePrintfString(*this, PrintfFormatString.data(),
                    PrintfFormatString.data() + PrintfFormatString.size(),
                    LangOpts, Context->getTargetInfo(), IsFreeBsdkPrintf);
}

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

  using analyze_format_string::ConversionSpecifier;
  const ConversionSpecifier Spec = FS.getConversionSpecifier();

  if (Spec.getKind() == ConversionSpecifier::PercentArg)
    StandardFormatString.push_back('%');
  else if (Spec.getKind() == ConversionSpecifier::Kind::nArg) {
    // fmt doesn't do the equivalent of %n
    ConversionPossible = false;
    return false;
  } else if (Spec.getKind() == ConversionSpecifier::Kind::PrintErrno) {
    // fmt doesn't support %m. In theory we could insert a strerror(errno)
    // parameter (assuming that libc has a thread-safe implementation, which
    // glibc does), but that would require keeping track of the input and output
    // parameter indices for position arguments too.
    ConversionPossible = false;
    return false;
  } else {
    StandardFormatString.push_back('{');

    if (FS.usesPositionalArg()) {
      // fmt argument identifiers are zero based, whereas printf ones are
      // one based.
      assert(FS.getPositionalArgIndex() > 0U);
      StandardFormatString.append(llvm::utostr(FS.getPositionalArgIndex() - 1));
    }

    // [[fill]align][sign]["#"]["0"][width]["." precision][type]
    std::string FormatSpec;

    // We only care about alignment if a field width is specified
    if (FS.getFieldWidth().getHowSpecified() != OptionalAmount::NotSpecified) {
      const ConversionSpecifier Spec = FS.getConversionSpecifier();
      if (Spec.getKind() == ConversionSpecifier::sArg) {
        // Strings are left-aligned by default with {fmt}, so we only need to
        // emit an alignment if this one needs to be right aligned.
        if (!FS.isLeftJustified())
          FormatSpec.push_back('>');
      } else {
        // Numbers are right-aligned by default with {fmt}, so we only need to
        // emit an alignment if this one needs to be left aligned.
        if (FS.isLeftJustified())
          FormatSpec.push_back('<');
      }
    }

    {
      const ConversionSpecifier Spec = FS.getConversionSpecifier();
      // Ignore on something that isn't numeric. For printf it's would be a
      // compile-time warning but ignored at runtime, but for {fmt} pre-C++20 it
      // would be a runtime exception.
      if (Spec.isAnyIntArg() || Spec.isDoubleArg()) {
        // + is preferred to ' '
        if (FS.hasPlusPrefix())
          FormatSpec.push_back('+');
        else if (FS.hasSpacePrefix())
          FormatSpec.push_back(' ');
      }
    }

    if (FS.hasAlternativeForm()) {
      FormatSpec.push_back('#');
    }

    if (FS.hasLeadingZeros())
      FormatSpec.push_back('0');

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
          FormatSpec.append(
              llvm::utostr(FieldWidth.getPositionalArgIndex() - 1));
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
          FormatSpec.append(
              llvm::utostr(FieldPrecision.getPositionalArgIndex() - 1));
        }
        FormatSpec.push_back('}');
        break;
      case OptionalAmount::Invalid:
        break;
      }
    }

    {
      if (FS.getArgIndex() + ArgsOffset >= NumArgs) {
        // Argument index out of range. Give up.
        ConversionPossible = false;
        return false;
      }

      // If we've got this far, then the specifier must have an associated
      // argument
      assert(FS.consumesDataArgument());

      const Expr *Arg =
          Args[FS.getArgIndex() + ArgsOffset]->IgnoreImplicitAsWritten();
      using analyze_format_string::ConversionSpecifier;
      const ConversionSpecifier Spec = FS.getConversionSpecifier();
      switch (Spec.getKind()) {
      case ConversionSpecifier::Kind::sArg:
        // Strings never need to be specified
        break;
      case ConversionSpecifier::Kind::cArg:
        // Only specify char if the argument is of a different type
        if (!Arg->getType()->isCharType())
          FormatSpec.push_back('c');
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

  FormatStringNeededRewriting = true;
  return true;
}

std::string FormatStringConverter::getStandardFormatString() {
  StandardFormatString.append(PrintfFormatString.begin() +
                                  PrintfFormatStringPos,
                              PrintfFormatString.end());
  PrintfFormatStringPos = PrintfFormatString.size();

  std::string Result;
  Result.push_back('\"');
  for (const char Ch : StandardFormatString) {
    if (Ch == '\a')
      Result += "\\a";
    else if (Ch == '\b')
      Result += "\\b";
    else if (Ch == '\f')
      Result += "\\f";
    else if (Ch == '\n')
      Result += "\\n";
    else if (Ch == '\r')
      Result += "\\r";
    else if (Ch == '\t')
      Result += "\\t";
    else if (Ch == '\v')
      Result += "\\v";
    else if (Ch == '\"')
      Result += "\\\"";
    else if (Ch == '\\')
      Result += "\\\\";
    else if (Ch < 32) {
      Result += "\\x";
      Result += llvm::hexdigit(Ch >> 4, true);
      Result += llvm::hexdigit(Ch & 0xf, true);
    } else
      Result += Ch;
  }
  Result.push_back('\"');
  return Result;
}

void FormatStringConverter::applyFixes(DiagnosticBuilder &Diag,
                                       SourceManager &SM) {
  if (FormatStringNeededRewriting) {
    Diag << FixItHint::CreateReplacement(
        CharSourceRange::getTokenRange(FormatExpr->getBeginLoc(),
                                       FormatExpr->getEndLoc()),
        getStandardFormatString());
  }
  for (const Expr *Arg : PointerArgs) {
    SourceLocation AfterOtherSide =
        Lexer::findNextToken(Arg->getEndLoc(), SM, LangOpts)->getLocation();
    Diag << FixItHint::CreateInsertion(Arg->getBeginLoc(), "fmt::ptr(")
         << FixItHint::CreateInsertion(AfterOtherSide, ")");
  }
}
} // namespace fmt
} // namespace tidy
} // namespace clang
