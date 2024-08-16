/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/fnt/fntdrv.c	1.1"
#ident	"$Header: $"

/*
 * fntdrv.c, Memory Resident Font Module driver code.
 */

#include <io/ldterm/euc.h>
#include <io/ws/chan.h>
#include <io/ws/ws.h>
#include <util/cmn_err.h>
#include <io/fnt/fnt.h>
#include <io/gsd/gsd.h>
#include <util/types.h>

char	fntid_string[] = "Memory Resident Font Module Version 1.0";
char	fntcopyright[] = "Copyright (c) 1994 Novell, Inc.";

/*
 * int fntinit(void) - called from fnt_load()
 *	Initializes fnt.
 *
 * Calling/Exit status:
 *
 */
int
fntinit(void)
{
	cmn_err(CE_CONT, "%s\n%s\n(%s, %s, %s)\n\n",
		fntid_string, fntcopyright,
		codeset0registry, codeset1registry, codeset2registry);

	Gs.fnt_getbitmap = fnt_getbitmap;
	Gs.fnt_unlockbitmap = fnt_unlockbitmap;

	fnt_init_flg = 1;
}

unsigned char EmptyBitMap[] =
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

unsigned char *
fnt_getbitmap(channel_t *chp, wchar_t ch)
{
	int	eucw, scrw;

	switch(ch & EUCMASK)
	{
	case P00: /* codeset 0 */
		eucw = EUC_INFO(chp)->eucw[0];
		scrw = EUC_INFO(chp)->scrw[0];

		if (eucw == 1) {
			if (ch >= 32 && ch < 128)
				return (codeset0data[0] + ((ch-32)*16*scrw));
			else if (ch >= 160 && ch < 256) {
				unsigned char	*data = altcharsetdata[0];
				if (data != 0)
					return (data + ((ch-160)*16*scrw));
			}
		}
		break;
	case P11: /* codeset 1 */
		eucw = EUC_INFO(chp)->eucw[1];
		scrw = EUC_INFO(chp)->scrw[1];

		if (eucw == 1)
			;
		else if (eucw == 2) {
			int		row = (ch >> 8) & 0xff;
			int		col = ch & 0xff;
			unsigned char	*data;

			if (row < 32 || row >= 128)
				break;
			if (col < 32 || col >= 128)
				break;
			data = codeset1data[row-32];
			if (data == 0)
				break;
			return (data + ((col-32)*16*scrw));
		}
		break;
	case P01: /* codeset 2 */
		eucw = EUC_INFO(chp)->eucw[2];
		scrw = EUC_INFO(chp)->scrw[2];

		if (eucw == 1) {
			ch &= 0x7f;
                        if (ch < 32 || ch > 127)
                                break;
                        return (codeset2data[0] + ((ch-32)*16*scrw));
		}
		break;
	default:
		cmn_err(CE_WARN, "codeset not implemented for character 0x%x\n",
			ch);
	}

	return EmptyBitMap;
}

/*
 * void
 * fnt_unlockbitmap(channel_t *chp, wchar_t)
 *
 * Calling/Exit status:
 *
 * Description:
 *	This routine is called to unlock the font cache line.
 *	This implementation uses in core fonts and therefore
 *	this routine is a dummy.
 */
void
fnt_unlockbitmap(channel_t *chp, wchar_t ch)
{
	/* do nothing */
}
