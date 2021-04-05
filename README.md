# clang-tidy printf->fmt::print converter

This "fork" of [llvm-project][1] adds a proof-of-concept clang-tidy checker
that converts occurrences of printf and fprintf to fmt::print and modifies
the format string appropriately. In other words, it turns:

```C++
fprintf(stderr, "The %s is %3d\n", answer, value);
```
into:
```C++
fmt::print(stderr, "The {} is {:3}\n", answer, value);
```

It doesn't do a bad job, but it's not perfect. In particular:

* At the point that the check runs, the AST contains a single
  `StringLiteral` for the format string and any macro expansion, token
  pasting, adjacent string literal concatenation and escaping has been
  handled. Although it's possible to put the escapes back, they may not be
  exactly as they were written (e.g. "\x0a" will become "\n".) It turns out
  that it's probably quite important that macro expansion and adjacent
  string literal concatenation happen before we parse the format string in
  order to cope with the <inttypes.h> PRI macros.

* It tries to support field widths, positional arguments and alternative
  forms. It doesn't support padding or precision, among other things, yet.
  They can be added once I'm confident that this is the right approach.

* It makes no attempt to translate printf-style format specifiers to their
  {fmt} equivalents. For example, `%d`, `%f` and `%g` all currently end up
  being `{}`.

* It is assumed that the `fmt/format.h` header has already been included.
  No attempt is made to include it.

* It has too few tests in
  clang-tools-extra/test/clang-tidy/checkers/fmt-printf-convert.cpp.

* I'd really like to unit test `FormatStringConverter` rather than doing
  everything in the fmt-printf-convert.cpp tests. This mainly because
  clang-tidy takes so long to link on my machine that the test cycle is far
  too long! In order to do so I need a way to create a `TargetInfo`
  instance to pass to `ParsePrintfString`. Any advice on how to do so
  gratefully received.

## Usage:

Build clang-tidy following the [upstream instructions][1]. Install it if
you wish, or just run from the build directory with something like:

    bin/clang-tidy -checks='-*,fmt-printf-convert' --fix input.cpp

## How it works

There are no clang-tidy checks for fmt yet, so I've added
`clangTidyFmtModule`. The `FormatStringConverter` class is used to
implement `printfFormatStringToFmtString` which does the hard work of
translating the format strings. After that, the check in
`PrintfConvertCheck` simply needs to replace `printf` or `fprintf` with
`fmt::print`, call `printfFormatStringToFmtString` and replace the format
string if necessary.

## Will it work for my printf-like function too?

In addition to the `fmt-printf-convert` check, there are two other checks
that are unlikely to be useful as they are, but they may be modifiable to
do what you want: `fmt-strprintf-convert` and `fmt-trace-convert`.

### fmt-strprintf-convert

The `fmt-strprintf-convert` check converts calls to a commonly-implemented
`sprintf` wrapper function that is expected to return `std::string` to the
equivalent `fmt::format` call. For example, it turns:

```C++
const std::string s = strprintf("%d", 42);
```
into:
```C++
const std::string s = fmt::format("{}", 42);
```

### fmt-trace-convert

The `fmt-trace-convert` check converts calls to operator() on an object
that derives from a particular class (in this case, the class is
`BaseTrace`. For example, it converts:
```C++
class DerivedTrace : public BaseTrace {};
BaseTrace TRACE;
DerivedTrace TRACE2;

TRACE("%s=%d\n", name, value);
TRACE2("%s\n", name);
```
into:
```C++
class DerivedTrace : public BaseTrace {};
BaseTrace TRACE;
DerivedTrace TRACE2;

TRACE("{}={}\n", name, value);
TRACE2("{}\n", name);
```

(It is assumed that the implementation of `BaseTrace` will be modified at
the same time to expect the new form of format string.)

[1]: https://github.com/llvm/llvm-project
