/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 *
 */

#ident	"@(#)fmli:inc/message.h	1.4.4.3"

extern	int Mess_lock;
#define mess_lock()	(Mess_lock++)
#define mess_unlock()	(Mess_lock = 0)
#define MESSIZ	(256)  /* that should be wider than any screen */
