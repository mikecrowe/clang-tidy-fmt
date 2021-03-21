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

void printf_double() {
  printf("Hello %f after\n", 42.0);
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: replace format string [fmt-printf-convert]
  // CHECK-FIXES: printf("Hello {} after\n", 42.0);
}

void printf_positional_arg() {
  printf("Hello %2$d %1$s\n", "Goodbye", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: replace format string [fmt-printf-convert]
  // CHECK-FIXES: printf("Hello {2} {1}\n", "Goodbye", 42);
}

void printf_field_width() {
  printf("Hello %3d after\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: replace format string [fmt-printf-convert]
  // CHECK-FIXES: printf("Hello {:3} after\n", 42);

  printf("Hello %*d after\n", 4, 4242);
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: replace format string [fmt-printf-convert]
  // CHECK-FIXES: printf("Hello {:{}} after\n", 4, 4242);

  printf("Hello %2$*1$d after\n", 5, 424242);
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: replace format string [fmt-printf-convert]
  // CHECK-FIXES: printf("Hello {2:{1}} after\n", 5, 424242);
}
