// RUN: %check_clang_tidy %s fmt-trace-convert %t

class BaseTrace {
public:
  template <typename... Args>
  void operator()(const char *fmt, Args &&...args) {
  }

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

int main() {
  TRACE("Goodbye %d trailing\n" "Another line %s\n", 42, "wobble");
  // CHECK-MESSAGES: [[@LINE-1]]:9: warning: Replace TRACE format string [fmt-trace-convert]
  // CHECK-FIXES: TRACE("Goodbye {} trailing\nAnother line {}\n", 42, "wobble");

  TRACE2("Goodbye %s Wibble %d %d %c %c\n", "Hello", 42, 'A', 66, 'B');
  // CHECK-MESSAGES: [[@LINE-1]]:10: warning: Replace TRACE format string [fmt-trace-convert]
  // CHECK-FIXES: TRACE2("Goodbye {} Wibble {} {:d} {:c} {}\n", "Hello", 42, 'A', 66, 'B');

  TRACE("\'value\' = \"%s\"\\/\r\n", "pickle");
  // CHECK-MESSAGES: [[@LINE-1]]:9: warning: Replace TRACE format string [fmt-trace-convert]
  // CHECK-FIXES: TRACE("'value' = \"{}\"\\/\r\n", "pickle");

  return 0;
}
