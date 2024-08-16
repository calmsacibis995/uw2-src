/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/Video7.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Video7.c,v 2.2 1993/09/27 12:23:30 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, 0x3C4, 0x3C5};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Video7_Descriptor = {
	"Video7",
	Probe_Video7,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_Video7(int *Chipset)
#else
Bool Probe_Video7(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte old, old1, id;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(0x3C4, 0x06);
	wrinx(0x3C4, 0x06, 0xEA);		/* enable extensions */
	old1 = rdinx(CRTC_IDX, 0x0C);
	wrinx(CRTC_IDX, 0x0C, 0x55);
	id = rdinx(CRTC_IDX, 0x1F);
	wrinx(CRTC_IDX, 0x0C, old1);
	if (id == (0x55 ^ 0xEA))
	{
		/*
		 * It's Video7
		 */
		result = TRUE;
		id = rdinx(0x3C4, 0x8E);
		if ((id < 0xFF) && (id >= 0x80))
		{
			*Chipset = CHIP_V7_VEGA;
		}
		else if ((id < 0x7F) && (id >= 0x70))
		{
			*Chipset = CHIP_V7_FWRITE;
		}
		else if ((id < 0x5A) && (id >= 0x50))
		{
			*Chipset = CHIP_V7_VRAM2;
		}
		else if ((id < 0x4A) && (id >= 0x40))
		{
			*Chipset = CHIP_V7_1024i;
		}
		else
		{
			Chip_data = id;
			*Chipset = CHIP_V7_UNKNOWN;
		}
	}
	wrinx(0x3C4, 6, old);		/* disable extensions */
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
