/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_ACC_PRIV_LPM_LPM_H	/* wrapper symbol for kernel use */
#define	_ACC_PRIV_LPM_LPM_H	/* subject to change without notice */

#ident	"@(#)kern:acc/priv/lpm/lpm.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 *
 * Structure definitions for the kernel privilege table
 * data types.  Used by any privilege mechanism that stores
 * the information in the kernel.
 *
 */

typedef struct	LPMftab {
	struct	LPMftab	*next;	/* ptr to next file in list	*/
	ino_t	nodeid;		/* node id			*/
	pvec_t	fixpriv;	/* fixed privileges		*/
	pvec_t	inhpriv;	/* inheritable privileges	*/
	time_t	validity;	/* validity info for integrity	*/
} LPMftab_t;

typedef struct	LPMdtab {
	struct	LPMdtab	*next;	/* ptr to next file system in list	*/
	LPMftab_t	*list;	/* ptr to a privileged file on		*/
				/* this particular file system		*/
	dev_t	fsid;		/* the id number for this file system	*/
} LPMdtab_t;

typedef struct	LPMktab {
	struct	LPMktab	*next;	/* ptr to next device in list */
	LPMdtab_t	*list;	/* ptr to a file system on */
					/* this particular device */
	dev_t	dev;		/* the id number for this device */
} LPMktab_t;

#endif	/* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif	/*_ACC_PRIV_LPM_LPM_H  */
