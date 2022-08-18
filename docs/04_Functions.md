# Functions
Function definition statements always start with the `fun` keyword, followed by it's name, argument list and then body.

Here are some examples of function definitions:
```py
# No arguments
fun sayHelloToEveryone() {
    print('Hello, everyone!\n');
}

# Some arguments
fun sayHelloTo(name) {
    print('Hello, ', name, '!\n');
}

fun sayHelloToBoth(name1, name2) {
    print('Hello, ', name1, '!\n');
    print('Hello, ', name2, '!\n');
}
```
When function bodies only have one statement, you can drop the brackets
```py
fun sayHelloToEveryone()
    print('Hello, everyone!\n');

fun sayHelloTo(name)
    print('Hello, ', name, '!\n');

fun sayHelloToBoth(name1, name2) {
    print('Hello, ', name1, '!\n');
    print('Hello, ', name2, '!\n');
}
```
Like if-else and while statements, they don't need an ending `;`.

The evaluation of a function definition statement results in the definition of a variable who's name is the name of the defined function and it's value is the function itself. You may overwrite the variable or use it in an expression, although the only valid operation on it is a function call.

```py
fun abc() {}

# Now "abc" is a function

abc = 10;

# Now it's an integer!
```

## Functions calls and return values
To execute a function in an expression, it's name must be followed by an argument list enclosed in brackets `(..)`. All functions return at least one value, which may be `none` when a function has no useful value to be returned.

To specify the return value of a function, one must use the `return` statement
```py
fun myName() {
    # ..do stuff..
    return "Francesco";
}

fun add(a, b)
    return a + b;
```

when calling a function, the resulting value of the call will be the one evaluated by the last return statement.

```py
fun add(a, b)
    return a + b;

six = add(2, 3) + 1; 
```

Note that a return statement must always end with a `;`.

When a function doesn't return a value explicitly, then the `none` value is returned:
```py
fun iDontReturnExplicitly() {
    1 + 2;
}

print(iDontReturnExplicitly(), '\n'); # Prints: none 
```

## Multiple return values
Functions may return more than one value:
```py
fun swap(a, b)
    return b, a;
```

When calling a function with more than one return value in an expression, only the first return value is considered.

```py
fun returnTwoThings()
    return 5, "I'm ignored";

print(1 + returnTwoThings(), '\n'); # Prints 6.
```

The way to use the additional return values is through an assignment to multiple variables

```py
first, second = returnTwoThings();
print(first, '\n'); # Prints 5
print(second, '\n'); # Prints "I'm ignored"
```

An assignment to multiple variables is still an expression, and it evaluates to the leftmost value
```py
fun combine(a, b) 
    return (a+b), (a-b);

print((x,y = combine(3, 4))); # Prints 7.
```

When assigning to more values than returned, the excess variables are set to none. If more values are returned than what were expected, the extra ones are ignored.

## Function arguments and default values
When a function is called with less arguments than it expected, the unprovided arguments will have the `none` value. If more arguments than what were expected are provided, the unexpected ones are ignored.

It's possible to define default values for any argument. The default value will be used when the respective argument is `none`.

```py
fun sayHello(name = "unnamed person")
    print("Hello, ", name, "!\n");

sayHello("Francesc"); # Hello, Francesco!
sayHello(none); # Hello, unnamed person!
sayHello();     # Hello, unnamed person!
```

## Scoping
Functions are the only thing that creates a new scope. When variables are defined inside a function, they're relative to the function and the function only. When the function returns, the variable defined inside it are no longer available.

## Closures
When defining a function, the scope of all the parent contexts are captured and are always accessible by the function's body. When functions are defined in the global scope, this means that they can always access global variables

```py
name = "Francesco";

fun printName()
    print(name);

printName(); # Prints "Francesco"

name = "Giovanni";

printName(); # Prints "Giovanni"
```

A function that's defined inside another one will be able to access both the scope of the parent function and the global scope
```py
name1 = "Giovanni";
fun outerFunc() {
    name2 = "Francesco";
    fun innerFunc() {
        print(name1, " and ", name2, " are friends!\n");
    }
}

outerFunc(); # Prints: Giovanni and Francesco are friends!
```

But this mechanism is cooler than that! This also applies when the function is returned
```py
name1 = "Giovanni";
fun outerFunc() {
    name2 = "Francesco";
    fun innerFunc() {
        print(name1, " and ", name2, " are friends!\n");
    }
    return innerFunc;
}

innerFunc = outerFunc();
innerFunc(); # Prints the same thing
```

But functions may never modify the captures scopes. Variables can only be defined relative to the local scope.

## Type hints
Type hints are a lightweight way to check that a function is called with the proper arguments. All of the checks are done at runtime. Each argument may be associated to one or more types. If the function is called with a type other than the specified ones, a runtime error is triggered.

```py
fun add(a: int, b: int)
    return a + b;

c = add(2, 3.0); # Error!! Second argument is a float but an int was expected.
```

```py
fun add(a: int, b: int | float)
    return a + b;

c = add(2, 3.0); # Now this is ok!
```

To allow a function to be `none` when using type hints, you can use the `None` type
```py
fun someFunction(optionalNumber: int | float | None) {
    # ..do stuff..
}
```

Default arguments are evaluated before the type hints, therefore when `none` is provided as argument value, no error is triggered if a proper default argument was specified, even when it wasn't allowed as a type. If the default value doesn't result in a valid type, an error is triggered.

```py
fun someFunction(a: int = 4) {}
someFunction(none); # No error. The argument value will be 4.

fun someFunction2(a: int) {}
someFunction2(none); # Error!

fun someFunction(a: int = 1.3) {}
someFunction(none); # Error!
```
