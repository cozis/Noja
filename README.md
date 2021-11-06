# The Noja language

## Introduction

This language was written as a personal study of how interpreters and compilers work. For this reason, the language is very basic. One of the main inspirations was the CPython's source code since it's extremely readable and has a very simple and clean architecture.

## Implementation overview

The interpreter works by compiling the provided source to a bytecode 
format and executing it. The bytecode is very high level since it does things like:

  - explicitly referring to variables by name.

  - treating values as atomic things: from the perspective of the 
    bytecode, a list and an integer occupy the same space on the 
    stack, which is 1.

  - referring to instructions by their index.

All values (objects) are allocated on a garbage-collected heap. All variables are simply references to these objects. The garbage collection algorithm is a copy-and-compact. It behaves as a bump-pointer allocator until there is space left, and when space runs out, it creates a new heap, copies all of the alive object into it, calls the destructors of the dead objects and frees the old one.

