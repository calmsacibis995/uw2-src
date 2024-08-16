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

#ident	"@(#)stand:i386at/standalone/boot/bootlib/getfhdr.c	1.3"
#ident  "$Header: $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/bootinfo.h>
#include <boothdr/libfm.h>
#include <boothdr/boot.h>
#include <boothdr/error.h>

Elf32_Ehdr	elfhdr;


/*
 *	high level interface routine for identifying ELF
 *	format files
 *	transfer header information into the boot file table
 */
getfhdr(lpcbp)
struct	lpcb	*lpcbp;
{
	struct	bftbl	*bftblp = &LP_BFTBL;
	int	status;
	ulong	actual;
	int	i, size;

	switch( lpcbp->lp_type ){
	case MEMFSROOT_FS: 
	case MEMFSROOT_META: 
	case RM_DATABASE: 
		/* start at the beginning of the file 	*/
		BL_file_lseek(0L, &status);
		size = BL_file_statsize( &status );
		if (status != E_OK) {
			printf("getfhdr: Cannot stat file.\n");
			return(FAILURE);
		}
		bftblp->t_nbph = 1;
		bftblp->t_entry = 0;
		LP_BFTBL.t_bph[0].p_type = DLOAD;
		LP_BFTBL.t_bph[0].p_filsz = size;
		LP_BFTBL.t_bph[0].p_memsz = size;
		LP_BFTBL.t_bph[0].p_vaddr = 0;
		LP_BFTBL.t_bph[0].p_offset = 0;
		break;
	default:
		/* start at the beginning of the file 	*/
		BL_file_lseek(0L, &status);
		BL_file_read( physaddr(&elfhdr),NULL,(long)sizeof(elfhdr),
			&actual,&status);
		if (status != E_OK) {
			printf("gethead: cannot read header.\n");
			return(FAILURE);
		}

		bftblp->t_type = ELF;
		bftblp->t_nsect = (int)elfhdr.e_phnum;
		bftblp->t_entry = (ulong)elfhdr.e_entry;
		bftblp->t_offset = (ulong)elfhdr.e_phoff;
		if ((status = elfseg(bftblp)) == SUCCESS)
			/* for kernel, only mark 1st segment as physical */
			for ( i=0; i<LP_BFTBL.t_nbph; i++)
				if ((LP_BFTBL.t_bph[i].p_type == TLOAD) ||
					(LP_BFTBL.t_bph[i].p_type == DLOAD)) {
					LP_BFTBL.t_bph[i].p_type = PLOAD;
					if(lpcbp->lp_type == KERNEL)
						break;
				}
#ifdef BOOT_DEBUG
printf("getfhdr: ELF nsect= 0x%x entry= 0x%x offset= 0x%x\n",
bftblp->t_nsect, bftblp->t_entry, bftblp->t_offset); 	
#endif
		break;
	}
	return(status);
}


/*
 *	ELF file header handler
 */
elfseg(bftblp)
struct	bftbl	*bftblp;
{
	Elf32_Phdr elfphdr;
	int	i,j;
	ulong	scnsz;
	int	status;
	ulong	actual;

	scnsz = sizeof(Elf32_Phdr);

	BL_file_lseek(bftblp->t_offset,&status);
	for (i=0, j=0; i<bftblp->t_nsect; i++) {
        	BL_file_read(physaddr(&elfphdr),NULL,scnsz,&actual,&status);
        	if (status != E_OK) {
                	printf("elfseg: cannot read ELF program header.\n");
			return(FAILURE);
		}
		if (j >= NBPH) {
			printf("elfseg: reserved boot program header space overflow.\n");
			return(FAILURE);
		}
/*		fill in program header information			*/
		switch(elfphdr.p_type) {
			case PT_LOAD:
				switch(elfphdr.p_flags & (PF_R|PF_W|PF_X)) {
					case (PF_R|PF_W|PF_X):
						bftblp->t_bph[j].p_type= DLOAD;
						break;
					case (PF_R|PF_X):
						bftblp->t_bph[j].p_type= TLOAD;
						break;
					default:
						bftblp->t_bph[j].p_type= NOLOAD;
				}
				break;
			case PT_NOTE:
				bftblp->t_bph[j].p_type = BKI;
				break;
			default:
				bftblp->t_bph[j].p_type = NOLOAD;
		}
		if (bftblp->t_bph[j].p_type == NOLOAD)
			continue;
		bftblp->t_bph[j].p_vaddr = elfphdr.p_vaddr;
		bftblp->t_bph[j].p_paddr = elfphdr.p_paddr;
		bftblp->t_bph[j].p_filsz = elfphdr.p_filesz;
		bftblp->t_bph[j].p_memsz = elfphdr.p_memsz;
		bftblp->t_bph[j].p_offset = elfphdr.p_offset;
/*		check for BSS segment					*/
		if (elfphdr.p_filesz == 0)
			bftblp->t_bph[j].p_type= BLOAD;
		j++;
	}
	bftblp->t_nbph = j;
	return(SUCCESS);
}
