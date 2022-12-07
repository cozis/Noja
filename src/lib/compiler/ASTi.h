
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

#ifndef ASTi_H
#define ASTi_H
#include "../utils/source.h"
#include "AST.h"

typedef enum {
	NODE_EXPR, 
	NODE_IFELSE,
	NODE_COMP,
	NODE_RETURN,
	NODE_FUNC,
	NODE_ARG,
	NODE_WHILE,
	NODE_BREAK,
	NODE_DOWHILE,
} NodeKind;

typedef enum {
	EXPR_POS,
	EXPR_NEG,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_MUL,
	EXPR_DIV,
	EXPR_MOD,

	EXPR_EQL,
	EXPR_NQL,
	EXPR_LSS,
	EXPR_LEQ,
	EXPR_GRT,
	EXPR_GEQ,

	EXPR_AND,
	EXPR_OR,
	EXPR_NOT,

	EXPR_ASS,
	EXPR_INT,
	EXPR_MAP,
	EXPR_CALL,
	EXPR_PAIR,
	EXPR_LIST,
	EXPR_NONE,
	EXPR_TRUE,
	EXPR_FALSE,
	EXPR_FLOAT,
	EXPR_STRING,
	EXPR_IDENT,
	EXPR_SELECT,

	EXPR_NULLABLETYPE,
	EXPR_SUMTYPE,
} ExprKind;

typedef struct Node Node;

struct xAST {
	Source 	*src;
	Node 	*root;
};

struct Node {
	NodeKind kind;
	Node 	*next;
	int 	 offset, 
			 length;
};

typedef struct {
	Node 	 base;
	ExprKind kind;
} ExprNode;

typedef struct {
	ExprNode base;
	long long int val;
} IntExprNode;

typedef struct {
	ExprNode base;
	double 	 val;
} FloatExprNode;

typedef struct {
	ExprNode base;
	char *val;
	int   len;
} StringExprNode;

typedef struct {
	ExprNode base;
	Node    *items;
	int      itemc;
} ListExprNode;

typedef struct {
	ExprNode base;
	Node    *keys;
	Node    *items;
	int      itemc;
} MapExprNode;

typedef struct {
	ExprNode base;
	char *val;
	int   len;
} IdentExprNode;

typedef struct {
	ExprNode base;
	Node    *head;
	int	     count;
} OperExprNode;

typedef struct {
	ExprNode base;
	Node    *func;
	Node    *argv;
	int      argc;
} CallExprNode;

typedef struct {
	ExprNode base;
	Node    *idx;
	Node    *set;
} IndexSelectionExprNode;

typedef struct {
	Node  base;
	Node *condition;
	Node *true_branch;
	Node *false_branch;
} IfElseNode;

typedef struct {
	Node  base;
	Node *condition;
	Node *body;
} WhileNode;

typedef struct {
	Node  base;
	Node *body;
	Node *condition;
} DoWhileNode;

typedef struct {
	Node  base;
	Node *head;
} CompoundNode;

typedef struct {
	Node base;
	Node *val;
} ReturnNode;

typedef struct {
	Node  base;
	char *name;
	Node *argv;
	int   argc;
	Node *body;
} FunctionNode;

typedef struct {
	Node  base;
	char *name;
	Node *type;
	Node *value;
} ArgumentNode;
#endif