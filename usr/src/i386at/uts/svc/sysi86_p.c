/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/sysi86_p.c	1.4"
#ident	"$Header: $"

/*
 * Platform-specific sysi86() system call subfunctions for i386at.
 */

#include <acc/priv/privilege.h>
#include <io/ioctl.h>
#include <io/rtc/rtc.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/p_sysi86.h>
#include <svc/sysi86.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern int rtodc(struct rtc_t *clkp);

extern time_t time;	/* XXX - obsolete */

extern major_t sdi_pass_thru_major;

time_t localtime_cor = 0;		/* Local time correction in secs */


/*
 * int
 * sysi86_p(struct sysi86a *uap, rval_t *rvp)
 *	Platform-specific extensions to sysi86 syscall.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
sysi86_p(struct sysi86a *uap, rval_t *rvp)
{
	struct rtc_t clkx;
	dev_t sdi_dev;

	switch (uap->cmd) {

	case RTODC:	/* read time-of-day clock */

		if (pm_denied(CRED(), P_SYSOPS))
			return EPERM;
		if (rtodc(&clkx))
			return ENXIO;
		if (copyout(&clkx, uap->arg.sparg, sizeof(clkx)) == -1)
			return EFAULT;
		break;

	case STIME:	/* set internal time, not hardware clock */
		if (pm_denied(CRED(), P_SYSOPS))
			return EPERM;
		TIME_LOCK();
		time = hrestime.tv_sec = (time_t)uap->arg.larg;
		hrestime.tv_nsec = 0;
		TIME_UNLOCK();
		break;
 
	/*
	 * Set the local time correction in secs (includes Daylight savings)
	 */
	case SI86SLTIME:
		if (pm_denied(CRED(), P_SYSOPS))
			return EPERM;
		localtime_cor = (time_t)(uap->arg.larg);
		break;

	case SI86RDID:	/* Read the ROM BIOS ID bit */
		if ((bootinfo.id[0] == 'I') && (bootinfo.id[1] == 'D') &&
		    (bootinfo.id[2] == 'N') && (bootinfo.id[3] == 'O'))
			rvp->r_val1 = bootinfo.id[4];
		else
			rvp->r_val1 = 0;
		break;

	case SI86RDBOOT: /* Bootable Non-SCSI Hard Disk? */
		if (bootinfo.hdparams[0].hdp_ncyl == 0)
			rvp->r_val1 = 0;
		else
			rvp->r_val1 = 1;
		break;

	case SI86BUSTYPE: /* What bus type is this machine? */
		rvp->r_val1 = EISA_BUS;
		if (bootinfo.machflags & MC_BUS)
			rvp->r_val1 = MCA_BUS;
		else if (bootinfo.machflags & AT_BUS)
			rvp->r_val1 = ISA_BUS;
		break;

	case SI86SDIDEV: /* What is the SDI pass-thru device? */
		sdi_dev = makedevice(sdi_pass_thru_major, (minor_t)0);
		if (copyout(&sdi_dev, uap->arg.lparg, sizeof(dev_t)) == -1)
			return EFAULT;
		break;

	default:
		return EINVAL;
	}

	return 0;
}
