/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/sysdump.c	1.20"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/file.h>
#include <fs/specfs/snode.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <proc/bind.h>
#include <proc/mman.h> 
#include <svc/bootinfo.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/kcore.h>
#include <svc/psm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/locktest.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern int sleep();
extern void wakeup();

extern void modify_code(vaddr_t, vaddr_t);

extern int nintr;
extern  uchar_t intpri[];
extern  int	intcpu[];
extern  void	(*ivect[])();
extern  int	int_itype[];
extern 	int	intr_bindcpu[];
extern  int	intr_upcount[];
extern void intnull();

STATIC volatile buf_t *sysdump_bp;
STATIC ulong_t granularity;
STATIC ulong_t sysdump_buf_size;
STATIC vaddr_t sysdump_v;
STATIC vaddr_t zaddr;

#ifdef DEBUG
boolean_t sys_dump = B_FALSE;
#endif

STATIC volatile boolean_t code_modified = B_FALSE;
STATIC boolean_t sysdump_init_done = B_FALSE;

typedef struct {
	vaddr_t oldfunc;
	vaddr_t newfunc;
} mod_func_t;

STATIC pl_t sysdump_lock(lock_t *, pl_t);
STATIC void sysdump_unlock(lock_t *, pl_t);
STATIC void sysdump_void_return(void);
STATIC boolean_t sysdump_true_return(void);
 
/*
 * The following array consists of all the ksynch functions specified
 * in ddi that the drivers could call during disk i/o.
 */
static mod_func_t mod_funcs[] = {
#ifndef UNIPROC
#if defined DEBUG || defined SPINDEBUG
	{(vaddr_t)lock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)trylock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)unlock_dbg, (vaddr_t)sysdump_unlock},
	{(vaddr_t)rw_rdlock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_wrlock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_tryrdlock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_trywrlock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_unlock_dbg, (vaddr_t)sysdump_unlock},
	{(vaddr_t)lock_owned_dbg, (vaddr_t)sysdump_true_return},
	{(vaddr_t)rw_owned_dbg, (vaddr_t)sysdump_true_return},
#else
	{(vaddr_t)lock_nodbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)trylock_nodbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)unlock_nodbg, (vaddr_t)sysdump_unlock},
	{(vaddr_t)rw_rdlock, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_wrlock, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_tryrdlock, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_trywrlock, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_unlock, (vaddr_t)sysdump_unlock},
#endif  /* DEBUG  */
#endif  /* UNIPROC */
	{(vaddr_t)sleep_lock, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sleep_trylock, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sleep_unlock, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sv_broadcast, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sv_signal, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sv_wait, (vaddr_t)sysdump_void_return},
	{(vaddr_t)hier_lockcount, (vaddr_t)sysdump_true_return},
	{(vaddr_t)kbind, (vaddr_t)sysdump_void_return},
	{(vaddr_t)kunbind, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sleep, (vaddr_t)sysdump_void_return},
	{(vaddr_t)wakeup, (vaddr_t)sysdump_void_return},
	{0, 0}
};

/*ARGSUSED*/
STATIC pl_t
sysdump_lock(lock_t *lck, pl_t pl)
{
	return(spl(pl));
}

/*ARGSUSED*/
STATIC void
sysdump_unlock(lock_t *lck, pl_t pl)
{
	splx(pl);
}

/*ARGSUSED*/
STATIC void
sysdump_void_return(void)
{
	return;
}

/*ARGSUSED*/
STATIC boolean_t
sysdump_true_return(void)
{
	return B_TRUE;
}

#define DUMP_IO_SIZE	 8192	/* I/O size in bytes */
#define MZERO_GRANULARITY 512

/*
 * void sysdump_init(void)
 *	Open the dump device and pre-allocate a buffer.
 *
 * Calling/Exit State:
 *	Called from main. This function uses the sys_cred structure
 *	and thus this struct. should have been initialized by now.
 */
void
sysdump_init(void)
{
	extern dev_t dumpdev;
	extern cred_t  *sys_cred;
	vnode_t *sysdump_vp;
	bcb_t *bcbp;
	int err;

	if (dumpdev == NODEV)
		return;

	sysdump_vp = makespecvp(dumpdev, VBLK);
	err = VOP_OPEN(&sysdump_vp, FWRITE, sys_cred);
	if (err)
		return;
	/*
	 * The device minor could have changed by now by VOP_OPEN()
	 * for ex. if clone devices need to be opened. Thus reinitalize
	 * dumpdev here.
	 */
	dumpdev = sysdump_vp->v_rdev;

	err = bdevsw[getmajor(dumpdev)].d_devinfo(dumpdev, DI_BCBP, &bcbp);
	if (err)
		return;
	if (bcbp == NULL)
		return;

	sysdump_bp = getrbuf(KM_NOSLEEP);
	ASSERT(sysdump_bp);

	granularity = bcbp->bcb_granularity;
	sysdump_buf_size = min(DUMP_IO_SIZE, bcbp->bcb_max_xfer);
	sysdump_buf_size -= sysdump_buf_size % granularity;

	sysdump_bp->b_un.b_addr = kmem_alloc_physreq(sysdump_buf_size,
					bcbp->bcb_physreqp, KM_NOSLEEP);
	ASSERT(sysdump_bp->b_un.b_addr);

	sysdump_v = kpg_vm_alloc(btopr(sysdump_buf_size), NOSLEEP);
	ASSERT(sysdump_v);

	zaddr = (vaddr_t)kmem_zalloc(MZERO_GRANULARITY, KM_NOSLEEP);
	ASSERT(zaddr);

	sysdump_init_done = B_TRUE;
}

/*
 * void sysdump_sync(void)
 *	When doing system dump, we make all other processors just idle.
 * 	This is because, we are modifying code that cannot be executed
 *	by other processors.
 *
 * Calling/Exit State:
 *	None
 */
/* ARGSUSED */
void
sysdump_sync(void *dummy)
{
	DISABLE_PRMPT();
	saveregs(&u.u_kcontext);
	while (!code_modified)
		;
#ifdef DEBUG
	if (l.intr_depth) {
		int lvl = l.intr_depth;
		l.intr_depth = 0;
		spl0();
		l.intr_depth = lvl;
	}
#else
	spl0();
#endif
	while (1)
		;
	/* NOTREACHED */
}

/*
 * void sysdump_write_buf(void *addr, off_t *off, size_t sz)
 *	Writes the buffer to dumpdev in the given offset.
 *
 * Calling/Exit State:
 *	Returns -1 , if error.
 *	Returns 0, if success.
 */
STATIC int
sysdump_write_buf(void *addr, off_t *off, size_t sz)
{
	uint_t	maj = getmajor(dumpdev);

	while (sz) {
		if (CONSOLE_GETC() != -1) {
			/* Aborted by user */
			return -1;
		}

		bcopy(addr, sysdump_bp->b_un.b_addr, min(sysdump_buf_size, sz));
		sysdump_bp->b_blkno = btodt(*off);
		bioreset((buf_t *)sysdump_bp);

		sysdump_bp->b_bcount = sysdump_buf_size;
		if (sz < sysdump_buf_size) {
			if (sz % granularity != 0)
				sysdump_bp->b_bcount = (sz + granularity) & 
							~(granularity - 1);
			else
				sysdump_bp->b_bcount = sz;
		}

		*off += sysdump_bp->b_bcount;

		(*bdevsw[maj].d_strategy)(sysdump_bp);

		while ((sysdump_bp->b_flags & B_DONE) == 0)
			;

		if (geterror(sysdump_bp)) {
			cmn_err(CE_CONT, "^\n");
			cmn_err(CE_NOTE, "^I/O error during system dump.");
			return -1;
		}

		if (sz < sysdump_buf_size)
			break; 
		sz -= sysdump_buf_size;
	}
	return 0;
}

STATIC int print_count;

/*
 * void sysdump_new_chunk(mreg_chunk_t *mchunk, off_t *chunk_off,
 *		vaddr_t *base_addr)
 *	Writes the chunk header to dumpdev in offset chunk_off
 *	and writes all the image data belonging to the chunk.
 *
 * Calling/Exit State:
 *	Argument base_addr contains the physical address of all 
 *	image regions.
 *	The offset for the next chunk is set in outarg chunk_off.
 *	The mchunk is zeroed after it is written out.
 */
STATIC int
sysdump_new_chunk(mreg_chunk_t *mchunk, off_t *chunk_off, vaddr_t *base_addr)
{
	int err = 0, num_mregs = 0;
	size_t sz, len;
	off_t base, blkoff;
	int npages;
	extern void _watchdog_hook(void);

	err = sysdump_write_buf(mchunk, chunk_off, sizeof(mreg_chunk_t));
	if (err)
		return -1;

	while (num_mregs < NMREG &&
		(sz = MREG_LENGTH(mchunk->mc_mreg[num_mregs])) != 0) {

		if (MREG_TYPE(mchunk->mc_mreg[num_mregs]) != MREG_IMAGE) {
			num_mregs++;
			continue;
		}
		base = base_addr[num_mregs];
		while (sz) {
			npages = min(btopr(sysdump_buf_size), btopr(sz));
			hat_statpt_devload((vaddr_t)sysdump_v, npages,
				phystoppid(base), PROT_READ | PROT_WRITE);

			blkoff = base & (PAGESIZE - 1);
			len = min((npages * PAGESIZE) - blkoff, sz);

			err = sysdump_write_buf((void *)(sysdump_v + blkoff),
						chunk_off, len);
			if (err)
				break;

			sz -= len;

			hat_statpt_unload(sysdump_v, npages);
			TLBSflushtlb();
			if (err)
				return -1;

			base += len;

			if ((++print_count & 0xf) == 0)
				cmn_err(CE_CONT, "^.");
			/*
			 * Indicate that the system is alive. Call all the
			 * registered handlers to reset the watchdog timer.
			 */
			_watchdog_hook();
		}
		num_mregs++;
	}

	bzero((caddr_t)mchunk, sizeof(mreg_chunk_t));
	mchunk->mc_magic = MRMAGIC;
	return 0;
}

/*
 * void sysdump(void)
 *	Take a system dump.
 *
 * Calling/Exit State:
 *	The function is called when all other cpus have been quiesced
 *	and at this point the execution is single-threaded.
 */
void
sysdump(void)
{
	int i, sz, err, num_mregs, oldcpu;
	off_t chunk_off = 0;
	size_t len, vlen, blkoff;
	paddr_t base, baddr;
	emask_t responders;
	kcore_t header;
	mreg_chunk_t mchunk;
	vaddr_t base_addr[NMREG];
	int type;
	ppid_t ppid;
	ulong_t npages;

	if (!sysdump_init_done)
		return;

#ifdef DEBUG
	sys_dump = B_TRUE;
#endif
	DISABLE_PRMPT();

	CONSOLE_SUSPEND();

	/*
	 * Turn off all interrupts except the disk and the clock 
	 * and xcall interrupts.
	 */
	for (i = 0; i < nintr; i++) {
		if (ivect[i] == intnull || intpri[i] >= PLHI || 
		    intpri[i] == PLDISK)
			continue;

		oldcpu = intr_bindcpu[i];
		if (oldcpu == -1 && intr_upcount[i] != 0)
			oldcpu = 0;
		psm_introff(i, intpri[i], intcpu[i] = oldcpu, int_itype[i]);
	}

	xcall_all(&responders, B_TRUE, sysdump_sync, NULL);

	for (i = 0; mod_funcs[i].oldfunc != 0; i++)
		modify_code(mod_funcs[i].oldfunc, mod_funcs[i].newfunc);

	code_modified = B_TRUE;

#ifdef DEBUG
	if (l.intr_depth)
		l.intr_depth = 0;
#endif
	spl0();

	cmn_err(CE_CONT, "^\nSaving system memory image for crash analysis...\n"
		"   (Press any key to abort dump.)\n");

	sysdump_bp->b_edev = dumpdev;

	i = 0;

	header.k_align = granularity;
	header.k_magic = KCORMAG;
	header.k_version = KCORVER;
	header.k_size = 0;	/* size is unknown */

	u.u_flags |= U_CRITICAL_MEM;

	/*
	 * write header out.
	 */
	err = sysdump_write_buf(&header, &chunk_off, sizeof(kcore_t));
	if (err)
		goto abort;

	baddr = 0;
	num_mregs = 0;

	bzero((caddr_t)&mchunk, sizeof(mreg_chunk_t));
	mchunk.mc_magic = MRMAGIC;

	for (i = 0; i < bootinfo.memavailcnt; i++) {

		base = bootinfo.memavail[i].base;
		sz = bootinfo.memavail[i].extent;

		ASSERT((sz & PAGEOFFSET) == 0);

		if (baddr != base) {
			if (MREG_LENGTH(mchunk.mc_mreg[num_mregs]) &&
				 ++num_mregs == NMREG) {

				err = sysdump_new_chunk(&mchunk, &chunk_off,
					base_addr);
				if (err)
					goto abort;
				num_mregs = 0;
			}
			mchunk.mc_mreg[num_mregs] = MREG_HOLE;
			ASSERT(base > baddr);
			mchunk.mc_mreg[num_mregs] += 
					(base - baddr) << MREG_LENGTH_SHIFT;
		}

		baddr = base + sz;

#ifdef DEBUG
		cmn_err(CE_CONT, "^base = %x extent = %x sz = %x\n", 
			bootinfo.memavail[i].base,
			bootinfo.memavail[i].extent, sz);
#endif
		while (!geterror(sysdump_bp) && sz) {
			npages = min(btopr(sysdump_buf_size), btopr(sz));
			ppid = phystoppid(base);
			hat_statpt_devload((vaddr_t)sysdump_v, npages, ppid,
					   PROT_READ | PROT_WRITE);

			vlen = min(sz, ptob(npages));
			sz -= vlen;
			blkoff = 0;
			while (vlen) {
				if ((len = vlen) > MZERO_GRANULARITY)
					len = MZERO_GRANULARITY;

				if (!bcmp((caddr_t)zaddr, 
					  (caddr_t)sysdump_v + blkoff,
					  len))
					type = MREG_ZERO;
				else
					type = MREG_IMAGE;

				if (MREG_LENGTH(mchunk.mc_mreg[num_mregs]) != 
						0) {
					if (MREG_TYPE(mchunk.mc_mreg[num_mregs]) !=
								type) {
						if (++num_mregs == NMREG) {
							hat_statpt_unload(sysdump_v, 
								  npages);
							TLBSflushtlb();
							err = 
							    sysdump_new_chunk(&mchunk,
								&chunk_off,
								base_addr);
							if (err)
								goto abort;
							hat_statpt_devload(sysdump_v,
								   npages,
								   ppid, 
								   PROT_READ |
								   PROT_WRITE);
							num_mregs = 0;
						}
						base_addr[num_mregs] = base;
					}
				} else {
					base_addr[num_mregs] = base;
				}
				mchunk.mc_mreg[num_mregs] |= type;
				mchunk.mc_mreg[num_mregs] += len << 
							     MREG_LENGTH_SHIFT;
				vlen -= len;
				base += len;
				blkoff += len;
			}
			hat_statpt_unload(sysdump_v, npages);

			TLBSflushtlb();
		}
	}
	/*
	 * write the last chunk for the image.
	 */
	if (num_mregs || MREG_LENGTH(mchunk.mc_mreg[0]) != 0) {
		err = sysdump_new_chunk(&mchunk, &chunk_off, base_addr);
		if (err == -1)
			goto abort;
	}
	/*
	 * write final marker chunk out. 
	 */
	err = sysdump_new_chunk(&mchunk, &chunk_off, base_addr);
	if (err == -1) {
abort:
		cmn_err(CE_CONT, "^\n\nSystem memory dump aborted.\n");
		return;
	}

	/*
	 * give some time for disk buffers to get written out since
	 * the system will be rebooted.
	 */
	for (i = 0; i < 100000; i++)
		;
	cmn_err(CE_CONT, "^\n\nSystem memory dump complete.\n");
}
