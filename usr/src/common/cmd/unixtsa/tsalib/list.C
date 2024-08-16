#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/list.C	1.1"

#include <smsutapi.h>

#if defined(DEBUG_CODE)
#include <conio.h>
#include <stdio.h>
#endif

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMInitList"
#endif
void NWSMInitList(
		NWSM_LIST_PTR	 *listPtr,
		void			(*freeRoutine)(
								void *memoryPointer))
{
    listPtr->head = NULL;
    listPtr->tail = NULL;
    listPtr->sortProc = stricmp;
	listPtr->freeProcedure = freeRoutine;
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMAppendToList"
#endif
NWSM_LIST *NWSMAppendToList(
		NWSM_LIST_PTR	*list,
		BUFFERPTR		 text,
		void			*otherInfo)
{
	NWSM_LIST *el;

	if ((el = (NWSM_LIST *)calloc(1, sizeof(NWSM_LIST) + strlen((PSTRING)text))) is NULL)
		return (NULL);

	el->prev = list->tail;			/* attach to the tail */
	if (!list->head)				/* is this the 1st in the list */
		list->head = el;			/* put at the head of the list */

	else
		list->tail->next = el;		/* point last's next at this one */

	list->tail = el;				/* new element is now the last one */

	strcpy((PSTRING)el->text, (PSTRING)text);	/* copy over the text info */
	el->otherInfo = otherInfo;		/* set the otherInfo pointer */

	return (el);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetListHead"
#endif
NWSM_LIST *NWSMGetListHead(
		NWSM_LIST_PTR	*listPtr)
{
	if (listPtr->head)
		while (listPtr->head->prev)
			listPtr->head = listPtr->head->prev;

	return (listPtr->head);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMDestroyList"
#endif
void NWSMDestroyList(
		NWSM_LIST_PTR	*list)
{
	NWSM_LIST *el1, *el2;

	el1 = NWSMGetListHead(list);

	while (el1)
	{
		el2 = el1;
		el1 = el1->next;
		if (el2->otherInfo and list->freeProcedure)
		{

			(*(list->freeProcedure))(el2->otherInfo);
		}

		free((char *)el2);
	}

    list->head = list->tail = NULL;
}

