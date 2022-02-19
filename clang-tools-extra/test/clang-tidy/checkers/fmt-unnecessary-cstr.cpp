// RUN: %check_clang_tidy %s fmt-unnecessary-cstr %t

// check_clang_tidy can't find C++ headers, so we have to make do with something
// else #include <string>
namespace std {

    template<typename CharType>
    struct basic_string {
    const char *c_str() const;
    };

    using string = basic_string<char>;
//    using wstring = basic_string<wchar_t>;
    // struct string {
    // const char *c_str() const;
//};
}

namespace fmt {
    template<typename ...Args>
    void print(const char *, Args...);
    template<typename ...Args>
    std::string format(const char *, Args...);
}

namespace notfmt {
    template<typename ...Args>
    void print(const char *, Args...);
    template<typename ...Args>
    std::string format(const char *, Args...);
}

struct some_iterator {
    std::string *operator->() const;
};

void fmt_print(const std::string &s1) {
  fmt::print("One:{}\n", s1.c_str());
  // CHECK-MESSAGES: [[@LINE-1]]:26: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES: fmt::print("One:{}\n", s1);
}

void fmt_print_complex(const std::string &s1) {
  // we don't want to automatically convert anything clever
  fmt::print("One: {}\n", s1.c_str() + 2);
}

void fmt_print_pointers(const std::string *s1, std::string *s2) {
  fmt::print("One:{} Two:{}\n", *s1, s2->c_str());
  // CHECK-MESSAGES-not-yet: [[@LINE-1]]:37: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES-not-yet: fmt::print("One:{} Two:{}\n", *s1, *s2);
}

void fmt_print_iterator(some_iterator it) {
  fmt::print("One:{}\n", it->c_str());
  // CHECK-MESSAGES-not-yet: [[@LINE-1]]:25: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES-not-yet: fmt::print("One:{} Two:{}\n", *it);
}

const std::string &get_s1();
std::string get_s2();

void fmt_print_functions() {
  fmt::print("One:{} Two:{}\n", get_s1(), get_s2().c_str());
  // CHECK-MESSAGES: [[@LINE-1]]:43: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES: fmt::print("One:{} Two:{}\n", get_s1(), get_s2());
}

const std::string *get_s1_pointer();
std::string *get_s2_pointer();

void fmt_print_pointers_functions() {
  fmt::print("One:{} Two:{}\n", get_s1_pointer()->c_str(), *get_s2_pointer());
  // CHECK-MESSAGES-not-yet: [[@LINE-1]]:32: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES-not-yet: fmt::print("One:{} Two:{}\n", *get_s1_pointer(), *get_s2_pointer());
}

void fmt_print_unqualified(const std::string &s1) {
    using namespace fmt;
    print("One:{}\n", s1.c_str());
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES: print("One:{}\n", s1);
}

void fmt_print_no_cstr(const std::string &s1, const std::string &s2) {
    fmt::print("One: {}, Two: {}\n", s1, s2);
}

void not_fmt_print(const std::string &s1) {
    notfmt::print("One: {}\n", s1.c_str());
}

std::string fmt_format(const std::string &s1)
{
  return fmt::format("One:{}\n", s1.c_str());
  // CHECK-MESSAGES: [[@LINE-1]]:34: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES: return fmt::format("One:{}\n", s1);
}

std::string not_fmt_format(const std::string &s1) {
    return notfmt::format("One: {}\n", s1.c_str());
}

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

void trace_base(const std::string &s1, const std::string &s2)
{
  TRACE.F("One:{} Two:{}\n", s1.c_str(), s2.c_str());
  // CHECK-MESSAGES: [[@LINE-1]]:30: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES: TRACE.F("One:{} Two:{}\n", s1, s2.c_str());
}

DerivedTrace TRACE2;

void trace_derived(const std::string &s1, const std::string &s2)
{
  TRACE2.F("One:{} Two:{}\n", s1.c_str(), s2.c_str());
  // CHECK-MESSAGES: [[@LINE-1]]:31: warning: Remove unnecessary call to std::string::c_str() [fmt-unnecessary-cstr]
  // CHECK-FIXES: TRACE2.F("One:{} Two:{}\n", s1, s2.c_str());
}
