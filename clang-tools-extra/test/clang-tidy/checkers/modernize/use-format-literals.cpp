// RUN: %check_clang_tidy -std=c++20 %s modernize-use-format-literals %t -- -- -isystem %clang_tidy_headers

#include <string>

// TODO: share with redundant-string-cstr-function.cpp via header
namespace std {
  template<typename T>
    struct type_identity { using type = T; };
  template<typename T>
    using type_identity_t = typename type_identity<T>::type;

  template <typename CharT, typename... Args>
  struct basic_format_string {
    consteval basic_format_string(const CharT *format) : str(format) {}
    basic_string_view<CharT, std::char_traits<CharT>> str;
  };

  template<typename... Args>
    using format_string = basic_format_string<char, type_identity_t<Args>...>;

  template<typename... Args>
    using wformat_string = basic_format_string<wchar_t, type_identity_t<Args>...>;

  template<typename ...Args>
  std::string format(format_string<Args...>, Args &&...);
  template<typename ...Args>
  std::string format(wformat_string<Args...>, Args &&...);

  template<typename ...Args>
  void print(format_string<Args...>, Args &&...);
  template<typename ...Args>
  void print(wformat_string<Args...>, Args &&...);

  template<typename ...Args>
  void println(format_string<Args...>, Args &&...);
  template<typename ...Args>
  void println(wformat_string<Args...>, Args &&...);
}

std::string getString();

std::string one_simple_parameter(const char *s) {
  return std::format("Hello, {}!", s);
  // CHECK-MESSAGES: [[@LINE-1]]:22: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: return std::format("Hello, {s}!");
}

std::string two_simple_parameters(const char *s, const char *t) {
  return std::format("Hello, {} {}!", s, t);
  // CHECK-MESSAGES: [[@LINE-1]]:22: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: return std::format("Hello, {s} {t}!");
}

void one_function_call_parameter() {
  std::println("One function {}", getString());
  // CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: std::println("One function {getString()}");
}

void two_function_call_parameters() {
  std::println("Two functions {} and {}", getString(), getString());
  // CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: std::println("Two functions {getString()} and {getString()}");
}

void one_lambda_call() {
  std::println("One lambda call {}", []() { return getString(); }());
  // CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: std::println("One lambda call {[]() { return getString(); }()}");
}

// clang-tidy sees the std::println call first and that the second call inside
// the argument would overlap with it, so we just get one level of replacement.
// If they happened in the other order then current clang-tidy would work if run
// more than once. It's possible that once Clang supports format literals that
// running twice would work then too. It's also possible that the check could
// somehow recurse into arguments and fix them first. It's not clear whether
// this is worth the effort though since such usage wasn't efficient anyway.
void format_inside_argument() {
  std::println("Replacement inside argument {}", std::format("'{}'", getString()));
  // CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // CHECK-MESSAGES: [[@LINE-2]]:62: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: std::println("Replacement inside argument {std::format("'{}'", getString())}");
}

// TODO: We don't remove unnecessary parens yet, but we probably should do
void remove_unnecessary_parens() {
  std::println("Remove unnecessary parens {}", (1, 2));
  // CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: std::println("Remove unnecessary parens {(1, 2)}");
}

void format_specifiers(int i, double d, const char *s) {
  std::println("Width {:03}", i);
  // CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: std::println("Width {i:03}");

  std::println("Width and precision {:03.4}", d);
  // CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: std::println("Width and precision {d:03.4}");

  std::println("Fill and centre {: ^20}", s ? s : "(null)");
  // CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // CHECK-FIXES: std::println("Fill and centre {s ? s : "(null)": ^20}");
}

#if 0
// The current f-literals implementation doesn't seem to work with clang-tidy. Resulting in:

// .../llvm-project/build/tools/clang/tools/extra/test/clang-tidy/checkers/modernize/Output/use-format-literals.cpp.tmp.cpp:79:53: error: expected ')' [clang-diagnostic-error]
//   79 |   std::println("Inside format literal {std::format("'{}'", getString())}");
//      |                                                     ^
// .../llvm-project/build/tools/clang/tools/extra/test/clang-tidy/checkers/modernize/Output/use-format-literals.cpp.tmp.cpp:79:15: note: to match this '('
//   79 |   std::println("Inside format literal {std::format("'{}'", getString())}");
//      |               ^
void format_inside_format_literal() {
  std::println("Inside format literal {std::format("'{}'", getString())}");
  // NOT-CHECK-MESSAGES: [[@LINE-1]]:16: warning: use format literals [modernize-use-format-literals]
  // NOT-CHECK-FIXES: std::println("Inside format literal {std::format("'{}'", getString())}");
}
#endif
