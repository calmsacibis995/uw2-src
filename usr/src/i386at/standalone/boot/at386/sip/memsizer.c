/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)stand:i386at/standalone/boot/at386/sip/memsizer.c	1.4.1.15"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/bootinfo.h> 
#include <boothdr/boot.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/cram.h>
#include <sys/pci.h>
#include <boothdr/bootdef.h>
#include <boothdr/initprog.h>
#include <boothdr/sip.h>

/*
 *	memory scanning routine
 *	testing all memory chunks except shadow memory
 */
memtest()
{

	struct 	bootmem *map, *mrp;
	ulong	memsrt;
	ulong	base_sum, ext_sum, cmos_base_sum;
	struct	memconfig	*mem = &BOOTENV->memconfig;
	int	i;


	/* set page boundary for base memory */
	base_sum = (ulong)ptob(btop(mem->base_mem * 1024));

	/* merging the memory used by the boot program and BIOS together */
	if (BOOTENV->bf_resv_base)
		base_sum = (ulong)ptob(btop(BOOTENV->bf_resv_base));

	/* set total base memory from CMOS value */
	cmos_base_sum = (ulong)(mem->CMOSmembase * 1024);

	/* set page boundary for extended memory */
	ext_sum = (ulong)ptob(btop(mem->CMOSmemext * 1024));

#ifdef MEM_DEBUG
		shomem("memtst: memory ranges", BOOTENV->memrngcnt,
			BOOTENV->memrng);
		printf("memsize: CMOS base %dKB, expansion %dKB, sys >1MB(max in PS2) %dKB\n",
			mem->CMOSmembase, mem->CMOSmemext,
			mem->CMOSmem_hi);
		printf("         BIOS base %dKB, system %dKB\n",
			mem->base_mem, mem->sys_mem);
		printf("sizer base_sum = 0x%x, ext_sum = 0x%x \n",
				(ulong)base_sum, ext_sum);
		goany();
#endif

	/* set aside the first avail slot for use by boot */
	BTE_INFO.memavailcnt = 1;
	map = &BTE_INFO.memavail[0];
	map->base = 0;
	map->extent = BOOTENV->bootsize;
	map++->flags |= B_MEM_BOOTSTRAP;

	/* loop through all memory ranges */
	mrp = &BOOTENV->memrng[0]; 
	mrp[BOOTENV->memrngcnt].extent = 0; 

	for (; mrp->extent > 0; mrp++) {

		memcpy(map, mrp, sizeof(struct bootmem));

		/* set shadow memory as boot reserved */
		if (map->flags & B_MEM_SHADOW) {
			map->flags |= B_MEM_BOOTSTRAP;
			goto continue_probe;
		} 

		/*
		 * split base memory if used by BIOS/boot program memory
		 * allocator. Take the CMOS value and do no memory scan
		 */
		if ( map->flags & B_MEM_BASE) {
			map->base = BOOTENV->bootsize;
			map->extent = base_sum - map->base;
			if ((cmos_base_sum > base_sum) &&
				(BOOTENV->be_flags & BE_BIOSRAM)) {
#ifdef MEM_DEBUG
					printf("NOTICE: Base memory used by \
BIOS\n");
					printf("cmos base end= 0x%x bios base \
end= 0x%x\n",
					cmos_base_sum, base_sum);
#endif
				map++;
				BTE_INFO.memavailcnt++;
				map->extent=ptob(btop(cmos_base_sum-base_sum));
				map->base = (paddr_t) base_sum;
				map->flags= mrp->flags| B_MEM_BOOTSTRAP;
			}

			goto continue_probe;
		} 

		/* check extended memory here */
		if (map->flags & B_MEM_EXPANS) { 
#ifdef MEM_DEBUG
			if (BOOTENV->db_flag & BOOTTALK) {
				printf("base 0x%x extent 0x%x flags 0x%x\n",
				    map->base, map->extent, map->flags );
			}
#endif
			memsrt = map->base;
			if (mrp->flags & B_MEM_TREV) 
				memsrt -= MMU_PAGESIZE;

			/* For memory above 16M:-	
			 * no memory scan will be performed unless
			 * there must be at least 3 frames of non-wrap memory
			 * and no clipped extended memory below.
			 * If explicit memory probing is set then ignore clipped
			 * extended memory flag.
			 * For memory less than 16M:	
			 * clip extended memory to CMOS recorded limit	
			 */
			if (memsrt >= MEM16M) {
				if ((BOOTENV->be_flags & BE_16MWRAPSET) || (map->flags & B_MEM_SKIPCHK) )
					;
				else {
					/* probe first three frames */
					for (i=0; i<3; i++) {
				    		if (memwrap(memsrt + (ulong)(i*MMU_PAGESIZE),memsrt))
							break;
					}
					if ( i < 3 )
						map->extent = 0;
				}

			} else if (( INTERVAL(BIOS_32_START,BIOS_32_SIZE, 
							memsrt)) && pcichk() )
					map->extent = 0;
			else if (( memsrt < 0x100000 ) &&
					!(map->flags & B_MEM_FORCE))
				map->extent = 0;
			else if ((map->extent > ext_sum) && 
					!(map->flags & B_MEM_FORCE))
				map->extent = ext_sum;

			/* skip range if no memory specified */
			if (map->extent == 0) 
				continue;

			/* check if we need to actually probe memory */
			if (map->flags & B_MEM_SKIPCHK){
				/* use CMOS memory size */
				if(map->base > ext_sum)
					map->extent=0;
				else if((map->base+map->extent) > ext_sum)
					map->extent=ext_sum-map->base;
				goto continue_probe;
			}

			if((BOOTENV->memrng_updated == TRUE) ||
					(map->flags & B_MEM_FORCE)){
				if (map->extent = memchk(memsrt, map->extent,
							 map->flags)) {

					if (map->flags & B_MEM_TREV)
						map->base -= map->extent;
					if (memsrt < MEM16M) 
						ext_sum -= map->extent;
				}
				goto continue_probe;
			}
				
		}
continue_probe:
		/* if memavail defined, bump counter/pointer */
		if (map->extent) {
			map++; 
			BTE_INFO.memavailcnt++;
		}
	}
	map->extent = 0;

}

/*
 *	testing shadow memory - stop when testr fails
 */
tst_shdram()
{
	struct 	bootmem *map;
	struct 	bootmem *macurp;
	struct 	bootmem *manxtp;
	ulong	memsrt;

	for (map = &BTE_INFO.memavail[0]; map->extent > 0; map++) {

/*		skip non-shadow ram				*/
		if (!(map->flags & B_MEM_SHADOW))
			continue;

		memsrt = map->base;
		if (map->flags & B_MEM_TREV) 
			memsrt -= MMU_PAGESIZE;

		map->extent = memchk(memsrt, map->extent, map->flags);
		if (map->extent > 0) {
			if (map->flags & B_MEM_TREV)
				map->base -= map->extent;
		} else {
/*			move all next one's up one slot			*/
			macurp = map;
			manxtp = map+1;
			for (;manxtp->extent; macurp++, manxtp++)
				memcpy(macurp,manxtp,sizeof(struct bootmem));
			map--;
			BTE_INFO.memavail[--BTE_INFO.memavailcnt].extent = 0;
		}
	}

#ifdef MEM_DEBUG
	if ( BOOTENV->db_flag & BOOTTALK) 
		shomem("tst_shdram: found avail memory area",BTE_INFO.memavailcnt,BTE_INFO.memavail);
#endif
}

/*
 *	checkerboard memory tester
 */
memchk(memsrt, cnt, flag)
ulong	memsrt;
ulong	cnt;
ushort	flag;
{
	volatile ulong	*ta;
	volatile ulong	memsave1;
	volatile ulong	memsave2;
	volatile ulong	x,cache_lines=8;
	int	bytecnt = 0;
	int	c_off = 0, i;

	ta = (ulong *)memsrt;

#ifdef MEM_DEBUG
	if ( BOOTENV->db_flag & BOOTTALK)
		printf("Memory test starting %x %s cnt %x",ta,
			(flag & B_MEM_TREV)?"down":"up", cnt);
	goany();
#endif
	if (((BOOTENV->be_flags & BE_NOINVD) == 0)&& ((c_off = detect_cpu()) > 0)){
		cache_off();
		inv_cache();
		cache_lines=0;
	}

	for (; cnt; cnt-=MMU_PAGESIZE) {
		memsave1 = *ta;
		*ta++ = MEMTEST1;
		memsave2 = *ta;
		*ta-- = MEMTEST0;
		/* force cache line flush */
		for( i=0; i<cache_lines; i++)
			x = *(ulong *)((ulong)ta%65536 + i*65536); 

		if ( *ta == MEMTEST1 ) {
			*ta++ = MEMTEST2;
			*ta-- = MEMTEST0;
			for( i=0; i<cache_lines; i++)
				x = *(ulong *)((ulong)ta%65536 + i*65536); 

			if (*ta == MEMTEST2 ) {
				bytecnt += MMU_PAGESIZE;
			} else {
#ifdef BOOT_DEBUG
				if ( BOOTENV->db_flag & MEMDEBUG)
					printf(" abort 2");
#endif
				break;
			}
		} else {
#ifdef BOOT_DEBUG
			if ( BOOTENV->db_flag & MEMDEBUG)
				printf(" abort 1");
#endif
			break;
		}
		*ta++ = memsave1;
		*ta-- = memsave2;
		
		if (flag & B_MEM_TREV)
			ta -= (MMU_PAGESIZE / sizeof(long));
		else 
			ta += (MMU_PAGESIZE / sizeof(long));
	}

#ifdef MEM_DEBUG
	if (BOOTENV->db_flag & (BOOTTALK|MEMDEBUG))
		printf(" ended at %x, area size %x\n", ta, bytecnt);
	goany();
#endif
	if ( c_off > 0 )
		cache_on();

	return(bytecnt);
}


/*
 *	testing memory at two different locations to determine
 *	whether the physical lines are wrapped around
 */
memwrap(memsrt, memoff)
ulong	memsrt;
ulong	memoff;
{
	volatile ulong	*ta;
	volatile ulong	*ta_wrap;
	volatile ulong	save_val;
	volatile ulong	x,cache_lines=8;
	int	mystatus = 1;	/* assume memory is wrap around		*/
	int	c_off = 0,i;

	if (((BOOTENV->be_flags & BE_NOINVD) == 0)&& ((c_off = detect_cpu()) > 0)){
		cache_off();
		inv_cache();
		cache_lines=0;
	}

	ta = (ulong *)memsrt;
	ta_wrap = (ulong *)(memsrt-memoff);
	save_val = *ta_wrap;
	*ta = MEMTEST1;
	/* force cache line flush */
	for( i=0; i<cache_lines; i++)
		x = *((ulong *)((ulong)ta%65536 + i*65536)); 

	if ((*ta == MEMTEST1) && (*ta != *ta_wrap)) {
		*ta = MEMTEST2;
		for( i=0; i<cache_lines; i++)
			x = *((ulong *)((ulong)ta%65536 + i*65536)); 

		if ((*ta == MEMTEST2) && (*ta != *ta_wrap))
			mystatus = 0;
	}
	if ( c_off > 0 )
		cache_on();

	*ta = save_val;

#ifdef BOOT_DEBUG
	printf("memwrap: ta= 0x%x ta_wrap= 0x%x memory %s\n", ta, ta_wrap, (mystatus? "WRAP": "NO WRAP"));
#endif

	return(mystatus);
}

/*
 * time to be smart about what we do re: memory.  On MCA
 * systems, an extended BIOS call can be used to read
 * in a memory configuration structure.  Similarly, EISA
 * can provide memory information, but it is provided on
 * a per-slot/per-function basis.
 */

sip_memconfig()
{
	paddr_t membuf;
	struct int_pb ic;
	int	memrngidx, success, rv;

	memrngidx = 0;
	if (BTE_INFO.machflags & MC_BUS) {
		mca_config mca;
		unsigned int	sizeInK;

#ifdef BOOT_DEBUG
		printf("sip_memconfig: MCA Architecture\n");
#endif
		success = 1;
		membuf = physaddr(&mca);
		ic.ds = (ushort) membuf >> 4;
		ic.si = (ushort) membuf & 0xf;
		ic.intval = 0x15;
		ic.ax = 0xC700;
		if (rv = doint(&ic))
			success = 0;
		if ((ic.ax & 0xFF00) == 0x8000)
			success = 0;
		if ((ic.ax & 0xFF00) == 0x8600)
			success = 0;
		if (!success)
			return;

#ifdef BOOT_DEBUG
		printf("	sip_memconfig: meminfo structure:\n");
		printf("	data size: %u\n",
		    mca.dsize);
		printf("	local 1-16: %u\n",
		    combine(mca.locHI, mca.locLO));
		printf("	local 16-4G: %u\n",
		    combine(mca.loc4GBHI, mca.loc4GBLO));
		printf("	system 1-16: %u\n",
		    combine(mca.sysHI, mca.sysLO));
		printf("	system 16-4G: %u\n",
		    combine(mca.sys4GBHI, mca.sys4GBLO));
		printf("	cache 1-16: %u\n",
		    combine(mca.cacheHI, mca.cacheLO));
		printf("	cache 16-4G: %u\n",
		    combine(mca.cache4GBHI, mca.cache4GBLO));
		printf("	start 1-16: %u\n",
		    combine(mca.startHI, mca.startLO));
		printf("	start 16-4G: %u\n",
		    combine(mca.start4GBHI, mca.start4GBLO));
#endif

		/* assume 640K base.  We'll allow BIOS/CMOS to override */
		BOOTENV->memrng[0].base = 0;
		BOOTENV->memrng[0].extent = 640 * 1024;
		BOOTENV->memrng[0].flags = B_MEM_BASE;

		memrngidx = 1;
		/* now do memory between 1MB and 16 MB  if it exists */

		if (sizeInK = combine(mca.sysHI, mca.sysLO)) {
			BOOTENV->memrng[memrngidx].base = 0x100000;
			BOOTENV->memrng[memrngidx].extent = sizeInK * 1024;
			/*			BOOTENV->memrng[memrngidx].flags = B_MEM_EXPANS|B_MEM_BIOS; */
			BOOTENV->memrng[memrngidx].flags = B_MEM_EXPANS | B_MEM_FORCE;
			memrngidx++;
		}

		/* now do memory between 16MB and 4GB if it exists */
		if (sizeInK = combine(mca.sys4GBHI, mca.sys4GBLO)) {
			BOOTENV->memrng[memrngidx].base = 0x100000 * 16;
			BOOTENV->memrng[memrngidx].extent = sizeInK * 1024;
			/*			BOOTENV->memrng[memrngidx].flags = B_MEM_EXPANS|B_MEM_BIOS; */
			BOOTENV->memrng[memrngidx].flags = B_MEM_EXPANS;
			memrngidx++;
		}

		if( memrngidx != 0 ) {
			BOOTENV->memrngcnt = memrngidx;
		}
	} else 
	if (BTE_INFO.machflags & EISA_IO_BUS) {
		int	i, j, k;
		eisa_config eisa;
		eisa_mem_info * emp;

#ifdef BOOT_DEBUG
		printf("	EISA Architecture\n");
#endif
		i = (unsigned int) &eisa.memlist[0].memflgs - 
		    (unsigned int) &eisa.pad1[0];

		for (i = 0; i < EISA_NSLOTS; i++) {
			/* do a BRIEF configuration BIOS call */
			ic.ds = 0;
			ic.intval = 0x15;
			ic.ax = EISA_CONFIG | EISA_CONFIG_BRIEF;
			ic.cx = i; /* CL gets set to slot number */
			rv = doint(&ic);
			if (rv) {
#ifdef BOOT_DEBUG
				printf("	EISA config didn't find slot %d\n", i);
#endif
				continue;
			}
			if ((ic.dx & EISA_HAS_MEMS) == 0) {
#ifdef BOOT_DEBUG
				printf("	EISA slot %d didn't have mem entries\n", i);
#endif
				continue;
			}

			membuf = physaddr(&eisa);
			for (j = 0; j < (int)(ic.dx >> 8); j++) {
				ic.ds = (ushort) membuf >> 4;
				ic.si = (ushort) membuf & 0xf;
				ic.cx = (j << 8) | i;
				ic.intval = 0x15;
				ic.ax = EISA_CONFIG | EISA_CONFIG_EXTEN;
				if (rv = doint(&ic)) {
#ifdef BOOT_DEBUG
					printf("	EISA_CONFIG_EXTEN for slot %d failed\n",
					    i);
#endif
					continue;
				}

				if (!(eisa.flags & EISA_HAS_MEMS))
					continue;	/* try next function */

				emp = &eisa.memlist[0];
				do {
					if (memrngidx == (B_MAXARGS - 2))
						break;
					success = 0;
					k = (unsigned int) emp - (unsigned int) &eisa.memlist[0];
					k = k / sizeof(eisa_mem_info);
					if (k >= 9)
						break; /* break after checking 9 entries */
#ifdef BOOT_DEBUG
					printf("	slot %d function %d mementry %d\n",
					    i, j, k);
#endif
					if (emp->memflgs & EISA_MEM_WRITE) {
						success = 1;
#ifdef BOOT_DEBUG
						printf("	writable ");
#endif
					}
					if (emp->memflgs & EISA_MEM_CACHE) {
#ifdef BOOT_DEBUG
						printf("	cacheable ");
#endif
					}
					switch (emp->memflgs & EISA_MEM_TYPE) {

					case EISA_RESERVED_SYS: 
						 {
#ifdef BOOT_DEBUG
							printf("	reserved sys ");
#endif
							break;
						}
					case EISA_EXPANDED: 
						 {
#ifdef BOOT_DEBUG
							printf("	expanded memory ");
#endif
							success = 0; /* don't count this memory */
							break;
						}
					case EISA_VIRTUAL: 
						 {
#ifdef BOOT_DEBUG
							printf("	EISA virtual memory ");
#endif
							success = 0; /* don't count this memory */
							break;
						}
					case EISA_OTHER: 
						 {
#ifdef BOOT_DEBUG
							printf("	other memory ");
#endif
							success = 0; /* don't count this memory */
							break;
						}
					} /* switch */

					if (success) {
						k = bcombine(emp->startMED, emp->startLO);
						k = combine(emp->startHIGH, k);
						BOOTENV->memrng[memrngidx].base = k * 0x100;
						if (k != 0) {
							if (BOOTENV->memrng[memrngidx].base < MEM16M)
								BOOTENV->memrng[memrngidx].flags = 
								((BOOTENV->memrng[memrngidx].base < 0x100000) ?
									B_MEM_EXPANS : B_MEM_EXPANS | B_MEM_FORCE);
							else
								BOOTENV->memrng[memrngidx].flags = B_MEM_EXPANS;
						} else
							BOOTENV->memrng[memrngidx].flags = B_MEM_BASE;
						k = bcombine(emp->endHIGH, emp->endLO);

						if (k == 0) /* k is really 64MB */
							k = 64 * 0x400; /* 64MB in 1K chunks */

						BOOTENV->memrng[memrngidx].extent = k * 0x400;
						memrngidx++;
					} else {
						k = bcombine(emp->startMED, emp->startLO);
						k = combine(emp->startHIGH, k);
						k = bcombine(emp->endHIGH, emp->endLO);

						if (k == 0) /* k is really 64MB */
							k = 64 * 0x400; /* 64MB in 1K chunks */

					}

					if (!(emp->memflgs & EISA_MORE_LIST))
						break;
					emp++;
				} while (1); /* do */
			}
		}
		if( memrngidx != 0 )
			BOOTENV->memrngcnt = memrngidx;

	} /* otherwise, assume it's an ISA bus */

	for (rv = BOOTENV->memrngcnt - 1; rv > 0; rv--) {
		struct bootmem tbootmem;
		int	j;

		for (j = 0; j < rv; j++)
			if (BOOTENV->memrng[j].base > BOOTENV->memrng[j+1].base) {
				tbootmem = BOOTENV->memrng[j+1];
				BOOTENV->memrng[j+1] = BOOTENV->memrng[j];
				BOOTENV->memrng[j] = tbootmem;
			}
	}
#ifdef BOOT_DEBUG
	for (rv = 0; rv < BOOTENV->memrngcnt; rv++) {
		printf("sip_memrng %d, base %x, extent %x\n",
		    rv, BOOTENV->memrng[rv].base,
		    BOOTENV->memrng[rv].extent);
	}
	printf("sip_memconfig: Returning\n");
	goany();
#endif
}

pcichk()
{
	int i;
	char	*memsrt = (char *)BIOS_32_START;

	/* search for BIOS32 service directory id  */

	for (i = 0; i < BIOS_32_SIZE; i += 16)
		if ((memcmp(memsrt + i, BIOS_32_ID, 4) == 0) &&
			csum(memsrt + i) == 0)
			return 1;

	return 0;
}


/*
 * int
 * csum(char *)
 * 
 * Calling/Exit State:
 *	Find if the 11 bytes pointed at by p sum to 0. Necessary
 *	for determining presence of BIOS 32 service dir. Return
 *	the sum.
 */
int
csum(p)
char *p;
{
	/* inelegant way to compute checksum of BIOS 32 service directory */
	uchar_t sum;
	if (p == 0) return -1;
	sum = p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7] + p[8] +
		p[9] + p[10];	
	return (int) sum;
}
