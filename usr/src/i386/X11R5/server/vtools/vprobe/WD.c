/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/WD.c	1.4"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/WD.c,v 2.2 1993/09/22 15:42:22 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, 0x3CE, 0x3CF, 0x3C4, 0x3C5};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor WD_Descriptor = {
	"WD",
	Probe_WD,
	WDMemory,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_WD(int *Chipset)
#else
Bool Probe_WD(Chipset)
int *Chipset;
#endif
{
	Byte old, old1, old2, old3;
	Bool result = FALSE;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(0x3CE, 0x0F);
	modinx(0x3CE, 0x0F, 0x17, 0x00);	/* Lock registers */
	if (!testinx2(0x3CE, 0x09, 0x7F))
	{
		wrinx(0x3CE, 0x0F, 0x05);	/* Unlock them again */
		if (testinx2(0x3CE, 0x09, 0x7F))
		{
			result = TRUE;
			old2 = rdinx(CRTC_IDX, 0x29);
			/* Unlock WD90Cxx regs */
			modinx(CRTC_IDX, 0x29, 0x8F, 0x85);
			if (!testinx(CRTC_IDX, 0x2B))
			{
				*Chipset = CHIP_WD_PVGA1;
			}
			else
			{
				old1 = rdinx(0x3C4, 0x06);
				wrinx(0x3C4, 0x06, 0x48);
				if (!testinx2(0x3C4, 0x07, 0xF0))
				{
					*Chipset = CHIP_WD_90C00;
				}
				else if (!testinx(0x3C4, 0x10))
				{
					if (testinx2(CRTC_IDX, 0x31, 0x68))
					{
						*Chipset = CHIP_WD_90C22;
					}
					else if (testinx2(CRTC_IDX, 0x31, 0x90))
					{
						*Chipset = CHIP_WD_90C20A;
					}
					else
					{
						*Chipset = CHIP_WD_90C20;
					}
					wrinx(CRTC_IDX, 0x34, 0xA6);
					if (rdinx(CRTC_IDX, 0x32))
					{
						wrinx(CRTC_IDX, 0x34, 0x00);
					}
				}
				else if (testinx(0x3C4, 0x25))
				{
					*Chipset = CHIP_WD_90C24;
				}
				else if (testinx2(0x3C4, 0x14, 0x0F))
				{
					old3 = rdinx(CRTC_IDX, 0x34);
					wrinx(CRTC_IDX, 0x34, 0xA0);
					if (testinx(CRTC_IDX, 0x31))
					{
						*Chipset = CHIP_WD_90C26;
					}
					else if (rdinx(CRTC_IDX, 0x37) == 0x31)
					{
						*Chipset = CHIP_WD_90C31;
					}
					else
					{	
						*Chipset = CHIP_WD_90C30;
					}
					wrinx(CRTC_IDX, 0x34, old3);
				}
				else if (!testinx2(0x3C4, 0x10, 0x04))
				{
					*Chipset = CHIP_WD_90C10;
				}
				else
				{
					*Chipset = CHIP_WD_90C11;
				}
				wrinx(0x3C4, 0x06, old1);
			}
			wrinx(CRTC_IDX, 0x29, old2);
		}
	}
	wrinx(0x3CE, 0x0F, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

int
WDMemory()
{
	int temp;
	int mem = -1;

#if 0
/* I don't think this is fool-proof way of detecting. If there is no
 * fool proof way, it is better not to detect than return a wrong value
 * 2/25/94
 */
	/*
	 * Detect how much memory is installed
	 */
	outp(0x3CE, 0x0B); temp = inp(0x3CF);
	switch(temp & 0xC0) {
	    case 0x00:
		mem = 256;
		break;
	    case 0x40:
		mem = 512;
		break;
	    case 0x80:
		mem = 1024;
		break;
	    case 0xC0:
		mem = 2048;
		break;
	}
#endif
	return(mem);
}
