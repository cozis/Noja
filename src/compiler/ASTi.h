
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
	NODE_DOWHILE,
} NodeKind;

typedef enum {
	EXPR_POS,
	EXPR_NEG,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_MUL,
	EXPR_DIV,

	EXPR_EQL,
	EXPR_NQL,
	EXPR_LSS,
	EXPR_LEQ,
	EXPR_GRT,
	EXPR_GEQ,

	EXPR_ASS,
	EXPR_INT,
	EXPR_CALL,
	EXPR_LIST,
	EXPR_NONE,
	EXPR_TRUE,
	EXPR_FALSE,
	EXPR_FLOAT,
	EXPR_STRING,
	EXPR_IDENT,
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
	char 	 body[];
};

typedef struct {
	Node 	 base;
	ExprKind kind;
	char 	 body[];
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
} ArgumentNode;
#endif