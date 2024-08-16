/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)kern:util/cmn_err.c	1.31"
#ident	"$Header: $"

#include <io/conssw.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <mem/kmem.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ghier.h>
#include <util/inline.h>
#include <util/kcontext.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#define TO_PUTBUF	0x1
#define TO_CONSOLE	0x2

extern void *saveregs(kcontext_t *);
extern void call_demon(void);

extern void panic_start(kcontext_t *);
extern void panic_shutdown(void);
extern void double_panic(kcontext_t *);
extern void concurrent_panic(kcontext_t *);

STATIC void putchar(char);
int  conslog_set(int);

STATIC void xprintf(int, const char *, VA_LIST, const char *);
STATIC void xxprintf(const char *, VA_LIST, const char *);
STATIC void printn(long, int, int, int, int);
STATIC void output(char);
STATIC void xcmn_err(int, const char *, VA_LIST);
STATIC void xcmn_panic(int, const char *, VA_LIST);

STATIC lock_t cmn_err_lock;
	/*+ cmn_err() and printf() serialization */
STATIC LKINFO_DECL(cmn_err_lkinfo, "KU:CMN_ERR:cmn_err_lock", LK_BASIC);
lock_t putbuf_lock;
	/*+ mutex putbuf access */
STATIC LKINFO_DECL(putbuf_lkinfo, "KU:CMN_ERR:putbuf_lock", LK_BASIC);
lock_t panic_lock;
	/*+ panic serialization */
STATIC LKINFO_DECL(panic_lkinfo, "KU:CMN_ERR:panic_lock", LK_BASIC);

#ifndef UNIPROC
#define	NOTWAITING	0
#define	PANIC_TAKE1	2
#define	PANIC_TAKE2	1
/* panic_waiting handshake between multiple concurrent panics */
STATIC volatile int panic_waiting[MAXNUMCPU];
#endif /* UNIPROC */

#ifdef DEBUG
STATIC char pclastc[MAXNUMCPU]; /* last char output; for engine number output */
#endif

extern int  in_demon;	/* Flag: kernel debugger is running */

extern char putbuf[];
extern int  putbufsz;

int putbufrpos;	/* next byte to read in putbuf[] */
int putbufwpos;	/* next byte to write in putbuf[] */
ulong_t putbufwrap; /* # times putbufwpos has wrapped around to 0 */

static int  conslog_st;	/* current state of console logging */
static char consbuf[256];
static int  conspos;
static int  where;
static int  conslogging;

/*
 * void
 * cmn_err_init(void)
 *	Initialize locks and things needed for this file.
 *
 * Calling/Exit State:
 *	Should be called before printing to the console is enabled.
 *	Returns: none.
 */
void
cmn_err_init(void)
{
	unsigned int i;

	LOCK_INIT(&cmn_err_lock, CMNERR_HIER, PLHI, &cmn_err_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&putbuf_lock, PUTBUF_HIER, PLHI, &putbuf_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&panic_lock, PANIC_HIER, PLHI, &panic_lkinfo, KM_NOSLEEP);

	conslog_st = CONSLOG_ENA;

#ifdef DEBUG
	for (i = MAXNUMCPU; i-- != 0;)
		pclastc[i] = '\n';
#endif
}

/*
 * STATIC void
 * xprintf(int wh, const char *fmt, VA_LIST ap, const char *prefix)
 *	An internal routine for cmn_err()
 * Calling/Exit State:
 *	call serialized using cmn_err_lock.
 *	Returns: none.
 */
STATIC void
xprintf(int wh, const char *fmt, VA_LIST ap, const char *prefix)
{
	pl_t pl;

	where = wh;
	if (in_demon)
		conslog_set(CONSLOG_DIS);
	if ((conslog_set(CONSLOG_STAT) == CONSLOG_DIS) || !canconslog()) {
		conslogging = 0;
	} else { 
		conslogging = 1;
		conspos = 0;
	}
	if (!(conslogging && (wh & TO_CONSOLE)) && (wh & TO_PUTBUF)) {
		/*
		 * xxprintf() sometimes does not
		 * access putbuf[] even when
		 * (wh & TO_PUTBUF) is true.
		 */
		pl = LOCK(&putbuf_lock, PLHI);
		xxprintf(fmt, ap, prefix);
		UNLOCK(&putbuf_lock, pl);
	} else
		xxprintf(fmt, ap, prefix);
	if (in_demon)
		conslog_set(CONSLOG_ENA);
}

/*
 * STATIC void
 * xxprintf(const char *fmt, VA_LIST ap, const char *prefix)
 *	An internal routine to implement printf().
 *
 * Calling/Exit State:
 *	Return: none.
 *	cmn_err_lock assumed held.  putbuf_lock should also be
 *	acquired when writing to putbuf[].
 */
STATIC void
xxprintf(const char *fmt, VA_LIST ap, const char *prefix)
{
	char c;
	char *s;
	int width, prec;

	if (prefix) {
		while (*prefix != '\0')
			output(*prefix++);
	}

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0') {
			if (prefix)
				output('\n');
			return;
		}
		output(c);
	}
	width = prec = 0;
	while ('0' <= *fmt && *fmt <= '9')
		width = width * 10 + (*fmt++ - '0');
	if (*fmt == '.') {
		++fmt;
		while ('0' <= *fmt && *fmt <= '9')
			prec = prec * 10 + (*fmt++ - '0');
	}
	/* For compatibility, we support "%.3d" as if it were "%3d". */
	if (prec && !width)
		width = prec;
	if ((c = *fmt++) == 'l')
		c = *fmt++;

	switch (c) {
	case 'd':
	case 'u':
		printn(VA_ARG(ap, long), 10, (c != 'u'), width, 0);
		break;
	case 'o':
		printn(VA_ARG(ap, long), 8, 0, width, 11);
		break;
	case 'x':
	case 'X':
		printn(VA_ARG(ap, long), 16, 0, width, 8);
		break;
	case 'b':
		printn((long)(VA_ARG(ap, int) & 0xFF), 16, 0, width, 2);
		break;
	case 's':
		s = VA_ARG(ap, char *);
		while ((c = *s++) != 0)
			output(c);
		break;
	case 'c':
		c = (VA_ARG(ap, int) & 0xFF);
		output(c);
		break;
	default:
		/* unknown format -- print it */
		output(c);
	}
	goto loop;
}

/*
 * STATIC void
 * printn(long n, int b, int sflag, int width, int zero)
 *	An internal routine to implement printf() -
 *	Print a number in a specified format.
 *
 * Calling/Exit State:
 *	Prints a number <n> in the base <b>.
 * 	It prints unsigned if <sflag> is 0; signed otherwise.
 *	It prints in variable width if <width> is 0; pads with
 *	spaces on the left, to <width> otherwise.
 *	If <zero> is non-zero, pads with leading '0's to width <zero>.
 */
STATIC void
printn(long n, int b, int sflag, int width, int zero)
{
	unsigned long nn = n;
	int i;
	char d[11];  /* 32 bits in octal needs 11 digits */

	if (sflag && n < 0) {
		output('-');
		nn = -nn;
	}
	for (i=0;;) { /* output at least one digit (for 0) */
		d[i++] = nn % b;
		nn = nn / b;
		if (nn == 0)
			break;
	}

	while (width-- > (zero > i ? zero : i))
		output(' ');
	while (zero-- > i)
		output('0');
	while (i-- > 0)
		output("0123456789ABCDEF"[d[i]]);
}

/* void
 * output(char c)
 *	An intermediate output routine
 *
 * Calling/Exit State:
 *	Should be called with putbuf_lock held when writing to putbuf[].
 *	Returns: none.
 *
 * Description:
 *	Save output in a putbuf[] where we can look at it
 *	with a kernel debugger or with crash.
 *	If the message begins with a '!', then only put it
 *	in putbuf[], not out to the console.
 *	Because of console logging feature, !/^ rule is now
 *	interpreted as follows:
 *	+-----------------------------------------------+
 *	| 1st char/	|_______console_logging_________|
 *	| destinations	| disabled	| enabled	|
 *	|---------------+---------------+---------------|
 *	| ! putbuf[]	| putbuf[]	| putbuf[]	|
 *	|...............................................|
 *	| ^ console	| putchar()	| strlog() ++	|
 *	|...............................................|
 *	| otherwise	| putbuf[],	| strlog()	|
 *	|   both	| putchar()	|		|
 *	+-----------------------------------------------+
 *	(++) even if the message begins with a '^', put
 *	it in putbuf[], when console logging is enabled.
 */
STATIC void
output(char c)
{
	if (conslogging && (where & TO_CONSOLE)) {
		if (conspos < sizeof consbuf - 1)
			consbuf[conspos++] = c;
		return;
	}
	if (where & TO_PUTBUF) {
		if (putbufwpos >= putbufsz) {
			putbufwpos = 0;
			++putbufwrap;
		}
		putbuf[putbufwpos++] = c;
		putbufrpos = putbufwpos;
	}
	if (where & TO_CONSOLE)
		putchar(c);
}

/*
 * void
 * putchar(char c)
 *	Put a char on the console.
 * Calling/Exit State:
 *	Nothing in particular.
 *	Returns:  none.
 */
STATIC void
putchar(char c)
{
#ifdef DEBUG
	if (pclastc[l.eng_num] == '\n' && upyet && Nengine > 1) {
		pclastc[l.eng_num] = '\0';
		/* prepend engine number to output lines. */
		putchar('0' + (l.eng_num / 10));
		putchar('0' + (l.eng_num % 10));
		putchar(':');
		putchar(' ');
	}
#endif
	if (c == '\n')
		putchar('\r');
	while (CONSOLE_PUTC(c & 0xff) == 0)
		;
#ifdef DEBUG
	if (upyet && Nengine > 1)
		pclastc[l.eng_num] = c;
#endif
}

/*
 * void
 * cmn_err(int, const char *, ...)
 *	Common error handling.
 *	Prints formatted messages with a subset of libc printf format controls.
 *
 * Calling/Exit State:
 *	Returns: none.
 *	No particular locking assumed.
 *
 * Description:
 *	Used to print diagnostic information directly on console device.
 *	Since it is not interrupt driven and at PLHI,
 *	all system activities are pretty much suspended for possibly
 *	quite a long time.
 *
 *	Restrictions are:
 *	- only %s, %u, %d (==%u), %o, %X, %x (==%X), %c, %b (byte)
 *	  are recognized.
 *	- "l (ell) option" is recognized but ignored.
 *	- "field width" is disallowed.
 *	- numeric "precision" is allowed.
 */
void
cmn_err(int level, const char *fmt, ...)
{
	VA_LIST ap;

	VA_START(ap, fmt);

	xcmn_err(level, fmt, ap);
}

/*
 * void
 * panic_printf(const char *, ...)
 *	Print a formatted string from a panic routine.
 *
 * Calling/Exit State:
 *	Caller holds panic_lock and cmn_err_lock.
 *	Should only be called from panic.c .
 */
void
panic_printf(const char *fmt, ...)
{
	VA_LIST ap;

	VA_START(ap, fmt);

	xprintf(TO_CONSOLE|TO_PUTBUF, fmt, ap, NULL);
}


/*
 * STATIC void
 * do_conslog(int sltyp, pl_t pl)
 *	Send queued up output to strlog() when conslogging
 *
 * Calling/Exit State:
 *	Called with the cmn_err_lock lock held; returns with this lock
 *	released and ipl dropped to pl.
 */
STATIC void
do_conslog(int sltyp, pl_t pl)
{
	char cbuf[256];

	/* Copy out to a private buffer before dropping cmn_err_lock */
	bcopy(consbuf, cbuf, conspos);
	cbuf[conspos] = '\0';
	UNLOCK(&cmn_err_lock, pl);

	(void) strlog(0, 0, 0, SL_CONSOLE|sltyp, cbuf);
}


/*
 * STATIC void
 * xcmn_err(int level, const char *fmt, VA_LIST ap)
 *	Internal routine to implement cmn_err().
 *
 * Calling/Exit State:
 *	The first parameter indicates level. The legal values for
 *	this parameter are:
 *	CE_CONT - to display additional lines of a
 *			multi-line kernel message
 *	CE_NOTE - to report unusual kernel events that
 *			are non-fatal
 *	CE_WARN - to warn the system administrator about events that
 *			may cause unexpected failures or
 *			performance degradation 
 *	CE_PANIC - to report a fatal error that requires
 *			taking down the machine gracefully.
 *	The second parameter includes the format for a message to
 *	be printed on the console and the message itself.
 *
 * 	No return value.
 *
 * Description:
 *	In the case of CE_CONT, CE_NOTE and CE_WARN, a message is
 *	printed on the console.  The CE_PANIC case is handled by xcmn_panic().
 */
STATIC void
xcmn_err(int level, const char *fmt, VA_LIST ap)
{
	pl_t pl;
	int wh, sltyp;
	char *prefix = NULL;

	if (*fmt == '^') {
		wh = TO_CONSOLE;
		++fmt;
	} else if (*fmt == '!') {
		wh = TO_PUTBUF;
		++fmt;
	} else
		wh = TO_CONSOLE|TO_PUTBUF;
	sltyp = 0;

	switch (level) {

	case CE_NOTE:
		prefix = "\nNOTICE: ";
		sltyp = SL_NOTE;
		goto do_xprintf;

	case CE_WARN:
		prefix = "\nWARNING: ";
		sltyp = SL_WARN;
		/* FALLTHRU */

	case CE_CONT:
do_xprintf:
		pl = LOCK(&cmn_err_lock, PLHI);
		xprintf(wh, fmt, ap, prefix);
		if (conslogging && conspos != 0)
			do_conslog(sltyp, pl);
		else
			UNLOCK(&cmn_err_lock, pl);
		break;

	case CE_PANIC:
		xcmn_panic(wh, fmt, ap);
		/* NOTREACHED */

	default:
		/*
		 *+ An illegal 'level' argument was passed to
		 *+ cmn_err.  This indicates a kernel software
		 *+ problem.
		 */
		cmn_err(CE_PANIC,
		    "unknown level: cmn_err(level=%d, msg=\"%s\")",
		    level, fmt);
		/* NOTREACHED */
	}
}

/*
 * STATIC void
 * xcmn_panic(int where, const char *fmt, VA_LIST ap)
 *	Internal routine to implement cmn_err(CE_PANIC).
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Prints the PANIC message and calls platform-dependent routines
 *	to save and print machine state. If multiple processors try to take
 *	the machine down, only one will succeed in acquiring panic_lock and the
 *	others pause. The processor that has the lock then completes the
 *	panic processing.
 */
STATIC void
xcmn_panic(int where, const char *fmt, VA_LIST ap)
{
	kcontext_t *kcp;
#ifndef UNIPROC
	uint_t i, j;
#endif

	/*
	 * Disable interrupts and preemption.
	 */
	(void) splhi();

	if (++l.panic_level > 2) {
		/*
		 * If panic level is greater than 2, we're probably
		 * recursing here, due to some problem in something we're
		 * accessing.  Try to get out while doing the least amount
		 * we can, to reduce the likelihood of recursing again.
		 */
		double_panic(NULL);
	}

	/*
	 * Console loggers will never run;
	 * force messages to go to console.
	 */
	(void) conslog_set(CONSLOG_DIS);
	CONSOLE_SUSPEND();

	kcp = (kcontext_t *)saveregs(&u.u_kcontext);

	if (l.panic_level == 1) {
#ifndef UNIPROC
		if (TRYLOCK(&panic_lock, PLHI) != INVPL) {
			/*
			 * Use TRYLOCK() instead of LOCK(), since we don't want
			 * anything to prevent us from eventually continuing
			 * (especially hierarchy violations).
			 */
			for (i = 10000;
			     TRYLOCK(&cmn_err_lock, PLHI) == INVPL &&
			     i-- != 0;)
				;
		} else {
			/* Multiple processors in panic. */
			panic_waiting[l.eng_num] = PANIC_TAKE2;
			while (panic_waiting[l.eng_num] == PANIC_TAKE2)
				;
			xprintf(where, fmt, ap, "\nCONCURRENT PANIC: ");
			panic_waiting[l.eng_num] = NOTWAITING;
			concurrent_panic(kcp);
			/* NOTREACHED */
		}
#endif /* !UNIPROC */

		xprintf(where, fmt, ap, "\nPANIC: ");
		panic_start(kcp);

#ifndef UNIPROC
		/*
		 * Give other engines a chance to print concurrent panic
		 * messages, if any.
		 */
		for (i = Nengine; i-- != 0;) {
			if (i == l.eng_num || panic_waiting[i] == NOTWAITING)
				continue;
			panic_waiting[i] = PANIC_TAKE1;
			for (j = 100000;
			    panic_waiting[i] == PANIC_TAKE1 && j-- != 0;)
				;
		}
#endif /* UNIPROC */

		UNLOCK(&cmn_err_lock, PLHI);
		panic_shutdown();
		/* NOTREACHED */
	}

	xprintf(where, fmt, ap, "\nDOUBLE PANIC: ");
	double_panic(kcp);
	/* NOTREACHED */
}

/*
 * int
 * conslog_set(int s)
 *	The interface to disable console logging temporarily.
 *
 * Calling/Exit State:
 *	When console logging is disabled all console messages
 *	are sent directly to the system console.  We need
 *	to disable console logging during some kernel functions,
 *	for instance when the system enters debug mode.
 *
 *	CONSLOG_DIS:	disable console logging
 *	CONSLOG_ENA:	enable console logging only if /dev/log
 *			reader is registered.
 *	CONSLOG_STAT:	return current state of console logging.
 */
int
conslog_set(int s)
{
	int rc = 0;
	extern int upyet;

	switch (s) {

	case CONSLOG_DIS:
		conslog_st = s;
		break;

	case CONSLOG_ENA:
		if (!upyet) {
			rc = -1;
			break;
		}
		if (canconslog())
			conslog_st = s;
		else
			rc = -1;
		break;

	case CONSLOG_STAT:
		rc = conslog_st;
	}
	return rc;
}

/*
 * void
 * console_output_lock(struct conssw *conssw)
 *	Serialize console output.
 *
 * Calling/Exit State:
 *	On return, the console output lock will be locked at PLHI;
 *	it is assumed that the caller already holds their own lock, so the
 *	previous pl level is not returned.
 *
 *	This lock must be held in order to call CONSOLE_PUTC(); therefore,
 *	a console driver can call console_output_lock() to protect against
 *	simultaneous console output while it is handling user-level output.
 *
 *	The lock is only actually acquired if conssw is the current
 *	console device.  The caller must not assume that a lock was acquired
 *	or that the pl was changed.
 */
void
console_output_lock(struct conssw *conssw)
{
	if (conssw == consswp)
		(void) LOCK(&cmn_err_lock, PLHI);
}

/*
 * void
 * console_output_unlock(struct conssw *conssw, pl_t pl)
 *	Unlock console output.
 *
 * Calling/Exit State:
 *	Must match a previous call to console_output_lock().
 *	If conssw is the current console device, the console output lock
 *	is unlocked and the pl level is restored to pl.
 *
 *	Since the pl level is not changed if not the console device,
 *	pl must be the same as the current pl at the time of the call to
 *	console_output_lock() in order to achieve consistent results.
 */
void
console_output_unlock(struct conssw *conssw, pl_t pl)
{
	if (conssw == consswp)
		UNLOCK(&cmn_err_lock, pl);
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_putbuf(void)
 *	Print out contents of putbuf[].
 *
 * Calling/Exit State:
 *	None.
 *
 * Discussion:
 *	No locks are acquired because this routine is only called
 *	from a kernel debugger.
 */
void
print_putbuf(void)
{
	int i, wpos;
	int c;

	ASSERT(in_demon);

	i = wpos = putbufwpos;
	ASSERT(wpos >= 0 && wpos <= putbufsz);

	if (i != putbufsz && putbuf[i] != '\0') {
		do {
			debug_printf("%c", putbuf[i++]);
			if (debug_output_aborted())
				return;
		} while (i != putbufsz);
	}
	i = 0;
	while (i != wpos) {
		debug_printf("%c", putbuf[i++]);
		if (debug_output_aborted())
			return;
	}
}

#endif /* DEBUG || DEBUG_TOOLS */
 
/*
 * void
 * printf(const char *, ...)
 *	Obsolete interface to cmn_err(CE_CONT, ...); for old drivers.
 *
 * Calling/Exit State:
 *	No return value.
 */
void
printf(const char *fmt, ...)
{
	VA_LIST ap;

	VA_START(ap, fmt);

	xprintf(TO_CONSOLE|TO_PUTBUF, fmt, ap, NULL);
}

/*
 * void
 * _Compat_panic(char *fmt, ...)
 *	Obsolete interface to cmn_err(CE_PANIC, ...); for old drivers.
 *
 * Calling/Exit State:
 *	It may be called on unresolvable fatal errors.
 */
void
_Compat_panic(const char *fmt, ...)
{
	VA_LIST ap;

	VA_START(ap, fmt);

	xcmn_panic(TO_CONSOLE|TO_PUTBUF, fmt, ap);
	/* NOTREACHED */
}

/*
 * void
 * debug_printf(const char *fmt, ...)
 *	Print function for debugging routines.
 *
 * Calling/Exit State:
 *	The first arg is a printf format string.  Any remaining arguments
 *	are values to be printed according to the format string.
 *
 * Remarks:
 *	If the system's kernel debugger provides an enhanced printing function
 *	(i.e. a printf-like function with output flow control), cmn_err.h
 *	should define a macro _DEBUG_PRINTF(fmt, ap) which will invoke that
 *	printing function.  This macro will only be invoked if the system is
 *	currently running in the kernel debugger (as indicated by in_demon).
 *
 *	The effective prototype for _DEBUG_PRINTF is:
 *
 *		void _DEBUG_PRINTF(const char *fmt, VA_LIST ap);
 *
 *	We go to these lengths because output flow control can only be
 *	done when all engines have been suspended and the console has
 *	been suspended.  Kernel debuggers of necessity have to do this.
 */
void
debug_printf(const char *fmt, ...)
{
	VA_LIST ap;

	VA_START(ap, fmt);

#ifdef _DEBUG_PRINTF
	if (in_demon) {
		_DEBUG_PRINTF(fmt, ap);
		return;
	}
#endif
	xprintf(TO_CONSOLE, fmt, ap, NULL);
}


/*
 * boolean_t
 * debug_output_aborted(void)
 *	Check if output was aborted during debug_printf().
 *
 * Calling/Exit State:
 *	Called from a debugging print routine.  Returns B_TRUE if
 *	output has been aborted for this command.
 *
 *	It is not required to call this function before every
 *	debug_printf() call, since calls to debug_printf() after output
 *	has been aborted and before output has been resumed will do
 *	nothing.  debug_output_aborted() should be called, however,
 *	before looping, to keep from spending too much time computing
 *	output which is discarded.
 */
boolean_t
debug_output_aborted(void)
{
	return in_demon && _DEBUG_OUTPUT_ABORTED();
}
