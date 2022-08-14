#ifndef CODEGENCTX_H
#define CODEGENCTX_H
#include <setjmp.h>
#include "../common/executable.h"

typedef struct CodegenContext CodegenContext;
CodegenContext *CodegenContext_New(Error *error, BPAlloc *alloc);
void            CodegenContext_EmitInstr(CodegenContext *ctx, Opcode opcode, Operand *opv, int opc, int off, int len);
void            CodegenContext_SetJumpDest(CodegenContext *ctx, jmp_buf *env);
void            CodegenContext_Free(CodegenContext *ctx);
Executable     *CodegenContext_MakeExecutableAndFree(CodegenContext *ctx, Source *src);
void            CodegenContext_ReportErrorAndJump_(CodegenContext *ctx, const char *file, const char *func, int line, bool internal, const char *format, ...);
#define         CodegenContext_ReportErrorAndJump(ctx, int, fmt, ...) CodegenContext_ReportErrorAndJump_(ctx, __FILE__, __func__, __LINE__, int, fmt, ## __VA_ARGS__) 
int             CodegenContext_InstrCount(CodegenContext *ctx);

typedef struct Label Label;
Label   *Label_New(CodegenContext *ctx);
void     Label_Set(Label *label, long long int value);
void     Label_SetHere(Label *label, CodegenContext *ctx);
void     Label_Free(Label *label);
Promise *Label_ToPromise(Label *label);

#endif /* CODEGENCTX_H */