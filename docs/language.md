
# The Noja language
This documentation was intended for people who already program in other high level languages (such as Python, Javascript, Ruby) and don't need to be introduced to basic programming concepts like variables, expressions and branches. Not having to introduce common programming concepts leaves more space to the characteristics specific to this language.

## TL;DR
Here's an overview:
1. The available types are `int`, `float`, `bool`
`string`. `list` (array), `map` (associative array), `none` (null)
3. No implicit casts.
2. Arithmetic operations only allowed on numbers.
3. Division between `int`s rounds down.
4. Logical operators expect boolean operands.
5. Assignments are expressions that return the newly assigned value.
6. No for loop (yet)

## Table of contents
3. [The first program](#the-first-program)
4. [Expressions](#expressions)
5. [Branches](#branches)
6. [Loops](#loops)
7. [Functions](#functions)

## A Noja program
A Noja program is a list of statements that can be of multiple kinds. Any whitespace, if not inside strings, is ignored. Comments start with the `#` character and end with the line. There are no multi-line comments.

## Expressions
The most basic type of statement is an expression. They work similarly to other languages. The basic types of values are

```py
3; # Integers:
   #   They are represented internally by 64 bits and are always signed, therefore
   #   the minimum value representable is -2^63 and the maximum is 2^63-1.

3.4; # Floats
     #   Like integers, they are represented using 64 bits.

"hello!"; # Strings:
'hello!'; #   They represent UTF-8 encoded text.

true;  # Booleans:
false; #   Nothing new here. They represent two values that have the property of being
       #   each other's logical negation. 

none; # The none value:
      #   This is specific to the language. It can be used to represent the absence of
      #   a value when one was expected. It's only propery is to be identical to and
      #   only to itself.

[1, 2, none, false]; # Lists:
                     #   They are ordered and etherogenic collections of values.
                     #   Nested values can be accessed by their index relative to the
                     #   start.

{ 1: 'hello', true: [1, 2]} # Maps:
                            #   Maps are a list of associations between values. You can
                            #   see them as a list of key and value pairs. The values
                            #   are the content of the map and the keys are what you
                            #   use to access them. Both keys and values can be any type
                            #   of value.
```

there are also other types of values (like functions and buffers) but they'll be covered further on.

This language is stricter than others in regards to type conversions and on what types you can apply operators.

### Operators
Operators in noja genererally behave like you would expect.

Arithmetic operators can only be used on numeric values
```py
+1; # 1
-1; # -1
1 + 1; #  2
1 - 2; # -1
3 * 2; #  6
10 / 3; # 3
10 / 3.0; # 3.33..
10.0 / 3; # 3.33..
```
If the operands are integers, the result also will. If a float is involved, then the result is also a float. The only relevant thing to note here is that the division between integers results in a rounding down of the float version.

Relational operators also can only be applied on numeric types, exception made only for `==` and `!=`, which can be used to compare any type of value. These operations always result in a boolean value.
```py
1 < 2;  # true
1 > 2;  # false
1 >= 0; # true
1 <= 0; # false
1 == 1; # true
1 != 1; # false
```

Logical operators can only be applied on booleans and also always return booleans:
```py
not true;  # false
not false; # true

true  and true;  # true
true  and false; # false
false and true;  # false
false and false; # false

true  or true;  # true
true  or false; # true
false or true;  # true
false or false; # false

not 1; # ERROR!
```

### Variables
Variable names can contain letters, digits and underscores (the first character can't be a digit though). You can define variables by assigning a value to them directly:
```py
a = 5;
```
which is similar to Python's assignment, but is a little different. In this language, assignments are considered as expressions, in fact you can do things like
```py
a = (b = 1) + 1;

# The value resulting from an assignment is the assigned value.
# After this expression, b's value is 1 and a's value is 2. 

print('b = ', b, '\n'); # b = 1
print('a = ', a, '\n'); # a = 2
```

## If-Else statements
Like in every other language, it's possible to make the execution of one or more statements optional. 
```py
if 1 < 2: {
	print('I\'m executed!\n');
}

if 1 > 2: {
	print('I\'m not..\n');
}

if 1 < 2: {
	print('I\'m executed!\n');
} else {
	print('I\'m not..\n');
}
```
If you only have one statement inside the if or the else branch, you can drop the curly brackets:

```py
if 1 < 2:
	print('I\'m executed!\n');

if 1 > 2:
	print('I\'m not..\n');

if 1 < 2:
	print('I\'m executed!\n');
else
	print('I\'m not..\n');
```

if-else statements don't create new variable scopes, which means variables defined inside an if-else statement's branch are defined in the parent's context. This implies that variables may or may not be defined when you access them, based on which branch is taken.

```py
# .. do stuff where you define a variable [a]..

if a < 2:
	x = 100;

print(x); # May abort the execution because if [a < 2] isn't true, x would be undefined.
```

## Loop statements
Looping constructs are available in the form of while and do-while statements:

```py
i = 0;

# Iterates 10 times.
while i < 10: {
	i = i + 1;
}

i = 0;

# Iterates 11 times.
do {
	i = i + 1;
} while i < 10;

```

Like for if-else statements, variables defined inside the loop
body are shared with the parent's context.

## Functions

Functions can be defined using the following syntax:

```py
# Define it..
fun say_hello_to(name) {
	print('Hello, ', name, '!\n\n');
}

# .. and then call it.
say_hello_to('Francesco');
```

They can have an arbitrary amount of arguments. If the function is called with more arguments than it expected, the extra values are thrown away. If the function is called with less arguments than it expected, the argument set if filled up with none values.

```py
fun test_func(a, b, c) {
	print('a = ', a, '\n');
	print('b = ', b, '\n');
	print('c = ', c, '\n\n');
}

test_func();
# a = none
# b = none
# c = none

test_func(1, 2);
# a = 1
# b = 2
# c = none

test_func(1, 2, 3);
# a = 1
# b = 2
# c = 3

test_func(1, 2, 3, 4);
# a = 1
# b = 2
# c = 3
```

Functions are actually variables, like the ones that are defined using the assignment operator. In fact, you can reassign them new values if you want.

```py
fun test_func() {
	print('Hello!\n');
}

test_func = 5;

# The following line, if executed, returns an error because the test_func
# identifier is now associated to 5, which is not a function. 

test_func(); # Error!!
```

Functions can return values exactly like in other languages:

```py
fun multiply(x, y)
	return x * y;

p = 4;
q = 7;
r = multiply(p, q);

print(p, ' * ', q, ' = ', r, '\n');
```

If the function doesn't return any values, then the `none` value is returned. For example, the `print` function always returns `none`

```py
print(print()); # none
```

Functions can also access variables that weren't defined locally but in parent scopes, like the global one.
```py
a = 10;
fun f() {
	print(a, '\n');
}

f(); # 10
```

a common example to explain this feature is using the adder function generator
```py
fun makeAdder(n) {
	fun adder(m) {
		return m + n;
	}
}

add10 = makeAdder(10);
add20 = makeAdder(20);
print(add10(1), '\n'); # 11
print(add20(1), '\n'); # 21
```