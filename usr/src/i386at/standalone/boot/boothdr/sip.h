/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _BOOT_SIP_H	/* wrapper symbol for kernel use */
#define _BOOT_SIP_H	/* subject to change without notice */

#ident	"@(#)stand:i386at/standalone/boot/boothdr/sip.h	1.2"
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

/*
 * The following defines are added for EISA.  These values are
 * based on EISA spec 3.12
 * We're going to use EISA call int 0x15 with AH set to 0xd8
 * and CL to the slot number.  Config summary info is obtained
 * by setting AL to 0 (specifying "real" mode).  A 32-bit
 * protected mode option can be used by setting AL to 0x80.
 * The extended config info is selected by * setting AL to 1
 * (specifying 16-bit addressing (i.e. "real") mode,
 * and setting DS:SI to a 310-byte data area.  If making the BIOS
 * call in protected mode, AL would be set to 0x81.
 */

#define	EISA_NSLOTS		20	/* only check 20 slots */

#define	EISA_CONFIG		0xd800	/* AH value is 0xd8 */
#define	EISA_CONFIG_BRIEF	0x00	/* brief summary for slot */
#define	EISA_CONFIG_EXTEN	0x01	/* extended for slot/func */

#define	EISA_SLOT_INVAL		0x80	/* AH return vals on error */
#define	EISA_FUNC_INVAL		0x81
#define	EISA_MEM_CORRUPT	0x82
#define	EISA_SLOT_EMPTY		0x83 	/* empty slot */
#define	EISA_INVAL_BIOS		0x86	/* invalid BIOS call */
#define	EISA_CFG_INVAL		0x87 	/* invalid config */

/* Bit masks defining DL register contents upon return from
 * EISA_CONFIG_BRIEF BIOS call.  Only one of interest for
 * the moment is whether the slot defines any memory entries.
 */

#define	EISA_HAS_MEMS		2

/* Bit masks defining memory info in EISA config structure below */
#define	EISA_MEM_WRITE		0x0001	/* bit 0 for writeable */
#define	EISA_MEM_CACHE		0x0002	/* bit 1 for cacheable */
#define	EISA_WRITE_BACK		0x0004	/* bit 2 for cache policy */
#define	EISA_MEM_TYPE		0x00018	/* bits 3-4 for mem type */
#define	EISA_MEM_SHARE		0x00020	/* bits 5 for shared memory flag */
#define	EISA_MORE_LIST		0x00080 /* bit 7 for more-of-list indicator */
/* if this is set, then memory continues */ 

/* memory types masked out by EISA_MEM_TYPE above */
#define	EISA_RESERVED_SYS	0
#define	EISA_EXPANDED		0x08
#define	EISA_VIRTUAL		0x10
#define	EISA_OTHER		0x18

/* the config structure returned by the EXTENDED BIOS call contains
 * an array of structures describing memory usage on the slot.
 * There are up to 9, 7 bytes each, in this format
 */
typedef struct {
	unsigned char memflgs;		/* see bit masks above */
	unsigned char sizeflags;	/* type of access -- unimportant */
	unsigned char startLO;
	unsigned char startMED;
	unsigned char startHIGH;	/* start address divided by 0x100 */
	unsigned char endLO;
	unsigned char endHIGH;		/* size in bytes divided by 0x400.
					* A 0 value means a 64MB size.
					*/
} eisa_mem_info;

/*
 * this is the layout of the structure filled in by the EXTENDED
 * CONFIG bios call.
 */

typedef struct {
	char pad1[0x22];
	unsigned char flags;
	char pad2[0x50];
	eisa_mem_info  memlist[9];
	char pad3[156];			/* total structure size is 320 bytes */
} eisa_config;


/*
 * The following defines are added for MCA.  
 * We're going to use int 0x15 with AH set to 0xc7 and
 * DS:SI pointing to the structure below.
 */

#define	MCA_MEM_CONFIG	0xc700


typedef struct {
	ushort dsize;		/* structure size in bytes excluding this short */
	ushort locLO;		/* lsb of local mem between 1MB and 16MB in 1K */
	ushort locHI;		/* msb of local mem between 1MB and 16MB in 1K */
	ushort loc4GBLO;	/* lsb of local mem between 16MB and 4GB in 1K */
	ushort loc4GBHI;	/* msb of local mem between 16MB and 4GB in 1K */
	ushort sysLO;		/* lsb of system mem between 1MB and 16MB  in 1K */
	ushort sysHI;		/* msb of system mem between 1MB and 16MB  in 1K */
	ushort sys4GBLO;	/* lsb of system mem between 16MB and 4GB  in 1K */
	ushort sys4GBHI;	/* msb of system mem between 16MB and 4GB  in 1K */
	ushort cacheLO;		/* lsb of cacheable mem between 1MB and 16MB  in 1K */
	ushort cacheHI;		/* msb of cacheable mem between 1MB and 16MB  in 1K */
	ushort cache4GBLO;	/* lsb of cacheable mem between 16MB and 4GB in 1K */
	ushort cache4GBHI;	/* msb of cacheable mem between 16MB and 4GB in 1K */
	ushort startLO;		/* lsb of mem before non-sys mem between 1MB and 16MB in 1K */
	ushort startHI;		/* msb of mem before non-sys mem between 1MB and 16MB in 1K */
	ushort start4GBLO;	/* lsb of mem before non-sys mem between 16MB and 4GB in 1K */
	ushort start4GBHI;	/* msb of mem before non-sys mem between 16MB and 4GB in 1K */
	ushort pad[32] ;	/* pad */
} mca_config;


/*
 * The macro combine (bcombine) is used to combine words (bytes)
 * into a double-word (word) value.
 */
#define combine(x, y)  (((x)<<16) | (y))
#define bcombine(x, y)  (((x)<<8) | (y))

#endif	/* _BOOT_SIP_H */
