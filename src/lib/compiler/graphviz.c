
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
#include <string.h>
#include <stdbool.h>
#include "../utils/defs.h"
#include "graphviz.h"
#include "ASTi.h"

typedef struct {
	char  *data;
	size_t size;
	size_t used;
} Buffer;

typedef struct {
	Error *error;
	Buffer head, body;
} GraphViz;

static void Buffer_Init(Buffer *buf)
{
	buf->data = NULL;
	buf->size = 0;
	buf->used = 0;
}

static void Buffer_Free(Buffer *buf)
{
	free(buf->data);
}

static void GraphViz_Init(GraphViz *gv, Error *error)
{
	Buffer_Init(&gv->head);
	Buffer_Init(&gv->body);
	gv->error = error;
	gv->error->occurred = false;
}
static void GraphViz_Free(GraphViz *gv)
{
	Buffer_Free(&gv->head);
	Buffer_Free(&gv->body);
}

static bool Buffer_EnsureFreeSpace(Buffer *buf, Error *error, size_t num)
{
	if (buf->used + num > buf->size) {

		// Need to add space

		size_t oldsize = buf->size;
		size_t newsize = buf->size * 2;

		if (newsize < oldsize + num)
			newsize = oldsize + num;

		void *p = realloc(buf->data, newsize);
		if (p == NULL) {
			Error_Report(error, ErrorType_INTERNAL, "Out of memory");
			return false;
		}

		buf->size = newsize;
		buf->data = p;
	}

	return true;
}

static bool Buffer_VAppendF(Buffer *buf, Error *error, const char *fmt, va_list args)
{
	va_list args2;

	// Calculate the string length required to hold
	// the evaluated format
	va_copy(args2, args);
	int required = vsnprintf(NULL, 0, fmt, args2);
	va_end(args2);
	if (required < 0) {
		Error_Report(error, ErrorType_INTERNAL, "Bad format");
		return false;
	}

	if (!Buffer_EnsureFreeSpace(buf, error, required+1))
		return false;

	vsnprintf(buf->data + buf->used, buf->size - buf->used, fmt, args);
	va_end(args);

	buf->used += required;
	return true;
}

static void GraphViz_AppendHead(GraphViz *gv, const char *fmt, ...)
{
	if (gv->error->occurred)
		return;

	va_list args;
	va_start(args, fmt);
	Buffer_VAppendF(&gv->head, gv->error, fmt, args);
	va_end(args);
}

static void GraphViz_AppendBody(GraphViz *gv, const char *fmt, ...)
{
	if (gv->error->occurred)
		return;

	va_list args;
	va_start(args, fmt);
	Buffer_VAppendF(&gv->body, gv->error, fmt, args);
	va_end(args);
}

static char *GraphViz_Complete(GraphViz *gv, size_t *len)
{
	if (gv->error->occurred)
		return NULL;

	char  *result;
	size_t reslen;

	reslen = gv->head.used + gv->body.used;
	result = malloc(reslen+1);
	if (!result) {
		Error_Report(gv->error, ErrorType_INTERNAL, "Out of memory");
		return NULL;
	}

	memcpy(result,                 gv->head.data, gv->head.used);
	memcpy(result + gv->head.used, gv->body.data, gv->body.used);
	result[reslen] = '\0';

	if (len) *len = reslen;
	return result;
}

static void nodeToGraphViz(GraphViz *ctx, Node *node, int *count, char *name);

static void flattenTupleTree(ExprNode *root, ExprNode *tuple[], int max, int *count)
{
	if(root->kind == EXPR_PAIR)
	{
		flattenTupleTree((ExprNode*) ((OperExprNode*) root)->head, tuple, max, count);
		flattenTupleTree((ExprNode*) ((OperExprNode*) root)->head->next, tuple, max, count);
	} 
	else 
	{

		if(max == *count)
		{
			assert(0); // TODO: Do something smart instead
		}

		tuple[(*count)++] = root;
	}
}

static void generateName(int *count, char *dst, size_t max)
{
	snprintf(dst, max, "node_%d", ++(*count));
}

static const char *exprkind_to_label(ExprKind kind)
{
	switch(kind)
	{
		case EXPR_NULLABLETYPE: return "Nullable Type";
		case EXPR_SUMTYPE: return "Sum Type";
		case EXPR_NOT: return "!";
		case EXPR_POS: return "+";
		case EXPR_NEG: return "-";
		case EXPR_ADD: return "+";
		case EXPR_SUB: return "-";
		case EXPR_MUL: return "&times;";
		case EXPR_DIV: return "/";
		case EXPR_MOD: return "%";
		case EXPR_EQL: return "==";
		case EXPR_NQL: return "!=";
		case EXPR_LSS: return "<";
		case EXPR_LEQ: return "<=";
		case EXPR_GRT: return ">";
		case EXPR_GEQ: return ">=";
		case EXPR_AND: return "and";
		case EXPR_OR : return "or";
		case EXPR_ASS: return "=";
		case EXPR_ARW: return "->";
		default:
		UNREACHABLE;
		break;
	}
	UNREACHABLE;
	return "???";
}

static void childToGraphViz(GraphViz *ctx, Node *child, int *count, char *parent)
{
	char name2[128];
	generateName(count, name2, sizeof(name2));
	nodeToGraphViz(ctx, child, count, name2);
	GraphViz_AppendBody(ctx, "\t%s -> %s;\n", parent, name2);
}

static void funcExprToGraphViz(GraphViz *ctx, FuncExprNode *func, char *func_name, int *count, char *name)
{
	GraphViz_AppendHead(ctx, "\t%s [label=\"%s\"];\n", name, func_name);

	char name2[128];
	generateName(count, name2, sizeof(name2));
	GraphViz_AppendHead(ctx, "\t%s [label=\"args\"];\n", name2);
	GraphViz_AppendBody(ctx, "\t%s -> %s;\n", name, name2);

	ArgumentNode *arg = (ArgumentNode*) func->argv;
	while(arg)
	{
		char name3[128];
		generateName(count, name3, sizeof(name3));
		GraphViz_AppendHead(ctx, "\t%s [label=\"%s\"];\n", name3, arg->name);
		GraphViz_AppendBody(ctx, "\t%s -> %s;\n", name2, name3);
		if (arg->value != NULL) childToGraphViz(ctx, arg->value, count, name3);
		if (arg->type != NULL)  childToGraphViz(ctx, arg->type,  count, name3);
		arg = (ArgumentNode*) arg->base.next;
	}

	childToGraphViz(ctx, func->body, count, name);
}

static void exprToGraphViz(GraphViz *ctx, ExprNode *expr, int *count, char *name)
{
	switch(expr->kind)
	{
		case EXPR_PAIR:
		UNREACHABLE;
		return; // For the compiler warning.

		case EXPR_NULLABLETYPE:
		case EXPR_SUMTYPE:
		case EXPR_ASS: case EXPR_ARW:
		case EXPR_NOT: case EXPR_MOD:
		case EXPR_POS: case EXPR_NEG:
		case EXPR_ADD: case EXPR_SUB:
		case EXPR_MUL: case EXPR_DIV:
		case EXPR_EQL: case EXPR_NQL:
		case EXPR_LSS: case EXPR_LEQ:
		case EXPR_GRT: case EXPR_GEQ:
		case EXPR_AND: case EXPR_OR:
		{
			OperExprNode *oper = (OperExprNode*) expr;
			GraphViz_AppendHead(ctx, "\t%s [label=\"%s\"];\n", name, exprkind_to_label(expr->kind));
			for(Node *operand = oper->head; operand; operand = operand->next)
				childToGraphViz(ctx, operand, count, name);
			return;
		}

		case EXPR_INT:
		{
			IntExprNode *p = (IntExprNode*) expr;
			GraphViz_AppendHead(ctx, "\t%s [label=\"%d\"];\n", name, p->val);
			return;
		}

		case EXPR_FLOAT:
		{
			FloatExprNode *p = (FloatExprNode*) expr;
			GraphViz_AppendHead(ctx, "\t%s [label=\"%g\"];\n", name, p->val);
			return;
		}

		case EXPR_STRING:
		{
			StringExprNode *p = (StringExprNode*) expr;
			GraphViz_AppendHead(ctx, "\t%s [label=\"", name);
			char *s = p->val;
			size_t i = 0;
			size_t len = strlen(s);
			while (i < len) {

				size_t start = i;
				while (i < len && (s[i] != '"' && s[i] != '\\' && s[i] != '\n' && s[i] != '\t'))
					i++;
				size_t end = i;
				GraphViz_AppendHead(ctx, "%.*s", end - start, s + start);
				if (i == len)
					break;
				switch (s[i]) {
					case '"': GraphViz_AppendHead(ctx, "\\\""); break;
					case '\\': GraphViz_AppendHead(ctx, "\\\\"); break;
					case '\t': GraphViz_AppendHead(ctx, "\\t"); break;
					case '\n': GraphViz_AppendHead(ctx, "\\n"); break;
				}
				i++;
			}
			GraphViz_AppendHead(ctx, "\"];\n");
			return;
		}

		case EXPR_IDENT:
		{
			IdentExprNode *p = (IdentExprNode*) expr;
			GraphViz_AppendHead(ctx, "\t%s [label=\"%s\"];\n", name, p->val);
			return;
		}

		case EXPR_LIST:
		{
			ListExprNode *l = (ListExprNode*) expr;
			GraphViz_AppendHead(ctx, "\t%s [label=\"List\"];\n", name);
			for (Node *item = l->items; item; item = item->next)
				childToGraphViz(ctx, item, count, name);
			return;
		}

		case EXPR_MAP:
		{
			MapExprNode *m = (MapExprNode*) expr;

			GraphViz_AppendHead(ctx, "\t%s [label=\"Map\"];\n", name);

			Node *key  = m->keys;
			Node *item = m->items;
			
			while(item)
			{
				char name2[128];
				generateName(count, name2, sizeof(name2));
				GraphViz_AppendHead(ctx, "\t%s [label=\"Entry\"];\n", name2);
				GraphViz_AppendBody(ctx, "\t%s -> %s;\n", name, name2);

				childToGraphViz(ctx, key,  count, name2);
				childToGraphViz(ctx, item, count, name2);

				key  =  key->next;
				item = item->next;
			}
			return;
		}

		case EXPR_CALL:
		{
			CallExprNode *call = (CallExprNode*) expr;
			GraphViz_AppendHead(ctx, "\t%s [label=\"Call\"];\n", name);

			childToGraphViz(ctx, call->func, count, name);

			Node *arg = call->argv;
			while(arg)
			{
				childToGraphViz(ctx, arg, count, name);
				arg = arg->next;
			}
		}
		return;

		case EXPR_FUNC:
		funcExprToGraphViz(ctx, (FuncExprNode*) expr, "???", count, name);
		return;

		case EXPR_SELECT:
		{
			IndexSelectionExprNode *sel = (IndexSelectionExprNode*) expr;
			GraphViz_AppendHead(ctx, "\t%s [label=\"Select\"];\n", name);
			childToGraphViz(ctx, sel->set, count, name);
			childToGraphViz(ctx, sel->idx, count, name);
			return;
		}

		case EXPR_NONE:
		GraphViz_AppendHead(ctx, "\t%s [label=\"none\"];\n", name);
		return;

		case EXPR_TRUE:
		GraphViz_AppendHead(ctx, "\t%s [label=\"true\"];\n", name);
		return;
		
		case EXPR_FALSE:
		GraphViz_AppendHead(ctx, "\t%s [label=\"false\"];\n", name);
		return;

		default:
		UNREACHABLE;
		break;
	}
}

static void nodeToGraphViz(GraphViz *ctx, Node *node, int *count, char *name)
{
	assert(node != NULL);

	switch(node->kind)
	{
		case NODE_EXPR:
		exprToGraphViz(ctx, (ExprNode*) node, count, name);
		return;

		case NODE_BREAK:
		GraphViz_AppendHead(ctx, "\t%s [label=\"break\"];\n", name);
		return;

		case NODE_IFELSE:
		{
			IfElseNode *ifelse = (IfElseNode*) node;

			if (ifelse->false_branch)
				GraphViz_AppendHead(ctx, "\t%s [label=\"if-else\"];\n", name);
			else
				GraphViz_AppendHead(ctx, "\t%s [label=\"if\"];\n", name);

			childToGraphViz(ctx, ifelse->condition,   count, name);
			childToGraphViz(ctx, ifelse->true_branch, count, name);
			if (ifelse->false_branch)
				childToGraphViz(ctx, ifelse->false_branch, count, name);
		}
		return;

		case NODE_WHILE:
		{
			WhileNode *while_ = (WhileNode*) node;
			GraphViz_AppendHead(ctx, "\t%s [label=\"while\"];\n", name);
			childToGraphViz(ctx, while_->condition, count, name);
			childToGraphViz(ctx, while_->body, count, name);
		}
		return;

		case NODE_DOWHILE:
		{
			DoWhileNode *dowhile = (DoWhileNode*) node;
			GraphViz_AppendHead(ctx, "\t%s [label=\"do-while\"];\n", name);
			childToGraphViz(ctx, dowhile->condition, count, name);
			childToGraphViz(ctx, dowhile->body, count, name);
		}
		return;

		case NODE_COMP:
		{
			CompoundNode *comp = (CompoundNode*) node;

			GraphViz_AppendHead(ctx, "\t%s [label=\"compound\"];\n", name);

			Node *stmt = comp->head;
			while(stmt)
			{
				childToGraphViz(ctx, stmt, count, name);
				stmt = stmt->next;
			}
			return;
		}

		case NODE_RETURN:
		{
			ReturnNode *ret = (ReturnNode*) node;

			GraphViz_AppendHead(ctx, "\t%s [label=\"return\"];\n", name);

			ExprNode *tuple[32];
			int return_count = 0;

			flattenTupleTree((ExprNode*) ret->val, tuple, sizeof(tuple)/sizeof(tuple[0]), &return_count);

			for(int i = 0; i < return_count; i += 1)
				childToGraphViz(ctx, (Node*) tuple[i], count, name);
			return;
		}

		case NODE_FUNC:
		{
			FuncDeclNode *func = (FuncDeclNode*) node;
			GraphViz_AppendHead(ctx, "\t%s [label=\"func\"];\n", name);
			funcExprToGraphViz(ctx, func->expr, func->name->val, count, name);
		}
		return;

		default:
		UNREACHABLE;
	}
	UNREACHABLE;
}

char *graphviz(AST *ast, Error *err, size_t *len)
{
	err->occurred = false;

	GraphViz gv;
	GraphViz_Init(&gv, err);
	GraphViz_AppendHead(&gv, "digraph G {\n");

	int count = 0;
	char name[128];
	generateName(&count, name, sizeof(name));
	nodeToGraphViz(&gv, ast->root, &count, name);
	
	GraphViz_AppendHead(&gv, "\n");
	GraphViz_AppendBody(&gv, "}\n");
	
	char *res = GraphViz_Complete(&gv, len);

	GraphViz_Free(&gv);
	return res;
}
