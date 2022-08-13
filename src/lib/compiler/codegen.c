
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
** | The function that does the heavy lifting is `emitInstrForNode` which  |
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
#include <stdbool.h>
#include "../utils/defs.h"
#include "compile.h"
#include "ASTi.h"

typedef struct {
	Error *error;
	BPAlloc *alloc;
	ExeBuilder *builder;
	bool own_alloc;
	jmp_buf env;
} CodegenContext;

typedef Promise Label;

static void setLabel(Label *label, long long int value)
{
	Promise *promise = (Promise*) label;
	Promise_Resolve(promise, &value, sizeof(value));
}

static void setLabelHere(Label *label, CodegenContext *ctx)
{
	long long int value = ExeBuilder_InstrCount(ctx->builder);
	setLabel(label, value);
}

static void freeLabel(Label *label)
{
	Promise *promise = (Promise*) label;
	Promise_Free(promise);
}

static void okNowJump(CodegenContext *ctx)
{
	longjmp(ctx->env, 1);
}

static void reportErrorAndJump_(CodegenContext *ctx, const char *file, 
	                            const char *func, int line, bool internal, 
	                            const char *format, ...)
{
	va_list args;
	va_start(args, format);
	_Error_Report2(ctx->error, internal, file, func, line, format, args);
	va_end(args);

	okNowJump(ctx);
}

#define reportErrorAndJump(ctx, int, fmt, ...) \
	reportErrorAndJump_(ctx, __FILE__, __func__, __LINE__, int, fmt, ## __VA_ARGS__) 

static void freeCodegenContext(CodegenContext *ctx)
{
	if(ctx->own_alloc)
		BPAlloc_Free(ctx->alloc);
}

static bool setOrcatchJump(CodegenContext *ctx)
{
	bool jumped = setjmp(ctx->env);
	return jumped;
}

static bool initCodegenContext(CodegenContext *ctx, Error *error, BPAlloc *alloc)
{
	if(alloc == NULL) {
		alloc = BPAlloc_Init(-1);
		if(alloc == NULL) {
			Error_Report(error, 1, "No memory");
			return false;
		}
		ctx->own_alloc = true;
	} else {
		ctx->own_alloc = false;
	}

	ExeBuilder *builder = ExeBuilder_New(alloc);
	if(builder == NULL) {
		if(ctx->own_alloc)
			BPAlloc_Free(alloc);
		return false;
	}

	ctx->error = error;
	ctx->alloc = alloc;
	ctx->builder = builder;
	return true;
}

static void emitInstr(CodegenContext *ctx, Opcode opcode, Operand *opv, int opc, int off, int len)
{
	if(!ExeBuilder_Append(ctx->builder, ctx->error, opcode, opv, opc, off, len))
		okNowJump(ctx);
}

static void emitInstr_POP(CodegenContext *ctx, 
	                      long long int op0,
	                      int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_INT, .as_int = op0 }
	};
	emitInstr(ctx, OPCODE_POP, opv, 1, off, len);
}

static void emitInstr_POP1(CodegenContext *ctx, int off, int len)
{
	emitInstr_POP(ctx, 1, off, len);
}

static void emitInstr_ASS(CodegenContext *ctx, const char *name, int off, int len)
{
	Operand opv[] = {
		{ .type = OPTP_STRING, .as_string = name },
	};
	emitInstr(ctx, OPCODE_ASS, opv, 1, off, len);
}

static void emitInstr_RETURN(CodegenContext *ctx, 
	                         long long int op0,
	                         int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_INT, .as_int = op0 }
	};
	emitInstr(ctx, OPCODE_RETURN, opv, 1, off, len);
}

static void emitInstr_JUMP(CodegenContext *ctx, 
	                         Promise *op0,
	                         int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_PROMISE, .as_promise = op0 }
	};
	emitInstr(ctx, OPCODE_JUMP, opv, 1, off, len);
}

static void emitInstr_JUMPIFNOTANDPOP(CodegenContext *ctx, 
	                                  Promise *op0,
	                                  int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_PROMISE, .as_promise = op0 }
	};
	emitInstr(ctx, OPCODE_JUMPIFNOTANDPOP, opv, 1, off, len);
}

static void emitInstr_JUMPIFANDPOP(CodegenContext *ctx, 
	                               long long int op0,
	                               int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_INT, .as_int = op0 }
	};
	emitInstr(ctx, OPCODE_JUMPIFANDPOP, opv, 1, off, len);
}

static Label *newLabel(CodegenContext *ctx)
{
	Promise *promise = Promise_New(ctx->alloc, sizeof(long long int));
	if(promise != NULL)
		return promise;
	
	reportErrorAndJump(ctx, 1, "No memory");
	UNREACHABLE;
	return NULL; // For the compiler warning.
}

static void emitInstrForNode(CodegenContext *ctx, Node *node, Label *label_break);

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
	UNREACHABLE;
	return -1;
}

static void emitInstrForFuncCallNode(CodegenContext *ctx, CallExprNode *expr, 
	                                 Label *label_break, int returns)
{
	Node *arg = expr->argv;
    
	while(arg)
	{
		emitInstrForNode(ctx, arg, label_break);
		arg = arg->next;
	}

	emitInstrForNode(ctx, expr->func, label_break);

	Operand ops[2];
	ops[0] = (Operand) { .type = OPTP_INT, .as_int = expr->argc };
	ops[1] = (Operand) { .type = OPTP_INT, .as_int = returns };
	emitInstr(ctx, OPCODE_CALL, ops, 2, expr->base.base.offset, expr->base.base.length);
}

static void emitInstrForFuncNode(CodegenContext *ctx, FunctionNode *func)
{
	Label *label_func = newLabel(ctx);
	Label *label_jump = newLabel(ctx);

	// Push function.
	{
		Operand ops[2] = {
			{ .type = OPTP_PROMISE, .as_promise = label_func },
			{ .type = OPTP_INT,     .as_int     = func->argc },
		};
		emitInstr(ctx, OPCODE_PUSHFUN, ops, 2, func->base.offset, func->base.length);
	}
	emitInstr_ASS(ctx, func->name, func->base.offset, func->base.length); // Assign variable
	emitInstr_POP1(ctx, func->base.offset, func->base.length); // Pop function object
	emitInstr_JUMP(ctx, label_jump, func->base.offset, func->base.length); // Jump after the function code
	setLabelHere(label_func, ctx); // This is the function code index.

	// Compile the function body.
	{
		// Assign the arguments.
		ArgumentNode *arg = (ArgumentNode*) func->argv;
		while(arg)
		{
			emitInstr_ASS(ctx, arg->name, arg->base.offset, arg->base.length);
			emitInstr_POP1(ctx, arg->base.offset, arg->base.length);
			arg = (ArgumentNode*) arg->base.next;
		}

		emitInstrForNode(ctx, func->body, NULL);

		if(func->body->kind == NODE_EXPR)
			emitInstr_POP1(ctx, func->body->offset + func->body->length, 0);

		// Write a return instruction, just 
		// in case it didn't already return.
		emitInstr_RETURN(ctx, 0, func->body->offset, 0);
	}

	// This is the first index after the function code.
	setLabelHere(label_jump, ctx);

	freeLabel(label_func);
	freeLabel(label_jump);
}

static void emitInstrForIfElseNode(CodegenContext *ctx, IfElseNode *ifelse, Label *label_break)
{
	emitInstrForNode(ctx, ifelse->condition, label_break);
	if(ifelse->false_branch)
	{
		Label *label_else = newLabel(ctx);
		Label *label_done = newLabel(ctx);
		emitInstr_JUMPIFNOTANDPOP(ctx, label_else, ifelse->base.offset, ifelse->base.length);
		emitInstrForNode(ctx, ifelse->true_branch, label_break);
		if(ifelse->true_branch->kind == NODE_EXPR)
			emitInstr_POP(ctx, 1, ifelse->true_branch->offset, 0);
		emitInstr_JUMP(ctx, label_done, ifelse->base.offset, ifelse->base.length);
		setLabelHere(label_else, ctx);
		emitInstrForNode(ctx, ifelse->false_branch, label_break);
		if(ifelse->false_branch->kind == NODE_EXPR)
			emitInstr_POP1(ctx, ifelse->false_branch->offset, 0);
		setLabelHere(label_done, ctx);
		freeLabel(label_else);
		freeLabel(label_done);
	}
	else
	{
		Label *label_done = newLabel(ctx);
		emitInstr_JUMPIFNOTANDPOP(ctx, label_done, ifelse->base.offset, ifelse->base.length);
		emitInstrForNode(ctx, ifelse->true_branch, label_break);
		if(ifelse->true_branch->kind == NODE_EXPR)
			emitInstr_POP1(ctx, ifelse->true_branch->offset, 0);
		setLabelHere(label_done, ctx);
		freeLabel(label_done);
	}
}

static void flattenTupleTree(CodegenContext *ctx, ExprNode *root, ExprNode *tuple[], int max, int *count)
{
	if(root->kind == EXPR_PAIR)
	{
		flattenTupleTree(ctx, (ExprNode*) ((OperExprNode*) root)->head, tuple, max, count);
		flattenTupleTree(ctx, (ExprNode*) ((OperExprNode*) root)->head->next, tuple, max, count);
	} 
	else 
	{

		if(max == *count)
		{
			reportErrorAndJump(ctx, 0, "Static buffer is too small");
			UNREACHABLE;
		}

		tuple[(*count)++] = root;
	}
}

static void emitInstrForAssignmentNode(CodegenContext *ctx, OperExprNode *asgn, Label *label_break)
{
	Node *lop, *rop;
	lop = asgn->head;
	rop = lop->next;

	ExprNode *tuple[32];
	int count = 0;

	flattenTupleTree(ctx, (ExprNode*) lop, tuple, sizeof(tuple)/sizeof(tuple[0]), &count);

	assert(count > 0);

	if(count == 1) /* No tuple. */
		emitInstrForNode(ctx, rop, label_break);
	else
	{
		if(((ExprNode*) rop)->kind == EXPR_CALL)
			emitInstrForFuncCallNode(ctx, (CallExprNode*) rop, label_break, count);
		else {
			reportErrorAndJump(ctx, 0, "Assigning to %d variables only 1 value", count);
			UNREACHABLE;
		}
	}
	
	for(int i = 0; i < count; i += 1)
	{
		ExprNode *tuple_item = tuple[count-i-1];
		switch(tuple_item->kind)
		{
			case EXPR_IDENT:
			{
				const char *name = ((IdentExprNode*) tuple_item)->val;
				emitInstr_ASS(ctx, name, tuple_item->base.offset, tuple_item->base.length);
				break;
			}

			case EXPR_SELECT:
			{
				Node *idx = ((IndexSelectionExprNode*) tuple_item)->idx;
				Node *set = ((IndexSelectionExprNode*) tuple_item)->set;
				emitInstrForNode(ctx, set, label_break);
				emitInstrForNode(ctx, idx, label_break);
				emitInstr(ctx, OPCODE_INSERT2, NULL, 0, tuple_item->base.offset, tuple_item->base.length);
				break;
			}

			default:
			reportErrorAndJump(ctx, 0, "Assigning to something that it can't be assigned to");
			UNREACHABLE;
		}

		if(i+1 < count)
			emitInstr_POP1(ctx, asgn->base.base.offset, 0);
	}
}

static void emitInstrForNode(CodegenContext *ctx, Node *node, Label *label_break)
{
	assert(node != NULL);

	switch(node->kind)
	{
		case NODE_EXPR:
		{
			ExprNode *expr = (ExprNode*) node;
			switch(expr->kind)
			{
				case EXPR_PAIR:
				reportErrorAndJump(ctx, 0, "Tuple outside of assignment or return statement");
				UNREACHABLE;
				return; // For the compiler warning.

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
						emitInstrForNode(ctx, operand, label_break);
					emitInstr(ctx, exprkind_to_opcode(expr->kind), NULL, 0, node->offset, node->length);
					return;
				}

				case EXPR_ASS:
				emitInstrForAssignmentNode(ctx, (OperExprNode*) expr, label_break);
				return;

				case EXPR_INT:
				{
					IntExprNode *p = (IntExprNode*) expr;
					Operand op = { .type = OPTP_INT, .as_int = p->val };
					emitInstr(ctx, OPCODE_PUSHINT, &op, 1, node->offset, node->length);
					return;
				}

				case EXPR_FLOAT:
				{
					FloatExprNode *p = (FloatExprNode*) expr;
					Operand op = { .type = OPTP_FLOAT, .as_float = p->val };
					emitInstr(ctx, OPCODE_PUSHFLT, &op, 1, node->offset, node->length);
					return;
				}

				case EXPR_STRING:
				{
					StringExprNode *p = (StringExprNode*) expr;
					Operand op = { .type = OPTP_STRING, .as_string = p->val };
					emitInstr(ctx, OPCODE_PUSHSTR, &op, 1, node->offset, node->length);
					return;
				}

				case EXPR_IDENT:
				{
					IdentExprNode *p = (IdentExprNode*) expr;
					Operand op = { .type = OPTP_STRING, .as_string = p->val };
					emitInstr(ctx, OPCODE_PUSHVAR, &op, 1, node->offset, node->length);
					return;
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
					emitInstr(ctx, OPCODE_PUSHLST, &op, 1, node->offset, node->length);

					Node *item = l->items;
					int i = 0;

					while(item)
					{
						op = (Operand) { .type = OPTP_INT, .as_int = i };
						emitInstr(ctx, OPCODE_PUSHINT, &op, 1, item->offset, item->length);
						emitInstrForNode(ctx, item, label_break);
						emitInstr(ctx, OPCODE_INSERT, NULL, 0, item->offset, item->length);
						i += 1;
						item = item->next;
					}
					return;
				}

				case EXPR_MAP:
				{
					MapExprNode *m = (MapExprNode*) node;

					Operand op;

					op = (Operand) { .type = OPTP_INT, .as_int = m->itemc };
					emitInstr(ctx, OPCODE_PUSHMAP, &op, 1, node->offset, node->length);

					Node *key  = m->keys;
					Node *item = m->items;
								
					while(item)
					{
						emitInstrForNode(ctx, key, label_break);
						emitInstrForNode(ctx, item, label_break);
						emitInstr(ctx, OPCODE_INSERT, NULL, 0, item->offset, item->length);
						key  =  key->next;
						item = item->next;
					}
					return;
				}

				case EXPR_CALL:
				emitInstrForFuncCallNode(ctx, (CallExprNode*) expr, label_break, 1);
				return;

				case EXPR_SELECT:
				{
					IndexSelectionExprNode *sel = (IndexSelectionExprNode*) expr;
					emitInstrForNode(ctx, sel->set, label_break);
					emitInstrForNode(ctx, sel->idx, label_break);
					emitInstr(ctx, OPCODE_SELECT, NULL, 0, node->offset, node->length);
					return;
				}

				case EXPR_NONE:
				emitInstr(ctx, OPCODE_PUSHNNE, NULL, 0, node->offset, node->length);
				return;

				case EXPR_TRUE:
				emitInstr(ctx, OPCODE_PUSHTRU, NULL, 0, node->offset, node->length);
				return;

				case EXPR_FALSE:
				emitInstr(ctx, OPCODE_PUSHFLS, NULL, 0, node->offset, node->length);
				return;

				default:
				UNREACHABLE;
				break;
			}
			return;
		}

		case NODE_BREAK:
		if(label_break == NULL)
			reportErrorAndJump(ctx, 0, "Break not inside a loop");
		emitInstr_JUMP(ctx, label_break, node->offset, node->length);
		return;

		case NODE_IFELSE:
		emitInstrForIfElseNode(ctx, (IfElseNode*) node, label_break);
		return;

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

			Label *label_start = newLabel(ctx);
			Label *label_end = newLabel(ctx);
			setLabelHere(label_start, ctx);
			emitInstrForNode(ctx, whl->condition, label_break);
			emitInstr_JUMPIFNOTANDPOP(ctx, label_end, whl->condition->offset, whl->condition->length);
			emitInstrForNode(ctx, whl->body, label_end);
			if(whl->body->kind == NODE_EXPR)
				emitInstr_POP1(ctx, whl->body->offset, 0);			
			emitInstr_JUMP(ctx, label_start, node->offset, node->length);
			setLabelHere(label_end, ctx);
			freeLabel(label_start);
			freeLabel(label_end);
			return;
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

			Label *label_end = newLabel(ctx);
			long long int start = ExeBuilder_InstrCount(ctx->builder);
			emitInstrForNode(ctx, dowhl->body, label_end);
			if(dowhl->body->kind == NODE_EXPR)
				emitInstr_POP1(ctx, dowhl->body->offset, 0);
			emitInstrForNode(ctx, dowhl->condition, label_break);
			emitInstr_JUMPIFANDPOP(ctx, start, dowhl->condition->offset, dowhl->condition->length);
			setLabelHere(label_end, ctx);
			freeLabel(label_end);
			return;
		}

		case NODE_COMP:
		{
			CompoundNode *comp = (CompoundNode*) node;

			Node *stmt = comp->head;

			while(stmt)
			{
				emitInstrForNode(ctx, stmt, label_break);
				if(stmt->kind == NODE_EXPR)
					emitInstr_POP1(ctx, stmt->offset, 0);
				stmt = stmt->next;
			}
			return;
		}

		case NODE_RETURN:
		{
			ReturnNode *ret = (ReturnNode*) node;

			ExprNode *tuple[32];
			int count = 0;

			flattenTupleTree(ctx, (ExprNode*) ret->val, tuple, sizeof(tuple)/sizeof(tuple[0]), &count);

			for(int i = 0; i < count; i += 1)
				emitInstrForNode(ctx, (Node*) tuple[i], label_break);
			emitInstr_RETURN(ctx, count, ret->base.offset, ret->base.length);
			return;
		}

		case NODE_FUNC:
		emitInstrForFuncNode(ctx, (FunctionNode*) node);
		return;

		default:
		UNREACHABLE;
	}
	UNREACHABLE;
}

static Executable *makeExecutableAndFreeCodegenContext(CodegenContext *ctx, Source *src)
{
	Executable *exe = ExeBuilder_Finalize(ctx->builder, ctx->error);
	if(exe == NULL)
		okNowJump(ctx);

	Executable_SetSource(exe, src);
	freeCodegenContext(ctx);
	return exe;
}

/* Symbol: codegen
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
Executable *codegen(AST *ast, BPAlloc *alloc, Error *error)
{
	assert(ast != NULL);
	assert(error != NULL);

	CodegenContext ctx;
	if(!initCodegenContext(&ctx, error, alloc))
		return NULL;

	bool jumped = setOrcatchJump(&ctx);
	if(jumped) {
		freeCodegenContext(&ctx);
		return NULL;
	}

	emitInstrForNode(&ctx, ast->root, NULL);
	emitInstr_RETURN(&ctx, 0, Source_GetSize(ast->src), 0);
	return makeExecutableAndFreeCodegenContext(&ctx, ast->src);
}