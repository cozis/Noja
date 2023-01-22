
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
** | This file implements the parser of the language, that transforms `Source`|
** | objects into `AST` objects. The functionalities of this file are exposed |
** | throigh the `parse` function.                                            |
** |                                                                          |
** | It's mainly composed by routines that can each parse specific parts of a |
** | noja source string. For example, `parse_expression` parses expressions   |
** | and `parse_while_statement` parses while statements. These functions     |
** | call each other recursively to parse the source and build the abstract   |
** | syntax tree (AST) that can be then compiled into bytecode. If at any     |
** | point the parsing fails because of an external or internal error, then   |
** | the error is reported and the parsing is aborted.                        |
** |                                                                          |
** | Since the nodes of the AST always have the same lifetime (they're        |
** | allocated at the same time and die all together), the allocator scheme   |
** | of choise is a bump-pointer allocator. This way each of the parsing      |
** | routines can allocate memory if it need it but doesn't need to free it   |
** | if an error occurres.                                                    |
** |                                                                          |
** | The parsing routines don't operate directly on the source text, but on   |
** | the tokenized version of it. Before parsing a linked list of tokens is   |
** | produced through the `tokenize` function.                                |
** +--------------------------------------------------------------------------+
*/

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include "../utils/defs.h"
#include "parse.h"
#include "ASTi.h"

#ifdef DEBUG
#include <stdio.h>
FILE *token_dest;
#endif

typedef enum {
	
	TOPT = '?',
	TPOS = '+',
	TNEG = '-',
	
	TADD = '+',
	TSUB = '-',
	TMUL = '*',
	TDIV = '/',
	TMOD = '%',

	TLSS = '<',
	TGRT = '>',

	TASS = '=',
	TSMT = '|',

	TLBRK = '(',
	TRBRK = ')',
	TLSBRK = '[',
	TRSBRK = ']',
	TLCBRK = '{',
	TRCBRK = '}',
	TDOT = '.',
	TCOMMA = ',',

	TDONE = 256,
	TINT,
	TFLOAT,
	TSTRING,
	TIDENT,

	TKWIF,
	TKWFUN,
	TKWNOT,
	TKWAND,
	TKWOR,
	TKWELSE,
	TKWNONE,
	TKWTRUE,
	TKWFALSE,
	TKWRETURN,
	TKWWHILE,
	TKWBREAK,
	TKWDO,

	TEQL,
	TNQL,
	TLEQ,
	TGEQ,
	TARW,

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
	int *error_offset;
} Context;

static Node *parse_statement(Context *ctx);
static Node *parse_expression(Context *ctx, _Bool allow_toplev_tuples, _Bool allow_assignments);
static Node *parse_expression_statement(Context *ctx);
static Node *parse_ifelse_statement(Context *ctx);
static Node *parse_compound_statement(Context *ctx, TokenKind end);
static FuncDeclNode *parse_function_definition(Context *ctx);
static Node *parse_postfix_expression(Context *ctx);
static Node *parse_prefix_expression(Context *ctx);
static Node *parse_while_statement(Context *ctx);
static Node *parse_dowhile_statement(Context *ctx);

static inline _Bool isoper(char c)
{
	return 	c == '+' || c == '-' || 
			c == '*' || c == '/' || 
			c == '<' || c == '>' ||
			c == '!' || c == '=' ||
			c == ',' || c == '|' ||
			c == '%';
}

#warning "update doc arguments"
/* Symbol: tokenize
 * 
 *   Build a list of tokens that represents the 
 *   provided source code.
 *
 *
 * Arguments:
 *
 *   src: The source code to be tokenized.
 *   alloc: The allocator that will contain all of the
 *          generated tokens.
 *   error: Error information structure that is filled out if
 *          an error occurres.
 *
 *   None of the arguments are optional.
 *
 *
 * Returns:
 *   A pointer to the first node of a linked list of tokens.
 *   If an error occurres, NULL is returned and the `error` 
 *   structure is filled out.
 *
 */
static Token *tokenize(Source *src, BPAlloc *alloc, Error *error, int *error_offset)
{
	const char *str = Source_GetBody(src);
	int 		len = Source_GetSize(src);
	assert(str != NULL);
	assert(len >= 0);

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

			if(i < len && str[i] == '#')
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
			*error_offset = i;
			Error_Report(error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}

		if(isalpha(str[i]) || str[i] == '_')
		{
			tok->kind = TIDENT;
			tok->offset = i;
					
			while(i < len && (isalpha(str[i]) || isdigit(str[i]) || str[i] == '_'))
				i += 1;

			tok->length = i - tok->offset;

			static const struct {
				TokenKind    kind;
				int          size;
				const char  *text;
			} kwords[] = {
				{     TKWIF, 2, "if"     },
				{    TKWFUN, 3, "fun"    },
				{    TKWNOT, 3, "not"    },
				{    TKWAND, 3, "and"    },
				{     TKWOR, 2, "or"     },
				{   TKWELSE, 4, "else"   },
				{   TKWNONE, 4, "none"   },
				{   TKWTRUE, 4, "true"   },
				{  TKWFALSE, 5, "false"  },
				{ TKWRETURN, 6, "return" },
				{  TKWWHILE, 5, "while"  },
				{  TKWBREAK, 5, "break"  },
				{     TKWDO, 2, "do"     },
			};

			for(unsigned int i = 0; i < sizeof(kwords)/sizeof(*kwords); i += 1)
				if(kwords[i].size == tok->length && !strncmp(kwords[i].text, str + tok->offset, tok->length))
				{
					tok->kind = kwords[i].kind;
					break;
				}
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

			while(1)
			{
				while(i < len && str[i] != '\\' && str[i] != f)
					i += 1;

				if(str[i] == '\\')
				{
					i += 1; // Consume the \.
					if(i < len && (str[i] == '\'' || str[i] == '"' || str[i] == '\\'))
						i += 1;
				}
				else break;
			}

			if(i == len)
			{
				*error_offset = i;
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

			static const struct {
				TokenKind    kind;
				int          size;
				const char  *text;
			} optable[] = {
				{   TADD, 1, "+"  },
				{   TSUB, 1, "-"  },
				{   TMUL, 1, "*"  },
				{   TDIV, 1, "/"  },
				{   TEQL, 2, "==" },
				{   TNQL, 2, "!=" },
				{   TLSS, 1, "<"  },
				{   TLEQ, 2, "<=" },
				{   TGRT, 1, ">"  },
				{   TGEQ, 2, ">=" },
				{   TASS, 1, "="  },
				{ TCOMMA, 1, ","  },
				{   TSMT, 1, "|"  },
				{   TMOD, 1, "%"  },
				{   TARW, 2, "->" },
			};

			_Bool found = 0;
			for(unsigned int i = 0; i < sizeof(optable)/sizeof(*optable); i += 1)
				if(optable[i].size == tok->length && !strncmp(optable[i].text, str + tok->offset, tok->length))
				{
					found = 1;
					tok->kind = optable[i].kind;
					break;
				}

			if(found == 0)
			{
				// Not a known operator.
				tok->kind = str[tok->offset];
				tok->length = 1;
				i = tok->offset + 1;
			}
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
			*error_offset = i;
			Error_Report(error, ErrorType_INTERNAL, "No memory");
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

	return head;
}

/* Symbol: parse
 * 
 *   Build an AST that represents the provided source code.
 *
 *
 * Arguments:
 *
 *   src: The source code to be parsed.
 *   alloc: The allocator that will contain all of the garbage
 *          the function needs and the final AST.
 *   error: Error information structure that is filled out if
 *          an error occurres.
 *
 *   None of the arguments are optional.
 *
 *
 * Returns:
 *
 *   A pointer to the generated AST object. The AST object and
 *   all of the stuff that's referenced by it will be stored
 *   onto the provided allocator, therefore the AST will have
 *   the same lifetime of the allocator. If an error occurres,
 *   NULL is returned and the `error` structure is filled out.
 *
 * Notes:
 *   The AST structure holds a weak reference to the source
 *   object, therefore it will be invalidated if the source
 *   is freed before the AST. 
 *
 */
AST *parse(Source *src, BPAlloc *alloc, Error *error, int *error_offset)
{
	assert(src != NULL);
	assert(alloc != NULL);
	assert(error != NULL);

#ifdef DEBUG
		token_dest = fopen("tokens.txt", "wb");
#endif

	AST *ast = BPAlloc_Malloc(alloc, sizeof(AST));

	if(ast == NULL)
		#warning "should an error be reported here?"
		return NULL;

	Token *tokens = tokenize(src, alloc, error, error_offset);

	if(tokens == NULL)
		return NULL;

	Context ctx;
	ctx.src   = Source_GetBody(src);
	ctx.token = tokens;
	ctx.alloc = alloc;
	ctx.error = error;
	ctx.error_offset = error_offset;
	Node *root = parse_compound_statement(&ctx, TDONE);

	if(root == NULL)
		return NULL;

	ast->src = src; // Not copying! Be sure not to free the source before the AST!
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

#ifdef DEBUG
static inline TokenKind next(Context *ctx, const char *file, int line)
{
	assert(ctx != NULL);
	assert(ctx->token != NULL);
	assert(ctx->token->kind != TDONE);

	Token *prev = ctx->token;

	ctx->token = ctx->token->next;

	fprintf(token_dest, "NEXT [%.*s] -> [%.*s] from %s:%d\n", 
		      prev->length, ctx->src +       prev->offset,
		ctx->token->length, ctx->src + ctx->token->offset, 
		file, line);

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

#define next(ctx) next(ctx, __FILE__, __LINE__)

static void Error_Report_(Error *error, const char *file, const char *func, int line, _Bool internal, const char *fmt, ...)
{
	//fprintf(stderr, "Reporting error at %s:%d (in %s)\n", file, line, func);
	
	va_list args;
	va_start(args, fmt);
	_Error_Report2(error, internal, file, func, line, fmt, args);
	va_end(args);
}

#ifdef Error_Report
#undef Error_Report
#endif

#define Error_Report(error, internal, fmt, ...) Error_Report_(error, __FILE__, __func__, __LINE__, internal, fmt, ## __VA_ARGS__)

#else

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

#endif

static inline _Bool done(Context *ctx)
{
	return current(ctx) == TDONE;
}

static Node *parse_statement(Context *ctx)
{
	assert(ctx != NULL);

	switch(current(ctx))
	{
		default:
		break;

		case '(':
		case '[':
		case '+':
		case '-':
		case '?':
		case TINT:
		case TFLOAT:
		case TSTRING:
		case TIDENT:
		case TKWNOT:
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

		case TKWBREAK:
		{
			Token *token = current_token(ctx);

			next(ctx); // Consume the "break".

			if(current(ctx) != ';')
			{
				*ctx->error_offset = token->offset;
				Error_Report(ctx->error, ErrorType_SYNTAX, "Got token \"%.*s\" where \";\" was expected", ctx->token->length, ctx->src + ctx->token->offset);
				return NULL;
			}

			next(ctx); // Consume the ';'.
				
			Node *node = BPAlloc_Malloc(ctx->alloc, sizeof(Node));
				
			if(node == NULL)
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
				return NULL;
			}

			node->kind = NODE_BREAK;
			node->next = NULL;
			node->offset = token->offset;
			node->length = token->length;
			return node;
		}

		case TKWRETURN:
		{
			int offset = current_token(ctx)->offset;

			next(ctx); // Consume the "return" keyword.

			Node *val = parse_expression(ctx, 1, 1);

			if(val == NULL)
				return NULL;

			if(current(ctx) != ';')
			{
				*ctx->error_offset = current_token(ctx)->offset;
				Error_Report(ctx->error, ErrorType_SYNTAX, 
					         "Got token \"%.*s\" where \";\" was expected", 
					         ctx->token->length, ctx->src + ctx->token->offset);
				return NULL;
			}

			next(ctx); // Consume the ';'.

			ReturnNode *node = BPAlloc_Malloc(ctx->alloc, sizeof(ReturnNode));
				
			if(node == NULL)
			{
				*ctx->error_offset = current_token(ctx)->offset;
				Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
				return NULL;
			}

			node->base.kind = NODE_RETURN;
			node->base.next = NULL;
			node->base.offset = offset;
			node->base.length = val->offset + val->length - offset;
			node->val = val;
			return (Node*) node;
		}

		case TKWFUN:
		return (Node*) parse_function_definition(ctx);

		case TKWWHILE:
		return parse_while_statement(ctx);

		case TKWDO:
		return parse_dowhile_statement(ctx);
	}

	*ctx->error_offset = current_token(ctx)->offset;
	Error_Report(ctx->error, ErrorType_SYNTAX, "Got token \"%.*s\" where the start of a statement was expected", 
		ctx->token->length, ctx->src + ctx->token->offset);
	return NULL;
}

static Node *parse_expression_statement(Context *ctx)
{
	assert(ctx != NULL);

	Node *expr = parse_expression(ctx, 1, 1);
	
	if(expr == NULL) 
		return NULL;

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended right after an expression, where a ';' was expected");
		return NULL;
	}

	if(current(ctx) != ';')
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got token \"%.*s\" where \";\" was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	next(ctx);
	return expr;
}

static Node *makeStringExprNode(Context *ctx, const char *str, int len)
{
	if(str == NULL)
		str = "";

	if(len < 0)
		len = strlen(str);

	char *copy;
	int   copyl;
	{
		copy = BPAlloc_Malloc(ctx->alloc, len + 1);

		if(copy == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}

		memcpy(copy, str, len);
		copy[len] = '\0';
		copyl = len;
	}

	StringExprNode *node;
	{
		node = BPAlloc_Malloc(ctx->alloc, sizeof(StringExprNode));

		if(node == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
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

	return (Node*) node;
}

static Node *parse_string_primary_expression(Context *ctx)
{
	assert(ctx != NULL);
	
	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a string literal was expected");
		return NULL;
	}

	if(current(ctx) != TSTRING)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got token \"%.*s\" where a string literal was expected", ctx->token->length, ctx->src + ctx->token->offset);
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
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "String is too big to be rendered inside the fixed size buffer");
			return NULL;
		}

		memcpy(temp + temp_used, src + segm_off, segm_len);
		temp_used += segm_len;

		if(src[i] == '\\')
		{
			i += 1; // Consume the \.
						
			if(temp_used + 1 >= (int) sizeof(temp))
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_INTERNAL, "String is too big to be rendered inside the fixed size buffer");
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
					case '"': temp[temp_used++] = '"'; break;
					case 'n': temp[temp_used++] = '\n'; break;
					case 't': temp[temp_used++] = '\t'; break;
					case 'r': temp[temp_used++] = '\r'; break;
					case '\\': temp[temp_used++] = '\\'; break;
					case '\'': temp[temp_used++] = '\''; break;
								
					default:
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_SYNTAX, "Invalid escape sequence \\%c", src[i]);
					return NULL;
				}

				i += 1; // Consume the char after the \.
			}
		}
	}
	while(i < len);

	assert(temp_used < (int) sizeof(temp));

	temp[temp_used] = '\0';

	next(ctx);

	return makeStringExprNode(ctx, temp, temp_used);
}

static Node *parse_list_primary_expression(Context *ctx)
{
	assert(ctx != NULL);
	
	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a list literal was expected");
		return NULL;
	}

	if(current(ctx) != '[')
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got token \"%.*s\" where a list literal was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	int offset = current_token(ctx)->offset;

	Node *items = NULL;
	int   itemc = 0;
		
	next(ctx); // Skip the '['.

	if(current(ctx) != ']')
	{
		Node *tail = NULL;

		while(1)
		{
			// Parse.
			Node *item = parse_expression(ctx, 0, 1);

			if(item == NULL)
				return NULL;

			// Append.
			if(tail)
				tail->next = item;
			else
				items = item;
			tail = item;
			itemc += 1;

			// Get ',' or ']'.

			if(current(ctx) == ']')
				break;

			if(current(ctx) != ',')
			{
				*ctx->error_offset = ctx->token->offset;
				if(current(ctx) == TDONE)
					Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended inside a list literal");
				else
					Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" inside list literal, where ',' or ']' were expected", ctx->token->length, ctx->src + ctx->token->offset);
				return NULL;
			}

			next(ctx); // Skip the ','.
		}
	}

	int length = current_token(ctx)->offset
			   + current_token(ctx)->length
			   - offset;

	next(ctx); // Skip the ']'.

	ListExprNode *list;
	{
		list = BPAlloc_Malloc(ctx->alloc, sizeof(ListExprNode));

		if(list == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}

		list->base.base.kind = NODE_EXPR;
		list->base.base.next = NULL;
		list->base.base.offset = offset;
		list->base.base.length = length;
		list->base.kind = EXPR_LIST;
		list->items = items;
		list->itemc = itemc;
	}

	return (Node*) list;
						
}

static Node *parse_map_primary_expression(Context *ctx)
{
	assert(ctx != NULL);
	
	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, 
			"Source ended where a map literal was expected");
		return NULL;
	}

	if(current(ctx) != '{')
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, 
			"Got token \"%.*s\" where a map literal was expected", 
			ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	int offset = current_token(ctx)->offset;

	Node *keys  = NULL;
	Node *items = NULL;
	int   itemc = 0;
		
	next(ctx); // Skip the '{'.

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a map child item's key was expected");
		return NULL;
	}

	if(current(ctx) != '}')
	{
		Node *ktail = NULL, *tail = NULL;

		while(1)
		{
			Node *key;
			Node *item;

			bool item_is_func_decl = false;

			if (current(ctx) == TKWFUN) 
			{
				FuncDeclNode *func = parse_function_definition(ctx);
				if (func == NULL) return NULL;
				key  = (Node*) func->name;
				item = (Node*) func->expr;
				item_is_func_decl = true;
			}
			else
			{
				_Bool key_is_a_single_ident;
				{
					_Bool key_starts_with_an_ident = current(ctx) == TIDENT;

					next(ctx);

					key_is_a_single_ident = key_starts_with_an_ident && (current(ctx) == TDONE || current(ctx) == ':');

					prev(ctx);
				}

				if(key_is_a_single_ident)
				{
					assert(current(ctx) == TIDENT);
					key = makeStringExprNode(ctx, ctx->src + ctx->token->offset, ctx->token->length);

					next(ctx);
				}
				else
					key = parse_expression(ctx, 0, 1);

				if(key == NULL)
					return NULL;

				if(done(ctx))
				{
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_SYNTAX, 
						"Source ended where a map key-value "
						"separator ':' was expected");
					return NULL;
				}

				if(current(ctx) != ':')
				{
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_SYNTAX, 
						"Got token \"%.*s\" where a map key-value "
						"separator ':' was expected", 
						ctx->token->length, 
						ctx->src + ctx->token->offset);
					return NULL;
				}

				next(ctx);

				// Parse.
				item = parse_expression(ctx, 0, 1);
				if(item == NULL) return NULL;
			}

			// Append.
			if(tail)
			{
				ktail->next = key;
				tail->next = item;
			}
			else
			{
				keys = key;
				items = item;
			}

			ktail = key;
			tail = item;
			itemc += 1;

			// Get ',' or '}'.

			if(current(ctx) == '}')
				break;

			if (!item_is_func_decl) {
				if(current(ctx) != ',')
				{
					*ctx->error_offset = ctx->token->offset;
					if(current(ctx) == TDONE)
						Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended inside a map literal");
					else
						Error_Report(ctx->error, ErrorType_SYNTAX, 
							"Got unexpected token \"%.*s\" inside "
							"map literal, where ',' or '}' were expected", 
							ctx->token->length, ctx->src + ctx->token->offset);
					return NULL;
				}
				next(ctx); // Skip the ','.
			}
		}
	}

	int length = current_token(ctx)->offset
			   + current_token(ctx)->length
			   - offset;

	next(ctx); // Skip the ']'.

	MapExprNode *map;
	{
		map = BPAlloc_Malloc(ctx->alloc, sizeof(MapExprNode));

		if(map == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}

		map->base.base.kind = NODE_EXPR;
		map->base.base.next = NULL;
		map->base.base.offset = offset;
		map->base.base.length = length;
		map->base.kind = EXPR_MAP;
		map->keys = keys;
		map->items = items;
		map->itemc = itemc;
	}

	return (Node*) map;
						
}

static char *copy_token_text(Context *ctx)
{
	char *copy = BPAlloc_Malloc(ctx->alloc, ctx->token->length + 1);

	if(copy == NULL)
		return NULL;

	memcpy(copy, ctx->src + ctx->token->offset, ctx->token->length);

	copy[ctx->token->length] = '\0';

	return copy;
}

static Node *makeIdentExprNode(Context *ctx)
{
	char *copy  = copy_token_text(ctx);
	int   copyl = ctx->token->length;

	if(copy == NULL)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
		return NULL;
	}

	IdentExprNode *node;
	{
		node = BPAlloc_Malloc(ctx->alloc, sizeof(IdentExprNode));

		if(node == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
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

	return (Node*) node;
}

static Node *parse_primary_expresion(Context *ctx)
{
	assert(ctx != NULL);

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a primary expression was expected");
		return NULL;
	}
	

	switch(current(ctx))
	{
		case '[':
		return parse_list_primary_expression(ctx);

		case '{':
		return parse_map_primary_expression(ctx);

		case '(':
		{
			next(ctx); // Consume the '('.
				
			Node *node = parse_expression(ctx, 1, 1);
				
			if(node == NULL)
				return NULL;

			if(done(ctx))
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended before \")\", after sub-expression");
				return NULL;
			}

			if(current(ctx) != ')')
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_SYNTAX, "Missing \")\", after sub-expression");
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
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_INTERNAL, "Integer is too big");
				return NULL;
			}

			memcpy(buffer, ctx->src + ctx->token->offset, ctx->token->length);
			buffer[ctx->token->length] = '\0';

			errno = 0;

			long long int val = strtoll(buffer, NULL, 10);

			if(errno == ERANGE)
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_INTERNAL, "Integer is too big");
				return NULL;
			}
			else assert(errno == 0);

			IntExprNode *node;
			{
				node = BPAlloc_Malloc(ctx->alloc, sizeof(IntExprNode));

				if(node == NULL)
				{
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
					return NULL;
				}

				node->base.base.kind = NODE_EXPR;
				node->base.base.next = NULL;
				node->base.base.offset = ctx->token->offset;
				node->base.base.length = ctx->token->length;
				node->base.kind = EXPR_INT;
				node->val = val;
			}
				
			next(ctx);

			return (Node*) node;
		}

		case TFLOAT:
		{
			char buffer[64];

			if(ctx->token->length >= (int) sizeof(buffer))
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_INTERNAL, "Floating is too big");
				return NULL;
			}

			memcpy(buffer, ctx->src + ctx->token->offset, ctx->token->length);
			buffer[ctx->token->length] = '\0';

			errno = 0;

			double val = strtod(buffer, NULL);

			if(errno == ERANGE)
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_INTERNAL, "Floating is too big");
				return NULL;
			}
			else assert(errno == 0);

			FloatExprNode *node;
			{
				node = BPAlloc_Malloc(ctx->alloc, sizeof(FloatExprNode));

				if(node == NULL)
				{
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
					return NULL;
				}

				node->base.base.kind = NODE_EXPR;
				node->base.base.next = NULL;
				node->base.base.offset = ctx->token->offset;
				node->base.base.length = ctx->token->length;
				node->base.kind = EXPR_FLOAT;
				node->val = val;
			}

			next(ctx);
				
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
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
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
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
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
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
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
			Node *node = makeIdentExprNode(ctx);

			if(node == NULL)
				return NULL;

			next(ctx);

			return (Node*) node;
		}

		case TDONE:
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_INTERNAL, "Unexpected end of source where a primary expression was expected");
		return NULL;

		default:
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_INTERNAL, "Unexpected token \"%.*s\" where a primary expression was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	UNREACHABLE;
	return NULL;
}

static Node *makeIndexOrArrowSelectionExprNode(Context *ctx, bool arrow, Node *set, Node *idx)
{
	IndexSelectionExprNode *sel = BPAlloc_Malloc(ctx->alloc, sizeof(IndexSelectionExprNode));

	if(sel == NULL)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
		return NULL;
	}

	sel->base.base.kind = NODE_EXPR;
	sel->base.base.next = NULL;
	sel->base.base.offset = -1;
	sel->base.base.length = -1;
	sel->base.kind = arrow ? EXPR_ARW : EXPR_SELECT;
	sel->set = set;
	sel->idx = idx;
	return (Node*) sel;
}

static Node *parse_postfix_expression(Context *ctx)
{
	assert(ctx != NULL);

	Node *node = parse_primary_expresion(ctx);

	if(node == NULL)
		return NULL;

	while(1)
	{
		switch(current(ctx))
		{
			case '.':
			case TARW:
			{
				bool arrow = (current(ctx) == TARW);

				next(ctx);

				// We expect an identifier after the dot or arrow

				if(done(ctx))
				{
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended after dot or arrow");
					return NULL;
				}

				if(current(ctx) != TIDENT)
				{
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" after dot or arrow, where an identifier was expected", ctx->token->length, ctx->src + ctx->token->offset);
					return NULL;
				}

				Node *idx = makeStringExprNode(ctx, ctx->src + ctx->token->offset, ctx->token->length);
				if(idx == NULL)
					return NULL;

				Node *sel = makeIndexOrArrowSelectionExprNode(ctx, arrow, node, idx);
				if(sel == NULL)
					return NULL;

				sel->offset = node->offset;
				sel->length = idx->offset + idx->length - node->offset;

				next(ctx); // Get to the token after the identifier.

				node = (Node*) sel;
				break;
			}

			case '[':
			{
				Node *idx = parse_list_primary_expression(ctx);
				if(idx == NULL)
					return NULL;

				ListExprNode *ls = (ListExprNode*) idx;

				if(ls->itemc == 0)
				{
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_SYNTAX, "Missing index in index selection expression");
					return NULL;
				}

				Node *sel = makeIndexOrArrowSelectionExprNode(ctx, false, node, ls->itemc == 1 ? ls->items : (Node*) ls);
				if(sel == NULL)
					return NULL;

				sel->offset = node->offset;
				sel->length = idx->offset + idx->length - node->offset;

				node = (Node*) sel;
				break;
			}

			case '(':
			{
				int offset = current_token(ctx)->offset;

				Node *argv = NULL;
				int   argc = 0;
							
				next(ctx); // Skip the '('.

				if(current(ctx) != ')')
					while(1)
					{
						// Parse.
						Node *arg = parse_expression(ctx, 0, 1);

						if(arg == NULL)
							return NULL;

						// Append.
						arg->next = argv;
						argv = arg;
						argc += 1;

						// Get ',' or ')'.

						if(current(ctx) == ')')
							break;

						if(current(ctx) != ',')
						{
							*ctx->error_offset = ctx->token->offset;
							if(current(ctx) == TDONE)
								Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended inside a function argument list");
							else
								Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" where ',' or ')' were expected", ctx->token->length, ctx->src + ctx->token->offset);
							return NULL;
						}

						next(ctx); // Skip the ','.
					}

				int length = current_token(ctx)->offset
						   + current_token(ctx)->length
						   - offset;

				next(ctx); // Skip the ')'.

				CallExprNode *call;
				{
					call = BPAlloc_Malloc(ctx->alloc, sizeof(CallExprNode));

					if(call == NULL)
					{
						*ctx->error_offset = ctx->token->offset;
						Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
						return NULL;
					}

					call->base.base.kind = NODE_EXPR;
					call->base.base.next = NULL;
					call->base.base.offset = offset;
					call->base.base.length = length;
					call->base.kind = EXPR_CALL;
					call->func = node;
					call->argv = argv;
					call->argc = argc;
				}
				node = (Node*) call;
				break;
			}

			default: 
			goto done;
		} // End switch.
	} // End loop.
done:
	
	return node;
}

static Node *parse_prefix_expression(Context *ctx)
{
	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a prefix expression was expected");
		return NULL;
	}

	switch(current(ctx))
	{
		case TDONE:
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_INTERNAL, "Unexpected end of source where a prefix expression was expected");
		return NULL;

		case '+':
		case '-':
		case '?':
		case TKWNOT:
		{
			Token *unary_operator = current_token(ctx);

			next(ctx);

			Node *operand = parse_prefix_expression(ctx);
				
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
				temp->head = operand;
				temp->count = 1;

				switch(unary_operator->kind)
				{
					case '?': temp->base.kind = EXPR_NULLABLETYPE; break;
					case '+': temp->base.kind = EXPR_POS; break;
					case '-': temp->base.kind = EXPR_NEG; break;
					case TKWNOT: temp->base.kind = EXPR_NOT; break;
					default: assert(0); break;
				}
			}
			return (Node*) temp;
		}

		default:
		return parse_postfix_expression(ctx);
	}

	UNREACHABLE;
	return NULL;
}

static inline _Bool isbinop(Token *tok)
{
	assert(tok != NULL);

	return 	tok->kind == '+' || tok->kind == '-' || 
			tok->kind == '*' || tok->kind == '/' ||
			tok->kind == '<' || tok->kind == '>' ||
			tok->kind == TLEQ || tok->kind == TGEQ ||
			tok->kind == TEQL || tok->kind == TNQL ||
			tok->kind == TKWAND || tok->kind == TKWOR ||
			tok->kind == '=' || tok->kind == ',' ||
			tok->kind == '|' || tok->kind == '%';
}

static inline _Bool isrightassoc(Token *tok)
{
	assert(tok != NULL);

	return tok->kind == '=';
}

static inline int precedenceof(Token *tok)
{
	assert(tok != NULL);

	switch(tok->kind)
	{
		case '=':
		return 0;

		case '|':
		return 1;

		case TKWOR:
		return 2;

		case TKWAND:
		return 3;

		case '<':
		case '>':
		case TLEQ:
		case TGEQ:
		case TEQL:
		case TNQL:
		return 4;

		case '+':
		case '-':
		return 5;

		case '*':
		case '/':
		case '%':
		return 6;
		
		case ',':
		return 7;

		default:
		return -100000000;
	}

	UNREACHABLE;
	return -100000000;
}

static Node *parse_expression_2(Context *ctx, Node *left_expr, int min_prec, _Bool allow_toplev_tuples, _Bool allow_assignments)
{
	while(isbinop(ctx->token) && precedenceof(ctx->token) >= min_prec)
	{
		Token *op = ctx->token;

		if((op->kind == ',' && allow_toplev_tuples == 0) || (op->kind == '=' && allow_assignments == 0))
			break;

		next(ctx);

		Node *right_expr = parse_prefix_expression(ctx);

		if(right_expr == NULL)
			return NULL;

		while(isbinop(ctx->token) && (precedenceof(ctx->token) > precedenceof(op) || (precedenceof(ctx->token) == precedenceof(op) && isrightassoc(ctx->token))))
		{
			right_expr = parse_expression_2(ctx, right_expr, precedenceof(op) + 1, allow_toplev_tuples, allow_assignments);
			
			if(right_expr == NULL)
				return NULL;				
		
#warning "Should this break also trigger when the token is a = and allow_assignments is false?"
			if(ctx->token->kind == ',' && allow_toplev_tuples == 0)
				break;
		}

		OperExprNode *temp;
		{
			temp = BPAlloc_Malloc(ctx->alloc, sizeof(OperExprNode));

			if(temp == NULL)
			{
				Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
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
				case '%': temp->base.kind = EXPR_MOD; break;
				case '<': temp->base.kind = EXPR_LSS; break;
				case '>': temp->base.kind = EXPR_GRT; break;
				case TLEQ: temp->base.kind = EXPR_LEQ; break;
				case TGEQ: temp->base.kind = EXPR_GEQ; break;
				case TEQL: temp->base.kind = EXPR_EQL; break;
				case TNQL: temp->base.kind = EXPR_NQL; break;
				case TKWAND: temp->base.kind = EXPR_AND; break;
				case TKWOR: temp->base.kind = EXPR_OR; break;
				case '=': temp->base.kind = EXPR_ASS; break;
				case '|': temp->base.kind = EXPR_SUMTYPE; break;
				case ',': temp->base.kind = EXPR_PAIR; break;
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

static Node *parse_expression(Context *ctx, _Bool allow_toplev_tuples, _Bool allow_assignments)
{
	Node *left_expr = parse_prefix_expression(ctx);

	if(left_expr == NULL)
		return NULL;

	if(done(ctx))
		return left_expr;

	return parse_expression_2(ctx, left_expr, -1000000000, allow_toplev_tuples, allow_assignments);
}

static Node *parse_ifelse_statement(Context *ctx)
{
	assert(ctx != NULL);

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where an if-else statement was expected");
		return NULL;
	}

	if(current(ctx) != TKWIF)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" where an if-else statement was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	Token *if_token = current_token(ctx);
	assert(if_token != NULL);

	next(ctx); // Consume the "if" keyword.

	Node *condition = parse_expression(ctx, 1, 1);
	
	if(condition == NULL) 
		return NULL;

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended right after an if-else condition, where a ':' was expected");
		return NULL;
	}

	if(current(ctx) != ':')
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" after an if-else condition, where a ':' was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	next(ctx); // Skip the ':'.

	Node *true_branch = parse_statement(ctx);
	
	if(true_branch == NULL) 
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
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
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
	int end_offset = 0;
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
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended inside compound statement");
		return NULL;
	}

	CompoundNode *node;
	{
		node = BPAlloc_Malloc(ctx->alloc, sizeof(CompoundNode));
		
		if(node == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}

		node->base.kind = NODE_COMP;
		node->base.next = NULL;
		node->base.offset = head ? head->offset : 0;
		node->base.length = head ? end_offset - head->offset : 0;
		node->head = head;
	}

	return (Node*) node;
}

static _Bool parse_function_arguments(Context *ctx, int *argc_, Node **argv_)
{
#warning "What if the source ends here?"

	if(next(ctx) != '(')
	{
		*ctx->error_offset = ctx->token->offset;
		if(done(ctx))
			Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a function argument list was expected");
		else
			Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" where a function argument list was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return 0;
	}

	Node *argv = NULL;
	int   argc = 0;

	if(next(ctx) != ')')
	{
		// Parse arguments.

		while(1)
		{
			if(done(ctx))
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended inside a function argument list");
				return 0;
			}

			if(current(ctx) != TIDENT)
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" where a function argument name was expected", ctx->token->length, ctx->src + ctx->token->offset);
				return 0;
			}

			char *arg_name = copy_token_text(ctx);

			if(arg_name == NULL)
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
				return 0;
			}

			Node *type;
			if(next(ctx) == ':') {
				next(ctx); // Skip the ':'.
				type = parse_expression(ctx, 0, 0);
				if(type == NULL)
					return 0;
			} else
				type = NULL;

			Node *defarg; // Default argument (or NULL if there isn't one)
			if(current(ctx) == '=') {
				next(ctx); // Skip the '='.
				defarg = parse_expression(ctx, 0, 1);
				if(defarg == NULL)
					return 0;
			} else
				defarg = NULL;

			ArgumentNode *arg;
			{
				// Make argument node.
				arg = BPAlloc_Malloc(ctx->alloc, sizeof(ArgumentNode));

				if(arg == NULL)
				{
					*ctx->error_offset = ctx->token->offset;
					Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
					return 0;
				}

				arg->base.kind = NODE_ARG;
				arg->base.next = NULL;
				arg->base.offset = current_token(ctx)->offset;
				arg->base.length = current_token(ctx)->length;
				arg->name = arg_name;
				arg->type = type;
				arg->value = defarg;
			}

			// Add it to the list.
			argc += 1;
			arg->base.next = argv;
			argv = (Node*) arg;

			// Expect either ',' or ')'.

			if(done(ctx))
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended inside a function argument list");
				return 0;
			}

			if(current(ctx) == ')')
				break;

			if(current(ctx) != ',')
			{
				*ctx->error_offset = ctx->token->offset;
				Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" inside function argument list, where either ',' or ')' were expected", ctx->token->length, ctx->src + ctx->token->offset);
				return 0;
			}

			// Now prepare for the next identifier.
			next(ctx);
		}
	}

	next(ctx); // Consume the ')'.

	if(argc_ != NULL) *argc_ = argc;
	if(argv_ != NULL) *argv_ = argv;
	return 1;
}

static FuncDeclNode *parse_function_definition(Context *ctx)
{
	assert(ctx != NULL);

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a function definition was expected");
		return NULL;
	}

	if(current(ctx) != TKWFUN)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" where a function definition was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	int offset = current_token(ctx)->offset;

	if(next(ctx) != TIDENT)
	{
		*ctx->error_offset = ctx->token->offset;
		if(done(ctx))
			Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where an identifier was expected as function name");
		else
			Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" where an identifier was expected as function name", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	char *name_val = copy_token_text(ctx);
	if(name_val == NULL)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
		return NULL;
	}
	int name_len = strlen(name_val);
	int name_offset = current_token(ctx)->offset;
	int name_length = current_token(ctx)->length;

	int   argc = 0; // Initialization for the warning.
	Node *argv;
	if(!parse_function_arguments(ctx, &argc, &argv))
		return NULL;
	
	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended before function body");
		return NULL;
	}

	Node *body = parse_statement(ctx);

	if(body == NULL)
		return NULL;

	FuncDeclNode *func;
	{
		int length = body->offset 
				   + body->length 
				   - offset;

		FuncExprNode *expr = BPAlloc_Malloc(ctx->alloc, sizeof(FuncExprNode));
		if (expr == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}
		expr->base.base.kind = NODE_EXPR;
		expr->base.base.next = NULL;
		expr->base.base.offset = offset;
		expr->base.base.length = length;
		expr->base.kind = EXPR_FUNC;
		expr->argv = argv;
		expr->argc = argc;
		expr->body = body;

		StringExprNode *name = BPAlloc_Malloc(ctx->alloc, sizeof(StringExprNode));
		if (name == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}
		name->base.base.kind = NODE_EXPR;
		name->base.base.next = NULL;
		name->base.base.offset = name_offset;
		name->base.base.length = name_length;
		name->base.kind = EXPR_STRING;
		name->val = name_val;
		name->len = name_len;

		func = BPAlloc_Malloc(ctx->alloc, sizeof(FuncDeclNode));
		if(func == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}
		func->base.kind = NODE_FUNC;
		func->base.next = NULL;
		func->base.offset = offset;
		func->base.length = length;
		func->name = name;
		func->expr = expr;
	}

	return func;
}

static Node *parse_while_statement(Context *ctx)
{
	assert(ctx != NULL);

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a while statement was expected");
		return NULL;
	}

	if(current(ctx) != TKWWHILE)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" where a while statement was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	Token *while_token = current_token(ctx);
	assert(while_token != NULL);

	next(ctx); // Consume the "while" keyword.

	Node *condition = parse_expression(ctx, 1, 1);
	
	if(condition == NULL) 
		return NULL;

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended right after a while loop condition, where a ':' was expected");
		return NULL;
	}

	if(current(ctx) != ':')
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" after a while loop condition, where a ':' was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	next(ctx); // Skip the ':'.

	Node *body = parse_statement(ctx);
	
	if(body == NULL) 
		return NULL;

	WhileNode *whl;
	{
		whl = BPAlloc_Malloc(ctx->alloc, sizeof(WhileNode));
		
		if(whl == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}

		whl->base.kind = NODE_WHILE;
		whl->base.next = NULL;
		whl->base.offset = while_token->offset;
		whl->base.length = ctx->token->offset + ctx->token->length - while_token->offset;
		whl->condition = condition;
		whl->body = body;
	}

	return (Node*) whl;
}

static Node *parse_dowhile_statement(Context *ctx)
{
	assert(ctx != NULL);

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended where a do-while statement was expected");
		return NULL;
	}

	if(current(ctx) != TKWDO)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" where a do-while statement was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	Token *do_token = current_token(ctx);
	assert(do_token != NULL);

	next(ctx); // Consume the "do" keyword.

	Node *body = parse_statement(ctx);
	
	if(body == NULL) 
		return NULL;

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended right after a do-while body, where the \"while\" keyword was expected");
		return NULL;
	}

	if(current(ctx) != TKWWHILE)
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" after a do-while body, where the \"while\" keyword was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	next(ctx); // Consume the "while" keyword.

	Node *condition = parse_expression(ctx, 1, 1);
	
	if(condition == NULL) 
		return NULL;

	if(done(ctx))
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Source ended right after a do-while condition, where a ';' was expected");
		return NULL;
	}

	if(current(ctx) != ';')
	{
		*ctx->error_offset = ctx->token->offset;
		Error_Report(ctx->error, ErrorType_SYNTAX, "Got unexpected token \"%.*s\" after a do-while conditnion, where a ';' was expected", ctx->token->length, ctx->src + ctx->token->offset);
		return NULL;
	}

	next(ctx); // Skip the ';'.

	DoWhileNode *dowhl;
	{
		dowhl = BPAlloc_Malloc(ctx->alloc, sizeof(DoWhileNode));
		
		if(dowhl == NULL)
		{
			*ctx->error_offset = ctx->token->offset;
			Error_Report(ctx->error, ErrorType_INTERNAL, "No memory");
			return NULL;
		}

		dowhl->base.kind = NODE_DOWHILE;
		dowhl->base.next = NULL;
		dowhl->base.offset = do_token->offset;
		dowhl->base.length = ctx->token->offset + ctx->token->length - do_token->offset;
		dowhl->condition = condition;
		dowhl->body = body;
	}

	return (Node*) dowhl;
}