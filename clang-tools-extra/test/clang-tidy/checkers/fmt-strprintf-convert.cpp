// RUN: %check_clang_tidy %s fmt-strprintf-convert %t

// check_clang_tidy can't find C++ headers, so we have to make do with something
// else #include <string>
struct string {};

extern string strprintf(const char *format, ...) __attribute__((format(printf, 1, 2)));

string strprintf_simple() {
  return strprintf("Hello");
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: Replace strprintf with fmt::format [fmt-strprintf-convert]
  // CHECK-FIXES: return fmt::format("Hello");
}

string strprintf_complex(const char *name, double value) {
  return strprintf("'%s'='%f'", name, value);
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: Replace strprintf with fmt::format [fmt-strprintf-convert]
  // CHECK-FIXES: return fmt::format("'{}'='{}'", name, value);
}
