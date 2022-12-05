
/* +--------------------------------------------------------------------------+
** |                          _   _       _                                   |
** |                         | \ | |     (_)                                  |
** |                         |  \| | ___  _  __ _                             |
** |                         | . ` |/ _ \| |/ _` |                            |
** |                         | |\  | (_) | | (_| |                            |
** |                         |_| \_|\___/| |\__,_|                            |
** |                                    _/ |                                  |
** |                                   |__/                                   |
** +--------------------------------------------------------------------------+
** | Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>       |
** +--------------------------------------------------------------------------+
** | This file is part of The Noja Interpreter.                               |
** |                                                                          |
** | The Noja Interpreter is free software: you can redistribute it and/or    |
** | modify it under the terms of the GNU General Public License as published |
** | by the Free Software Foundation, either version 3 of the License, or (at |
** | your option) any later version.                                          |
** |                                                                          |
** | The Noja Interpreter is distributed in the hope that it will be useful,  |
** | but WITHOUT ANY WARRANTY; without even the implied warranty of           |
** | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General |
** | Public License for more details.                                         |
** |                                                                          |
** | You should have received a copy of the GNU General Public License along  |
** | with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.   |
** +--------------------------------------------------------------------------+ 
*/

#ifndef EXECUTABLE_H
#define EXECUTABLE_H
#include <stdio.h>
#include "../utils/source.h"
#include "../utils/promise.h"

typedef enum {
	OPTP_IDX,
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
	OPCODE_NLB,
	OPCODE_STP,
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
	OPCODE_PUSHTYP,
	OPCODE_PUSHTYPTYP,
	OPCODE_PUSHNNETYP,
	OPCODE_RETURN,
	OPCODE_ERROR,
	OPCODE_EXIT,
	OPCODE_JUMPIFANDPOP,
	OPCODE_JUMPIFNOTANDPOP,
	OPCODE_JUMP,
	OPCODE_CHECKTYPE,
} Opcode;

typedef struct xExecutable Executable;
typedef struct xExeBuilder ExeBuilder;

Executable *Executable_Copy(Executable *exe);
void 		Executable_Free(Executable *exe);
void 		Executable_Dump(Executable *exe);
_Bool       Executable_Equiv(Executable *exe1, Executable *exe2, FILE *log, const char *log_prefix);
_Bool		Executable_Fetch(Executable *exe, int index, Opcode *opcode, Operand *ops, int *opc);
_Bool 		Executable_SetSource(Executable *exe, Source *src);
Source 	   *Executable_GetSource(Executable *exe);
int 		Executable_GetInstrOffset(Executable *exe, int index);
int 		Executable_GetInstrLength(Executable *exe, int index);
const char *Executable_GetOpcodeName(Opcode opcode);
_Bool       Executable_GetOpcodeBinaryFromName(const char *name, size_t name_len, Opcode *opcode);

ExeBuilder *ExeBuilder_New(BPAlloc *alloc);
_Bool 		ExeBuilder_Append(ExeBuilder *exeb, Error *error, Opcode opcode, Operand *opv, int opc, int off, int len);
Executable *ExeBuilder_Finalize(ExeBuilder *exeb, Error *error);
BPAlloc    *ExeBuilder_GetAlloc(ExeBuilder *exeb);
int 		ExeBuilder_InstrCount(ExeBuilder *exeb);

#endif