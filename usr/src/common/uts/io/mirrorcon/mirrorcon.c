/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/mirrorcon/mirrorcon.c	1.1"
#ident	"$Header: $"

/*
 * mirrorcon driver --
 *
 * Console device driver which allows multiple console devices to be used
 * simultaneously.  The list of real console devices, separated by plus
 * signs ('+'), is passed as the parameter string to mcon_cnopen().  All
 * outputs are duplicated to each of the real devices.  Input from any
 * real device will be accepted and passed on.
 */

#include <io/conf.h>
#include <io/conssw.h>
#include <mem/hatstatic.h>
#include <svc/errno.h>
#include <util/types.h>

#include <io/ddi.h>	/* Must come last */

int mcon_devflag = D_MP;

extern int bs_lexcon(const char **strp, conssw_t **cswp, minor_t *mp,
		     char **paramstrp);

STATIC dev_t mcon_cnopen(minor_t, boolean_t, const char *);
STATIC void mcon_cnclose(minor_t, boolean_t);
STATIC int mcon_cnputc(minor_t, int);
STATIC int mcon_cngetc(minor_t);
STATIC void mcon_cnsuspend(minor_t);
STATIC void mcon_cnresume(minor_t);

conssw_t mcon_conssw = {
	mcon_cnopen, mcon_cnclose, mcon_cnputc, mcon_cngetc,
	mcon_cnsuspend, mcon_cnresume
};

typedef struct mcon_dev {
	conschan_t	mc_chan;
	uint_t		mc_flags;
} mcon_dev_t;

/* Flag values for mc_flags: */
#define MC_SENT		(1 << 0)	/* char has been sent to this dev */

STATIC mcon_dev_t *mcon_devs;
STATIC uint_t mcon_ndev, mcon_nalloced;


/*
 * STATIC dev_t
 * mcon_cnopen(minor_t minor, boolean_t syscon, const char *params)
 *	Mirrorcon console open
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity
 */
STATIC dev_t
mcon_cnopen(minor_t minor, boolean_t syscon, const char *params)
{
	const char *p;
	uint_t ndev;
	dev_t retdev;
	mcon_dev_t *mcp;
	conssw_t *cswp;
	char *dparams;

	if (minor != 0)
		return NODEV;
	/*
	 * Make a first pass through params to count the number of devices.
	 */
	for (ndev = 0, p = params;;) {
		if (!bs_lexcon(&p, &cswp, &minor, NULL))
			return ENODEV;
		++ndev;
		if (*p == '\0')
			break;
		if (*p++ != '+')
			return ENODEV;
	}
	if (ndev == 0)
		return ENODEV;
	/*
	 * Allocate per-device array, if not already allocated.
	 */
	if (mcon_nalloced < ndev) {
		void *mem = consmem_alloc(ndev * sizeof(mcon_dev_t), 0);
		if (mem == NULL)
			return ENODEV;
		mcon_devs = mem;
		mcon_nalloced = ndev;
	}
	/*
	 * Now parse params to open each device and fill in mcon_devs.
	 */
	retdev = NODEV;
	for (mcp = mcon_devs, p = params;;) {
		if (!bs_lexcon(&p, &cswp, &minor, &dparams))
			goto cont;
		bzero(mcp, sizeof *mcp);
		if (!console_openchan(&mcp->mc_chan, cswp, minor, dparams,
				      syscon))
			goto cont;
		/*
		 * Pick the first device to use for the dev_t.
		 */
		if (retdev == NODEV)
			retdev = mcp->mc_chan.cnc_dev;
		++mcp;
cont:
		if (*p == '\0')
			break;
		if (*p++ != '+')
			return ENODEV;
	}
	mcon_ndev = mcp - mcon_devs;
	return retdev;
}

/*
 * STATIC void
 * mcon_cnclose(minor_t minor, boolean_t syscon)
 *	Mirrorcon console close
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity
 */
STATIC void
mcon_cnclose(minor_t minor, boolean_t syscon)
{
	mcon_dev_t *mdev;

	for (mdev = &mcon_devs[mcon_ndev]; mdev-- != mcon_devs;)
		console_closechan(&mdev->mc_chan);
}

/*
 * STATIC int
 * mcon_cnputc(minor_t minor, int chr)
 *	Mirrorcon console output
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity
 */
STATIC int
mcon_cnputc(minor_t minor, int chr)
{
	mcon_dev_t *mdev;
	int retval = 1;

	for (mdev = &mcon_devs[mcon_ndev]; mdev-- != mcon_devs;) {
		if (mdev->mc_flags & MC_SENT)
			continue;
		if (console_putc(&mdev->mc_chan, chr)) {
			mdev->mc_flags |= MC_SENT;
			continue;
		}
		retval = 0;
	}
	if (retval == 1) {
		for (mdev = &mcon_devs[mcon_ndev]; mdev-- != mcon_devs;)
			mdev->mc_flags &= ~MC_SENT;
	}

	return retval;
}

/*
 * STATIC int
 * mcon_cngetc(minor_t minor)
 *	Mirrorcon console input
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity
 */
STATIC int
mcon_cngetc(minor_t minor)
{
	mcon_dev_t *mdev;
	int chr;

	for (mdev = &mcon_devs[mcon_ndev]; mdev-- != mcon_devs;) {
		if ((chr = console_getc(&mdev->mc_chan)) != -1)
			return chr;
	}
	return -1;
}

/*
 * STATIC void
 * mcon_cnsuspend(minor_t minor)
 *	Mirrorcon console suspend normal input
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity
 */
STATIC void
mcon_cnsuspend(minor_t minor)
{
	mcon_dev_t *mdev;

	for (mdev = &mcon_devs[mcon_ndev]; mdev-- != mcon_devs;)
		console_suspend(&mdev->mc_chan);
}

/*
 * STATIC void
 * mcon_cnresume(minor_t minor)
 *	Mirrorcon console resume normal input
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity
 */
STATIC void
mcon_cnresume(minor_t minor)
{
	mcon_dev_t *mdev;

	for (mdev = &mcon_devs[mcon_ndev]; mdev-- != mcon_devs;)
		console_resume(&mdev->mc_chan);
}
