
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

#ifndef BPALLOC_H
#define BPALLOC_H
typedef struct xBPAlloc BPAlloc;
BPAlloc*	BPAlloc_Init(int chunk_size);
BPAlloc*	BPAlloc_Init2(int first_size, int chunk_size, void *userp, void *(*fn_malloc)(void *userp, int size), void  (*fn_free  )(void *userp, void *addr));
BPAlloc*	BPAlloc_Init3(void *mem, int mem_size, int chunk_size, void *userp, void *(*fn_malloc)(void *userp, int size), void  (*fn_free  )(void *userp, void *addr));
void 	 	BPAlloc_Free(BPAlloc *alloc);
void*		BPAlloc_Malloc(BPAlloc *alloc, int req_size);
#endif