
/* Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>
**
** This file is part of The Noja Interpreter.
**
** The Noja Interpreter is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at 
** your option) any later version.
**
** The Noja Interpreter is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of 
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
** Public License for more details.
**
** You should have received a copy of the GNU General Public License along 
** with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.
*/

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