#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "buildvalue.h"
#include "eval.h"

static Object *walkValue(const char *fmt, int *i, va_list va, Heap *heap, Error *error);

static Object *walkMapValue(const char *fmt, int *i, va_list va, Heap *heap, Error *error)
{
	assert(fmt[*i] == '{');

	*i += 1; // Skip the '{'.

	while(isspace(fmt[*i]))
		*i += 1;

	Object *map = Object_NewMap(-1, heap, error);

	if(map == NULL)
		return NULL;

	if(fmt[*i] != '}')
		while(1)
			{
				Object *key = walkValue(fmt, i, va, heap, error);

				if(key == NULL)
					return NULL;

				while(isspace(fmt[*i]))
					*i += 1;

				if(fmt[*i] == '\0')
					{
						Error_Report(error, 0, "Format ended inside a map expression");
						return NULL;
					}

				if(fmt[*i] != ':')
					{
						Error_Report(error, 0, "Got unexpected '%c' in format where ':' was expected", fmt[*i]);
						return NULL;
					}

				*i += 1; // Skip the ':'.

				Object *val = walkValue(fmt, i, va, heap, error);

				if(val == NULL)
					return NULL;

				if(!Object_Insert(map, key, val, heap, error))
					return NULL;

				while(isspace(fmt[*i]))
					*i += 1;

				if(fmt[*i] == '\0')
					{
						Error_Report(error, 0, "Format ended inside a map expression");
						return NULL;
					}

				if(fmt[*i] == '}')
					break;

				if(fmt[*i] != ',')
					{
						Error_Report(error, 0, "Got unexpected '%c' in format where either ',' or '}' were expected", fmt[*i]);
						return NULL;
					}

				*i += 1; // Skip the ','.
			}

	assert(fmt[*i] == '}');

	*i += 1; // Skip the '}'.
	
	return map;
}
static Object *walkListValue(const char *fmt, int *i, va_list va, Heap *heap, Error *error)
{
	assert(fmt[*i] == '[');

	*i += 1; // Skip the '['.

	while(isspace(fmt[*i]))
		*i += 1;

	Object *maybe[8], **ptrs = maybe;
	int ptrs_cap = sizeof(maybe) / sizeof(maybe[0]), 
		ptrs_num = 0;

	if(fmt[*i] != ']')
		while(1)
			{
				Object *item = walkValue(fmt, i, va, heap, error);

				if(item == NULL)
					{
						if(ptrs != maybe) free(ptrs);
						return NULL;
					}

				if(ptrs_cap == ptrs_num)
					{
						int new_capacity = ptrs_cap * 2;
						
						Object **temp;

						if(ptrs == maybe)
							{
								temp = malloc(sizeof(Object*) * new_capacity);
								
								if(temp != NULL)
									memcpy(temp, ptrs, ptrs_num * sizeof(Object*));	
							}
						else
							temp = realloc(ptrs, sizeof(Object*) * new_capacity);

						if(temp == NULL)
							{
								Error_Report(error, 1, "Insufficient memory");
								if(ptrs != maybe) free(ptrs);
								return NULL;
							}

						ptrs = temp;
						ptrs_cap = new_capacity;
					}

				ptrs[ptrs_num++] = item;

				while(isspace(fmt[*i]))
					*i += 1;

				if(fmt[*i] == '\0')
					{
						Error_Report(error, 0, "Format ended inside a list expression");
						if(ptrs != maybe) free(ptrs);
						return NULL;
					}

				if(fmt[*i] == ']')
					break;

				if(fmt[*i] != ',')
					{
						Error_Report(error, 0, "Got unexpected '%c' in format where either ',' or ']' were expected", fmt[*i]);
						if(ptrs != maybe) free(ptrs);
						return NULL;
					}

				*i += 1; // Skip the ','.
			}

	assert(fmt[*i] == ']');

	*i += 1; // Skip the ']'.

	Object *list = Object_NewList2(ptrs_num, ptrs, heap, error);

	if(ptrs != maybe) 
		free(ptrs);
	
	return list;
}

static Object *walkValue(const char *fmt, int *i, va_list va, Heap *heap, Error *error)
{
	while(isspace(fmt[*i]))
		*i += 1;

	if(fmt[*i] == '\0')
		{
			Error_Report(error, 0, "Empty format");
			return NULL;
		}

	if(fmt[*i] == '{')
		return walkMapValue(fmt, i, va, heap, error);

	if(fmt[*i] == '[')
		return walkListValue(fmt, i, va, heap, error);

	if(fmt[*i] != '$')
		{
			Error_Report(error, 0, "Unexpected character '%c' in format", fmt[*i]);
			return NULL;
		}

	assert(fmt[*i] == '$');

	*i += 1;

	Object *o;

	switch(fmt[*i])
		{
			case '\0':
			Error_Report(error, 0, "Format ended after '$'");
			return NULL;
		
			case '{':
			{
				// Noja expression until the closing '}'.
				
				*i += 1; // Skip the '{'.

				int start = *i;

				while(fmt[*i] != '\0' && fmt[*i] != '}') // TODO: Escape '}'.
					*i += 1;

				if(fmt[*i] == '\0')
					{
						// ERROR: Format ended inside of ${..}.
						Error_Report(error, 0, "Format ended inside ${..}");
						return NULL;
					}

				assert(fmt[*i] == '}');

				int end = *i;

				// NOTE: The last '}' is skipped after the switch.
				
				const char *src = fmt + start;
				int         len = end - start;
				
				o = eval(src, len, NULL, heap, error);

				if(o == NULL)
					return NULL;
				break;
			}

			case 'i':
			o = Object_FromInt(va_arg(va, long long int), heap, error);
			if(o == NULL) return NULL;
			break;

			case 'f':
			o = Object_FromFloat(va_arg(va, double), heap, error);
			if(o == NULL) return NULL;
			break;

			case 'b':
			// NOTE: Actually we want a _Bool, not an int.
			//       But gcc says:
			//       > warning: ‘_Bool’ is promoted to ‘int’ when passed through ‘...’
			//       > [...]
			//       > (so you should pass ‘int’ not ‘_Bool’ to ‘va_arg’)
			//
			o = Object_FromBool(va_arg(va, int), heap, error);
			if(o == NULL) return NULL;
			break;

			case 's':
			o = Object_FromString(va_arg(va, char*), -1, heap, error);
			if(o == NULL) return NULL;
			break;

			case 'o':
			o = va_arg(va, Object*);
			assert(o != NULL);
			break;

			default:
			Error_Report(error, 0, "Invalid format specifier '%c'", fmt[*i]);
			return NULL;
		}

	*i += 1;
	return o;
}

Object *buildValue2(Heap *heap, Error *error, const char *fmt, va_list va)
{
	int i = 0;
	return walkValue(fmt, &i, va, heap, error);
}

Object *buildValue(Heap *heap, Error *error, const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	Object *o = buildValue2(heap, error, fmt, va);
	va_end(va);
	return o;
}