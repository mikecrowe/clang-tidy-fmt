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

void printf_unsupported() {
  int pos;
  printf("%d %n %d\n", 42, &pos, 72);
  // fmt doesn't do the equivalent of %n, so leave the call alone.
}

void printf_integer() {
  printf("Integer %d from integer\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Integer {} from integer\n", 42);

  printf("Integer %i from integer\n", 65);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Integer {} from integer\n", 65);

  printf("Integer %i from char\n", 'A');
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Integer {:d} from char\n", 'A');

  printf("Integer %d from char\n", 'A');
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Integer {:d} from char\n", 'A');
}

// This checks that we get the argument offset right with the extra FILE * argument
void fprintf_integer() {
  fprintf(stderr, "Integer %d from integer\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print(stderr, "Integer {} from integer\n", 42);

  fprintf(stderr, "Integer %i from integer\n", 65);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print(stderr, "Integer {} from integer\n", 65);

  fprintf(stderr, "Integer %i from char\n", 'A');
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print(stderr, "Integer {:d} from char\n", 'A');

  fprintf(stderr, "Integer %d from char\n", 'A');
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print(stderr, "Integer {:d} from char\n", 'A');
}

void printf_char() {
  printf("Char %c from char\n", 'A');
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Char {} from char\n", 'A');

  printf("Char %c from integer\n", 65);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Char {:c} from integer\n", 65);
}

void printf_bases() {
  printf("Hex %lx %#lx\n", 42L, 42L);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hex {:x} {:#x}\n", 42L, 42L);

  printf("HEX %X %#X\n", 42, 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("HEX {:X} {:#X}\n", 42, 42);

  printf("Oct %lo %#lo\n", 42L, 42L);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Oct {:o} {:#o}\n", 42L, 42L);
}

void printf_string() {
  printf("Hello %s after\n", "Goodbye");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {} after\n", "Goodbye");
}

void printf_double() {
  printf("Hello %f after\n", 42.0);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:f} after\n", 42.0);

  printf("Hello %g after\n", 42.0);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:g} after\n", 42.0);

  printf("Hello %e after\n", 42.0);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:e} after\n", 42.0);
}

void printf_pointer() {
  int i;
  double j;
  printf("Int* %p %s %p\n", &i, "Double*", &j);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Int* {} {} {}\n", fmt::ptr(&i), "Double*", fmt::ptr(&j));
}

void printf_positional_arg() {
  printf("Hello %2$d %1$s\n", "Goodbye", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {1} {0}\n", "Goodbye", 42);
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
  // CHECK-FIXES: fmt::print("Hello {1:{0}} after\n", 5, 424242);
}

void printf_precision() {
  printf("Hello %.3f\n", 3.14159);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:.3f}\n", 3.14159);

  printf("Hello %.*f after\n", 4, 3.14159265358979323846);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {:.{}f} after\n", 4, 3.14159265358979323846);

  printf("Hello %1$.*2$f after\n", 3.14159265358979323846, 4);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {0:.{1}f} after\n", 3.14159265358979323846, 4);
}

void printf_field_width_and_precision() {
  printf("Hello %1$*2$.*3$f after\n", 3.14159265358979323846, 4, 2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Hello {0:{1}.{2}f} after\n", 3.14159265358979323846, 4, 2);
}

void printf_alternative_form() {
  printf("Wibble %#x\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Wibble {:#x}\n", 42);
}
