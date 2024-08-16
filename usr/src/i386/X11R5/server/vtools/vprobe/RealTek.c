/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/RealTek.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/RealTek.c,v 2.1 1993/09/21 15:20:45 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, 0x3D6, 0x3D7};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor RealTek_Descriptor = {
	"RealTek",
	Probe_RealTek,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_RealTek(int *Chipset)
#else
Bool Probe_RealTek(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);
	if (testinx2(CRTC_IDX, 0x1F, 0x3F) &&
	    tstrg(0x3D6, 0x0F) &&
	    tstrg(0x3D7, 0x0F))
	{
		result = TRUE;
		*Chipset = CHIP_REALTEK;
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}