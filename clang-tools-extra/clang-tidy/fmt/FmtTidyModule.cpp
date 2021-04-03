//===------- FmtTidyModule.cpp - clang-tidy ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "TraceConvertCheck.h"
#include "PrintfConvertCheck.h"

namespace clang {
namespace tidy {
namespace fmt {

class FmtModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<TraceConverterCheck>(
        "fmt-trace-convert");
    CheckFactories.registerCheck<PrintfConvertCheck>(
        "fmt-printf-convert");
  }
};

// Register the FmtModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<FmtModule> X("fmt-module",
                                                    "Add {fmt} library checks.");

} // namespace fmt

// This anchor is used to force the linker to link in the generated object file
// and thus register the FmtModule.
volatile int FmtModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
