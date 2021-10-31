#ifndef STACK_H
#define STACK_H
typedef struct xStack Stack;
void 		*Stack_New(int size);
void 		*Stack_Top(Stack *s, int n);
_Bool 		 Stack_Pop(Stack *s, unsigned int n);
_Bool 		 Stack_Push(Stack *s, void *item);
void 		 Stack_Free(Stack *s);
unsigned int Stack_Size(Stack *s);
unsigned int Stack_Capacity(Stack *s);
#endif