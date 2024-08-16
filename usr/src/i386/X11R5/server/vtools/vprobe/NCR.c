/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/NCR.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/NCR.c,v 2.2 1993/09/27 12:23:19 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3C4, 0x3C5};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor NCR_Descriptor = {
	"NCR",
	Probe_NCR,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_NCR(int *Chipset)
#else
Bool Probe_NCR(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte old, tmp;

	EnableIOPorts(NUMPORTS, Ports);
	if (testinx2(0x3C4, 0x05, 0x05))
	{
		old = rdinx(0x3C4, 0x05);
		wrinx(0x3C4, 0x05, 0x00);
		if (!testinx2(0x3C4, 0x10, 0xFF))
		{
			wrinx(0x3C4, 0x05, 0x01);
			if (testinx2(0x3C4, 0x10, 0xFF))
			{
				result = TRUE;
				tmp = rdinx(0x3C4, 0x08) >> 4;
				if (tmp == 0)
				{
					*Chipset = CHIP_NCR77C22;
				}
				else if (tmp == 1)
				{
					*Chipset = CHIP_NCR77C21;
				}
				else if (tmp == 2)
				{
					*Chipset = CHIP_NCR77C22E;
				}
				else if ((tmp >= 8) && (tmp <= 15))
				{
					*Chipset = CHIP_NCR77C22EP;
				}
				else
				{
					Chip_data = tmp;
					*Chipset = CHIP_NCR_UNK;
				}
			}
		}
		wrinx(0x3C4, 0x05, old);
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
