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
I'm not entirely sure why, but the [AST](https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGe1wAyeAyYAHI%2BAEaYxBIAzKQADqgKhE4MHt6%2BekkpjgJBIeEsUTFc8XaYDmlCBEzEBBk%2Bfly2mPZ5DDV1BAVhkdFxtrX1jVktCsM9wX3FA2UAlLaoXsTI7BzmscHI3lgA1Caxbgy0qAgshgwA9NoKAgB0CAkJh9gmGgCCm9u7mAdHqASHTEr3eX0%2BwQIeyoEBOZwuDD8IFuAj2ACptPMwSYAOxWT57Ql7YiYAgrBjoiDaaxcLGxfFfHEAEWxEMEe2AsNO50uIGRdwp5gAbJjsXiwUTiaTyXtqfSxSzPmCIqhPHsIlz4bz%2BajhaKleKCUSmF4iHs8P8mbL7jQGOgIOYzCrPI66QzJSSycQKWi8IcGbjFeCPhN0HzAcDaIc3JDXnsmJqeYi%2BSjBWYRViDe6iWgGBN46bUOaoYcrdobcF7Y6CJgJq7/RKiXgqHsIIQ9mAwKXra4IPNMx9JR7pd70YQG0bCW0lI2h1KvRTQ3yGF5aKcgRPGUyOItaJwAKy8PwcLSkVCcNzWax7O4rNYHMyxHikAiaHeLADWIFiQvuAE4uD/DQhQ0DR9zMMxANiMx9E4SQjzfM9OF4BQQA0F830WOBYBgRAUFQFgEjoaJyEoNBCOImIdkMYAAH0CGILwGA/Pg6BrYhUIgCJEIiYI6gAT04Z9eOYYh%2BIAeQibRKlfbheHIthBHEk5BJPXgsAiLxgDcMRaFQuTSCwBFgHENTDLwEkqgAN1rRDMFUSpTXWZ9ITaRDaDwCJiAEjwsEQhi8BYITeBs4gVSUJlMGMjyjEwvgDGABQADU8EwAB3cSEkYYKZEEEQxHYKRcvkJQ1EQ3QWgMWLTEsax9E81DYGYNgQDhJMGFo1NSFCzgny4Hh%2BzPIE0n03hUFC4g8CwRq%2B1ado0hcO1RmaUhAmmIoSmyZJUgEZattyNJeg2uY5pk6pJj28Y2jOgQunqI7%2BlKIZuku577vWx6JEWW9VkK3cDwQszzw4PZVAADiFABaIVJD2aijFbBimI/eZW1wQgSAfJ95l4WStEGr993QvcOHg0ggsfe5Yn3HEQI0P8cQ0Rn90kP9pGPU9gZQtCMLUrDcIgJAmAmUiIHIoj6GIUJWHWcGoZhuGquARHGOYnHVvwIhJrDFp%2BDy0RxC4GC9ZKlR1DMirTqqZwIFcV61sKT6WhyHb0k8Jp9tdh7Zieiprc6C73bGK2OjuqZHZ9vQJheoOVuj96I822kXxJTAprQ/6OEPUgObGzgQyhFL0uiUGIeh2H4eViAkbVtHNcxzYWj2DwKMlrHaVxzDFgQTAmCwGJZpJsmgsZ%2B4hUAjQjaFfcaa4MH9xzxCudsHm8Z3UhsLw5YCASU1RfFyjpZajg5fLxWaJV5H1cwevtb0E38sN6QTcUM3yr0NLvISYLM%2Bz3OkI4OJU0u8oSoBbKfBWldL61wgC3CWJdG7qzXoNIevAgq/g0I%2BWIENYg4mpjifcf4wZ/kXkDZCK90LIM/CAIUsR7gaDBvTWIf42Y4hxIwoUYNYIcFiIDTm5CqHcLMHwvOHBO580WKFFIzhJBAA) looks rather different because the conversion happens during construction of the `std::optional` and there's no `operator int` in it. I suspect that the matcher I'm using to match everything else is actually wrong but just happens to work for the cases I'm testing and were found in the code I converted. I didn't spend very long trying to solve this because I had only one such occurrence in my code and could fix it by hand.
