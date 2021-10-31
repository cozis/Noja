#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "bucketlist.h"
#include "defs.h"

#define MIN_BUCKET_SIZE 4096

typedef struct Bucket Bucket;
struct Bucket {
	Bucket*	next;
	int 	size, 
			used, 
			aidx;
	char    body[];
};

struct xBucketList {
	BPAlloc *alloc;
	Bucket  *head, 
		    *tail;
	int 	 size;
};

BucketList *BucketList_New(BPAlloc *alloc)
{
	assert(alloc != NULL);

	BucketList *blist = BPAlloc_Malloc(alloc, sizeof(BucketList) + sizeof(Bucket) + MIN_BUCKET_SIZE);

	if(blist == NULL)
		return NULL;

	Bucket *head = (Bucket*) (blist + 1);
	head->next = NULL;
	head->size = MIN_BUCKET_SIZE;
	head->used = 0;
	head->aidx = 0;

	blist->alloc = alloc;
	blist->head = head;
	blist->tail = head;
	blist->size = 0;
	return blist;	
}

int BucketList_Size(BucketList *blist)
{
	return blist->size;
}

static Bucket *make_bucket(BPAlloc *alloc, int size)
{
	Bucket *new_bucket = BPAlloc_Malloc(alloc, sizeof(Bucket) + size);
						
	if(new_bucket == NULL)
		return NULL;
	
	// Initialize it.
	new_bucket->next = NULL;
	new_bucket->size = size;
	new_bucket->used = 0;
	new_bucket->aidx = -1;
	return new_bucket;
}

static void append_bucket(BucketList *blist, Bucket *new_bucket)
{
	new_bucket->aidx = blist->size;
	blist->tail->next = new_bucket;
	blist->tail = new_bucket;
}

_Bool BucketList_Append(BucketList *blist, const void *data, int size)
{
	assert(blist != NULL);
	assert(size >= 0);

	int not_copied_yet = size;

	while(not_copied_yet > 0)
		{
			// Copy until there's nothing left
			// or until the current bucket is
			// full. If the bucket is already
			// full, add another one.

			int left_in_bucket = blist->tail->size - blist->tail->used;

			if(left_in_bucket == 0)
				{
					Bucket *new_bucket = make_bucket(blist->alloc, MIN_BUCKET_SIZE);

					if(new_bucket == NULL)
						return 0;

					append_bucket(blist, new_bucket);
				}

			// Decide how much to copy.
			int copying = MIN(not_copied_yet, left_in_bucket);

			// Copy into the bucket.
			{
				char *dst = blist->tail->body + blist->tail->used;

				if(data == NULL)
					memset(dst, 0, copying);
				else
					memcpy(dst, data + size - not_copied_yet, copying);

				blist->tail->used += copying;
			}

			not_copied_yet -= copying;				
		}

	blist->size += size;
	return 1;
}

void *BucketList_Append2(BucketList *blist, const void *data, int size)
{
	assert(blist != NULL);
	assert(size >= 0);

	// If the data doesn't fit inside the
	// current bucket, add another one with
	// enough space.
	if(blist->tail->used + size > blist->tail->size)
		{
			int bucket_size = MAX(MIN_BUCKET_SIZE, size);

			Bucket *new_bucket = make_bucket(blist->alloc, bucket_size);

			if(new_bucket == NULL)
				return 0;

			append_bucket(blist, new_bucket);
		}

	void *addr = blist->tail->body + blist->tail->used;

	// Do the copying.
	if(data == NULL)
		memset(addr, 0, size);
	else
		memcpy(addr, data, size);
	
	blist->tail->used += size;
	blist->size += size;
	return addr;
}

void BucketList_Copy(BucketList *blist, void *dest, int len)
{
	assert(blist != NULL);
	assert(dest != NULL);

	if(len < 0)
		len = blist->size;

	int copied = 0;

	Bucket *bucket = blist->head;

	while(bucket && copied < len)
		{
			int copying = MIN(len - copied, bucket->used);
			assert(copying >= 0);

			memcpy((char*) dest + copied, bucket->body, copying);

			copied += copying;
			bucket = bucket->next;
		}
}