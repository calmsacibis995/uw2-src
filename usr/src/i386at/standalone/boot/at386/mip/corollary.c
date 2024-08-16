/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1993 UNIX System Laboratories, Inc. 	*/
/*	  All Rights Reserved                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.   	            	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/mip/corollary.c	1.12"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/inline.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/mip.h>
#include <sys/param.h>

#include <boothdr/corollary.h>
#include <boothdr/cbus.h>
#define NEEDS_RDMA_FIX

#define COROLLARY_NAME1		0x0d

#define COROLLARY_NAME2		0xf2

#define K(x)			(x*1024)
#define XM_MEM_CEILING		MB(256)

#define IS_HOLE(x)		((x->current_mb < 16) && \
				(x->config_ptr->mbj[x->current_mb]))

/*
 * Only add memory out of the old configuration structure up to
 * OLDRRD_MAXMB.  The rest comes out of the new memory configuration
 * structure.
 */
#define CAN_ADD_MORE(x)		((BTE_INFO.memavailcnt < B_MAXARGS) && \
				 (x->current_mb < OLDRRD_MAXMB))

#define NULL	0

/*
 * The elements of the ci_findmem structure.
 *
 * current_mb:
 * 	The meg that is being currently being considered for inclusion
 *	in the bootinfo structure.
 *
 * config_ptr:
 *	Pointer to the beginning of the configuration structure.
 *
 * bootmemp:
 *	Pointer to the next free entry in the bootinfo structure.
 *	
 * mem_ceiling:
 *	Top of memory.
 * 
 * mem_base
 *	Bottom of memory.
 */

struct ci_findmem {
	int			current_mb;
	struct configuration	*config_ptr;
	struct bootmem		*bootmemp;
	unsigned int		mem_ceiling;
	unsigned int		mem_base;
};

extern int corollary_end();

struct ci_findmem	ci_findmem;

#define BIOS_START		((volatile unsigned *)0xF0000)
#define RAM_START		((volatile unsigned *)RRD_RAM)
#define CHECK_PATTERN1		((unsigned)0xABACADAE)
#define CHECK_PATTERN2		((unsigned)0x76543210)

extern int	corollary();
extern unsigned	corollary_check_ram();

int
corollary_ident()
{
	unsigned 		old_val;

	if (membrk((char *)0xF0000, "Corollary", 0xffff, 9) != 0)
	{
		a20();
#ifdef COROLLARY_DEBUG
		printf("SYM or CBUS2 machine\n");
#endif
		return M_ID_COROLLARY;
	}

	if (BTE_INFO.machflags & EISA_IO_BUS)
	{
		if ((inb(EISA_MFG_NAME1) & MFG_NAME1_MASK) != COROLLARY_NAME1)
			return 0;

		if (inb(EISA_MFG_NAME2) != COROLLARY_NAME2)
			return 0;

		a20();

#ifdef COROLLARY_DEBUG
		printf("EISA machine\n");
#endif
		return M_ID_COROLLARY;
	}

	old_val = *BIOS_START;

	*BIOS_START = CHECK_PATTERN1;

	if (corollary_check_ram() != CHECK_PATTERN1)
	{
		*BIOS_START = old_val;
		return 0;
	}

	*BIOS_START = CHECK_PATTERN2;

	if (corollary_check_ram() != CHECK_PATTERN2)
	{
		*BIOS_START = old_val;
		return 0;
	}

	*BIOS_START = old_val;

	a20();

	if (membrk((char *)0xFFFE0000, "Corollary", 0xffff, 9) == 0)
	{
		return 0;
	}

#ifdef COROLLARY_DEBUG
	printf("ISA machine\n");
#endif

	return M_ID_COROLLARY;
}

/*
 * This routine keeps the compiler from optimizing the 
 * RAM_START/BIOS_START read and writes;
 */
unsigned
corollary_check_ram()
{
	return *RAM_START;
}

int
corollary(lpcbp, machine)
struct	lpcb *lpcbp;
unsigned char machine;
{
	struct	int_pb		ic;
	unsigned		memsz;
	paddr_t			corollary_config_ptr;
	unsigned		corollary_string = 0xdeadbeef;
#ifdef COROLLARY_DEBUG
	unchar			c;
#endif
	/* 
	 * search the ram for the info check word 
	 */
	ci_findmem.config_ptr = (struct configuration *)
		membrk(RRD_RAM, &corollary_string, 0x8000, sizeof(int));

	if (ci_findmem.config_ptr == 0)
	{
#ifdef COROLLARY_DEBUG
		printf("Didn't find Corollary config ptr\n");
#endif
		return 0;
	}

	ci_findmem.config_ptr = (struct configuration *)
		((char *)ci_findmem.config_ptr - 1);

#ifdef COROLLARY_DEBUG
	printf("Found 0x%x at 0x%x\n", corollary_string, ci_findmem.config_ptr);
#endif

	ic.intval = 0x12;
	ic.ds = 0;
	doint(&ic);
	memsz = (unsigned)ic.ax;

#ifdef COROLLARY_DEBUG
	printf("Base memory size in K = 0x%x\n", (unsigned)memsz);
#endif
	corollary_findmem(&ci_findmem, memsz);

#ifdef COROLLARY_DEBUG
	shomem("\n- Memory Table\n", BTE_INFO.memavailcnt, BTE_INFO.memavail);
#endif

	MIP_end = (ulong)corollary_end;

#ifdef COROLLARY_DEBUG
	printf("dumping 0x%x = 0x%x\n", 
		ci_findmem.config_ptr, *((unsigned *)ci_findmem.config_ptr));

	printf("use table? (y/n) ");

	c = getchar();

	if (c != 'y')
		return 1;
#endif
	BOOTENV->be_flags |= BE_MEMAVAILSET;

	if (ci_findmem.mem_base == 0)
	{
		corollary_fix_base(&ci_findmem, BTE_INFO.memavail, 
			&BTE_INFO.memavailcnt);

		corollary_fix_base(&ci_findmem, BTE_INFO.memused, 
			&BTE_INFO.memusedcnt);
	}
	return 1;
}


corollary_findmem(cifm, memsz)
struct ci_findmem	*cifm;
unsigned 		memsz;
{
	struct ext_cfg_header		*ptr_header;
	struct ext_memory_board		*mem_ptr = NULL;
	struct ext_cfg_override		*cfg_ptr = NULL;
	char				*ptr_source;
	unsigned long 			basemem;
	unsigned 			corollary_xm_rev = 0;
	unsigned			malloc_base;
	int i;
#ifdef COROLLARY_DEBUG
	int j;
#endif

	BTE_INFO.memavail[0].base = 0;
	BTE_INFO.memavail[0].extent = BOOTENV->bootsize;
	BTE_INFO.memavail[0].flags |= B_MEM_BOOTSTRAP;
	BTE_INFO.memavailcnt = 1;

	ptr_source = (char *)cifm->config_ptr;

	ptr_source += sizeof(configuration);
	ptr_header = (struct ext_cfg_header *)ptr_source;

	if (*(unsigned *)ptr_source == EXT_CHECKWORD) 
	{
		do 
		{
			ptr_source += sizeof(struct ext_cfg_header);

			switch (ptr_header->ext_cfg_checkword) {
			case EXT_MEM_BOARD:
#ifdef COROLLARY_DEBUG
				printf("found EXT_MEM_BOARD\n");
#endif
				mem_ptr = (struct ext_memory_board *)ptr_source;
				break;
			case EXT_CHECKWORD:
			case EXT_VENDOR_INFO:
				break;
			case EXT_CFG_OVERRIDE:
#ifdef COROLLARY_DEBUG
				printf("found EXT_CFG_OVERRIDE\n");
#endif
				cfg_ptr = (struct ext_cfg_override *)ptr_source;
				break;
			case EXT_ID_INFO:
			case EXT_CFG_END:
				break;
			default:
				break;
			}
			
			ptr_source += ptr_header->ext_cfg_length;
			ptr_header = (struct ext_cfg_header *)ptr_source;

		} while (ptr_header->ext_cfg_checkword != EXT_CFG_END);
	}


	if (cfg_ptr)
	{
#ifdef COROLLARY_DEBUG
		printf("Base Ram 0x%x\n", cfg_ptr->baseram);
#endif
		cifm->mem_base = cfg_ptr->baseram;

#ifdef COROLLARY_DEBUG
		printf("Memory Ceiling 0x%x\n", cfg_ptr->memory_ceiling);
#endif
		cifm->mem_ceiling = cfg_ptr->memory_ceiling;
#ifdef COROLLARY_DEBUG
		printf("Hit return\n");
		getchar();
#endif
	}
	else
	{
		cifm->mem_base = MB(OLDRRD_MAXMB);
		cifm->mem_ceiling = MB(OLDRRD_MAXMB) + cifm->mem_base;
	}

	if (cifm->mem_base == 0)
	{
		if (inb(EISA_PROD_TYPE) == XM_TYPE)
		{
			corollary_xm_rev = inb(EISA_PROD_REV);

			if (REV(corollary_xm_rev) < NO_ROM_REV)
			{
#ifdef COROLLARY_DEBUG
				printf("Turning on 15 Meg hole\n");
#endif
				cifm->config_ptr->mbj[15] = 1;
			}
		}
	}
	else
	{
#ifdef COROLLARY_DEBUG
		printf("Turning on 15 Meg hole\n");
#endif
		cifm->config_ptr->mbj[15] = 1;
	}

	for ( cifm->current_mb = 0 ; CAN_ADD_MORE(cifm) ; cifm->current_mb++ )
	{
		cifm->bootmemp = &BTE_INFO.memavail[BTE_INFO.memavailcnt];

		cifm->bootmemp->base = MB(cifm->current_mb);
		cifm->bootmemp->extent = 0;
		cifm->bootmemp->flags = 0;

		/* special case the first chunk of memory below 1 MEG */
		if (cifm->bootmemp->base == 0)
		{
#ifdef COROLLARY_DEBUG
			printf("memory < 1MEG = %d\n", memsz);
			printf("BOOTENV->bf_resv_base = 0x%x\n",
				BOOTENV->bf_resv_base);

#endif
			cifm->bootmemp->flags = B_MEM_BASE;
			cifm->bootmemp->base = BOOTENV->bootsize;

			malloc_base = ptob(btop(BOOTENV->bf_resv_base));

			if (malloc_base)
			{
				cifm->bootmemp->extent = malloc_base - 
					BOOTENV->bootsize;
			}
			else
			{
				cifm->bootmemp->extent = ptob(btop(memsz*1024))
					- BOOTENV->bootsize;
			}

			BTE_INFO.memavailcnt++;

			if (malloc_base && (BOOTENV->be_flags & BE_BIOSRAM))
			{
				unsigned flags = cifm->bootmemp->flags;

				cifm->bootmemp++;
				cifm->bootmemp->flags = flags | B_MEM_BOOTSTRAP;
				cifm->bootmemp->base = (paddr_t) malloc_base;
				cifm->bootmemp->extent =
					ptob(btop(memsz - malloc_base));

				BTE_INFO.memavailcnt++;
			}
			continue;
		}

		if (IS_HOLE(cifm))
			collect_holes(cifm);
		else
			collect_memory(cifm);

		if (BTE_INFO.memavail[BTE_INFO.memavailcnt].extent == 0)
		{
			BTE_INFO.memavail[BTE_INFO.memavailcnt].base = 0;
			continue;
		}

		BTE_INFO.memavailcnt++;
	}

	if (mem_ptr == NULL)
	{
#ifdef COROLLARY_DEBUG
		shomem("\nMemory Table\n", BTE_INFO.memavailcnt, 
			BTE_INFO.memavail);
#endif
		return;
	}

	i = 0;
	while ((mem_ptr->mem_start + mem_ptr->mem_size) <= MB(64))
	{
#ifdef COROLLARY_DEBUG
		printf("skipping 0x%x 0x%x -", mem_ptr->mem_start, 
			mem_ptr->mem_size);
#endif

		mem_ptr++;
		i++;
	}

	for ( ; i < OLDRRD_MAXSLOTS ; i++, mem_ptr++)
	{
		if (mem_ptr->mem_size == 0)
			continue;

#ifdef COROLLARY_DEBUG
		printf("adding 0x%x 0x%x\n", mem_ptr->mem_start, 
			mem_ptr->mem_size);
#endif

		BTE_INFO.memavail[BTE_INFO.memavailcnt].base = 
			mem_ptr->mem_start;
		BTE_INFO.memavail[BTE_INFO.memavailcnt].extent = 
			mem_ptr->mem_size;
		BTE_INFO.memavail[BTE_INFO.memavailcnt].flags = 
			B_MEM_EXPANS | B_MEM_NODMA;

		if (mem_ptr->mem_start < MB(64))
		{
			BTE_INFO.memavail[BTE_INFO.memavailcnt].base = MB(64);
			BTE_INFO.memavail[BTE_INFO.memavailcnt].extent -= 
				(MB(64)-mem_ptr->mem_start);
		}

		BTE_INFO.memavailcnt++;
	}

#ifdef COROLLARY_DEBUG
	shomem("\nMemory Table\n", BTE_INFO.memavailcnt, BTE_INFO.memavail);
#endif
}

int
collect_memory(cifm)
struct ci_findmem	*cifm;
{
#ifdef COROLLARY_DEBUG
	printf("[0x%x]", MB(cifm->current_mb));
#endif
	cifm->bootmemp->base = MB(cifm->current_mb);
	cifm->bootmemp->extent = 0;

	/* Can't DMA above 16 MB */
	if (cifm->current_mb >= 16)
	{
		/*
		 *  Must be an XM machine.  We can re-claim the holes
		 *  above 256 meg.
		 */
		if (cifm->mem_ceiling == XM_MEM_CEILING)
			cifm->bootmemp->base += cifm->mem_ceiling;

		cifm->bootmemp->flags = B_MEM_NODMA | B_MEM_EXPANS;
	}
	else
		cifm->bootmemp->flags = B_MEM_EXPANS;

	while (CAN_ADD_MORE(cifm))
	{
		if (cifm->config_ptr->mb[cifm->current_mb] == 0)
			break;

		/* 
		 * Can't DMA above 16 MB, so don't include this MB.
		 */
		if ((cifm->current_mb >= 16) && 
			(cifm->bootmemp->flags == B_MEM_EXPANS))
		{
			BTE_INFO.memavailcnt++;
			cifm->bootmemp = 
				&BTE_INFO.memavail[BTE_INFO.memavailcnt];
			collect_memory(cifm);
			break;
		}


		if (IS_HOLE(cifm))
		{
			BTE_INFO.memavailcnt++;
			cifm->bootmemp = 
				&BTE_INFO.memavail[BTE_INFO.memavailcnt];
			collect_holes(cifm);
			break;
		}
#ifdef COROLLARY_DEBUG
		printf(".");
#endif
		cifm->bootmemp->extent += MB(1);

		cifm->current_mb++;
	}
}


int
collect_holes(cifm)
struct ci_findmem	*cifm;
{
#ifdef COROLLARY_DEBUG
	printf("<0x%x>", MB(cifm->current_mb));
#endif
	cifm->bootmemp->base = MB(cifm->current_mb);
	cifm->bootmemp->extent = 0;
	cifm->bootmemp->flags = B_MEM_NODMA | B_MEM_EXPANS;

	/*
	 *  Must be an XM machine.  We can re-claim the holes
	 *  above 256 meg.
	 */
	if (cifm->mem_ceiling == XM_MEM_CEILING)
		cifm->bootmemp->base += cifm->mem_ceiling;

	while (CAN_ADD_MORE(cifm))
	{
		if (cifm->config_ptr->mb[cifm->current_mb] == 0)
			break;

		if (!IS_HOLE(cifm))
		{
			BTE_INFO.memavailcnt++;
			cifm->bootmemp = 
				&BTE_INFO.memavail[BTE_INFO.memavailcnt];
			collect_memory(cifm);
			break;
		}
#ifdef COROLLARY_DEBUG
		printf("+");
#endif
		cifm->bootmemp->extent += MB(1);
		cifm->current_mb++;
	}
}


int
corollary_end()
{
	int i;
	struct ci_findmem	*cifm = &ci_findmem;
#ifdef COROLLARY_DEBUG
	unchar			c;
#endif
	struct bootmem		*bmptr, *bmptr_new;

	/*
	 * XM machines do not need to have mem_base added in.
	 */
	if (cifm->mem_base == 0)
	{
		corollary_reverse();
#ifdef COROLLARY_DEBUG
		shomem("mem-used\n", BTE_INFO.memusedcnt, BTE_INFO.memused);
		shomem("XM corollary_end\n", BTE_INFO.memavailcnt,
			BTE_INFO.memavail);
#endif
		return;
	}

	/*
	 * Not an XM machine.  We must re-locate memory for additional
	 * processors.
	 */
#ifdef COROLLARY_DEBUG
	printf("add mem_base? (y/n) ");

	c = getchar();

	if (c != 'y')
	{
		corollary_reverse();

		shomem("mem-used\n", BTE_INFO.memusedcnt, 
			BTE_INFO.memused);
		shomem("corollary_end\n", BTE_INFO.memavailcnt, 
			BTE_INFO.memavail);
		return;
	}
#endif

	/*
	 * Until rdma.c is changed, we have to throw away all non-dmaable
	 * memory below 16 meg.
	 */
#ifdef NEEDS_RDMA_FIX
	bmptr = &BTE_INFO.memavail[0];

	for (i=0 ; i < BTE_INFO.memavailcnt ; )
	{
		if (((bmptr->flags & B_MEM_NODMA) != B_MEM_NODMA) ||
		    (bmptr->base > MB(16)))
		{
			bmptr++;
			i++; 
			continue;
		}

		BTE_INFO.memavailcnt--;

		memcpy(bmptr, bmptr + 1,
			sizeof(struct bootmem) * (BTE_INFO.memavailcnt - i));
	}
#endif

	/*
	 * If COROLLARY is not defined then this a uniprocessor
	 * mip.  The uniprocessor kernel assumes that all DMA-able
	 * memory exists between 0 and 16MB.  For the MP case, all
	 * physical memory addresses must be shifted and dma_offset
	 * must be set, in order to change the tune.t_dmalimit and
	 * tune.t_dmabase to support memory on CBUS-I non-XM machines.
	 */
#ifdef COROLLARY
	corollary_fix_boot_ranges(&BTE_INFO.memused[0]);
	corollary_fix_boot_ranges(&BTE_INFO.memused[1]);

	for (i=0 ; i < BTE_INFO.memavailcnt ; i++)
	{
		BTE_INFO.memavail[i].base += cifm->mem_base;
	}

	/*
	 * starting at 2 to skip boot entries
	 */
	for (i=2 ; i < BTE_INFO.memusedcnt ; i++)
	{
		BTE_INFO.memused[i].base += cifm->mem_base;
	}

	/*
	 * BTE_INFO.dma_offset is used by the kernel to give the offset,
	 * in megabytes, for t_dmalimit and t_dmabase.
	 */
	BTE_INFO.dma_offset = cifm->mem_base / (1024 * 1024);
#endif

	/*
	 * Reversing first dma-able EXPANS entry and last entry.
	 */
	corollary_reverse();

#ifdef COROLLARY_DEBUG
	shomem("mem-used\n", BTE_INFO.memusedcnt, BTE_INFO.memused);
	shomem("CBUS corollary_end\n", BTE_INFO.memavailcnt, BTE_INFO.memavail);
#endif
}

/*
 * We must reverse the first dma-able B_MEM_EXPANS entry with the
 * last entry because bss is allocated from the end of this table.
 * The first B_MEM_EXPANS entry will be the largest dma-able chunk
 * of memory.
 */
corollary_reverse()
{
	struct bootmem		*bmptr, *bmptr_last, bm;
	int i;

	bmptr = &BTE_INFO.memavail[0];
	bmptr_last = &BTE_INFO.memavail[BTE_INFO.memavailcnt-1];

	for (i=0 ; i < BTE_INFO.memavailcnt ; i++, bmptr++)
	{
		if ((bmptr->flags & (B_MEM_EXPANS|B_MEM_NODMA)) != B_MEM_EXPANS)
			continue;

		bm.base = bmptr->base;
		bm.extent = bmptr->extent;
		bm.flags = bmptr->flags;

		bmptr->base = bmptr_last->base;
		bmptr->extent = bmptr_last->extent;
		bmptr->flags = bmptr_last->flags;

		bmptr_last->base = bm.base;
		bmptr_last->extent = bm.extent;
		bmptr_last->flags = bm.flags;

		return;
	}
	printf("Warning: no dma-able memory found\n");
}

corollary_fix_base(cifm, bmptr, count)
struct ci_findmem	*cifm;
struct bootmem *bmptr;
unsigned *count;
{
	struct bootmem		*bmptr_last;
	int i;

	bmptr_last = bmptr + *count;

	for (i=0 ; i < *count ; i++, bmptr++)
	{
		/*
		 * This is an XM machine and the additional processor
		 * can't see the memory between 512K and 1MB.  If a
		 * memavail extent is in this range then it can be
		 * recovered in high memory. 
		 */
		if (bmptr->base >= K(512)) 
		{
			if (bmptr->base < MB(1))
			{
				bmptr->base += cifm->mem_ceiling;
				bmptr->flags |= B_MEM_NODMA;
			}
			continue;
		}

		if (bmptr->base + bmptr->extent > K(512))
		{
			/*
			 * Add the portion of the extent that stretches
			 * over 512K to the end of the table
			 */
			bmptr_last->base = K(512) + cifm->mem_ceiling;
			bmptr_last->extent = 
				bmptr->base + bmptr->extent - K(512);
			bmptr_last->flags = bmptr->flags | B_MEM_NODMA;

			/*
			 * Reduce the original extent to not span the 512K
			 * boundry.
			 */
			bmptr->extent = bmptr->extent - bmptr_last->extent;

			(*count)++;
			bmptr_last++;
		}
	}
}

#define IN_EXTENT(memp, addr)	\
	((addr >= memp->base) && (addr < (memp->base + memp->extent)))

/*
 * Remove a segment of memory from the memavail array.
 */
corollary_fix_boot_ranges(memp)
struct bootmem	*memp;
{
	register struct bootmem		*bmptr, *bmptr_new;
	struct bootmem			bm;
	register int			i;

	bm.base = memp->base;
	bm.extent = memp->extent;
	bm.flags = memp->flags;

	bmptr = &BTE_INFO.memavail[0];

	for (i=0 ; i < BTE_INFO.memavailcnt ; bmptr++, i++)
	{
		if (IN_EXTENT(bmptr, bm.base))
			break;
	}

	if (i == BTE_INFO.memavailcnt)
	{
#ifdef COROLLARY_DEBUG
		printf("A chunk of memused memory did not exist in memavail");
#endif
		return;
	}

	/*
	 * Pick up any memory at the beginning of the memavail
	 * chunk and put it at the end of memavail.
	 */
	if (bm.base > bmptr->base)
	{
		bmptr_new = &BTE_INFO.memavail[BTE_INFO.memavailcnt++];

		bmptr_new->extent = (bm.base - bmptr->base);
		bmptr_new->base = bmptr->base;
		bmptr_new->flags = bmptr->flags;

		bmptr->base = bm.base;
		bmptr->extent -= (bm.base - bmptr->base);
	}

	/*
	 * after the above check, the memavail and memused chunks both
	 * start at the same address.
	 */

	/*
	 * Found all of the memused chunk in a larger memavail chunk.
	 * The memused chunk will be completely exhausted, so there
	 * is no more checking that is needed.
	 */
	if (bmptr->extent > bm.extent)
	{
		bmptr->base += bm.extent;
		bmptr->extent -= bm.extent; 
		return;
	}

	/*
	 * The amount memused is greater than the memavail chunk.
	 * The memavail chunk will be exausted and removed.
	 */
	bm.extent = bm.extent - bmptr->extent;
	bm.base += bmptr->extent;

	BTE_INFO.memavailcnt--;

	memcpy(bmptr, bmptr+1, 
		sizeof(struct bootmem) * (BTE_INFO.memavailcnt - i));

	/*
	 * If there is no more left in the memused chunk then exit
	 * the loop
	 */
	if (bm.extent == 0)
		return;

	/*
	 * The rest of this memused chunk falls into another memavail
	 * chunk.
	 */
	corollary_fix_boot_ranges(&bm);
}

