# clang-tidy fmt checks

This "fork" of [llvm-project][1] adds a proof-of-concept clang-tidy checker
that converts occurrences of `printf` and `fprintf` to `fmt::print` (as
provided by the [{fmt}][2] library) and modifies the format string
appropriately. In other words, it turns:

```C++
fprintf(stderr, "The %s is %3d\n", answer, value);
```
into:
```C++
fmt::print(stderr, "The {} is {:3}\n", answer, value);
```

It doesn't do a bad job, but it's not perfect. In particular:

* It assumes that the input is mostly sane. If you get any warnings when
  compiling with `-Wformat` then misbehaviour is possible.

* At the point that the check runs, the AST contains a single
  `StringLiteral` for the format string and any macro expansion, token
  pasting, adjacent string literal concatenation and escaping has been
  handled. Although it's possible for the check to automatically put the
  escapes back, they may not be exactly as they were written (e.g. "\x0a"
  will become "\n" and "ab" "cd" will become "abcd".) It turns out that
  it's probably quite important that macro expansion and adjacent string
  literal concatenation happen before we parse the format string in order
  to cope with the <inttypes.h> PRI macros.

* It tries to support field widths, precision, positional arguments,
  leading zeros, leading +, alignment and alternative forms.

* It is assumed that the `fmt/format.h` header has already been included.
  No attempt is made to include it.

* Use of any unsupported flags or specifiers will cause the entire
  statement to be left alone. Known unsupported features are:

  * The `%'` flag for thousands separators. It looks like this could be
    translated to `{:L}`, but I'm not sure it will do exactly the same
    thing.

  * The glibc extension `%m`. This could be supported relatively easily if
    we can assume that `strerror` is thread safe (which glibc version is.)

* It has some tests in
  clang-tools-extra/test/clang-tidy/checkers/fmt-printf-convert.cpp but
  they probably don't cover the full set of possibilities.

* It copes with calls to printf, ::printf and std::printf. Unfortunately
  this means that it also changes mine::printf which is probably incorrect.
  My attempts to fix this using isInStdNamespace() have failed.

* This is my first attempt at a clang-tidy checker, so it's probably full
  of things that aren't done the idiomatic LLVM way.

* It's not separated into easily-understandable commits with good commit
  messages yet.

## Usage:

Build clang-tidy following the [upstream instructions][1]. Install it if
you wish, or just run from the build directory with something like:

    bin/clang-tidy -checks='-*,fmt-printf-convert' --fix input.cpp

## How it works

There are no clang-tidy checks for fmt yet, so I've added
`clangTidyFmtModule`. The `FormatStringConverter` class makes use of
Clang's own `ParsePrintfString` to walk the format string deciding what to
do. If the format string can be converted then `PrintfConvertCheck` simply
needs to replace `printf` or `fprintf` with `fmt::print`, and tell
`FormatStringConverter` to apply the necessary fixes. The applied fixes are:

* `printf`/`fprintf` becomes `fmt::print`
* rewrite the format string to use the [{fmt} format language][3]
* wrap any arguments that corresponded to `%p` specifiers that {fmt} won't
  deal with in a call to `fmt::ptr`.

## Will it work for my printf-like function too?

Maybe. In addition to the `fmt-printf-convert` check, there are two other
checks that are unlikely to be useful as they are, but they may be
modifiable to do what you want: `fmt-strprintf-convert` and
`fmt-trace-convert`.

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

## Running the LIT tests

Once you've built everything, run something like:

    bin/llvm-lit -v ../clang-tools-extra/test/clang-tidy/checkers/fmt-printf-convert.cpp


## The Future

The [{fmt}][2] library is gradually being standardised. `fmt::format` is in
C++20 as `std::format`. It would not be hard to adapt these checks to
convert to the standard versions instead.

[1]: https://github.com/llvm/llvm-project
[2]: https://fmt.dev/
[3]: https://fmt.dev/latest/syntax.html
