/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/mip/olivetti.c	1.4"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/inline.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/cram.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/mip.h>

#define	OLIVETTI_NAME1	0x3d
#define OLIVETTI_NAME2	0x89
#define OLIVETTI_NAME3	0x03

extern int	olivetti(), olivetti_ident();

olivetti(lpcbp, machine)
struct	lpcb *lpcbp;
unsigned char machine;
{
	/* none for the Olivetti LSX systems */
	at386(lpcbp, machine);	/* use AT386 type initialization */
	return 0;
}

int
olivetti_ident()
{

	if (membrk((char *)0xfe017, "OLIVETTI", 0xffff, 8) != 0) 
	   if ((BTE_INFO.machflags & EISA_IO_BUS) &&
		((inb(EISA_MFG_NAME1) & MFG_NAME1_MASK) == OLIVETTI_NAME1) && 
		((inb(EISA_MFG_NAME2) == OLIVETTI_NAME2)) && 
		((inb(EISA_PROD_TYPE) == OLIVETTI_NAME3))){
			BOOTENV->be_flags |= BE_NOINVD;
			return M_ID_OLIVETTILSX;
		}
	return 0;
}
