/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vprobe/RamDac.c	1.3"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/RamDac.c,v 2.3 1993/09/27 12:23:25 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0, 0, 0x3C6, 0x3C7, 0x3C8, 0x3C9};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

#if __STDC__
static Bool Width8Check(void)
#else
static Bool Width8Check()
#endif
{
	int i;
	Byte save[3];
	Bool result = FALSE;

	/*
	 * Figure out whether this RAMDAC has 6-bit or 8-bit wide lookup
	 * table columns.
	 */
	outp(0x3C6, 0xFF);
	outp(0x3C7, 0x00);
	for (i=0; i < 3; i++)
	{
		save[i] = inp(0x3C9);
	}
	outp(0x3C8, 0x00);
	for (i=0; i < 3; i++)
	{
		outp(0x3C9, 0xFF);
	}
	outp(0x3C7, 0x00);
	if ((inp(0x3C9) == (Byte)0xFF) && 
	    (inp(0x3C9) == (Byte)0xFF) &&
	    (inp(0x3C9) == (Byte)0xFF))
	{
		result = TRUE;
	}
	outp(0x3C8, 0x00);
	for (i=0; i < 3; i++)
	{
		outp(0x3C9, save[i]);
	}

	return(result);
}

#ifdef __STDC__
static void CheckATT(int *RamDac)
#else
static void CheckATT(RamDac)
int *RamDac;
#endif
{
	Byte savecomm, tmp;

	(void)dactocomm();
	savecomm = inp(0x3C6);
	(void)dactocomm();
	outp(0x3C6, 0xE0);
	(void)dactocomm();
	if ((inp(0x3C6) & 0xE0) != 0xE0)
	{
		*RamDac = DAC_ATT497;
	}
	else
	{
		(void)dactocomm();
		outp(0x3C6, 0x60);
		(void)dactocomm();
		if ((inp(0x3C6) & 0xE0) == 0x00)
		{
			(void)dactocomm();
			tmp = inp(0x3C6);
			(void)dactocomm();
			outp(0x3C6, tmp | 0x02);
			(void)dactocomm();
			if ((inp(0x3C6) & 0x02) == 0x02)
			{
				*RamDac = DAC_ATT490;
				*RamDac |= DAC_6_8_PROGRAM;
			}
			else
			{
				*RamDac = DAC_ATT493;
			}
		}
		else
		{
			(void)dactocomm();
			tmp = inp(0x3C6);
			(void)dactocomm();
			outp(0x3C6, tmp | 0x02);
			if (Width8Check())
			{
				*RamDac = DAC_ATT491;
				*RamDac |= DAC_6_8_PROGRAM;
			}
			else
			{
				*RamDac = DAC_ATT492;
			}
		}
	}
	(void)dactocomm();
	outp(0x3C6, savecomm);
}

#ifdef __STDC__
static Bool S3_Bt485Check(int *RamDac)
#else
static Bool S3_Bt485Check(RamDac)
int *RamDac;
#endif
{
	Byte old1, old2, old3, old4;
	Byte lock1, lock2;
	Bool Found = FALSE;

	lock1 = rdinx(CRTC_IDX, 0x38);
	lock2 = rdinx(CRTC_IDX, 0x39);
	wrinx(CRTC_IDX, 0x38, 0x48);
	wrinx(CRTC_IDX, 0x39, 0xA5);

	old1 = inp(0x3C6);
	old2 = rdinx(CRTC_IDX, 0x55);
	outp(0x3C6, 0xFF);
	wrinx(CRTC_IDX, 0x55, (old2 & 0xFC) | 0x02);
	old3 = inp(0x3C6);
	if ((old3 & 0xC0) == 0x80)
	{
		Found = TRUE;
		*RamDac = DAC_BT485;
		*RamDac |= DAC_6_8_PROGRAM;
		if (Width8Check())
		{
			*RamDac |= DAC_8BIT;
		}
	}
	else
	{
		/* Perhaps status reg is hidden behind CR3 */
		wrinx(CRTC_IDX, 0x55, (old2 & 0xFC) | 0x01);
		old3 = inp(0x3C6);
		if ((old3 & 0x80) == 0x80)
		{
			/* OK.  CR3 is active... */
			wrinx(CRTC_IDX, 0x55, (old2 & 0xFC) | 0x00);
			old3 = inp(0x3C8);
			outp(0x3C8, 0x00);
			wrinx(CRTC_IDX, 0x55, (old2 & 0xFC) | 0x02);
			old4 = inp(0x3C6);
			if ((old4 & 0xC0) == 0x80)
			{
				Found = TRUE;
				*RamDac = DAC_BT485;
				*RamDac |= DAC_6_8_PROGRAM;
				wrinx(CRTC_IDX, 0x55, (old2 & 0xFC) | 0x00);
				if (Width8Check())
				{
					*RamDac |= DAC_8BIT;
				}
			}
			wrinx(CRTC_IDX, 0x55, (old2 & 0xFC) | 0x00);
			outp(0x3C8, old3);
		}
	}
	wrinx(CRTC_IDX, 0x55, old2);
	outp(0x3C6, old1);

	wrinx(CRTC_IDX, 0x39, lock2);
	wrinx(CRTC_IDX, 0x38, lock1);

	return(Found);
}

#ifdef __STDC__
void Probe_RamDac(int Chipset, int *RamDac)
#else
void Probe_RamDac(Chipset, RamDac)
int Chipset;
int *RamDac;
#endif
{
	Byte x, y, z, v, oldcommreg, oldpelreg;

	*RamDac = DAC_STANDARD;
	Ports[0] = CRTC_IDX;
	Ports[1] = CRTC_REG;
	EnableIOPorts(NUMPORTS, Ports);

	if (Chipset == CHIP_AL2101)
	{
		*RamDac = DAC_ALG1101;
		if (Width8Check())
		{
			*RamDac |= DAC_8BIT;
		}
		DisableIOPorts(NUMPORTS, Ports);
		return;
	}
	else if (SVGA_VENDOR(Chipset) == V_ATI)
	{
		if (ReadBIOS(0x44, &x, 1) != 1)
		{
			fprintf(stderr, "%s: Failed to read ATI BIOS data\n",
				MyName);
			DisableIOPorts(NUMPORTS, Ports);
			return;
		}
		if (x & 0x80)
		{
			*RamDac = DAC_ATI;
			if (Width8Check())
			{
				*RamDac |= DAC_8BIT;
			}
			DisableIOPorts(NUMPORTS, Ports);
			return;
		}
	}
	else if ((SVGA_VENDOR(Chipset) == V_CIRRUS) &&
		 (Chipset >= CHIP_CL5420) &&
		 (Chipset != CHIP_CL_UNKNOWN))
	{
		if (Chipset == CHIP_CL5420)
		{
			*RamDac = DAC_CIRRUSA;
		}
		else
		{
			*RamDac = DAC_CIRRUSB;
		}
		if (Width8Check())
		{
			*RamDac |= DAC_8BIT;
		}
		DisableIOPorts(NUMPORTS, Ports);
		return;
	}
	else if ((SVGA_VENDOR(Chipset) == V_S3) && (Chipset >= CHIP_S3_924))
	{
		if (S3_Bt485Check(RamDac))
		{
			DisableIOPorts(NUMPORTS, Ports);
			return;
		}
	}
	else if ((SVGA_VENDOR(Chipset) == V_COMPAQ)) {
		*RamDac = Compaq_RAMDAC();
		return;
	}
	dactopel();
	x = inp(0x3C6);
	do
	{
		y = x;
		x = inp(0x3C6);
	} while (x != y);
	z = x;
	x = dactocomm();
	y = 8;
	while ((x != 0x8E) && (y > 0))
	{
		x = inp(0x3C6);
		y--;
	}
	if (x == 0x8E)
	{
		*RamDac = DAC_SS24;
		dactopel();
	}
	else
	{
		(void)dactocomm();
		oldcommreg = inp(0x3C6);
		dactopel();
		oldpelreg = inp(0x3C6);
		x = oldcommreg ^ 0xFF;
		outp(0x3C6, x);
		(void)dactocomm();
		v = inp(0x3C6);
		if (v != x)
		{
			(void)dactocomm();
			x = oldcommreg ^ 0x60;
			outp(0x3C6, x);
			(void)dactocomm();
			v = inp(0x3C6);
			*RamDac = DAC_SIERRA15;
			if ((x & 0xE0) == (v & 0xE0))
			{
				x = inp(0x3C6);
				dactopel();
				*RamDac = DAC_SIERRA15_16;
				if (x == inp(0x3C6))
				{
					(void)dactocomm();
					outp(0x3C6, 0xFF);
					(void)dactocomm();
					if (inp(0x3C6) != 0xFF)
					{
						*RamDac = DAC_ACUMOS;
					}
					/*
					 * It's an AT&T RAMDAC; figure out
					 * which one.
					 */
					CheckATT(RamDac);
				}
			}
			else
			{
				(void)dactocomm();
				x = oldcommreg ^ 0xC0;
				outp(0x3C6, x);
				(void)dactocomm();
				v = inp(0x3C6);
				if ((x & 0xC0) == (v & 0xC0))
				{
					/*
					 * It's an AT&T RAMDAC; figure out
					 * which one.
					 */
					CheckATT(RamDac);
				}
			}
		}
		(void)dactocomm();
		outp(0x3C6, oldcommreg);
		dactopel();
		outp(0x3C6, oldpelreg);
	}
	if (Width8Check())
	{
		*RamDac |= DAC_8BIT;
	}
	DisableIOPorts(NUMPORTS, Ports);
	return;
}
