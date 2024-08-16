/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_POLL_H	/* wrapper symbol for kernel use */
#define _IO_POLL_H	/* subject to change without notice */
#define _SYS_POLL_H	/* SVR4.0COMPAT */

#ident	"@(#)kern:io/poll.h	1.13"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Structure of file descriptor/event pairs supplied in
 * the poll arrays.
 */
struct pollfd {
	int fd;				/* file desc to poll */
	short events;			/* events of interest on fd */
	short revents;			/* events that occurred on fd */
};

/*
 * Testable select events 
 */
#define POLLIN		0x0001		/* fd is readable */
#define POLLPRI		0x0002		/* high priority info at fd */
#define	POLLOUT		0x0004		/* fd is writeable (won't block) */
#define POLLRDNORM	0x0040		/* normal data is readable */
#define POLLWRNORM	POLLOUT
#define POLLRDBAND	0x0080		/* out-of-band data is readable */
#define POLLWRBAND	0x0100		/* out-of-band data is writeable */

#define POLLNORM	POLLRDNORM

/*
 * Non-testable poll events (may not be specified in events field,
 * but may be returned in revents field).
 */
#define POLLERR		0x0008		/* fd has error condition */
#define POLLHUP		0x0010		/* fd has been hung up on */
#define POLLNVAL	0x0020		/* invalid pollfd entry */

#if defined(_KERNEL) || defined(_KMEMUSER)
/*
 * Poll list head structure.  A pointer to this is passed
 * to pollwakeup() from the caller indicating the event has
 * occurred.
 */
struct pollhead {
	struct polldat	*ph_list;	/* list of pollers */
	lock_t		*ph_mutex;	/* protects list and events */
	short		ph_events;	/* events pending on list */
	char		ph_type;	/* phalloc or compat */
	char		ph_resv;	/* reserved for future use */
	unsigned long	ph_gen;		/* generation number */
	long		ph_filler[4];	/* reserved for future use */
};

/* Types */
#define PHCOMPAT	0	/* static allocation by driver */
#define PHALLOC		1	/* dynamic allocation by driver */

/*
 * Data necessary to notify process sleeping in poll(2)
 * when an event has occurred.
 */
struct polldat {
	struct polldat *pd_next;	/* next in poll list */
	short		pd_events;	/* events being polled */
	void		(*pd_fn)();	/* function to call */
	long		pd_arg;		/* argument to function */
	struct pollhead	*pd_headp;	/* backpointer to head of list */
};

struct pollx {
	struct file	*px_fp;
	struct pollhead	*px_php;
};

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#ifdef _KERNEL

/* Notify pollers of the occurrence of an event. */
extern void pollwakeup(struct pollhead *, short);
extern struct pollhead *phalloc(int);		/* Allocate pollhead */
extern void phfree(struct pollhead *);		/* Free pollhead */

#else /* !_KERNEL */

#if defined(__STDC__)
int poll(struct pollfd *, unsigned long, int);
#endif

#endif /* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_POLL_H */
