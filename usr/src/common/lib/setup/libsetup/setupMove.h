/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if	!defined(SETUPMOVE_H)
#define	SETUPMOVE_H

#include	<mail/setupTypes.h>

setupVar_t
    *setupMoveVar(setupMove_t *move_p);

void
    setupMoveSet
	(
	setupMove_t *move_p,
	moveDirection_t direction,
	setupMove_t *nextMove_p
	);

setupMove_t
    *setupMove(setupMove_t *move_p, moveDirection_t direction),
    *setupMoveNew(setupVar_t *setupVar_p);

void
    setupMoveFree(setupMove_t *setupMove_p);

#endif
