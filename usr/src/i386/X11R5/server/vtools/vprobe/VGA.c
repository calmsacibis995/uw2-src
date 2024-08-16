/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/VGA.c	1.3"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/VGA.c,v 2.2 1993/09/21 15:20:51 dawes Exp $ */

#include "Probe.h"

/*
 * Determine if this is a VGA or an EGA.  VGA has one more attribute
 * register than EGA, so see if we can read/write it.
 */
static Word Ports[] = {0x3C0, 0x3C1};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor VGA_Descriptor = {
	"VGA",
	Probe_VGA,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
        FALSE
};

#ifdef __STDC__
Bool Probe_VGA(int *Chipset)
#else
Bool Probe_VGA(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte origVal, newVal;

	EnableIOPorts(NUMPORTS, Ports);
	origVal = rdinx(0x3C0, 0x14 | 0x20);
	outp(0x3C0, origVal ^ 0x0F);
	newVal  = rdinx(0x3C0, 0x14 | 0x20);
	outp(0x3C0, origVal);
	DisableIOPorts(NUMPORTS, Ports);
	if (newVal == (origVal ^ 0x0F))
	{
		result = TRUE;
		*Chipset = CHIP_VGA;
	}
	return(result);
}
