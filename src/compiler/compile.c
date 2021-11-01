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
								if(!emit_instr_for_node(exeb, operand, error))
									return 0;

							return ExeBuilder_Append(exeb, error,
								exprkind_to_opcode(expr->kind), 
								NULL, 0, node->offset, node->length);
						}

						case EXPR_INT:
						{
							IntExprNode *p = (IntExprNode*) expr;
							Operand op = { .type = OPTP_INT, .as_int = p->val };
							return ExeBuilder_Append(exeb, error, OPCODE_PUSHINT, &op, 1, node->offset, node->length);
						}

						case EXPR_FLOAT:
						{
							FloatExprNode *p = (FloatExprNode*) expr;
							Operand op = { .type = OPTP_FLOAT, .as_float = p->val };
							return ExeBuilder_Append(exeb, error, OPCODE_PUSHFLT, &op, 1, node->offset, node->length);
						}

						case EXPR_STRING:
						{
							StringExprNode *p = (StringExprNode*) expr;
							Operand op = { .type = OPTP_STRING, .as_string = p->val };
							return ExeBuilder_Append(exeb, error, OPCODE_PUSHSTR, &op, 1, node->offset, node->length);
						}

						case EXPR_IDENT:
						{
							IdentExprNode *p = (IdentExprNode*) expr;
							Operand op = { .type = OPTP_STRING, .as_string = p->val };
							return ExeBuilder_Append(exeb, error, OPCODE_PUSHVAR, &op, 1, node->offset, node->length);
						}

						case EXPR_NONE:
						return ExeBuilder_Append(exeb, error, OPCODE_PUSHNNE, NULL, 0, node->offset, node->length);

						case EXPR_TRUE:
						return ExeBuilder_Append(exeb, error, OPCODE_PUSHTRU, NULL, 0, node->offset, node->length);

						case EXPR_FALSE:
						return ExeBuilder_Append(exeb, error, OPCODE_PUSHFLS, NULL, 0, node->offset, node->length);

						default:
						UNREACHABLE;
						break;
					}
				break;
			}

			case NODE_IFELSE:
			{
				IfElseNode *ifelse = (IfElseNode*) node;

				if(!emit_instr_for_node(exeb, ifelse->condition, error))
					return 0;

				if(ifelse->false_branch)
					{
						Promise *else_offset = Promise_New(ExeBuilder_GetAlloc(exeb), sizeof(long long int));
						Promise *done_offset = Promise_New(ExeBuilder_GetAlloc(exeb), sizeof(long long int));

						if(else_offset == NULL || done_offset == NULL)
							{
								Error_Report(error, 1, "No memory");
								return 0;
							}

						Operand op = { .type = OPTP_PROMISE, .as_promise = else_offset };
						if(!ExeBuilder_Append(exeb, error, OPCODE_JUMPIFNOTANDPOP, &op, 1, node->offset, node->length))
							return 0;

						if(!emit_instr_for_node(exeb, ifelse->true_branch, error))
							return 0;

						if(ifelse->true_branch->kind == NODE_EXPR)
							{
								Operand op = (Operand) { .type = OPTP_INT, .as_int = 1 };
								if(!ExeBuilder_Append(exeb, error, OPCODE_POP, &op, 1, ifelse->true_branch->offset, 0))
									return 0;
							}
						
						op = (Operand) { .type = OPTP_PROMISE, .as_promise = done_offset };
						if(!ExeBuilder_Append(exeb, error, OPCODE_JUMP, &op, 1, node->offset, node->length))
							return 0;

						long long int temp = ExeBuilder_InstrCount(exeb);
						Promise_Resolve(else_offset, &temp, sizeof(temp));

						if(!emit_instr_for_node(exeb, ifelse->false_branch, error))
							return 0;

						if(ifelse->false_branch->kind == NODE_EXPR)
							{
								Operand op = (Operand) { .type = OPTP_INT, .as_int = 1 };
								if(!ExeBuilder_Append(exeb, error, OPCODE_POP, &op, 1, ifelse->false_branch->offset, 0))
									return 0;
							}

						temp = ExeBuilder_InstrCount(exeb);
						Promise_Resolve(done_offset, &temp, sizeof(temp));

						Promise_Free(else_offset);
						Promise_Free(done_offset);
					}
				else
					{
						Promise *done_offset = Promise_New(ExeBuilder_GetAlloc(exeb), sizeof(long long int));

						if(done_offset == NULL)
							{
								Error_Report(error, 1, "No memory");
								return 0;
							}

						if(!ExeBuilder_Append(exeb, error, OPCODE_JUMPIFNOTANDPOP, &(Operand) { .type = OPTP_PROMISE, .as_promise = done_offset }, 1, node->offset, node->length))
							return 0;

						if(!emit_instr_for_node(exeb, ifelse->true_branch, error))
							return 0;

						if(ifelse->true_branch->kind == NODE_EXPR)
							{
								Operand op = (Operand) { .type = OPTP_INT, .as_int = 1 };
								if(!ExeBuilder_Append(exeb, error, OPCODE_POP, &op, 1, ifelse->true_branch->offset, 0))
									return 0;
							}

						long long int temp = ExeBuilder_InstrCount(exeb);
						Promise_Resolve(done_offset, &temp, sizeof(temp));

						Promise_Free(done_offset);
					}

				return 1;
			}

			case NODE_COMP:
			{
				CompoundNode *comp = (CompoundNode*) node;

				Node *stmt = comp->head;

				while(stmt)
					{
						if(!emit_instr_for_node(exeb, stmt, error))
							return 0;

						if(stmt->kind == NODE_EXPR)
							{
								Operand op = (Operand) { .type = OPTP_INT, .as_int = 1 };
								if(!ExeBuilder_Append(exeb, error, OPCODE_POP, &op, 1, stmt->offset, 0))
									return 0;
							}

						stmt = stmt->next;
					}

				return 1;
			}

			default:
			UNREACHABLE;
			return 0;
		}
	UNREACHABLE;
	return 0;
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