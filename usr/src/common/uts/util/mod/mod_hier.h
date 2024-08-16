/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_HIER_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:util/mod/mod_hier.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the DLM subsystem. Locks that can be 
 * held across subsystem boundaries need to be dealt with separately.
 * These global locks will have their hierarchy defined in the ghier.h file
 * under util.
 */

#ifdef _KERNEL

#define	PLDLM		PLSTR	
#define	PLIV		PLHI	

#define DLM_HIER_BASE	KERNEL_HIER_BASE

#define DLM_HIER_IVECT	DLM_HIER_BASE			/* ivect table */
#define DLM_HIER_MPATH	(DLM_HIER_BASE + 5)		/* mpath variable */
#define DLM_HIER_SWTAB	(DLM_HIER_BASE + 5)		/* switch tables */
#define DLM_HIER_LIST	(DLM_HIER_BASE + 15) 		/* modlist */
#define DLM_HIER_CTL	(DLM_HIER_BASE + 20)		/* modctl */

#define DLM_FMODSW_HIER	(STR_HIER_BASE + 2)		/* fmodsw table */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_HIER_H */
