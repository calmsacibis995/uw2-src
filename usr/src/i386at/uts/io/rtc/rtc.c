/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/rtc/rtc.c	1.4"
#ident	"$Header: $"

/* 
 * Driver for PC AT Calendar clock chip
 *	
 * PC-DOS compatibility requirements:
 *      1. must use local time, not GMT (sigh!)
 *      2. must use bcd mode, not binary
 *
 * To really use this device effectively there should be a program
 * called e.g. rtclock that works like date does.  i.e. "rtclock"
 * would return local time and "rtclock arg" would set local time arg
 * into the chip.  To set the system clock on boot /etc/rc should do
 * something like "date `rtclock`".
 */

#include <fs/file.h>
#include <io/rtc/rtc.h>
#include <io/uio.h>
#include <proc/signal.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>	/* must come last */

int rtcdevflag = 0;

/*
 * void
 * rtcinit(void)
 *	Initialize real time clock to interrupt us when the chip updates.
 *	(approx. once a second)
 *
 * Calling/Exit State:
 *	none
 */

void
rtcinit(void)
{
	struct rtc_t rtcbuf;

	if (!rtcget(&rtcbuf)) {
		/*
		 * reinitialize the real time clock.
		 */
		outb(RTC_ADDR, RTC_A);
		outb(RTC_DATA, RTC_DIV2 | RTC_RATE6);
		outb(RTC_ADDR, RTC_B);
		outb(RTC_DATA, RTC_HM);
		outb(RTC_ADDR, RTC_C);
		(void)inb(RTC_DATA);
		outb(RTC_ADDR, RTC_D);
		(void)inb(RTC_DATA);
	}
}


/*
 * int
 * rtcopen(dev_t *devp, int flags, int otyp, void *cred_p)
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcopen(dev_t *devp, int flags, int otyp, void *cred_p)
{
	int error;

	if ((flags & FWRITE) && (error = drv_priv(cred_p)) != 0)
		return error;

	return 0;
}

/*
 * int
 * rtcread(dev_t dev, struct uio *uio_p, void *cred_p)
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcread(dev_t dev, struct uio *uio_p, void *cred_p)
{
	return 0;
}

/*
 * int
 * rtcwrite(dev_t dev, struct uio *uio_p, void *cred_p)
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcwrite(dev_t dev, struct uio *uio_p, void *cred_p)
{
	return 0;
}


/*
 * int
 * rtcioctl(dev_t dev, int cmd, void *addr, int mode,
 *	    void *cred_p, int *rval_p)
 *	This is used to read and write the clock.
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcioctl(dev_t dev, int cmd, void *addr, int mode,
	 void *cred_p, int *rval_p)
{
	struct rtc_t rtcbuf;
	int ecode = 0;

	switch (cmd) {
	case RTCRTIME:
		if (rtcget(&rtcbuf)) {
			ecode = EIO;
			break;
		}
		if (copyout(&rtcbuf, addr, RTC_NREG) != 0)
			ecode = EFAULT;
		break;
	case RTCSTIME:
		if ((ecode = drv_priv(cred_p)) != 0) {
			break;
		}
		if (copyin(addr, &rtcbuf, RTC_NREGP) != 0) {
			ecode = EFAULT;
			break;
		}
		rtcput(&rtcbuf);
		break;
	default:
		ecode = EINVAL;
		break;
	}

	return ecode;
}

/*
 * int
 * rtcclose(dev_t dev, int flags, int otyp, void *cred_p)
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
int
rtcclose(dev_t dev, int flags, int otyp, void *cred_p)
{
	return 0;
}

/*
 * int
 * rtcget(struct rtc_t *buf)
 *	Read contents of real time clock to the specified buffer.
 *
 * Calling/Exit State:
 *	Returns -1 if clock not valid, else 0.
 *	Returns RTC_NREG (which is 15) bytes of data in (*buf), as given
 *	in the technical reference data.  This data includes both the time
 *	and the status registers.
 */

int
rtcget(struct rtc_t *buf)
{
	uchar_t *bufp;
	uchar_t reg;
	uint_t i, cnt;
	pl_t pl;

	pl = splhi();

	outb(RTC_ADDR, RTC_D); /* check if clock valid */
	reg = inb(RTC_DATA);
	if (!(reg & RTC_VRT)) {
		splx(pl);
		return -1;
	}

	cnt = 0;
checkuip:
	/*
	 * Check if the clock is in the middle of updating itself
	 * (for the next second).  During such a time, the clock data
	 * are not stable and may not be self-consistent.
	 */
	outb(RTC_ADDR, RTC_A); /* check if update in progress */
	reg = inb(RTC_DATA);
	if (reg & RTC_UIP) {
		drv_usecwait(1);
		goto tryagain;
	}

	bufp = (uchar_t *)buf;
	for (i = 0; i < RTC_NREG; i++) {
		outb(RTC_ADDR, i);
		*bufp++ = inb(RTC_DATA);
	}

	/*
	 * Check again and compare results, in case an update started
	 * while we were reading the data out.
	 */

	outb(RTC_ADDR, RTC_A); /* check if update in progress */
	reg = inb(RTC_DATA);
	if (reg & RTC_UIP) {
		drv_usecwait(1);
		goto tryagain;
	}

	bufp = (uchar_t *)buf;
	for (i = 0; i < RTC_NREG; i++) {
		outb(RTC_ADDR, i);
		if (inb(RTC_DATA) != *bufp++)
			goto tryagain;
	}

	splx(pl);

	return 0;

tryagain:
	if (++cnt == 1000) {
		splx(pl);
		/*
		 *+ There is a problem with the hardware Real Time Clock.
		 */
		cmn_err(CE_WARN, "Real Time Clock not responding");
		return -1;
	}
	goto checkuip;
}

/*
 * void
 * rtcput(struct rtc_t *buf)
 *	This routine writes the contents of the given buffer to the real time
 *	clock.
 *
 * Calling/Exit State:
 *	Takes RTC_NREGP bytes of data, which are the 10 bytes
 *	used to write the time and set the alarm.
 */

void
rtcput(struct rtc_t *buf)
{
	uchar_t *bufp;
	uchar_t reg;
	uint_t i;
	pl_t pl;

	pl = splhi();

	outb(RTC_ADDR, RTC_B);
	reg = inb(RTC_DATA);
	outb(RTC_ADDR, RTC_B);
	outb(RTC_DATA, reg | RTC_SET); /* allow time set now */
	bufp = (uchar_t *)buf;
	for (i = 0; i < RTC_NREGP; i++) { /* set the time */
		outb(RTC_ADDR, i);
		outb(RTC_DATA, *bufp++);
	}
	outb(RTC_ADDR, RTC_B);
	outb(RTC_DATA, reg & ~RTC_SET); /* allow time update */

	splx(pl);
}

/*
 * void
 * rtcintr(int ivect)
 *	handle interrupt from real time clock
 *
 * Calling/Exit State:
 *	none
 */

/* ARGSUSED */
void
rtcintr(int ivect)
{
	outb(RTC_ADDR, RTC_C); /* clear interrupt */
	(void)inb(RTC_DATA);
}
