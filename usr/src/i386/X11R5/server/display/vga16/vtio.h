/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vtio.h	1.7"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#include "vtdefs.h"

#define BYTE	unsigned char
#define BITS16	unsigned short
#define SUCCESS	1
#define FAIL	0

/*
 * structure used to initialize the EGA/VGA registers
 */
struct vga_regs {
	unchar		seqtab[NSEQ];
	unchar		miscreg;
	struct egainit	egatab;
};


/*
 * General adapter information such as size of display, pixels per inch, etc.
 */
struct at_disp_info {
	char	*entry;		/* entry, "ET4000" */
	char	*monitor;	/* monitor type, "MULTISYNC" */
	int	vt_type;	/* defined type, VT_VGA */
	BYTE	is_vga;		/* true if adapter supports VGA write modes */
	BYTE	is_chained;	/* true if display mode chains planes */
	int	xpix;		/* pixels per scanline */
	int	ypix;		/* number of scanlines */
	int	planes;		/* number of planes of memory */
	int	colors;		/* number of colors available */
	BYTE	*vt_buf;	/* virtual address of screen memory */
	int	buf_size;	/* size of screen memory */
	int	map_size;	/* size of one plane of memory */
	int	ad_addr;	/* base register address for adapter */
	int	slbytes;	/* number of bytes in a scanline */
	BITS16	wmode;		/* write mode 0 setting */
	int	(*ext_init)();	/* called to initialize 'extended' modes */
	int	(*ext_rest)();	/* called to recover from 'extended' modes */
	struct vga_regs *regs;	/* vga registers for mode */
};

#define ErrorF printf
