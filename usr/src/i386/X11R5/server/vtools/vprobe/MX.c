/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/MX.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/MX.c,v 2.1 1993/09/21 15:20:34 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3C4, 0x3C5};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor MX_Descriptor = {
	"MX",
	Probe_MX,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_MX(int *Chipset)
#else
Bool Probe_MX(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte old;

	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(0x3C4, 0xA7);
	wrinx(0x3C4, 0xA7, 0x00);
	if (!testinx(0x3C4, 0xC5))
	{
		wrinx(0x3C4, 0xA7, 0x87);
		if (testinx(0x3C4, 0xC5))
		{
			result = TRUE;
			*Chipset = CHIP_MX68010;
		}
	}
	wrinx(0x3C4, 0xA7, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
