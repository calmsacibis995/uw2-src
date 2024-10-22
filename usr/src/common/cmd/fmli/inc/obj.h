/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:inc/obj.h	1.1.4.3"

#define ON	1
#define OFF	0
/* the following are the defines for the hard-coded columns of the OOT */
#define OF_VI	0	/* view init function */
#define OF_SH	1	/* selection handler function */
#define OF_EX	2	/* exit function */
#define	OF_MV	3	/* make viewable function */
#define OF_OPEN	4	/* default action (open) */

#define PROMPT	(-4)
#define REINIT	(-5)
#define BACKUP	3
#define REPAINT	4
#define OBJECT	5
#define REREAD	6
#define NOPAINT	7
#define NOOBJECT	8

#define MAX_LABELS	12

#define SAME_OBJECT	1
#define DIFF_OBJECT	2
#define FILE_PARENT	3
#define DIR_PARENT	4

/* flags for make_object */
#define NOFORCE	(0x1)
#define FORCE	(0x2)
#define PARENT	(0x4)

struct label {
	struct operation	*oper;
	int	number;
};
