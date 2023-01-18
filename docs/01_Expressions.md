
# EXPRESSIONS

## Basics
Expression use infix notation. You can have expressions of numeric values, boolean values and other datatypes like strings of text. When expressions are used as statements, they need to be terminated using a semicolon. Here's an example:
```
2 * (1 + 2); 
```
The basic values that can be used are integers, floats, booleans and "none".
 
## Integers, floats and arithmetic operators
 
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

## Booleans and logical operators

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


## Relational operators

Relational operators are those which evaluate to booleans and take, in general, non-boolean operands. They are:

* equal `==`
* not equal `!=`
* less than `<`
* less than or equal `<=`
* greater than `>`
* greater than or equal `>=`
 
Equal (not equal) can be applied to all type of operands and return true (false) when the operands have the same type and hold the same value. These operator don't allow funny business like `1 == "1"` evaluating to `true`.
 
The remaining relational operators are applied to numeric datatypes (ints and floats). 

## The none value

The none value used to represent the void of a value and has the only property of being equal to itself and itself only. You can use the none value using the `none` keyword:

```
x = none; 
```

## Variables and assignments

You can store computed values into variables in order to reuse them later on. Variables are created using the assignment operator:
```
x = 1 + 4;
y = x + 2;
```
here we're assigning to the variable "x" the number 5 then, we're assigning to "y" the value 7 by accessing the value previously stored into "x". The left operand of the assignment operator must be a variable name while the right operator can have any type.

Variable names can consist of digits, letters or underscores, but the first character can't be a digit though.
 
Since the assignment operator is an operator, other than do an assignment it also returns a value. Any assignment evaluates to it's right operand. By instance
```
y = (x = 1 + 4) + 2;
```
this expression will result in `x` having value 5 and `y` value 7. This is because `x = 1 + 4`, other than assigning to `x`, is equivalent to writing `5`.


## Composit types and square bracket notation

Composit values are collections of other values, which may also be composite. The composite types are `List`, `Map` and `String`. For all collection types it's possible to insert and retrieve values using the `[]` notation:
```
coll[key] = item; # Store the value associated to the 
                  # variable "item" with key "key" in 
                  # the collection "coll".

item = coll[key]; # Get the item back by selecting it 
                  # using it's key
```
In this example, the "coll" variable is a collection type, while the types of "key" and "item" depend on the type of collection.


## Lists

Lists are heterogeneous and ordered collections of values. Each item they contain is associated to it's position in the list (the first element has position 0). They're defined and used with the following syntax:
```
my_list = [true, 1.2, 19];

x = my_list[0]; # true
y = my_list[2]; # 19

my_list[0] = 13;

z = my_list[0]; # z is 13 now!
```
Trying to access an item using as key something which isn't an integer or an in range integer (less then 0 or higher than the length of the array minus one) will result in an error. The only exception to this rule is the index equal to the item count of the list (which is out of bounds since the last item of the list has index equals to the item count minus one): by inserting a value at this index, the list will increase it's size by 1. There isn't a limit on how many values a list can contain.


## Strings

Strings are values which contain UTF-8 encoded text. A string can be instanciated placing text between single or double quotes:
```
"I'm a string!";

'I am too!';
```
Special character (such as horizontal tabs and carriage returns) can be specified using the `\x` notation:

* `\t` - tab
* `\r` - carriage return
* `\n` - newline

When strings contain quotes that match the ones surrounding them or the "\" character, it's necessary to escape them:
``` 
'Hi, I\'m Francesco!';
"Hi \"Francesco\", how old are you?";
"This is a backlash \\ and you can do nothing about it";
``` 
Like arrays, single characters can be selected referring to them by their position relative to the first character using the `[]` notation. When selecting single characters from a string, they're returned as new strings.

Once a string is created, it's not possible to modify it. If you want to change a string's value, you need to create a new updated version of the string.

 
## Maps and the dot operator

Maps are collections of key-value pairs, where both keys and values can have any type. The syntax for defining and using maps is this:
```
me = {"name": "Francesco", "age": 24};

my_name = me["name"]; # Francesco

me["name"] = true;

my_name = me["name"]; # true
```
When selecting from a map a value associated to a key which was never inserted, "none" is returned:
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
If instead you wanted to use the variable named "name" as a key, you can do that by adding some redundancy:
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

## Function calls

We haven't seen how function definitions work yet, but you can imagine they work like other languages such as Python or JavaScript for now. Assuming we defined a function named "sayHello", we can call it using the usual "()" notation:
```
sayHello();
sayHello(1);
sayHello(1, 2, 3);
```

## Functions useful for collections
count, keysof