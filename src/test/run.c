#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include "run.h"
#include "test.h"

static const TestType *findTestType(const TestType *test_types, const char *name)
{
    for (size_t i = 0; test_types[i].name != NULL; i++)
        if (!strcmp(name, test_types[i].name))
            return test_types + i;
    return NULL;
}

TestResult runTestString(const TestType *test_types, const char *src, FILE *log_stream)
{
    NojaTest test;
    ErrorID error = parseTest(src, &test);
    if (error != ErrorID_VOID) {
        fprintf(log_stream, "Error: %s\n", getErrorString(error));
        return TestResult_ABORTED;
    }

    const char *type = queryTest(&test, "type");
    if (type == NULL) {
        fprintf(log_stream, "Error: Test is missing the \"type\" field\n");
        return TestResult_ABORTED;
    }

    const TestType *info = findTestType(test_types, type);
    if (info == NULL) {
        fprintf(log_stream, "Error: Invalid test type \"%s\"\n", type);
        return TestResult_ABORTED;
    }

    bool uses_type = false;
    const char *ordered_fields[8];

    size_t i = 0;
    while (info->fields[i] != NULL) {

        uses_type = uses_type || !strcmp(info->fields[i], "type");

        const char *field = queryTest(&test, info->fields[i]);
        if (field == NULL) {
            fprintf(log_stream, "Error: Test is missing the \"%s\" field, necessary for \"%s\" tests\n", info->fields[i], info->name);
            return TestResult_ABORTED;
        }

        if (i == sizeof(ordered_fields)/sizeof(ordered_fields[0]))
            abort();

        ordered_fields[i] = field;
        i++;
    }
    size_t used_fields = i;

    size_t unused_fields = test.fields_count - used_fields;
    if (!uses_type) unused_fields--;

    if (unused_fields > 0)
        fprintf(log_stream, "Warning: %ld field of the test weren't used\n", unused_fields);

    TestResult result = info->routine(ordered_fields, log_stream);

    freeTest(&test);
    return result;
}

static char *getStreamData(FILE *file_stream, FILE *log_stream)
{
    size_t copied = 0, just_copied;
    size_t capacity = 512;
    char *content = NULL;
    do {
    
        capacity *= 2;
    
        content = realloc(content, capacity);
        if (content == NULL) {
            fprintf(log_stream, "Error: Out of memory\n");
            return NULL;
        }

        just_copied = fread(content + copied, 1, capacity - copied, file_stream);
        copied += just_copied;
        
    } while (just_copied > 0);

    if (ferror(file_stream)) {
        free(content);
        fprintf(log_stream, "Error: Couldn't read file contents\n");
        return NULL;
    }

    if (copied+1 >= capacity) {
        content = realloc(content, capacity+1);
        if (content == NULL) {
            fprintf(log_stream, "Error: Out of memory");
            return NULL;
        }
    }
    content[copied] = '\0';

    return content;
}

TestResult runTestFile(const TestType *test_types, const char *file, FILE *log_stream)
{
    FILE *file_stream = fopen(file, "rb");
    if (file_stream == NULL) {
        fprintf(log_stream, "Error: Couldn't open file \"%s\" (%s)\n", file, strerror(errno));
        return TestResult_ABORTED;
    }

    char *data = getStreamData(file_stream, log_stream);
    if (data == NULL) {
        fclose(file_stream);
        return TestResult_ABORTED;
    }

    TestResult result = runTestString(test_types, data, log_stream);

    free(data);
    fclose(file_stream);
    return result;
}

static const char *getExtension(const char *file)
{
    int dot = -1;
    for (int i = 0; file[i] != '\0'; i++)
        if (file[i] == '.')
            dot = i;

    return (dot == -1) ? "" : file + dot;
}

TestBatchResults runTestDirectory(const TestType *test_types, const char *dir_name, FILE *log_stream)
{
    TestBatchResults results = {.passed=0, .failed=0, .aborted=0, .batch_aborted=false};

    DIR *d = opendir(dir_name);
    if (d == NULL) {
        results.batch_aborted = true;
        return results;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {

        if (ent->d_name[0] == '.') {
            //fprintf(log_stream, "Info: Ignoring file \"%s\"\n", ent->d_name);
            continue;
        }

        char full[1024];
        strcpy(full, dir_name);
        if (dir_name[strlen(dir_name)-1] != '/')
            strcat(full, "/");
        strcat(full, ent->d_name);

        if (ent->d_type == DT_DIR) {
            TestBatchResults sub_results = runTestDirectory(test_types, full, log_stream);
            if (!sub_results.batch_aborted) {
                results.passed += sub_results.passed;
                results.failed += sub_results.failed;
                results.aborted += sub_results.aborted;
            }
            continue;
        }

        if (ent->d_type == DT_UNKNOWN) {
            abort();
        }

        if (ent->d_type != DT_REG) {
            //fprintf(log_stream, "Info: Ignoring file \"%s\"\n", ent->d_name);
            continue;
        }

        const char *ext = getExtension(ent->d_name);
        if (strcmp(ext, ".noja-test")) {
            //fprintf(log_stream, "Info: Ignoring file \"%s\"\n", ent->d_name);
            continue;
        }

        TestResult result = runTestFile(test_types, full, log_stream);
        switch (result) {
            case TestResult_PASSED:
            fprintf(log_stream, "Test %s .. PASSED\n", full);
            results.passed++;
            break;
            
            case TestResult_FAILED:
            fprintf(log_stream, "Test %s .. FAILED\n", full);
            results.failed++;
            break;
            
            case TestResult_ABORTED:
            fprintf(log_stream, "Test %s .. ABORTED\n", full);
            results.aborted++;
            break;
        }
    }

    closedir(d);
    return results;
}
