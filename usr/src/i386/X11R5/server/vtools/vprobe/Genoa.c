/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/Genoa.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Genoa.c,v 2.1 1993/09/21 15:20:33 dawes Exp $ */

#include "Probe.h"

Chip_Descriptor Genoa_Descriptor = {
	"Genoa",
	Probe_Genoa,
	NullEntry,
	NULL,
	0,
	FALSE,
	TRUE,
	FALSE
};

#ifdef __STDC__
Bool Probe_Genoa(int *Chipset)
#else
Bool Probe_Genoa(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte addr, data[4];

	if (ReadBIOS(0x37, &addr, 1) != 1)
	{
		fprintf(stderr, "%s: Failed to read Genoa BIOS address.\n",
			MyName);
		return(FALSE);
	}
	if (ReadBIOS((unsigned)addr, data, 4) != 4)
	{
		fprintf(stderr, "%s: Failed to read Genoa BIOS signature.\n",
			MyName);
		return(FALSE);
	}
	if ((data[0] == 0x77) && (data[2] == 0x99) && (data[3] == 0x66))
	{
		/*
		 * Genoa also has ET3000 and (possibly) ET4000 based
		 * boards that match this signature.  We only match
		 * the ones with Genoa chips, and let other probe
		 * functions deal with other chipsets.
		 */
		switch (data[1])
		{
		case 0x00:
			*Chipset = CHIP_G_6200;
			result = TRUE;
			break;
		case 0x11:
			*Chipset = CHIP_G_6400;
			result = TRUE;
			break;
		case 0x22:
			*Chipset = CHIP_G_6100;
			result = TRUE;
			break;
		}
	}
	return(result);
}
