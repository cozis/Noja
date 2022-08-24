
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
#include <stdlib.h>
#include <stdbool.h>
#include "../utils/defs.h"
#include "codegenctx.h"
#include "compile.h"
#include "ASTi.h"

static void emitInstr_POP(CodegenContext *ctx, 
	                      long long int op0,
	                      int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_INT, .as_int = op0 }
	};
	CodegenContext_EmitInstr(ctx, OPCODE_POP, opv, 1, off, len);
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
	CodegenContext_EmitInstr(ctx, OPCODE_ASS, opv, 1, off, len);
}

static void emitInstr_RETURN(CodegenContext *ctx, 
	                         long long int op0,
	                         int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_INT, .as_int = op0 }
	};
	CodegenContext_EmitInstr(ctx, OPCODE_RETURN, opv, 1, off, len);
}

static void emitInstr_EXIT(CodegenContext *ctx, 
	                       int off, int len)
{
	CodegenContext_EmitInstr(ctx, OPCODE_EXIT, NULL, 0, off, len);
}

static void emitInstr_JUMP(CodegenContext *ctx, 
	                         Label *op0,
	                         int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_PROMISE, .as_promise = Label_ToPromise(op0) }
	};
	CodegenContext_EmitInstr(ctx, OPCODE_JUMP, opv, 1, off, len);
}

static void emitInstr_JUMPIFNOTANDPOP(CodegenContext *ctx, 
	                                  Label *op0,
	                                  int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_PROMISE, .as_promise = Label_ToPromise(op0) }
	};
	CodegenContext_EmitInstr(ctx, OPCODE_JUMPIFNOTANDPOP, opv, 1, off, len);
}

static void emitInstr_JUMPIFANDPOP(CodegenContext *ctx, 
	                               long long int op0,
	                               int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_IDX, .as_int = op0 }
	};
	CodegenContext_EmitInstr(ctx, OPCODE_JUMPIFANDPOP, opv, 1, off, len);
}

static void emitInstr_JUMPIFANDPOP_2(CodegenContext *ctx, 
	                                 Label *op0,
	                                 int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_PROMISE, .as_promise = Label_ToPromise(op0) }
	};
	CodegenContext_EmitInstr(ctx, OPCODE_JUMPIFANDPOP, opv, 1, off, len);
}

static void emitInstr_EQL(CodegenContext *ctx, int off, int len)
{
	CodegenContext_EmitInstr(ctx, OPCODE_EQL, NULL, 0, off, len);
}

static void emitInstr_ERROR(CodegenContext *ctx, const char *msg, int off, int len)
{
	Operand opv[1] = {
		{ .type = OPTP_STRING, .as_string = msg },
	};
	CodegenContext_EmitInstr(ctx, OPCODE_ERROR, opv, 1, off, len);
}

static void emitInstr_PUSHTRU(CodegenContext *ctx, int off, int len)
{
	CodegenContext_EmitInstr(ctx, OPCODE_PUSHTRU, NULL, 0, off, len);
}

static void emitInstr_PUSHFLS(CodegenContext *ctx, int off, int len)
{
	CodegenContext_EmitInstr(ctx, OPCODE_PUSHFLS, NULL, 0, off, len);
}

static void emitInstr_PUSHTYP(CodegenContext *ctx, int off, int len)
{
	CodegenContext_EmitInstr(ctx, OPCODE_PUSHTYP, NULL, 0, off, len);
}

static void emitInstr_PUSHTYPTYP(CodegenContext *ctx, int off, int len)
{
	CodegenContext_EmitInstr(ctx, OPCODE_PUSHTYPTYP, NULL, 0, off, len);
}

static void emitInstr_PUSHNNETYP(CodegenContext *ctx, int off, int len)
{
	CodegenContext_EmitInstr(ctx, OPCODE_PUSHNNETYP, NULL, 0, off, len);
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
	CodegenContext_EmitInstr(ctx, OPCODE_CALL, ops, 2, expr->base.base.offset, expr->base.base.length);
}

static void emitInstrForArgumentNode(CodegenContext *ctx, ArgumentNode *arg, int argidx)
{
	/*
	 *   // If a default value for the argument was specified
	 *   PUSHTYP;
	 *   PUSHNNETYP;
	 *   EQL;
	 *   JUMPIFNOTANDPOP default_handled;
	 *   POP 1;
	 *   <default-value>
	 * default_handled:
	 *   // Push the type of the argument
	 *   PUSHTYP;
	 *   
	 *   // Evaluate the type annotation and make
	 *   // sure that it evaluated to a type.
	 *   <arg-type-0>
	 *   PUSHTYP;
	 *   PUSHTYPTYP;
	 *   EQL;
	 *   JUMPIFANDPOP argument_annotation_0_not_type;
     *
	 *   // If the annotated type and the argument's
	 *   // type match, jump to the argument assignment.
	 *   EQL;
	 *   JUMPIFANDPOP argument_type_ok;
	 *
	 *   // To the same for the next annotation.
	 *
	 *   PUSHTYP;
	 *   <arg-type-1>
	 *   PUSHTYP;
	 *   PUSHTYPTYP;
	 *   EQL;
	 *   JUMPIFANDPOP argument_annotation_1_not_type;
	 *   EQL;
	 *   JUMPIFANDPOP argument_type_ok;
	 *
	 *   PUSHTYP;
	 *   <arg-type-2>
	 *   PUSHTYP;
	 *   PUSHTYPTYP;
	 *   EQL;
	 *   JUMPIFANDPOP argument_annotation_2_not_type;
	 *   EQL;
	 *   JUMPIFANDPOP argument_type_ok;
	 *
	 *   ERROR "Bad type of argument N";
	 * argument_annotation_0_not_a_type:
	 *   ERROR "Argument N annotation M isn't a type";
	 * argument_type_ok:
	 *
	 *   // At this point we know that the argument has
	 *   // a valid type.
	 *   ASS <arg-name>;
	 *   POP 1;
	 */

	if(arg->value != NULL) {
		/* Emit bytecode for the default argument */
		Label *label_default_handled = Label_New(ctx);
		emitInstr_PUSHTYP(ctx, arg->value->offset, arg->value->length);
		emitInstr_PUSHNNETYP(ctx, arg->value->offset, arg->value->length);
		emitInstr_EQL(ctx, arg->value->offset, arg->value->length);
		emitInstr_JUMPIFNOTANDPOP(ctx, label_default_handled, arg->value->offset, arg->value->length);
		emitInstr_POP1(ctx, arg->value->offset, arg->value->length);
		emitInstrForNode(ctx, arg->value, NULL);
		Label_SetHere(label_default_handled, ctx);
		Label_Free(label_default_handled);
	}

	if(arg->typev != NULL) {

		/* Emit checks for the argument type */

		Node *typev = arg->typev;
		int   typec = arg->typec;
		assert(typec > 0);

		Label *maybe[8];
		Label **label_annotation_not_a_type;
		if((size_t) typec > sizeof(maybe)/sizeof(maybe[0])) {
			label_annotation_not_a_type = malloc(typec * sizeof(Label*));
			if(label_annotation_not_a_type == NULL)
				CodegenContext_ReportErrorAndJump(ctx, 1, "No memory");
		} else
			label_annotation_not_a_type = maybe;

		for(int i = 0; i < typec; i += 1)
			label_annotation_not_a_type[i] = Label_New(ctx);

		Label *label_argument_type_ok = Label_New(ctx);

		Node *type = typev;
		for(int i = 0; i < typec; i += 1) {
			assert(type != NULL);
			emitInstr_PUSHTYP(ctx, arg->base.offset, arg->base.length); // The source slice should refer only to the name label, not the whole operand
			emitInstrForNode(ctx, type, NULL);
			{
				emitInstr_PUSHTYP(ctx, type->offset, type->length);
				emitInstr_PUSHTYPTYP(ctx, type->offset, type->length);
				emitInstr_EQL(ctx, type->offset, type->length);
				emitInstr_JUMPIFNOTANDPOP(ctx, label_annotation_not_a_type[i], type->offset, type->length);
			}
			emitInstr_EQL(ctx, type->offset, type->length);
			emitInstr_JUMPIFANDPOP_2(ctx, label_argument_type_ok, type->offset, type->length);
			type = type->next;
		}

		char msg[256];
		snprintf(msg, sizeof(msg), "Bad type for argument %d", argidx);
		emitInstr_ERROR(ctx, msg, arg->base.offset, arg->base.length);
		for(int i = 0; i < typec; i += 1) {
			Label_SetHere(label_annotation_not_a_type[i], ctx);
			snprintf(msg, sizeof(msg), "Argument %d type annotation %d is not a type", argidx, i);
			emitInstr_ERROR(ctx, msg, arg->base.offset, arg->base.length);
		}

		Label_SetHere(label_argument_type_ok, ctx);
		
		Label_Free(label_argument_type_ok);
		for(int i = 0; i < typec; i += 1)
			Label_Free(label_annotation_not_a_type[i]);
		if(label_annotation_not_a_type != maybe)
			free(label_annotation_not_a_type);
	}
	emitInstr_ASS(ctx, arg->name, arg->base.offset, arg->base.length);
	emitInstr_POP1(ctx, arg->base.offset, arg->base.length);
}

static void emitInstrForFuncNode(CodegenContext *ctx, FunctionNode *func)
{
	Label *label_func = Label_New(ctx);
	Label *label_jump = Label_New(ctx);

	// Push function.
	{
		Operand ops[2] = {
			{ .type = OPTP_PROMISE, .as_promise = Label_ToPromise(label_func) },
			{ .type = OPTP_INT,     .as_int     = func->argc },
		};
		CodegenContext_EmitInstr(ctx, OPCODE_PUSHFUN, ops, 2, func->base.offset, func->base.length);
	}
	emitInstr_ASS(ctx, func->name, func->base.offset, func->base.length); // Assign variable
	emitInstr_POP1(ctx, func->base.offset, func->base.length); // Pop function object
	emitInstr_JUMP(ctx, label_jump, func->base.offset, func->base.length); // Jump after the function code
	Label_SetHere(label_func, ctx); // This is the function code index.

	// Compile the function body.
	{
		// Assign the arguments.
		ArgumentNode *arg = (ArgumentNode*) func->argv;
		int argidx = func->argc-1;
		while(arg)
		{
			emitInstrForArgumentNode(ctx, arg, argidx);
			arg = (ArgumentNode*) arg->base.next;
			argidx -= 1;
		}

		emitInstrForNode(ctx, func->body, NULL);

		if(func->body->kind == NODE_EXPR)
			emitInstr_POP1(ctx, func->body->offset + func->body->length, 0);

		// Write a return instruction, just 
		// in case it didn't already return.
		emitInstr_RETURN(ctx, 0, func->body->offset, 0);
	}

	// This is the first index after the function code.
	Label_SetHere(label_jump, ctx);

	Label_Free(label_func);
	Label_Free(label_jump);
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
			CodegenContext_ReportErrorAndJump(ctx, 0, "Static buffer is too small");
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
			CodegenContext_ReportErrorAndJump(ctx, 0, "Assigning to %d variables only 1 value", count);
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
				CodegenContext_EmitInstr(ctx, OPCODE_INSERT2, NULL, 0, tuple_item->base.offset, tuple_item->base.length);
				break;
			}

			default:
			CodegenContext_ReportErrorAndJump(ctx, 0, "Assigning to something that it can't be assigned to");
			UNREACHABLE;
		}

		if(i+1 < count)
			emitInstr_POP1(ctx, asgn->base.base.offset, 0);
	}
}

static void emitInstrForExprNode(CodegenContext *ctx, ExprNode *expr, 
	                             Label *label_break)
{
	switch(expr->kind)
	{
		case EXPR_PAIR:
		CodegenContext_ReportErrorAndJump(ctx, 0, "Tuple outside of assignment or return statement");
		UNREACHABLE;
		return; // For the compiler warning.

		case EXPR_NOT:
		case EXPR_POS: case EXPR_NEG:
		case EXPR_ADD: case EXPR_SUB:
		case EXPR_MUL: case EXPR_DIV:
		case EXPR_EQL: case EXPR_NQL:
		case EXPR_LSS: case EXPR_LEQ:
		case EXPR_GRT: case EXPR_GEQ:
		{
			OperExprNode *oper = (OperExprNode*) expr;
			for(Node *operand = oper->head; operand; operand = operand->next)
				emitInstrForNode(ctx, operand, label_break);
			CodegenContext_EmitInstr(ctx, exprkind_to_opcode(expr->kind), NULL, 0, expr->base.offset, expr->base.length);
			return;
		}

		case EXPR_AND:
		{
			OperExprNode *oper = (OperExprNode*) expr;
			
			/*
			 *   <left_oper>
			 *   JUMPIFNOTANDPOP false;
			 *   <right_oper>
			 *   JUMPIFNOTANDPOP false;
			 *   PUSHTRU;
			 *   JUMP end;
			 * false:
			 *   PUSHFLS;
			 * end:
			 *
			 */

			Label *label_end   = Label_New(ctx);
			Label *label_false = Label_New(ctx);
			for(Node *operand = oper->head; operand; operand = operand->next) {
				emitInstrForNode(ctx, operand, label_break);
				emitInstr_JUMPIFNOTANDPOP(ctx, label_false, expr->base.offset, expr->base.length);
			}
			emitInstr_PUSHTRU(ctx, expr->base.offset, expr->base.length);
			emitInstr_JUMP(ctx, label_end, expr->base.offset, expr->base.length);
			Label_SetHere(label_false, ctx);
			emitInstr_PUSHFLS(ctx, expr->base.offset, expr->base.length);
			Label_SetHere(label_end, ctx);
			return;
		}

		case EXPR_OR:
		{
			OperExprNode *oper = (OperExprNode*) expr;
			
			/*
			 *   <left_oper>
			 *   JUMPIFANDPOP true;
			 *   <right_oper>
			 *   JUMPIFANDPOP true;
			 *   PUSHFLS;
			 *   JUMP end;
			 * true:
			 *   PUSHTRU;
			 *   JUMP end;
			 * end:
			 *
			 */

			Label *label_end  = Label_New(ctx);
			Label *label_true = Label_New(ctx);
			for(Node *operand = oper->head; operand; operand = operand->next) {
				emitInstrForNode(ctx, operand, label_break);
				emitInstr_JUMPIFANDPOP_2(ctx, label_true, expr->base.offset, expr->base.length);
			}
			emitInstr_PUSHFLS(ctx, expr->base.offset, expr->base.length);
			emitInstr_JUMP(ctx, label_end, expr->base.offset, expr->base.length);
			Label_SetHere(label_true, ctx);
			emitInstr_PUSHTRU(ctx, expr->base.offset, expr->base.length);
			Label_SetHere(label_end, ctx);
			return;
		}

		case EXPR_ASS:
		emitInstrForAssignmentNode(ctx, (OperExprNode*) expr, label_break);
		return;

		case EXPR_INT:
		{
			IntExprNode *p = (IntExprNode*) expr;
			Operand op = { .type = OPTP_INT, .as_int = p->val };
			CodegenContext_EmitInstr(ctx, OPCODE_PUSHINT, &op, 1, expr->base.offset, expr->base.length);
			return;
		}

		case EXPR_FLOAT:
		{
			FloatExprNode *p = (FloatExprNode*) expr;
			Operand op = { .type = OPTP_FLOAT, .as_float = p->val };
			CodegenContext_EmitInstr(ctx, OPCODE_PUSHFLT, &op, 1, expr->base.offset, expr->base.length);
			return;
		}

		case EXPR_STRING:
		{
			StringExprNode *p = (StringExprNode*) expr;
			Operand op = { .type = OPTP_STRING, .as_string = p->val };
			CodegenContext_EmitInstr(ctx, OPCODE_PUSHSTR, &op, 1, expr->base.offset, expr->base.length);
			return;
		}

		case EXPR_IDENT:
		{
			IdentExprNode *p = (IdentExprNode*) expr;
			Operand op = { .type = OPTP_STRING, .as_string = p->val };
			CodegenContext_EmitInstr(ctx, OPCODE_PUSHVAR, &op, 1, expr->base.offset, expr->base.length);
			return;
		}

		case EXPR_LIST:
		{
			// PUSHLST
			// PUSHINT
			// <expr>
			// INSERT

			ListExprNode *l = (ListExprNode*) expr;

			Operand op;
			
			op = (Operand) { .type = OPTP_INT, .as_int = l->itemc };
			CodegenContext_EmitInstr(ctx, OPCODE_PUSHLST, &op, 1, expr->base.offset, expr->base.length);

			Node *item = l->items;
			int i = 0;

			while(item)
			{
				op = (Operand) { .type = OPTP_INT, .as_int = i };
				CodegenContext_EmitInstr(ctx, OPCODE_PUSHINT, &op, 1, item->offset, item->length);
				emitInstrForNode(ctx, item, label_break);
				CodegenContext_EmitInstr(ctx, OPCODE_INSERT, NULL, 0, item->offset, item->length);
				i += 1;
				item = item->next;
			}
			return;
		}

		case EXPR_MAP:
		{
			MapExprNode *m = (MapExprNode*) expr;

			Operand op;

			op = (Operand) { .type = OPTP_INT, .as_int = m->itemc };
			CodegenContext_EmitInstr(ctx, OPCODE_PUSHMAP, &op, 1, expr->base.offset, expr->base.length);

			Node *key  = m->keys;
			Node *item = m->items;
						
			while(item)
			{
				emitInstrForNode(ctx, key, label_break);
				emitInstrForNode(ctx, item, label_break);
				CodegenContext_EmitInstr(ctx, OPCODE_INSERT, NULL, 0, item->offset, item->length);
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
			CodegenContext_EmitInstr(ctx, OPCODE_SELECT, NULL, 0, expr->base.offset, expr->base.length);
			return;
		}

		case EXPR_NONE:  CodegenContext_EmitInstr(ctx, OPCODE_PUSHNNE, NULL, 0, expr->base.offset, expr->base.length); return;
		case EXPR_TRUE:	 CodegenContext_EmitInstr(ctx, OPCODE_PUSHTRU, NULL, 0, expr->base.offset, expr->base.length); return;
		case EXPR_FALSE: CodegenContext_EmitInstr(ctx, OPCODE_PUSHFLS, NULL, 0, expr->base.offset, expr->base.length); return;
		default: UNREACHABLE; break;
	}
}

static void emitInstrForIfElseNode(CodegenContext *ctx, IfElseNode *ifelse, Label *label_break)
{
	emitInstrForNode(ctx, ifelse->condition, label_break);
	if(ifelse->false_branch)
	{
		Label *label_else = Label_New(ctx);
		Label *label_done = Label_New(ctx);
		emitInstr_JUMPIFNOTANDPOP(ctx, label_else, ifelse->condition->offset, ifelse->condition->length);
		emitInstrForNode(ctx, ifelse->true_branch, label_break);
		if(ifelse->true_branch->kind == NODE_EXPR)
			emitInstr_POP(ctx, 1, ifelse->true_branch->offset, 0);
		emitInstr_JUMP(ctx, label_done, ifelse->base.offset, ifelse->base.length);
		Label_SetHere(label_else, ctx);
		emitInstrForNode(ctx, ifelse->false_branch, label_break);
		if(ifelse->false_branch->kind == NODE_EXPR)
			emitInstr_POP1(ctx, ifelse->false_branch->offset, 0);
		Label_SetHere(label_done, ctx);
		Label_Free(label_else);
		Label_Free(label_done);
	}
	else
	{
		Label *label_done = Label_New(ctx);
		emitInstr_JUMPIFNOTANDPOP(ctx, label_done, ifelse->condition->offset, ifelse->condition->length);
		emitInstrForNode(ctx, ifelse->true_branch, label_break);
		if(ifelse->true_branch->kind == NODE_EXPR)
			emitInstr_POP1(ctx, ifelse->true_branch->offset, 0);
		Label_SetHere(label_done, ctx);
		Label_Free(label_done);
	}
}

static void emitInstrForWhileLoopNode(CodegenContext *ctx, WhileNode *loop, Label *label_break)
{
	/* 
	 * start:
	 *   <condition>
	 *   JUMPIFNOTANDPOP end
	 *   <body>
	 *   JUMP start
	 * end:
	 */

	Label *label_start = Label_New(ctx);
	Label *label_end = Label_New(ctx);
	Label_SetHere(label_start, ctx);
	emitInstrForNode(ctx, loop->condition, label_break);
	emitInstr_JUMPIFNOTANDPOP(ctx, label_end, loop->condition->offset, loop->condition->length);
	emitInstrForNode(ctx, loop->body, label_end);
	if(loop->body->kind == NODE_EXPR)
		emitInstr_POP1(ctx, loop->body->offset, 0);			
	emitInstr_JUMP(ctx, label_start, loop->base.offset, loop->base.length);
	Label_SetHere(label_end, ctx);
	Label_Free(label_start);
	Label_Free(label_end);
}

static void emitInstrForDoWhileLoopNode(CodegenContext *ctx, DoWhileNode *loop, Label *label_break)
{
	/*
	 * start:
	 *   <body>
	 *   <condition>
	 *   JUMPIFANDPOP start
	 */

	Label *label_end = Label_New(ctx);
	long long int start = CodegenContext_InstrCount(ctx);
	emitInstrForNode(ctx, loop->body, label_end);
	if(loop->body->kind == NODE_EXPR)
		emitInstr_POP1(ctx, loop->body->offset, 0);
	emitInstrForNode(ctx, loop->condition, label_break);
	emitInstr_JUMPIFANDPOP(ctx, start, loop->condition->offset, loop->condition->length);
	Label_SetHere(label_end, ctx);
	Label_Free(label_end);
}

static void emitInstrForNode(CodegenContext *ctx, Node *node, Label *label_break)
{
	assert(node != NULL);

	switch(node->kind)
	{
		case NODE_EXPR:
		emitInstrForExprNode(ctx, (ExprNode*) node, label_break);
		return;

		case NODE_BREAK:
		if(label_break == NULL)
			CodegenContext_ReportErrorAndJump(ctx, 0, "Break not inside a loop");
		emitInstr_JUMP(ctx, label_break, node->offset, node->length);
		return;

		case NODE_IFELSE:
		emitInstrForIfElseNode(ctx, (IfElseNode*) node, label_break);
		return;

		case NODE_WHILE:
		emitInstrForWhileLoopNode(ctx, (WhileNode*) node, label_break);
		return;

		case NODE_DOWHILE:
		emitInstrForDoWhileLoopNode(ctx, (DoWhileNode*) node, label_break);
		return;

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
#warning "What if this is in the global scope?"
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

	jmp_buf env;

	CodegenContext *ctx = CodegenContext_New(error, alloc);
	if(ctx == NULL) {
		Error_Report(error, 1, "No memory");
		return NULL;
	}

	if(setjmp(env)) {
		assert(error->occurred == true);
		CodegenContext_Free(ctx);
		return NULL;
	}

	assert(error->occurred == false);
	CodegenContext_SetJumpDest(ctx, &env);

	emitInstrForNode(ctx, ast->root, NULL);
	emitInstr_EXIT(ctx, Source_GetSize(ast->src), 0);
	assert(error->occurred == false);

	return CodegenContext_MakeExecutableAndFree(ctx, ast->src);
}
