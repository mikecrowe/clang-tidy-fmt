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

TEST(FormatStringConverter, PlainString) {
  const char plain[] = "This string contains nothing that needs translating";
  ASSERT_EQ(stdFormatStringFromPrintfFormatString(plain), plain);
}

} // namespace test
} // namespace tidy
} // namespace clang
