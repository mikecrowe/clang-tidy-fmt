// RUN: %check_clang_tidy %s fmt-printf-convert %t

#include <stdio.h>

void printf_integer() {
  printf("Hello %d after\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: replace format string [fmt-printf-convert]
  // CHECK-FIXES: printf("Hello {} after\n", 42);
}

void printf_string() {
  printf("Hello %s after\n", "Goodbye");
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: replace format string [fmt-printf-convert]
  // CHECK-FIXES: printf("Hello {} after\n", "Goodbye");
}
