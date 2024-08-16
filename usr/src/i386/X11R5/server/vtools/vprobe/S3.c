/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/S3.c	1.4"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/S3.c,v 2.4 1993/09/27 12:23:26 dawes Exp $ */

#include "Probe.h"
#ifndef xxx
#include "AsmMacros.h"
#endif

static Word Ports[] = {0x000, 0x000};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

#ifdef xxx
Chip_Descriptor S3_Descriptor = {
	"S3",
	Probe_S3,
	NullEntry,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};
#else
Chip_Descriptor S3_Descriptor = {
	"S3",
	Probe_S3,
	S3Memory,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE
};
#endif

#ifdef __STDC__
Bool Probe_S3(int *Chipset)
#else
Bool Probe_S3(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte old, tmp, rev;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);
	old = rdinx(CRTC_IDX, 0x38);
	wrinx(CRTC_IDX, 0x38, 0x00);
	if (!testinx2(CRTC_IDX, 0x35, 0x0F))
	{
		wrinx(CRTC_IDX, 0x38, 0x48);
		if (testinx2(CRTC_IDX, 0x35, 0x0F))
		{
			result = TRUE;
			rev = rdinx(CRTC_IDX, 0x30);
			switch (rev & 0xF0)
			{
			case 0x80:
				switch (rev & 0x0F)
				{
				case 0x01:
					*Chipset = CHIP_S3_911;
					break;
				case 0x02:
					*Chipset = CHIP_S3_924;
					break;
				default:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				}
				break;
			case 0xA0:
				tmp = rdinx(CRTC_IDX, 0x36);
				switch (tmp & 0x03)
				{
				case 0x00:
				case 0x01:
					/* EISA or VLB - 805 */
					switch (rev & 0x0F)
					{
					case 0x00:
						*Chipset = CHIP_S3_805B;
						break;
					case 0x01:
						Chip_data = rev;
						*Chipset = CHIP_S3_UNKNOWN;
						break;
					case 0x02:
					case 0x03:
					case 0x04:
						*Chipset = CHIP_S3_805C;
						break;
					case 0x05:
						*Chipset = CHIP_S3_805D;
						break;
					default:
						/* Call >0x05 D step for now */
						*Chipset = CHIP_S3_805D;
						break;
					}
					break;
				case 0x03:
					/* ISA - 801 */
					switch (rev & 0x0F)
					{
					case 0x00:
						*Chipset = CHIP_S3_801B;
						break;
					case 0x01:
						Chip_data = rev;
						*Chipset = CHIP_S3_UNKNOWN;
						break;
					case 0x02:
					case 0x03:
					case 0x04:
						*Chipset = CHIP_S3_801C;
						break;
					case 0x05:
						*Chipset = CHIP_S3_801D;
						break;
					default:
						/* Call >0x05 D step for now */
						*Chipset = CHIP_S3_801D;
						break;
					}
					break;
				default:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				}
				break;
			case 0x90:
				switch (rev & 0x0F)
				{
				case 0x00:
				case 0x01:
					/*
					 * Contradictory documentation -
					 * one says 0, the other says 1.
					 */
					*Chipset = CHIP_S3_928D;
					break;
				case 0x02:
				case 0x03:
					Chip_data = rev;
					*Chipset = CHIP_S3_UNKNOWN;
					break;
				case 0x04:
					*Chipset = CHIP_S3_928E;
					break;
				case 0x05:
					*Chipset = CHIP_S3_928G;
					break;
				default:
					/* Call >0x05 G step for now */
					*Chipset = CHIP_S3_928G;
				}
				break;
			case 0xB0:
				/*
				 * Don't know anything more about this
				 * just yet.
				 */
				*Chipset = CHIP_S3_928P;
				break;
			case 0xC0:
				/*
				 * No information on the chipset step
				 */
				*Chipset = CHIP_S3_864;
				break;	
			case 0xD0:
				/*
				 * No information on the chipset step
				 */
				*Chipset = CHIP_S3_964;
				break;	
			default:
				Chip_data = rev;
				*Chipset = CHIP_S3_UNKNOWN;
				break;
			}
		}
	}
	wrinx(CRTC_IDX, 0x38, old);
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

#ifndef xxx
int
S3Memory()
{
	Byte old1, old2;
	unsigned char	val;
	int	mem = -1;

	old1 = rdinx(CRTC_IDX, 0x38);
	old2 = rdinx(CRTC_IDX, 0x39);

	outb(0x3d4,0x38);
	outb(0x3d5,0x48);

	outb(0x3d4,0x39);
	outb(0x3d5,0xa0);

	/*
	 * Put the memory detection code here.
	 */

	val = rdinx(CRTC_IDX,0x36);
	switch ( val & 0xE0 )
	{
		case 0xe0:
			mem = 512;
			break;
		case 0xc0:
			mem = 1024;
			break;
		case 0x80:
			mem = 2*1024;
			break;
		case 0x40:
			mem = 3*1024;
			break;
		case 0x00:
			mem = 4*1024;
			break;
		default:
			/* CONSTANTCONDITION */
			mem = -1;
			break;
	}
	
	wrinx(CRTC_IDX, 0x38, old1);
	wrinx(CRTC_IDX, 0x39, old2);
	return(mem);
}
#endif
