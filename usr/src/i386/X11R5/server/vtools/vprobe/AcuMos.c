/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/AcuMos.c	1.2"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/AcuMos.c,v 2.1 1993/09/21 15:20:27 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3C4, 0x3C5, 0x3CE, 0x3CF};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor AcuMos_Descriptor = {
	"AcuMos",
	Probe_AcuMos,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};

#ifdef __STDC__
Bool Probe_AcuMos(int *Chipset)
#else
Bool Probe_AcuMos(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte old;

	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(0x3C4, 0x06);
	wrinx(0x3C4, 0x06, 0x12);
	if (testinx2(0x3CE, 0x09, 0x30))
	{
		result = TRUE;
		*Chipset = CHIP_ACUMOS;
	}
	wrinx(0x3C4, 0x06, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
