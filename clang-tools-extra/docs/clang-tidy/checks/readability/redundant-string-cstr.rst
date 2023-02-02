.. title:: clang-tidy - readability-redundant-string-cstr

readability-redundant-string-cstr
=================================


Finds unnecessary calls to ``std::string::c_str()`` and ``std::string::data()``.

Only the first such call in an argument to ``std::print`` or
``std::format`` will be detected, so it may be necessary to run the check
more than once when applying fixes.
