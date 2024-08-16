/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:libsetup/setupMove.c	1.2"
#include	<stdio.h>
#include	<malloc.h>
#include	<mail/table.h>

#define	SETUPMOVE_OBJ

typedef struct setupMove_s
    {
    struct setupVar_s
	*sm_var;

    struct setupMove_s
	*sm_right,
	*sm_left,
	*sm_up,
	*sm_down,
	*sm_next,
	*sm_tab;
    }	setupMove_t;

#include	<mail/setupVar.h>

setupVar_t
    *setupMoveVar(setupMove_t *move_p)
	{
	return(move_p->sm_var);
	}

void
    setupMoveSet
	(
	setupMove_t *move_p,
	moveDirection_t direction,
	setupMove_t *nextMove_p
	)
	{
	switch(direction)
	    {
	    case	SM_LEFT:
		{
		move_p->sm_left = nextMove_p;
		break;
		}

	    case	SM_RIGHT:
		{
		move_p->sm_right = nextMove_p;
		break;
		}

	    case	SM_UP:
		{
		move_p->sm_up = nextMove_p;
		break;
		}

	    case	SM_DOWN:
		{
		move_p->sm_down = nextMove_p;
		break;
		}

	    case	SM_TAB:
		{
		move_p->sm_tab = nextMove_p;
		break;
		}

	    case	SM_NEXT:
		{
		move_p->sm_next = nextMove_p;
		break;
		}
	    }
	}

setupMove_t
    *setupMove(setupMove_t *move_p, moveDirection_t direction)
	{
	setupMove_t
	    *result;

	switch(direction)
	    {
	    case	SM_LEFT:
		{
		result = move_p->sm_left;
		break;
		}

	    case	SM_RIGHT:
		{
		result = move_p->sm_right;
		break;
		}

	    case	SM_UP:
		{
		result = move_p->sm_up;
		break;
		}

	    case	SM_DOWN:
		{
		result = move_p->sm_down;
		break;
		}

	    case	SM_TAB:
		{
		result = move_p->sm_tab;
		break;
		}

	    case	SM_NEXT:
		{
		result = move_p->sm_next;
		break;
		}
	    }

	return(result);
	}

void
    setupMoveFree(setupMove_t *setupMove_p)
	{
	if(setupMove_p != NULL)
	    {
	    free(setupMove_p);
	    }
	}

setupMove_t
    *setupMoveNew(setupVar_t *setupVar_p)
	{
	setupMove_t
	    *result;
	
	if((result = (setupMove_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else
	    {
	    result->sm_var = setupVar_p;
	    result->sm_right = result;
	    result->sm_left = result;
	    result->sm_up = result;
	    result->sm_down = result;
	    result->sm_next = NULL;
	    result->sm_tab = result;
	    }

	return(result);
	}
