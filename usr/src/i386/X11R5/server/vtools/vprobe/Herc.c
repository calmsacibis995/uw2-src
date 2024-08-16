/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/Herc.c	1.1"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Herc.c,v 1.2 1993/05/04 10:16:55 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3BA};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

#define DSP_VSYNC_MASK 0x80
#define DSP_ID_MASK 0x70

#ifdef __STDC__
Bool Probe_Herc(int *Chipset)
#else
Bool Probe_Herc(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	int i, cnt = 0;
	Byte dsp, dsp_old;

	EnableIOPorts(NUMPORTS, Ports);
	dsp_old = inp(0x3BA) & DSP_VSYNC_MASK;
	for (i = 0; i < 0x10000; i++)
	{
		dsp = inp(0x3BA) & DSP_VSYNC_MASK;
		if (dsp != dsp_old)
			cnt++;
		dsp_old = dsp;
	}

	/* If there are active sync changes, we found a Hercules board. */
	if (cnt)
	{
		dsp = inp(0x3BA) & DSP_ID_MASK;
		switch(dsp)
		{
		case 0x10:
			*Chipset = CHIP_HERC_PLUS;
			break;
		case 0x50:
			*Chipset = CHIP_HERC_COL;
			break;
		default:
			*Chipset = CHIP_HERC_STD;
		}
		result = TRUE;
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
