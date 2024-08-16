/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/ssm/ssm.cf/Space.c	1.2"

#include <sys/types.h>	
#include <config.h>	
#include <sys/SGSproc.h>
#include <sys/ssm_vme.h>

/*
 *  VME Physical lookup table
 *      The i'th entry is the physical address to which the
 *      i'th SSM VME PIC should be programmed to respond to.
 *	Each VME takes SSMVME_ADDR_SPACE bytes of physical 
 *	address space.
 *
 * NOTE: VME 0 must remain at the PHYS_IO_SPACE + SSMVME_ADDR_SPACE to
 *	 be backward compatible with older SSM firmware.
 */
ulong ssmvme_phys_addr[] = {
	PHYS_IO_SPACE + (1 * SSMVME_ADDR_SPACE),  /* DON'T MOVE THIS ENTRY */
	PHYS_IO_SPACE + (2 * SSMVME_ADDR_SPACE),
	PHYS_IO_SPACE + (3 * SSMVME_ADDR_SPACE),
	PHYS_IO_SPACE + (9 * SSMVME_ADDR_SPACE),
	PHYS_IO_SPACE + (10 * SSMVME_ADDR_SPACE),
	PHYS_IO_SPACE + (11 * SSMVME_ADDR_SPACE),
	PHYS_IO_SPACE + (12 * SSMVME_ADDR_SPACE),
	PHYS_IO_SPACE + (13 * SSMVME_ADDR_SPACE)
};

int ssmvme_phys_addr_cnt= sizeof(ssmvme_phys_addr) / sizeof(ulong);

