#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include "../utils/defs.h"
#include "parse.h"
#include "ASTi.h"

typedef enum {
	
	TPOS = '+',
	TNEG = '-',
	
	TADD = '+',
	TSUB = '-',
	TMUL = '*',
	TDIV = '/',
	TASS = '=',

	TDONE = 256,
	TINT,
	TFLOAT,
	TSTRING,
	TIDENT,

	TKWIF,
	TKWELSE,
	TKWNONE,
	TKWTRUE,
	TKWFALSE,
	TKWRETURN,
} TokenKind;

typedef struct Token Token;
struct Token {
	TokenKind kind;
	Token *prev, *next;
	int offset, length;
};

typedef struct {
	const char *src;
	Token   *token;
	BPAlloc *alloc;
	Error   *error;
} Context;

static Node *parse_statement(Context *ctx);
static Node *parse_expression(Context *ctx);
static Node *parse_expression_statement(Context *ctx);
static Node *parse_ifelse_statement(Context *ctx);
static Node *parse_compound_statement(Context *ctx, TokenKind end);

static inline _Bool isoper(char c)
{
	return 	c == '+' || 
			c == '-' || 
			c == '*' || 
			c == '/' || 
			c == '=';
}

AST *parse(Source *src, BPAlloc *alloc, Error *error)
{
	assert(src != NULL);
	assert(alloc != NULL);

	const char *str = Source_GetBody(src);
	int 		len = Source_GetSize(src);
	assert(str != NULL);
	assert(len >= 0);

	AST *ast = BPAlloc_Malloc(alloc, sizeof(AST));

	if(ast == NULL)
		return NULL;

	Token *head = NULL, 
		  *tail = NULL;
	int i = 0;
	while(1)
		{
			// Skip whitespace and comments.
			while(i < len && (isspace(str[i]) || str[i] == '#'))
				{
					while(i < len && isspace(str[i]))
						i += 1;

					if(str[i] == '#')
						{
							i += 1;

							while(i < len && str[i] != '\n')
								i += 1;
						}
				}

			if(i == len)
				break; // No more tokens left.

			Token *tok = BPAlloc_Malloc(alloc, sizeof(Token)); // Allocate a token.
			
			if(tok == NULL)
				{
					// Error: No memory.
					Error_Report(error, 1, "No memory");
					return NULL;
				}

			if(isalpha(str[i]) || str[i] == '_')
				{
					tok->kind = TIDENT;
					tok->offset = i;
					
					while(i < len && (isalpha(str[i]) || str[i] == '_'))
						i += 1;

					tok->length = i - tok->offset;

					#define matchstr(str, len, const_str) \
						(len == sizeof(const_str)-1 && !strncmp(str, const_str, sizeof(const_str)-1))
					
					if(matchstr(str + tok->offset, tok->length, "if"))
						{
							tok->kind = TKWIF;
						}
					else if(matchstr(str + tok->offset, tok->length, "else"))
						{
							tok->kind = TKWELSE;
						}
					else if(matchstr(str + tok->offset, tok->length, "none"))
						{
							tok->kind = TKWNONE;
						}
					else if(matchstr(str + tok->offset, tok->length, "true"))
						{
							tok->kind = TKWTRUE;
						}
					else if(matchstr(str + tok->offset, tok->length, "false"))
						{
							tok->kind = TKWFALSE;
						}
					else if(matchstr(str + tok->offset, tok->length, "return"))
						{
							tok->kind = TKWRETURN;
						}

					#undef matchstr
				}
			else if(isdigit(str[i]))
				{
					tok->kind = TINT;
					tok->offset = i;

					while(i < len && isdigit(str[i]))
						i += 1;

					if(i+1 < len && str[i] == '.' && isdigit(str[i+1]))
						{
							i += 1; // Consume the dot.

							tok->kind = TFLOAT;

							while(i < len && isdigit(str[i]))
								i += 1;
						}

					tok->length = i - tok->offset;
				}
			else if(str[i] == '\'' || str[i] == '"')
				{
					tok->kind = TSTRING;
					tok->offset = i;

					char f = str[i];

					i += 1; // Skip the starting quote.

					while(i < len && str[i] != f)
						i += 1;

					if(i == len)
						{
							Error_Report(error, 0, "Source ended inside string literal");
							return NULL;
						}

					i += 1; // Consume the ' or ".

					tok->length = i - tok->offset;
				}
			else if(isoper(str[i]))
				{
					tok->offset = i;
					
					while(i < len && isoper(str[i]))
						i += 1;

					tok->length = i - tok->offset;
				
					// Determine the token

					#define matchop(str, len, const_str) \
						(len == sizeof(const_str)-1 && !strncmp(str, const_str, sizeof(const_str)-1))

					if(matchop(str + tok->offset, tok->length, "+"))
						{
							tok->kind = TADD;
						}
					else if(matchop(str + tok->offset, tok->length, "-"))
						{
							tok->kind = TSUB;
						}
					else if(matchop(str + tok->offset, tok->length, "*"))
						{
							tok->kind = TMUL;
						}
					else if(matchop(str + tok->offset, tok->length, "/"))
						{
							tok->kind = TDIV;
						}
					else if(matchop(str + tok->offset, tok->length, "="))
						{
							tok->kind = TASS;
						}
					else 
						{
							// Not a known operator.
							tok->kind = str[tok->offset];
							tok->length = 1;
							i = tok->offset + 1;
						}

					#undef matchop
				}
			else	
				{
					tok->kind = str[i];
					tok->offset = i;
					tok->length = 1;
					i += 1;
				}

			// Append to the token list.
			if(head)
				tail->next = tok;
			else
				head = tok;
			tok->prev = tail;
			tok->next = NULL;
			tail = tok;
		}

	{
		Token *tok = BPAlloc_Malloc(alloc, sizeof(Token)); // Allocate a token.
			
		if(tok == NULL)
			{
				// Error: No memory.
				Error_Report(error, 1, "No memory");
				return NULL;
			}

		tok->kind = TDONE;
		tok->offset = i;
		tok->length = 0;

		if(head)
			tail->next = tok;
		else
			head = tok;
		tok->prev = tail;
		tok->next = NULL;
		tail = tok;
	}

	Context ctx;
	ctx.src   = str;
	ctx.token = head;
	ctx.alloc = alloc;
	ctx.error = error;

	Node *root = parse_compound_statement(&ctx, TDONE);

	if(root == NULL)
		return NULL;

	ast->src = src; // Not copying! Be sure to not free the source before the AST!
	ast->root = root;

	if(ast->src == NULL)
		return NULL;

	return ast;
}

static inline Token *current_token(Context *ctx)
{
	assert(ctx != NULL);
	assert(ctx->token != NULL);
	return ctx->token;
}

static inline TokenKind current(Context *ctx)
{
	assert(ctx != NULL);
	return current_token(ctx)->kind;
}

static inline TokenKind next(Context *ctx)
{
	assert(ctx != NULL);
	assert(ctx->token != NULL);
	assert(ctx->token->kind != TDONE);
	ctx->token = ctx->token->next;
	return current(ctx);
}

static inline TokenKind prev(Context *ctx)
{
	assert(ctx != NULL);
	assert(ctx->token != NULL);
	assert(ctx->token->prev != NULL);
	ctx->token = ctx->token->prev;
	return current(ctx);
}

static inline _Bool done(Context *ctx)
{
	return current(ctx) == TDONE;
}

static Node *parse_statement(Context *ctx)
{
	assert(ctx != NULL);

	switch(current(ctx))
		{
			case '*':
			case '/':
			case TASS:
			case TDONE:
			UNREACHABLE;
			break;

			case '(':
			case '+':
			case '-':
			case TINT:
			case TFLOAT:
			case TSTRING:
			case TIDENT:
			case TKWNONE:
			case TKWTRUE:
			case TKWFALSE:
			return parse_expression_statement(ctx);

			case TKWIF:
			return parse_ifelse_statement(ctx);

			case '{':
			{
				next(ctx); // Consume the '{'.

				Node *node = parse_compound_statement(ctx, '}');

				if(node != NULL)
					next(ctx); // Consume the '}'.

				return node;
			}

			case TKWRETURN:
			{
				int offset = current_token(ctx)->offset;

				next(ctx); // Consume the "return" keyword.

				Node *val = parse_expression_statement(ctx);

				if(val == NULL)
					return NULL;

				ReturnNode *node = BPAlloc_Malloc(ctx->alloc, sizeof(ReturnNode));
				
				if(node == NULL)
					{
						Error_Report(ctx->error, 1, "No memory");
						return NULL;
					}

				node->base.kind = NODE_RETURN;
				node->base.next = NULL;
				node->base.offset = offset;
				node->base.length = val->offset + val->length - offset;
				node->val = val;
				return (Node*) node;
			}
		}

	Error_Report(ctx->error, 0, "Got token \"%.*s\" where the start of a statement was expected", 
		ctx->token->length, ctx->src + ctx->token->offset);
	return NULL;
}

static Node *parse_expression_statement(Context *ctx)
{
	assert(ctx != NULL);

	Node *expr = parse_expression(ctx);
	if(expr == NULL) return NULL;

	// The final statement doesn't need a ';'.
	if(done(ctx)) return expr;

	if(current(ctx) != ';')
		{
			// ERROR: 	Got something other than a semicolon at the end 
			// 			of statement.

			Error_Report(ctx->error, 0, "Got token \"%.*s\" where \";\" was expected", ctx->token->length, ctx->src + ctx->token->offset);
			return NULL;
		}

	next(ctx);
	return expr;
}

static Node *parse_string_primary_expression(Context *ctx)
{
	assert(ctx != NULL);
	
	if(done(ctx))
		{
			Error_Report(ctx->error, 0, "Source ended where a string literal was expected");
			return NULL;
		}

	if(current(ctx) != TSTRING)
		{
			Error_Report(ctx->error, 0, "Got token \"%.*s\" where a string literal was expected", ctx->token->length, ctx->src + ctx->token->offset);
			return NULL;
		}

	const char *src = ctx->src;
	int 		len = ctx->token->offset + ctx->token->length - 1;
	int 		i   = ctx->token->offset + 1;
	
	assert(src[i-1] == '"' || src[i-1] == '\'');
	assert(src[len] == '"' || src[len] == '\'');

	char temp[4096];
	int  temp_used = 0;

	do
		{
			int segm_off, segm_len;
			{
				segm_off = i;

				while(i < len && src[i] != '\\')
					i += 1;

				segm_len = i - segm_off; 
			}

			if(temp_used + segm_len >= (int) sizeof(temp))
				{
					Error_Report(ctx->error, 1, "String is too big to be rendered inside the fixed size buffer");
					return NULL;
				}

			memcpy(temp + temp_used, src + segm_off, segm_len);
			temp_used += segm_len;

			if(src[i] == '\\')
				{
					i += 1; // Consume the \.
					
					if(temp_used + 1 >= (int) sizeof(temp))
						{
							Error_Report(ctx->error, 1, "String is too big to be rendered inside the fixed size buffer");
							return NULL;
						}

					if(i == len)
						{
							// Append the \ as a normal char.
							temp[temp_used++] = '\\';
						}
					else
						{
							switch(src[i])
								{
									case 'n': temp[temp_used++] = '\n'; break;
									case 't': temp[temp_used++] = '\t'; break;
									case 'r': temp[temp_used++] = '\r'; break;

									default:
									Error_Report(ctx->error, 0, "Invalid escape sequence \\%c", src[i]);
									return NULL;
								}

							i += 1; // Consume the char after the \.
						}
				}
		}
	while(i < len);

	assert(temp_used < (int) sizeof(temp));

	temp[temp_used] = '\0';

	char *copy;
	int   copyl;

	{
		copy = BPAlloc_Malloc(ctx->alloc, temp_used + 1);

		if(copy == NULL)
			{
				Error_Report(ctx->error, 1, "No memory");
				return NULL;
			}

		strcpy(copy, temp);
		copyl = temp_used;
	}

	StringExprNode *node;
	{
		node = BPAlloc_Malloc(ctx->alloc, sizeof(StringExprNode));

		if(node == NULL)
			{
				Error_Report(ctx->error, 1, "No memory");
				return NULL;
			}

		node->base.base.kind = NODE_EXPR;
		node->base.base.next = NULL;
		node->base.base.offset = ctx->token->offset;
		node->base.base.length = ctx->token->length;
		node->base.kind = EXPR_STRING;
		node->val = copy;
		node->len = copyl;
	}

	next(ctx);
	return (Node*) node;
}

static Node *parse_primary_expresion(Context *ctx)
{
	assert(ctx != NULL);

	if(done(ctx))
		{
			Error_Report(ctx->error, 0, "Source ended where a primary expression was expected");
			return NULL;
		}

	switch(current(ctx))
		{
			case '+':
			case '-':
			{
				Token *unary_operator = current_token(ctx);

				next(ctx);

				Node *operand = parse_primary_expresion(ctx);
				
				if(operand == NULL) 
					return NULL;

				OperExprNode *temp = BPAlloc_Malloc(ctx->alloc, sizeof(OperExprNode));
				{
					if(temp == NULL)
						return NULL;

					temp->base.base.kind = NODE_EXPR;
					temp->base.base.next = NULL;
					temp->base.base.offset = unary_operator->offset;
					temp->base.base.length = operand->offset + operand->length - unary_operator->offset;
					temp->base.kind = unary_operator->kind == '+' ? EXPR_POS : EXPR_NEG;
					temp->head = operand;
					temp->count = 1;
				}
				return (Node*) temp;
			}

			case '(':
			{
				next(ctx); // Consume the '('.
				
				Node *node = parse_expression(ctx);
				
				if(node == NULL)
					return NULL;

				if(done(ctx))
					{
						Error_Report(ctx->error, 0, "Source ended before \")\", after sub-expression");
						return NULL;
					}

				if(current(ctx) != ')')
					{
						Error_Report(ctx->error, 0, "Missing \")\", after sub-expression");
						return NULL;
					}

				next(ctx); // Consume the ')'.
				return node;
			}

			case TINT:
			{
				char buffer[64];

				if(ctx->token->length >= (int) sizeof(buffer))
					{
						Error_Report(ctx->error, 1, "Integer is too big");
						return NULL;
					}

				memcpy(buffer, ctx->src + ctx->token->offset, ctx->token->length);
				buffer[ctx->token->length] = '\0';

				errno = 0;

				long long int val = strtoll(buffer, NULL, 10);

				if(errno == ERANGE)
					{
						Error_Report(ctx->error, 1, "Integer is too big");
						return NULL;
					}
				else assert(errno == 0);

				IntExprNode *node;
				{
					node = BPAlloc_Malloc(ctx->alloc, sizeof(IntExprNode));

					if(node == NULL)
						{
							Error_Report(ctx->error, 1, "No memory");
							return NULL;
						}

					node->base.base.kind = NODE_EXPR;
					node->base.base.next = NULL;
					node->base.base.offset = ctx->token->offset;
					node->base.base.length = ctx->token->length;
					node->base.kind = EXPR_INT;
					node->val = val;
				}

				ctx->token = ctx->token->next;
				assert(ctx->token != NULL);

				return (Node*) node;
			}

			case TFLOAT:
			{
				char buffer[64];

				if(ctx->token->length >= (int) sizeof(buffer))
					{
						Error_Report(ctx->error, 1, "Floating is too big");
						return NULL;
					}

				memcpy(buffer, ctx->src + ctx->token->offset, ctx->token->length);
				buffer[ctx->token->length] = '\0';

				errno = 0;

				double val = strtod(buffer, NULL);

				if(errno == ERANGE)
					{
						Error_Report(ctx->error, 1, "Floating is too big");
						return NULL;
					}
				else assert(errno == 0);

				FloatExprNode *node;
				{
					node = BPAlloc_Malloc(ctx->alloc, sizeof(FloatExprNode));

					if(node == NULL)
						{
							Error_Report(ctx->error, 1, "No memory");
							return NULL;
						}

					node->base.base.kind = NODE_EXPR;
					node->base.base.next = NULL;
					node->base.base.offset = ctx->token->offset;
					node->base.base.length = ctx->token->length;
					node->base.kind = EXPR_FLOAT;
					node->val = val;
				}

				ctx->token = ctx->token->next;
				assert(ctx->token != NULL);
				
				return (Node*) node;
			}

			case TSTRING:
			return parse_string_primary_expression(ctx);

			case TKWNONE:
			{
				next(ctx);

				ExprNode *node;
				{
					node = BPAlloc_Malloc(ctx->alloc, sizeof(ExprNode));
				
					if(node == NULL)
						{
							Error_Report(ctx->error, 1, "No memory");
							return NULL;
						}

					node->base.kind = NODE_EXPR;
					node->base.next = NULL;
					node->base.offset = ctx->token->offset;
					node->base.length = ctx->token->length;
					node->kind = EXPR_NONE;
				}

				return (Node*) node;
			}

			case TKWTRUE:
			{
				next(ctx);

				ExprNode *node;
				{
					node = BPAlloc_Malloc(ctx->alloc, sizeof(ExprNode));
				
					if(node == NULL)
						{
							Error_Report(ctx->error, 1, "No memory");
							return NULL;
						}

					node->base.kind = NODE_EXPR;
					node->base.next = NULL;
					node->base.offset = ctx->token->offset;
					node->base.length = ctx->token->length;
					node->kind = EXPR_TRUE;
				}

				return (Node*) node;
			}

			case TKWFALSE:
			{
				next(ctx);

				ExprNode *node;
				{
					node = BPAlloc_Malloc(ctx->alloc, sizeof(ExprNode));
				
					if(node == NULL)
						{
							Error_Report(ctx->error, 1, "No memory");
							return NULL;
						}

					node->base.kind = NODE_EXPR;
					node->base.next = NULL;
					node->base.offset = ctx->token->offset;
					node->base.length = ctx->token->length;
					node->kind = EXPR_FALSE;
				}

				return (Node*) node;
			}

			case TIDENT:
			{
				char *copy;
				int   copyl;

				{
					copy = BPAlloc_Malloc(ctx->alloc, ctx->token->length + 1);

					if(copy == NULL)
						{
							Error_Report(ctx->error, 1, "No memory");
							return NULL;
						}

					copyl = ctx->token->length;
					memcpy(copy, ctx->src + ctx->token->offset, copyl);
					copy[copyl] = '\0';
					
				}

				IdentExprNode *node;
				{
					node = BPAlloc_Malloc(ctx->alloc, sizeof(IdentExprNode));

					if(node == NULL)
						{
							Error_Report(ctx->error, 1, "No memory");
							return NULL;
						}

					node->base.base.kind = NODE_EXPR;
					node->base.base.next = NULL;
					node->base.base.offset = ctx->token->offset;
					node->base.base.length = ctx->token->length;
					node->base.kind = EXPR_IDENT;
					node->val = copy;
					node->len = copyl;
				}

				ctx->token = ctx->token->next;
				assert(ctx->token != NULL);

				return (Node*) node;
			}

			case TDONE:
			Error_Report(ctx->error, 1, "Unexpected end of source where a primary expression was expected");
			return NULL;

			default:
			Error_Report(ctx->error, 1, "Unexpected token \"%.*s\" where a primary expression was expected", ctx->token->length, ctx->src + ctx->token->offset);
			return NULL;
		}

	UNREACHABLE;
	return NULL;
}

static inline _Bool isbinop(Token *tok)
{
	assert(tok != NULL);

	return 	tok->kind == '+' || 
			tok->kind == '-' || 
			tok->kind == '*' || 
			tok->kind == '/';
}

static inline _Bool isrightassoc(Token *tok)
{
	assert(tok != NULL);

	(void) tok;
	return 0;
}

static inline int precedenceof(Token *tok)
{
	assert(tok != NULL);

	switch(tok->kind)
		{
			case '+':
			case '-':
			return 1;

			case '*':
			case '/':
			return 2;
			
			default:
			return -100000000;
		}

	UNREACHABLE;
	return -100000000;
}

static Node *parse_expression_2(Context *ctx, Node *left_expr, int min_prec)
{
	while(isbinop(ctx->token) && precedenceof(ctx->token) >= min_prec)
		{
			Token *op = ctx->token;

			ctx->token = ctx->token->next;
			assert(ctx->token != NULL);

			Node *right_expr = parse_primary_expresion(ctx);

			if(right_expr == NULL)
				return NULL;

			while(isbinop(ctx->token) && (precedenceof(ctx->token) > precedenceof(op) || (precedenceof(ctx->token) == precedenceof(op) && isrightassoc(ctx->token))))
				{
					right_expr = parse_expression_2(ctx, right_expr, precedenceof(op) + 1);
					
					if(right_expr == NULL)
						return NULL;				
				}

			OperExprNode *temp;
			{
				temp = BPAlloc_Malloc(ctx->alloc, sizeof(OperExprNode));

				if(temp == NULL)
					{
						Error_Report(ctx->error, 1, "No memory");
						return NULL;
					}

				temp->base.base.kind = NODE_EXPR;
				temp->base.base.next = NULL;
				temp->base.base.offset = left_expr->offset;
				temp->base.base.length = right_expr->offset + right_expr->length - left_expr->offset;

				switch(op->kind)
					{
						case '+': temp->base.kind = EXPR_ADD; break;
						case '-': temp->base.kind = EXPR_SUB; break;
						case '*': temp->base.kind = EXPR_MUL; break;
						case '/': temp->base.kind = EXPR_DIV; break;

						default:
						UNREACHABLE;
						break;
					}

				temp->head = left_expr;
				temp->head->next = right_expr;
				temp->count = 2;
				assert(right_expr->next == NULL);
			}

			left_expr = (Node*) temp;
		}

	return left_expr;
}

static Node *parse_expression(Context *ctx)
{
	Node *left_expr = parse_primary_expresion(ctx);

	if(left_expr == NULL)
		return NULL;

	if(ctx->token->kind == TDONE)
		return left_expr;

	return parse_expression_2(ctx, left_expr, -1000000000);
}

static Node *parse_ifelse_statement(Context *ctx)
{
	assert(ctx != NULL);
	assert(ctx->token != NULL);

	if(done(ctx))
		{
			Error_Report(ctx->error, 0, "Source ended where an if-else statement was expected");
			return NULL;
		}

	if(current(ctx) != TKWIF)
		{
			Error_Report(ctx->error, 0, "Got unexpected token \"%.*s\" where an if-else statement was expected", ctx->token->length, ctx->src + ctx->token->offset);
			return NULL;
		}

	Token *if_token = current_token(ctx);
	assert(if_token != NULL);

	next(ctx); // Consume the "if" keyword.

	Node *condition = parse_expression(ctx);
	
	if(condition == NULL) 
		return NULL;

	Node *true_branch = parse_statement(ctx);
	
	if(condition == NULL) 
		return NULL;

	Node *false_branch = NULL;

	if(ctx->token->kind == TKWELSE)
		{
			next(ctx); // Consume the "else" token.

			false_branch = parse_statement(ctx);
			
			if(false_branch == NULL) 
				return NULL;
		}

	IfElseNode *ifelse;
	{
		ifelse = BPAlloc_Malloc(ctx->alloc, sizeof(IfElseNode));
		
		if(ifelse == NULL)
			{
				// ERROR: No memory.
				Error_Report(ctx->error, 1, "No memory");
				return NULL;
			}

		ifelse->base.kind = NODE_IFELSE;
		ifelse->base.next = NULL;
		ifelse->base.offset = if_token->offset;
		ifelse->base.length = ctx->token->offset + ctx->token->length - if_token->offset;
		ifelse->condition = condition;
		ifelse->true_branch = true_branch;
		ifelse->false_branch = false_branch;
	}

	return (Node*) ifelse;
}

static Node *parse_compound_statement(Context *ctx, TokenKind end)
{
	int end_offset;
	Node *head, **tail;

	tail = &head;
	*tail = NULL;

	while(current(ctx) != end && current(ctx) != TDONE)
		{
			Node *temp = parse_statement(ctx);

			if(temp == NULL)
				return NULL;

			*tail = temp;
			tail = &temp->next;

			end_offset = temp->offset + temp->length;
		}

	if(current(ctx) != end)
		{
			Error_Report(ctx->error, 0, "Source ended inside compound statement");
			return NULL;
		}

	CompoundNode *node;
	{
		node = BPAlloc_Malloc(ctx->alloc, sizeof(CompoundNode));
		
		if(node == NULL)
			{
				// ERROR: No memory.
				Error_Report(ctx->error, 1, "No memory");
				return NULL;
			}

		node->base.kind = NODE_COMP;
		node->base.next = NULL;
		node->base.offset = head->offset;
		node->base.length = end_offset - head->offset;
		node->head = head;
	}

	return (Node*) node;
}