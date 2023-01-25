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
Here's an example of a noja program taken from the example HTTP server in `examples/http_server/main.noja`
```
router->plug("/login", {
    GET: 'pages/login.html',
    fun POST(req: Request) {

        username, _, error = getUsername(req);
        if error != none:
            return respond(400, error);
        if username != none:
            return respond(400, "You're already logged in");

        fun areValid(params) {
            expect = {username: String, password: String};
            return istypeof(expect, params) 
               and count(params.username) > 0
               and count(params.password) > 0;
        }

        params = urlencoded.parse(req.body);
        if not areValid(params):
            return respond(400, "Invalid parameters");
        username = params.username;
        password = params.password;
        
        info = user_table[username];
        
        if info == none:
            return respond(200, "No such user");
        if info != password:
            return respond(200, "Wrong password");
        
        # Log-in succeded

        # Create a new session
        session_id = random.generate(0, 1000);
        session_table[session_id] = username;

        message = string.cat("Welcome, ", username, "!");           
        headers = {
            "Location"  : "/home", 
            "Set-Cookie": string.cat("SESSID=", toString(session_id))
        };
        return respond(303, message, headers);
    }
});
```

## Implementation Overview
The architecture is pretty much the same as CPython. The source code is executed by compilig it to bytecode. The bytecode is much more high level than what the CPU understands, it's more like a serialized version of the AST. For example, some bytecode instructions refer to variables by names, which means the compiler does very little static analisys. Memory is managed by a garbage collector that moves and compacts allocations. 

(More detailed explanations are provided alongside the code.)

## Supported platforms
I wrote it on a linux machine, but there should be very few places where a linux host is assumed. It should be very easy to port.

## Development state
The interpreter is fully functional, but many built-in functions that one would expect still need to be implemented. At this time the priority is writing documentation and tests so that more people can try it, give feedback and move forward without breaking the world!

## Build
To build the interpreter, run:
```sh
$ make
```
The `noja` executable will be generated, which is a command-line interface that runs Noja code.

By default, the compilation is in release mode. The mode can be specified through the `BUILD_MODE` variable
```sh
$ make -B BUILD_MODE=DEBUG
```
the `-B` option forces the makefile to rebuild all of the code, ignoring previous compilation artifacts. This is probably what you want when changing the build mode.

Also, a mode for analyzing code coverage is available if your interested in that kind of stuff
```sh
$ make -B BUILD_MODE=COVERAGE
```

## Usage
You can run files by providing a path to the command line utility, or you can simply provide the string to be executed
```sh
$ noja <filename>
$ noja -i <string>
```

More usage information can be accessed using the `-h` option
```sh
$ noja -h
```

## Testing

Running `make` will also generate the `test` executable, a program necessary to run the testcases in the `tests/` folder. A testcase is a text file with extension `.noja-test`.

Tu run all tests, you can do:
```sh
$ ./test tests
```
or you can execute specific suites of tests like this:
```sh
$ ./test tests/compiler/expr tests/runtime/push
```

By running `report_test_suite_coverage.sh`, the test suite will be executed and a report of the code coverage will be stored in the `/report` folder. Note that this script will compile the CLI in coverage mode, so you'll need to rebuild it again for generic use cases.
