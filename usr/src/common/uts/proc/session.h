/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_SESSION_H	/* wrapper symbol for kernel use */
#define _PROC_SESSION_H	/* subject to change without notice */

#ident	"@(#)kern:proc/session.h	1.16"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */
#include <proc/pid.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */
#include <sys/pid.h>

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct sess {
        lock_t s_mutex;                 /* session state lock */
        uchar_t s_ctty;                 /* TRUE iff session has or had ctty */
        ulong_t s_cttyref;              /* number of LWPs doing ops via s_vp */
	uint_t s_ref;			/* number of processes in session */
        struct vnode *s_vp;             /* tty's vnode */
        struct pid *s_pgrps;            /* non-zombie process group set */
        struct pid *s_sidp;             /* session ID info */
        struct cred *s_cred;            /* ctty allocation credentials */
} sess_t;

#define s_sid s_sidp->pid_id

/*
 * Enumeration of the types of access that can be requested for a
 * controlling terminal under job control.
 */

enum jcaccess {
	JCREAD,		/* read data on a ctty */
	JCWRITE,	/* write data to a ctty */
	JCSETP,		/* set ctty parameters */
	JCGETP		/* get ctty parameters */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * Hold an execution reference to the controlling terminal of the given session.
 * The session lock (s_mutex) is held upon entry and remains held upon exit.
 */
#define SP_CTTYHOLD(sp) \
	((sp)->s_cttyref++);

/* 
 * Release a previously acquired execution reference on the controlling
 * terminal of the given session.
 * The session lock is released at PLBASE upon return.  
 */
#define SP_CTTYREL(sp, ttyvp) { \
	(void) LOCK(&(sp)->s_mutex, PL_SESS);		\
	if (--(sp)->s_cttyref == 0 && (sp)->s_vp == NULL) { \
		if ((sp)->s_ref != 0) { \
			UNLOCK(&(sp)->s_mutex, PLBASE); \
			relectty((ttyvp), (sp)->s_cred); \
		} else { \
			UNLOCK(&(sp)->s_mutex, PLBASE); \
			relectty((ttyvp), (sp)->s_cred); \
			LOCK_DEINIT(&(sp)->s_mutex); \
			kmem_free((sp), sizeof(sess_t)); \
		} \
	} else \
		UNLOCK(&(sp)->s_mutex, PLBASE); \
}

extern sess_t *sess_create(sess_t *);
extern void freectty(void);
extern void relectty(struct vnode *, struct cred *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_SESSION_H */
