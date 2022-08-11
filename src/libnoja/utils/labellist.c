#include <string.h>
#include "labellist.h"

typedef struct LabelInfo LabelInfo;
struct LabelInfo {
    const char *name;
    size_t  name_len;
    Promise *promise;
    LabelInfo  *next;
};

struct LabelList {
    BPAlloc *alloc;
    LabelInfo *head;
};

LabelList *LabelList_New(BPAlloc *alloc)
{
    LabelList *list = BPAlloc_Malloc(alloc, sizeof(LabelList));
    if(list != NULL) {
        list->head = NULL;
        list->alloc = alloc;
    }
    return list;
}

void LabelList_Free(LabelList *list)
{
    LabelInfo *info = list->head;
    while(info != NULL) {
        Promise_Free(info->promise);
        info = info->next;
    }
    list->head = NULL;
}

bool LabelList_SetLabel(LabelList *list, const char *name, size_t name_len, long long int value)
{
    Promise *promise = LabelList_GetLabel(list, name, name_len);
    if(promise == NULL)
        return false;
    Promise_Resolve(promise, &value, sizeof(value));
    return true;
}

Promise *LabelList_GetLabel(LabelList *list, const char *name, size_t name_len)
{
    // Find the label with the given name.
    {
        LabelInfo *info = list->head;
        while(info != NULL) {
            
            if(name_len == info->name_len 
                && strncmp(name, info->name, name_len) == 0)
                return info->promise;

            info = info->next;
        }
    }

    // No such label. Create a new one.
    LabelInfo *new_info;
    {
        BPAlloc *alloc = list->alloc;
        Promise *promise = Promise_New(alloc, sizeof(long long int));
        if(promise == NULL)\
            return NULL;

        new_info = BPAlloc_Malloc(alloc, sizeof(LabelInfo));
        if(new_info == NULL) {
            Promise_Free(promise);
            return NULL;
        }

        new_info->next = NULL;
        new_info->name = name;
        new_info->name_len = name_len;
        new_info->promise = promise;
    }

    // Add it to the list
    new_info->next = list->head;
    list->head = new_info;

    return new_info->promise;
}

size_t LabelList_GetUnresolvedCount(LabelList *list)
{
    size_t count = 0;

    LabelInfo *info = list->head;
    while(info != NULL) {
        count += !Promise_hasResolved(info->promise);
        info = info->next;
    }
    
    return count;
}