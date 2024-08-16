/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/kdb/kdb_util/db_as.c	1.12"
#ident	"$Header: $"

#include <mem/as.h>
#include <mem/hat.h>
#include <mem/immu.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/kdb/db_as.h>
#include <util/kdb/db_slave.h>
#include <util/kdb/kdebugger.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/var.h>


/*
 * In order to map in an arbitrary physical address making minimal assumptions,
 * we need:
 *	1) the virtual address of our level 1 page table, and
 *	2) a page to use as a level 2 page table for which we know both
 *	   its virtual and physical address.
 *
 * We can achieve requirement #1 because the L1PT is at a well-known virtual
 * address (see db_kvtol1pte()).
 *
 * The only good candidate for requirement #2 is our level 1 page table.
 * This is because we can get its physical address from the hardware cr3
 * register, and we have its virtual address from #1.
 *
 * We will have to borrow two PTEs from the L1PT, one to use as an L1PTE
 * to point to the L1PT itself, and one to use as an L2PTE to point to the
 * desired page.  These are selected, by the virtual addresses they map,
 * with the #define's below; they must not map any virtual addresses which
 * include any text, data or stack of the kernel debugger.  The original
 * L1PTEs are restored by the mapout operation.
 *
 * We also need a third L1PTE to allow for a simultaneous second mapping
 * (via db_mapin2()).
 */
	/* Note: Arbitrary user addresses picked */
#define MAP_PTE_VADDR1	(UVBASE + VPTSIZE)
#define MAP_PTE_VADDR2	(UVBASE + 2 * VPTSIZE)
#define MAP_PTE_VADDR3	(UVBASE + 3 * VPTSIZE)

static pte_t	*map_pte1, *map_pte2, *map_pte3;
static uint_t	map_opte1, map_opte2, map_opte3;
static uint_t	map_mapped;
#ifdef DEBUG
static uint_t	map1_mapped, map2_mapped;
#endif

/*
 * db_kvtol1pte() gets the virtual addr of an L1PTE for a given cpu
 */
#define db_kvtol1pte(vaddr, cpu) \
		(&engine[cpu].e_local->pp_kl1pt[0][0] + ptnum(vaddr))

static void *
db_mapin(paddr_t paddr)
{
	ASSERT(map1_mapped++ == 0);
	ASSERT(map_mapped <= 1);

	if (map_mapped++ == 0) {
		/*
		 * Set up L1PTE:
		 *
		 * First, get a pointer to the PTE we're going to steal
		 */
		map_pte1 = db_kvtol1pte(MAP_PTE_VADDR1, l.eng_num);
		/* Save the current value */
		map_opte1 = map_pte1->pg_pte;
		/* Use the L1PT as the L2PT as well */
		map_pte1->pg_pte = mkpte(PG_V|PG_RW, pfnum(db_cr3()));
	}
	/*
	 * Set up the L2PTE:
	 *
	 * First, get a pointer to the PTE we're going to steal
	 */
	map_pte2 = db_kvtol1pte(MAP_PTE_VADDR2, l.eng_num);
	/* Save the current value */
	map_opte2 = map_pte2->pg_pte;
	/* Map the other stolen PTE to the desired address */
	map_pte2->pg_pte = mkpte(PG_V|PG_RW, pfnum(paddr));
	db_flushtlb();
	/* Compute the right address to wind through this incestuous mapping */
	return (void *)(MAP_PTE_VADDR1 + mmu_ptob(ptnum(MAP_PTE_VADDR2)) +
			PAGOFF(paddr));
}

static void
db_mapout(void)
{
	ASSERT(--map1_mapped == 0);
	ASSERT(map_mapped != 0);

	/* Restore the original values for the PTE(s) we borrowed */
	map_pte2->pg_pte = map_opte2;
	if (--map_mapped == 0)
		map_pte1->pg_pte = map_opte1;
	db_flushtlb();
}

void *
db_mapin2(paddr_t paddr)
{
	ASSERT(map2_mapped++ == 0);
	ASSERT(map_mapped <= 1);

	if (map_mapped++ == 0) {
		/*
		 * Set up L1PTE:
		 *
		 * First, get a pointer to the PTE we're going to steal
		 */
		map_pte1 = db_kvtol1pte(MAP_PTE_VADDR1, l.eng_num);
		/* Save the current value */
		map_opte1 = map_pte1->pg_pte;
		/* Use the L1PT as the L2PT as well */
		map_pte1->pg_pte = mkpte(PG_V|PG_RW, pfnum(db_cr3()));
	}
	/*
	 * Set up the L2PTE:
	 *
	 * First, get a pointer to the PTE we're going to steal
	 */
	map_pte3 = db_kvtol1pte(MAP_PTE_VADDR3, l.eng_num);
	/* Save the current value */
	map_opte3 = map_pte3->pg_pte;
	/* Map the stolen PTE to the desired address */
	map_pte3->pg_pte = mkpte(PG_V|PG_RW, pfnum(paddr));
	db_flushtlb();
	/* Compute the right address to wind through this incestuous mapping */
	return (void *)(MAP_PTE_VADDR1 + mmu_ptob(ptnum(MAP_PTE_VADDR3)) +
			PAGOFF(paddr));
}

void
db_mapout2(void)
{
	ASSERT(--map2_mapped == 0);
	ASSERT(map_mapped != 0);

	/* Restore the original value for the PTE(s) we borrowed */
	map_pte3->pg_pte = map_opte3;
	if (--map_mapped == 0)
		map_pte1->pg_pte = map_opte1;
	db_flushtlb();
}


#define BADADDR	((paddr_t)-1)

paddr_t
db_kvtop(ulong_t vaddr, int cpu)
{
	register pte_t		*pte;
	register paddr_t	paddr;

	pte = db_kvtol1pte(vaddr, cpu);
	if (!pte->pgm.pg_v)
		return BADADDR;

	if (PG_IS4MB(pte))
		return mmu_ptob(pte->pgm.pg_pfn) + PAG4OFF(vaddr);

	pte = (pte_t *)db_mapin(mmu_ptob(pte->pgm.pg_pfn) +
				pnum(vaddr) * sizeof(pte_t));
	if (!pte->pgm.pg_v)
		paddr = BADADDR;
	else
		paddr = mmu_ptob(pte->pgm.pg_pfn) + (vaddr & MMU_PAGEOFFSET);
	db_mapout();

	return paddr;
}


int
db_read(as_addr_t addr, void * buf, uint_t n)
{
	uint_t		cnt;
	ulong_t		vaddr;
	const proc_t	*uvirt_proc;

	do {
		cnt = n;
#ifndef UNIPROC
		if (addr.a_cpu != l.eng_num &&
		    (addr.a_as == AS_KVIRT || addr.a_as == AS_IO)) {
			if (mmu_btop((u_long)buf + cnt - 1) !=
			    mmu_btop((u_long)buf)) {
				cnt = MMU_PAGESIZE -
				      ((ulong_t)buf & MMU_PAGEOFFSET);
			}
			db_slave_command.addr = addr;
			db_slave_command.buf = buf;
			db_slave_command.n = cnt;
			KDB_SLAVE_CMD(addr.a_cpu, DBSCMD_AS_READ);
			if (db_slave_command.rval == -1)
				return -1;
			continue;
		}
#endif /* not UNIPROC */
		vaddr = addr.a_addr;
		if (addr.a_as != AS_IO) {
			if (mmu_btop(vaddr + cnt - 1) != mmu_btop(vaddr)) {
				cnt = MMU_PAGESIZE -
				      ((ulong_t)vaddr & MMU_PAGEOFFSET);
			}
		}
		if (addr.a_as == AS_KVIRT) {
			if (DB_KVTOP(vaddr, addr.a_cpu) == BADADDR)
				return -1;
		}
#ifndef UNIPROC
		if (l.eng_num != db_master_cpu)
			buf = db_mapin(DB_KVTOP((u_long)buf, db_master_cpu));
#endif /* not UNIPROC */

		switch (addr.a_as) {
		case AS_KVIRT:
			bcopy((caddr_t)vaddr, buf, cnt);
			break;
		case AS_UVIRT:
			if ((uvirt_proc = PSLOT2PROC(addr.a_mod)) == NULL)
				return -1;
			vaddr = db_uvtop(vaddr, uvirt_proc);
			if (vaddr == -1)
				return -1;
			/* FALLTHROUGH */
		case AS_PHYS:
			bcopy(db_mapin((paddr_t)vaddr), buf, cnt);
			db_mapout();
			break;
		case AS_IO:
			while (cnt >= sizeof(u_long)) {
				*((u_long *)buf) = db_inl(vaddr);
				buf = (u_long *)buf + 1;
				vaddr += sizeof(u_long);
				cnt -= sizeof(u_long);
				n -= sizeof(u_long);
			}
			if (cnt > 1) {
				*((u_short *)buf) = db_inw(vaddr);
				buf = (u_short *)buf + 1;
				vaddr += sizeof(u_short);
				cnt -= sizeof(u_short);
				n -= sizeof(u_short);
			}
			if (cnt != 0)
				*((u_char *)buf) = db_inb(vaddr);
			break;
		}

#ifndef UNIPROC
		if (l.eng_num != db_master_cpu)
			db_mapout();
#endif
	} while ((buf = (char *)buf + cnt),
		 (addr.a_addr += cnt),
		 (n -= cnt) > 0);

	return 0;
}

int
db_write(as_addr_t addr, const void * buf, uint_t n)
{
	uint_t		cnt;
	ulong_t		vaddr;
	const proc_t	*uvirt_proc;

	do {
		cnt = n;
#ifndef UNIPROC
		if (addr.a_cpu != l.eng_num &&
		    (addr.a_as == AS_KVIRT || addr.a_as == AS_IO)) {
			if (mmu_btop((u_long)buf + cnt - 1) !=
			    mmu_btop((u_long)buf)) {
				cnt = MMU_PAGESIZE -
				      ((ulong_t)buf & MMU_PAGEOFFSET);
			}
			db_slave_command.addr = addr;
			db_slave_command.buf = (void *)buf;
			db_slave_command.n = cnt;
			KDB_SLAVE_CMD(addr.a_cpu, DBSCMD_AS_WRITE);
			if (db_slave_command.rval == -1)
				return -1;
			continue;
		}
#endif /* not UNIPROC */
		vaddr = addr.a_addr;
		if (addr.a_as != AS_IO) {
			if (mmu_btop(vaddr + cnt - 1) != mmu_btop(vaddr)) {
				cnt = MMU_PAGESIZE -
				      ((ulong_t)vaddr & MMU_PAGEOFFSET);
			}
		}
		if (addr.a_as == AS_KVIRT) {
			if (DB_KVTOP(vaddr, addr.a_cpu) == BADADDR)
				return -1;
		}
#ifndef UNIPROC
		if (l.eng_num != db_master_cpu)
			buf = db_mapin2(DB_KVTOP((u_long)buf, db_master_cpu));
#endif /* not UNIPROC */

		switch (addr.a_as) {
		case AS_KVIRT:
			/*
			 * Note: for write, we have to re-map the virtual,
			 * since the original mapping may not include
			 * write permissions.
			 */
			bcopy(buf, db_mapin(DB_KVTOP(vaddr, l.eng_num)), cnt);
			db_mapout();
			break;
		case AS_UVIRT:
			if ((uvirt_proc = PSLOT2PROC(addr.a_mod)) == NULL)
				return -1;
			vaddr = db_uvtop(vaddr, uvirt_proc);
			if (vaddr == -1)
				return -1;
			/* FALLTHROUGH */
		case AS_PHYS:
			bcopy(buf, db_mapin((paddr_t)vaddr), cnt);
			db_mapout();
			break;
		case AS_IO:
			while (cnt >= sizeof(u_long)) {
				db_outl(vaddr, *((u_long *)buf));
				buf = (u_long *)buf + 1;
				vaddr += sizeof(u_long);
				cnt -= sizeof(u_long);
				n -= sizeof(u_long);
			}
			if (cnt > 1) {
				db_outw(vaddr, *((u_short *)buf));
				buf = (u_short *)buf + 1;
				vaddr += sizeof(u_short);
				cnt -= sizeof(u_short);
				n -= sizeof(u_short);
			}
			if (cnt != 0)
				db_outb(vaddr, *((u_char *)buf));
			break;
		}

#ifndef UNIPROC
		if (l.eng_num != db_master_cpu)
			db_mapout2();
#endif
	} while ((buf = (char *)buf + cnt),
		 (addr.a_addr += cnt),
		 (n -= cnt) > 0);

	return 0;
}


paddr_t
db_uvtop(ulong_t vaddr, const proc_t * proc)
{
	struct as *as;
	pte_t *ptep;
	hatpt_t *ptap;
	extern pte_t *hat_vtopte_l();

	if (vaddr >= UVEND || (as = proc->p_as) == NULL)
		return -1;

	ptep = hat_vtopte_l(&as->a_hat, vaddr, &ptap);
	if (ptep != NULL) {
		if (PG_ISVALID(ptep))
			return mmu_ptob(ptep->pgm.pg_pfn) + PAGOFF(vaddr);
		else
			return -1;
	} else if ((ptap != NULL) && PG_ISPSE(&ptap->hatpt_pde))
		return mmu_ptob(ptap->hatpt_pde.pgm.pg_pfn) + PSE_PAGOFF(vaddr);
	else
		return -1;
}
