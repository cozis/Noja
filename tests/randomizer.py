
from random import randint

NODE_KINDS = ['expr', 'if-else', 'while', 'do-while', 'return', 'comp', 'func']
EXPR_KINDS = ['int', 'float', 'string', 'array', 'object']

def generate_ident():
	return 'Apple'

def generate_string_expr_node():
	return 'Hello, world!'

def generate_node(kind=None, depth=0):

	if kind == None and depth > 3:
		return {
			'kind': 'comp',
			'body': []
		}

	if kind == None:
		kind = NODE_KINDS[randint(0, len(NODE_KINDS)-1)]

	assert kind in NODE_KINDS

	if kind == 'expr':
	
		return generate_expr_node(None, depth+1)

	elif kind == 'if-else':
		
		branches = randint(1, 2)
		assert branches in [1, 2]

		if branches == 1:
			return {
				'kind': 'if-else', 
				'cond': generate_expr_node(None, depth+1),
				'if-case': generate_node(None, depth+1)
			}
		else:
			return {
				'kind': 'if-else', 
				'cond': generate_expr_node(None, depth+1),
				'if-case': generate_node(None, depth+1), 
				'else-case': generate_node(None, depth+1)
			}

	elif kind == 'while':
		
		return {
			'kind': 'while',
			'cond': generate_expr_node(None, depth+1),
			'body': generate_node(None, depth+1)
		}

	elif kind == 'do-while':
		
		return {
			'kind': 'do-while',
			'cond': generate_expr_node(None, depth+1),
			'body': generate_node(None, depth+1)
		}

	elif kind == 'return':
		
		return {
			'kind': 'return',
			'value': generate_expr_node(None, depth+1)
		}

	elif kind == 'comp':

		n = randint(0, 3)

		return {
			'kind': 'comp',
			'body': [generate_node(None, depth+1) for i in range(n)]
		}

	elif kind == 'func':
		
		argc = randint(0, 3)

		return {
			'kind': 'func',
			'name': generate_ident(),
			'args': [generate_ident() for i in range(argc)],
			'body': generate_node(None, depth+1)
		}

def generate_expr_node(expr_kind=None, depth=0):

	if expr_kind == None:
		expr_kind = EXPR_KINDS[randint(0, len(EXPR_KINDS)-1)]

	assert expr_kind in EXPR_KINDS

	if depth > 3:
		expr_kind = ['int', 'float', 'string'][randint(0, 2)]

	if expr_kind == 'int':

		return {
			'kind': 'expr',
			'expr-kind': 'int',
			'value': randint(0, 100)
		}

	elif expr_kind == 'float':

		return {
			'kind': 'expr',
			'expr-kind': 'float',
			'value': float(str(randint(0, 100)) + '.' + str(randint(0, 100)))
		}

	elif expr_kind == 'string':

		return {
			'kind': 'expr',
			'expr-kind': 'string',
			'value': generate_string_expr_node()
		}

	elif expr_kind == 'array':

		n = randint(0, 3)

		return {
			'kind': 'expr',
			'expr-kind': 'array',
			'items': [generate_expr_node(None, depth+1) for i in range(n)]
		}

	elif expr_kind == 'object':

		n = randint(0, 3)

		return {
			'kind': 'expr',
			'expr-kind': 'object',
			'items': [(generate_expr_node(None, depth+1), generate_expr_node(None, depth+1)) for i in range(n)]
		}

def serialize_expr(node):

	if node['expr-kind'] == 'int':
			
		return str(node['value'])

	elif node['expr-kind'] == 'float':
		
		return str(node['value'])

	elif node['expr-kind'] == 'string':
		
		return '"' + node['value'] + '"'

	elif node['expr-kind'] == 'array':
		
		return '[' + ', '.join([serialize_expr(item) for item in node['items']]) + ']'

	elif node['expr-kind'] == 'object':
		
		return '{' + ', '.join([serialize_expr(item[0]) + ': ' + serialize_expr(item[1]) for item in node['items']]) + '}'

def serialize(node):

	if node['kind'] == 'expr':

		return serialize_expr(node) + ';'

	elif node['kind'] == 'if-else':

		if 'else-case' in node:
			return 'if ' + serialize_expr(node['cond']) + ': ' + serialize(node['if-case']) + ' else ' + serialize(node['else-case'])
		else:
			return 'if ' + serialize_expr(node['cond']) + ': ' + serialize(node['if-case'])

	elif node['kind'] == 'while':

		return 'while ' + serialize_expr(node['cond']) + ': ' + serialize(node['body'])

	elif node['kind'] == 'do-while':

		return 'do ' + serialize(node['body']) + ' while ' + serialize_expr(node['cond']) + ';'

	elif node['kind'] == 'return':

		return 'return ' + serialize_expr(node['value']) + ';'

	elif node['kind'] == 'comp': 

		return '{' + ''.join([serialize(stmt) for stmt in node['body']]) + '}'

	elif node['kind'] == 'func':

		return 'fun ' + node['name'] + '(' + ', '.join(node['args']) + ') ' + serialize(node['body'])

print(serialize(generate_node()))