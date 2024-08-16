/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlist:list.c	1.2"
#include "list.h"

int
InitSLList(	SL_LIST_T**	List,
			void		(*FreeFunc)(),
			int			(*CmpFunc)(),
			void*		data,
			int			Type )
{

	*List = (SL_LIST_T*)malloc( sizeof(SL_LIST_T) );
	if( *List == NULL ){
		return( FAILED );
	}
	(*List)->head = NULL;
	(*List)->tail = NULL;
	(*List)->count = NULL;
	(*List)->freeFunc = FreeFunc;
	(*List)->cmpFunc = CmpFunc;
	(*List)->data = data;
	(*List)->type = Type;

	return( SUCCESS );
}

int
AppendToSLList( SL_LIST_T* List, void* data, int DataSize )
{
	int					rc;
	void*				NodeData;
	SL_LIST_NODE_T*		NewNode;
	SL_LIST_NODE_T*		CurrentNode;
	SL_LIST_NODE_T*		PrevNode;

	if( List == NULL || data == NULL ){
		return( FAILED );
	}

	NewNode = (SL_LIST_NODE_T*)malloc( sizeof(SL_LIST_NODE_T) );
	if( NewNode == NULL ){
		return( FAILED );
	}

	if( DataSize != 0 ){
		NewNode->data = (void*)malloc( DataSize );
		if( NewNode->data == NULL ){
			return( FAILED );
		}
		memcpy( NewNode->data, data, DataSize );
	}else{
		NewNode->data = data;
	}

	if( List->cmpFunc == NULL ){
		/*	If there is not a sort function registered
		 *	just append the node to the end of the list.
		 */
		if( isSLListEmpty(List) ){
			List->head = NewNode;
		}else{
			(List->tail)->next = NewNode;
		}

		List->tail = NewNode;
		NewNode->next = NULL;
	}else{
		if( isSLListEmpty(List) ){
			List->head = NewNode;
			List->tail = NewNode;
			NewNode->next = NULL;
		}else{
			CurrentNode = List->head;
			PrevNode = NULL;
			while( TRUE ){
				if(((*(List->cmpFunc))( NewNode->data, CurrentNode->data)) < 0){
					if( PrevNode == NULL ){
						List->head = NewNode;
					}else{
						PrevNode->next = NewNode;
					}
					NewNode->next = CurrentNode;
					break;
				}
				if( isSLListLastNode(CurrentNode) ){
					List->tail = NewNode; 
					CurrentNode->next = NewNode;
					NewNode->next = NULL;
					break;
				}
				PrevNode = CurrentNode;
				CurrentNode = CurrentNode->next;
				continue;
			}
		}
	}
	List->count++;

	return( SUCCESS );
}

int
MakeTableFromSLList( SL_LIST_T* List, void** TablePtr[], int* TableSize )
{
	int				i;
	SL_LIST_NODE_T* Node;

	*TablePtr = (void**)malloc( sizeof(void**) * (List->count+1) );
	if( *TablePtr == NULL ){
		return( FAILED );
	}

	Node = List->head;
	for( i=0; i < (List->count); i++ ){
		(*TablePtr)[i] = Node->data;
		Node = Node->next;
	}
	(*TablePtr)[List->count] = NULL;

	*TableSize = List->count;

	return( SUCCESS );
}

void
FreeSLList( SL_LIST_T** List, int Flag )
{
	int					i;
	SL_LIST_NODE_T*		Current;
	SL_LIST_NODE_T*		Next;

	if( *List == NULL ){
		return;
	}

	Current = (*List)->head;
	for( i=0; i < (*List)->count; i++ ){
		Next = Current->next;
		if( ((*List)->freeFunc != NULL) && (Flag == FREE_DATA) ){
			(*((*List)->freeFunc))( Current->data );
		}
		free( Current );
		Current = Next;
	}
	free( *List );
	List = NULL;
}

void
FreeTable( void** Table[], void (*FreeFunc)(void*) )
{
	int			i=0;

	if( *Table == NULL || Table == NULL ){
		return;
	}

	while( (*Table)[i] != NULL ){
		if( FreeFunc != NULL ){
			(*FreeFunc)( *Table[i] );
		}
		i++;
	}
	free( *Table );
	Table = NULL;
}

void
SortArray( char* ObjNameArray[], int NumObjs )
{
	char*		TmpPtr;
	int			i;
	int			j;
	int			rc;

	for( i=0; i<NumObjs; i++ ){
		for( j=0; j<(NumObjs-1); j++ ){
			rc = strcmp( ObjNameArray[j], ObjNameArray[j+1] );
			if( rc > 0 ){
				TmpPtr = ObjNameArray[j+1];
				ObjNameArray[j+1] = ObjNameArray[j];
				ObjNameArray[j] = TmpPtr;
			}
		}
	}
}
