#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include "../lib/utils/source.h"
#include "../lib/compiler/compile.h"
#include "../lib/common/executable.h"
#include "../lib/assembler/assemble.h"

//Regular text
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define BWHT "\e[1;37m"

//Regular underline text
#define UBLK "\e[4;30m"
#define URED "\e[4;31m"
#define UGRN "\e[4;32m"
#define UYEL "\e[4;33m"
#define UBLU "\e[4;34m"
#define UMAG "\e[4;35m"
#define UCYN "\e[4;36m"
#define UWHT "\e[4;37m"

//Regular background
#define BLKB "\e[40m"
#define REDB "\e[41m"
#define GRNB "\e[42m"
#define YELB "\e[43m"
#define BLUB "\e[44m"
#define MAGB "\e[45m"
#define CYNB "\e[46m"
#define WHTB "\e[47m"

//High intensty background 
#define BLKHB "\e[0;100m"
#define REDHB "\e[0;101m"
#define GRNHB "\e[0;102m"
#define YELHB "\e[0;103m"
#define BLUHB "\e[0;104m"
#define MAGHB "\e[0;105m"
#define CYNHB "\e[0;106m"
#define WHTHB "\e[0;107m"

//High intensty text
#define HBLK "\e[0;90m"
#define HRED "\e[0;91m"
#define HGRN "\e[0;92m"
#define HYEL "\e[0;93m"
#define HBLU "\e[0;94m"
#define HMAG "\e[0;95m"
#define HCYN "\e[0;96m"
#define HWHT "\e[0;97m"

//Bold high intensity text
#define BHBLK "\e[1;90m"
#define BHRED "\e[1;91m"
#define BHGRN "\e[1;92m"
#define BHYEL "\e[1;93m"
#define BHBLU "\e[1;94m"
#define BHMAG "\e[1;95m"
#define BHCYN "\e[1;96m"
#define BHWHT "\e[1;97m"

//Reset
#define reset "\e[0m"
#define CRESET "\e[0m"
#define COLOR_RESET "\e[0m"

// NOTE: From https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a

typedef enum {
    TokenType_END,
    TokenType_TEXT,
    TokenType_DIRECTIVE_SOURCE,
    TokenType_DIRECTIVE_BYTECODE,
} TokenType;

typedef struct {
    TokenType type;
    size_t  offset;
    size_t  length;
} Token;

static bool startsWithSourceDirective(const char *str, size_t len)
{
    return (((7 < len && !isalpha(str[7])) || 7 == len)
        && str[0] == '#' && str[1] == 's'
        && str[2] == 'o' && str[3] == 'u'
        && str[4] == 'r' && str[5] == 'c'
        && str[6] == 'e');
}

static bool startsWithBytecodeDirective(const char *str, size_t len)
{
    return (((9 < len && !isalpha(str[9])) || 9 == len)
        && str[0] == '#' && str[1] == 'b'
        && str[2] == 'y' && str[3] == 't'
        && str[4] == 'e' && str[5] == 'c'
        && str[6] == 'o' && str[7] == 'd'
        && str[8] == 'e');
}

static bool startsWithDirective(const char *str, size_t len)
{
    return startsWithSourceDirective(str, len)
        || startsWithBytecodeDirective(str, len);
}

static Token nextToken(const char *str, size_t len, size_t *cur_)
{
    size_t cur = *cur_;

    while(cur < len && isspace(str[cur]))
        cur += 1;

    Token token;
    if(cur == len) {
        token.type = TokenType_END;
        token.offset = cur;
        token.length = 0;
    } else if(startsWithSourceDirective(str + cur, len - cur)) {
        token.type = TokenType_DIRECTIVE_SOURCE;
        token.offset = cur;
        cur += strlen("#source");
        token.length = cur - token.offset;
    } else if(startsWithBytecodeDirective(str + cur, len - cur)) {
        token.type = TokenType_DIRECTIVE_BYTECODE;
        token.offset = cur;
        cur += strlen("#bytecode");
        token.length = cur - token.offset;
    } else {
        token.type = TokenType_TEXT;
        token.offset = cur;

        while(1) {
            // Consume all characters until
            // the next '#'.
            while(cur < len && str[cur] != '#')
                cur += 1;

            if(cur == len)
                break;

            // If it's the start of a directive,
            // stop the loop. If it was not, skip
            // the '#'.

            if(startsWithDirective(str + cur, len - cur))
                break;

            cur += 1; // Skip the '#'
        }

        token.length = cur - token.offset;
    }
    *cur_ = cur;
    return token;
}

typedef struct {
    Source *source;
    Source *bytecode;
} TestCase;

static bool parseTestCaseSource(Source *source, TestCase *testcase)
{   
    const char *str = Source_GetBody(source);
    size_t      len = Source_GetSize(source);
    assert(str != NULL);

    bool no_source = true;
    size_t source_offset;
    size_t source_length;

    bool no_bytecode = true;
    size_t bytecode_offset;
    size_t bytecode_length;

    size_t cur = 0;
    while(1) {
        Token token = nextToken(str, len, &cur);

        if(token.type == TokenType_END)
            break;

        if(token.type == TokenType_TEXT) {
            fprintf(stderr, RED "Error" CRESET " :: Expected a directive at offset %ld\n", token.offset);
            return false;
        }

        assert(token.type == TokenType_DIRECTIVE_SOURCE
            || token.type == TokenType_DIRECTIVE_BYTECODE);

        Token directive = token;

        size_t directive_body_offset;
        size_t directive_body_length;

        size_t backup = cur;
        token = nextToken(str, len, &cur);
        switch(token.type) {
            case TokenType_END:
            case TokenType_DIRECTIVE_SOURCE:
            case TokenType_DIRECTIVE_BYTECODE:
            // Right after the previous directive
            // there's either the end of the file
            // or another directive. We assume 
            // that the directive is followed by
            // and empty string.
            directive_body_offset = token.offset;
            directive_body_length = 0;

            // Put the cursor back so that this
            // token is tokenized again at the
            // start of the next iteration.
            cur = backup;
            break;

            case TokenType_TEXT:
            directive_body_offset = token.offset;
            directive_body_length = token.length;
            break;
        }

        if(directive.type == TokenType_DIRECTIVE_SOURCE) {
            
            if(no_source == false) {
                fprintf(stderr, RED "Error" CRESET " :: #source directive specified twice.\n");
                return false;
            }

            no_source = false;
            source_offset = directive_body_offset;
            source_length = directive_body_length;

        } else {
            assert(directive.type == TokenType_DIRECTIVE_BYTECODE);
            
            if(no_bytecode == false) {
                fprintf(stderr, RED "Error" CRESET " :: #bytecode directive specified twice.\n");
                return false;
            }

            no_bytecode = false;
            bytecode_offset = directive_body_offset;
            bytecode_length = directive_body_length;
        }
    }

    if(no_source == true && no_bytecode == true) {
        fprintf(stderr, RED "Error" CRESET " :: Missing both #source and #bytecode directives.\n");
        return false;
    }
    if(no_source == true) {
        fprintf(stderr, RED "Error" CRESET " :: Missing #source directive.\n");
        return false;
    }
    if(no_bytecode == true) {
        fprintf(stderr, RED "Error" CRESET " :: Missing #bytecode directive.\n");
        return false;
    }

    {
        Error error;
        Error_Init(&error);

        testcase->source = Source_FromString("<input>", str + source_offset, source_length, &error);
        if(testcase->source == NULL) {
            Error_Print(&error, ErrorType_UNSPECIFIED);
            Error_Free(&error);
            return false;
        }

        testcase->bytecode = Source_FromString("<output>", str + bytecode_offset, bytecode_length, &error);
        if(testcase->bytecode == NULL) {
            Error_Print(&error, ErrorType_UNSPECIFIED);
            Source_Free(testcase->source);
            Error_Free(&error);
            return false;
        }
        Error_Free(&error);
    }
    return true;
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

typedef enum {
    TestResult_PASSED,
    TestResult_FAILED,
    TestResult_ABORT,
} TestResult;

static TestResult runTest(const char *file)
{
    Source *source;
    {
        Error error;
        Error_Init(&error);

        source = Source_FromFile(file, &error);
        if(source == NULL) {
            Error_Print(&error, ErrorType_UNSPECIFIED);
            Error_Free(&error);
            return TestResult_ABORT;
        }
        Error_Free(&error);
    }

    TestCase testcase;
    {
        if(!parseTestCaseSource(source, &testcase)) {
            Source_Free(source);
            return TestResult_ABORT;
        }
    }

    Source_Free(source);

    Executable *exe1 = compile_source_and_print_error_on_failure(testcase.source);
    if(exe1 == NULL) {
        Source_Free(testcase.source);
        Source_Free(testcase.bytecode);
        return TestResult_ABORT;
    }
    
    Executable *exe2;
    {
        Error error;
        Error_Init(&error);

        exe2 = assemble(testcase.bytecode, &error);
        if(exe2 == NULL) {
            Error_Print(&error, ErrorType_UNSPECIFIED);
            Error_Free(&error);
            Executable_Free(exe1);
            Source_Free(testcase.source);
            Source_Free(testcase.bytecode);
            return TestResult_ABORT;
        }

        Error_Free(&error);
    }

    bool passed = Executable_Equiv(exe1, exe2, stderr, "  " WHT "Log" CRESET " :: ");

    Executable_Free(exe1);
    Executable_Free(exe2);
    Source_Free(testcase.source);
    Source_Free(testcase.bytecode);

    return passed 
        ? TestResult_PASSED
        : TestResult_FAILED;
}

static void runTestSuite(const char *folder)
{
    DIR *handle = opendir(folder);
    if(handle == NULL) {
        fprintf(stderr, RED "Error" CRESET " :: Failed to open folder '%s'.\n", folder);
        return;
    }

    size_t total_tests = 0;
    size_t passed_tests = 0;
    size_t failed_tests = 0;
    size_t aborted_tests = 0;
    size_t skipped_files = 0;

    size_t folder_len = strlen(folder);

    // If the folder path ends with a "/",
    // but it's not the first character,
    // then pop it.
    if(folder_len > 0 && folder[folder_len-1] == '/')
        folder_len -= 1;

    // Now we know for sure that the provided
    // folder needs a trailing "/".

    char test_path[1024];

    // Make sure the buffer can contain the
    // folder's name (plus the trailing "/"
    // and the null byte).
    if(folder_len+2 > sizeof(test_path)) {
        fprintf(stderr, RED "Error" CRESET " :: Suite folder name is too long\n");
        return;
    }
    memcpy(test_path, folder, folder_len);
    test_path[folder_len] = '/';
    test_path[folder_len+1] = '\0';

    // Now count the "/".
    folder_len += 1;

    struct dirent *dir;
    while((dir = readdir(handle)) != NULL) {
            
        const char *name = dir->d_name;
        size_t name_len = strlen(name);

        if(dir->d_type != DT_REG) {
            fprintf(stderr, "  " WHT "Log" CRESET " :: Skipping '%s' (not a regular file).\n", name);
            skipped_files += 1;
            continue;
        }

        if(folder_len + name_len >= sizeof(test_path)) {
            fprintf(stderr, "  " WHT "Log" CRESET " :: Skipping '%s' (path is too long).\n", name);
            skipped_files += 1;
            continue;
        }

        // Remove the previous file's name
        // by truncating the parent folder
        // name with a null byte.
        test_path[folder_len] = '\0';
        strcat(test_path, name);

        TestResult res = runTest(test_path);
        switch(res) {

            case TestResult_PASSED:
            fprintf(stdout, " " BLU "Test" CRESET " :: Passed (%s)\n", name);
            passed_tests += 1;
            break;
            
            case TestResult_FAILED:
            fprintf(stdout, " " YEL "Test" CRESET " :: Failed (%s)\n", name);
            failed_tests += 1;
            break;

            case TestResult_ABORT:
            fprintf(stdout, " " RED "Test" CRESET " :: Aborted (%s)\n", name);
            aborted_tests += 1;
            break;
        }
        total_tests += 1;
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "@-----SUMMARY-----@\n");
    fprintf(stdout, "| Total   : %-3ld   |\n", total_tests);
    fprintf(stdout, "| Passed  : %-3ld   |\n", passed_tests);
    fprintf(stdout, "| Failed  : %-3ld   |\n", failed_tests);
    fprintf(stdout, "| Aborted : %-3ld   |\n", aborted_tests);
    fprintf(stdout, "| Skipped : %-3ld   |\n", skipped_files);
    fprintf(stdout, "@-----------------@\n");

    closedir(handle);
}

int main(int argc, char **argv)
{
    if(argc < 2) {
        fprintf(stderr, RED "Error" CRESET " :: No test suite folder name was provided.\n");
        fprintf(stderr, WHT "Usage" CRESET " :: %s <path>\n", argv[0]);
        return -1;
    }

    const char *folder = argv[1];
    runTestSuite(folder);
    return 0;
}