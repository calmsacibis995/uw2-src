/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/ddi.c	1.42"
#ident	"$Header: $"

/*
 *            Device Driver Interface functions          
 *
 * This file contains functions that implement the DDI/DKI standard
 * device driver interfaces.  This is not a complete set; some functions
 * are implemented in their respective subsystems.  This file contains
 * only current interfaces; compatibility support is implemented in dcompat.c.
 *
 * Note that some of the interfaces are used as macros in the base kernel;
 * for these, this file provides wrapper functions, to hide the implementation
 * from drivers, allowing binary compatibility.
 */

#define	_DDI_C

#include <acc/priv/privilege.h>
#include <io/poll.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <mem/physmap.h>
#include <mem/seg_kmem.h>
#include <mem/vmparam.h>
#include <net/inet/if.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/ghier.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <net/inet/if.h>

#include <io/ddi.h>

/*
 * ifstats -- list of NIC statistics structures
 * Ensure that drivers can call ifstats_attach()/ifstats_detach()
 * with locks held and not encounter hierarchy violations.
 */
#define IFSTATS_RWLCK_HIER	KERNEL_HIER_BASE
rwlock_t	ifstats_rwlck_shadow;
rwlock_t	*ifstats_rwlck;
LKINFO_DECL(ifstats_lkinfo, "ifstats_rwlck", 0);

struct ifstats *ifstats;

void ddisleep_init(void);
void cpubind_init(void);
void met_ds_init(void);

extern void ddi_init_f(void);

#define POLLHIER (LWP_HIER - 1)
LKINFO_DECL(_pollinfo, "POLL::ph_mutex/pollcompat", 0);
extern fspin_t	pollgen_fspin;
extern lock_t	pollcompat;
extern unsigned long	pollgen;
extern int strmsgsz;

/*
 * void
 * ddi_init(void)
 *	init ddi/dki.   sleep/wakeup, UP bind, and disk metrics code are in
 *	separate files; do their init from here.
 *
 * Calling/Exit State:
 *	none
 */
void
ddi_init(void)
{
	ddisleep_init();
	cpubind_init();
	met_ds_init();
	LOCK_INIT(&pollcompat, POLLHIER, PLHI, &_pollinfo, KM_NOSLEEP);
	FSPIN_INIT(&pollgen_fspin);
	ddi_init_f();
	RW_INIT(&ifstats_rwlck_shadow, IFSTATS_RWLCK_HIER, PLSTR,
		&ifstats_lkinfo, KM_NOSLEEP);
	ifstats_rwlck = &ifstats_rwlck_shadow;
}

/*
 * paddr_t
 * pptophys(const page_t *pp)
 *
 *	Translate a page_t to the physical address of the corresponding page.
 *
 * Calling/Exit State:
 *
 *	Returns the physical address of the page corresponding to "pp".
 */

paddr_t
pptophys(const page_t *pp)
{
	return page_pptophys(pp);
}


/*
 * struct pollhead *
 * phalloc(int flag)
 *	Allocate a pollhead structure.
 *
 * Calling/Exit State:
 *	No locking assumptions.  Returns a pointer to a pollhead structure
 *	if the allocations succeed; else returns NULL.
 */
	
struct pollhead *
phalloc(int flag)
{
	struct pollhead *php;

	php = (struct pollhead *) kmem_zalloc(sizeof(struct pollhead), flag);
	if (!php)
		return NULL;
	php->ph_type = PHALLOC;
	php->ph_mutex = LOCK_ALLOC(POLLHIER, plhi, &_pollinfo, flag);
	if (!php->ph_mutex) {
		kmem_free(php, sizeof(struct pollhead));
		return NULL;
	}
	return php;
}

/*
 * void
 * phfree(struct pollhead *php)
 *	Free a previously allocated pollhead structure.
 *
 * Calling/Exit State:
 *	No locking assumptions.  Frees memory allocated by phalloc.
 */

void
phfree(struct pollhead *php)
{
	ASSERT(php->ph_type == PHALLOC);
	LOCK_DEALLOC(php->ph_mutex);
	kmem_free(php, sizeof(struct pollhead));
}

/*
 * void
 * pollwakeup(struct pollhead *php, short event)
 *	This function notifies the system of an event so processes
 *	sleeping in poll() can be awakened.
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

void
pollwakeup(struct pollhead *php, short event)
{
	struct polldat *pdp;
	lock_t *lockp;
	pl_t pl;

	if (php->ph_type == PHCOMPAT)
		lockp = &pollcompat;
	else
		lockp = php->ph_mutex;
	pl = LOCK(lockp, PLHI);
	if ((event == POLLHUP) || (event == POLLERR)) {
		for (pdp = php->ph_list; pdp; pdp = pdp->pd_next)
			(void) (*pdp->pd_fn)(pdp->pd_arg);
	} else {
		for (pdp = php->ph_list; pdp; pdp = pdp->pd_next) {
			if (pdp->pd_events & event)
				(void) (*pdp->pd_fn)(pdp->pd_arg);
		}
	}
	FSPIN_LOCK(&pollgen_fspin);
	php->ph_gen = pollgen++;
	FSPIN_UNLOCK(&pollgen_fspin);
	UNLOCK(lockp, pl);
}


/*
 * int
 * drv_getparm(ulong_t parm, void *value_p)
 *	retrieve kernel state information
 *
 * Calling/Exit State:
 *	Store value of kernel parameter associated with parm in
 *	(*value_p) and return 0 if parm is a supported parameter;
 *	return -1 otherwise.
 *
 * NOTE: PPGRP, PPID, and PSID are not current; they are provided only
 *	 for backwards compatibility.
 */

int
drv_getparm(ulong_t parm, void *valuep)
{
	pl_t pl;

	switch (parm) {
	case UPROCP:
		*(proc_t **)valuep = u.u_procp;
		break;
	case UCRED:
		*(cred_t **)valuep = CRED();
		break;
	case LBOLT:
		*(clock_t *)valuep = lbolt;
		break;
	case TIME:
		*(time_t *)valuep = hrestime.tv_sec;
		break;
	case PPGRP:
		pl = LOCK(&u.u_procp->p_mutex, PLHI);
		*(pid_t *)valuep = u.u_procp->p_pgid;
		UNLOCK(&u.u_procp->p_mutex, pl);
		break;
	case PPID:
		*(pid_t *)valuep = u.u_procp->p_pidp->pid_id;
		break;
	case PSID:
		*(pid_t *)valuep = u.u_procp->p_sid;
		break;
	case DRV_MAXBIOSIZE:
		*(size_t *)valuep = MAXBIOSIZE;
		break;
	case STRMSGSIZE:
		*(int *)valuep = strmsgsz;
		break;
	default:
		return -1;
	}

	return 0;
}

/*
 * int
 * drv_setparm(ulong_t parm, ulong_t value)
 *	set kernel state information
 * 
 * Calling/Exit State:
 *	set value of kernel parameter associated with parm to
 *	value and return 0 if parm is SYSRINT, SYSXINT,
 *	SYSMINT, SYSRAWC, SYSCANC, SYSOUTC; return -1 otherwise;
 */
int 
drv_setparm(ulong_t parm, ulong_t value)
{

	switch (parm) {
	case SYSRINT:
		MET_RCVINT(value);
		break;
	case SYSXINT:
		MET_XMTINT(value);
		break;
	case SYSMINT:
		MET_MDMINT(value);
		break;
	case SYSRAWC:
		MET_RAWCH(value);
		break;
	case SYSCANC:
		MET_CANCH(value);
		break;
	case SYSOUTC:
		MET_OUTCH(value);
		break;
	default:
		return -1;
	}
	return 0;
}


/*
 * clock_t
 * drv_hztousec(clock_t ticks)
 *	convert clock ticks to microseconds
 *
 * Calling/Exit State:
 *           convert from system time units (given by parameter HZ)
 *           to microseconds. This code makes no assumptions about the
 *           relative values of HZ and ticks and is intended to be
 *           portable. 
 *
 *           A zero or lower input returns 0, otherwise we use the formula
 *           microseconds = (hz/HZ) * 1,000,000. To minimize overflow
 *           We divide first and then multiply. Note that we want
 *           upward rounding, so if there is any fractional part,
 *           we increment the return value by one. If an overflow is
 *           detected (i.e.  resulting value exceeds the
 *           maximum possible clock_t, then truncate
 *           the return value to CLOCK_MAX.
 *
 *           No error value is returned.
 *
 *           This function's intended use is to remove driver object
 *           file dependencies on the kernel parameter HZ.
 *           many drivers may include special diagnostics for
 *           measuring device performance, etc., in their ioctl()
 *           interface or in real-time operation. This function
 *           can express time deltas (i.e. lbolt - savelbolt)
 *           in microsecond units.
 *
 */
clock_t
drv_hztousec(clock_t ticks)
{
	clock_t quo, rem;
	clock_t remusec, quousec;

	if (ticks <= 0)
		return 0;

	quo = ticks / HZ; /* number of seconds */
	rem = ticks % HZ; /* fraction of a second */
	quousec = 1000000 * quo; /* quo in microseconds */
	remusec = 1000000 * rem; /* remainder in millionths of HZ units */

	/* check for overflow */
	if (quo != quousec / 1000000)
		return CLOCK_MAX;
	if (rem != remusec / 1000000)
		remusec = CLOCK_MAX;
	
	/* adjust remusec since it was in millionths of HZ units */
	remusec = (remusec % HZ) ? remusec / HZ + 1 : remusec / HZ;

	/* check for overflow again. If sum of quousec and remusec
	 * would exceed CLOCK_MAX then return CLOCK_MAX 
	 */

	if ((CLOCK_MAX - quousec) < remusec)
		return CLOCK_MAX;

	return quousec + remusec;
}


/*
 * clock_t
 * drv_usectohz(clock_t microsecs)
 *	convert microseconds to clock ticks
 *
 * Calling/Exit State:
 *	     convert from microsecond time units to system time units
 *           (given by parameter HZ) This code also makes no assumptions
 *           about the relative values of HZ and ticks and is intended to
 *           be portable. 
 *
 *           A zero or lower input returns 0, otherwise we use the formula
 *           hz = (microsec/1,000,000) * HZ. Note that we want
 *           upward rounding, so if there is any fractional part,
 *           we increment by one. If an overflow is detected, then
 *           the maximum clock_t value is returned. No error value
 *           is returned.
 *
 *           The purpose of this function is to allow driver objects to
 *           become independent of system parameters such as HZ, which
 *           may change in a future release or vary from one machine
 *           family member to another.
 */
clock_t
drv_usectohz(clock_t microsecs)
{
	clock_t quo, rem;
	clock_t remhz, quohz;

	if (microsecs <= 0)
		return 0;

	quo = microsecs / 1000000; /* number of seconds */
	rem = microsecs % 1000000; /* fraction of a second */
	quohz = HZ * quo; /* quo in HZ units */
	remhz = HZ * rem; /* remainder in millionths of HZ units */

	/* check for overflow */
	if (quo != quohz / HZ)
		return CLOCK_MAX;
	if (rem != remhz / HZ)
		remhz = CLOCK_MAX;
	
	/* adjust remhz since it was in millionths of HZ units */
	remhz = (remhz % 1000000) ? remhz / 1000000 + 1 : remhz / 1000000;


	if ((CLOCK_MAX - quohz) < remhz)
		return CLOCK_MAX;
	return quohz + remhz;
}



/*
 * void
 * drv_usecwait(count)
 * 
 * Calling/Exit State:
 *	     when a driver is in its init() routine or at interrupt
 *           level, it cannot call a function that might put it
 *           to sleep. delay() is such a function. In the past,
 *           drivers set up FOR loops of their own such as tenmicrosec()
 *           (see i386 kernel code) or used macros to busy-wait a
 *           period of time (i.e. when writing device registers).
 *           These solutions are not very portable and introduce
 *           binary compatibility problems. This function will
 *           offer the same basic functionality without detracting
 *           from the portability and binary compatibility of
 *           drivers.
 */

void
drv_usecwait(clock_t count)
{
	int tens=0;
	extern void _tenmicrosec(void);

	if (count > 10)
		tens=count/10;
	tens++;			/* roundup; wait at least 10 microseconds */
	while(tens > 0){
		_tenmicrosec();
		tens--;
	}
}

/*
 * int
 * drv_priv(void *credp)
 *	determine whether credentials are privileged
 *
 * Calling/Exit State:
 *           determine if the supplied credentials identify a privileged
 *           process.  To be used only when file access modes and
 *           special minor device numbers are insufficient to provide
 *           protection for the requested driver function.  Returns 0
 *           if the privilege is granted, otherwise EPERM.
 */ 

int
drv_priv(void *credp)
{
	return pm_privon((cred_t *)credp, pm_privbit(P_DRIVER)) ? 0 : EPERM;
}


/*
 * major_t
 * getmajor(dev_t dev)
 *	get internal major device number
 *
 * Calling/Exit State:
 *	none
 */

major_t
getmajor(dev_t dev)
{
 	return _GETMAJOR(dev);
}

/*
 * minor_t
 * getminor(dev_t dev)
 *	get internal minor device number
 *
 * Calling/Exit State:
 *	none
 */

minor_t
getminor(dev_t dev)
{
 	return _GETMINOR(dev);
}

/*
 * major_t
 * getemajor(dev_t dev)
 *	get external major device number
 *
 * Calling/Exit State:
 *	none
 */

major_t
getemajor(dev_t dev)
{
 	return _GETEMAJOR(dev);
}

/*
 * minor_t
 * geteminor(dev_t dev)
 *	get external minor device number
 *
 * Calling/Exit State:
 *	none
 */

minor_t
geteminor(dev_t dev)
{
 	return _GETEMINOR(dev);
}

/*
 * dev_t
 * makedevice(major_t major, minor_t minor)
 *	construct device number from major and minor numbers
 *
 * Calling/Exit State:
 *	none
 */

dev_t
makedevice(major_t major, minor_t minor)
{
	return _MAKEDEVICE(major, minor);
}

/*
 * ulong_t
 * ptob(ulong_t npages)
 *
 * Calling/Exit State:
 *	Returns npages (logical) pages converted to a number of bytes.
 */

ulong_t
ptob(ulong_t npages)
{
	return _PTOB(npages);
}

/*
 * ulong_t
 * btop(ulong_t nbytes)
 *
 * Calling/Exit State:
 *	Returns nbytes bytes converted to a number of pages, truncated.
 */

ulong_t
btop(ulong_t npages)
{
	return _BTOP(npages);
}

/*
 * ulong_t
 * btopr(ulong_t nbytes)
 *
 * Calling/Exit State:
 *	Returns nbytes bytes converted to a number of pages, rounded up.
 */

ulong_t
btopr(ulong_t npages)
{
	return _BTOPR(npages);
}

void
ATOMIC_INT_INIT(atomic_int_t *atomic_intp, int ival)
{
	_ATOMIC_INT_INIT(atomic_intp, ival);
}

int
ATOMIC_INT_READ(atomic_int_t *atomic_intp)
{
	return _ATOMIC_INT_READ(atomic_intp);
}

void
ATOMIC_INT_WRITE(atomic_int_t *atomic_intp, int ival)
{
	_ATOMIC_INT_WRITE(atomic_intp, ival);
}

void
ATOMIC_INT_INCR(atomic_int_t *atomic_intp)
{
	_ATOMIC_INT_INCR(atomic_intp);
}

boolean_t
ATOMIC_INT_DECR(atomic_int_t *atomic_intp)
{
	return _ATOMIC_INT_DECR(atomic_intp);
}

void
ATOMIC_INT_ADD(atomic_int_t *atomic_intp, int ival)
{
	_ATOMIC_INT_ADD(atomic_intp, ival);
}

void
ATOMIC_INT_SUB(atomic_int_t *atomic_intp, int ival)
{
	_ATOMIC_INT_SUB(atomic_intp, ival);
}


/*
 * int
 * uiomove(void *kernbuf, long n, uio_rw_t rw, uio_t *uio)
 *	Copy data to/from a kernel buffer for I/O.
 *
 * Calling/Exit State:
 *	See uiomove_catch().
 *
 * Description:
 *	See uiomove_catch().
 */

int
uiomove(void *kernbuf, long n, uio_rw_t rw, uio_t *uio)
{
	return _UIOMOVE(kernbuf, n, rw, uio);
}


/*
 * caddr_t
 * physmap(paddr_t, ulong_t, uint_t)
 * 	Allocate a virtual address mapping for a given range of
 *	physical addresses.  Generally used from a driver's init or
 *	start routine to get a pointer to device memory (a.k.a.
 *	memory-mapped I/O).  The flags argument may be KM_SLEEP or
 *	KM_NOSLEEP.  Returns a virtual address, or NULL if the
 *	mapping cannot be allocated.
 *
 * Calling/Exit State:
 *	None.
 */

caddr_t
physmap(paddr_t physaddr, ulong_t nbytes, uint_t flags)
{
	paddr_t base;
	ulong_t npages;
	caddr_t addr;

	if (nbytes == 0)
		return (caddr_t)NULL;
	
	PHYSMAP_F(physaddr, nbytes, flags);

	base = physaddr & PAGEMASK; 	/* round-down physical address */
	npages = _BTOPR(physaddr - base + nbytes);	/* round-up pages */

	addr = kpg_ppid_mapin(npages, phystoppid(base), PROT_ALL & ~PROT_USER,
				flags);

	if (addr == (caddr_t)NULL)
		return (caddr_t)NULL;
	else
		return addr + PAGOFF(physaddr);
}

/*
 * void
 * physmap_free(caddr_t, ulong_t, uint_t)
 * 	Release a mapping allocated by physmap().  The nbytes argument
 *	must be identical to that given to physmap().  Generally,
 *	physmap_free() will never be called, since drivers keep the
 *	mapping forever, but it is provided in case a driver wants to
 *	dynamically allocate mappings.  Currently, no flags are
 *	supported and the flags argument should be zero.
 *
 * Calling/Exit State:
 *	None.
 */

/*ARGSUSED*/
void
physmap_free(caddr_t vaddr, ulong_t nbytes, uint_t flags)
{
	vaddr_t base;
	ulong_t npages;

	if (vaddr == NULL)
		return;
	
	PHYSMAP_FREE_F(vaddr, nbytes, flags);

	base = (vaddr_t)vaddr & PAGEMASK; 	/* round-down address */
	npages = _BTOPR((vaddr_t)vaddr + nbytes - base);  /* round-up pages */

	kpg_mapout((void *)base, npages);
}

/*
 * void
 * ifstats_attach(struct ifstats *ifsp)
 *	Add a module/driver's interface statistics structure (struct ifstats)
 *	to the global list of such structures.  Drivers/modules will access
 *	their entry in the list directly, without needing to acquire the lock
 *	first.  The lock is only necessary in order to add/remove elements
 *	from the list and to traverse the list (which IP does to collect the
 *	statistics for the user).
 *
 * Calling/Exit State:
 *	None.
 */
void
ifstats_attach(struct ifstats *ifsp)
{
	pl_t	pl;

	pl = RW_WRLOCK(ifstats_rwlck, plstr);
	ifsp->ifs_next = ifstats;
	ifstats = ifsp;
	RW_UNLOCK(ifstats_rwlck, pl);
}

/*
 * struct ifstats *
 * ifstats_detach(struct ifstats *ifsp)
 *	Remove a module/driver's interface statistics structure
 *	(struct ifstats) from the global list of such structures.
 *	Returns the address of the structure (if found) or NULL (if not)
 *	to allow the caller to take action (if appropriate) if its
 *	statistics structure wasn't on the list.
 *
 * Calling/Exit State:
 *	None.
 */
struct ifstats *
ifstats_detach(struct ifstats *ifsp)
{
	struct ifstats	*tifsp;
	struct ifstats	**lastifspp;
	pl_t	pl;

	pl = RW_WRLOCK(ifstats_rwlck, plstr);
	lastifspp = &ifstats;
	for (tifsp = ifstats; tifsp != NULL; tifsp = tifsp->ifs_next) {
		if (tifsp == ifsp) {
			*lastifspp = tifsp->ifs_next;
			break;
		}
		lastifspp = &tifsp->ifs_next;
	}
	RW_UNLOCK(ifstats_rwlck, pl);
	return tifsp;
}
