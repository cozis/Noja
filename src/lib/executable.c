
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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "executable.h"
#include "utils/defs.h"
#include "utils/bucketlist.h"

#define MAX_OPS 3
#define MAX_OPCODE_NAME_SIZE 127

typedef struct {
	Opcode 	opcode;
	int 	offset;
	int 	length;
	union {
		long long int as_int;
		double 		  as_float;
	} operands[MAX_OPS];
} Instruction;

struct xExecutable {
	int refs;
	int headl, bodyl;
	char 		*head;
	Instruction *body;
	Source 		*src;
};

struct xExeBuilder {
	BucketList *data, *code;
	int promc;
};

typedef struct {
	Opcode opcode;
	const char  *name;
	int opcount;
	OperandType *optypes;
} InstrInfo;

#define INSTR(name, ...)  	 	 		 \
	{							 		 \
		OPCODE_##name, 			 		 \
		#name,					 		 \
		sizeof((OperandType[]) { __VA_ARGS__ }) / sizeof(OperandType), \
		(OperandType[]) { __VA_ARGS__ }, \
	},

static const InstrInfo instr_table[] = {
	INSTR(NOPE)
	INSTR(POS)
	INSTR(NEG)
	INSTR(NOT)
	INSTR(ADD)
	INSTR(SUB)
	INSTR(MUL)
	INSTR(DIV)
	INSTR(MOD)
	INSTR(EQL)
	INSTR(NQL)
	INSTR(LSS)
	INSTR(GRT)
	INSTR(LEQ)
	INSTR(GEQ)
	INSTR(NLB)
	INSTR(STP)
	INSTR(ASS, OPTP_STRING)
	INSTR(POP, OPTP_INT)
	INSTR(CHECKTYPE, OPTP_INT, OPTP_STRING)
	INSTR(CALL, OPTP_INT, OPTP_INT)
	INSTR(SELECT)
	INSTR(SELECT2)
	INSTR(INSERT)
	INSTR(INSERT2)
	INSTR(PUSHINT, OPTP_INT)
	INSTR(PUSHFLT, OPTP_FLOAT)
	INSTR(PUSHSTR, OPTP_STRING)
	INSTR(PUSHVAR, OPTP_STRING)
	INSTR(PUSHTRU)
	INSTR(PUSHFLS)
	INSTR(PUSHNNE)
	INSTR(PUSHFUN, OPTP_IDX, OPTP_INT, OPTP_STRING)
	INSTR(PUSHLST, OPTP_INT)
	INSTR(PUSHMAP, OPTP_INT)
	INSTR(PUSHTYP)
	INSTR(PUSHNNETYP)
	INSTR(RETURN, OPTP_INT)
	INSTR(EXIT)
	INSTR(JUMPIFNOTANDPOP, OPTP_IDX)
	INSTR(JUMPIFANDPOP, OPTP_IDX)
	INSTR(JUMP, OPTP_IDX)
};

static const size_t instr_count = sizeof(instr_table)/sizeof(instr_table[0]);

_Bool Executable_GetOpcodeBinaryFromName(const char *name, size_t len, Opcode *opcode)
{
	// The input name is assumed not zero-terminated,
	// so to simplify things we duplicate it to add
	// an extra null byte.
	char buff[MAX_OPCODE_NAME_SIZE+1]; // No opcode should have a name this big.
	if (len >= sizeof(buff))
		return false;
	memcpy(buff, name, len);
	buff[len] = '\0';
	name = buff;

	// Now name is zero terminated.
	ASSERT(name[len] == '\0');

	for(size_t i = 0; i < instr_count; i += 1) {
		if(!strcmp(name, instr_table[i].name)) {
			if (opcode != NULL)
				*opcode = instr_table[i].opcode;
			return 1;
		}
	}
	return 0;
}

static const InstrInfo *Executable_GetInstrByOpcode(Opcode opcode)
{
	for (size_t i = 0; i < instr_count; i++)
		if (instr_table[i].opcode == opcode)
			return instr_table + i;
	return NULL;
}

const char *Executable_GetOpcodeName(Opcode opcode)
{
	const InstrInfo *instr = Executable_GetInstrByOpcode(opcode);
	if (instr == NULL)
		return NULL;
	return instr->name;
}

Executable *Executable_Copy(Executable *exe)
{
	ASSERT(exe != NULL);

	exe->refs += 1;
	return exe;
}

void Executable_Free(Executable *exe)
{
	exe->refs -= 1;
	ASSERT(exe->refs >= 0);

	if(exe->refs == 0)
	{
		if(exe->src)
			Source_Free(exe->src);
		free(exe);
	}
}

void Executable_Dump(Executable *exe, FILE *fp)
{
	for(int i = 0; i < exe->bodyl; i += 1)
	{
		Opcode opcode;
		Operand ops[MAX_OPS];
		int opc = MAX_OPS;

		(void) Executable_Fetch(exe, i, &opcode, ops, &opc);
		
		const InstrInfo *info = Executable_GetInstrByOpcode(opcode);

		fprintf(fp, "%d: %s ", i, info->name);

		for(int j = 0; j < opc; j += 1)
		{
			switch(ops[j].type)
			{
				case OPTP_IDX:
				case OPTP_INT:
				fprintf(fp, "%lld ", ops[j].as_int);
				break;

				case OPTP_FLOAT:
				fprintf(fp, "%f ", ops[j].as_float);
				break;

				case OPTP_STRING:
				fprintf(fp, "[%s] ", ops[j].as_string);
				break;

				case OPTP_PROMISE:
				UNREACHABLE;
				break;
			}
		}

		fprintf(fp, "\n");
	}
}

_Bool Executable_SetSource(Executable *exe, Source *src)
{
	src = Source_Copy(src);
	
	if(src == NULL)
		return 0;

	exe->src = src;
	return 1;
}

Source *Executable_GetSource(Executable *exe)
{
	return exe->src;
}

int Executable_GetInstrOffset(Executable *exe, int index)
{
	if(index < 0 || index >= exe->bodyl)
		return -1;

	if(exe->src)
		return exe->body[index].offset;
	else
		return -1;
}

int Executable_GetInstrLength(Executable *exe, int index)
{
	if(index < 0 || index >= exe->bodyl)
		return -1;

	if(exe->src)
		return exe->body[index].length;
	else
		return -1;
}

_Bool Executable_Fetch(Executable *exe, int index, Opcode *opcode, Operand *ops, int *opc)
{
	ASSERT(index >= 0);

	if(index >= exe->bodyl)
		return 0;

	const Instruction *instr = exe->body + index;
	const InstrInfo *info = Executable_GetInstrByOpcode(instr->opcode);

	if(opcode)
		*opcode = instr->opcode;

	int i;

	if(ops && opc)
	{
		int k = MIN(*opc, info->opcount);

		for(i = 0; i < k; i += 1)
		{
			OperandType type = info->optypes[i];
			ASSERT(type != OPTP_PROMISE);

			switch(type) 
			{
				case OPTP_STRING:
				{
					int data_offset = instr->operands[i].as_int;

					ASSERT(data_offset < exe->headl);

					ops[i].type = OPTP_STRING;
					ops[i].as_string = exe->head + data_offset;
					break;
				}
						
				case OPTP_IDX:
				ops[i].type = OPTP_IDX;
				ops[i].as_int = instr->operands[i].as_int;
				break;

				case OPTP_INT:
				ops[i].type = OPTP_INT;
				ops[i].as_int = instr->operands[i].as_int;
				break;

				case OPTP_FLOAT:
				ops[i].type = OPTP_FLOAT;
				ops[i].as_int = instr->operands[i].as_int;
				break;

				case OPTP_PROMISE:
				UNREACHABLE;
				break;
			}
		}
		
		*opc = MIN(*opc, info->opcount);
	}

	return 1;
}

_Bool Executable_Equiv(Executable *exe1, Executable *exe2, FILE *log, const char *log_prefix)
{
	int idx = 0;
	while(1) {
		Operand exe1_opv[MAX_OPS];
		Operand exe2_opv[MAX_OPS];
		int exe1_opc = MAX_OPS;
		int exe2_opc = MAX_OPS;
		Opcode exe1_opcode;
		Opcode exe2_opcode;
		_Bool exe1_done = !Executable_Fetch(exe1, idx, &exe1_opcode, exe1_opv, &exe1_opc);
		_Bool exe2_done = !Executable_Fetch(exe2, idx, &exe2_opcode, exe2_opv, &exe2_opc);
		if(exe1_done != exe2_done) {
			if(log != NULL)
				fprintf(log, "%sExecutables have different sizes\n", log_prefix);
			return false;
		}

		if(exe1_done == true)
			break;

		if(exe1_opcode != exe2_opcode) {
			if(log != NULL)
				fprintf(log, "%sInstructions at index %d have different opcodes (\"%s\" != \"%s\")\n", log_prefix, idx, 
					Executable_GetOpcodeName(exe1_opcode), Executable_GetOpcodeName(exe2_opcode));
			return false;
		}

		// Since the instruction opcode is the
		// same, the number of operands must be
		// the same too. (Their type must be
		// the same too.)
		ASSERT(exe1_opc == exe2_opc);

		for(int opno = 0; opno < exe1_opc; opno += 1) {

			ASSERT(exe1_opv[opno].type == exe2_opv[opno].type);

			// Also, an executable can never have
			// a promise operand. That's only used
			// when building the executable.
			ASSERT(exe1_opv[opno].type != OPTP_PROMISE);

			switch(exe1_opv[opno].type) {
				
				case OPTP_IDX:
				case OPTP_INT:
				{
					int v1 = exe1_opv[opno].as_int;
					int v2 = exe2_opv[opno].as_int;
					if(v1 != v2) {
						if(log != NULL)
							fprintf(log, "%s%s Instructions (at index %d) have different integer operands (at index %d) (%d != %d)\n", 
								log_prefix, Executable_GetOpcodeName(exe1_opcode), idx, opno, v1, v2);
						return false;
					}
				}
				break;

				case OPTP_FLOAT:
				{
					double v1 = exe1_opv[opno].as_float;
					double v2 = exe2_opv[opno].as_float;
					if(v1 != v2) {
						if(log != NULL)
							fprintf(log, "%s%s Instructions (at index %d) have different floating operands (at index %d) (%f != %f)\n", 
								log_prefix, Executable_GetOpcodeName(exe1_opcode), idx, opno, v1, v2);
						return false;
					}
				}
				break;

				case OPTP_STRING:
				{
					const char *v1 = exe1_opv[opno].as_string;
					const char *v2 = exe2_opv[opno].as_string;
					if(strcmp(v1, v2)) {
						if(log != NULL)
							// TODO: Escape the strings before printing them.
							fprintf(log, "%s%s Instructions (at index %d) have different string operands (at index %d) (\"%s\" != \"%s\")\n", 
								log_prefix, Executable_GetOpcodeName(exe1_opcode), idx, opno, v1, v2);
						return false;
					}
				}
				break;
				
				case OPTP_PROMISE:
				UNREACHABLE;
				break;
			}
		}

		idx += 1;
	}
	return true;
}

ExeBuilder *ExeBuilder_New(BPAlloc *alloc)
{
	ASSERT(alloc != NULL);

	ExeBuilder *exeb = BPAlloc_Malloc(alloc, sizeof(ExeBuilder));

	if(exeb == NULL)
		return NULL;

	exeb->promc = 0;
	exeb->data = BucketList_New(alloc);
	exeb->code = BucketList_New(alloc);

	if(exeb->data == NULL || exeb->code == NULL)
		return NULL;
	return exeb;
}

Executable *ExeBuilder_Finalize(ExeBuilder *exeb, Error *error)
{
	ASSERT(exeb != NULL);

	if(exeb->promc > 0)
	{
		Error_Report(error, ErrorType_INTERNAL, "There are still %d unfulfilled promises", exeb->promc);
		return 0;
	}

	Executable *exe;
	{
		int data_size = BucketList_Size(exeb->data);
		int code_size = BucketList_Size(exeb->code);

		ASSERT(code_size % sizeof(Instruction) == 0);

		void *temp = malloc(sizeof(Executable) + data_size + code_size);

		if(temp == NULL)
		{
			Error_Report(error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}

		exe = temp;
		exe->headl = data_size;
		exe->bodyl = code_size / sizeof(Instruction);
		exe->body = (Instruction*) (exe + 1);
		exe->head = (char*) (exe->body + exe->bodyl);
		exe->refs = 1;
		exe->src = NULL;
		
	}

	BucketList_Copy(exeb->data, exe->head, -1);
	BucketList_Copy(exeb->code, exe->body, -1);
	return exe;
}

static void promise_callback(void *userp)
{
	ASSERT(userp != NULL);

	ExeBuilder *exeb = userp;

	exeb->promc -= 1;
	ASSERT(exeb->promc >= 0);
}

_Bool ExeBuilder_Append(ExeBuilder *exeb, Error *error, Opcode opcode, Operand *opv, int opc, int off, int len)
{
	ASSERT(exeb != NULL);
	ASSERT(opc >= 0);

	static const char *operand_type_names[] = {
		[OPTP_IDX] = "index",
		[OPTP_INT] = "int",
		[OPTP_FLOAT]  = "float",
		[OPTP_STRING] = "string",
	};

	static const char *operand_type_arts[] = {
		[OPTP_IDX] = "an",
		[OPTP_INT] = "an",
		[OPTP_FLOAT]  = "a",
		[OPTP_STRING] = "a",
	};

	static const unsigned int operand_type_sizes[] = {
		[OPTP_IDX] = membersizeof(Operand, as_int),
		[OPTP_INT] = membersizeof(Operand, as_int),
		[OPTP_FLOAT]  = membersizeof(Operand, as_float),
		[OPTP_STRING] = membersizeof(Operand, as_string),
	};

	const InstrInfo *info = Executable_GetInstrByOpcode(opcode);

#ifndef NDEBUG
	if (info == NULL) {
		Error_Report(error, ErrorType_INTERNAL, "Missing instruction table entry for opcode %d", opcode);
		return 0;
	}
#endif

	if(opc != info->opcount)
	{
		// ERROR: Too many operands were provided.
		Error_Report(error, ErrorType_INTERNAL, 
			"Instruction %s expects %d operands, but %d were provided", 
			info->name, info->opcount, opc);
		return 0;
	}
	
	ASSERT(opc <= MAX_OPS);

	{
		Instruction *instr = BucketList_Append2(exeb->code, NULL, sizeof(Instruction));

		if(instr == NULL)
		{
			Error_Report(error, ErrorType_INTERNAL, "No memory");
			return 0;
		}

		instr->opcode = opcode;
		instr->offset = off;
		instr->length = len;

		for(int i = 0; i < opc; i += 1)
		{
			// Check that the expected type and the provided one
			// match, or that the provided type is a promise with
			// the same size of the expected type.
			{
				OperandType expected_type = info->optypes[i];
				OperandType provided_type = opv[i].type;

				ASSERT(expected_type != OPTP_PROMISE);

				if(provided_type == OPTP_PROMISE)
				{
					ASSERT(opv[i].as_promise != NULL);

					if(expected_type == OPTP_STRING)
					{
						Error_Report(error, ErrorType_INTERNAL, "Promise values can't be provided as string operands");
						return 0;
					}
							
					if(Promise_Size(opv[i].as_promise) != operand_type_sizes[expected_type])
					{
						Error_Report(error, ErrorType_INTERNAL, 
							"Provided promise has a value size of %d, "
							"but since %s %s was expected, the promise "
							"size must be %d",
							Promise_Size(opv[i].as_promise),
							operand_type_arts[expected_type],
							operand_type_names[expected_type],
							operand_type_sizes[expected_type]);
						return 0;
					}
				}
				else if(expected_type != provided_type)
				{
					// ERROR: Wrong operand type provided.
					Error_Report(error, ErrorType_INTERNAL, 
						"Instruction %s expects %s %s as operand %d, but %s %s was provided instead", 
						info->name, 
						operand_type_arts[expected_type], 
						operand_type_names[expected_type], 
						i, 
						operand_type_arts[provided_type], 
						operand_type_names[provided_type]
					);
					return 0;
				}
			}

			// Do the copying of the operands.
			switch(opv[i].type)
			{
				case OPTP_STRING:

				instr->operands[i].as_int = BucketList_Size(exeb->data);
						
				if(!BucketList_Append(exeb->data, opv[i].as_string, strlen(opv[i].as_string)+1))
				{
					Error_Report(error, ErrorType_INTERNAL, "No memory");
					return 0;
				}
				break;

				case OPTP_PROMISE:
				ASSERT(info->optypes[i] != OPTP_STRING);

				// This must be incremented before subscribing
				// since the counter-decrementing callback may 
				// be called immediately if the promise was 
				// already fulfilled.
				exeb->promc += 1;

				if(!Promise_Subscribe2(opv[i].as_promise, &instr->operands[i], exeb, promise_callback))
				{
					Error_Report(error, ErrorType_INTERNAL, "No memory");
					return 0;
				}
				break;

				case OPTP_IDX:
				instr->operands[i].as_int = opv[i].as_int;
				break;

				case OPTP_INT:
				instr->operands[i].as_int = opv[i].as_int;
				break;

				case OPTP_FLOAT:
				instr->operands[i].as_float = opv[i].as_float;
				break;
			}
		}
	}

	return 1;
}

BPAlloc *ExeBuilder_GetAlloc(ExeBuilder *exeb)
{
	return BucketList_GetAlloc(exeb->code);
}

int ExeBuilder_InstrCount(ExeBuilder *exeb)
{
	int raw_size = BucketList_Size(exeb->code);

	ASSERT(raw_size % sizeof(Instruction) == 0);

	return raw_size / sizeof(Instruction);
}