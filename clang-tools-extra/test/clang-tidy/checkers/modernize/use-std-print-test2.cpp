// RUN: %check_clang_tidy \
// RUN:   -std=c++23 %s modernize-use-std-print %t -- \
// RUN:   -config="{CheckOptions: [{key: StrictMode, value: true}]}" \
// RUN:   -- -isystem %clang_tidy_headers -fexceptions
#include <cstddef>
#include <cstdint>
#include <cstdio>
// CHECK-FIXES: #include <print>
#include <inttypes.h>
#include <string.h>
#include <string>

// FOR TESTING ONLY - ROLL INTO COMPLETE SUITE

void printf_field_width_and_precision()
{
  const std::string s;
  const unsigned int ui = 42;

  printf("%*s=%*d\n", 4, s.c_str(), 3, ui);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: use 'std::println' instead of 'printf' [modernize-use-std-print]
  // CHECK-FIXES: std::println("{1:>{0}}={3:{2}}", 4, s, 3, static_cast<int>(ui));
}
