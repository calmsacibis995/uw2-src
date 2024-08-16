/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_AUTOCONF_RESMGR_RESMGR_H
#define _IO_AUTOCONF_RESMGR_RESMGR_H

#ident	"@(#)kern:io/autoconf/resmgr/resmgr.h	1.13"
#ident	"$Header: $"

#if defined ( __cplusplus )
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <io/uio.h>		/* REQUIRED */

#else

#include <sys/types.h>		/* REQUIRED */
#include <sys/uio.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef	uint_t		rm_key_t;

/*
** The special key used to access Resource Manager specific parameters.
** Iterating through the database via RMIOC_NEXTKEY NEVER returns RM_KEY.
*/

#define RM_KEY		((rm_key_t)~0)

/* These are the Resource Manager specific parameters */

#define RM_TIMESTAMP	"RM_TIMESTAMP"	/* time of last change to database */
#define RM_INITFILE	"RM_INITFILE"	/* State of initial resmgr file */

/* RM_MAXPARAMLEN is the maximum length of a parameter name */

#define	RM_MAXPARAMLEN	16	/* 15 for string, 1 for '\0' */

/*
** These are used as delimiters in /stand/resmgr.  RM_NULL_KEY is also
** used to initiate iterating through the keys via RMIOC_NEXTKEY.
*/

#define	RM_ENDOFVALS	0
#define RM_ENDOFPARAMS	"RM_ENDOFPARAMS"
#define RM_NULL_KEY	0
#define DEV_RESMGR	"/dev/resmgr"
#define STAND_RESMGR	"/stand/resmgr"

/* ioctl's for user level access */

#define RMIOC		('R' << 16 | 'M' << 8)
#define RMIOC_DELVAL	(RMIOC | 0)
#define RMIOC_DELKEY	(RMIOC | 1)
#define RMIOC_ADDVAL	(RMIOC | 2)
#define RMIOC_GETVAL	(RMIOC | 3)
#define RMIOC_NEWKEY	(RMIOC | 4)
#define RMIOC_NEXTKEY	(RMIOC | 5)
#define RMIOC_NEXTPARAM	(RMIOC | 6)

/* rm_args is the input/output argument for the resmgr routines */

struct rm_args
{
	rm_key_t	rm_key;
	char		rm_param[ RM_MAXPARAMLEN ];
	void		*rm_val;
	size_t		rm_vallen;
	uint_t		rm_n;
};

#ifdef _KERNEL

#ifdef __STDC__

extern int rm_newkey( struct rm_args * );
extern int rm_delkey( struct rm_args * );
extern int rm_nextkey( struct rm_args * );
extern int rm_nextparam( struct rm_args * );
extern int rm_addval( struct rm_args *, uio_seg_t );
extern int rm_getval( struct rm_args *, uio_seg_t );
extern int rm_delval( struct rm_args * );

#else

extern int rm_newkey();
extern int rm_delkey();
extern int rm_nextkey();
extern int rm_nextparam();
extern int rm_addval();
extern int rm_getval();
extern int rm_delval();

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined ( __cplusplus )
	}
#endif

#endif /* _IO_AUTOCONF_RESMGR_RESMGR_H */
