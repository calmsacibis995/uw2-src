/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/Utils.c	1.3"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Utils.c,v 2.2 1993/09/27 12:23:29 dawes Exp $ */

#include "Probe.h"
#include "AsmMacros.h"

/*
 * Return the byte value of register 'port'
 */
#ifdef __STDC__
Byte inp(Word port)
#else
Byte inp(port)
Word port;
#endif
{
	return(inb(port));
}

/*
 * Return the word value of register 'port'
 */
#ifdef __STDC__
Word inpw(Word port)
#else
Word inpw(port)
Word port;
#endif
{
	return(inw(port));
}

/*
 * Return the long value of register 'port'
 */
#ifdef __STDC__
unsigned long inpl(unsigned long port)
#else
unsigned long inpl(port)
unsigned long port;
#endif
{
	return(inl(port));
}

/*
 * Set the byte register 'port' to 'val'
 */
#ifdef __STDC__
void outp(Word port, Byte val)
#else
void outp(port, val)
Word port;
Byte val;
#endif
{
	outb(port, val);
}

/*
 * Set the word register 'port' to 'val'
 */
#ifdef __STDC__
void outpw(Word port, Word val)
#else
void outpw(port, val)
Word port, val;
#endif
{
	outw(port, val);
}

/*
 * Set the long register 'port' to 'val'
 */
#ifdef __STDC__
void outpl(unsigned long port, unsigned long val)
#else
void outpl(port, val)
unsigned long port, val;
#endif
{
	outl(port, val);
}

/*
 * Read byte register at 'port', index 'index'
 */
#ifdef __STDC__
Byte rdinx(Word port, Byte index)
#else
Byte rdinx(port, index)
Word port;
Byte index;
#endif
{
	Byte tmp;
	Word Port = (inp(0x3CC) & 0x01) ? 0x3DA : 0x3BA;

	if (port == 0x3C0)
	{
		EnableIOPorts(1, &Port);
		tmp = inb(Port);	/* Reset Attribute Reg flip-flop */
		DisableIOPorts(1, &Port);
	}
	outb(port, index);
	return(inb(port+1));
}

/*
 * Set the byte register 'port', index 'index' to 'val'
 */
#ifdef __STDC__
void wrinx(Word port, Byte index, Byte val)
#else
void wrinx(port, index, val)
Word port;
Byte index;
Byte val;
#endif
{
	outb(port, index);
	outb(port+1, val);
}

/*
 * In byte register 'port', index 'index', set the bit that are 1 in 'mask'
 * to the value of the corresponding bits in 'mask'
 */
#ifdef __STDC__
void modinx(Word port, Byte index, Byte mask, Byte newval)
#else
void modinx(port, index, mask, newval)
Word port;
Byte index; 
Byte mask; 
Byte newval;
#endif
{
	Byte tmp;

	tmp = (rdinx(port, index) & ~mask) | (newval & mask);
	wrinx(port, index, tmp);
}

/*
 * Return TRUE iff the bits in 'mask' of register 'port' are read/write.
 */
#ifdef __STDC__
Bool tstrg(Word port, Byte mask)
#else
Bool tstrg(port, mask)
Word port;
Byte mask;
#endif
{
	Byte old, new1, new2;

	old = inp(port);
	outp(port, (old & ~mask));
	new1 = inp(port) & mask;
	outp(port, (old | mask));
	new2 = inp(port) & mask;
	outp(port, old);
	return((new1==0) && (new2==mask));
}

/*
 * Returns true iff the bits in 'mask' of register 'port', index 'index'
 * are read/write.
 */
#ifdef __STDC__
Bool testinx2(Word port, Byte index, Byte mask)
#else
Bool testinx2(port, index, mask)
Word port;
Byte index;
Byte mask;
#endif
{
	Byte old, new1, new2;

	old = rdinx(port, index);
	wrinx(port, index, (old & ~mask));
	new1 = rdinx(port, index) & mask;
	wrinx(port, index, (old | mask));
	new2 = rdinx(port, index) & mask;
	wrinx(port, index, old);
	return((new1==0) && (new2==mask));
}

/*
 * Returns true iff all bits of register 'port', index 'index' are read/write
 */
#ifdef __STDC__
Bool testinx(Word port, Byte index)
#else
Bool testinx(port, index)
Word port;
Word index;
#endif
{
	return(testinx2(port, index, 0xFF));
}

/*
 * Force DAC back to PEL mode
 */
#ifdef __STDC__
void dactopel(void)
#else
void dactopel()
#endif
{
	(void)inp(0x3C8);
}

/*
 * Enter command mode of HiColor DACs (result is stored in extern DACcommand)
 */
#ifdef __STDC__
Byte dactocomm(void)
#else
Byte dactocomm()
#endif
{
	dactopel();
	(void)inp(0x3C6);
	(void)inp(0x3C6);
	(void)inp(0x3C6);
	return(inp(0x3C6));
}

/*
 * Check chip descriptor against exclusion list
 */
#ifdef __STDC__
Bool Excluded(Range *ExList, Chip_Descriptor *Chip, Bool Mask10)
#else
Bool Excluded(ExList, Chip, Mask10)
Range *ExList;
Chip_Descriptor *Chip;
Bool Mask10;
#endif
{
	int i, j;
	Word mask = (Mask10 ? 0x3FF : 0xFFFF);

	if (Chip->num_ports == 0)
	{
		return(FALSE);
	}
	if (Chip->ports[0] == 0)
	{
		/* Fill in CRTC */
		Chip->ports[0] = CRTC_IDX;
		Chip->ports[1] = CRTC_REG;
	}
	for (i=0; i < Chip->num_ports; i++)
	{
		for (j=0; ExList[j].lo != (Word)-1; j++)
		{
			if (ExList[j].hi == (Word)-1)
			{
				/* single port */
				if ((Chip->ports[i] & mask) == ExList[j].lo)
				{
					return(TRUE);
				}
			}
			else
			{
				/* range */
				if (( (Word)(Chip->ports[i] & mask) >= ExList[j].lo) &&
				    ( (Word)(Chip->ports[i] & mask) <= ExList[j].hi))
				{
					return(TRUE);
				}
			}
		}
	}
	return(FALSE);
}

#ifdef __STDC__
int StrCaseCmp(char *s1, char *s2)
#else
int StrCaseCmp(s1, s2)
char *s1;
char *s2;
#endif
{
	char c1, c2;

	if (*s1 == 0)
		if (*s2 == 0)
			return(0);
		else
			return(1);

	c1 = (isupper(*s1) ? tolower(*s1) : *s1);
	c2 = (isupper(*s2) ? tolower(*s2) : *s2);
	while (c1 == c2)
	{
		if (c1 == '\0')
			return(0);
		s1++; 
		s2++;
		c1 = (isupper(*s1) ? tolower(*s1) : *s1);
		c2 = (isupper(*s2) ? tolower(*s2) : *s2);
	}
	return(c1 - c2);
}

#ifdef __STDC__
unsigned int StrToUL(const char *str)
#else
unsigned int StrToUL(str)
char *str;
#endif
{
	int base = 10;
	const char *p = str;
	unsigned int tot = 0;

	if (*p == '0')
	{
		p++;
		if (*p == 'x')
		{
			p++;
			base = 16;
		}
		else
		{
			base = 8;
		}
	}
	while (*p)
	{
		if ((*p >= '0') && (*p <= ((base == 8)?'7':'9')))
		{
			tot = tot * base + (*p - '0');
		}
		else if ((base == 16) && (*p >= 'a') && (*p <= 'f'))
		{
			tot = tot * base + 10 + (*p - 'a');
		}
		else if ((base == 16) && (*p >= 'A') && (*p <= 'F'))
		{
			tot = tot * base + 10 + (*p - 'A');
		}
		else
		{
			return(tot);
		}
		p++;
	}
	return(tot);
}
