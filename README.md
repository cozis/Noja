# The Noja language

## Introduction

This language was written as a personal study of how interpreters and compilers work. For this reason, the language is very basic. One of the main inspirations was CPython's source code.

## Implementation overview

The architecture is pretty much the same as CPython. 
The source code is compiled to a bytecode format and executed. The bytecode is very high level since it does things like explicitly referring to variables by name, which means the compiler does very little static analisys and errors are moved from the compile to run time.
Memory is managed by a garbage collector that moves and compacts allocations.


All values (objects) are allocated on a garbage-collected heap. All variables are simply references to these objects. The garbage collection algorithm is a copy-and-compact. It behaves as a bump-pointer allocator until there is space left, and when space runs out, it creates a new heap, copies all of the alive object into it, calls the destructors of the dead objects and frees the old one.

