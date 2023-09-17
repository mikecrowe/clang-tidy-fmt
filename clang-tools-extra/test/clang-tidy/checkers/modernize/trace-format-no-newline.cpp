// RUN: %check_clang_tidy %s modernize-trace-format-no-newline %t -- \
// RUN:   -- -isystem %clang_tidy_headers -fexceptions
#include <string>

class BaseTrace {
public:
  template <typename... Args>
  void Trace(const char *fmt, Args &&...args) {
  }

  template <typename... Args>
  void F(const char *fmt, Args &&...args) {
  }
};

class DerivedTrace : public BaseTrace {
};

BaseTrace TRACE;
DerivedTrace TRACE2;

int f() {
  TRACE.F("No newline to remove");
  TRACE2.F("No newline to remove 2");

  TRACE.F("Newline to remove\n");
  // CHECK-MESSAGES: [[@LINE-1]]:11: warning: unwanted newline in TRACE.F format string [modernize-trace-format-no-newline]
  // CHECK-FIXES: TRACE.F("Newline to remove");

  TRACE2.F("Newline to remove 2\n");
  // CHECK-MESSAGES: [[@LINE-1]]:12: warning: unwanted newline in TRACE.F format string [modernize-trace-format-no-newline]
  // CHECK-FIXES: TRACE2.F("Newline to remove 2");

  TRACE.F("Double-escaped newline to not remove\\n");
  TRACE2.F("Double-escaped newline to not remove 2\\n");

  return 0;
}
