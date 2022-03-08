# The Noja language

## Introduction
This language was written as a personal study of how interpreters and compilers work. For this reason, the language is very basic. One of the main inspirations was CPython.

## Supported platforms
I wrote it on a linux machine, but there should be very few places where a linux host is assumed, so it should be very easy to port.

## Development state
The interpreter is fully functional, but lots of built-in functions that one would expect still need to be implemented. Unfortunately, I feel like this requires much more work than what it's worth at the moment.

## Implementation overview
The architecture is pretty much the same as CPython. The source code is executed by compilig it to bytecode. The bytecode is much more high level than what the CPU understands, it's more like a serialized version of the AST. For example, some bytecode instructions refer to variables by names, which means the compiler does very little static analisys. Memory is managed by a garbage collector that moves and compacts allocations.

More detailed explanations are provided alongside the code.

## Build

To build it, just run:
```sh
$ ./build.sh
```
it will create a `build` folder where the interpreter's executable will be generated.

You may need to give executable permissions to the script. You can do so with by running:

```sh
$ chmod +x build.sh
```

## Usage

You can run files by doing:
```sh
location/of/noja run <filename>
```

or you can run strings by doing:
```sh
location/of/noja run inline <string>
```