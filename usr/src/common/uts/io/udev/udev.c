/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/udev/udev.c	1.1"
#ident	"$Header: $"

#include <fs/buf.h>
#include <io/conf.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <util/bitmasks.h>
#include <util/debug.h>
#include <util/ghier.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/sysmacros.h>
#include <util/types.h>

int udev_devflag = D_MP;

typedef struct udev_hash {
	struct udev_hash *ud_next;
	dev_t		 ud_cdev;
	dev_t		 ud_rdev;
} udev_hash_t;

#define N_UDEV_HASH	31

#define UDEV_HASHF(rdev) \
		((getemajor(rdev) ^ geteminor(rdev)) % N_UDEV_HASH)

STATIC udev_hash_t *udev_hash[N_UDEV_HASH];

STATIC sv_t udev_sv;
STATIC lock_t udev_hash_mutex;
STATIC fspin_t udev_bits_mutex;

STATIC LKINFO_DECL(udev_hash_lkinfo, "udev_hash_mutex", 0);

STATIC boolean_t udev_waiting;

major_t udev_emajor;

extern major_t udev_imajor;
extern uint_t udev_nmajors;
extern uint_t udev_bits[];

#define BITS_MAX	(udev_nmajors << O_BITSMINOR)
#define BITS_SIZE	BITMASK_NWORDS(BITS_MAX)


/*
 * void
 * udev_init(void)
 *	Initialization for udev pseudo-device
 *
 * Calling/Exit State:
 *	Called at sysinit time.
 */
void
udev_init(void)
{
	SV_INIT(&udev_sv);
	LOCK_INIT(&udev_hash_mutex, KERNEL_HIER_BASE, PLMIN,
		  &udev_hash_lkinfo, KM_NOSLEEP);
	FSPIN_INIT(&udev_bits_mutex);
	BITMASKN_CLRALL(udev_bits, BITS_SIZE);
	udev_emajor = itoemajor(udev_imajor, -1);
}

/*
 * int
 * udev_open(dev_t *dev_p, int flags, int otype, cred_t *cred_p)
 *	Dummy open routine for udev pseudo-device.
 *
 * Calling/Exit State:
 *	Should never happen, but just in case...
 */
/* ARGSUSED */
int
udev_open(dev_t *dev_p, int flags, int otype, cred_t *cred_p)
{
	/* Should never happen, but fail it just in case. */
	return ENODEV;
}

/*
 * int
 * udev_close(dev_t dev, int flags, int otype, cred_t *cred_p)
 *	Dummy close routine for udev pseudo-device.
 *
 * Calling/Exit State:
 *	Should never happen, but just in case...
 */
/* ARGSUSED */
int
udev_close(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	/* Should never happen. */
	return 0;
}

/*
 * int
 * udev_strategy(buf_t *bp)
 *	Dummy strategy routine for udev pseudo-device.
 *
 * Calling/Exit State:
 *	Should never happen, but just in case...
 */
/* ARGSUSED */
int
udev_strategy(buf_t *bp)
{
	/* Should never happen. */
	return 0;
}


/*
 * dev_t
 * getudev(void)
 *	Generate an unused unique device number.
 *
 * Calling/Exit State:
 *	None.
 */
dev_t
getudev(void)
{
	minor_t _minor;
	major_t _major;
	int i;

	FSPIN_LOCK(&udev_bits_mutex);
	i = BITMASKN_FFCSET(udev_bits, BITS_SIZE);
	FSPIN_UNLOCK(&udev_bits_mutex);

	if (i == -1 || i >= BITS_MAX)
		return NODEV;

	_major = udev_emajor + ((uint_t)i >> O_BITSMINOR);
	_minor = ((uint_t)i & O_MAXMIN);

	return makedevice(_major, _minor);
}


/*
 * void
 * putudev(dev_t dev)
 *	Recycle a device number previously allocated by getudev().
 *
 * Calling/Exit State:
 *	None.
 */
void
putudev(dev_t dev)
{
	minor_t _minor;
	major_t _major;
	int i;
#ifdef DEBUG
	boolean_t was_set;
#endif

	_major = getmajor(dev);
	_minor = getminor(dev);

	ASSERT(_major >= udev_emajor && _major < udev_emajor + udev_nmajors);
	ASSERT(_minor <= O_MAXMIN);

	i = ((_major - udev_emajor) << O_BITSMINOR) + _minor;

	FSPIN_LOCK(&udev_bits_mutex);
#ifdef DEBUG
	was_set = BITMASKN_TEST1(udev_bits, i);
#endif
	BITMASKN_CLR1(udev_bits, i);
	FSPIN_UNLOCK(&udev_bits_mutex);
	ASSERT(was_set);
}


/*
 * o_dev_t
 * make_cmpdev(dev_t rdev)
 *	Generate a compatibility device number, mapped to the real one.
 *
 * Calling/Exit State:
 *	This routine may block, so no locks may be held on entry.
 */
o_dev_t
make_cmpdev(dev_t rdev)
{
	udev_hash_t *udhp, **udhpp;
	dev_t cdev;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	udhpp = &udev_hash[UDEV_HASHF(rdev)];

retry:
	(void) LOCK_PLMIN(&udev_hash_mutex);
	for (udhp = *udhpp; udhp != NULL; udhp = udhp->ud_next) {
		if (udhp->ud_rdev == rdev) {
			cdev = udhp->ud_cdev;
			goto got_cdev;
		}
	}

	cdev = getudev();
	if (cdev == NODEV) {
		UNLOCK_PLMIN(&udev_hash_mutex, PLBASE);
		return O_NODEV;
	}
	ASSERT(cmpdev_fits(cdev));

	udhp = kmem_alloc(sizeof(udev_hash_t), KM_NOSLEEP);
	if (udhp == NULL) {
		if (udev_waiting) {
			SV_WAIT(&udev_sv, PRIMED, &udev_hash_mutex);
			goto retry;
		}
		udev_waiting = B_TRUE;
		UNLOCK_PLMIN(&udev_hash_mutex, PLBASE);

		udhp = kmem_alloc(sizeof(udev_hash_t), KM_SLEEP);

		(void) LOCK_PLMIN(&udev_hash_mutex);
		udev_waiting = B_FALSE;
	}

	udhp->ud_cdev = cdev;
	udhp->ud_rdev = rdev;
	udhp->ud_next = *udhpp;
	*udhpp = udhp;

got_cdev:
	UNLOCK_PLMIN(&udev_hash_mutex, PLBASE);

	return _cmpdev(cdev);
}


/*
 * dev_t
 * udev_getrdev(dev_t dev)
 *	Find the real dev_t, if any, corresponding to a fake cdev.
 *
 * Calling/Exit State:
 *	No locks may be held on entry.
 */
dev_t
udev_getrdev(dev_t dev)
{
	udev_hash_t *udhp;
	uint_t i;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	(void) LOCK_PLMIN(&udev_hash_mutex);

	for (i = 0; i < N_UDEV_HASH; i++) {
		for (udhp = udev_hash[i]; udhp; udhp = udhp->ud_next) {
			if (udhp->ud_cdev == dev) {
				dev = udhp->ud_rdev;
				break;
			}
		}
	}

	UNLOCK_PLMIN(&udev_hash_mutex, PLBASE);

	return dev;
}
