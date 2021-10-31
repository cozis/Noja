#include <assert.h>
#include <xjson.h>
#include "serialize.h"
#include "ASTi.h"

#define UNREACHABLE assert(0)

static xj_value *convert_node(Node *node, xj_error *error, xj_alloc **alloc);

char *serialize(AST *ast, int *len)
{
	xj_alloc *alloc = NULL;
	xj_error  error;

	xj_value *value = convert_node(ast->root, &error, &alloc);

	if(value == NULL)
		{
			xj_free(alloc);
			return NULL;
		}
	else
		{
			char *serialized = xj_encode(value, len);
			xj_free(alloc);
			return serialized;
		}
}

static const char *expr_kind_name(ExprNode *expr)
{
	switch(expr->kind)
		{
			case EXPR_POS: return "pos";
			case EXPR_NEG: return "neg";
			case EXPR_ADD: return "add";
			case EXPR_SUB: return "sub";
			case EXPR_MUL: return "mul";
			case EXPR_DIV: return "div";
			case EXPR_INT: return "int";
			case EXPR_FLOAT: return "float";
			case EXPR_STRING: return "string";
			case EXPR_IDENT: return "ident";

			default:
			UNREACHABLE;
			return NULL;
		}

	UNREACHABLE;
	return NULL;
}

static xj_value *convert_node_list(Node *head, xj_error *error, xj_alloc **alloc);

static xj_value *convert_node(Node *node, xj_error *error, xj_alloc **alloc)
{
	switch(node->kind)
		{
			case NODE_EXPR:
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

						xj_value *operands = NULL;

						if(oper->count > 0)
							{
								operands = convert_node_list((Node*) oper->head, error, alloc);
						
								if(operands == NULL)
									return NULL;
							}

						return xj_decodef(error, alloc,
							"{"
							"\t\"node-kind\": \"expr\", \n"
							"\t\"expr-kind\": %Q, \n"
							"\t\"operands\": %L\n"
							"}", expr_kind_name(expr), operands);
					}

					case EXPR_INT:
					return xj_decodef(error, alloc,
						"{"
						"\t\"node-kind\": \"expr\", \n"
						"\t\"expr-kind\": %Q, \n"
						"\t\"value\": %d\n"
						"}", expr_kind_name(expr), ((IntExprNode*) expr)->val);

					case EXPR_FLOAT:
					return xj_decodef(error, alloc,
						"{"
						"\t\"node-kind\": \"expr\", \n"
						"\t\"expr-kind\": %Q, \n"
						"\t\"value\": %f\n"
						"}", expr_kind_name(expr), ((FloatExprNode*) expr)->val);
				
					case EXPR_STRING:
					return xj_decodef(error, alloc,
						"{"
						"\t\"node-kind\": \"expr\", \n"
						"\t\"expr-kind\": %Q, \n"
						"\t\"value\": %Q\n"
						"}", expr_kind_name(expr), ((StringExprNode*) expr)->val);
				
					case EXPR_IDENT:
					return xj_decodef(error, alloc,
						"{"
						"\t\"node-kind\": \"expr\", \n"
						"\t\"expr-kind\": %Q, \n"
						"\t\"value\": %Q\n"
						"}", expr_kind_name(expr), ((IdentExprNode*) expr)->val);
					
					default:
					UNREACHABLE;
					break;
				}
			break;


			default:
			UNREACHABLE;
			break;
		}

	UNREACHABLE;
	return NULL;
}

static xj_value *convert_node_list(Node *head, xj_error *error, xj_alloc **alloc)
{
	xj_value  *json_head = NULL;
	xj_value **json_tail = &json_head;

	Node *curs = head;
	while(curs)
		{
			xj_value *temp = convert_node(curs, error, alloc);
	
			if(temp == NULL)
				return NULL;
			
			temp->next = NULL;
			
			*json_tail = temp;
			json_tail = &temp->next;

			curs = curs->next;
		}

	return json_head;
}