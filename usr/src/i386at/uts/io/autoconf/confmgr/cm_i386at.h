/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_AUTOCONF_CONFMGR_CM_I386AT_H /* wrapper symbol for kernel use */
#define _IO_AUTOCONF_CONFMGR_CM_I386AT_H /* subject to change without notice */

#ident	"@(#)kern-i386at:io/autoconf/confmgr/cm_i386at.h	1.24"
#ident	"$Header: $"

#if defined ( __cplusplus )
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/autoconf/resmgr/resmgr.h>	/* REQUIRED */

#else

#include <sys/resmgr.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* The following BRDINST parameters are based on current sdevice fields */

#define CM_UNIT		"UNIT"		/* Unit Field */
#define CM_IPL		"IPL"		/* Interrupt Priority Level */
#define CM_ITYPE	"ITYPE"		/* Interrupt Type */
#define CM_IRQ		"IRQ"		/* Interrupt Vector */
#define CM_IOADDR	"IOADDR"	/* I/O Address Range */
#define CM_MEMADDR	"MEMADDR"	/* Memory Address Range */
#define CM_DMAC		"DMAC"		/* DMA Channel */
#define CM_BINDCPU	"BINDCPU"	/* Explicit CPU binding, if any;
					** -1 or not present means unbound
					*/

/* Symbolic values for ITYPE parameter */

#define CM_ITYPE_EDGE	0x01
#define CM_ITYPE_LEVEL	0x04

/* Other needed BRDINST parameters */

#define CM_BRDBUSTYPE	"BRDBUSTYPE"	/* What type of board, ISA, EISA ... */
#define CM_BRDID	"BRDID"		/* Board ID from NVRAM */
#define CM_CA_DEVCONFIG	"CA_DEVCONFIG"	/* Magic cookie we pass to ca_ funcs*/
#define CM_SLOT		"SLOT"		/* Slot the board is in */
#define CM_DCU_MODE	"DCU_MODE"	/* How should DCU run ?? */
#define CM_BOOTHBA	"BOOTHBA"	/* Used to "mark" boot hba entry */
#define CM_CLAIM	"CLAIM"		/* Bitmask of parameters to claim
					** when using cm_AT_putconf
					*/

/* Symbolic Bus types for CM_BRDBUSTYPE */

#define CM_BUS_UNK	0x00
#define CM_BUS_ISA	0x01
#define CM_BUS_EISA	0x02
#define CM_BUS_PCI	0x04
#define CM_BUS_PCMCIA	0x08
#define CM_BUS_PNPISA	0x10
#define CM_BUS_MCA	0x20
#define CM_BUS_SYS	0x40		/* System devices: serial, parallel.. */

/* Symbolic types for CM_DCU_MODE */

#define CM_DCU_NOCHG		1	/* NO relevant changes made to resmgr */
#define CM_DCU_SILENT		2	/* run DCU in silent mode */
#define CM_DCU_INTERACTIVE	3	/* Unconditionally run interactively */

/* Parameter setmask values for cm_AT_putconf() */

#define CM_SET_IRQ	0x01
#define CM_SET_ITYPE	0x02
#define CM_SET_IOADDR	0x04
#define CM_SET_MEMADDR	0x08
#define CM_SET_DMAC	0x10

/* The 'type' used for parameters above that are numerical */

typedef long	cm_num_t;

/* The following struct is returned for CM_IOADDR and CM_MEMADDR. */

typedef struct cm_addr_rng
{
	long	startaddr;
	long	endaddr;
} cm_range_t;

#ifdef _KERNEL

#ifdef __STDC__

extern int cm_AT_putconf(rm_key_t, int, int, ulong, ulong, ulong, ulong, int,
				uint, int );

#else

extern int cm_AT_putconf()

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined ( __cplusplus )
	}
#endif

#endif /* _IO_AUTOCONF_CONFMGR_CM_I386AT_H */
