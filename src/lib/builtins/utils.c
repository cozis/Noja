#include "utils.h"

int returnValuesVA(Error *error, Heap *heap, Object *rets[static MAX_RETS], const char *fmt, va_list va)
{
	int retc = 0, i = 0;
	while (fmt[i] != '\0') {

		if (retc == MAX_RETS) {
			Error_Report(error, 1, "Return value limit reached");
			return -1;
		}
		
		Object *ret;
		switch (fmt[i]) {
			case 'o': ret = va_arg(va, Object*); break;
			case 'n': ret = Object_NewNone(heap, error); break;
			case 'i': ret = Object_FromInt  (va_arg(va, int),    heap, error); break;
			case 'f': ret = Object_FromFloat(va_arg(va, double), heap, error); break;
			case 's': ret = Object_FromString(va_arg(va, char*), -1, heap, error); break;
			default:
			Error_Report(error, 1, "Invalid format specifier '%c'", fmt[i]);
			return -1;
		}

		if (ret == NULL)
			return -1;

		rets[retc++] = ret;
		i++;
	}
	return retc;
}

bool parseArgs(Error *error, 
			   Object **argv, 
			   unsigned int argc, 
			   ParsedArgument *pargs, 
			   const char *fmt)
{
	unsigned int current_arg = 0;
	int i = 0;
	while (fmt[i] != '\0') {

		if (current_arg == argc) {
			Error_Report(error, 0, "Missing arguments");
			return false;
		}
		Object *arg = argv[current_arg];

		bool may_be_none = false;
		if (fmt[i] == '?') {
			may_be_none = true;
			i++;
			if (fmt[i] == '\0') {
				Error_Report(error, 1, "Format terminated unexpectedly");
				return false;
			}
		}

		if (may_be_none && Object_IsNone(arg)) {
			pargs[current_arg].defined = false;
		} else {
			switch (fmt[i]) {
				
				case 'o': /* Any object */
				pargs[current_arg].defined = true;
				break;
				
				case 'b': /* Boolean */ 
				if (!Object_IsBool(arg)) {
					Error_Report(error, 0, "Argument %d was expected to be bool, but a %s was provided", current_arg+1, arg->type->name);
					return false;
				}
				pargs[current_arg].defined = true;
				pargs[current_arg].as_bool = Object_GetBool(arg);
				break;

				case 'B': /* Buffer */
				{
					if (!Object_IsBuffer(arg)) {
						Error_Report(error, 0, "Argument %d was expected to be a buffer, but a %s was provided", current_arg+1, arg->type->name);
						return false;
					}
					void  *data;
					size_t size;
					data = Object_GetBuffer(arg, &size);
					pargs[current_arg].defined = true;
					pargs[current_arg].as_buffer.data = data;
					pargs[current_arg].as_buffer.size = size;
					break;
				}

				case 'i': /* Integer */
				if (!Object_IsInt(arg)) {
					Error_Report(error, 0, "Argument %d was expected to be an int, but a %s was provided", current_arg+1, arg->type->name);
					return false;
				}
				pargs[current_arg].defined = true;
				pargs[current_arg].as_int = Object_GetInt(arg);
				break;

				case 'f': /* Float */
				if (!Object_IsFloat(arg)) {
					Error_Report(error, 0, "Argument %d was expected to be a float, but a %s was provided", current_arg+1, arg->type->name);
					return false;
				}
				pargs[current_arg].defined = true;
				pargs[current_arg].as_float = Object_GetFloat(arg);
				break;

				case 'l': /* List */
				if (!Object_IsList(arg)) {
					Error_Report(error, 0, "Argument %d was expected to be a list, but a %s was provided", current_arg+1, arg->type->name);
					return false;
				}
				pargs[current_arg].defined = true;
				break;

				case 'm': /* Map */
				if (!Object_IsMap(arg)) {
					Error_Report(error, 0, "Argument %d was expected to be a map, but a %s was provided", current_arg+1, arg->type->name);
					return false;
				}
				break;

				case 's': /* String */
				{
					if (!Object_IsString(arg)) {
						Error_Report(error, 0, "Argument %d was expected to be a string, but a %s was provided", current_arg+1, arg->type->name);
						return false;
					}
					const void *data;
					size_t size;
					data = Object_GetString(arg, &size);
					pargs[current_arg].defined = true;
					pargs[current_arg].as_string.data = data;
					pargs[current_arg].as_string.size = size;
					break;
				}

				default:
				Error_Report(error, 1, "Invalid argument parser format specifier");
				return false;
			}
		}
		i++;
		current_arg++;
	}

#ifndef NDEBUG
	if (current_arg < argc) {
		// Ignoring part of the format string
	}
#endif
	return true;
}

int returnValues2(Error *error, Runtime *runtime, Object *rets[static MAX_RETS], const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	int retc = returnValuesVA(error, Runtime_GetHeap(runtime), rets, fmt, va);
	va_end(va);
	return retc;
}

int returnValues(Error *error, Heap *heap, Object *rets[static MAX_RETS], const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	int retc = returnValuesVA(error, heap, rets, fmt, va);
	va_end(va);
	return retc;
}