/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/ChipsTech.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/ChipsTech.c,v 2.2 1993/09/27 12:23:15 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3D6, 0x3D7};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor CT_Descriptor = {
	"CT",
	Probe_CT,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_CT(int *Chipset)
#else
Bool Probe_CT(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte vers;

	EnableIOPorts(NUMPORTS, Ports);
	if ((testinx(0x3D6, 0x18) && (testinx2(0x3D6, 0x7E, 0x3F))))
	{
		/*
		 * It's a Chips & Tech.  Now figure out which one.
		 */
		result = TRUE;
		vers = rdinx(0x3D6, 0x00);
		switch (vers >> 4)
		{
		case 0:
			*Chipset = CHIP_CT451;
			break;
		case 1:
			*Chipset = CHIP_CT452;
			break;
		case 2:
			*Chipset = CHIP_CT455;
			break;
		case 3:
			*Chipset = CHIP_CT453;
			break;
		case 4:
			*Chipset = CHIP_CT450;
			break;
		case 5:
			*Chipset = CHIP_CT456;
			break;
		case 6:
			*Chipset = CHIP_CT457;
			break;
		case 7:
			*Chipset = CHIP_CTF65520;
			break;
		case 8:
			*Chipset = CHIP_CTF65530;
			break;
		case 9:
			*Chipset = CHIP_CTF65510;
			break;
		default:
			Chip_data = (vers >> 4);
			*Chipset = CHIP_CT_UNKNOWN;
			break;
		}
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
