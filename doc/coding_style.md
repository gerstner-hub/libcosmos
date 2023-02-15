Coding Style Rules for libcosmos
================================

The project currently has a unified coding style. Without going into
meticulous details describing every aspect of it I simply ask of you to adhere
the coding style you find in the rest of the code.

A few pointers about the coding style follow.

Function Names
--------------

- Member functions: Use camel case starting with a lower case letter:
  `myobj.doSomething()`.
- Global functions (in namespaces, not bound to a class): Use snake case:
  `cosmos::do_something()`.
- Prefer single word function names like `raw()` instead of `getRaw()`. Except
  the attribute can also be set, then use `getThis()` and `setThis()`.

Type Names
----------

Use camel case with a starting upper case letter: `class MyType`. The same for
typedefs, `using` types, enums etc.

Other Identifiers
-----------------

- Variables: use snake case: `int my_variable;`
- Parameter names: same as for variables. Use no prefix.
- Constants: all upper case, separated with underscore: `constexpr int MY_CONSTANT = 10;`
- Preprocessor defines / macros: lice constants
- Namespaces: all lower case, one word only like `cosmos::proc`

Braces
------

- Opening curly brace for code blocks should start on the same line like:
    ```
    void myfunc() {
        // some code
    }
    ```
