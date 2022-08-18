# Expressions

Expressions behave very much like Python, with few exceptions.

You can evaluate arithmetic, relational and logical operations or call into functions defined by you or the runtime!

Here are some examples:
```py
2 * (1 + 4);
```
expressions are delimited by semicolons.

This language is very strict in the implicit casts that it allows.

## Primitive types
Primitive types (or, more accurately, non aggregate types) are:
* signed integers (`int`)
* floats (`float`)
* booleans (`bool`)
* strings (`String`)
* the none value (`None`)

When referring to "numeric" values, only integers and floats are implied.

### Integers and floats
Integers and floats correspond to C's `long long int` and `double` types, so they are 8 bytes big each on most systems.

### Booleans
As in most languages, the boolean type of value is one that can only assume the `true` or `false` value. These values are such that the logical negation of one (`not`) gives the other.

### Strings
Strings are immutable sequences of text encoded as UTF-8.

### The none value
The none value is a value that's used to represent the void of a value. The token for the none value is `none`, while the none type is `None` (with a capital letter). 

An example of usage of `none` is when assigning to a variable the result of a function call that doesn't return a value (such as `print`).

## Arithmetic operations
Arithmetic operations are:
* negation (unary `-`)
* addition (`+`)
* subtraction (`-`)
* multiplication (`*`) 
* division (`/`)
and they are only allowed on numeric operands.

An operation on integers returns an integer, unless the operation is a division, in which case the result is a float. If one of the operands is a float, then the result is also a float. 

If an integer overflow or underflow occurres, a runtime error triggers and the execution stops. Floating point exceptions aren't reported though.

## Relational operations
The relational operands are:
* lower (`<`)
* greater (`>`)
* equal (`==`)
* not equal (`!=`)
* lower or equal (`<=`)
* greater or equal (`>=`)
and they either return `true` or `false`.

The `==` and `!=` operands may be applied to any primitive type, while the remaining ones may only be applied to numeric values.

The `==` operator may only return `true` when the operands have the same value. 

## Logical operations
The logical operations in Noja are:
* `and`
* `or`
* `not`
They can only be applied to boolean values and always return a boolean value. 

Note that this isn't always true for other languages. For example python allows operands of any kind. The return value in python also isn't always a boolean. It returns the left operand if it's considered to be equivalent to `True`, else the right operand is returned.

The `and` and `or` operands are short-circuit operands, which means that they only evaluate the minimum amount of operands that's required to know the result. This is true in most languages. For example, if the left operand of an `and` operation is `false`, the right one will never be evaluated since the result will inevitably be `false`. This is useful because often the right operand will assume the left one to be `true` or `false`.

The `and` operand has a slightly higher priority than `or`.

## Variables and assignments
It's possible to store computed values into *variables* in order to use them later on.

The syntax to store a value to a variable is:
```py
variable = 1 + 4;
```
This is usually referred to as an assignment. On the left side of the assignment there must be a valid identifier. On the right can be any expression.

A valid variable name may contain alphabetical letters, digits or underscores, but a digit can't be the first character.

The assignment token `=` is actually an operator that returns the assigned value. By instance the following expression
```py
(a = 3) + 1;
```
evaluates to 4.

The default behaviour of Noja's `=` is the same as Python's `:=`.

To use the value assigned to a variable, just name the variable where you want the assigned value to go.
```py
six = variable + 1;
```

## Aggregate types
The aggregate types available in Noja are `List` and `Map`.

The `List` type is an heterogeneous ordered collection of items that can have any type and can be accessed by their index. They're usually referred to as arrays in other languages.

The `Map` type is an heterogeneous unordered collection of key-value pairs, where both key and values can have any type. Values are retrieved by their key. They are usually referred to as associative arrays.

## Lists
Lists are defined as a comma-separated list of expressions between square brackets:
```py
[none, 2 + 1, true];
```

The `n`-th list can be accessed using the `[]` notation:
```py
ls = [true, false, none];
ls[1]; # false
```
where the list value is followed by `[]` which contain the index of the item to retrieve.

The index may be evaluated dynamically, but it will trigger a runtime error if it doesn't evaluate to an integer value, aborting the execution of the whole program
```py
ls = [true, false, none];
ls[0 + 2]; # OK!
ls[true or false]; # Runtime error!!
```

## Maps
Maps are defined as a list of key-value pairs:
```py
me = {"name": "Francesco", "age": 25};
```
keys can be of any type (even non-immutable ones).

To retrieve a value, the syntax is analogous to the `List` case
```py
name = me["name"];
```

The keys may be evaluated dynamically and, unlike `List`s, any type of key is allowed.

When instanciating a map, if a key is a string and follows the rules of variable identifiers, it's possible to use it without double quotes:
```py
# These are equivalent
a = {"name": "Francesco", "age": 25};
b = {name: "Francesco", age: 25};
```

If you don't want to use the identifier as key but evaluate it as a variable to use it's value as key, then you can explicitly tell the interpreter that it's an expression
```py
name = "x";

# These are equivalent
a = {   "x": "Francesco", "age": 25};
b = {(name): "Francesco", age: 25};
c = { +name: "Francesco", age: 25};
```

Because of compound statements (discussed in the next chapter), an expression can't start with the `{` of a map literal because the interpreter will assume it's a compount statement.
The following statement is invalid
```py
{'name': 'John'};
```
because since it starts with a `{` it's assumed to be a compound statement but a compount statement can't contain a `:` like that. To tell the parser that it must interpret it as a map, it's possible to add some redundancy:
```py
# These are equivalent
+{'name': 'John'};
({'name': 'John'});
```
This is valid code that's interpreted correctly.

## Selection with multiple keys
In general, when you have a collection type, you can retrieve items using the `[]` notation.

The `[ .. ]` is actually a `List` expression. You can provide more than one value as key:
```py
val = coll[key0, key1];
``` 
in which case, the key will be a `List` containing the list of provided keys. Since `List`s only allow `int` keys, this only can be used on `Map`s.