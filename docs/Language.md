# Noja's language documentation

## 1 - Introduction
Noja is a high level programming language implemented as a learning exercise. Still, it aims to be a non-trivial example of how a language may be built. The use-cases of Noja are the same as Python since their abstraction level is comparable. The syntax is more similar to the C-family of languages though (curly brackets to denote scope).

A Noja program is a sequence of statements. If not in string literals, whitespace doesn't matter. Comments starts with `#` and end with the line.

## 2 - Expressions

### 2.1 - Basics
Expression use infix notation. You can have expressions of numeric values, boolean values and other datatypes like strings of text. When expressions are used as statements, they need to be terminated using a semicolon. Here's an example:
```
2 * (1 + 2); 
```
The basic values that can be used are integers, floats, booleans and `none`.

### 2.2 - Integers, floats and arithmetic operators
Both integers and floats (floating point values) are signed and represented using 8 bytes. Integers can represent integer values between [2^61-1, 2^61], like `int64_t`s in C/C++. On the other hand, floats are equivalent to C/C++ `double`s. Numeric values can be operated onto using the arithmetic operators:
 
* addition `+` (binary and unary)
* subtraction `-` (binary and unary)
* multiplication `*`
* division `/`
* modulo `%`
 
Here "modulo" refers to the remainder of the division. These operations mainly behave like one would expect and have the following type conversion rules:
1. Operations involving integers evaluate to integers, except division. The result of a division is always a float.
1. If an arithmetic operation involves a float, the result is also float. 
If operations on integers overflow or underflow, the program's execution is aborted.

### 2.3 - Booleans and logical operators

Boolean values are values that can either be `true` or `false`. They have the property that the logical negation of one equals the other.
 
Booleans can be operated onto using logical operators such as `and`, `or` and `not`. These operators expect boolean values and return a new boolean value. You probably know these well, but for completeness sake, here's how they work:
```
true  and true;  # = true
true  and false; # = false
false and true;  # = false
false and false; # = false

true  or true;   # = true
true  or false;  # = true
false or true;   # = true
false or false;  # = false

not true;  # = false
not false; # = true
``` 
If any of the operands aren't booleans, the program's execution is aborted.

Operators `and` and `or` are short-circuit operators. This means that they only consider the right operand if they can't deduce the solution from the left one. For example, if the left operand of an `and` evaluates to `false`, it isn't necessary to evaluate the right one, since the result of the overall operation can only be `false`.


### 2.4 - Relational operators

Relational operators are those which evaluate to booleans and take, in general, non-boolean operands. They are:

* equal `==`
* not equal `!=`
* less than `<`
* less than or equal `<=`
* greater than `>`
* greater than or equal `>=`
 
Equal (not equal) can be applied to all type of operands and return true (false) when the operands have the same type and hold the same value. These operator don't allow funny business like `1 == "1"` evaluating to `true`.
 
The remaining relational operators are applied to numeric datatypes (ints and floats). 

### 2.5 - The none value

The none value used to represent the void of a value and has the only property of being equal to itself and itself only. You can use the none value using the `none` keyword:

```
x = none; 
```

### 2.6 - Variables and assignments

You can store computed values into variables in order to reuse them later on. Variables are created using the assignment operator:
```
x = 1 + 4;
y = x + 2;
```
here we're assigning to the variable `x` the number 5 then, we're assigning to `y` the value 7 by accessing the value previously stored into `x`. The left operand of the assignment operator must be a variable name while the right operator can have any type.

Variable names can consist of digits, letters or underscores, but the first character can't be a digit though.
 
Since the assignment operator is an operator, other than do an assignment it also returns a value. Any assignment evaluates to it's right operand. By instance
```
y = (x = 1 + 4) + 2;
```
this expression will result in `x` having value 5 and `y` value 7. This is because `x = 1 + 4`, other than assigning to `x`, is equivalent to writing `5`.


### 2.7 - Composit types and square bracket notation

Composit values are collections of other values, which may also be composite. The composite types are `List`, `Map` and `String`. For all collection types it's possible to insert and retrieve values using the `[]` notation:
```
coll[key] = item; # Store the value associated to the 
                  # variable "item" with key "key" in 
                  # the collection "coll".

item = coll[key]; # Get the item back by selecting it 
                  # using it's key
```
In this example, the `coll` variable is a collection type, while the types of `key` and `item` depend on the type of collection.


### 2.8 - Lists

Lists are heterogeneous and ordered collections of values. Each item they contain is associated to it's position in the list (the first element has position 0). They're defined and used with the following syntax:
```
my_list = [true, 1.2, 19];

x = my_list[0]; # true
y = my_list[2]; # 19

my_list[0] = 13;

z = my_list[0]; # z is 13 now!
```
Trying to access an item using as key something which isn't an integer or an in range integer (less then 0 or higher than the length of the array minus one) will result in an error. The only exception to this rule is the index equal to the item count of the list (which is out of bounds since the last item of the list has index equals to the item count minus one): by inserting a value at this index, the list will increase it's size by 1. There isn't a limit on how many values a list can contain.


### 2.9 - Strings

Strings are values which contain UTF-8 encoded text. A string can be instanciated placing text between single or double quotes:
```
"I'm a string!";

'I am too!';
```
Special character (such as horizontal tabs and carriage returns) can be specified using the `\x` notation:

* `\t` - tab
* `\r` - carriage return
* `\n` - newline

When strings contain quotes that match the ones surrounding them or the `\` character, it's necessary to escape them:
``` 
'Hi, I\'m Francesco!';
"Hi \"Francesco\", how old are you?";
"This is a backlash \\ and you can do nothing about it";
``` 
Like arrays, single characters can be selected referring to them by their position relative to the first character using the `[]` notation. When selecting single characters from a string, they're returned as new strings.

Once a string is created, it's not possible to modify it. If you want to change a string's value, you need to create a new updated version of the string.

 
### 2.10 - Maps and the dot operator

Maps are collections of key-value pairs, where both keys and values can have any type. The syntax for defining and using maps is this:
```
me = {"name": "Francesco", "age": 24};

my_name = me["name"]; # Francesco

me["name"] = true;

my_name = me["name"]; # true
```
When selecting from a map a value associated to a key which was never inserted, `none` is returned:
```
my_map = {1: "one", 3: "three"};
two = my_map[2]; # none
```
Because of the existence of compount statements, expression statements can't start with the `{` token. The parser would assume it's a badly formatted compount statement. To avoid the ambiguity, you can add some tokens that have no effect before the `{`:
```
{"day": "Monday"}; # invalid
+{"day": "Monday"}; # valid
({"day": "Monday"}); # also valid
``` 
This isn't very pretty but it's a case that doesn't occur in practice.

When instantiating a map, when a key is a string that follows variable name rules, the encoling quotes can be dropped:
```
# These are equivalent
+{"name": "Francesco", "age": 25};
+{name: "Francesco", age: 25};
```
If instead you wanted to use the variable named `name` as a key, you can do that by adding some redundancy:
```
name = "x";

# These are equivalent
+{(name): "Francesco"};
+{ +name: "Francesco"};
+{"x": "Francesco"};

# And are different from these
+{name: "Francesco"};
+{"name": "Francesco"};
```
Similarly, when querying a map for an item associated to a string that follows variable name rules, you can use the dot operator:
```
# These are equivalent
me["name"] = "Francesco";
me.name = "Francesco";
``` 

### 2.11 - Function calls

We haven't seen how function definitions work yet, but you can imagine they work like other languages such as Python or JavaScript for now. Assuming we defined a function named `sayHello`, we can call it using the usual `()` notation:
```
sayHello();
sayHello(1);
sayHello(1, 2, 3);
```

### 2.12 - Functions useful for collections
count, keysof

## 3 - If-else statements
### 3.1 - Basics
An if-else statement lets you specify which portions to code the interpreter must run based on the result of an expression.
The syntax of an if-else statement is the following:
```py
if condition: {
    # Executed when the condition is true
} else {
    # Executed when the condition is false
}
```
Unlike expressions statements, they don't end with a `;`.

The condition may be any type of expression, but must evaluate to a boolean type. No implicit casts are performed. 

When the `else` block is empty, it can me omitted:
```py
if condition: {
    # Executed when the condition is true
}
```

If the blocks only contain one statement, it's possible to omit the curly brackets:
```py
if condition:
    doSomething();
else
    doSomethingElse();
```

### 3.2 - Chains
Since curly brackets can be dropped for blocks with only one statement, the following code:
```py
if cond0: {
    doSomething();
} else {
    if cond1: {
        doSomethingElse();
    } else {
        doSomethingDumb();
    }
}
```
can be simplified to
```py
if cond0: {
    doSomething();
} else if cond1: {
    doSomethingElse();
} else {
    doSomethingDumb();
}
```
creating a chain of if-else statements.

### 3.3 - Compound statements
Actually the meaning of the curly brackets is to group multiple statements into one.

The if-else statement expects only one statement for each branch, though it's possible to provide more than one statement each by wrapping them into curly brackets.

### 3.4 - Scopes
If-else and compound statements don't create new scopes, which means that variables defined inside one of those statements will be accessible outside of them:
```py
if 1 < 2:
    a = 10;
# Here "a" is still defined.
print(a); # Prints 10
```
```py
{
    a = 1;
}
print(a); # Prints 1
```

## 4 - Loops
You can tell the Noja interpreter to execute a block of code iteratively based on the result of an expression using while and do-while statements. 

The condition may be an expression of any kind, but must always evaluate to a boolean value.

### 4.1 - While loops
A while statement has the following form:
```py
while condition: {
    doSomething();
    doSomethingElse();
}
``` 
as you can see, it doesn't need an ending `;`.

When the interpreter encounters a while statement, it evaluates the condition. If the condition is true, it executes its body. When the execution of the inner block of code is completed, the execution jumps back to the condition, evaluating it again. If it's again true, the inner code is executed again, else the execution jumps after the while statement. This mechanism can go on potentially for ever!

Like for if-else statements, when the inner block only has one statement, you can drop the `{}`
```py
while condition:
    doSomething();
doSomethingElse(); # This isn't in the while loop's body!
```

### 4.2 - Do-while loops
Do-while loops behave very similarly to while loops, but have minor differences. Do-while loops evaluate the condition *after* each iteration.

They're used like this
```py
do {
    doSomething();
    doSomethingElse();
} while condition;
```
or, if the body only has one statement, like this
```py
do
    doSomething();
while condition;
```

Unlike while loops, do-while loops execute at least once and must end with a `;`.

### 4.3 - Scoping
Loops don't create new variable scopes. When defining a variable in a loop statement (either in the condition or the body) they're defined relative to the loop's parent scope.

### 4.4 - Break jumps
Inside any type of loop, it's always possible to break out of it using the `break` statement. 

```js
while condition: {
    
    if shouldStopLoopin():
        break;

    doSomething()
}
```
the `break` statement will immediately move the execution to after the while or do-while statement.

When inside multiple nested loops, the `break` statement will refer to the inner loop.

## 5 - Functions
### 5.1 Definition and basics

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

### 5.2 - Scoping and closures
 
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

## 6 - Type assertions
Type assertions are a way to check that a function is called with the proper arguments. All of the checks are done at runtime. Each argument may be associated to one or more types. If the function is called with a type other than the specified ones, a runtime error is triggered.

Here's an example of a function that can only take integer arguments:
```
fun add(a: int, b: int)
    return a + b;

c = add(2, 3); # OK
c = add(2, 3.0); # Error!! Second argument is a float but an int was expected.
```
The types you can assert are `int`, `float`,  `None`, `Bool`, `List`, `Map`, `Buffer`, `File`, `Directory`, `String`, `any`.

The identifier `any` allows any value and is the default.
### 6.1 - Sum type operator
You can allow multiple values using the sum type operator `|` :
```
fun add(a: int, b: int | float)
    return a + b;

c = add(2, 3.0); # Now this is ok!
```
and, optionally, define a new symbol for the sum type
```
Numeric = int | float;
fun add(a: Numeric, b: Numeric)
    return a + b;
```
### 6.2 - Nullable type operator
If an argument can be a given type or `none`, you can use the nullable type operator `?`
```
fun someFunction(optional_integer: ?int) {
    # ..do stuff..
}
```
which is equivalent to `int | None`.

### 6.3 - Default arguments and type hints
Default arguments are evaluated before the type assertions, therefore when `none` is provided as argument value, no error is triggered if a proper default argument was specified, even when it wasn't allowed as a type. If the default value doesn't result in a valid type, an error is triggered.

```
fun someFunction(a: int = 4) {}
someFunction(none); # No error. The argument value will be 4.

fun someFunction2(a: int) {}
someFunction2(none); # Error!

fun someFunction3(a: int = 1.3) {}
someFunction3(none); # Error!
```
### 6.4 - Map and List type assertions
For maps and lists, it's often useful to also assert the types of their children values.
You can do it using this syntax:
```
# This function only allows map arguments
# with fields "name" of type "String" and 
# "age" of type "int".
fun sayHello(me: {name: Stirng, age: int}) {
    print("Hello, I'm ", me.name, " and I'm ", me.age, " years old!\n");
}

# This function only allows an argument
# which contains a pair of integers:
fun printCoordinates(coords: [int, int]) {
    print("x=", coords[0], ", y=", coords[1], "\n");
}
```
This functionality is limited since it doesn't allow assertions about lists and maps of variable length, but still it's very useful in practice!
You can also define new symbols if you want
```
Person = {name: String, age: int};
fun sayHello(me: Person) {
    # ...
}
```
