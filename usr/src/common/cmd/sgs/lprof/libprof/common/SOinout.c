/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/SOinout.c	1.5"

#include <link.h>
#include "dprof.h"	


#define TRUE	1
#define FALSE	0

typedef struct link_map	LinkMap_s;
typedef	struct r_debug  Debug_s;

/*
*	Local routines.
*/
static void update();
static void addObj();
static void delObj();

/*
*	File scope variables.
*/
static LinkMap_s	*linkmap_p;

/*
*	external  variables.
*/
extern SOentry		*_inact_SO;
extern SOentry		*_last_SO;
extern SOentry		*_act_SO;

/* 
 * _SOin() - called by rtld when a change to the a.out's link map has
 * occurred. Any new objects are added to end of the profiler's list
 * based on the assumption that rtld does the same.
 *
 */
void 
_SOin(Debug_s *dep_p)
{


	if (linkmap_p == NULL)
		linkmap_p = dep_p->r_map;
	if (_act_SO == NULL || linkmap_p == NULL)
		return;
	update();

}

static void 
addObj(LinkMap_s *lm_p)
{

	SOentry	*tmpSO;

	/* add to the end of the linked list */
	tmpSO = _last_SO;
	/* get the space necessary and set the pointer */
	_last_SO = (SOentry *) _lprof_Malloc(1, sizeof(SOentry));

	_last_SO->SOpath = lm_p->l_name; 
	_last_SO->baseaddr = lm_p->l_addr;

	_last_SO->textstart = lm_p->l_tstart;
	_last_SO->endaddr = lm_p->l_tsize ? (lm_p->l_tstart + lm_p->l_tsize) : 0;
	_last_SO->ccnt = 0;
	_last_SO->size = ((lm_p->l_tsize + 7) >>  3); 
	_last_SO->tcnt = (WORD *) _lprof_Calloc(_last_SO->size, sizeof(WORD));
	_last_SO->prev_SO = tmpSO;
	tmpSO->next_SO = _last_SO;
}

static void 
delObj(SOentry *delSO)
{
	
	if (_last_SO == delSO)
		_last_SO = delSO->prev_SO;
	else
		delSO->next_SO->prev_SO = delSO->prev_SO;

	delSO->prev_SO->next_SO = delSO->next_SO;
		
	/* add to front of deleted list */
	delSO->next_SO = _inact_SO;
	_inact_SO = delSO;


}

/*
*	update() 
*	- Search the link map and the active shared object list until 
*	there is a difference between them.
*	- If the end of the object list has not been reached, then the
*	object currently pointed to does not match the current entry
*	on the link map and must have been deleted. Call delObj to 
*	place the object on the inactive list.
*	- If we're at the end of the object list, then the remaining
*	entries on the link map must be added.
*
*/
static void 
update()
{
	LinkMap_s	*lm_p = linkmap_p;
	SOentry		*SO_p = _act_SO;
	SOentry		*nSO_p;

	do {
		nSO_p = SO_p->next_SO;
		if (lm_p == NULL || strcmp(lm_p->l_name, SO_p->SOpath) != 0)
			delObj(SO_p);
		else 
			lm_p = lm_p->l_next;	

	} while ((SO_p = nSO_p) != NULL);

	while(lm_p){
		addObj(lm_p);
		lm_p = lm_p->l_next;
	}
	 
}

