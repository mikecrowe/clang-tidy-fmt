// RUN: %check_clang_tidy %s fmt-printf-convert %t

#include <stdio.h>

void printf_simple() {
  printf("Hello");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello");
}

void fprintf_simple() {
  fprintf(stderr, "Hello");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print(stderr, "Hello");
}

void printf_escape() {
  printf("Bell\aBackspace\bFF\fNewline\nCR\rTab\tVT\vEscape\x1b\x07 %d", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Bell\aBackspace\bFF\fNewline\nCR\rTab\tVT\vEscape\x1b\a {}", 42);
}

void printf_percent() {
  printf("Hello %% and another %%");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello % and another %");
}

void printf_integer() {
  printf("Hello %d after\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {} after\n", 42);
}

void printf_string() {
  printf("Hello %s after\n", "Goodbye");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {} after\n", "Goodbye");
}

void printf_double() {
  printf("Hello %f after\n", 42.0);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {} after\n", 42.0);
}

void printf_positional_arg() {
  printf("Hello %2$d %1$s\n", "Goodbye", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {2} {1}\n", "Goodbye", 42);
}

void printf_field_width() {
  printf("Hello %3d after\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:3} after\n", 42);

  printf("Hello %*d after\n", 4, 4242);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:{}} after\n", 4, 4242);

  printf("Hello %2$*1$d after\n", 5, 424242);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {2:{1}} after\n", 5, 424242);
}

void printf_precision() {
  printf("Hello %.3f\n", 3.14159);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:.3}\n", 3.14159);

  printf("Hello %.*f after\n", 4, 3.14159265358979323846);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:.{}} after\n", 4, 3.14159265358979323846);

  printf("Hello %1$.*2$f after\n", 3.14159265358979323846, 4);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {1:.{2}} after\n", 3.14159265358979323846, 4);
}

void printf_field_width_and_precision() {
  printf("Hello %1$*2$.*3$f after\n", 3.14159265358979323846, 4, 2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {1:{2}.{3}} after\n", 3.14159265358979323846, 4, 2);
}

void printf_alternative_form() {
  printf("Wibble %#x\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Wibble {:#}\n", 42);
}
