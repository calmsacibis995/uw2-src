/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:xterm.h	1.1.1.18"
#endif

/*
 xterm.h (C hdr file)
	Acc: 601052495 Tue Jan 17 10:01:35 1989
	Mod: 601054202 Tue Jan 17 10:30:02 1989
	Sta: 601054202 Tue Jan 17 10:30:02 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

#ifndef Boolean
#include <X11/Intrinsic.h>
#endif

#define GC_Foreground(gc)	((gc)->values.foreground)
#define GC_Background(gc)	((gc)->values.background)

#ifndef CTRL
#define CTRL(x)	(037 & (x))
#endif

#define EWOULDBLOCK	EAGAIN

#ifndef SVR4
#define	SIGCHLD SIGCLD
#endif /* SVR4 */
