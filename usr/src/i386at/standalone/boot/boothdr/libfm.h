/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _LIBFM_H
#define _LIBFM_H

#ident	"@(#)stand:i386at/standalone/boot/boothdr/libfm.h	1.1.1.5"
#ident  "$Header: $"

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
 /* "(c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990" */

#ifndef _UTIL_TYPES_H
#include <sys/types.h>		/* REQUIRED */
#endif	/* _UTIL_TYPES_H */

#ifndef _PROC_OBJ_ELF_H
#include <sys/elf.h>
#endif	/* _PROC_OBJ_ELF_H */

#include <filehdr.h>

#define BKINAME		('B' + ('K' << 8) + ('I' << 16))
#define BKIVERSION	16		

enum bfstyp { s5, BFS, UNKNOWN };
typedef enum bfstyp bfstyp_t;

#define DFL_BFS	BFS


enum lfsect { TLOAD, DLOAD, NOLOAD, BKI, BLOAD, PLOAD};
typedef enum lfsect lfsect_t;

/*  common program header for ELF 					*/
struct bootproghdr {
	lfsect_t p_type;	/* type of section 			*/
	ulong p_vaddr;		/* virtual address to load section 	*/
	ulong p_paddr;		/* physical address to load section 	*/
	ulong p_memsz;		/* memory size of section 		*/
	ulong p_filsz;		/* file size of section 		*/
	off_t p_offset;		/* offset in file 			*/
};


enum lfhdr {ELF, NONE};
typedef enum lfhdr lfhdr_t;
				/* max boot program headers		*/
#define NBPH	6		/* phys, phystokv, text, data, bss, bki	*/
struct bftbl {
	lfhdr_t	t_type;		/* type of file 			*/
	int 	t_nsect;	/* number of sections or segments 	*/
	ulong 	t_entry;	/* entry point virtual 			*/
	ulong	t_offset;	/* file offset to program headers	*/
	int	t_nbph;		/* number of boot program headers	*/
	struct	bootproghdr t_bph[NBPH]; /* boot program header		*/
};

/*	
 *	loadable file types	
 *
 *	bootinfo.h defines:
 *
 *	 MEMFSROOT_META	0
 *	 MEMFSROOT_FS	1
 *	 RM_DATABASE	2
 */
#define	SIP	3
#define MIP	4
#define KERNEL	5
#define DCMP	6
#define IMAGE	7
#define BOOT	8	

#define	NPDATA	11
/*	loadable program control block					*/
struct lpcb {
	int 	lp_type;	/* type of file 			*/
	ulong	lp_flag;	/* status flag				*/
	ulong	lp_entry;	/* entry point - physical		*/
	paddr_t	lp_memsrt;	/* starting memory address - physical	*/
	paddr_t	lp_memend;	/* ending memory address - physical	*/
	char	*lp_path;	/* program path name			*/
	struct	bftbl lp_bftbl; /* boot file table			*/
	char	*lp_pptr;	/* program pointer			*/
	ulong	lp_pdata[NPDATA];/* program data area			*/
};
#define	LP_BFTBL	(lpcbp->lp_bftbl)	

/*	command flag	- SIP						*/
#define	SIP_INIT	0x1	/* collect system and BIOS parameters	*/
#define SIP_KPREP	0x2	/* prepare system for kernel loading	*/
#define SIP_KSTART	0x3	/* startup kernel			*/
#define	SIP_DCMP_INIT	0x4	/* enable decompression support */
#define SIP_KLOAD	0x5	/* stand/unix load request	*/

/*	command flag	- MIP						*/
#define MIP_INIT	0x1	/* identify machine & startup machine	*/
#define MIP_END		0x2	/* complete final machine startup	*/

/*	global buffer cache 						*/
struct gcache {
	daddr_t	gc_bno;
	int	gc_cnt;
};

#endif	/* _BOOT_LIBFM_H */
