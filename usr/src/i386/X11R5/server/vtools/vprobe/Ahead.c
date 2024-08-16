/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/Ahead.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Ahead.c,v 2.2 1993/09/27 12:23:12 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3CE, 0x3CF};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Ahead_Descriptor = {
	"Ahead",
	Probe_Ahead,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_Ahead(int *Chipset)
#else
Bool Probe_Ahead(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte old, tmp;

	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(0x3CE, 0x0F);
	wrinx(0x3CE, 0x0F, 0);
	if (!testinx2(0x3CE, 0x0C, 0xFB))
	{
		wrinx(0x3CE, 0x0F, 0x20);
		if (testinx2(0x3CE, 0x0C, 0xFB))
		{
			result = TRUE;
			tmp = rdinx(0x3CE, 0x0F) & 0x0F;
			switch (tmp)
			{
			case 0x01:
				*Chipset = CHIP_AHEAD_A;
				break;
			case 0x02:
				*Chipset = CHIP_AHEAD_B;
				break;
			default:
				Chip_data = tmp;
				*Chipset = CHIP_AHEAD_UNK;
				break;
			}
		}
	}
	wrinx(0x3CE, 0x0F, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
