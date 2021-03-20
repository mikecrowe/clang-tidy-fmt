#include "../clang-tidy/utils/FormatStringConverter.h"

#include "ClangTidy.h"
#include "ClangTidyTest.h"
#include "gtest/gtest.h"

namespace clang {
namespace tidy {
namespace test {

using clang::tidy::utils::stdFormatStringFromPrintfFormatString;

TEST(FormatStringConverter, EmptyString) {
  ASSERT_EQ(stdFormatStringFromPrintfFormatString(""), "");
}

TEST(FormatStringConverter, NoDirectivesOrEscapes) {
  const char Input[] = "This string contains nothing that needs translating";
  ASSERT_EQ(stdFormatStringFromPrintfFormatString(Input), Input);
}

TEST(FormatStringConverter, Escapes) {
  const char Input[] = "This\nstring\bcontains\v\only\x45scapes";
  ASSERT_EQ(stdFormatStringFromPrintfFormatString(Input), Input);
}

TEST(FormatStringConverter, PercentDirective) {
  const char Input[] = "%%";
  ASSERT_EQ(stdFormatStringFromPrintfFormatString(Input), "%");
}

#if 0
TEST(FormatStringConverter, SimpleStringSubstitution) {
  const char Input[] = "%s";
  ASSERT_EQ(stdFormatStringFromPrintfFormatString(Input), "{}");
}
#endif

} // namespace test
} // namespace tidy
} // namespace clang
