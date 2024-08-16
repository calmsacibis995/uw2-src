/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vprobe/ATIMach.c	1.6"

/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/ATIMach.c,v 2.3 1993/09/25 05:01:39 dawes Exp $ */

#include "Probe.h"
#include "AsmMacros.h"


static Word Ports[] = {ROM_ADDR_1,DESTX_DIASTP,READ_SRC_X,
	CONFIG_STATUS_1,MISC_OPTIONS, M64_SCRATCH_REG0, M64_MEM_CNTL};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor ATIMach_Descriptor = {
	"ATI_Mach",
	Probe_ATIMach,
	ATIMachMemory,
	Ports,
	NUMPORTS,
	TRUE,
	FALSE,
	TRUE
};

#define WaitIdleEmpty() { int i; \
			  for (i=0; i < 100000; i++) \
				if (!(inpw(GP_STAT) & (GPBUSY | 1))) \
					break; \
			}


#ifdef __STDC__
Bool Probe_ATIMach64(int *Chipset)
#else
Bool Probe_ATIMach64(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	int chip = -1;
	unsigned long	scratch_reg0;

	/*
	 * Try to detect the chip. See page 2.1 of programmers manual for details.
	 */
	scratch_reg0 = inpl(M64_SCRATCH_REG0);
	outpl (M64_SCRATCH_REG0, 0x55555555);
	if (inpl(M64_SCRATCH_REG0) != 0x55555555)
	{
		result = FALSE;
	}
	else
	{
		outpl (M64_SCRATCH_REG0, 0xAAAAAAAA);
		if (inpl(M64_SCRATCH_REG0) != 0xAAAAAAAA)
		{
			result = FALSE;
		}
	}
	outpl (M64_SCRATCH_REG0, scratch_reg0);
	result = TRUE;

	if (result)
	{
		chip = CHIP_MACH64;
	}
	else
	{
		chip = -1;
		result = FALSE;
	}

	if (chip != -1)
	{
		*Chipset = chip;
	}

	return(result);
}


#ifdef __STDC__
Bool Probe_ATIMach(int *Chipset)
#else
Bool Probe_ATIMach(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Word tmp;
	int chip = -1;

	EnableIOPorts(NUMPORTS, Ports);

	/*
	 * Check for 8515/A registers first.  Don't read BIOS, or an
	 * attached 8514 Ultra won't be detected (the slave SVGA's BIOS
	 * is in the normal SVGA place).
	 */
	tmp = inpw(ROM_ADDR_1);
	outpw(ROM_ADDR_1, 0x5555);
	WaitIdleEmpty();
	if (inpw(ROM_ADDR_1) == 0x5555)
	{
		outpw(ROM_ADDR_1, 0x2A2A);
		WaitIdleEmpty();
		if (inpw(ROM_ADDR_1) == 0x2A2A)
		{
			result = TRUE;
		}
	}
	outpw(ROM_ADDR_1, tmp);
	if (result)
	{
		/*
		 * Accelerator is really present; now figure
		 * out which one.
		 */
		outpw(DESTX_DIASTP, 0xAAAA);
		WaitIdleEmpty();
		if (inpw(READ_SRC_X) != 0x02AA)
		{
			chip = CHIP_MACH8;
		}
		else
		{
			chip = CHIP_MACH32;
		}
		outpw(DESTX_DIASTP, 0x5555);
		WaitIdleEmpty();
		if (inpw(READ_SRC_X) != 0x0555)
		{
			if (chip != CHIP_MACH8)
			{
				/*
				 * Something bizarre is happening.
				 */
				chip = -1;
				result = FALSE;
			}
		}
		else
		{
			if (chip != CHIP_MACH32)
			{
				/*
				 * Something bizarre is happening.
				 */
				chip = -1;
				result = FALSE;
			}
		}
	}

	if (chip != -1)
	{
		*Chipset = chip;
	}

	/*
	 * Not mach8 or mach32; check for mach64 
	 */
	if (result == FALSE)
	{
		result = Probe_ATIMach64(Chipset);
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

int
ATIMachMemory()
{
	int	val;
	int	mem = -1;	/* illegal value */
	int chip = -1;		/* illegal chipset */
	
	/*
	 * Call the probe routine to determine the type of chipset.
	 */

	Probe_ATIMach(&chip);

	EnableIOPorts(NUMPORTS, Ports);

	if (chip == CHIP_MACH32)
	{
		/*
		 * Memory ranges from 512 KB to 4 MB.
		 */

		val = inpw(MISC_OPTIONS);

		switch (val & 0x0C)
		{
		case 0x00 :
			mem = 512;
			break;
				
		case 0x04 :
			mem = 1024;
			break;
	
		case 0x08 :
			mem = 2 * 1024;
			break;
	
		case 0x0C :
			mem = 4 * 1024;
			break;
	
		default:
			mem = -1;
			break;
		}
	}
	else if (chip == CHIP_MACH8)
	{

		/*
		 * The MACH8 supports 512KB and 1 MB configurations.
		 */

		val = inpw(CONFIG_STATUS_1);

		switch (val & 0x30)
		{
		case 0x0 :
			mem = 512;
			break;

		case 0x20 : 
			mem = 1024;
			break;
		default :
			mem = -1;
			break;
		}
	}
	else if (chip == CHIP_MACH64)
	{
		unsigned long	mem_cntl;

		/*
		 * Memory ranges from 512 KB to 8 MB.
		 */
		mem_cntl = inpl(M64_MEM_CNTL);
		switch (mem_cntl & 7)
		{
			case  0 :
				mem = 512;
			break;

			case  1 :
				mem = 1024;
			break;

			case  2 :
			case  3 :
			case  4 :
			case  5 :
				mem = (((mem_cntl & 7) * 2 ) - 2) * 1024;
			break;

			default:
				mem = -1;
				break;
		}
	}

	DisableIOPorts(NUMPORTS, Ports);

	return(mem);
}
