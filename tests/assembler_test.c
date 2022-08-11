#include <stdlib.h>
#include <stdbool.h>
#include "../src/noja/utils/bpalloc.h"
#include "../src/noja/assembler/assembler.h"

static void stopTesting_(const char *file, int line)
{
    fprintf(stderr, "\n%s:%d :: Failed to set up test environment\n", file, line);
    abort();
}
#define stopTestingIf(exp)                    \
    do {                                      \
        if(exp) {                             \
            stopTesting_(__FILE__, __LINE__); \
        }                                     \
    } while(0);

static void testCase(int line, bool exp, const char *msg)
{
    if(exp == false)
        fprintf(stdout, "Test case failed (line %d) :: %s\n", line, msg);
    else
        fprintf(stdout, "Test case passed (line %d)\n", line);
}

typedef struct {
    Opcode opcode;
    int operand_count;
    Operand *operands;
} StaticInstruction;

typedef struct {
    int instr_count;
    StaticInstruction *instr_list;
} StaticExecutable;

static Executable *buildExpectedExecutable(BPAlloc *alloc, StaticExecutable *static_exp_exe)
{
    ExeBuilder *builder = ExeBuilder_New(alloc);
    stopTestingIf(builder == NULL);

    Error error;
    Error_Init(&error);
    for(int i = 0; i < static_exp_exe->instr_count; i += 1) {
        StaticInstruction instr = static_exp_exe->instr_list[i];
        bool ok = ExeBuilder_Append(builder, &error, instr.opcode, instr.operands, instr.operand_count, -1, -1);
        stopTestingIf(ok == false);
    }

    Executable *exe = ExeBuilder_Finalize(builder, &error);
    stopTestingIf(exe == NULL);

    Error_Free(&error);
    return exe;
}

int main()
{
    struct {
        int line;
        bool fail;
        const char *text;
        StaticExecutable expected;
    } tests[] = {

        // Test the assembly of an empty string
        // into an executable with no instructions.
        {
            .line = __LINE__,
            .text = "",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 0,
                .instr_list = NULL,
            },
        },

        // Test the assembly of the simplest
        // executable possible
        {
            .line = __LINE__,
            .text = "POS",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 1,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_POS,
                        .operands = NULL,
                        .operand_count = 0,
                    }
                },
            },
        },

        // Invalid opcode
        {
            .line = __LINE__,
            .text = "GUCCI",
            .fail = true,
        },

        // Label but no opcode
        {
            .line = __LINE__,
            .text = "label:",
            .fail = true,
        },

        // Basic executable with a label
        {
            .line = __LINE__,
            .text = "label: POS",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 1,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_POS,
                        .operands = NULL,
                        .operand_count = 0,
                    }
                },
            },
        },

        {
            .line = __LINE__,
            .text = "  label  :  POS  ",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 1,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_POS,
                        .operands = NULL,
                        .operand_count = 0,
                    }
                },
            },
        },

        {
            .line = __LINE__,
            .text = "  label  :  POS  ;  ",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 1,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_POS,
                        .operands = NULL,
                        .operand_count = 0,
                    }
                },
            },
        },

        {
            .line = __LINE__,
            .text = "  label0  :  POS  ;  "
                    "  label1  :  NEG  ;  ",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 2,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_POS,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                    {
                        .opcode = OPCODE_NEG,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                },
            },
        },

        {
            .line = __LINE__,
            .text = "  label0  :  POS  ;  "
                    "  label1  :  NEG  ",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 2,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_POS,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                    {
                        .opcode = OPCODE_NEG,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                },
            },
        },

        {
            .line = __LINE__,
            .text = "             POS  ;  "
                    "  label1  :  NEG  ",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 2,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_POS,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                    {
                        .opcode = OPCODE_NEG,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                },
            },
        },

        {
            .line = __LINE__,
            .text = "  label0  :  POS  ;  "
                    "             NEG  ",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 2,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_POS,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                    {
                        .opcode = OPCODE_NEG,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                },
            },
        },

        // Instruction with the wrong number of arguments.
        {
            .line = __LINE__,
            .text = "POS  1;",
            .fail = true,
        },

        {
            .line = __LINE__,
            .text = "PUSHINT 1;"
                    "PUSHINT 2;"
                    "ADD;",
            .fail = false,
            .expected = (StaticExecutable) {
                .instr_count = 3,
                .instr_list = (StaticInstruction[]) {
                    {
                        .opcode = OPCODE_PUSHINT,
                        .operands = (Operand[]) {
                            { OPTP_INT, .as_int = 1, }
                        },
                        .operand_count = 1,
                    },
                    {
                        .opcode = OPCODE_PUSHINT,
                        .operands = (Operand[]) {
                            { OPTP_INT, .as_int = 2, }
                        },
                        .operand_count = 1,
                    },
                    {
                        .opcode = OPCODE_ADD,
                        .operands = NULL,
                        .operand_count = 0,
                    },
                },
            },
        },
    };

    Error error;
    char pool[4096];

    for(size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i += 1) {

        // Use an allocator backed by a static
        // buffer to avoid reallocating a pool
        // for each test.
        BPAlloc *alloc = BPAlloc_Init3(pool, sizeof(pool), -1, NULL, NULL, NULL);
        stopTestingIf(alloc == NULL);

        Error_Init(&error);

        Source *src = Source_FromString(NULL, tests[i].text, -1, &error);
        stopTestingIf(src == NULL);

        Executable *got_exe = assemble(src, &error);

        if(tests[i].fail == true) {
            testCase(tests[i].line, got_exe == NULL, "Managed to assemble an invalid source");
        } else {
            Executable *exp_exe = buildExpectedExecutable(alloc, &tests[i].expected);

            if(got_exe == NULL)
                fprintf(stderr, "Build error :: %s.\n", error.message);

            testCase(tests[i].line, got_exe != NULL, "Failed to assemble a valid source");
/*          
            fprintf(stderr, "== Generated ==\n");
            Executable_Dump(got_exe);
            fprintf(stderr, "\n");
            fprintf(stderr, "== Expected ==\n");
            Executable_Dump(exp_exe);
*/
            testCase(tests[i].line, Executable_Equiv(got_exe, exp_exe, stderr, "Executable diff :: "), "Invalid assemblation result");

            Executable_Free(got_exe);
            Executable_Free(exp_exe);
        }
        Source_Free(src);
        Error_Free(&error);
        BPAlloc_Free(alloc);
    }
    return 0;
}