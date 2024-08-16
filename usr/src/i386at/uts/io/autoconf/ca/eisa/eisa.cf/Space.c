/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/autoconf/ca/eisa/eisa.cf/Space.c	1.3"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>

/*
 * Bitmask of EISA slots to be left unparsed. 
 */
uint_t eisa_slot_mask = 0x01;

/*
 * Configuration Space access type -- real or protected mode.
 * According to the EISA spec., int 15 is a bimodal bios call, but
 * this is not supported on all EISA platforms. The <ca_eisa_realmode>
 * can be set to 0, if the system supports protected mode bios call.
 * The CA will then explicitly make the bios call to read NVRAM,
 * otherwise it will physmap the read NVRAM while the system was
 * in real-mode during early startup.
 */
int ca_eisa_realmode = 1;

/*
 * Bitmask of DMA channels to be left unparsed.
 */
uint_t eisa_dma_mask = 0x04;
