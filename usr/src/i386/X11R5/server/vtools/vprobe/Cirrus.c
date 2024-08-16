/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vprobe/Cirrus.c	1.6"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Cirrus.c,v 2.4 1993/10/07 13:54:45 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, 0x3C4, 0x3C5, 0x3CE, 0x3CF};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Cirrus_Descriptor = {
	"Cirrus",
	Probe_Cirrus,
	CirrusMemory,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	FALSE,
};

#if 0
#ifdef __STDC__
Bool Probe_Cirrus(int *Chipset)
#else
Bool Probe_Cirrus(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte old, old1, tmp;

	/* Add CRTC to enabled ports */
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);
	/*
	 * First the old Cirrus chips.
	 */
	old = rdinx(CRTC_IDX, 0x0C);
	old1 = rdinx(0x3C4, 0x06);
	wrinx(CRTC_IDX,0x0C,0);
	tmp = rdinx(CRTC_IDX, 0x1F);
	wrinx(0x3C4, 0x06, (tmp >> 4) | (tmp << 4));
	if (inp(0x3C5) == 0)
	{
		outp(0x3C5, tmp);
		if (inp(0x3C5) == 1)
		{
			result = TRUE;
			switch (tmp)
			{
			case 0xEC:
				*Chipset = CHIP_CL510;
				break;
			case 0xCA:
				*Chipset = CHIP_CL610;
				break;
			case 0xEA:
				*Chipset = CHIP_CLV7;
				break;
			default:
				Chip_data = tmp;
				*Chipset = CHIP_CL_UNKNOWN;
				break;
			}
		}
	}
	wrinx(0x3C4, 0x06, old1);
	wrinx(CRTC_IDX, 0x0C, old);
	/*
	 * Now the new Cirrus chips
	 */
	if (!result)
	{
		old = rdinx(0x3C4, 0x06);
		wrinx(0x3C4, 0x06, 0x12);
		if ((rdinx(0x3C4, 0x06) == 0x12) &&
		    (testinx2(0x3C4, 0x1E, 0x3F)) &&
		    (testinx2(CRTC_IDX, 0x1B, 0xE3)))
		{
			result = TRUE;
			if (testinx(CRTC_IDX, 0x21))
			{
				/* 62x5 */
				tmp = rdinx(CRTC_IDX, 0x27);
				switch ((tmp & 0xF0) >> 4)
				{
				case 0x00:
					*Chipset = CHIP_CL6205;
					break;
				case 0x08:
					*Chipset = CHIP_CL6215;
					break;
				case 0x0C:
					*Chipset = CHIP_CL6225;
					break;
				case 0x01:
					*Chipset = CHIP_CL6235;
					break;
				default:
					Chip_data = tmp;
					*Chipset = CHIP_CL_UNKNOWN;
					break;
				}
			}
			else
			{
				/* 542x */
				tmp = rdinx(CRTC_IDX, 0x27);
				switch ((tmp & 0xFC) >> 2)
				{
				case 0x22:
					switch (tmp & 0x03)
					{
					case 0x00:
						*Chipset = CHIP_CL5402;
						break;
					case 0x01:
						*Chipset = CHIP_CL5402R1;
						break;
					case 0x02:
						*Chipset = CHIP_CL5420;
						break;
					case 0x03:
						*Chipset = CHIP_CL5420R1;
						break;
					}
					break;
				case 0x23:
					*Chipset = CHIP_CL5422;
					break;
				case 0x25:
					*Chipset = CHIP_CL5424;
					break;
				case 0x24:
					*Chipset = CHIP_CL5426;
					break;
				case 0x26:
					*Chipset = CHIP_CL5428;
					break;
				default:
					Chip_data = tmp;
					*Chipset = CHIP_CL_UNKNOWN;
					break;
				}
			}
		}
		wrinx(0x3C4, 0x06, old);
	}
	if (!result)
	{
		old = rdinx(0x3CE, 0x0A);
		wrinx(0x3CE, 0x0A, 0xEC);
		if (rdinx(0x3CE, 0x0A) == 0x01)
		{
			result = TRUE;
			if (rdinx(0x3CE, 0xAB) == 0x6F)
			{
				/* 6420 */
				if (testinx2(0x3CE, 0x87, 0x90))
				{
					*Chipset = CHIP_CL6420B;
				}
				else
				{
					*Chipset = CHIP_CL6420A;
				}
			}
			else
			{
				*Chipset = CHIP_CL6410;	/* I think */
			}
		}
		wrinx(0x3CE, 0x0A, old);
	}

	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}
#endif 0

#define GD5420   0x88
#define GD5422   0x8c
#define GD5424   0x94
#define GD5426   0x90
#define GD5428   0x98
#define GD5434   0xa8


#ifdef __STDC__
Bool Probe_Cirrus(int *Chipset)
#else
Bool Probe_Cirrus(Chipset)
int *Chipset;
#endif
{
	unsigned char temp, chipID;

	/*
	 * To identify the Cirrus chip:
	 *	a. enable (unlock) all extended regs (SR6)
	 *	b. read the chip ID Register (CR27)
	 *
	 *	8a = 5420
	 *	8c = 5422
	 *	94 = 5424
	 *	90 = 5426
	 *	99 = 5428
	 *	a8 = 5434
	 */
#if 0
	outp (0x3c4, 6); 		/* enable extended regs, SR6 */
	outp (0x3c5, 0x12);
#endif
	outp (0x3d4, 0x27);		/* read chip ID Register CR27 */ 
	*Chipset = 0;

	   /*
	    * NOTE: chipID - ignore bits 0 and 1; the doc is not clear, but it 
	    * was confirmed with Cirrus engineers 10/12/93
	    */
	   switch( chipID = (inp(0x3d5)&0xfc) )
	   {
	    case GD5420:
		*Chipset = CHIP_CL5420;
		break;
	    case GD5422:
		*Chipset = CHIP_CL5422;
		break;
	    case GD5424:
		*Chipset = CHIP_CL5424;
		break;
	    case GD5426:
		*Chipset = CHIP_CL5426;
		break;
	    case GD5428:
		*Chipset = CHIP_CL5428;
		break;
	    case GD5434:
		*Chipset = CHIP_CL5434;
		break;
	    default:
		break;
	   };

	if (*Chipset == 0)
		return (FALSE);
	else
		return (TRUE);
}


int
CirrusMemory ()
{
	int temp;
	int mem = -1;

	EnableIOPorts(NUMPORTS, Ports);

	/* enable extended reg SR6 */
	outp(0x3c4, 6);
	outp(0x3c5, 0x12);
	outp(0x3d4, 0x27);

	/*
	 *
	 * bits 3,4:
	 *	00 : 256	01 : 512
	 *	10 : 1024	11 : 2048 
	 */
	outp(0x3C4, 0x0a);
	temp = (inp(0x3C5) >>3) & 0x03;
	switch ( temp )
	{
	    case 0:
		mem = 256;
		break;
	    case 1:
		mem = 512;
		break;
	    case 2:
		mem = 1024;
		break;
	    case 3:
		mem = 2048;
		break;
	}
	DisableIOPorts(NUMPORTS, Ports);

	return (mem);
}

