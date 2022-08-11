#ifndef LABELLIST_H
#define LABELLIST_H
#include <stddef.h>
#include <stdbool.h>
#include "bpalloc.h"
#include "promise.h"
typedef struct LabelList LabelList;
LabelList *LabelList_New(BPAlloc *alloc);
void       LabelList_Free(LabelList *list);
bool       LabelList_SetLabel(LabelList *list, const char *name, size_t name_len, long long int value);
Promise   *LabelList_GetLabel(LabelList *list, const char *name, size_t name_len);
size_t     LabelList_GetUnresolvedCount(LabelList *list);
#endif /* LABELLIST_H */