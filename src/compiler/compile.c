
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
** |                         WHAT IS THIS FILE?                               |
** |                                                                          |
** | This file implements the routines that transform the AST into a list of  |
** | bytecodes. The functionalities of this file are exposed through the      |
** | `compile` function, that takes as input an `AST` and outputs an          |
** | `Executable`.                                                            |
** |                                                                          |
** | The function that does the heavy lifting is `emit_instr_for_node` which  |
** | walks the tree and writes instructions to the `ExeBuilder`.              |
** |                                                                          |
** | Some semantic errors are catched at this phase, in which case, they are  |
** | reported by filling out the `error` structure and aborting. It's also    |
** | possible that the compilation fails bacause of internal errors (which    |
** | usually means "out of memory").                                          |
** +--------------------------------------------------------------------------+
*/

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
			case EXPR_NOT: return OPCODE_NOT;
			case EXPR_POS: return OPCODE_POS;
			case EXPR_NEG: return OPCODE_NEG;
			case EXPR_ADD: return OPCODE_ADD;
			case EXPR_SUB: return OPCODE_SUB;
			case EXPR_MUL: return OPCODE_MUL;
			case EXPR_DIV: return OPCODE_DIV;
			case EXPR_EQL: return OPCODE_EQL;
			case EXPR_NQL: return OPCODE_NQL;
			case EXPR_LSS: return OPCODE_LSS;
			case EXPR_LEQ: return OPCODE_LEQ;
			case EXPR_GRT: return OPCODE_GRT;
			case EXPR_GEQ: return OPCODE_GEQ;
			case EXPR_AND: return OPCODE_AND;
			case EXPR_OR:  return OPCODE_OR;
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
						case EXPR_NOT:
						case EXPR_POS:
						case EXPR_NEG:
						case EXPR_ADD:
						case EXPR_SUB:
						case EXPR_MUL:
						case EXPR_DIV:
						case EXPR_EQL:
						case EXPR_NQL:
						case EXPR_LSS:
						case EXPR_LEQ:
						case EXPR_GRT:
						case EXPR_GEQ:
						case EXPR_AND:
						case EXPR_OR:
						{
							OperExprNode *oper = (OperExprNode*) expr;

							for(Node *operand = oper->head; operand; operand = operand->next)
								if(!emit_instr_for_node(exeb, operand, error))
									return 0;

							return ExeBuilder_Append(exeb, error,
								exprkind_to_opcode(expr->kind), 
								NULL, 0, node->offset, node->length);
						}

						case EXPR_ASS:
						{
							OperExprNode *oper = (OperExprNode*) expr;

							Node *lop, *rop;
							lop = oper->head;
							rop = lop->next;

							if(!emit_instr_for_node(exeb, rop, error))
								return 0;

							if(((ExprNode*) lop)->kind == EXPR_IDENT)
								{
									const char *name = ((IdentExprNode*) lop)->val;

									Operand op = { .type = OPTP_STRING, .as_string = name };
									if(!ExeBuilder_Append(exeb, error, OPCODE_ASS, &op, 1, node->offset, node->length))
										return 0;
								}
							else if(((ExprNode*) lop)->kind == EXPR_SELECT)
								{
									Node *idx = ((IndexSelectionExprNode*) lop)->idx;
									Node *set = ((IndexSelectionExprNode*) lop)->set;

									if(!emit_instr_for_node(exeb, set, error))
										return 0;

									if(!emit_instr_for_node(exeb, idx, error))
										return 0;

									if(!ExeBuilder_Append(exeb, error, OPCODE_INSERT2, NULL, 0, node->offset, node->length))
										return 0;
								}
							else
								{
									Error_Report(error, 0, "Assignment left operand can't be assigned to");
									return 0;
								}

							return 1;
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

						case EXPR_LIST:
						{
							// PUSHLST
							// PUSHINT
							// <expr>
							// INSERT

							ListExprNode *l = (ListExprNode*) node;

							Operand op;

							op = (Operand) { .type = OPTP_INT, .as_int = l->itemc };
							if(!ExeBuilder_Append(exeb, error, OPCODE_PUSHLST, &op, 1, node->offset, node->length))
								return 0;

							Node *item = l->items;
							int i = 0;

							while(item)
								{
									op = (Operand) { .type = OPTP_INT, .as_int = i };
									if(!ExeBuilder_Append(exeb, error, OPCODE_PUSHINT, &op, 1, item->offset, item->length))
										return 0;

									if(!emit_instr_for_node(exeb, item, error))
										return 0;

									if(!ExeBuilder_Append(exeb, error, OPCODE_INSERT, NULL, 0, item->offset, item->length))
										return 0;
										
									i += 1;
									item = item->next;
								}
							return 1;
						}

						case EXPR_MAP:
						{
							MapExprNode *m = (MapExprNode*) node;

							Operand op;

							op = (Operand) { .type = OPTP_INT, .as_int = m->itemc };
							if(!ExeBuilder_Append(exeb, error, OPCODE_PUSHMAP, &op, 1, node->offset, node->length))
								return 0;

							Node *key  = m->keys;
							Node *item = m->items;
								
							while(item)
								{
									if(!emit_instr_for_node(exeb, key, error))
										return 0;

									if(!emit_instr_for_node(exeb, item, error))
										return 0;

									if(!ExeBuilder_Append(exeb, error, OPCODE_INSERT, NULL, 0, item->offset, item->length))
										return 0;
									
									key  =  key->next;
									item = item->next;
								}

							return 1;
						}

						case EXPR_CALL:
						{
							CallExprNode *p = (CallExprNode*) expr;

							Node *arg = p->argv;

							while(arg)
								{
									if(!emit_instr_for_node(exeb, arg, error))
										return 0;

									arg = arg->next;
								}

							if(!emit_instr_for_node(exeb, p->func, error))
								return 0;

							Operand op = { .type = OPTP_INT, .as_int = p->argc };
							return ExeBuilder_Append(exeb, error, OPCODE_CALL, &op, 1, node->offset, node->length);
						}

						case EXPR_SELECT:
						{
							IndexSelectionExprNode *sel = (IndexSelectionExprNode*) expr;
					
							if(!emit_instr_for_node(exeb, sel->set, error))
								return 0;

							if(!emit_instr_for_node(exeb, sel->idx, error))
								return 0;

							return ExeBuilder_Append(exeb, error, OPCODE_SELECT, NULL, 0, node->offset, node->length);
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

			case NODE_WHILE:
			{
				WhileNode *whl = (WhileNode*) node;

				/* 
				 * start:
				 *   <condition>
				 * 	 JUMPIFNOTANDPOP end
				 *   <body>
				 *   JUMP start
				 * end:
				 */

				Promise *start_offset = Promise_New(ExeBuilder_GetAlloc(exeb), sizeof(long long int));
				Promise   *end_offset = Promise_New(ExeBuilder_GetAlloc(exeb), sizeof(long long int));

				if(start_offset == NULL || end_offset == NULL)
					{
						Error_Report(error, 1, "No memory");
						return 0;
					}

				long long int temp = ExeBuilder_InstrCount(exeb);
				Promise_Resolve(start_offset, &temp, sizeof(temp));

				if(!emit_instr_for_node(exeb, whl->condition, error))
					return 0;

				Operand op = { .type = OPTP_PROMISE, .as_promise = end_offset };
				if(!ExeBuilder_Append(exeb, error, OPCODE_JUMPIFNOTANDPOP, &op, 1, whl->condition->offset, whl->condition->length))
					return 0;

				if(!emit_instr_for_node(exeb, whl->body, error))
					return 0;

				if(whl->body->kind == NODE_EXPR)
					{
						Operand op = (Operand) { .type = OPTP_INT, .as_int = 1 };
						if(!ExeBuilder_Append(exeb, error, OPCODE_POP, &op, 1, whl->body->offset, 0))
							return 0;
					}
				
				op = (Operand) { .type = OPTP_PROMISE, .as_promise = start_offset };
				if(!ExeBuilder_Append(exeb, error, OPCODE_JUMP, &op, 1, node->offset, node->length))
					return 0;

				temp = ExeBuilder_InstrCount(exeb);
				Promise_Resolve(end_offset, &temp, sizeof(temp));

				Promise_Free(start_offset);
				Promise_Free(  end_offset);
				return 1;
			}

			case NODE_DOWHILE:
			{
				DoWhileNode *dowhl = (DoWhileNode*) node;

				/*
				 * start:
				 *   <body>
				 *   <condition>
				 *   JUMPIFANDPOP start
				 */

				long long int start = ExeBuilder_InstrCount(exeb);

				if(!emit_instr_for_node(exeb, dowhl->body, error))
					return 0;

				if(dowhl->body->kind == NODE_EXPR)
					{
						Operand op = (Operand) { .type = OPTP_INT, .as_int = 1 };
						if(!ExeBuilder_Append(exeb, error, OPCODE_POP, &op, 1, dowhl->body->offset, 0))
							return 0;
					}

				if(!emit_instr_for_node(exeb, dowhl->condition, error))
					return 0;

				Operand op = { .type = OPTP_INT, .as_int = start };
				if(!ExeBuilder_Append(exeb, error, OPCODE_JUMPIFANDPOP, &op, 1, dowhl->condition->offset, dowhl->condition->length))
					return 0;
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

			case NODE_RETURN:
			{
				ReturnNode *ret = (ReturnNode*) node;

				if(!emit_instr_for_node(exeb, ret->val, error))
					return 0;

				if(!ExeBuilder_Append(exeb, error, OPCODE_RETURN, NULL, 0, ret->base.offset, ret->base.length))
					return 0;

				return 1;
			}

			case NODE_FUNC:
			{
				FunctionNode *func = (FunctionNode*) node;

				Promise *func_index = Promise_New(ExeBuilder_GetAlloc(exeb), sizeof(long long int));
				Promise *jump_index = Promise_New(ExeBuilder_GetAlloc(exeb), sizeof(long long int));

				if(func_index == NULL || jump_index == NULL)
					{
						Error_Report(error, 1, "No memory");
						return 0;
					}

				// Push function.
				{
					Operand ops[2] = {
						{ .type = OPTP_PROMISE, .as_promise = func_index },
						{ .type = OPTP_INT,     .as_int     = func->argc },
					};

					if(!ExeBuilder_Append(exeb, error, OPCODE_PUSHFUN, ops, 2, func->base.offset, func->base.length))
						return 0;
				}
				
				// Assign variable.
				Operand op = (Operand) { .type = OPTP_STRING, .as_string = func->name };
				if(!ExeBuilder_Append(exeb, error, OPCODE_ASS, &op, 1,  func->base.offset, func->base.length))
					return 0;

				// Pop function object.
				op = (Operand) { .type = OPTP_INT, .as_int = 1 };
				if(!ExeBuilder_Append(exeb, error, OPCODE_POP, &op, 1,  func->base.offset, func->base.length))
					return 0;

				// Jump after the function code.
				op = (Operand) { .type = OPTP_PROMISE, .as_promise = jump_index };
				if(!ExeBuilder_Append(exeb, error, OPCODE_JUMP, &op, 1,  func->base.offset, func->base.length))
					return 0;

				// This is the function code index.
				long long int temp = ExeBuilder_InstrCount(exeb);
				Promise_Resolve(func_index, &temp, sizeof(temp));

				// Compile the function body.
				{
					// Assign the arguments.

					if(func->argv)
						assert(func->argv->kind == NODE_ARG);

					ArgumentNode *arg = (ArgumentNode*) func->argv;

					while(arg)
						{
							op = (Operand) { .type = OPTP_STRING, .as_string = arg->name };
							if(!ExeBuilder_Append(exeb, error, OPCODE_ASS, &op, 1,  arg->base.offset, arg->base.length))
								return 0;

							op = (Operand) { .type = OPTP_INT, .as_int = 1 };
							if(!ExeBuilder_Append(exeb, error, OPCODE_POP, &op, 1,  arg->base.offset, arg->base.length))
								return 0;

							if(arg->base.next)
								assert(arg->base.next->kind == NODE_ARG);

							arg = (ArgumentNode*) arg->base.next;
						}

					if(!emit_instr_for_node(exeb, func->body, error))
						return 0;

					// Write a return instruction, just 
					// in case it didn't already return.
					if(!ExeBuilder_Append(exeb, error, OPCODE_RETURN, NULL, 0, func->body->offset, 0))
						return 0;
				}

				// This is the first index after the function code.
				temp = ExeBuilder_InstrCount(exeb);
				Promise_Resolve(jump_index, &temp, sizeof(temp));

				Promise_Free(func_index);
				Promise_Free(jump_index);
				return 1;
			}

			default:
			UNREACHABLE;
			return 0;
		}
	UNREACHABLE;
	return 0;
}

/* Symbol: compile
 * 
 *   Serializes an AST into bytecode format.
 *
 *
 * Arguments:
 *
 *   ast: The AST to be serialized.
 *   alloc: The allocator that will be used to get new
 *			memory. (optional)
 *   error: Error information structure that is filled out if
 *          an error occurres.
 *
 *
 * Returns:
 *   A pointer to an `Executable` that is the object that
 *	 contains the bytecode. If an error occurres, NULL is 
 *   returned and the `error` structure is filled out.
 *
 */
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

	if(exeb != NULL)
		{
			if(!emit_instr_for_node(exeb, ast->root, error))
				return 0;

			if(ExeBuilder_Append(exeb, error, OPCODE_RETURN, NULL, 0, Source_GetSize(ast->src), 0))
				{
					exe = ExeBuilder_Finalize(exeb, error);
					
					if(exe != NULL)
						Executable_SetSource(exe, ast->src);
				}
		}

	if(alloc == NULL)
		BPAlloc_Free(alloc2);
	return exe;
}