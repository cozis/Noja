
The source files directly contained by this folder expose to the user through the command line the functionalities implemented inside the subfolders.

The `main.c` file implements the entry of the program and, based on the options provided by the user, executes, parses or disassembles the noja source code. In `debug.c` is implemented the callback that `main.c` provides to the runtime if the user asks for an execution in debug mode that makes it possible to expose the internal state of the interpreter during the execution.

The `utils` folder contains the implementations of general purpose data structures and definitions that are useful through all of the codebase. Some of the more used data structures implemented in it are:

* `BPAlloc`: A bump-pointer allocator. It's useful during the compilation phase because lots of small objects need to be allocated and then freed at the same time when the final executable is produced.
* `Error`: A structure useful to report errors to function callers.
* `Source`: A string object that is used in place of raw strings to move source code around.

The `common` folder implements the `Executable` data structure, which contains the result of a source's compilation. It can be though about as an array of bytecode instructions that can be directly executed. 

The `compiler` folder implements the compiler of the interpreter. The main routine that is exported from here is `compile`, which transforms a `Source` into an `Executable`. Other functions are exported like `serialize` that transforms an `AST` to a JSON string. This subfolder is the only part of the codebase that should be able to access the `AST` nodes.

The `objects` folder implements the object model. In the context of this language, an object is a virtual class that implements a given set of methods. This folder exports functions that transform "raw" data types into objects, functions that do the inverse transformation and functions that trigger the virtual methods. This folder also contains the implementation of the heap and the garbage collector that needs to be tightly coupled with the object model.

The `runtime` folder implements the routines that run the `Executable`. It depends heavily on `objects`. It basically iterates over the executable's bytecode and applies the described actions to the objects.
It also implements a couple of objects that can't be implemented inside `objects` because they depend on the runtime internals.