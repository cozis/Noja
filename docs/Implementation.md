# Implementation

(This is still work in progress!)

## 1 - The Object Model
The way the runtime operates on values is through a specific interface. Any value that implements this interface is referred to as an "object". The interface makes it possible to run noja code in a polymorphic way. In other words, it's possible to implement algorithms that operate on values in an abstract way, without knowing their specific layout in memory. For people who know C well, this is not a new concept and to them it's enough to say that all object must have a header field which refers to their method table. Many people will not know what this means though and it's necessary to explain it in more detail.

### 1.1 - Polymorphism
Different kinds of values have different layouts in memory and methods that can change their state. Since these methods depend on the specific layout, different values with different layouts will have 
different methods which operate in a very different way. Though is possible that for some values there are different yet analogous methods: methods which operate differently but have equivalent semantic meaning. An example of this is the "insert(set, idx, value)" method of a tree data structure versus an array. In both cases it inserts a value in the set at a given index, but the way it does it is very different in both cases. Polymorphism is a way to write an algorithm using these methods in an abstract way and, based on which value it operates on, the specific implementation for that value is used. In practice, this is done through the use of function pointers. All objects define their specific fields in a structure, and in the first half of the structure they place the pointers to their methods that have shared meaning with other objects. In the case of trees and arrays, we may say that the first pointer must be to the "insert" method, the second to the "select" method and the last to "delete". Each value will fill these pointers so that all objects have as first field the pointer to the "insert" method etc. At this point code which needs to use a generic set can use these values through this interface (the pointers), making it possible to use it interchangably with values that implement that interface.

### 1.2 - Polymorphism in Noja
In Noja the interface is called "Object". All values that implement the interface are referred to as "objects". The interpreter can only operate on such objects. Since objects must implement many methods, they don't store pointers directly in their headers. Instead, they hold only one pointer to another struct which holds the method pointers. This way objects that share method implementations can share the pointers. As strange as it may sound, this method "table" is referred to as the "Type Object" and is also an object with its methods. The type object is defined in "objects.h". At the moment of writing this (commit 248fc1d1a88656d3a0b293581c4d70360a314ed2, 24 January 2023), it looks like this:

```c
struct TypeObject {

    Object base;
    
    const char *name;
    size_t      size;

    bool    (*init)(Object *self, Error *err);
    bool    (*free)(Object *self, Error *err);
    int     (*hash)(Object *self);
    Object* (*copy)(Object *self, Heap *heap, Error *err);
    int     (*call)(Object *self, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Heap *heap, Error *err);
    void    (*print)(Object *self, FILE *fp);

    // Collections.
    Object *(*select)(Object *self, Object *key, Heap *heap, Error *err);
    Object *(*delete)(Object *self, Object *key, Heap *heap, Error *err);
    bool    (*insert)(Object *self, Object *key, Object *val, Heap *heap, Error *err);
    int     (*count)(Object *self);
    Object *(*keysof)(Object *self, Heap *heap, Error *error);

    // Types.
    bool (*istypeof)(Object *self, Object *other, Heap *heap, Error *error);

    bool (*op_eql)(Object *self, Object *other);
    void (*walk)    (Object *self, void (*callback)(Object **referer,                    void *userp), void *userp);
    void (*walkexts)(Object *self, void (*callback)(void   **referer, unsigned int size, void *userp), void *userp);
};
```

As a convention, structures that implement objects have the "Object" word at the end of their name and embed the "Object" structure in the first field. The "Object" structure contains all of the fields that must be shared between objects, so it will include the pointer to the value's TypeObject. Following there are all of the methods that object can implement: there are generic methods such as "print" and "copy" and more specific ones like "select" and "insert". Object can only implement a subset of these by leaving the others set to NULL. You may have noticed that not all of the fields are functin pointers! The fields "name" and "size" refer to the name of the tyoe of object (such as "int" or "string") and the size (in bytes) of the object structure.

## 2 - Bytecode
Before a Noja script is executed, it is converted to a "bytecode" format, which is easier to evaluate. Bytecode is a format analogous to that of assembly but is higher level. The techniques used to convert source code to bytecode will be covered in the "Compiler" section. For now, we'll just show how each of the language's constructs is expressed in bytecode.

### 2.1 - Expressions
To evaluate a sequence of bytecode instructions, the interpreter uses a stack. Values are pushed and popped from the stack to evaluate expressions. By instance, the expression "1+2*3" would be translated to the following bytecode:
```
PUSHINT 1;
PUSHINT 2;
PUSHINT 3;
MUL;
ADD;
```
The instruction `PUSHINT` pushes an integer on the stack, `MUL` pops two objects and pushes back their multiplication, "ADD" pops two operands and pushes their addition.

Here's the stack instruction by instruction:
```
 Stack items | Last Executed instruction
 ------------+--------------------------
 1           | PUSHINT 1
 ------------+--------------------------
 1, 2        | PUSHINT 2
 ------------+--------------------------
 1, 2, 3     | PUSHINT 3
 ------------+--------------------------
 1, 6        | MUL
 ------------+--------------------------
 7           | ADD
 ------------+--------------------------
```
There are many other instructions which behave like `ADD` and `MUL` but perform other operations.

The arithmetic operations are `ADD`, `SUB` (subtraction), `MUL`, `DIV` (division) and `MOD` (modulo a.k.a. the remainder of division).

Other 

They are: SUB, DIV, MOD, NOT, AND,
OR, ..?

### 2.2 - Variables
### 2.3 - If-Else Statements
### 2.4 - While and Do-While loops
### 2.5 - Functions

<bytecode is a serialized version of the AST>


## 3 - The Compiler
When a string of Noja code must be run, it is first compiled to another
representation easier to evaluate called "bytecode". The bytecode
representation is an array of instructions analogous to machine code
instructions. The bytecode representation in the interpreter is called
an "Executable", and is implemented in "executable.c".

## 5 - Built-Ins

## 4 - Testing

[Encapsulation]