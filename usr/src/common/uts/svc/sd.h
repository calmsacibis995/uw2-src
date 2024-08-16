/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_SD_H	/* wrapper symbol for kernel use */
#define _SVC_SD_H	/* subject to change without notice */

#ident	"@(#)kern:svc/sd.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif

/* Shared Data flags */
#define	SD_RDONLY	0x00
#define	SD_WRITE	0x01

/* flags for various XENIX shared data system calls */
#define	SD_CREAT	0x02	/* the caller want to create a xsd object */
#define	SD_UNLOCK	0x04	/* the caller want to create a shared data
				 * object which is not lockable.
				 */
#define	SD_NOWAIT	0x08	/* the caller does not want to wait */

#if defined(_KERNEL) || defined(_KMEMUSER)

#define	SD_LOCKED	0x10	/* The object is in locked state now.
				 * An object is in the "locked" state after
				 * it has been created as lockable and been
				 * sdenter'ed but before a corresponding
				 * sdleave is called.
				 */
#define	SD_BTWN		0x40	/* the process was between sdenter and
				 * sdleave.
				 */ 

#define SDI_LOCKED	0x10

struct sd {                 		/* XENIX shared data table entry*/
	struct xnamnode	*sd_xnamnode;	/* pointer to inode for segment */
	char    	*sd_addr;	/* address in this proc's data space */
	char      	*sd_cpaddr;	/* version # for local copy */
	char      	sd_flags;	/* SD_WRITE, SD_RDONLY, or SD_BTWN */
	struct sd    	*sd_link;	/* ptr to next shared data seg for
					 * this proc
					 */
	uint_t   	sd_keepcnt;	/* the lock count on the attachment
					 */
	sv_t		sd_sv;		/* synchronization variables for
					 * sd_keepcnt
					 */
};

extern void xsdinit(void);
void xsd_destroy(struct xnamnode *);

#endif /* _KERNEL || KMEMUSER */

#ifndef _KERNEL
#ifdef __STDC__
extern char *sdget(char *path, int flags, ...);
int sdenter(char *addr, int flags);
int sdleave(char *addr);
int sdfree(char *addr);
int sdgetv(char *addr);
int sdwaitv(char *addr, int vnum);
#else
extern char *sdget();
int sdenter();
int sdfree();
int sdgetv();
int sdwaitv();
#endif /* __STDC__  */
#endif /* #ifndef _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_SD_H */
