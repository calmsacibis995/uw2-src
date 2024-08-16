/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MOD_H		/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_H		/* subject to change without notice */

#ident	"@(#)kern:util/mod/mod.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/param.h>		/* REQUIRED */

#elif defined (_KERNEL) || defined (_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */

#else

#include <sys/param.h>		/* SVR4.2 COMPAT */

#endif /* _KERNEL_HEADERS */

/* info for modadm */
#define MOD_C_MREG	1	/* command to register modules */
#define MOD_C_VERIFY	2	/* ask module to verify hardware */
#define	MODMAXNAMELEN	15	/* max number of characters (incl '\0') in 
				   module name , should be consistent with 
				   the length used in idtools */

struct mod_mreg {		/* structure for module registration */
	char md_modname[MODMAXNAMELEN];	/* name of file for module */
	void *md_typedata;	/* module type specific data */
};


/* module types */
#define	MOD_TY_NONE	0
#define	MOD_TY_CDEV	1
#define	MOD_TY_BDEV	2
#define	MOD_TY_STR	3
#define	MOD_TY_FS	4
#define	MOD_TY_SDEV	5
#define MOD_TY_MISC	6
#define MOD_TY_EXEC	7


/*info for modstat */
#define MODMAXLINKINFOLEN	81	/* max length of info string per
					 * module type for each module
					 * (incl. '\0') */

/* one per module type for each module queried through modadm 
   (e.g., if a module is both a file system and a driver, there would be two of these
   structures returned through the modstatus structure for that module */
struct modspecific_stat {
	char	mss_linkinfo[MODMAXLINKINFOLEN];	/* informational */
	int	mss_type;		/* type of module - from above MOD_TY* */
	int	mss_p0[2];		/* type specific info */
	int	mss_p1[2];		/* type specific info */
};

#define MODMAXLINK 4
/* structure for communicating info about module (used through modadm) */
struct modstatus {
	int	ms_id;		/* numeric id of module */
	void    *ms_base;	/* base address of module */
	unsigned int ms_size;	/* amount of memory of module when loaded */
	int	ms_rev;		/* version number of loadable modules */
	char	ms_path[MAXPATHLEN];	/* path from which module was loaded */
	time_t 	ms_unload_delay;	/* unload delay configured for this module */
	int	ms_holdcnt;	/* hold count */
	int	ms_depcnt;	/* dependent count */
	struct modspecific_stat	ms_msinfo[MODMAXLINK];	/* module type specific info */
};


/* system call prototypes */
#ifndef _KERNEL
#ifdef __STDC__
extern int modadm(unsigned int, unsigned int, void *);
extern int modstat(int, struct modstatus *, boolean_t);
extern int modload(const char *);
extern int moduload(unsigned int);
extern int modpath(const char *);
#else
extern int modadm();
extern int modstat();
extern int modload();
extern int moduload();
extern int modpath();
#endif
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MOD_H */
