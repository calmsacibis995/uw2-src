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

#ident	"@(#)stand:i386at/standalone/boot/at386/load.c	1.3.1.6"
#ident  "$Header: $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/vtoc.h>
#include <sys/sysmacros.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/bootcntl.h>
#include <boothdr/libfm.h>
#include <boothdr/error.h>


/* standard error message; the 0L is the 'error' return code */
#define ERROR(msg, arg) (printf("\n%s %s\n", msg, arg), FAILURE)

/* occasionally check for keyboard input to abort boot process */
#define CHECK_KBD

/* The stand-alone loader; returns a FAILURE load address in case of error */

/*	Boot loader - Loads the file whose path is in path.
 *
 *	If KERNEL, load the kernel using path into available
 *		memory as defined in the memavail array in the bootinfo
 *		structure marking used areas in the memused array.
 *
 *	else load the file for a non-virtual module at the address
 *		specified in the file but not below the address in
 *		loadaddr.
 *
 *	Return the loaded module start address or:
 *		0 - if the load interrupted by a keystroke(kernel load only).
 *		  - if the file could not be found.
 *		  - if a load error occured.
 */

extern	paddr_t secboot_mem_loc;

struct	lpcb	lpcb[NBT_LOADABLES+1];

int	phyused;
int	curseg;
int	krdata_cnt = 0;
char	*path;
paddr_t	kstart;
paddr_t	loadaddr;
paddr_t	endseg;

void
btload_init()
{
	struct	bootcntl *btcntlp;

	btcntlp = (struct bootcntl *)secboot_mem_loc;
	/*
	 * initialize the loadable program control block
	 */

	lpcb[MEMFSROOT_FS].lp_path = "memfs.fs";
	lpcb[MEMFSROOT_FS].lp_type = MEMFSROOT_FS;
	lpcb[MEMFSROOT_META].lp_path = "memfs.meta";
	lpcb[MEMFSROOT_META].lp_type = MEMFSROOT_META;
	lpcb[RM_DATABASE].lp_path = btcntlp->rmdatabase;
	lpcb[RM_DATABASE].lp_type = RM_DATABASE;
	lpcb[SIP].lp_path = btcntlp->sip;
	lpcb[SIP].lp_type = SIP;
	lpcb[MIP].lp_path = btcntlp->mip;
	lpcb[MIP].lp_type = MIP;
	lpcb[DCMP].lp_path = btcntlp->dcmp;
	lpcb[DCMP].lp_type = DCMP;
	lpcb[KERNEL].lp_path = btcntlp->bootstring;
	lpcb[KERNEL].lp_type = KERNEL;

}

unsigned long
btload(lpcbp)
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
		return(FAILURE);

	/* get program header information from file */
	if ((status = getfhdr( lpcbp )) == FAILURE)
		return(ERROR("Bad file header in ",path));

	/*
	 * load the initialization program segments :
	 * dcmp, sip, mip files.
	 */
	loadaddr = BOOTENV->bootsize;
	if ((foundtd = btload_segments( lpcbp )) == SUCCESS){
		for (i=0, j=0, addr = 0; i<LP_BFTBL.t_nbph; i++) {
			if (addr < LP_BFTBL.t_bph[i].p_vaddr) {
				addr = LP_BFTBL.t_bph[i].p_vaddr;
				j=i;
			}
		}
		addr = LP_BFTBL.t_bph[j].p_vaddr +
				   LP_BFTBL.t_bph[j].p_memsz;
		BOOTENV->bootsize = (paddr_t) ptob(btopr(addr));
	}

	CHECK_KBD;

	BL_file_close(&status);

	return(foundtd);
}

int
btload_segments(lpcbp)
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
printf("btload_segments: section 0x%x\n",i);
goany();
#endif

		/* skip non-loadable segment types */
		if (LP_BFTBL.t_bph[i].p_type == BLOAD ){
			i++;
			continue;
		}

		/* 	TEXT or DATA:
		 * 	find memory where this segment is supposed to live, 
		 * 	and see if it fits...
		 */

		if (LP_BFTBL.t_bph[i].p_type == PLOAD )
			if (btload_phys(lpcbp, &LP_BFTBL.t_bph[i]) == SUCCESS){
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
 * handle absolute addressed segment
 */
int
btload_phys( lpcbp, bphp)
struct lpcb	*lpcbp;
struct bootproghdr	*bphp;
{
	ulong	actual;
	int	status;
	long	size;
	paddr_t	physaddr;
	int n;

#ifdef LOAD_DEBUG
printf("btload_phys: type: 0x%x\n",lpcbp->lp_type);
#endif
	/* loading bootstrap init program segments */
	if ( bphp->p_paddr < BOOTENV->bootsize)
		return(ERROR("Bad load address in file", path));
	physaddr = bphp->p_paddr;
	BOOTENV->bootsize += bphp->p_memsz;
	size = bphp->p_filsz;
	BL_file_lseek(bphp->p_offset, &status);
	BL_file_read(physaddr,NULL,size,&actual,&status);
	if (status != E_OK)
		return (ERROR("Detected error reading ", path));
	if (lpcbp->lp_memsrt == 0) 
		lpcbp->lp_memsrt = loadaddr;

#ifdef LOAD_DEBUG
printf("btload_phys: cnt= 0x%x physaddr= 0x%x\n", BTE_INFO.memusedcnt, physaddr);
#endif

	return( SUCCESS );

}
