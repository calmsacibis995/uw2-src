/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_BIND_H	/* wrapper symbol for kernel use */
#define _PROC_BIND_H	/* subject to change without notice */

#ident	"@(#)kern:proc/bind.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <proc/procset.h>	/* REQUIRED */

#else

#include <sys/types.h>		/* REQUIRED */
#include <sys/procset.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#define PBIND_NONE	-1
#define PBIND_QUERY	-2

#define	PEXBIND_NONE	-1
#define	PEXBIND_QUERY	-2

#ifdef _KERNEL

struct lwp;
struct engine;

struct engine *kbind(struct engine *);
void kunbind(struct engine *);
void bind_create(struct lwp *, struct lwp *);
void bind_exit(struct lwp *);
int bindproc(processorid_t);
void unbindproc(void);

#else /* !KERNEL */

#ifdef	__STDC__
int processor_exbind(idtype_t, id_t *, int, processorid_t, processorid_t *);
int processor_bind(idtype_t, id_t, processorid_t, processorid_t *);
#else
int processor_exbind();
int processor_bind();
#endif /* __STDC__ */

#endif /* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_BIND_H */
