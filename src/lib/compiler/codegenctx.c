#include <stdbool.h>
#include "../utils/defs.h"
#include "codegenctx.h"

struct CodegenContext {
    Error *error;
    BPAlloc *alloc;
    ExeBuilder *builder;
    bool own_alloc;
    bool env_set;
    jmp_buf *env;
    int *error_offset;
};

Label *Label_New(CodegenContext *ctx)
{
    Promise *promise = Promise_New(ctx->alloc, sizeof(long long int));
    if(promise != NULL)
        return (Label*) promise;
    
    CodegenContext_ReportErrorAndJump(ctx, -1, ErrorType_INTERNAL, "No memory");
    UNREACHABLE;
    return NULL; // For the compiler warning.
}

void Label_Set(Label *label, long long int value)
{
    Promise *promise = (Promise*) label;
    Promise_Resolve(promise, &value, sizeof(value));
}

void Label_SetHere(Label *label, CodegenContext *ctx)
{
    long long int value = ExeBuilder_InstrCount(ctx->builder);
    Label_Set(label, value);
}

void Label_Free(Label *label)
{
    Promise *promise = (Promise*) label;
    Promise_Free(promise);
}

Promise *Label_ToPromise(Label *label)
{
    Promise *promise = (Promise*) label;
    return promise;
}

static void okNowJump(CodegenContext *ctx)
{
    ASSERT(ctx->env_set == true);
    longjmp(*ctx->env, 1);
    UNREACHABLE;
}

void CodegenContext_ReportErrorAndJump_(CodegenContext *ctx, const char *file, 
                                        const char *func, int line, int error_offset,
                                        ErrorType type, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _Error_Report2(ctx->error, type, file, func, line, format, args);
    va_end(args);
    *ctx->error_offset = error_offset;
    okNowJump(ctx);
    UNREACHABLE;
}

void CodegenContext_SetJumpDest(CodegenContext *ctx, jmp_buf *env)
{
    ASSERT(ctx->env_set == false);
    ctx->env = env;
    ctx->env_set = true;
}

CodegenContext *CodegenContext_New(Error *error, BPAlloc *alloc)
{
    bool own_alloc;
    if(alloc == NULL) {
        alloc = BPAlloc_Init(-1);
        if(alloc == NULL) {
            Error_Report(error, ErrorType_INTERNAL, "No memory");
            return false;
        }
        own_alloc = true;
    } else {
        own_alloc = false;
    }

    CodegenContext *ctx = BPAlloc_Malloc(alloc, sizeof(CodegenContext));
    if(ctx == NULL) {
        if(own_alloc)
            BPAlloc_Free(alloc);
        Error_Report(error, ErrorType_INTERNAL, "No memory");
        return NULL;
    }

    ExeBuilder *builder = ExeBuilder_New(alloc);
    if(builder == NULL) {
        if(ctx->own_alloc)
            BPAlloc_Free(alloc);
        return NULL;
    }

    ctx->error = error;
    ctx->alloc = alloc;
    ctx->builder = builder;
    ctx->env_set = false;
    ctx->own_alloc = own_alloc;
    return ctx;
}

int CodegenContext_InstrCount(CodegenContext *ctx)
{
    return ExeBuilder_InstrCount(ctx->builder);
}


void CodegenContext_Free(CodegenContext *ctx)
{
    if(ctx->own_alloc)
        BPAlloc_Free(ctx->alloc);
}

Executable *CodegenContext_MakeExecutableAndFree(CodegenContext *ctx, Source *src)
{
    Executable *exe = ExeBuilder_Finalize(ctx->builder, ctx->error);
    if(exe == NULL) {
        okNowJump(ctx);
        UNREACHABLE;
    }

    if(src != NULL)
        Executable_SetSource(exe, src);
    
    CodegenContext_Free(ctx);
    return exe;
}

void CodegenContext_EmitInstr(CodegenContext *ctx, Opcode opcode, Operand *opv, int opc, int off, int len)
{
    if(!ExeBuilder_Append(ctx->builder, ctx->error, opcode, opv, opc, off, len)) {
        okNowJump(ctx);
        UNREACHABLE;
    }
}
