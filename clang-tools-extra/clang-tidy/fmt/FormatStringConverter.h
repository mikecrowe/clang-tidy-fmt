#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FMT_FORMATSTRINGCONVERTER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FMT_FORMATSTRINGCONVERTER_H

#include <string>
#include "llvm/ADT/Optional.h"
#include "clang/AST/ASTContext.h"

namespace clang {
namespace tidy {
namespace fmt {

/// If PrintfFormatString would change if converted from printf format to {fmt}
/// format then return a string containing the equivalent {fmt} format.
/// Otherwise return None.
llvm::Optional<std::string>
printfFormatStringToFmtString(const ASTContext *Context,
                              const StringRef PrintfFormatString);

} // namespace fmt
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_FMT_FORMATSTRINGCONVERTER_H
