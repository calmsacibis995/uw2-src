/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfimacro.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfimacro.h,v 2.53.2.3 1995/01/24 18:15:09 mdash Exp $"

#ifndef _FS_NUCFS_NWFIMACRO_H
#define _FS_NUCFS_NWFIMACRO_H

/*
**  Netware Unix Client
**
**	MODULE:
**		nwfimacro.h -	Macros used in the NWfi layer.
**
*/

#ifdef _KERNEL_HEADERS
#include <util/ksynch.h>
#endif	/* _KERNEL_HEADERS */

#ifdef NUCFS_BOUND

#define	NUCFS_ENGINE_DATA(eng)		engine_t *eng;

#define NUCFS_BIND(oldengine) {			\
	oldengine = kbind(&engine[0]);		\
	block_preemption();			\
}

#define NUCFS_UNBIND(oldengine) {		\
	unblock_preemption(); 			\
	kunbind(oldengine); 			\
}

#else /* !NUCFS_BOUND */

#define	NUCFS_ENGINE_DATA(eng)
#define NUCFS_BIND(oldengine)		((void)0)
#define NUCFS_UNBIND(oldengine) 	((void)0)

#endif /* NUCFS_BOUND */

#endif /* _FS_NUCFS_NWFIMACRO_H */
