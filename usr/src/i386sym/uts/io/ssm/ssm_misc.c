/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/ssm/ssm_misc.c	1.15"

/*
 * ssm_misc.c
 *	Routines for manipulating misc 
 *	pieces of the SSM.
 */

#include <util/types.h>
#include <util/debug.h>
#include <util/sysmacros.h>
#include <io/autoconf.h> 
#include <io/ssm/ssm.h> 
#include <io/ssm/ssm_misc.h> 
#include <io/ssm/ssm_cb.h> 
#include <io/cfg.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/kmem.h>
#include <svc/systm.h>
#include <util/ksynch.h>
#include <util/ipl.h>
#include <util/cmn_err.h>
#include <io/scan.h>
#include <io/slic.h>

static LKINFO_DECL(ssmlkinfo, "IO: sm_lock", 0);
static LKINFO_DECL(ssm_interrupt_lkinfo, "IO: ssm_interrupt", 0);

static void ssm_gen_cmd(volatile struct ssm_misc *, unchar, int, int);
static void ssm_chk_intr(volatile struct ssm_misc *);
void *ssm_calloc(unsigned, unsigned, unsigned);
void ssm_scan_cmd(int, paddr_t, int, int, int, int, int);
int biopscan(int, int, int, int, long *, int); 
int ssm_chain_size(int, int, int);

/*
 * void
 * ssm_send_misc_addr(const struct ssm_misc *, unchar) 
 *	Send address of ssm_misc structure.
 *
 * Calling/Exit State:
 *	addr is the address to send, dest is the SLIC destination.
 * 	Assumes that no outstanding requests are 
 *	active using the current SSM message-passing 
 *	structure.
 */
void
ssm_send_misc_addr(const struct ssm_misc *addr, unchar dest)
{
	paddr_t phys_addr;
	unchar *addr_bytes = (unchar *)&phys_addr;

	phys_addr = (paddr_t) vtop((caddr_t)addr, NULL);

	slic_mIntr(dest, SM_BIN, SM_ADDRVEC);
	slic_mIntr(dest, SM_BIN, addr_bytes[0]);	/* low byte first */
	slic_mIntr(dest, SM_BIN, addr_bytes[1]);
	slic_mIntr(dest, SM_BIN, addr_bytes[2]);
	slic_mIntr(dest, SM_BIN, addr_bytes[3]);	/* high byte last */
}

/*
 * ulong
 * ssm_get_fpst(int)
 *	Return the state of the front-panel.
 *
 * Calling/Exit State:
 * 	'context' is either SM_LOCK or SM_NOLOCK, 
 *	depending on whether this routine is called 
 *	in a context where locking of the common SSM 
 *	communication area is required.
 *
 * 	Returns a bit-vector defining the front-panel 
 *	state.
 *
 * 	Assumes this command can never fail.
 */
ulong
ssm_get_fpst(int context)
{	
	register volatile struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	pl_t pl;
	ulong val;

	if (context == SM_LOCK) {
		pl = LOCK(ssm_mc->sm_lockp, plhi);
		ssm_chk_intr(ssm_mc);
	}

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_FP, SM_GET);

	/* suck out value (while ssm_mc still locked) */
	val = ssm_mc->sm_fpst;

	if (context == SM_LOCK)
		UNLOCK(ssm_mc->sm_lockp, pl);

	return (val);
}

/*
 * void
 * ssm_set_fpst(ulong_t, int)
 *	Set the state of the front-panel.
 *
 * Calling/Exit State:
 * 	'fpst' is a bit-vector defining the new 
 *	front-panel state. Not all bits are 
 *	writable (as they correspond to physical 
 *	switches on the front panel).
 *	
 * 	'context' is either SM_LOCK or SM_NOLOCK, 
 *	depending on whether this routine is 
 *	called in a context where locking of the 
 * 	common SSM communication area is required.  
 *
 *	Assumes this command can never fail.
 */
void
ssm_set_fpst(ulong_t fpst, int context)
{	
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	pl_t pl;

	if (context == SM_LOCK) {
		pl = LOCK(ssm_mc->sm_lockp, plhi);
		ssm_chk_intr(ssm_mc);
	}

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_fpst = fpst;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_FP, SM_SET);

	if (context == SM_LOCK)
		UNLOCK(ssm_mc->sm_lockp, pl);
}

static ulong_t wdt_loc;
static ulong_t wdt_intvl;

#define WDT_INF_INTVL	0xffffffff	/* Near infinite interval */

/*
 * void
 * ssm_init_wdt(ulong_t)
 *	Set up watchdog timer.
 *
 * Calling/Exit State:
 * 	'ms' is the number of milliseconds before the 
 *	RUN light is extinguished.  The O.S. must poke 
 *	the watchdog timer at least this often to 
 *	keep the error light on.
 *
 * 	Called when locking is unnecessary.
 */
void
ssm_init_wdt(ulong_t ms)
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_wdtst.wdt_addr = (ulong) vtop((caddr_t)&wdt_loc, NULL);
	ssm_mc->sm_wdtst.wdt_intvl = wdt_intvl = ms;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_WDT, SM_SET);
}

/*
 * void
 * ssm_disable_wdt()
 *	Disable the watchdog timer.
 *
 * Calling/Exit State:
 *	The watchdog timer must have previously been
 *	started using ssm_init_wdt().  
 *
 * 	Called when locking is unnecessary, from the
 *	debugger.
 *
 * Remarks:
 *	The O.S. normally must poke the watchdog timer 
 *	periodically to keep the error light from turning 
 *	on.  This interface allows special functionality, 
 *	such as the debugger, from having to continue this
 *	by setting the value nearly infinitely large.
 *	
 */
void
ssm_disable_wdt()
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_wdtst.wdt_addr = (ulong) vtop((caddr_t)&wdt_loc, NULL);
	/*LINTED*/
	ssm_mc->sm_wdtst.wdt_intvl = WDT_INF_INTVL;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_WDT, SM_SET);
}

/*
 * void
 * ssm_reenable_wdt()
 *	Reenable the watchdog timer after disabling 
 *	with ssm_disable_wdt().
 *
 * Calling/Exit State:
 *	The watchdog timer must have previously been
 *	started using ssm_init_wdt(), then disabled
 *	with ssm_disable_wdt().  The interval for the
 *	SSM to check the WDT was saved in wdt_intlvl.
 *
 * 	Called when locking is unnecessary, from the
 *	debugger.
 *
 * Remarks:
 *	Afterwards, the O.S. must resume poking the 
 *	watchdog timer periodically to keep the error 
 *	light from turning on. 
 */
void
ssm_reenable_wdt()
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_wdtst.wdt_addr = (ulong) vtop((caddr_t)&wdt_loc, NULL);
	ssm_mc->sm_wdtst.wdt_intvl = wdt_intvl;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_WDT, SM_SET);
}

/*
 * void
 * void ssm_poke_wdt(void)
 *	Poke the watchdog timer to show SSM we're still alive.
 *
 * Calling/Exit State:
 * 	Doesn't require locking, as the SSM message 
 *	area is not used.
 */
void
ssm_poke_wdt(void)
{
	++wdt_loc;
}

/*
 * ulong ssm_get_tod(int)
 *	Retrieve the current time of day from the SSM.
 *
 * Calling/Exit State:
 * 	'context' is either SM_LOCK, or SM_NOLOCK, and 
 *	determines whether we are called from a context 
 *	where locking is necessary.
 *
 * 	Time returned is in seconds since Midnight Jan 1, 1970.
 */
ulong
ssm_get_tod(int context)
{
	register volatile struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	pl_t pl;

	if (context == SM_LOCK) {
		pl = LOCK(ssm_mc->sm_lockp, plhi);
		ssm_chk_intr(ssm_mc);
	}

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_TOD, SM_GET);

	if (context == SM_LOCK)
		UNLOCK(ssm_mc->sm_lockp, pl);
	
	return(ssm_mc->sm_todval);
}

/*
 * void ssm_set_tod(ulong_t)
 *	Set the current time of day on the SSM.
 *
 * Calling/Exit State:
 * 	't' is the current time, measured in seconds 
 *	from midnight Jan1, 1970.
 *
 * 	Assumes called when locking is required.
 */
void
ssm_set_tod(ulong_t t)
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	pl_t pl;

	pl = LOCK(ssm_mc->sm_lockp, plhi);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_todval = t;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_TOD, SM_SET);

	UNLOCK(ssm_mc->sm_lockp, pl);
}

/*
 * void ssm_tod_freq(int, int, unchar, unchar, unchar)
 *	Setup time-of-day interrupts from SSM
 *
 * Calling/Exit State:
 * 	'context' is either SM_LOCK or SM_NOLOCK,
 * 	depending on whether locking is required.
 * 	'freq' is the interval in milliseconds
 *	at which interrupts from SSM are generated.
 *	'freq' == 0 turns off the time-of-day interrupts.
 * 	'dest' is the SLIC interrupt destination.
 * 	'vec' is the SLIC interrupt vector for the 
 *	SSM to use.
 * 	'cmd' is the SLIC command for the SSM to use.
 */
void
ssm_tod_freq(int context, int freq, unchar dest, unchar vec, unchar cmd)
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	pl_t pl;

	if (context == SM_LOCK) {
		pl = LOCK(ssm_mc->sm_lockp, plhi);
		ssm_chk_intr(ssm_mc);
	}

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_todfreq.tod_freq = freq;
	ssm_mc->sm_todfreq.tod_dest = dest;
	ssm_mc->sm_todfreq.tod_vec = vec;
	ssm_mc->sm_todfreq.tod_cmd = cmd;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_TOD, SM_SET_FREQ);

	if (context == SM_LOCK)
		UNLOCK(ssm_mc->sm_lockp, pl);
}


/*
 * void
 * ssm_reboot(uint_t, ushort_t, const char *)
 *	Reboot with these flags and string.
 *
 * Calling/Exit State:
 * 	'flags' is the boot flags to reboot with.
 * 	'size' is the number of bytes in 'str'.
 * 	'str' is a character buffer with the boot string.
 *
 * 	Assumed called when no locking is required.
 */
void
ssm_reboot(uint_t flags, ushort_t size, const char *str)
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_boot.boot_flags = (unchar)flags;
	if (size) {
		bcopy(str, (char *)ssm_mc->sm_boot.boot_str,
	      		(size > BNAMESIZE)? BNAMESIZE: size);
	}
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_BOOT, SM_REBOOT);
}

/*
 * void
 * ssm_get_boot(int, int, ushort_t *, uint_t, char *)
 *	Returns boot info.
 *
 * Calling/Exit State:
 * 	'which' specifies which default boot string to get.
 *		It is one of SM_DYNIX or SM_DUMPER.
 *	'cmd' specifies whech set of boot strings to get.  It
 *		is SM_SET_DFT for the NOVRAM copy, or SM_SET for
 *		volatile copy.
 * 	'flags' is a pointer to the location that gets the current
 *		current boot flags.
 * 	'size' is the number of bytes that can be written in 'str'.
 * 	'str' is a character buffer for placing the boot string.
 *
 * 	Assumes called when locking is required.
 */
void
ssm_get_boot(int which, int cmd, ushort_t *flags, uint_t size, char *str)
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	pl_t pl;

	pl = LOCK(ssm_mc->sm_lockp, plhi);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_boot.boot_which = (unchar)which;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_BOOT, cmd);
	*flags = ssm_mc->sm_boot.boot_flags;
	bzero(str, size);		/* Clear the buffer to begin with */
	bcopy(ssm_mc->sm_boot.boot_str, str, (size > ssm_mc->sm_boot.boot_size)?
		 ssm_mc->sm_boot.boot_size : size);

	UNLOCK(ssm_mc->sm_lockp, pl);
}

/*
 * void
 * ssm_set_boot(int, int, ushort_t, uint, const char *)
 *	Set current boot info.
 *
 * Calling/Exit State:
 * 	'which' specifies which default boot string is to be
 *		set.  It is one of SM_DYNIX or SM_DUMPER.
 *	'cmd' specifies whech set of boot strings are to be
 *		set.  It is SM_SET_DFT for NOVRAM copy, or SM_SET
 *		for volatile copy.
 * 	'flags' is the new default boot flags.
 * 	'size' is the number of bytes in 'str'.
 * 	'str' is a character buffer with the boot string.
 *
 * 	Assumes called when locking is required.
 */
void
ssm_set_boot(int which, int cmd, ushort_t flags, uint size, const char *str)
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	pl_t pl;

	pl = LOCK(ssm_mc->sm_lockp, plhi);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_boot.boot_which = (unchar)which;
	ssm_mc->sm_boot.boot_flags = flags;
	bcopy(str, ssm_mc->sm_boot.boot_str,
	      (size > BNAMESIZE)? BNAMESIZE: size);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_BOOT, cmd);

	UNLOCK(ssm_mc->sm_lockp, pl);
}

/*
 * static void
 * ssm_gen_cmd(volatile struct ssm_misc *, unchar, int, int)
 *	Send a generic command to the SSM
 *
 * Calling/Exit State:
 * 	'dest' is the SLIC id of the SSM (for when 
 *	there are multiple SSM's in a system).
 *	'who' is who on the SSM gets the message.
 * 	'cmd' is the command to send.
 *
 * 	Returns the contents of 'sm_stat' after the 
 *	command completes.
 *
 * 	Assumes that the rest of the 'ssm_mc' has 
 *	been filled in, and that we have exclusive 
 *	control of it.
 */
static void
ssm_gen_cmd(volatile struct ssm_misc *ssm_mc, unchar dest, int who, int cmd)
{
	pl_t s;

	/* build command to ssm */
	ssm_mc->sm_who = (unchar)who;
	ssm_mc->sm_cmd = (unchar)cmd;
	ssm_mc->sm_stat = SM_BUSY;

	/* send it and wait for completion */
	s = splhi();
	slic_mIntr(dest, SM_BIN, (unchar)who);
	splx(s);
	while (ssm_mc->sm_stat == SM_BUSY)
		continue;
}

/*
 * SGS2 introduces SSM based scan library access.
 * Configuration and status registers are no longer accessible via
 * the slic; they must be accessed via a scan interface gained via
 * requests to the SSM.
 *
 * As with all previous SSM messages, certain access to these
 * command chains can be gotten using a "poll on completion" interface.
 * For performance software, a "interrupt on completion" interface is
 * required.  Thus, "poll on completion" routines must be aware of
 * "interrupt on completion" requests and take special precautions.
 *
 * In general, pre-SGS2 routines are only available in "poll on completion"
 * versions.  Post-SGS2 routines are available in both versions.
 *
 * The following variables allow sharing of the SSM mailbox by both the
 * poll on completion and the "interrupt on completion" routines.
 */
static int ssm_interrupt_active = 0;
static int ssm_copied = 0;
static sleep_t *ssm_interrupt;
static sv_t *ssm_done;
static caddr_t ssm_scanbuf[2];
static ulong scan_vec = (ulong)0xffffffff;	/* out of bounds value */
static unchar scan_dest = PROC_GROUP;
static unchar scan_bin = SCAN_BIN;
struct ssm_misc scan_misc_copy;

/*
 * The scan library must communicate with the console SSM long before
 * autoconfigure (when the ssm misc routines are initialized).  For this
 * reason, we keep a relatively independent ssm_misc structure for the
 * scan library and then attach this to the console SSM at autoconfig
 * time.
 */
static struct ssm_misc *scan_misc = 0;
static int scan_slicaddr = 0;
void ssm_misc_intr();

#define	round2long(x)	(((x) + sizeof(long)+1)/sizeof(long))

struct ssm_misc ssm_misc_copy;

/*
 * struct ssm_misc * 
 * ssm_misc_init(int, unchar)
 *	performs misc. SSM initializations
 *
 * Calling/Exit State:
 *	iscons == TRUE means that this is the console SSM.
 *	returns a pointer to a ssm_misc structure that has been filled out.
 *	Must be called after kvm_init.
 */
struct ssm_misc *
ssm_misc_init(int iscons, unchar slicaddr)
{
	register struct ssm_misc *mc;

	if (!iscons) {
		/* 
		 * Allocate the message passing structure 
		 * for misc. SSM commands and notify the 
		 * SSM of its location.
		 */
		mc = (struct ssm_misc *)
			ssm_alloc(sizeof(struct ssm_misc),
				  SSM_ALIGN_XFER, SSM_BAD_BOUND, KM_NOSLEEP);
		mc->sm_lockp = LOCK_ALLOC(31, plhi, &ssmlkinfo, KM_NOSLEEP);
		ssm_send_misc_addr(mc, slicaddr);
		return(mc);
	}

	if (scan_misc) {
		if (scan_vec == (ulong)0xffffffff) {
			scan_vec = ivec_alloc(scan_bin);
			ivec_init(scan_bin, scan_vec, ssm_misc_intr);
		}

		/*
		 * Console interface misc interface already allocated.
		 */
		return(scan_misc);
	}

	/*
	 * Initialize the console scan interface.  This is used by the
	 * scan library access routines early in startup (before autoconf).
	 */
	mc = (struct ssm_misc *) ssm_alloc(sizeof(struct ssm_misc),
			  SSM_ALIGN_XFER, SSM_BAD_BOUND, KM_NOSLEEP);

	mc->sm_lockp = LOCK_ALLOC(31, plhi, &ssmlkinfo, KM_NOSLEEP);
	ASSERT(mc->sm_lockp != NULL);

	ssm_send_misc_addr(mc, slicaddr);

	/*
	 * Initialize the other aspects of the scan interface.
	 */
	ssm_interrupt = SLEEP_ALLOC(0, &ssm_interrupt_lkinfo, KM_NOSLEEP);
	ASSERT(ssm_interrupt != NULL);

	ssm_done = SV_ALLOC(KM_NOSLEEP);
	ASSERT(ssm_done != NULL);

	/*
	 * Allocate 2 properly aligned buffers to send/receive the
	 * scan data.
	 */
	ssm_scanbuf[0] = (caddr_t)ssm_alloc(SCAN_MAXSIZE, SSM_ALIGN_XFER,
					    SSM_BAD_BOUND, KM_NOSLEEP);
	ssm_scanbuf[1] = (caddr_t)ssm_alloc(SCAN_MAXSIZE, SSM_ALIGN_XFER,
					    SSM_BAD_BOUND, KM_NOSLEEP);
	scan_misc = mc;
	scan_slicaddr = slicaddr;

	return(mc);
}

/*
 * void 
 * scan_init(int)
 *
 *	Initialize enough of the console handler to talk to the
 *	scan interface.
 *
 * Calling/Exit State:
 *
 *	Called from very early in initialization.  Must not sleep.
 */
void
scan_init(int slicaddr)
{
	(void)ssm_misc_init(1, (unchar)slicaddr);
}

/*
 * static void 
 * ssm_chk_intr(volatile struct ssm_misc *)
 *
 *	Check for and wait for any pending interrupts currently
 *	using this interface.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	Check to see if the interrupt interface to the system is active.
 *	If so, we must wait for the command to complete and copy the results
 *	off to the side.
 */
static void
ssm_chk_intr(volatile struct ssm_misc *ssm_mc)
{
	if (!ssm_interrupt_active)
		return;

	/*
	 * The SSM's busy with a "interrupt on completion" command.
	 * we must wait for this command to complete before issueing
	 * our own.
	 */
	while (ssm_mc->sm_stat == SM_BUSY)
		;

	/*
	 * The command's complete.  Copy the results off to the
	 * side and let the interrupt routine know that it's been
	 * copied.
	 */
	ssm_misc_copy = *ssm_mc;
	ssm_copied = 1;
	ssm_interrupt_active = 0;
	return;
}

/*
 * bicscan(sic, tap, chain, mask, ...)
 * bisscan(sic, tap, chain, mask, ...)
 * bitscan(sic, tap, chain, mask, ...)
 *
 * BIt Clear SCAN chain, BIt Set SCAN chain and BIt Test SCAN chain.
 * All operate on the command chain <sic, tap, chain>.
 * Bicscan clears the bits of the chain specified by "mask".
 * Bisscan sets the bits of the chain specified by "mask".
 * Bitscan tests the bits of the chain specified by "mask", returning
 * non-zero if any of the specified bits are set.
 * All three operations are polled only.
 */
#define	SCAN_BIC	0
#define	SCAN_BIS	1
#define	SCAN_BIT	2
#define	SCAN_READ	3
#define	SCAN_WRITE	4
#define	SCAN_FUNC	5

/*
 * int 
 * scan_func(int sic, int cmd, int intr)
 *
 *	Perform a scan function.
 *
 * Calling/Exit State:
 *
 *	Should be called without either of ssm_interrupt or ssm_done held.
 *
 * Description:
 *
 *	Send the command in question down to the SSM.
 */
int
scan_func(int sic, int cmd, int intr)
{
	return(biopscan(SCAN_FUNC, sic, cmd, 0, (long *)0, intr));
}

#define	SICEXT_INTERCEPT
#ifdef	SICEXT_INTERCEPT
static void ssm_sic_intercept(int, caddr_t, int, int);
#endif

/*
 * int 
 * biopscan(int op, int sic, int tap, int chain)
 *
 *	Perform scan operation.
 *
 * Calling/Exit State:
 *
 *	As above.
 *
 * Description:
 *
 *	Perform one of the above operations, doing the necessary
 *	interlocking depending on interrupt or polled operations.
 */
int
biopscan(op, sic, tap, chain, mask, intr) 
	int op, sic, tap, chain; 
	long *mask;
{
	register volatile struct ssm_misc *ssm_mc = scan_misc;
	register long *src, *dst;
	long result = 0;
	long *buf;
	paddr_t pbuf;
	int size = ssm_chain_size(sic, tap, chain);
	pl_t pl;

	if (!ssm_mc) {
		/*
		 *+ The kernel attempted to perform a scan operation
		 *+ before the necessary interface was initialized.
		 */
		cmn_err(CE_PANIC, "biopscan before init.\n");
		/* NOT REACHED */
	}

	if (intr) {
		/*LINTED*/
		buf = (long *)ssm_scanbuf[1];
		SLEEP_LOCK(ssm_interrupt, PRIZERO);
	} else {
		/*LINTED*/
		buf = (long *)ssm_scanbuf[0];
	}
	pbuf = vtop((caddr_t)buf, NULL);

	pl = LOCK(ssm_mc->sm_lockp, plhi);

	if (!intr)
		ssm_chk_intr(ssm_mc);

	bzero((caddr_t)buf, SCAN_MAXSIZE);

	if (op != SCAN_WRITE) {
#ifdef	SICEXT_INTERCEPT
		if (tap == SIC_TAP && (chain == SICEXT0 || chain == SICEXT1)) {
			ssm_sic_intercept(sic, (caddr_t)buf, chain, 0);
			goto read_next;
		}
#endif	/* SICEXT_INTERCEPT */
		/*
		 * Read the scan chain.
		 */
		ssm_scan_cmd(SM_GET, pbuf, size, sic, tap, chain, intr);

		if (intr) {
			while (ssm_interrupt_active != 0) {
				SV_WAIT(ssm_done, PRIZERO, ssm_mc->sm_lockp);
				(void)LOCK(ssm_mc->sm_lockp, plhi);
			}
		} else {
			while (ssm_mc->sm_stat == SM_BUSY)
				;
		}
#ifdef	SICEXT_INTERCEPT
		read_next:;
#endif	/* SICEXT_INTERCEPT */
	}

	switch (op) {
	case SCAN_BIC:
		/*
		 * Bic in the data.
		 */
		size = round2long(size);
		src = mask;
		dst = buf;
		while (size-- > 0)
			*dst++ &= ~*src++;
		break;

	case SCAN_BIS:
		/*
		 * Bis in the data.
		 */
		size = round2long(size);
		src = mask;
		dst = buf;
		while (size-- > 0)
			*dst++ |= *src++;
		break;

	case SCAN_BIT:
		/*
		 * Test the data.
		 */
		size = round2long(size);
		src = mask;
		dst = buf;
		result = 0;
		while (size-- > 0)
			result |= *src++ & *dst++;
		goto out;

	case SCAN_READ:
		/*
		 * Read the data.
		 */
		size = round2long(size);
		bcopy((caddr_t)buf, (caddr_t)mask, (unsigned)size);
		goto out;

	case SCAN_WRITE:
		/*
		 * Write the data.
		 */
		size = round2long(size);
		bcopy((caddr_t)mask, (caddr_t)buf, (unsigned)size);
		break;

	case SCAN_FUNC:
		/*
		 * New functional interface.  Eventually, this will
		 * be the only interface and all the above will be
		 * rewritten.
		 * We already sent the command down above, we just
		 * need to fetch the return code (if any) and return.
		 */
		result = buf[0];
		goto out;
	}

#ifdef	SICEXT_INTERCEPT
	if (tap == SIC_TAP && (chain == SICEXT0 || chain == SICEXT1)) {
		ssm_sic_intercept(sic, (caddr_t)buf, chain, 1);
		goto out;
	}
#endif	/* SICEXT_INTERCEPT */
	/*
	 * Write the data back.
	 */
	ssm_scan_cmd(SM_SET, pbuf, size, sic, tap, chain, intr);

	if (intr) {
		while (ssm_interrupt_active != 0) {
			SV_WAIT(ssm_done, PRIZERO, ssm_mc->sm_lockp);
			(void)LOCK(ssm_mc->sm_lockp, plhi);
		}
	} else {
		while (ssm_mc->sm_stat == SM_BUSY)
			;
	}
out:
	UNLOCK(ssm_mc->sm_lockp, pl);
	if (intr)
		SLEEP_UNLOCK(ssm_interrupt);

	return(result);
}

/*
 * int
 * scanfuncmsg(int, int, int, int)
 *
 *	Perform a SCAN function with associated data.
 *
 * Calling/Exit State:
 *
 * The indicated amount of data is provided to the SSM under the specified
 * function code.  An equivalent amount of data is copied back into the
 * buffer when the SSM indicates completion.
 *
 * Returns 0 on success, 1 on failure.
 */
int
scanfuncmsg(sic, op, data, length)
	int sic, op;
	caddr_t data;
	unsigned length;
{
	register volatile struct ssm_misc *ssm_mc = scan_misc;
	caddr_t buf;
	paddr_t pbuf;
	pl_t pl;
	int retval;

	ASSERT(length < SCAN_MAXSIZE);
	ASSERT(ssm_mc != NULL);

	buf = ssm_scanbuf[0];
	pbuf = vtop((caddr_t)buf, NULL);

	pl = LOCK(ssm_mc->sm_lockp, plhi);

	ssm_chk_intr(ssm_mc);

	/*
	 * Zero-fill the buffer, copy our data into its head
	 */
	bzero(buf, SCAN_MAXSIZE);
	bcopy(data, buf, length);

	/*
	 * Send the request down
	 */
	ssm_scan_cmd(SM_GET, pbuf, (int)length, sic, op, 0, 0);

	/*
	 * Wait for it to be processed
	 */
	while (ssm_mc->sm_stat == SM_BUSY)
		;

	/*
	 * Copy back the resulting data
	 */
	retval = ssm_mc->sm_stat;
	bcopy((caddr_t)buf, data, length);
	UNLOCK(ssm_mc->sm_lockp, pl);

	return((retval == SM_OK) ? 0 : 1);
}

#ifdef	SICEXT_INTERCEPT
/*
 * Defines the Register Numbers on the Sic when addressed over
 * the SLIC bus.
 */
#define SLIC_SIC_BASE   32
#define	SL_SIC_CHIP_ID 		(SLIC_SIC_BASE + 0)
#define	SL_SIC_VERSION 		(SLIC_SIC_BASE + 1)
#define	SL_SIC_EXT0 		(SLIC_SIC_BASE + 2)
#define	SL_SIC_EXT1 		(SLIC_SIC_BASE + 3)
#define	SL_SIC_STATUS 		(SLIC_SIC_BASE + 4)
#define	SL_SIC_CONFIG 		(SLIC_SIC_BASE + 5)
#define	SL_SIC_INT_EN 		(SLIC_SIC_BASE + 6)
#define	SL_SIC_INT 		(SLIC_SIC_BASE + 7)
#define	SL_SIC_XMIT0 		(SLIC_SIC_BASE + 8)
#define SL_SIC_INIT 		(SLIC_SIC_BASE + 8)
#define	SL_SIC_XMIT0_0		(SLIC_SIC_BASE + 8)
#define	SL_SIC_DATA 		(SLIC_SIC_BASE + 9)
#define	SL_SIC_XMIT0_1		(SLIC_SIC_BASE + 9)
#define	SL_SIC_XMIT0_2		(SLIC_SIC_BASE + 10)
#define	SL_SIC_INSTR_RD 	(SLIC_SIC_BASE + 10)
#define	SL_SIC_XMIT0_3 		(SLIC_SIC_BASE + 11)
#define	SL_SIC_XMIT0_4 		(SLIC_SIC_BASE + 12)
#define	SL_SIC_ADDR 		(SLIC_SIC_BASE + 12)
#define	SL_SIC_XMIT0_5 		(SLIC_SIC_BASE + 13)
#define	SL_SIC_DUMMY 		(SLIC_SIC_BASE + 13)
#define	SL_SIC_INSTR_WR 	(SLIC_SIC_BASE + 14)
#define	SL_SIC_XMIT0_6 		(SLIC_SIC_BASE + 14)
#define	SL_SIC_XMIT0_7 		(SLIC_SIC_BASE + 15)
#define	SL_SIC_XMIT1 		(SLIC_SIC_BASE + 16)
#define	SL_SIC_TYPE 		(SLIC_SIC_BASE + 17)
#define	SL_SIC_SYNC_DEPTH 	(SLIC_SIC_BASE + 18)
#define SL_SIC_RING_CONFIG 	(SLIC_SIC_BASE + 19)
#define	SL_SIC_RING_SELECT 	(SLIC_SIC_BASE + 20)
#define	SL_SIC_TMS 		(SLIC_SIC_BASE + 21)
#define SL_SIC_PRE_CNT		(SLIC_SIC_BASE + 22)
#define SL_SIC_POST_CNT 	(SLIC_SIC_BASE + 23)
#define	SL_SIC_LENGTH0 		(SLIC_SIC_BASE + 24)
#define	SL_SIC_LENGTH1 		(SLIC_SIC_BASE + 25)
#define	SL_SIC_START_CFG 	(SLIC_SIC_BASE + 26)
#define	SL_SIC_RUN_CFG 		(SLIC_SIC_BASE + 27)
#define	SL_SIC_COUNT0 		(SLIC_SIC_BASE + 28)
#define	SL_SIC_COUNT1 		(SLIC_SIC_BASE + 29)
#define	SL_SIC_START 		(SLIC_SIC_BASE + 30)
#define	SL_SIC_JTAG_STAT 	(SLIC_SIC_BASE + 31)
#define SL_SIC_SICADDR 		(SLIC_SIC_BASE + 31)

/*
 * static void 
 * ssm_sic_intercept(int sic, int buf, int chain, int wr)
 *
 *	Intercept a scan-library command and perform the operation
 *	directly against the sic.
 *
 * Calling/Exit State:
 *
 *	Must be called holding the ssm_misc mailbox lock.
 *
 * Description:
 *
 */
static void
ssm_sic_intercept(int sic, caddr_t buf, int chain, int wr)
{
	register int reg;

	if (chain == SICEXT0) {
		/*
		 * Must write the outputs down.
		 */
		buf[0] |= SICEXT0_INPUTS;
		reg = SL_SIC_EXT0;
	} else {
		buf[0] |= SICEXT1_INPUTS;
		reg = SL_SIC_EXT1;
	}

	if (wr)
		slic_wrslave((unchar)sic, (unchar)reg, (unchar)buf[0]);
	else
		buf[0] = slic_rdslave((unchar)sic, (unchar)reg);
}
#endif	/* SICEXT_INTERCEPT */

/*
 * void 
 * ssm_scan_cmd(int cmd, caddr_t buf, int size, int sic, int tap,
 *		    int chain, int intr)
 *
 *	Perform a scan command.
 *
 * Calling/Exit State:
 *
 *	Must be called holding the appropriate lock for the misc interface.
 *
 * Description:
 *
 *	Initiate a request to the scan library without waiting for
 *	it to finish.
 */
void
ssm_scan_cmd(int cmd, paddr_t buf, int size, int sic, int tap,
	     int chain, int intr)
{
	volatile struct ssm_misc *ssm_mc = scan_misc;
	lock_t *lockp = ssm_mc->sm_lockp;

	bzero((caddr_t)ssm_mc, sizeof(*ssm_mc));
	ssm_mc->sm_lockp = lockp;
	ssm_mc->sm_who = SM_SCANLIB;
	ssm_mc->sm_stat = SM_BUSY;
	ssm_mc->sm_scanst.buf = (caddr_t)buf;
	ssm_mc->sm_scanst.size = (unchar)size;
	ssm_mc->sm_scanst.sic = (unchar)sic;
	ssm_mc->sm_scanst.tap = (unchar)tap;
	ssm_mc->sm_scanst.chain = (unchar)chain;
	if (intr) {
		ssm_interrupt_active = 1;
		ssm_mc->sm_dest = scan_dest | SL_GROUP;
		ssm_mc->sm_bin = scan_bin | SL_MINTR;
		ssm_mc->sm_mesg = (unchar)scan_vec;
		cmd |= SM_INTR;
	}
	ssm_mc->sm_cmd = (unchar)cmd;

	slic_mIntr((unsigned char)scan_slicaddr, SM_BIN, (unchar)SM_SCANLIB);
}

/*
 * void 
 * ssm_misc_intr(void)
 *
 *	Process an ssm-misc interrupt.
 *
 * Calling/Exit State:
 *
 * Desctiption:
 *
 *	Process a message passing interrupt from the ssm.  After we get
 *	the ssm_mc lock, this implies that there's a process waiting on
 *	"ssm_done" for completion.  We should copy the results of the
 *	operation off to the side (assuming a poller didn't do so) and
 *	wake the process up off of ssm_done.
 */
void
ssm_misc_intr(void)
{
	struct ssm_misc *ssm_mc = scan_misc;
	pl_t pl;

	if (!ssm_mc) {
		/*
		 *+ an ssm misc interuppt occured prior to the
		 *+ initialization of the board
		 */
		cmn_err(CE_WARN, "ssm_misc_intr before init.\n");
		return;
	}

	pl = LOCK(ssm_mc->sm_lockp, plhi);

	if (!ssm_copied)
		scan_misc_copy = *ssm_mc;

	ssm_copied = 0;
	ssm_interrupt_active = 0;

	SV_SIGNAL(ssm_done, 0);
	UNLOCK(ssm_mc->sm_lockp, pl);
}

/*
 * int 
 * ssm_chain_size(int sic, int tap, int chain)
 *
 *	Return the size of a scan chain.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Descripton:
 *
 *	Determine what the size of a scan chain is (rounded up to the
 *	nearest byte).
 */
int
ssm_chain_size(sic, tap, chain)
	int sic, tap, chain;
{
	register int size;
#define	f(tap, chain)	((tap)<<16 | (chain))
#ifdef lint
	size = sic;
#endif

	/*
	 * Should probably do this with a table lookup.
	 * Do a switch statement for now.
	 */
	switch f(tap, chain) {
	case f(SIC_TAP, SICEXT0):
		size = SICEXT0_SIZE;
		break;
	case f(SIC_TAP, SICEXT1):
		size = SICEXT1_SIZE;
		break;
	case f(BICD_TAP, BCDSES):
		size = BCDSES_SIZE;
		break;
	case f(BICD_TAP, BCDTACH):
		size = BCDTACH_SIZE;
		break;
	case f(BICD_TAP, BCDTC):
		size = BCDTC_SIZE;
		break;
	case f(CIC0_TAP, CICTACH):
		size = CICTACH_SIZE;
		break;
	case f(CIC0_TAP, CICTCM):
		size = CICTCM_SIZE;
		break;
	default:
		return(16);
/*		cmn_err(CE_PANIC, "scan_chain_size: unknown chain"); */
		/* not reached */
	}

	size = (size+7) >> 3;
	return(size);
}

/*
 * void *
 * ssm_calloc(unsigned, unsigned, unsigned)
 *	Allocate a properly-aligned chunk of memory using calloc().
 *
 * Calling/Exit State:
 * 	'nbytes' is the number of bytes to allocate.
 * 	'align' is the byte multiple at which the memory 
 *	is to be aligned (e.g. 2 means align to two-byte 
 *	boundary).  
 *	'badbound' is a boundary which cannot be crossed 
 *	(usually one megabyte for the SSM); it must be a 
 *	power of two and a multiple of 'align'.
 */
void *
ssm_calloc(unsigned nbytes, unsigned align, unsigned badbound)
{
	ulong addr;

	callocrnd((int)align);
	addr = (long)calloc(0);
	if ((addr & ~(badbound - 1)) != ((addr + nbytes) & ~(badbound - 1))) {
		/*
		 * It would have crossed a 'badbound' boundary,
		 * so bump past this boundary.
		 */
		callocrnd((int)badbound);
	}
	return ((void *)calloc((int)nbytes));
}

/*
 * void *
 * ssm_alloc(unsigned, unsigned, unsigned, int)
 *	Allocate and zero properly-aligned chunk of physically
 *	contiguous memory for communicating with the SSM.
 *
 * Calling/Exit State:
 * 	'nbytes' is the number of bytes to allocate.
 * 	'align' is the byte multiple at which the memory 
 *	is to be aligned (e.g. 2 means align to two-byte 
 *	boundary).  
 *	'badbound' is a boundary which cannot be crossed 
 *	(usually one megabyte for the SSM); it must be a 
 *	power of two and a multiple of 'align'.
 *	'flag' is just passed straight through to kmem operations.
 *
 *	Returns the address of the usable memory upon success.
 *	Otherwise it returns NULL.
 *
 * Remarks: 
 *	Simply translate this into a call to kmem_alloc_physcontig()
 *	and zero the allocated memory if it succeeds.
 */
void *
ssm_alloc(unsigned nbytes, unsigned align, unsigned badbound, int flag)
{
	void *addr;
	physreq_t *preqp;

	preqp = physreq_alloc(flag);
	if (preqp == NULL)
		return (NULL);
	preqp->phys_align = align;
	preqp->phys_boundary = badbound;
	if (!physreq_prep(preqp, flag)) {
		physreq_free(preqp);
		return (NULL);
	}
	addr = kmem_alloc_physcontig(nbytes, preqp, flag);
	physreq_free(preqp);
	if (addr != NULL) 
		bzero(addr, nbytes);
	return (addr);
}

/*
 * void
 * ssm_free(void *, unsigned)
 *	Free memory allocated via ssm_alloc().
 *
 * Calling/Exit State:
 *	'addr' is the address previously returned by ssm_alloc().
 * 	'nbytes' is the number of bytes previously allocated.
 *	No return value.
 *
 * Remarks: 
 *	Simply translate this into a call to kmem_free_physcontig().
 */
void
ssm_free(void *addr, unsigned nbytes)
{
	kmem_free_physcontig(addr, nbytes);
}

/* 
 * struct cons_cb *
 * init_ssm_cons(unchar)
 *	allocate SSM console CBs.     
 *
 * Calling/Exit State:
 *	Assumes that slic_mIntr() retries messages until they
 *	succeed.
 *
 * Description:
 *	Allocate properly aligned console CBs and notify 
 *	the SSM by sending the address a byte at a time
 *	to it over the slic.  Return the base CB address
 *	to the caller.
 */
struct cons_cb *
init_ssm_cons(unchar slic)
{
	struct cons_cb *cons_cbs;
	paddr_t phys_cons_cbs;
	unchar *addr_bytes = (unchar *)&phys_cons_cbs;
	uint nbytes = sizeof(struct cons_cb) * NCONSDEV << NCBCONSHFT;

	cons_cbs = (struct cons_cb *)
		ssm_calloc(nbytes, SSM_ALIGN_BASE, SSM_BAD_BOUND);

	phys_cons_cbs = (paddr_t) vtop((caddr_t)cons_cbs, NULL);

 	/* Notify the SSM of the CB's location. */
	slic_mIntr(slic, CONS_BIN, CONS_ADDRVEC);
	slic_mIntr(slic, CONS_BIN, addr_bytes[0]);	/* low byte first */
	slic_mIntr(slic, CONS_BIN, addr_bytes[1]);
	slic_mIntr(slic, CONS_BIN, addr_bytes[2]);
	slic_mIntr(slic, CONS_BIN, addr_bytes[3]);	/* high byte last */

	return (cons_cbs);
}

/* 
 * struct print_cb *
 * init_ssm_prnt(unchar)
 *	allocate printer CBs     
 *
 * Calling/Exit State:
 *	Assumes that slic_mIntr() retries messages until they
 *	succeed.
 *
 * Description:
 *	Allocate properly aligned printer CBs and notify 
 *	the SSM by sending the address a byte at a time
 *	to it over the slic.  Return the base CB address
 *	to the caller.
 */
struct print_cb *
init_ssm_prnt(unchar slic)
{
	struct print_cb *cbs;
	paddr_t phys_cbs;
	unchar *addr_bytes = (unchar *)&phys_cbs;
	uint nbytes = sizeof(struct print_cb) * NPRINTDEV * NCBPERPRINT;
	pl_t s;

	cbs = (struct print_cb *)
		ssm_alloc(nbytes, SSM_ALIGN_BASE, SSM_BAD_BOUND, KM_NOSLEEP);	

	phys_cbs = vtop((caddr_t)cbs, NULL);

 	/* Notify the SSM of the CB's location. */
	s = splhi();
	slic_mIntr(slic, PRINT_BIN, PCB_ADDRVEC);
	slic_mIntr(slic, PRINT_BIN, addr_bytes[0]);	/* low byte first */
	slic_mIntr(slic, PRINT_BIN, addr_bytes[1]);
	slic_mIntr(slic, PRINT_BIN, addr_bytes[2]);
	slic_mIntr(slic, PRINT_BIN, addr_bytes[3]);	/* high byte last */
	splx(s);

	return (cbs);
}

/*
 * void
 * ssm_set_vme_mem_window(struct ssm_misc *, unchar, ulong)
 *
 *	Set the specied SSM's PIC responder address for its VME window.
 *
 * Calling/Exit State:
 *
 *	The window argument must be greater than zero.
 *
 * 	The caller must own the misc. CB for the 
 *	specified SSM.  Generally called only during
 *	bootup, when no locking is needed.
 *
 *	Sets the PIC's notion of its Sequent bus responder address.
 *
 *	No return value.
 *
 * Description:
 *      'ssm_mc' is the address of the misc cb.
 * 	'ssm_slic' is the SLIC id of the target SSM.
 *	'window' is the VME window to assign to this SSM, its
 *		physical address must be configured as follows:
 *
 *      window  0 = IO BASE + 0
 *              1 = IO BASE + 32mb
 *		2 = IO BASE + 64mb
 *              etc
 *
 *      The fw expects the value to be specified as:
 *
 *      window  0 = IO BASE + 32Mb
 *              1 = IO BASE + 64Mb
 *		2 = IO BASE + 96mb
 *              etc
 *
 *	Since the window index is off by by 1, this routine
 *	adjusts it prior to communicating it to the SSM.
 *	Note that the Multibus occupies part of the first 
 *	32Mb of I/O space.
 */
void
ssm_set_vme_mem_window(struct ssm_misc *ssm_mc, 
			unchar ssm_slic, ulong window)
{
	/* LINTED */
        ASSERT( window > 0);
        bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
        ssm_mc->sm_vme_imap.vme_window = window - 1;
        ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_SET_WINDOW );
}

/*
 * void
 * ssm_clr_vme_imap(struct ssm_misc *ssm_mc, unchar ssm_slic)
 *	
 *	Clear all the specified SSM's VME-to-SLIC interrupt mappings.
 *
 * Calling/Exit State:
 *
 * 	The caller must own the misc. CB for the specified SSM.  
 *
 *	No return value.
 *
 * Remarks:
 *
 *      'ssm_mc' is the address of the misc cb.
 * 	'ssm_slic' is the SLIC id of the target SSM.
 */
void
ssm_clr_vme_imap(struct ssm_misc *ssm_mc, unchar ssm_slic)
{
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_CLR_MAP);
}

/*
 * void
 * ssm_set_vme_imap(struct ssm_misc *, unchar, unchar, 
 *		    unchar, unchar, unchar, unchar)
 *
 *	Add an entry to SSM's VME-to-SLIC interrupt mapping.
 *
 * Calling/Exit State:
 *
 * 	The caller must own the misc. CB for the specified SSM.
 *
 *	No return value.
 *
 * Remarks:
 *
 *      'ssm_mc' is the address of the misc cb.
 * 	'ssm_slic' is the SLIC id of the target SSM.
 * 	'vlev' is the VMEbus interrupt level.
 * 	'vvec' is the VMEbus interrupt vector.
 * 	'dest' is the SLIC destination.
 * 	'svec' is the SLIC interrupt vector.
 * 	'cmd' is the SLIC command to issue (e.g. SL_MINTR | 7)
 */
void
ssm_set_vme_imap(struct ssm_misc *ssm_mc, unchar ssm_slic, 
		unchar vlev, unchar vvec, unchar dest, unchar svec, unchar cmd)
{
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_vme_imap.vi_vlev = vlev;
	ssm_mc->sm_vme_imap.vi_vvec = vvec;
	ssm_mc->sm_vme_imap.vi_dest = dest;
	ssm_mc->sm_vme_imap.vi_svec = svec;
	ssm_mc->sm_vme_imap.vi_cmd = cmd;
	ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_SET);
}

/*
 * void
 * ssm_vme_imap_ready(struct ssm_misc *ssm_mc, unchar ssm_slic)
 *
 *	Notify the SSM that the interrupt mapping is
 *	complete and all SSM VME devices have been
 *	booted.  
 *
 * Calling/Exit State:
 *
 * 	The specified SSM's misc. CB must not be locked
 *	by the caller upon entry or exit.
 *
 *	No return value.
 *
 * Description:
 *
 *	Lock the SSM's misc. CB, clear it, then call
 *	ssm_gen_cmd() to transfer a message to the
 *	SSM notifying it that all devices are initialized
 *	and that it may now begin monitoring the VMEbus
 *	for BUSERR and SYSFAIL problems.
 *
 * Remarks:
 *
 *      'ssm_mc' is the address of the misc cb.
 * 	'ssm_slic' is the SLIC id of the target SSM.
 *
 *	The SSM needs to know when mapping is complete to 
 *	correctly control SYSFAIL and BUSERR.
 */
void
ssm_vme_imap_ready(struct ssm_misc *ssm_mc, unchar ssm_slic)
{
	pl_t pl = LOCK(ssm_mc->sm_lockp, plhi);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_MAP_RDY);
	UNLOCK(ssm_mc->sm_lockp, pl);
}
