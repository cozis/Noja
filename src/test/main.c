#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "main.h"
#include "run.h"
#include "../lib/run.h"
#include "../lib/runtime.h"
#include "../lib/executable.h"
#include "../lib/compiler/compile.h"
#include "../lib/assembler/assemble.h"

static TestResult runCompilerTest(const char *inputs[static 2], FILE *log_stream);    
static TestResult  runRuntimeTest(const char *inputs[static 2], FILE *log_stream);

static const TestType test_types[] = {
    {.name="compiler", .routine=runCompilerTest, .fields=(const char*[]){"source", "bytecode", NULL}},
    {.name="runtime",  .routine=runRuntimeTest,  .fields=(const char*[]){"bytecode", "output", NULL}},
    {.name=NULL, .fields=NULL, .routine=NULL},
};

int main(int argc, char **argv)
{
    TestBatchResults results = {.passed=0, .failed=0, .aborted=0, .batch_aborted=false};
    for (int i = 1; i < argc; i++) {
        
        const char *dir_name = argv[i];

        TestBatchResults temp = runTestDirectory(test_types, dir_name, stderr);
        if (temp.batch_aborted)
            fprintf(stderr, "Error: Failed to run test batch\n");
        else {
            results.passed += temp.passed;
            results.failed += temp.failed;
            results.aborted += temp.aborted;
        }
    }
    fprintf(stderr, "SUMMARY: %ld passed, %ld failed, %ld aborted\n", 
            results.passed, results.failed, results.aborted);
    return 0;
}

static TestResult runCompilerTest(const char *inputs[static 2], FILE *log_stream)
{
    Error error;
    Error_Init(&error);

    Source *srcA = Source_FromString("<test>", inputs[0], -1, &error);
    if (srcA == NULL) {
        Error_Print(&error, ErrorType_UNSPECIFIED, log_stream);
        Error_Free(&error);
        return TestResult_ABORTED;
    }
    Source *srcB = Source_FromString("<test>", inputs[1], -1, &error);
    if (srcA == NULL) {
        Error_Print(&error, ErrorType_UNSPECIFIED, log_stream);
        Error_Free(&error);
        Source_Free(srcA);
        return TestResult_ABORTED;
    }

    int error_offset;
    Executable *exeA = compile(srcA, &error, &error_offset);
    if (exeA == NULL) {
        Error_Print(&error, ErrorType_UNSPECIFIED, log_stream);
        Error_Free(&error);
        Source_Free(srcA);
        Source_Free(srcB);
        return TestResult_ABORTED;
    }

    Executable *exeB = assemble(srcB, &error, &error_offset);
    if (exeB == NULL) {
        Error_Print(&error, ErrorType_UNSPECIFIED, log_stream);
        Error_Free(&error);
        Source_Free(srcA);
        Source_Free(srcB);
        Executable_Free(exeA);
        return TestResult_ABORTED;
    }

    bool passed = Executable_Equiv(exeA, exeB, log_stream, "Info: ");

    Source_Free(srcA);
    Source_Free(srcB);
    Executable_Free(exeA);
    Executable_Free(exeB);
    return passed ? TestResult_PASSED : TestResult_FAILED;
}

static TestResult runRuntimeTest(const char *inputs[static 2], FILE *log_stream)
{
    char buffer[1024];
    
    FILE *output_stream = fmemopen(buffer, sizeof(buffer), "wb");
    if (output_stream == NULL)
        return TestResult_ABORTED;

    RuntimeConfig config = Runtime_GetDefaultConfigs();
    config.stdout = output_stream;

    Runtime *runtime = Runtime_New(config);
    if (runtime == NULL) {
        fclose(output_stream);
        return TestResult_ABORTED;
    }

    Error error;
    Error_Init(&error);

    if (!Runtime_plugDefaultBuiltins(runtime, &error)) {
        Error_Print(&error, ErrorType_UNSPECIFIED, log_stream);
        Error_Free(&error);
        Runtime_PrintStackTrace(runtime, log_stream);
        Runtime_Free(runtime);
        fclose(output_stream);
        return TestResult_ABORTED;
    }

    if (!runBytecodeString(runtime, inputs[0], &error)) {
        Error_Print(&error, ErrorType_UNSPECIFIED, log_stream);
        Error_Free(&error);
        Runtime_PrintStackTrace(runtime, log_stream);
        Runtime_Free(runtime);
        fclose(output_stream);
        return TestResult_ABORTED;
    }
    fclose(output_stream);

    TestResult result;
    if (!strcmp(buffer, inputs[1])) // No bounds checks
        result = TestResult_PASSED;
    else {
        result = TestResult_FAILED;
        fprintf(stderr, "Error: Expected output [%s] but got [%s]\n", inputs[1], buffer);
    }
    
    Error_Free(&error);
    Runtime_Free(runtime);
    return result;
}