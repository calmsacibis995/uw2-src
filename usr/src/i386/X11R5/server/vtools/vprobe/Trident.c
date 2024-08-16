/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/Trident.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Trident.c,v 2.2 1993/09/27 12:23:28 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3C4, 0x3C5};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Trident_Descriptor = {
	"Trident",
	Probe_Trident,
	TridentMemory,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_Trident(int *Chipset)
#else
Bool Probe_Trident(Chipset)
int *Chipset;
#endif
{
        Bool result = FALSE;
	Byte chip, old, old1, val;

        EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(0x3C4, 0x0B);
	wrinx(0x3C4, 0x0B, 0x00);
	chip = inp(0x3C5);
	old1 = rdinx(0x3C4, 0x0E);
	outp(0x3C5, 0);
	val = inp(0x3C5);
	outp(0x3C5, (old1 ^ 0x02));
	wrinx(0x3C4, 0x0B, old);
	if ((val & 0x0F) == 2)
	{
		result = TRUE;
		switch (chip)
		{
		case 0x01:
			*Chipset = CHIP_TVGA8800BR;
			break;
		case 0x02:
			*Chipset = CHIP_TVGA8800CS;
			break;
		case 0x03:
			*Chipset = CHIP_TVGA8900B;
			break;
		case 0x04:
		case 0x13:
			*Chipset = CHIP_TVGA8900C;
			break;
		case 0x23:
			*Chipset = CHIP_TVGA9000;
			break;
		case 0x33:
			*Chipset = CHIP_TVGA8900CL;
			break;
		case 0x83:
			*Chipset = CHIP_TVGA9200;
			break;
		case 0x93:
			*Chipset = CHIP_TVGA9100;
			break;
		default:
			Chip_data = chip;
			*Chipset = CHIP_TVGA_UNK;
			break;
		}
	}
        DisableIOPorts(NUMPORTS, Ports);
        return(result);
}

TridentMemory()
{
	int vgaIOBase = (inp(0x3CC) & 0x01) ? 0x3d0 : 0x3b0;
	int temp;
	int mem = -1;

	outp(vgaIOBase+0x04, 0x1f);
	temp = inp(vgaIOBase+0x05);
	switch(temp & 0x03) {
	   case 0:
		mem = 256;
		break;
	   case 1:
		mem = 512;
		break;
	   case 2:
		mem = 768;
		break;
	   case 3:
		mem = 1024;
		break;
	}
	return (mem);
}
