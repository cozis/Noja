#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include "../utils/error.h"
#include "../utils/source.h"
#include "../utils/labellist.h"
#include "../common/executable.h"

typedef struct {
    const char *str;
    size_t len;
    size_t cur;
} Context;

static void skipIdentifier(Context *ctx)
{
    while(ctx->cur < ctx->len && (isalpha(ctx->str[ctx->cur]) || isdigit(ctx->str[ctx->cur]) || ctx->str[ctx->cur] == '_'))
        ctx->cur += 1;
}

static void skipSpaces(Context *ctx)
{
    while(ctx->cur < ctx->len && isspace(ctx->str[ctx->cur]))
        ctx->cur += 1;
}

typedef struct {
    size_t offset;
    size_t length;
} Slice;

static bool parseLabelAndOpcode(Context *ctx, bool *no_label, 
                                Slice *label, Slice *opcode, 
                                Error *error)
{
    assert(ctx != NULL && no_label != NULL 
        && label != NULL && opcode != NULL);

    // NOTE: This function must start at
    // the first byte of the label or
    // opcode. All whitespace must be
    // consumed by the caller. 

    // Now we expect either a label and an
    // opcode, or just an opcode
    //
    //   <label>: <opcode> ..operands..
    //   <opcode> ..operands..
    //
    // Labels and opcodes can bot contain
    // digits, letters and underscores.

    Slice label_or_opcode;
    {
        char c = ctx->str[ctx->cur];
        if(!isalpha(c) && c != '_') {
            // ERROR: Missing opcode
            Error_Report(error, ErrorType_SYNTAX, "Missing opcode");
            #warning "should there be a return here?"
        }
        
        label_or_opcode.offset = ctx->cur;
        skipIdentifier(ctx);
        label_or_opcode.length = ctx->cur - label_or_opcode.offset;

        assert(label_or_opcode.length > 0);
    }

    // Get the character after the label or opcode
    // (ignoring whitespace), and if it's a ':',
    // then it was a label.
    skipSpaces(ctx);
    if(ctx->cur < ctx->len && ctx->str[ctx->cur] == ':') {

        // Skip the ':' and the whitespace after it.
        ctx->cur += 1;
        skipSpaces(ctx);

        if(ctx->cur == ctx->len || (!isalpha(ctx->str[ctx->cur]) && ctx->str[ctx->cur] != '_')) {
            Error_Report(error, ErrorType_SYNTAX, "Missing opcode after label");
            return false;
        }

        // Now the opcode is expected.
        opcode->offset = ctx->cur;
        skipIdentifier(ctx);
        opcode->length = ctx->cur - opcode->offset;
        assert(opcode->length > 0);

        *no_label = false;
        *label = label_or_opcode;
        skipSpaces(ctx);
    } else {
        *no_label = true;
        *opcode = label_or_opcode;
    }
    return true;
}

static bool parseStringOperand(Context *ctx, Error *error, BPAlloc *alloc, Operand *op)
{
    // Skip the first double quote.
    assert(ctx->cur < ctx->len && ctx->str[ctx->cur] == '"');
    ctx->cur += 1;

    size_t literal_offset = ctx->cur;

    // For now do a dumb copy into the buffer
    // without considering special characters.
    while(ctx->cur < ctx->len && ctx->str[ctx->cur] != '"')
        ctx->cur += 1;

    if(ctx->cur == ctx->len) {
        Error_Report(error, ErrorType_SYNTAX, "End of source inside a string literal");
        return false;
    }

    size_t literal_length = ctx->cur - literal_offset;

    // Skip the ending double quotes.
    assert(ctx->cur < ctx->len && ctx->str[ctx->cur] == '"');
    ctx->cur += 1;
    
    char *copy = BPAlloc_Malloc(alloc, literal_length+1);
    if(copy == NULL) {
        Error_Report(error, ErrorType_INTERNAL, "No memory");
        return false;
    }

    memcpy(copy, ctx->str + literal_offset, literal_length);
    copy[literal_length] = '\0';

    op->type = OPTP_STRING;
    op->as_string = copy;
    return true;
}

static bool parseIntegerOperand(Context *ctx, Error *error, Operand *op)
{
    // It's ensured by the caller that the cursor is
    // pointing to a sequence of one or more digits
    // NOT followed by a dot and a digit after that
    // (so this is an integer for sure, not a float).
    assert(ctx->cur < ctx->len && isdigit(ctx->str[ctx->cur]));

    long long int buffer = 0;
    do {
        // Transform each digit into its integer value.
        int d = ctx->str[ctx->cur] - '0';
        assert(d >= 0 && d <= 9);

        // Will this overflow?
        if(buffer > (LLONG_MAX - d) / 10) {
            Error_Report(error, ErrorType_SEMANTIC, "Integer literal is too big to be represented in %d bits", 8*sizeof(buffer));
            return false;
        }

        buffer = buffer * 10 + d;
        ctx->cur += 1;
    } while(ctx->cur < ctx->len && isdigit(ctx->str[ctx->cur]));

    // Not a float!

    op->type = OPTP_INT;
    op->as_int = buffer;
    return true;
}

static bool parseFloatingOperand(Context *ctx, Error *error, Operand *op)
{   
    (void) error; // At the moment this function doesn't report
                  // any error. This may change when overflows
                  // and underflows are detected.

    // It's ensured by the caller that the cursor is
    // pointing to a sequence of one or more digits
    // followed by a dot and a digit after that.
    assert(ctx->cur < ctx->len && isdigit(ctx->str[ctx->cur]));

    double buffer = 0;
    do {
        // Transform each digit into its integer value.
        int d = ctx->str[ctx->cur] - '0';
        assert(d >= 0 && d <= 9);

        // Should overflow be checked?
        buffer = buffer * 10 + d;

        ctx->cur += 1;
    } while(ctx->str[ctx->cur] != '.');

    assert(ctx->cur+1 < ctx->len && ctx->str[ctx->cur] == '.' && isdigit(ctx->str[ctx->cur+1]));

    // Skip the dot.
    ctx->cur += 1;

    double q = 1;
    do {
        // Transform each digit into its integer value.
        int d = ctx->str[ctx->cur] - '0';
        assert(d >= 0 && d <= 9);

        q /= 10;
        buffer += q * d;

        ctx->cur += 1;
    } while(ctx->cur < ctx->len && isdigit(ctx->str[ctx->cur]));
    
    op->type = OPTP_FLOAT;
    op->as_float = buffer;
    return true;
}

static bool parseOperands(Context *ctx, BPAlloc *alloc, Error *error, 
                          LabelList *list, Operand *opv, int *opc, int opc_max)
{
    // NOTE: The whitespace before the first operand must
    // be consumed by the caller.

    if(ctx->cur < ctx->len && ctx->str[ctx->cur] != ';')
        while(1) {

            Operand op;

            char c = ctx->str[ctx->cur];
            if(c == '"') {
            
                if(!parseStringOperand(ctx, error, alloc, &op))
                    return false;
            
            } else if(isdigit(c)) {
            
                /* Integer or float operand */

                size_t k = ctx->cur;
                while(k < ctx->len && isdigit(ctx->str[k]))
                    k += 1;
                
                bool ok;
                if(k+1 >= ctx->len || ctx->str[k] != '.' || !isdigit(ctx->str[k+1]))
                    ok = parseIntegerOperand(ctx, error, &op);
                else
                    ok = parseFloatingOperand(ctx, error, &op);
                
                if(!ok)
                    return false;

            } else if(isalpha(c) || c == '_') {
                
                /* Label */
                size_t offset = ctx->cur;
                skipIdentifier(ctx);
                size_t length = ctx->cur - offset;

                Promise *promise = LabelList_GetLabel(list, ctx->str + offset, length);
                if(promise == NULL) {
                    Error_Report(error, ErrorType_INTERNAL, "No memory");
                    return false;
                }

                op.type = OPTP_PROMISE;
                op.as_promise = promise;

            } else {
                // ERROR: Unexpected character
                Error_Report(error, ErrorType_SYNTAX, "Unexpected character '%c'", c);
                return false;
            }

            if(*opc == opc_max) {
                Error_Report(error, ErrorType_SEMANTIC, "Too many operands");
                return false;
            }
            opv[*opc] = op;
            *opc += 1;
            // Now prepare for the next operand
            skipSpaces(ctx);

            if(ctx->cur == ctx->len || ctx->str[ctx->cur] == ';')
                break;

            c = ctx->str[ctx->cur];
            if(c != ',') {
                Error_Report(error, ErrorType_SYNTAX, "Unexpected character '%c' (',' or ';' were expected)", c);
                return false;
            }

            // Skip the comma
            ctx->cur += 1;

            // Skip the spaces before the next operand
            skipSpaces(ctx);
        }
    assert(ctx->cur == ctx->len || ctx->str[ctx->cur] == ';');
    return true;
}

Executable *assemble(Source *src, Error *error)
{
    Executable *exe = NULL;

    BPAlloc *alloc = BPAlloc_Init(-1);
    if(alloc == NULL) {
        Error_Report(error, ErrorType_INTERNAL, "No memory");
        return NULL;
    }

    LabelList *list = LabelList_New(alloc);
    if(list == NULL) {
        Error_Report(error, ErrorType_INTERNAL, "No memory");
        BPAlloc_Free(alloc);
        return NULL;
    }

    ExeBuilder *builder = ExeBuilder_New(alloc);
    if(builder == NULL) {
        Error_Report(error, ErrorType_INTERNAL, "No memory");
        LabelList_Free(list);
        BPAlloc_Free(alloc);
        return NULL;
    }

    Context ctx = { 
        .str = Source_GetBody(src),
        .len = Source_GetSize(src),
        .cur = 0,
    };
    while(1) {

        skipSpaces(&ctx);

        if(ctx.cur == ctx.len)
            break;

        bool no_label;
        Slice label, opcode_name;

        if(!parseLabelAndOpcode(&ctx, &no_label, &label, &opcode_name, error))
            goto done;

        // If a label was defined, add it to the list.
        if(no_label == false) {
            long long int value = ExeBuilder_InstrCount(builder);
            if(!LabelList_SetLabel(list, ctx.str + label.offset, label.length, value)) {
                Error_Report(error, ErrorType_INTERNAL, "Out of memory");
                goto done;
            }
        }

        // Check that the opcode is valid (at this
        // point it's just an unchecked identifier)
        Opcode opcode;
        const char *name = ctx.str + opcode_name.offset;
        if(!Executable_GetOpcodeBinaryFromName(name, opcode_name.length, &opcode)) {
            Error_Report(error, ErrorType_SEMANTIC, "Opcode %.*s doesn't exist", (int) opcode_name.length, name);
            goto done;
        }

        /* Parse operands */
        // (whitespace was already skipped)

        Operand opv[8];
        int     opc = 0;

        if(!parseOperands(&ctx, alloc, error, list, opv, &opc, sizeof(opv)/sizeof(opv[0])))
            goto done;

        // The operand list ended with a ';' or the
        // end of the file. If the file didn't end,
        // then the ';' must be consumed.
        assert(ctx.cur == ctx.len || ctx.str[ctx.cur] == ';');
        if(ctx.cur < ctx.len) ctx.cur += 1;

        if(!ExeBuilder_Append(builder, error, opcode, opv, opc, opcode_name.offset, opcode_name.length))
            goto done;
    }

    size_t unresolved_count = LabelList_GetUnresolvedCount(list);
    if(unresolved_count > 0) {
        Error_Report(error, ErrorType_SEMANTIC, "%d unresolved labels", unresolved_count);
        goto done;
    }

    exe = ExeBuilder_Finalize(builder, error);

    if(exe != NULL)
        (void) Executable_SetSource(exe, src);

done:
    LabelList_Free(list);
    BPAlloc_Free(alloc);
    return exe;
}
