/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/ATI.c	1.3"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/ATI.c,v 2.3 1993/09/27 12:23:11 dawes Exp $ */

#include "Probe.h"

Chip_Descriptor ATI_Descriptor = {
	"ATI",
	Probe_ATI,
	NullEntry,
	NULL,
	0,
	FALSE,
	TRUE,
	TRUE
};

#ifdef __STDC__
Bool Probe_ATI(int *Chipset)
#else
Bool Probe_ATI(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte bios[10];
	Byte *signature = (Byte *)"761295520";

	if (ReadBIOS(0x31, bios, 9) != 9)
	{
		fprintf(stderr, "%s: Failed to read ATI signature\n", MyName);
		return(FALSE);
	}
	if (memcmp(bios, signature, 9) == 0)
	{
		if (ReadBIOS(0x40, bios, 4) != 4)
		{
			fprintf(stderr, "%s: Failed to read ATI BIOS data\n",
				MyName);
			return(FALSE);
		}
		if ((bios[0] == '3') && (bios[1] == '1'))
		{
			result = TRUE;
			switch (bios[3])
			{
			case '1':
				*Chipset = CHIP_ATI18800;
				break;
			case '2':
				*Chipset = CHIP_ATI18800_1;
				break;
			case '3':
				*Chipset = CHIP_ATI28800_2;
				break;
			case '4':
				*Chipset = CHIP_ATI28800_4;
				break;
			case '5':
				*Chipset = CHIP_ATI28800_5;
				break;
			case 'a':
				*Chipset = CHIP_ATI28800_A;
				break;
			case 'c':
				*Chipset = CHIP_ATI28800_C; /* XLR? */
				break;
			default:
				Chip_data = bios[3];
				*Chipset = CHIP_ATI_UNK;
				break;
			}
		}
	}
	return(result);
}
