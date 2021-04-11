// RUN: %check_clang_tidy %s fmt-printf-convert %t

#include <stdio.h>
#include <inttypes.h>

// I can't find a way to include headers that contain these declarations. :(
namespace std {
int printf(const char *fmt, ...);
int fprintf(FILE *, const char *fmt, ...);
}

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

void std_printf_simple() {
  std::printf("std::Hello");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("std::Hello");
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

  printf("Error %m\n");
  // fmt doesn't support %m. In theory we could insert a strerror(errno)
  // parameter (assuming that libc has a thread-safe implementation, which glibc
  // does), but that would require keeping track of the input and output
  // parameter indices for position arguments too.
}

void printf_not_string_literal(const char *fmt) {
  // We can't convert the format string if it's not a literal
  printf(fmt, 42);
}

void printf_inttypes_ugliness() {
  // The one advantage of the checker seeing the token pasted version of the
  // format string is that we automatically cope with the horrendously-ugly
  // inttypes.h macros!
  uint64_t u64 = 42;
  uintmax_t umax = 4242;
  printf("uint64:%" PRId64 " uintmax:%" PRIdMAX "\n", u64, umax);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("uint64:{} uintmax:{}\n", u64, umax);
}

void printf_raw_string() {
  // This one doesn't require the format string to be changed, so it stays intact
  printf(R"(First\Second)");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print(R"(First\Second)");

  // This one does require the format string to be changed, so unfortunately it
  // gets reformatted as a normal string.
  printf(R"(First %d\Second)", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("First {}\\Second", 42);
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

  printf("Right-justified string %20s\n", "Hello");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Right-justified string {:>20}\n", "Hello");

  printf("Right-justified string with field width argument %2$*1$s after\n", 20, "wibble");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Right-justified string with field width argument {1:>{0}} after\n", 20, "wibble");
}

void printf_left_justified() {
  printf("Left-justified integer %-4d\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Left-justified integer {:<4}\n", 42);

  printf("Left-justified double %-4f\n", 227.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Left-justified double {:<4f}\n", 227.2);

  printf("Left-justified double %-4g\n", 227.4);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Left-justified double {:<4g}\n", 227.4);

  printf("Left-justified integer with field width argument %2$-*1$d after\n", 5, 424242);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Left-justified integer with field width argument {1:<{0}} after\n", 5, 424242);

  printf("Left-justified string %-20s\n", "Hello");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Left-justified string {:20}\n", "Hello");

  printf("Left-justified string with field width argument %2$-*1$s after\n", 5, "wibble");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Left-justified string with field width argument {1:{0}} after\n", 5, "wibble");
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

  printf("Wibble %#20x\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Wibble {:#20x}\n", 42);

  printf("Wibble %#020x\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Wibble {:#020x}\n", 42);

  printf("Wibble %#-20x\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Wibble {:<#20x}\n", 42);
}

void printf_leading_plus() {
  printf("Positive integer %+d\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Positive integer {:+}\n", 42);

  printf("Positive double %+f\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Positive double {:+f}\n", 42.2);

  printf("Positive double %+g\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Positive double {:+g}\n", 42.2);

  // Ignore leading plus on strings to avoid potential runtime exception where
  // printf would have just ignored it.
  printf("Positive string %+s\n", "string");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Positive string {}\n", "string");
}

void printf_leading_space() {
  printf("Spaced integer % d\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Spaced integer {: }\n", 42);

  printf("Spaced double % f\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Spaced double {: f}\n", 42.2);

  printf("Spaced double % g\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Spaced double {: g}\n", 42.2);
}

void printf_leading_zero() {
  printf("Leading zero integer %03d\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Leading zero integer {:03}\n", 42);

  printf("Leading zero double %03f\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Leading zero double {:03f}\n", 42.2);

  printf("Leading zero double %03g\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Leading zero double {:03g}\n", 42.2);
}

void printf_leading_plus_and_space() {
  // printf prefers plus to space. {fmt} will throw if both are present.
  printf("Spaced integer % +d\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Spaced integer {:+}\n", 42);

  printf("Spaced double %+ f\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Spaced double {:+f}\n", 42.2);

  printf("Spaced double % +g\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Spaced double {:+g}\n", 42.2);
}

void printf_leading_zero_and_plus() {
  printf("Leading zero integer %+03d\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Leading zero integer {:+03}\n", 42);

  printf("Leading zero double %0+3f\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Leading zero double {:+03f}\n", 42.2);

  printf("Leading zero double %0+3g\n", 42.2);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf with fmt::print [fmt-printf-convert]
  // CHECK-FIXES: fmt::print("Leading zero double {:+03g}\n", 42.2);
}
