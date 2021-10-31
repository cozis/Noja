#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include "../utils/defs.h"
#include "compile.h"
#include "ASTi.h"

static Opcode exprkind_to_opcode(ExprKind kind)
{
	switch(kind)
		{
			case EXPR_POS: return OPCODE_POS;
			case EXPR_NEG: return OPCODE_NEG;
			case EXPR_ADD: return OPCODE_ADD;
			case EXPR_SUB: return OPCODE_SUB;
			case EXPR_MUL: return OPCODE_MUL;
			case EXPR_DIV: return OPCODE_DIV;
			default:
			UNREACHABLE;
			break;
		}
}

static _Bool emit_instr_for_node(ExeBuilder *exeb, Node *node, Error *error)
{
	assert(node != NULL);

	switch(node->kind)
		{
			case NODE_EXPR:
			{
				ExprNode *expr = (ExprNode*) node;
				switch(expr->kind)
					{
						case EXPR_POS:
						case EXPR_NEG:
						case EXPR_ADD:
						case EXPR_SUB:
						case EXPR_MUL:
						case EXPR_DIV:
						{
							OperExprNode *oper = (OperExprNode*) expr;

							for(Node *operand = oper->head; operand; operand = operand->next)
								emit_instr_for_node(exeb, operand, error);

							return ExeBuilder_Append(exeb, error,
								exprkind_to_opcode(expr->kind), 
								NULL, 0, node->offset, node->length);
						}

						case EXPR_INT:
						{
							IntExprNode *p = (IntExprNode*) expr;
							Operand op = { .type = OPTP_INT, .as_int = p->val };
							return ExeBuilder_Append(exeb, error, OPCODE_PUSHI, &op, 1, node->offset, node->length);
						}

						case EXPR_FLOAT:
						{
							FloatExprNode *p = (FloatExprNode*) expr;
							Operand op = { .type = OPTP_FLOAT, .as_float = p->val };
							return ExeBuilder_Append(exeb, error, OPCODE_PUSHF, &op, 1, node->offset, node->length);
						}

						case EXPR_STRING:
						{
							StringExprNode *p = (StringExprNode*) expr;
							Operand op = { .type = OPTP_STRING, .as_string = p->val };
							return ExeBuilder_Append(exeb, error, OPCODE_PUSHS, &op, 1, node->offset, node->length);
						}

						case EXPR_IDENT:
						{
							IdentExprNode *p = (IdentExprNode*) expr;
							Operand op = { .type = OPTP_STRING, .as_string = p->val };
							return ExeBuilder_Append(exeb, error, OPCODE_PUSHV, &op, 1, node->offset, node->length);
						}

						default:
						UNREACHABLE;
						break;
					}
				break;
			}

			default:
			UNREACHABLE;
			break;
		}
}

Executable *compile(AST *ast, BPAlloc *alloc, Error *error)
{
	assert(ast != NULL);
	assert(error != NULL);

	BPAlloc *alloc2 = alloc;

	if(alloc2 == NULL)
		{
			alloc2 = BPAlloc_Init(-1);

			if(alloc2 == NULL)
				return NULL;
		}

	Executable *exe = NULL;
	ExeBuilder *exeb = ExeBuilder_New(alloc2);

	if(exeb)
		{
			if(!emit_instr_for_node(exeb, ast->root, error))
				return 0;

			if(ExeBuilder_Append(exeb, error, OPCODE_RETURN, NULL, 0, Source_GetSize(ast->src), 0))
				exe = ExeBuilder_Finalize(exeb, error);
		}

	if(alloc == NULL)
		BPAlloc_Free(alloc2);
	return exe;
}