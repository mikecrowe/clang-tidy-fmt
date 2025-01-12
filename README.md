# clang-tidy modernize-nlohmann-json-explicit-conversions check

Teach clang-tidy to convert [nlohmann::json](https://json.nlohmann.me/) implicit conversions to explicit calls to the `get` function.

It is expected that the next major version of nlohmann::json will stop supporting implicit conversions. It is possible to tell current versions to [disallow such conversions](https://json.nlohmann.me/api/macros/json_use_implicit_conversions/) now by defining `JSON_USE_IMPLICIT_CONVERSIONS=0`.

These changes to clang-tidy add a new _modernize-nlohmann-json-explicit-conversions_ check that will convert implicit conversions to explicit ones. In other words, it turns:
```c++
void f(const nlohmann::json &j1, const nlohmann::json &j2)
{
    int i = j1;
    double d = j2.at("value");
    std::cout << i << " " << d << "\n";
}
```
into
```c++
void f(const nlohmann::json &j1, const nlohmann::json &j2)
{
    int i = j1.get<int>();
    double d = j2.at("value").get<double>();
    std::cout << i << " " << d << "\n";
}
```
by knowing what the target type is for the implicit conversion and making it explicit.

# Usage

```sh
clang-tidy -fix -checks=-*,modernize-nlohmann-json-explicit-conversions source.cpp
```

# Missing features

Although the matcher matches and can happily turn the following code:
```c++
bool b(nlohmann::json &j)
{
    auto i = j.find("bool");
    return *i;
}
```
into
```c++
bool b(nlohmann::json &j)
{
    auto i = j.find("bool");
    return i->get<bool>();
}
```

It can't match the following code at all so makes no attempt to fix it:
```c++
std::optional<int> a(nlohmann::json &j)
{
    const auto it = j.find("test");
    if (it != j.end())
        return *it;
    else
        return std::nullopt;
}
```

This is because the implicit conversion occurs inside `std::optional`'s constructor. Such uses can be found after conversion by setting `JSON_USE_IMPLICIT_CONVERSIONS=0` and looking for compilation errors.
