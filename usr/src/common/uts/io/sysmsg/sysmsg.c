/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/sysmsg/sysmsg.c	1.2"
#ident	"$Header: $"

/*
 * sysmsg driver --
 *
 * Provides a user-level device interface to kernel console output.
 * All characters written are sent to the console via the console switch.
 */

#include <fs/file.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <io/open.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>	/* Must come last */

int smsgdevflag = D_MP;

/*
 * The smsgslplkp sleep lock serializes read() access to one caller
 * at a time.
 */
STATIC sleep_t *smsgslplkp;
STATIC LKINFO_DECL(smsgslplkinfo, "sysmsg sleep lock", 0);
/*
 * The smsglockp basic lock and the smsgsvp synch variable are used
 * to protect the smsgibuf ring queue and related data structures.
 */
STATIC lock_t *smsglockp;
STATIC LKINFO_DECL(smsglkinfo, "sysmsg lock", 0);
STATIC sv_t *smsgsvp;

STATIC boolean_t smsgrawmode;

STATIC volatile char smsgibuf[128];
STATIC volatile uint_t smsgdrain, smsgfill, smsgline;

STATIC void smsgpoll(void *);

#define EOF_CHR	'\004'


/*
 * int
 * smsgopen(dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Check for errors and return the value.
 */
/* ARGSUSED */
int
smsgopen(dev_t *devp, int oflag, int otype, cred_t *crp)
{
	pl_t oldpri;

	if (getminor(*devp) != 0)
		return ENXIO;
	if (otype != OTYP_CHR && otype != OTYP_LYR)
		return EINVAL;

	if (smsglockp == NULL) {
		smsgslplkp = SLEEP_ALLOC(0, &smsgslplkinfo, KM_SLEEP);
		smsglockp = LOCK_ALLOC(0, pltimeout, &smsglkinfo, KM_SLEEP);
		smsgsvp = SV_ALLOC(KM_SLEEP);
	}

	return 0;
}


/*
 * int
 * smsgclose(dev_t dev, int flag, int otype, cred_t *crp)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
smsgclose(dev_t dev, int flag, int otype, cred_t *crp)
{
	return 0;
}


/*
 * int
 * smsgwrite(dev_t, uio_t *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
smsgwrite(dev_t dev, uio_t *uiop, cred_t *crp)
{
	int c;

	while ((c = uwritec(uiop)) != -1) {
		console_output_lock(conschan.cnc_consswp);
		if (c == '\n')
			while (CONSOLE_PUTC('\r') == 0)
				;
		while (CONSOLE_PUTC(c) == 0)
				;
		console_output_unlock(conschan.cnc_consswp, plbase);
	}

	return 0;
}


/*
 * int
 * smsgread(dev_t, uio_t *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
smsgread(dev_t dev, uio_t *uiop, cred_t *crp)
{
	boolean_t raw, signalled, got_any;
	toid_t toid;
	uint_t next;
	volatile uint_t *endp;
	int retval = 0;
	int c;

	ASSERT(getpl() == plbase);

	if (uiop->uio_resid == 0)
		return 0;

	/*
	 * Special check for raw mode, indicated by zero uio_fmode.
	 */
	if ((raw = (uiop->uio_fmode == 0)) == B_TRUE)
		uiop->uio_fmode = FREAD;

	endp = (raw ? &smsgfill : &smsgline);

	SLEEP_LOCK(smsgslplkp, prilo);	/* Only one can read at a time */

	smsgrawmode = raw;

	splhi();
	CONSOLE_SUSPEND();
	splbase();

	/*
	 * We'd like to use a timeout-based mechanism to simulate interrupts
	 * and allow typeahed, but some drivers don't implement CONSOLE_SUSPEND
	 * correctly, so we can't do this (yet).
	 */

	toid = itimeout(smsgpoll, NULL,
			    (HZ / 20) | TO_PERIODIC, pltimeout);
	if (toid == 0) {
		retval = EAGAIN;
		goto done2;
	}

	(void) LOCK(smsglockp, pltimeout);

	/*
	 * Check if any characters are available.
	 */
	while (smsgdrain == *endp) {
		if (uiop->uio_fmode & FNDELAY)
			goto done;
		if (uiop->uio_fmode & FNONBLOCK) {
			retval = EAGAIN;
			goto done;
		}
		/*
		 * Must get at least one character.
		 */
		signalled = !SV_WAIT_SIG(smsgsvp, prilo, smsglockp);
		(void) LOCK(smsglockp, pltimeout);
		if (signalled)
			break;
	}

	/*
	 * Read as many characters as requested;
	 * if not in raw mode, read only complete lines.
	 */
	got_any = B_FALSE;
	do {
		c = smsgibuf[smsgdrain];
		if (c == EOF_CHR && got_any)
			break;
		next = (smsgdrain + 1) % sizeof smsgibuf;
		if (smsgline == smsgdrain)
			smsgline = next;
		smsgdrain = next;
		UNLOCK(smsglockp, plbase);
		got_any = B_TRUE;
		if (c != EOF_CHR || raw)
			(void) ureadc(c, uiop);
		(void) LOCK(smsglockp, pltimeout);
		if (c == '\n' || c == EOF_CHR)
			break;
	} while (smsgdrain != *endp && uiop->uio_resid);

done:
	UNLOCK(smsglockp, plbase);

	untimeout(toid);

done2:
	splhi();
	CONSOLE_RESUME();
	splbase();

	SLEEP_UNLOCK(smsgslplkp);

	return retval;
}


/*
 * STATIC void
 * smsgpoll(void *arg)
 *	Check for available console characters.
 *
 * Calling/Exit State:
 *	Called as a timeout routine, at pltimeout.
 */
/* ARGSUSED */
STATIC void
smsgpoll(void *arg)
{
	boolean_t any_new = B_FALSE;
	uint_t next;
	int c;

	ASSERT(getpl() == pltimeout);

	(void) LOCK(smsglockp, pltimeout);
	while ((next = (smsgfill + 1) % sizeof smsgibuf) != smsgdrain &&
	       (c = CONSOLE_GETC()) != -1) {
		if (smsgrawmode)
			goto raw;
		console_output_lock(conschan.cnc_consswp);
		switch (c) {
		case '\r':
			c = '\n';
			/* FALLTHROUGH */
		case '\n':
			while (CONSOLE_PUTC('\r') == 0)
				;
			smsgline = next;
			/* FALLTHROUGH */
		default:
			while (CONSOLE_PUTC(c) == 0)
				;
raw:
			smsgibuf[smsgfill] = c;
			smsgfill = next;
			break;
		case '\b':
			if (smsgfill != smsgline) {
				while (CONSOLE_PUTC('\b') == 0)
					;
				while (CONSOLE_PUTC(' ') == 0)
					;
				while (CONSOLE_PUTC('\b') == 0)
					;
				if (smsgfill-- == 0)
					smsgfill = sizeof smsgibuf - 1;
			}
			break;
		case EOF_CHR:
			smsgline = next;
			goto raw;
		}
		if (!smsgrawmode)
			console_output_unlock(conschan.cnc_consswp, pltimeout);
		any_new = B_TRUE;
	}
	UNLOCK(smsglockp, pltimeout);

	if (any_new)
		SV_SIGNAL(smsgsvp, 0);
}
