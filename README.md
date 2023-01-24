# The Noja language
1. [Introduction](#introduction)
2. [Objective](#objective)
3. [Show me the code!](#show-me-the-code)
4. [Implementation overview](#implementation-overview)
5. [Supported platforms](#supported-platforms)
6. [Development state](#development-state)
7. [Build](#build)
8. [Usage](#usage)
9. [Testing](#testing)

## Introduction
This language was written as a personal study of how interpreters and compilers work. For this reason, the language is very basic. One of the main inspirations was CPython.

Noja is a very high level and dynamic language. It operates in the same range of abstraction as languages like Ruby, Python and Javascript. 

## Objective
This project aims at being an interpreter design reference, therefore it optimizes for code quality and readability. That's not to mean that it won't be feature-complete. The end goal is to have a language you can do arbitrarily complex things in.

## Show me the code!
Here's an example of a noja program that orders the items in list using a bubble sort
```
L = [3, 2, 1];

fun swap(a: int, b: int)
    return b, a;

fun order(L: List) {

    do {
        swapped = false;

        i = 0;
        while i < count(L)-1 and not swapped: {

            swapped = L[i+1] < L[i];
            if swapped:
                L[i], L[i+1] = swap(L[i], L[i+1]);
            
            i = i+1;
        }
    } while swapped;
}

print(L, '\n'); # [3, 2, 1]
order(L);
print(L, '\n'); # [1, 2, 3]
```

## Implementation Overview
The architecture is pretty much the same as CPython. The source code is executed by compilig it to bytecode. The bytecode is much more high level than what the CPU understands, it's more like a serialized version of the AST. For example, some bytecode instructions refer to variables by names, which means the compiler does very little static analisys. Memory is managed by a garbage collector that moves and compacts allocations.

(More detailed explanations are provided alongside the code.)

## Supported platforms
I wrote it on a linux machine, but there should be very few places where a linux host is assumed. It should be very easy to port.

## Development state
The interpreter is fully functional, but lots of built-in functions that one would expect still need to be implemented. Unfortunately, I feel like, at the moment, this requires much more work than what it's worth. At this time the priority is writing documentation and tests so that more people can try it, give feedback and move forward without breaking the world.

## Build
To build the interpreter, run:
```sh
$ make
```
The `noja` executable will be generated, which is a command-line interface that runs Noja code.

## Usage
You can run files by doing:
```sh
location/of/noja <filename>
```

or you can run strings by doing:
```sh
location/of/noja -i <string>
```

## Testing

Running `make` will also generate the `test` executable, which is a program that lets you run the testcases in the `tests/` folder. A testcase is a text file with extension `.noja-test`.

Tu run all tests, you can do:
```sh
$ ./test tests
```
or you can execute specific suites of tests like this:
```sh
$ ./test tests/compiler/expr tests/runtime/push
```
