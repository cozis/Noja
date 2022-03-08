
# The Noja language

This file was intended for people who already program in other high level languages (such as Python, Javascript, Ruby) and don't need to be introduced to basic programming concepts (variables, expressions and branches). This way, there is more space for the comparison of the language's features with the mainstream ones. 

## Table of contents
3. [The first program](#the-first-program)
4. [Expressions](#expressions)
5. [Branches](#branches)
6. [Loops](#loops)
7. [Functions](#functions)

## A Noja program

A Noja program is a list of statements that can be of multiple kinds (function delcarations, expressions, if-else branches, etc). Any whitespace, if not inside strings, is ignored. Comments start with the `#` character and end with the line.

## Expressions

The most basic type of statement of a noja program is an expression. They work similarly to other languages. You can evaluate mathematical, relational, logical operartions, call functions and lots of other good stuff. Then, you can store the result of expressions inside variables and use them later.

The basic type of values are
```py
3; # Integers:
   #   They are represented internally by 64 bits and are always signed, therefore
   #   the minimum value representable is -2^63 and the maximum is 2^63-1.

3.4; # Floats
     #   Like integers, they are represented using 64 bits.

"hello!"; # Strings (which can be enclosed by double or single quotes)
'hello!'; #   They represent text. They are encoding-agnostic, which means that the
          #   language doesn't assume their encoding because it treats them as
          #   sequence of bytes. This may not be the most ergonomic choice, but it
          #   was the simplest one. 

true;  # Booleans
false; #   Nothing new here. They represent two values that have the property of being
       #   each other's logical negation. 

none; # The none value
      #   This is specific to the language. It can be used to represent the absence of
      #   a value when one was expected. It's only propery is to be identical to and
      #   only to itself.

[1, 2, none, false]; # Lists
                     #   They are ordered and etherogenic collections of other values.
                     #   Nested values can be accessed by their index relative to the
                     #   start.

{ 1: 'hello', true: [1, 2]} # Maps
                            #   Maps are a list of associations between values. You can
                            #   see them as a list of key and value pairs. The values
                            #   are the content of the map and the keys are what you
                            #   use to access them. Both keys and values can be any type
                            #   of value.
```



You can set variables without declaring them first by using the assignment operator:
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
all of the basic arithmetic operators are available:
```py
x = 1 + 1;
y = 1 - 2;
z = 3 * 2;
w = 10 / 3;

print('x = ', x, '\n'); # x = 2
print('y = ', y, '\n'); # y = -1
print('z = ', z, '\n'); # z = 6
print('w = ', w, '\n'); # w = 3
```
Note how the division returns the rounded down version of the result.
This is because the division was performed on integers. By making one
of the operands a floating point value, also a floating point result
is returned:
```py
w = 10 / 3.0;

print('w = ', w, '\n'); 
```
Arithmetic operators are only available for numeric types of objects.
If you try to apply them on other kinds of types, you get a runtime
error.

Relational operators are also available:
```py
print(1 < 2, '\n');  # true
print(1 > 2, '\n');  # false

print(1 >= 0, '\n'); # true
print(1 <= 0, '\n'); # false

print(1 == 5, '\n'); # false
print(6 == 6, '\n'); # true

print(1 != 5, '\n'); # true
print(6 != 6, '\n'); # false
```
The equal and not equal operators are available on every type of object,
while the others are only available for numeric types.

## Branches

It's possible to make the execution of a statement optional, based on the
result of an expression. Like in other languages, you do this using if-else
statements:

```py
if 1 < 2:
	print('Took the branch!\n'); # This is executed!

if 1 > 2:
	print('Didn\'t take the branch\n'); # This isn't!
```
..or you can specify an alternative branch, which is executed when the 
condition isn't true:

```py
if 1 > 2:
	print('Not executed..\n');
else
	print('Executed!\n');
```
You can have multiple statements inside a branch by having them inside a 
compound statement. Compound statements are statement lists wrapped inside
curly brackets, like this:

```py
{ print('Hello from a '); print('compound statement!\n'); }
```
This way they count as one statement.

```py
if 1 == 1:
	{
		print('Executed\n');
		print('Also executed\n');
	}
```

Variables defined inside an if-else statement's branch are defined
in the parent's context. This implies that variables may or may not
be defined when you access them, based on which branch is taken.

```py
a = 1;

if a < 2:
	x = 100;

# Now x is defined, but if "a" were to be higher or equal to 2, it 
# wouldn't be defined and the runtime would return an error.
```

## Loops

Looping constructs are available in the form of while and do-while 
statements. The while statement checks the condition before each
iteration:

```py
i = 0;
while i < 10:
	i = i + 1;
```

This loop runs for 10 times. As for the if-else statement, a single
statement is expected as the body of the while statement. You can
provide it a compound statement tho.

```py
i = 0;
while i < 10:
	{
		print('While iteration no. ', i, '\n');
		i = i + 1;
	}
```

The do-while statement checks the condition at the end of each
iteration. This means that at least one iteration is performed!

```py
i = 0;
do
	{
		print('Do-while iteration no. ', i, '\n');
		i = i + 1;
	}
while i < 10;
```

Like for if-else statements, variables defined inside the loop
body are shared with the parent's context.

## Functions

Functions can be defined using the following syntax:

```py
# Define it
fun say_hello_to(name)
	print('Hello, ', name, '!\n\n');

# .. and then call it.
say_hello_to('Francesco');
```

Functions can have an arbitrary amount of arguments. If the function is
called with more arguments than it expected, the extra values are thrown
away. If the function is called with less arguments than it expected,
the argument set if filled up with none values.

```py
fun test_func(a, b, c)
	{
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

Functions are actually variables like the ones that are be defined using
the assignment operator. In fact, you can reassign them new values if you
want.

```py
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

If the function doesn't return any values, then the `none` value is returned.
As an example, the `print` function always returns `none`

```py
print(print()); # none
```