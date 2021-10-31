#ifndef BUCKETLIST_H
#define BUCKETLIST_H
#include "bpalloc.h"
typedef struct xBucketList BucketList;
BucketList 	*BucketList_New(BPAlloc *alloc);
int 		 BucketList_Size(BucketList *blist);
void		 BucketList_Copy(BucketList *blist, void *dest, int len);
_Bool 		 BucketList_Append (BucketList *blist, const void *data, int size);
void 		*BucketList_Append2(BucketList *blist, const void *data, int size);
BPAlloc 	*BucketList_GetAlloc(BucketList *blist);
#endif