# FUNCTIONS
## Definition and basics

Functions are defined like this:
```
fun sayHello() {
    print("Hello!\n");
}
```
they must always start with the `fun` keyword followed by the function's name. Function name rules are the same as variables: they can contain letters, numbers or underscores, but the first character can't be a number. One or more arguments can be provided by specifying their names between the `()` following the name:
```
fun sayHelloTo(name1, name2) {
    print("Hello ", name1, " and ", name2, "!\n");
}
```
When a function is called with more arguments than the ones specified in it's definition, the extra ones are discarded. If less arguments than expected are provided, the remaining ones are `none` by default. It's possible to specify default values for any of the arguments. When the caller passes `none` as an argument, the default value is used:
```
fun sayHelloTo(name1="Francesco", b="Giovanni") {
    print("Hello ", name1, " and ", name2, "!\n");
}

sayHello(none, "Filippo"); # Hello Francesco and Filippo!

# Here the arguments are implicitly none
sayHello(); # Hello Francesco and Giovanni!
``` 
Return value can be specified using the `return` keyword:
```
fun sum(a, b) {
    return a + b;
}

print("The sum of 3 and 7 is ", sum(3, 7), "\n");
```
functions that don't return a value explicitly will return `none` implicitly. Multiple values can be returned. To get the extra return values, one must assign them to variables. If called outside of an assignment, only the first return value is considered.
```
fun divmod(x, y) {
    return (x / y), (x % y);
}

res1, res2 = divmod(100, 20); # 5, 0

print(divmod(100, 20)); # Prints 5 (the modulo's result is discarded) 
``` 
When a function returns more values than what was expected in an assignment expression, the extra values are ignored. If an assignment expects a function to return more values than it actually returns, the extra variables are set to `none`.

## Scoping and closures
 
Functions are the only construct that creates a new scope: when variables are defined inside a functions, they're only accessible from within that function. When the function returns, all variables defined by it are no longer accessible. Here are some examples:
```
fun doSomething() {
    name = "Francesco";
}

doSomething();
print(name); # Runtime Error: No variable "name" is defined!
```
By contrast, functions can access variables defined in their parent scope (relative to their definition)
```
name = "Francesco";
age = 24;

fun printVars() {
    print(name, age);
}

printVars(); # prints: Francesco24
```
An alternative way of saying this is that functions create closures. This mechanism also works recursively, functions defined within other functions can access both the parent function's variables and global variables.
```
X = 1;

fun wrapper() {

    Y = 2;

    fun printVars() {
        print(X, Y);
    }

    printVars();
}

wrapper(); # prints: 12
```
A cool implication of closures is the ability to create parametric definitions of functions. This is done by returning from a function a function defined inside it
```
fun createDivisibilityCheck(n) {
    fun isDivisibleByN(k) {
        return k % n == 0;
    }

    return isDivisibleByN;
}

isDivisibleBy2 = createDivisibilityCheck(2);
isDivisibleBy3 = createDivisibilityCheck(3);

isDivisibleBy2(100); # true
isDivisibleBy2(107); # false

isDivisibleBy3(39);  # true
isDivisibleBy3(100); # false
``` 
in this example, `isDivisibleBy2` and `isDivisibleBy3` share their implementation. What changes, is the closure of their parent scopes.

## Type assertions
Type assertions are a way to check that a function is called with the proper arguments. All of the checks are done at runtime. Each argument may be associated to one or more types. If the function is called with a type other than the specified ones, a runtime error is triggered.

```
fun add(a: int, b: int)
    return a + b;

c = add(2, 3.0); # Error!! Second argument is a float but an int was expected.
```

```
fun add(a: int, b: int | float)
    return a + b;

c = add(2, 3.0); # Now this is ok!
```

To allow a function to be `none` when using type assertions, you can use the `None` type or the `?` operator
```
fun someFunction(optionalNumber: int | float | None) {
    # ..do stuff..
}
fun someFunction2(optionalNumber: ?(int | float)) {
    # ..do stuff..
}
```

Default arguments are evaluated before the type assertions, therefore when `none` is provided as argument value, no error is triggered if a proper default argument was specified, even when it wasn't allowed as a type. If the default value doesn't result in a valid type, an error is triggered.

```
fun someFunction(a: int = 4) {}
someFunction(none); # No error. The argument value will be 4.

fun someFunction2(a: int) {}
someFunction2(none); # Error!

fun someFunction3(a: int = 1.3) {}
someFunction3(none); # Error!
```
