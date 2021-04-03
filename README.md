# clang-tidy printf->fmt::print converter

This "fork" of [llvm-project][1] adds a proof-of-concept clang-tidy checker
that converts occurrences of printf and fprintf to fmt::print. It doesn't
do a bad job, but it's not perfect. In particular:

* At the point that the check runs, the AST contains a single StringLiteral
  for the format string and any macro expansion, token pasting, adjacent
  string literal concatenation and escaping has been handled. Although it's
  possible to put the escapes back, they may not be exactly as they were
  written (e.g. "\x0a" will become "\n".) It turns out that it's probably
  quite important that macro expansion and adjacent string literal
  concatenation happen before we parse the format string in order to cope
  with the <inttypes.h> PRI macros.

* It tries to support field widths, positional arguments and alternative
  forms. It doesn't support padding or precision, among other things, yet.
  They can be added once I'm confident that this is the right approach.

* It makes no attempt to translate printf-style format specifiers to their
  {fmt} equivalents. For example, `%d`, `%f` and `%g` all currently end up
  being `{}`.

* It is assumed that the `fmt.h` header has already been included. No
  attempt is made to include it.

* It has too few tests in
  clang-tools-extra/test/clang-tidy/checkers/fmt-printf-convert.cpp.

* I'd really like to unit test `FormatStringConverter` rather than doing
  everything in the fmt-printf-convert.cpp tests. This mainly because
  clang-tidy takes so long to link on my machine that the test cycle is far
  too long! In order to do so I need a way to create a `TargetInfo`
  instance to pass to `ParsePrintfString`. Any advice on how to do so
  gratefully received.

## How it works

There are no clang-tidy checks for fmt yet, so I've added
`clangTidyFmtModule`. The `FormatStringConverter` class is used to
implement `printfFormatStringToFmtString` which does the hard work of
translating the format strings. After that, the check in
`PrintfConvertCheck` simply needs to replace `printf` or `fprintf` with
`fmt::print`, call `printfFormatStringToFmtString` and replace the format
string if necessary.

[1] https://github.com/llvm/llvm-project
