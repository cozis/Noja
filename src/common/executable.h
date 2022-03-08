#ifndef EXECUTABLE_H
#define EXECUTABLE_H
#include "../utils/source.h"
#include "../utils/promise.h"

typedef enum {
	OPTP_INT,
	OPTP_FLOAT,
	OPTP_STRING,
	OPTP_PROMISE,
} OperandType;

typedef struct {
	OperandType type;
	union {
		long long int as_int;
		double 		  as_float;
		const char 	 *as_string;
		Promise 	 *as_promise;
	};
} Operand;

typedef enum {
	OPCODE_NOPE,
	OPCODE_POS,
	OPCODE_NEG,
	OPCODE_NOT,
	OPCODE_ADD,
	OPCODE_SUB,
	OPCODE_MUL,
	OPCODE_DIV,
	OPCODE_EQL,
	OPCODE_NQL,
	OPCODE_LSS,
	OPCODE_GRT,
	OPCODE_LEQ,
	OPCODE_GEQ,
	OPCODE_AND,
	OPCODE_OR,
	OPCODE_ASS,
	OPCODE_POP,
	OPCODE_CALL,
	OPCODE_SELECT,
	OPCODE_INSERT,
	OPCODE_INSERT2,
	OPCODE_PUSHINT,
	OPCODE_PUSHFLT,
	OPCODE_PUSHSTR,
	OPCODE_PUSHVAR,
	OPCODE_PUSHTRU,
	OPCODE_PUSHFLS,
	OPCODE_PUSHNNE,
	OPCODE_PUSHFUN,
	OPCODE_PUSHLST,
	OPCODE_PUSHMAP,
	OPCODE_RETURN,
	OPCODE_JUMPIFANDPOP,
	OPCODE_JUMPIFNOTANDPOP,
	OPCODE_JUMP,
} Opcode;

typedef struct xExecutable Executable;
typedef struct xExeBuilder ExeBuilder;

Executable *Executable_Copy(Executable *exe);
void 		Executable_Free(Executable *exe);
void 		Executable_Dump(Executable *exe);
_Bool		Executable_Fetch(Executable *exe, int index, Opcode *opcode, Operand *ops, int *opc);
_Bool 		Executable_SetSource(Executable *exe, Source *src);
Source 	   *Executable_GetSource(Executable *exe);
int 		Executable_GetInstrOffset(Executable *exe, int index);
int 		Executable_GetInstrLength(Executable *exe, int index);
const char *Executable_GetOpcodeName(Opcode opcode);

ExeBuilder *ExeBuilder_New(BPAlloc *alloc);
_Bool 		ExeBuilder_Append(ExeBuilder *exeb, Error *error, Opcode opcode, Operand *opv, int opc, int off, int len);
Executable *ExeBuilder_Finalize(ExeBuilder *exeb, Error *error);
BPAlloc    *ExeBuilder_GetAlloc(ExeBuilder *exeb);
int 		ExeBuilder_InstrCount(ExeBuilder *exeb);
#endif