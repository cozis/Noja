#include <stdlib.h>
#include <string.h>
#include "../utils/defs.h"
#include "../utils/bucketlist.h"
#include "executable.h"

#define MAX_OPS 3

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
	const char  *name;
	int 		 stacksz;
	int 		 opcount;
	OperandType *optypes;
} InstrInfo;

static const InstrInfo instr_table[] = {

	[OPCODE_NOPE] = {"NOPE", 0, 0, NULL},

	[OPCODE_POS]  = {"POS",  1, 0, NULL},
	[OPCODE_NEG]  = {"NEG",  1, 0, NULL},

	[OPCODE_ADD]  = {"ADD",  2, 0, NULL},
	[OPCODE_SUB]  = {"SUB",  2, 0, NULL},
	[OPCODE_MUL]  = {"MUL",  2, 0, NULL},
	[OPCODE_DIV]  = {"DIV",  2, 0, NULL},

	[OPCODE_PUSHI]  = {"PUSHI",  1, 1, (OperandType[]) {OPTP_INT}},
	[OPCODE_PUSHF]  = {"PUSHF",  1, 1, (OperandType[]) {OPTP_FLOAT}},
	[OPCODE_PUSHS]  = {"PUSHS",  1, 1, (OperandType[]) {OPTP_STRING}},
	[OPCODE_PUSHV]  = {"PUSHV",  1, 1, (OperandType[]) {OPTP_STRING}},

	[OPCODE_RETURN] = {"RETURN", 0, 0, NULL},

	[OPCODE_JUMPIFNOTANDPOP] = {"JUMPIFNOTANDPOP", 1, 1, (OperandType[]) {OPTP_INT}},
	[OPCODE_JUMP] = {"JUMP", 0, 1, (OperandType[]) {OPTP_INT}},
};

static const char *operand_type_names[] = {
	[OPTP_INT] = "int",
	[OPTP_FLOAT] = "float",
	[OPTP_STRING] = "string",
};

static const char *operand_type_arts[] = {
	[OPTP_INT] = "an",
	[OPTP_FLOAT] = "a",
	[OPTP_STRING] = "a",
};

static unsigned int operand_type_sizes[] = {
	[OPTP_INT] = membersizeof(Operand, as_int),
	[OPTP_FLOAT] = membersizeof(Operand, as_float),
	[OPTP_STRING] = membersizeof(Operand, as_string),
};

Executable *Executable_Copy(Executable *exe)
{
	assert(exe != NULL);

	exe->refs += 1;
	return exe;
}

void Executable_Free(Executable *exe)
{
	exe->refs -= 1;
	assert(exe->refs >= 0);

	if(exe->refs == 0)
		{
			if(exe->src)
				Source_Free(exe->src);
			free(exe);
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

_Bool Executable_Fetch(Executable *exe, int index, Opcode *opcode, Operand *ops, int *opc)
{
	assert(index >= 0);

	if(index >= exe->bodyl)
		return 0;

	const Instruction *instr = exe->body + index;
	const InstrInfo *info = instr_table + instr->opcode;

	if(opcode)
		*opcode = instr->opcode;

	int i;

	if(ops && opc)
		{
			int k = MIN(*opc, info->opcount);

			for(i = 0; i < k; i += 1)
				{
					OperandType type = info->optypes[i];
					assert(type != OPTP_PROMISE);

					switch(type) {

						case OPTP_STRING:

						int data_offset = instr->operands[i].as_int;

						assert(data_offset < exe->headl);

						ops[i].type = OPTP_STRING;
						ops[i].as_string = exe->head + data_offset;
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

ExeBuilder *ExeBuilder_New(BPAlloc *alloc)
{
	assert(alloc != NULL);

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
	assert(exeb != NULL);

	if(exeb->promc > 0)
		{
			Error_Report(error, 0, "There are still %d unfulfilled promises", exeb->promc);
			return 0;
		}

	Executable *exe;
	{
		int data_size = BucketList_Size(exeb->data);
		int code_size = BucketList_Size(exeb->code);

		assert(code_size % sizeof(Instruction) == 0);

		void *temp = malloc(sizeof(Executable) + data_size + code_size);

		if(temp == NULL)
			{
				Error_Report(error, 1, "No memory");
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
	assert(userp != NULL);

	ExeBuilder *exeb = userp;

	exeb->promc -= 1;
	assert(exeb->promc >= 0);
}

_Bool ExeBuilder_Append(ExeBuilder *exeb, Error *error, Opcode opcode, Operand *opv, int opc, int off, int len)
{
	assert(exeb != NULL);
	assert(opc >= 0);

	const InstrInfo *info = instr_table + opcode;

	if(opc != info->opcount)
		{
			// ERROR: Too many operands were provided.
			Error_Report(error, 0, 
				"Instruction %s expects %d operands, but %d were provided", 
				info->name, opc, info->opcount);
			return 0;
		}

	assert(opc <= MAX_OPS);

	{
		Instruction *instr = BucketList_Append2(exeb->code, NULL, sizeof(Instruction));

		if(instr == NULL)
			{
				Error_Report(error, 1, "No memory");
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

					assert(expected_type != OPTP_PROMISE);

					if(provided_type == OPTP_PROMISE)
						{
							assert(opv[i].as_promise != NULL);

							if(expected_type == OPTP_STRING)
								{
									Error_Report(error, 0, "Promise values can't be provided as string operands");
									return 0;
								}
							
							if(Promise_Size(opv[i].as_promise) != operand_type_sizes[expected_type])
								{
									Error_Report(error, 0, 
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
							Error_Report(error, 0, 
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
								Error_Report(error, 1, "No memory");
								return 0;
							}
						break;

						case OPTP_PROMISE:

						assert(info->optypes[i] != OPTP_STRING);

						// This must be incremented before subscribing
						// since the counter-decrementing callback may 
						// be called immediately if the promise was 
						// already fulfilled.
						exeb->promc += 1;

						if(!Promise_Subscribe2(opv[i].as_promise, &instr->operands[i], exeb, promise_callback))
							{
								Error_Report(error, 1, "No memory");
								return 0;
							}
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

	assert(raw_size % sizeof(Instruction) == 0);

	return raw_size / sizeof(Instruction);
}

