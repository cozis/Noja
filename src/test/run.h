#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include "main.h"

typedef struct {
    const char *name;
    const char **fields;
    TestResult (*routine)(const char **fields, FILE *log_stream);
} TestType;

typedef struct {
    size_t passed;
    size_t failed;
    size_t aborted;
    bool batch_aborted;
} TestBatchResults;

TestResult       runTestFile(const TestType *test_types, const char *file, FILE *log_stream);
TestResult       runTestString(const TestType *test_types, const char *src, FILE *log_stream);
TestBatchResults runTestDirectory(const TestType *test_types, const char *dir_name, FILE *log_stream);
