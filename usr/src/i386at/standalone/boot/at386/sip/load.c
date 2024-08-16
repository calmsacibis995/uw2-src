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

#ident	"@(#)stand:i386at/standalone/boot/at386/sip/load.c	1.6"
#ident  "$Header: $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/vtoc.h>
#include <sys/sysmacros.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/bootcntl.h>
#include <boothdr/bootlink.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/error.h>


/* standard error message; the 0L is the 'error' return code */
#define ERROR(msg, arg) (printf("\n%s %s\n", msg, arg), FAILURE)

/* occasionally check for keyboard input to abort boot process */
#define CHECK_KBD	if (  checkkbd() ) return( FAILURE ) 


/*	KERNEL loader - Loads the unix file whose path is in path.
 *		returns a FAILURE load address in case of error.
 *
 *		memory as defined in the memavail array in the bootinfo
 *		structure marking used areas in the memused array.
 *
 *
 *	Return the loaded module start address or:
 *		0 - if the load interrupted by a keystroke(kernel load only).
 *		  - if the file could not be found.
 *		  - if a load error occured.
 */


int	phyused;
int	curseg;
int	krdata_cnt;
char	*path;
paddr_t	kstart;
paddr_t	loadaddr;
paddr_t	endseg;

unsigned long
bload(lpcbp)
struct	lpcb	*lpcbp; 
{
	paddr_t	addr;
	int	status;
	int	foundtd, omemusedcnt;
	int	n, i, j;
	ushort_t mflgs = 0;


#ifdef LOAD_DEBUG
printf("bload: %s type-- 0x%x\n",lpcbp->lp_path, lpcbp->lp_type);
#endif
	path = lpcbp->lp_path;
	/* open the file */
	BL_file_open(path, &status);
	if (status == E_FNEXIST)
		return(ERROR("Error opening", path));

	/* get program header information from file */
	if ((status = getfhdr( lpcbp )) == FAILURE)
		return(ERROR("Bad file header in ",path));

	switch ( lpcbp->lp_type ){
	case KERNEL:	
		/* 
		 * loading UNIX file;
		 * Load the kernel physical segment first
		 * then load the remaining kernel segments 
		 */
		kstart = -1;
		phyused = 0;
		krdata_cnt = 0;
		curseg = 1;
		BTE_INFO.memusedcnt = 1;
		loadaddr = BTE_INFO.memavail[1].base; 
		endseg = loadaddr + BTE_INFO.memavail[1].extent;
		foundtd = load_segments( lpcbp );
		break;
	case MEMFSROOT_FS: 
		mflgs = B_MEM_MEMFSROOT;
	case MEMFSROOT_META: 
	case RM_DATABASE: 
		mflgs |= B_MEM_KRDATA;
		/* loading kernel data: memfsroot, RM database etc.*/
		lpcbp->lp_memsrt = 0;
		omemusedcnt = BTE_INFO.memusedcnt;
		if( (foundtd = load_segments( lpcbp )) == SUCCESS){
			for (;omemusedcnt <  BTE_INFO.memusedcnt;omemusedcnt++)
				BTE_INFO.memused[omemusedcnt].flags |= mflgs;
			BTE_INFO.kd[krdata_cnt].paddr = lpcbp->lp_memsrt;
			BTE_INFO.kd[krdata_cnt].type = lpcbp->lp_type;
			BTE_INFO.kd[krdata_cnt++].size = LP_BFTBL.t_bph[0].p_memsz;
		};
		break;
	}


	BL_file_close(&status);

	return(foundtd);
}

int
load_segments(lpcbp)
struct	lpcb	*lpcbp; 
{
	register long size;
	ulong	actual;
	int	status;
	int	foundtd = FAILURE;
	int	i,j;

	CHECK_KBD;

	/* load the program in sections/segments */
	for ( i = 0 ; i < LP_BFTBL.t_nbph; ) {

		CHECK_KBD;
#ifdef LOAD_DEBUG
printf("load_segments: section 0x%x\n",i);
goany();
#endif

		/* skip non-loadable segment types */
		if (LP_BFTBL.t_bph[i].p_type == BLOAD ){
			i++;
			continue;
		}
		if (LP_BFTBL.t_bph[i].p_type == BKI ){
			if (check_bki(&LP_BFTBL.t_bph[i]) != SUCCESS)
				return(FAILURE);

			i++;
			continue;
		}

		/* 	TEXT or DATA:
		 * 	find memory where this segment is supposed to live, 
		 * 	and see if it fits...
		 */

		if (LP_BFTBL.t_bph[i].p_type == PLOAD )
			if (load_phys(lpcbp, &LP_BFTBL.t_bph[i]) == SUCCESS){
				foundtd = SUCCESS;
				/* Check if start address is in this section */
				if (INTERVAL(LP_BFTBL.t_bph[i].p_vaddr, 
					LP_BFTBL.t_bph[i].p_filsz, LP_BFTBL.t_entry))
					kstart = LP_BFTBL.t_entry;
				i++;
				continue;
			}else
				break;

		foundtd = SUCCESS;

		/*
		 * check room for current memory available.
		 */
		while ((size = (endseg - loadaddr)) <= 0) {
			if (loadaddr == BTE_INFO.memused[phyused].base){
				loadaddr +=
				     BTE_INFO.memused[phyused].extent;

				endseg = BTE_INFO.memavail[curseg].base
				     + BTE_INFO.memavail[curseg].extent;

				continue;
			}

			/*
			 * skipping memory reserved by boot prog
			 */
			for (curseg++; (curseg<BTE_INFO.memavailcnt)
			     && (BTE_INFO.memavail[curseg].flags &
			     B_MEM_BOOTSTRAP); curseg++)
				;

			if (curseg >= BTE_INFO.memavailcnt)
				return (ERROR("Insufficient memory for load of", path));
			loadaddr = BTE_INFO.memavail[curseg].base;
			endseg = loadaddr +
				       BTE_INFO.memavail[curseg].extent;

			if (loadaddr == BTE_INFO.memused[phyused].base)
				loadaddr += 
				       BTE_INFO.memused[phyused].extent;
			else
				if (INTERVAL(loadaddr,
				    BTE_INFO.memavail[curseg].extent,
				    BTE_INFO.memused[phyused].base))
					endseg = BTE_INFO.memused[phyused].base;
		}
		if (size > LP_BFTBL.t_bph[i].p_filsz)
			size = LP_BFTBL.t_bph[i].p_filsz;


		BL_file_lseek(LP_BFTBL.t_bph[i].p_offset, &status);
		BL_file_read(loadaddr,NULL,size,&actual,&status);
		if (status != E_OK)
			return (ERROR("Detected error reading ", path));
		if (lpcbp->lp_memsrt == 0) 
			lpcbp->lp_memsrt = loadaddr;

#ifdef LOAD_DEBUG
printf("loaded section[%d] at %lx, extent %lx\n", i, loadaddr, size);
printf("data: 0x%x --  0x%x\n", i, loadaddr, loadaddr+size-2);
#endif

		/* Check if start address is in this section */
		if (INTERVAL(LP_BFTBL.t_bph[i].p_vaddr, size, LP_BFTBL.t_entry))
			kstart = LP_BFTBL.t_entry - LP_BFTBL.t_bph[i].p_vaddr +
				  loadaddr;

#ifdef LOAD_DEBUG
printf("bload: cnt= 0x%x loadaddr= 0x%x\n", BTE_INFO.memusedcnt, loadaddr);
#endif
		BTE_INFO.memused[BTE_INFO.memusedcnt].base = loadaddr;
		BTE_INFO.memused[BTE_INFO.memusedcnt].extent =
					ptob(btopr(size));
		BTE_INFO.memused[BTE_INFO.memusedcnt++].flags = 
			BTE_INFO.memavail[curseg].flags |
			((LP_BFTBL.t_bph[i].p_type == TLOAD) ?
				B_MEM_KTEXT : B_MEM_KDATA);

		loadaddr = ptob(btopr(loadaddr + size));

/*		filsz will drop to zero after program loading		*/
		if ((LP_BFTBL.t_bph[i].p_filsz -= size) == 0)
			i++;
		else {
			LP_BFTBL.t_bph[i].p_vaddr += size;
			LP_BFTBL.t_bph[i].p_offset += size;
		}
#ifdef LOAD_DEBUG
goany();
#endif
	}
	if ( foundtd == SUCCESS ){
		/* return start physical address for the binary */
		lpcbp->lp_entry = kstart;	
#ifdef LOAD_DEBUG
printf("ent-point=0x%lx phys 0x%lx\n",LP_BFTBL.t_entry,kstart);
#endif
		return ( SUCCESS );
	}else
		return (ERROR("Missing text or data in ", path));



}

/*
 * handle absolute addressed kernel segment
 */
int
load_phys( lpcbp, bphp)
struct lpcb	*lpcbp;
struct bootproghdr	*bphp;
{
	ulong	actual;
	int	status;
	long	size;
	paddr_t	physaddr;
	int n;

#ifdef LOAD_DEBUG
printf("load_phys: type: 0x%x\n",lpcbp->lp_type);
#endif
	for (n = 0; n < BTE_INFO.memavailcnt; n++) {
		if (BTE_INFO.memavail[n].flags & B_MEM_BOOTSTRAP)
			continue;

		if (INTERVAL(BTE_INFO.memavail[n].base,
		    BTE_INFO.memavail[n].extent, bphp->p_paddr)) {
			physaddr = bphp->p_paddr;
			break;
		}
	}
	if (physaddr != bphp->p_paddr || (bphp->p_filsz > 
	    (BTE_INFO.memavail[n].base+BTE_INFO.memavail[n].extent-physaddr)))
		return(ERROR("Requested physical memory unavailable for", path));
	phyused = BTE_INFO.memusedcnt;
	BTE_INFO.memused[BTE_INFO.memusedcnt].base=physaddr;
	BTE_INFO.memused[BTE_INFO.memusedcnt].extent=ptob(btopr(bphp->p_filsz));
	BTE_INFO.memused[BTE_INFO.memusedcnt++].flags = 
		BTE_INFO.memavail[curseg].flags |
		((bphp->p_type == TLOAD) ? B_MEM_KTEXT : B_MEM_KDATA);
	if (loadaddr == BTE_INFO.memused[phyused].base)
		loadaddr += BTE_INFO.memused[phyused].extent;
	else
		if (INTERVAL(BTE_INFO.memavail[curseg].base,
			     BTE_INFO.memavail[curseg].extent,
			     BTE_INFO.memused[phyused].base))
			endseg = BTE_INFO.memused[phyused].base;
	size = bphp->p_filsz;
	BL_file_lseek(bphp->p_offset, &status);
	BL_file_read(physaddr,NULL,size,&actual,&status);
	if (status != E_OK)
		return (ERROR("Detected error reading ", path));
	if (lpcbp->lp_memsrt == 0) 
		lpcbp->lp_memsrt = loadaddr;

#ifdef LOAD_DEBUG
printf("load_phys: cnt= 0x%x physaddr= 0x%x\n", BTE_INFO.memusedcnt, physaddr);
#endif

	return( SUCCESS );

}

/* 
 * check BKI (Boot Kernel Interface) version 
 */
int
check_bki(bphp)
struct bootproghdr	*bphp;
{
	int 	foundbki = FAILURE;
	ulong	actual;
	int	status;
	short	bkiinfo = -1;

	BL_file_lseek(bphp->p_offset, &status);
	BL_file_read(physaddr(&bkiinfo),NULL,2L,&actual,&status);
	if (status != E_OK){
		return (ERROR("Cannot read BKI section in ", path));
	}else
		if (bkiinfo == BKIVERSION) 
			foundbki = SUCCESS;

	return( foundbki == SUCCESS ? SUCCESS : ERROR("Missing or bad BKI segment in ", path));
}
