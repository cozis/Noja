#include <stdio.h>
#include <assert.h>
#include "utils/error.h"
#include "utils/source.h"
#include "compiler/parse.h"
#include "compiler/compile.h"
#include "assembler/assemble.h"
#include "builtins/basic.h"
#include "noja.h"

static void print_error(const char *type, Error *error)
{
    if(type == NULL)
        fprintf(stderr, "Error");
    else if(error->internal)
        fprintf(stderr, "Internal Error");
    else
        fprintf(stderr, "%s Error", type);

    fprintf(stderr, ": %s.", error->message);

#ifdef DEBUG
    if(error->file != NULL)
    {
        if(error->line > 0 && error->func != NULL)
            fprintf(stderr, " (Reported in %s:%d in %s)", error->file, error->line, error->func);
        else if(error->line > 0 && error->func == NULL)
            fprintf(stderr, " (Reported in %s:%d)", error->file, error->line);
        else if(error->line < 1 && error->func != NULL)
            fprintf(stderr, " (Reported in %s in %s)", error->file, error->func);
    }
#endif
    
    fprintf(stderr, "\n");
}

static _Bool interpret(Executable *exe)
{
    Runtime *runt = Runtime_New(-1, 1024*1024, NULL, NULL);

    if(runt == NULL)
    {
        Error error;
        Error_Init(&error);
        Error_Report(&error, 1, "Couldn't initialize runtime");
        print_error(NULL, &error);
        Error_Free(&error);
        return 0;
    }

    // We use a [RuntimeError] instead of a simple [Error]
    // because the [RuntimeError] makes a snapshot of the
    // runtime state when an error is reported. Other than
    // this fact they are interchangable. Any function that
    // expects a pointer to [Error] can receive a [RuntimeError]
    // upcasted to [Error].
    RuntimeError error;
    RuntimeError_Init(&error, runt); // Here we specify the runtime to snapshot in case of failure.
    
    Object *bins = Object_NewStaticMap(bins_basic, runt, (Error*) &error);

    if(bins == NULL)
    {
        assert(error.base.occurred == 1);
        print_error(NULL, (Error*) &error);
        RuntimeError_Free(&error);
        Runtime_Free(runt);
        return 0;
    }

    Runtime_SetBuiltins(runt, bins);

    Object *rets[8];
    unsigned int maxretc = sizeof(rets)/sizeof(rets[0]);

    int retc = run(runt, (Error*) &error, exe, 0, NULL, NULL, 0, rets, maxretc);

    // NOTE: The pointer to the builtins object is invalidated
    //       now because it may be moved by the garbage collector.

    if(retc < 0)
    {
        print_error("Runtime", (Error*) &error);

        if(error.snapshot == NULL)
            fprintf(stderr, "No snapshot available.\n");
        else
            Snapshot_Print(error.snapshot, stderr);

        RuntimeError_Free(&error);
    }

    Runtime_Free(runt);
    return retc > -1;
}

static Executable *compile_source_and_print_error_on_failure(Source *src)
{
    Error error;
    Error_Init(&error);
    CompilationErrorType errtyp;
    Executable *exe = compile(src, &error, &errtyp);
    if(exe == NULL) {
        const char *errname;
        switch(errtyp) {
            default:
            case CompilationErrorType_INTERNAL: errname = NULL; break;
            case CompilationErrorType_SYNTAX:   errname = "Syntax"; break;
            case CompilationErrorType_SEMANTIC: errname = "Semantic"; break;
        }
        print_error(errname, &error);
        Error_Free(&error);
        return NULL;
    }
    Error_Free(&error);
    return exe;
}

static _Bool disassemble(Source *src)
{
    Executable *exe = compile_source_and_print_error_on_failure(src);
    if(exe == NULL) return 0;
    Executable_Dump(exe);
    return 1;
}

static _Bool interpret_file(const char *file)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromFile(file, &error);
    if(src == NULL)
    {
        assert(error.occurred == 1);
        print_error(NULL, &error);
        Error_Free(&error);
        return 0;
    }

    Executable *exe = compile_source_and_print_error_on_failure(src);
    if(exe == NULL) return 0;

    _Bool r = interpret(exe);

    Executable_Free(exe);
    Source_Free(src);
    return r;
}

static _Bool interpret_code(const char *code)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromString(NULL, code, -1, &error);

    if(src == NULL)
    {
        assert(error.occurred);
        print_error(NULL, &error);
        Error_Free(&error);
        return 0;
    }

    Executable *exe = compile_source_and_print_error_on_failure(src);
    if(exe == NULL) return 0;

    _Bool r = interpret(exe);

    Executable_Free(exe);
    Source_Free(src);
    return r;
}

static _Bool interpret_asm_file(const char *file)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromFile(file, &error);

    if(src == NULL)
    {
        assert(error.occurred == 1);
        print_error(NULL, &error);
        Error_Free(&error);
        return 0;
    }

    Executable *exe = assemble(src, &error);
    if(exe == NULL) {
        print_error("Assemblation", &error);
        Source_Free(src);
        Error_Free(&error);
        return 0;
    }

    _Bool r = interpret(exe);

    Executable_Free(exe);
    Source_Free(src);
    Error_Free(&error);
    return r;
}

static _Bool interpret_asm_code(const char *code)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromString(NULL, code, -1, &error);

    if(src == NULL)
    {
        assert(error.occurred);
        print_error(NULL, &error);
        Error_Free(&error);
        return 0;
    }

    Executable *exe = assemble(src, &error);
    if(exe == NULL) {
        print_error("Assemblation", &error);
        Source_Free(src);
        Error_Free(&error);
        return 0;
    }

    _Bool r = interpret(exe);

    Executable_Free(exe);
    Source_Free(src);
    Error_Free(&error);
    return r;
}

static _Bool disassemble_file(const char *file)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromFile(file, &error);

    if(src == NULL)
    {
        assert(error.occurred == 1);
        print_error(NULL, &error);
        Error_Free(&error);
        return 0;
    }

    _Bool r = disassemble(src);

    Source_Free(src);
    return r;
}

static _Bool disassemble_code(const char *code)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromString(NULL, code, -1, &error);

    if(src == NULL)
    {
        assert(error.occurred);
        print_error(NULL, &error);
        Error_Free(&error);
        return 0;
    }

    _Bool r = disassemble(src);

    Source_Free(src);
    return r;
}

_Bool NOJA_runString(const char *str)
{
    return interpret_code(str);
}

_Bool NOJA_runFile(const char *file)
{
    return interpret_file(file);
}

_Bool NOJA_dumpFileBytecode(const char *file)
{
    return disassemble_file(file);
}

_Bool NOJA_dumpStringBytecode(const char *str)
{
    return disassemble_code(str);
}

_Bool NOJA_runAssemblyFile(const char *file)
{
    return interpret_asm_file(file);
}

_Bool NOJA_runAssemblyString(const char *str)
{
    return interpret_asm_code(str);
}