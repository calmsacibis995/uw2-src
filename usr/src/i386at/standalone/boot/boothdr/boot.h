/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _BOOT_H
#define _BOOT_H

#ident	"@(#)stand:i386at/standalone/boot/boothdr/boot.h	1.2.2.11"
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

#ifndef _BOOT_BOOTDEF_H
#include <boothdr/bootdef.h>
#include <boothdr/libfm.h>
#endif	/* _BOOT_BOOTDEF_H */

/* definitions for generic AT(386) hard/floppy disk boot code */

#ifdef BOOTDEBUG
#define debug(x)	x
#else
#define debug(x)	/*x/**/
#endif

#define TRUE	1
#define FALSE	0

#define NULL	0

#define HINBL	(unchar)0xF0

/* Definitions for micro-channel HD type, Should be in cram.h */

#define MC_FD0TB	0x11	/* Drive 0 type */
#define MC_FD1TB	0x12	/* Drive 1 type */

/*	Definitions for key BIOS ram loactions */

#define FDBvect	*(ulong *)FDB_ADDR	/* Floppy base parameter table ptr */
#define HD0p	(caddr_t *)HD0V_ADDR	/* hard disk 0 parameter table ptr */
#define HD1p	(caddr_t *)HD1V_ADDR	/* hard disk 1 parameter table pte */
#define NUMHD()	*(short *)NUMHD_ADDR	/* number of HD drives 		   */
#define MEM_BASE() *(short *)MEMBASE_ADDR	/* Base memory size 	   */
#define COMM_B(x)  *(short *)(COMM_ADDR + (x-1))/* base addr for com port  */
#define LPT_B(x)   *(short *)(LPT_ADDR + (x-1)) /* base addr for lpt port  */

#define segoftop(s,o)	(paddr_t)( (uint)(s<<4) + o)

/*	To read a word(short) from CMOS */

#define CMOSreadwd(x)	(ushort) ((CMOSread(x+1) << 8)+CMOSread(x))

#define INTERVAL(b, e, p)     ((e) > 0 ? (((p) >= (b)) && ((p) < (b) + (e))) : \
					 (((p) < (b)) && ((p) >= (b) + (e))))


#define physaddr(x)	(paddr_t)(x)

#pragma	pack(1)

struct	fdpt	{		/* floppy disk parameter table entry */
	unsigned char	step;
	unsigned char	load;
	unsigned char	motor;
	unsigned char	secsiz;
	unsigned char	spt;
	unsigned char	dgap;
	unsigned char	dtl;
	unsigned char	fgap;
	unsigned char	fill;
	unsigned char	headsetl;
	unsigned char	motrsetl;
	unsigned char	mxtrk;
	unsigned char	dtr;
};
struct	hdpt	{
	unsigned short	mxcyl;
	unsigned char	mxhead;
	unsigned char	dmy1[2];
	unsigned short	precomp;
	unsigned char	mxecc;
	unsigned char	drvcntl;
	unsigned char	dmy2[3];
	unsigned short	lz;
	unsigned char	spt;
	unsigned char	dmy3;
};
#pragma pack()
struct bootenv {
	struct	bootinfo bootinfo;	/* boot information		*/
	struct  memconfig {		/* memory configuration		*/
		ushort	CMOSmembase;	/* Base mem from CMOS(0x15,16) in KB  */
		ushort	CMOSmemext;	/* Exted mem from CMOS(0x17,18) in KB */
		ushort	CMOSmem_hi;	/* Mem > 1MB from CMOS(0x30,31) in KB */
		ushort	base_mem;	/* Base memory size from 0040:0048    */
		ushort	sys_mem;	/* system mem size from int15 ah=0x88 */
	} memconfig;
	ushort_t db_flag;		/* debug control flags		*/
	ushort_t be_flags;		/* bootstrap internal flags	*/
	int	memrngcnt;		/* default memory sections	*/
	struct	bootmem memrng[B_MAXMEMA];
	long	bootsize;		/* top memory loc. of boot prog */
	paddr_t bf_resv_base;		/* base of memory reserved boot */
	int	memrng_updated;		/* MEMRANGE statement processed */
};

#define BOOTENV		((struct bootenv *) BOOTINFO_LOC)

#define BTE_INFO	(BOOTENV->bootinfo)

/* Flag definitions for be_flags */

#define BE_MEMAVAILSET	0x01		/* Mem available array is set 	*/
#define BE_BIOSRAM	0x02		/* restore ram used by BIOS	*/
#define BE_16MWRAPSET	0x04		/* above 16M ram wrap test done	*/
#define BE_NOINVD	0x08		/* suppress cache invalidation  */

/* Flag definitions for db_flag */

#define BOOTDBG		0x01		/* F2 basic boot debug messages */
#define MEMDEBUG	0x02		/* F3 memory debug messages 	*/
#define LOADDBG		0x04		/* F4 loader debug messages 	*/
#define ENVDBG		0x08		/* F5 environment debug messages */
#define BOOTTALK	0x10		/* F10 make boot process verbose */

/* static ram memory values ... */
#define	MALLOC_BASE_ADDR	0x11000


extern int	bprintf(), bputchar(), bstrlen(), bmemcmp(), 
		bgets(), ischar(), doint(), shomem(), no_op();

extern unsigned int	bt_malloc();

extern int	logo_up;
extern unsigned short	hdisk_id;

extern	char *bmemcpy(), *bmemset(), *bstrcat(), *bstrcpy(), *bstrncpy(), *bstrtok();

extern	unchar	bgetchar(), CMOSread();

extern void	BL_file_init(), BL_file_open(), BL_file_read(), 
	BL_file_close(), BL_file_lseek();

extern void bootabort();

extern	off_t	boot_delta;
extern	off_t	root_delta;

extern	bfstyp_t boot_fs_type;
extern	bfstyp_t root_fs_type;

extern unsigned char	*gbuf;	/* global input buffer */

#define strncmp	bstrncmp
#define strpbrk	bstrpbrk
#define strspn	bstrspn
#define strtok	bstrtok
#define strcat	bstrcat
#define memcmp	bmemcmp
#define gets	bgets
#define getchar	bgetchar
#define putchar	bputchar
#define memcpy	bmemcpy
#define printf	bprintf
#define memset	bmemset
#define read	bread
#define malloc	bmalloc
#endif	/* _BOOT_BOOT_H */
