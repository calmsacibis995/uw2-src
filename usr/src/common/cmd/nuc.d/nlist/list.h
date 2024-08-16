/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlist:list.h	1.2"
#ifndef LIST_H
#define LIST_H

#include <stddef.h>

#define TRUE				1
#define FALSE				0
#define FAILED				-1
#define SUCCESS				0
#define FREE_DATA			0x01110001

typedef struct SL_LIST {
	struct SL_LIST_NODE*	head;
	struct SL_LIST_NODE*	tail;
	int						count;
	int						type;
	void*					data;
	void					(*freeFunc)();
	int						(*cmpFunc)();
} SL_LIST_T;

typedef struct SL_LIST_NODE {
	struct SL_LIST*			list;
	struct SL_LIST_NODE*	next;
	char*					data;
} SL_LIST_NODE_T;

#define GetSLListData( n )		(((SL_LIST_NODE_T*)(n))->data)
#define GetSLListHead( L )		((L)->head)
#define GetSLListNextNode( n )	(((SL_LIST_NODE_T*)(n))->next)
#define isSLListLastNode( n )	((((SL_LIST_NODE_T*)(n))->next == NULL) ? \
									TRUE : FALSE)
#define isSLListEmpty( L )		(((L)->head == NULL || (L)->tail == NULL) ? \
									TRUE : FALSE)

int InitSLList( SL_LIST_T** List, void (*FreeFunc)(), int (*CmpFunc)(),
		void* data, int Type );
int AppendToSLList( SL_LIST_T* List, void* data, int DataSize );
int MakeTableFromSLList( SL_LIST_T* List, void** TablePtr[], int* TableSize );
void FreeSLList( SL_LIST_T** List, int Flag );
void FreeTable( void** Table[], void (*FreeFunc)(void*) );
void SortArray( char* ObjNameArray[], int NumObjs );

#endif NLIST_H
