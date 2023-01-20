#include <stdio.h>
#include <assert.h>
#include "utils/error.h"
#include "utils/source.h"
#include "compiler/compile.h"
#include "assembler/assemble.h"
#include "builtins/basic.h"
#include "runtime/timing.h"
#include "noja.h"

static void serializeProfilingResults(Runtime *runtime, const char *file)
{
    TimingTable *table = Runtime_GetTimingTable(runtime);
    if (table == NULL)
        return;

    FILE *stream = fopen(file, "wb");
    if (stream == NULL) {
        fprintf(stderr, "Failed to serialize profiling results\n");
        return;
    }

    const FunctionExecutionSummary *summary;
    size_t count;
            
    summary = TimingTable_getSummary(table, &count);
    assert(summary != NULL);
            
    for (size_t i = 0; i < count; i++) {
        if (summary[i].calls > 0) {
            fprintf(stream, "%20s - %s - %ld calls - %.2lfus\n",
                   summary[i].name,
                   Source_GetName(summary[i].src),
                   summary[i].calls,
                   summary[i].time * 1000000);
        }
    }
            
    fclose(stream);
    fprintf(stderr, "Wrote profiling result to %s\n", file);
}


#include <signal.h>

Runtime *runtime = NULL;

static void signalHandler(int signo)
{
    (void) signo;
    if (runtime != NULL)
       Runtime_Interrupt(runtime);
}

static _Bool interpret(Executable *exe, bool time, size_t heap)
{
    RuntimeConfig config = Runtime_GetDefaultConfigs();
    config.time = time;

    runtime = Runtime_New(heap, config);
    if(runtime == NULL)
    {
        Error error;
        Error_Init(&error);
        Error_Report(&error, ErrorType_INTERNAL, "Couldn't initialize runtime");
        Error_Print(&error, ErrorType_UNSPECIFIED);
        Error_Free(&error);
        return 0;
    }
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    // We use a [RuntimeError] instead of a simple [Error]
    // because the [RuntimeError] makes a snapshot of the
    // runtime state when an error is reported. Other than
    // this fact they are interchangable. Any function that
    // expects a pointer to [Error] can receive a [RuntimeError]
    // upcasted to [Error].
    RuntimeError error;
    RuntimeError_Init(&error, runtime); // Here we specify the runtime to snapshot in case of failure.
    
    {
        Object *native_bins = Object_NewStaticMap(bins_basic, bins_basic_init, runtime, (Error*) &error);
        if(native_bins == NULL)
        {
            assert(error.base.occurred == 1);
            Error_Print((Error*) &error, ErrorType_RUNTIME);
            RuntimeError_Free(&error);
            Runtime_Free(runtime);
            return 0;
        }

        // Just to execute the prelude
        Runtime_SetBuiltins(runtime, native_bins);

        extern char start_noja[];
        Source *prelude = Source_FromString("<prelude>", start_noja, -1, (Error*) &error);
        if (prelude == NULL) {
            assert(error.base.occurred == 1);
            Error_Print((Error*) &error, ErrorType_RUNTIME);
            RuntimeError_Free(&error);
            Runtime_Free(runtime);
            return 0;
        }

        Executable *prelude_exe = compile(prelude, (Error*) &error);
        if(prelude_exe == NULL) {
            Error_Print((Error*) &error, ErrorType_RUNTIME);
            RuntimeError_Free(&error);
            Runtime_Free(runtime);
            Source_Free(prelude);
            return 0;
        }

        Object *rets[8];
        int retc = run(runtime, (Error*) &error, prelude_exe, 0, NULL, NULL, 0, rets);
        if(retc < 0) {
            Error_Print((Error*) &error, ErrorType_RUNTIME);
            RuntimeError_Free(&error);
            Runtime_Free(runtime);
            Source_Free(prelude);
            Executable_Free(prelude_exe);
            return 0;
        }
        Object *noja_bins = rets[0];

        // Need to remake the native built-ins because
        // running the script invalidated the previous 
        // pointer.
        native_bins = Object_NewStaticMap(bins_basic, bins_basic_init, runtime, (Error*) &error);
        if(native_bins == NULL)
        {
            assert(error.base.occurred == 1);
            Error_Print((Error*) &error, ErrorType_RUNTIME);
            RuntimeError_Free(&error);
            Runtime_Free(runtime);
            Source_Free(prelude);
            Executable_Free(prelude_exe);
            return 0;
        }

        Object *all_bins = Object_NewClosure(native_bins, noja_bins, Runtime_GetHeap(runtime), (Error*) &error);
        if (all_bins == NULL) {
            Error_Print((Error*) &error, ErrorType_RUNTIME);
            RuntimeError_Free(&error);
            Runtime_Free(runtime);
            Source_Free(prelude);
            Executable_Free(prelude_exe);
            return 0;
        }

        Runtime_SetBuiltins(runtime, all_bins);

        Source_Free(prelude);
        Executable_Free(prelude_exe);
    }

    Object *rets[8];
    int retc = run(runtime, (Error*) &error, exe, 0, NULL, NULL, 0, rets);

    // NOTE: The pointer to the builtins object is invalidated
    //       now because it may be moved by the garbage collector.

    if(retc < 0)
    {
        Error_Print((Error*) &error, ErrorType_RUNTIME);

        if(error.snapshot == NULL)
            fprintf(stderr, "No snapshot available.\n");
        else
            Snapshot_Print(error.snapshot, stderr);

        RuntimeError_Free(&error);
    }

    serializeProfilingResults(runtime, "profiling-results.txt");

    Runtime_Free(runtime);
    return retc > -1;
}

static Executable *compile_source_and_print_error_on_failure(Source *src)
{
    Error error;
    Error_Init(&error);
    Executable *exe = compile(src, &error);
    if(exe == NULL) {
        Error_Print(&error, ErrorType_UNSPECIFIED);
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
    Executable_Dump(exe, stdout);
    Executable_Free(exe);
    return 1;
}

static _Bool interpret_file(const char *file, bool time, size_t heap)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromFile(file, &error);
    if(src == NULL)
    {
        assert(error.occurred == 1);
        Error_Print(&error, ErrorType_UNSPECIFIED);
        Error_Free(&error);
        return 0;
    }

    Executable *exe = compile_source_and_print_error_on_failure(src);
    if(exe == NULL) {
        Source_Free(src);
        return 0;
    }

    _Bool r = interpret(exe, time, heap);

    Executable_Free(exe);
    Source_Free(src);
    return r;
}

static _Bool interpret_code(const char *code, bool time, size_t heap)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromString(NULL, code, -1, &error);

    if(src == NULL)
    {
        assert(error.occurred);
        Error_Print(&error, ErrorType_UNSPECIFIED);
        Error_Free(&error);
        return 0;
    }

    Executable *exe = compile_source_and_print_error_on_failure(src);
    if(exe == NULL) {
        Source_Free(src);
        return 0;
    }

    _Bool r = interpret(exe, time, heap);

    Executable_Free(exe);
    Source_Free(src);
    return r;
}

static _Bool interpret_asm_file(const char *file, bool time, size_t heap)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromFile(file, &error);

    if(src == NULL)
    {
        assert(error.occurred == 1);
        Error_Print(&error, ErrorType_UNSPECIFIED);
        Error_Free(&error);
        return 0;
    }
    
    Executable *exe = assemble(src, &error);
    if(exe == NULL) {
        Error_Print(&error, ErrorType_UNSPECIFIED);
        Source_Free(src);
        Error_Free(&error);
        return 0;
    }

    _Bool r = interpret(exe, time, heap);

    Executable_Free(exe);
    Source_Free(src);
    Error_Free(&error);
    return r;
}

static _Bool interpret_asm_code(const char *code, bool time, size_t heap)
{
    Error error;
    Error_Init(&error);
    
    Source *src = Source_FromString(NULL, code, -1, &error);

    if(src == NULL)
    {
        assert(error.occurred);
        Error_Print(&error, ErrorType_UNSPECIFIED);
        Error_Free(&error);
        return 0;
    }

    Executable *exe = assemble(src, &error);
    if(exe == NULL) {
        Error_Print(&error, ErrorType_UNSPECIFIED);
        Source_Free(src);
        Error_Free(&error);
        return 0;
    }

    _Bool r = interpret(exe, time, heap);

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
        Error_Print(&error, ErrorType_UNSPECIFIED);
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
        Error_Print(&error, ErrorType_UNSPECIFIED);
        Error_Free(&error);
        return 0;
    }

    _Bool r = disassemble(src);

    Source_Free(src);
    return r;
}

_Bool NOJA_runString(const char *str, size_t heap)
{
    return interpret_code(str, false, heap);
}

_Bool NOJA_runFile(const char *file, size_t heap)
{
    return interpret_file(file, false, heap);
}

_Bool NOJA_dumpFileBytecode(const char *file)
{
    return disassemble_file(file);
}

_Bool NOJA_dumpStringBytecode(const char *str)
{
    return disassemble_code(str);
}

_Bool NOJA_runAssemblyFile(const char *file, size_t heap)
{
    return interpret_asm_file(file, false, heap);
}

_Bool NOJA_runAssemblyString(const char *str, size_t heap)
{
    return interpret_asm_code(str, false, heap);
}

_Bool NOJA_profileString(const char *str, size_t heap)
{
    return interpret_code(str, true, heap);
}

_Bool NOJA_profileFile(const char *file, size_t heap)
{
    return interpret_file(file, true, heap);
}

_Bool NOJA_profileAssemblyFile(const char *file, size_t heap)
{
    return interpret_asm_file(file, true, heap);
}

_Bool NOJA_profileAssemblyString(const char *str, size_t heap)
{
    return interpret_asm_code(str, true, heap);
}
