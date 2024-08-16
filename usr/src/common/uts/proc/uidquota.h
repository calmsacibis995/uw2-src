/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_UIDQUOTA_H	/* wrapper symbol for kernel use */
#define _PROC_UIDQUOTA_H	/* subject to change without notice */

#ident	"@(#)kern:proc/uidquota.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined _KERNEL || defined _KMEMUSER

typedef struct uidquotas uidquo_t;
struct uidquotas {
	uid_t	uq_uid;		/* Real uid associated with this object */
	uint_t	uq_ref;		/* This field is overloaded; it is both:
				 *   the reference count for this object, and
				 *   the number of processes with this real uid
				 * This overloading works because references
				 * are only held by processes, and they are
				 * held by exactly the set of processes with
				 * this real uid.  (Neither of these include
				 * processes which are exempt from UID quotas.)
				 */
	uint_t	uq_lwpcnt;	/* Number of LWPs with this real uid */
	uidquo_t *uq_link;	/* Next structure on the hash chain */
};

typedef struct uidhash {
	lock_t	ui_mutex;	/* Protects the chain and chain elements */
	uidquo_t *ui_link;	/* Pointer to the first object */
} uidhash_t;

typedef struct uidres { 
	uint_t	ur_lwpcnt;	/* Number of LWPs to be added/subtracted */
	uint_t	ur_prcnt;	/* Number of procs to be added/subtracted */
} uidres_t;

#endif /* _KERNEL || _KMEMUSER */

#if defined(_KERNEL)

void uidquota_init(void);
uidquo_t *uidquota_get(uid_t uid);
boolean_t uidquota_incr(uidquo_t *uidp, uidres_t *uidresp, boolean_t flag);
void uidquota_decr(uidquo_t *uidp, uidres_t *uidresp);

#define PL_UID		PLHI

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_UIDQUOTA_H */
