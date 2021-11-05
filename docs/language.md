
# The Noja language

## Table of contents

## Introduction

  This language was written as a personal study of how interpreters
and compilers work. For this reason, the language is very basic.
  One of the main inspirations was the CPython's source code since
it's extremely readable and has a very simple and clean architecture.

  This file was intended for people who already program in other 
high level languages (such as Python, Javascript, Ruby) and don't 
need to be introduced to basic programming concepts (variables, 
expressions and branches). This way, there is more space for the 
comparison of the language's features with the mainstream languages.

## Implementation overview

  The interpreter works by compiling the provided source to a bytecode 
format and executing it. The bytecode is very high level since it
does things like:

  - explicitly referring to variables by name.

  - treating values as atomic things: from the perspective of the 
    bytecode, a list and an integer occupy the same space on the 
    stack, which is 1.

  - referring to instructions by their index.

For example, by compiling the following snippet
```py
define = true;

if define:
        a = 33;

print(a, '\n');
```
one would obtain the following bytecode:
```
	 0: PUSHTRU 
	 1: ASS     "define" 
	 2: POP     1 
	 3: PUSHVAR "define" 
	 4: JUMPIFNOTANDPOP 8 
	 5: PUSHINT 33 
	 6: ASS     "a" 
	 7: POP     1 
	 8: PUSHSTR "\n"
	 9: PUSHVAR "a" 
	10: PUSHVAR "print" 
	11: CALL    2 
	12: POP     1 
	13: RETURN

```
as you can see, there are instructions like ASS and PUSHVAR that
assign to and read from variables by specifying names, and jumps
that refer to other points of the "executable" by specifying indices
(like JUMPIFNOTANDPOP) instead of raw addresses.

  All values (objects) are allocated on a garbage-collected heap. 
For this reason all variables are simply references to these objects.
The garbage collection algorithm is a copy-and-compact one. It 
behaves as a bump-pointer allocator until there is space left, 
and when space runs out, it creates a new heap, copies all of the 
alive object into it, calls the destructors of the dead objects 
and frees the old one.

## The first program

The sintax is similar to Python's but is more C-like. A Noja script 
is a list of statements that can be of multiple kinds:

  - function declaractions
  - expressions
  - if-else branches
  - while loops
  - do-while loops
  - return statements
  - composit statements

In general, unless it's inside strings, whitespace is ignored and 
comments start with the # character. 

The most basic yet interesting program is:
```py
print('Hello, world!\n'); 
```
as in other languages, this kind of statement is an expression.
Expression statements require a ';' to determine their end.

The print function can take any number of arguments of any type
and doesn't add any spaces or newlines to the output. 
```py
print(1, 2, 3, '\n');
```

## Expressions

You can set variables without declaring them first by using the
assignment operator:
```py
a = 5;
```
which is similar to Python's assignment, but is a little different.
In this language, assignments are considered as expressions, in fact 
you can do things like
```py
a = (b = 1) + 1;
```
The value resulting from an assignment is the assigned value.
After this expression, b's value is 1 and a's value is 2. 
```py
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
error:
```py
(Uncomment the following line and run this file to get the error)
# p = 5 + 'hello'; 
```
And relational operators are also available:
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

### Booleans
TODO

### None
TODO

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
or you can specify an alternative branch, which is executed when the 
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
```
Now x is defined, but if "a" were to be higher or equal to 2, it 
wouldn't be defined and the runtime would return an error.
