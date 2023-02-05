// RUN: %check_clang_tidy %s modernize-trace-format %t -- \
// RUN:   -- -isystem %clang_tidy_headers -fexceptions
#include <string>

class BaseTrace {
public:
  template <typename... Args>
  void operator()(const char *fmt, Args &&...args) {
  }

  template <typename... Args>
  void Trace(const char *fmt, Args &&...args) {
  }

  template <typename... Args>
  void FMT_STYLE(const char *fmt, Args &&...args) {
  }
};

class NullTrace {
public:
  template <typename... Args>
  void operator()(const char *fmt, Args &&...args) {
  }
};

class DerivedTrace : public BaseTrace {
};

class NeverTrace : public NullTrace {
};

BaseTrace TRACE;
NeverTrace TRACEN;
DerivedTrace TRACE2;

int f(const std::string &s, const std::string *ps) {
  TRACE("No arguments\n");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf-style TRACE with std::format equivalent [modernize-trace-format]
  // CHECK-FIXES: TRACE.FMT_STYLE("No arguments\n");

  TRACE("Goodbye %d trailing\n" "Another line %s\n", 42, "wobble");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf-style TRACE with std::format equivalent [modernize-trace-format]
  // CHECK-FIXES: TRACE.FMT_STYLE("Goodbye {} trailing\nAnother line {}\n", 42, "wobble");

  TRACE2("Goodbye %s Wibble %d %d %c %c\n", "Hello", 42, 'A', 66, 'B');
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf-style TRACE with std::format equivalent [modernize-trace-format]
  // CHECK-FIXES: TRACE2.FMT_STYLE("Goodbye {} Wibble {} {:d} {:c} {}\n", "Hello", 42, 'A', 66, 'B');

  TRACE("\'value\' = \"%s\"\\/\r\n", "pickle");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf-style TRACE with std::format equivalent [modernize-trace-format]
  // CHECK-FIXES: TRACE.FMT_STYLE("'value' = \"{}\"\\/\r\n", "pickle");

  TRACE2("Remove c_str() from %s and %s\n", s.c_str(), ps->c_str());
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf-style TRACE with std::format equivalent [modernize-trace-format]
  // CHECK-FIXES: TRACE2.FMT_STYLE("Remove c_str() from {} and {}\n", s, *ps);

  TRACEN("N Remove c_str() from %s and %s\n", s.c_str(), ps->c_str());
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf-style TRACE with std::format equivalent [modernize-trace-format]
  // CHECK-FIXES: TRACEN.FMT_STYLE("N Remove c_str() from {} and {}\n", s, *ps);

  return 0;
}

void trace_unsupported_format_specifiers() {

  TRACE("Error %m\n");
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: unable to use std::format syntax in TRACE because '%m' is not supported in format string [modernize-trace-format]
}

#define TRACE_IN_MACRO(X, err) \
  TRACE("GL error 0x%x in " #X " line %d\n", err, __LINE__);

void trace_in_macro()
{
  TRACE_IN_MACRO(f, 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf-style TRACE with std::format equivalent [modernize-trace-format]
  // but no fix is available
}

struct TraceInStruct
{
  DerivedTrace Trace;
};

void trace_in_struct()
{
  TraceInStruct s;
  s.Trace("Hello %d\n", 42);
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: Replace printf-style TRACE with std::format equivalent [modernize-trace-format]
  // CHECK-FIXES: s.Trace.FMT_STYLE("Hello {}\n", 42);
}
