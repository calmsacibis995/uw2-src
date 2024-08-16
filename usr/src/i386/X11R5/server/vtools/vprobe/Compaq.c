/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/Compaq.c	1.4"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Compaq.c,v 2.3 1993/09/27 12:23:17 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x3CE, 0x3CF, 0x33C8, 0x53C8};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))


int CompaqMemory();

Chip_Descriptor Compaq_Descriptor = {
	"Compaq",
	Probe_Compaq,
	CompaqMemory,
	Ports,
	NUMPORTS,
	TRUE,
	TRUE,
	FALSE
};

static int qv_ram_dac = -1;		/* unknown Ram Dac */
static int qvMemSize = -1;		/* unknown Mem Size */

#ifdef __STDC__
Bool Probe_Compaq(int *Chipset)
#else
Bool Probe_Compaq(Chipset)
int *Chipset;
#endif
{
	Bool result = FALSE;
	Byte addr, bios[6];
	Byte *signature = (Byte *)"COMPAQ";
	Byte old, old1, old2, old3;
	Byte tmp, tmp1;
	Word port;
	int rev;

	*Chipset = CHIP_CPQ_UNK;

	EnableIOPorts(NUMPORTS, Ports);
	if (ReadBIOS(0x02, &addr, 1) != 1)
	{
		fprintf(stderr, "%s: Failed to find get Compaq BIOS address\n",
			MyName);
		return(FALSE);
	}
	if (addr > 64)
	{
		/* Out of range */
		return(FALSE);
	}
	if (ReadBIOS((unsigned)((addr<<9)-0x16), bios, 6) != 6)
	{
		fprintf(stderr, "%s: Failed to read Compaq BIOS signature\n",
			MyName);
		return(FALSE);
	}
	if (memcmp(bios, signature, 6) == 0)
	{

		old = rdinx(0x3CE, 0x00);
		old1 = rdinx(0x3CE, 0x03);
		old2 = rdinx(0x3CE, 0x0F);
		wrinx(0x3CE, 0x0F, 0x05);
		old3 = rdinx(0x3CE, 0x10);
		tmp = old3 & 0x0F; 
		wrinx(0x3CE, 0x00, ~old);
		tmp1 = rdinx(0x3CE, 0x10) & 0x0F;
		if ((tmp != old) || (tmp1 != ~old))
		{
			wrinx(0x3CE, 0x10, old3 | 0x08);
			port = (old3 & 0x04) ? 0x53C8 : 0x33C8;
			tmp = inp(port);
			wrinx(0x3CE, 0x03, (~old1 & 0x07));
			tmp1 = inp(port);
			if (tmp == (old1 & 0x07) && (tmp1 == (~old1 & 0x07)))
			{
				/*
				 * OK.  It's an AVGA or QVision.  Let's
				 * see which one.
				 */
				result = TRUE;
				tmp = rdinx(0x3CE, 0x0C);
				tmp1 = (tmp & 0xF8U) >> 3;
				switch (tmp1)
				{
				case 0x05:
				case 0x10:
					*Chipset = CHIP_CPQ_AVGA;
					break;
				case 0x06:
					*Chipset = CHIP_CPQ_Q1024;
					break;
				case 0x0E:
					/*
					 * QVision.  Now see which one.
					 */
					qvMemSize = (rdinx(0x3CE,0x54)<< 8);
					tmp = rdinx(0x3CE, 0x56);
					if (tmp & 0x04)
					{
						*Chipset = CHIP_CPQ_Q1280;
					}
					else
					{
						*Chipset = CHIP_CPQ_Q1024;
					}
					rev = rdinx(0x3CE, 0x0d);
					if ((rev & 1) || ((rev & 0x7f) == 0)) {
					    rev = rdinx(0x3CE,0x56);
					    rev |= (rdinx(0x3CE,0x57) << 8);
					    if (rev == 0x936)
						qv_ram_dac = 1; /* BT485 */
					    else
						qv_ram_dac = 0;  /* BT484 */
					}
					break;
				default:
					Chip_data = tmp1;
					*Chipset = CHIP_CPQ_UNK;
					break;
				}
			}
		}
		wrinx(0x3CE, 0x10, old3);
		wrinx(0x3CE, 0x0F, old2);
		wrinx(0x3CE, 0x03, old1);
		wrinx(0x3CE, 0x00, old);
	}
	DisableIOPorts(NUMPORTS, Ports);
	return(result);
}

int Compaq_RAMDAC()
{
	if (qv_ram_dac == 0) {
		return ( DAC_BT484 | DAC_6_8_PROGRAM);
	}
	else {
		return ( DAC_BT485 | DAC_6_8_PROGRAM);
	}
}

int CompaqMemory()
{
	return qvMemSize;
}
