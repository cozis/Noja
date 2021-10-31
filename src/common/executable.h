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
	OPCODE_ADD,
	OPCODE_SUB,
	OPCODE_MUL,
	OPCODE_DIV,
	OPCODE_PUSHI,
	OPCODE_PUSHF,
	OPCODE_PUSHS,
	OPCODE_PUSHV,
	OPCODE_RETURN,
	OPCODE_JUMPIFNOTANDPOP,
	OPCODE_JUMP,
} Opcode;

typedef struct xExecutable Executable;
typedef struct xExeBuilder ExeBuilder;

Executable *Executable_Copy(Executable *exe);
void 		Executable_Free(Executable *exe);
_Bool		Executable_Fetch(Executable *exe, int index, Opcode *opcode, Operand *ops, int *opc);
_Bool 		Executable_SetSource(Executable *exe, Source *src);
Source 	   *Executable_GetSource(Executable *exe);

ExeBuilder *ExeBuilder_New(BPAlloc *alloc);
_Bool 		ExeBuilder_Append(ExeBuilder *exeb, Error *error, Opcode opcode, Operand *opv, int opc, int off, int len);
Executable *ExeBuilder_Finalize(ExeBuilder *exeb, Error *error);
BPAlloc    *ExeBuilder_GetAlloc(ExeBuilder *exeb);
int 		ExeBuilder_InstrCount(ExeBuilder *exeb);
#endif