# Style Guide

The style guide for this project is simply the google sytle c/c++ style guide. Only the differences between our style guide and google's are listed here.

google style guide:  [https://google.github.io/styleguide/cppguide.html][google-style-guide]

---

## Macros

The rules with macros remain the same in terms of complex code generation. For assisting in compile-time typ definitions or build configuration type
differences they are completely fine. If the section of code in question is a user-facing customizeable interface (think of a test library that needs to adapt to any software it interacts with)
then macros are fully acceptable.
