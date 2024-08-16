/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/Oak.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Oak.c,v 2.3 1993/09/27 12:23:20 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3DE, 0x3DF};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Oak_Descriptor = {
	"Oak",
	Probe_Oak,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_Oak(int *Chipset)
#else
Bool Probe_Oak(Chipset)
int *Chipset;
#endif
{
	Byte temp;
	Bool result = FALSE;

	EnableIOPorts(NUMPORTS, Ports);
	if (testinx2(0x3DE, 0x0D, 0x38))
	{
		result = TRUE;
		if (testinx2(0x3DE, 0x11, 0x77))
		{
			temp = inp(0x3DE) & 0xE0;
			switch (temp)
			{
			case 0x40:
				*Chipset = CHIP_OAK067;
				break;
			case 0xA0:
				*Chipset = CHIP_OAK077;
				break;
			case 0xE0:
				*Chipset = CHIP_OAK057;
				break;
			default:
				Chip_data = temp;
				*Chipset = CHIP_OAK_UNK;
				break;
			}
		}
		else
		{
			*Chipset = CHIP_OAK037C;
		}
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
