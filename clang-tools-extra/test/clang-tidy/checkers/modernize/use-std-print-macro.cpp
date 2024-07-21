// RUN: %check_clang_tidy -check-suffixes=,STRICT \
// RUN:   -std=c++23 %s modernize-use-std-print %t -- \
// RUN:   -config="{CheckOptions: {StrictMode: true}}" \
// RUN:   -- -isystem %clang_tidy_headers -fexceptions \
// RUN:      -DPRI_CMDLINE_MACRO="\"s\"" \
// RUN:      -D__PRI_CMDLINE_MACRO="\"s\""
#include <stdio.h>
#include <inttypes.h>

#define SURROUND_ALL(x) x
#define SURROUND_FORMAT_PARTIAL(x) x
#define FMT "s"

int main()
{
  SURROUND_ALL(printf(SURROUND_FORMAT_PARTIAL("%" FMT) "\n", "42"));
}
