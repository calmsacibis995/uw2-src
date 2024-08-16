/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/authusekey.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * authusekey - decode a key from ascii and use it
 */
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>

/*
 * Types of ascii representations for keys.  "Standard" means a 64 bit
 * hex number in NBS format, i.e. with the low order bit of each byte
 * a parity bit.  "NTP" means a 64 bit key in NTP format, with the
 * high order bit of each byte a parity bit.  "Ascii" means a 1-to-8
 * character string whose ascii representation is used as the key.
 */
#define	KEY_TYPE_STD	1
#define	KEY_TYPE_NTP	2
#define	KEY_TYPE_ASCII	3

#define	STD_PARITY_BITS	0x01010101

extern int auth_parity();
extern void auth_setkey();

int
authusekey(keyno, keytype, str)
	u_long keyno;
	int keytype;
	char *str;
{
	u_long key[2];
	u_char keybytes[8];
	char *cp;
	char *xdigit;
	int len;
	int i;
	static char *hex = "0123456789abcdef";

	cp = str;
	len = strlen(cp);
	if (len == 0)
		return 0;

	switch(keytype) {
	case KEY_TYPE_STD:
	case KEY_TYPE_NTP:
		if (len != 16)		/* Lazy.  Should define constant */
			return 0;
		/*
		 * Decode hex key.
		 */
		key[0] = 0;
		key[1] = 0;
		for (i = 0; i < 16; i++) {
			if (!isascii(*cp))
				return 0;
			xdigit = strchr(hex, isupper(*cp) ? tolower(*cp) : *cp);
			cp++;
			if (xdigit == 0)
				return 0;
			key[i>>3] <<= 4;
			key[i>>3] |= (u_long)(xdigit - hex) & 0xf;
		}

		/*
		 * If this is an NTP format key, put it into NBS format
		 */
		if (keytype == KEY_TYPE_NTP) {
			for (i = 0; i < 2; i++)
				key[i] = ((key[i] << 1) & ~STD_PARITY_BITS)
				    | ((key[i] >> 7) & STD_PARITY_BITS);
		}

		/*
		 * Check the parity, reject the key if the check fails
		 */
		if (!auth_parity(key)) {
			return 0;
		}
		
		/*
		 * We can't find a good reason not to use this key.
		 * So use it.
		 */
		auth_setkey(keyno, key);
		break;
	
	case KEY_TYPE_ASCII:
		/*
		 * Make up key from ascii representation
		 */
		memset(keybytes, '\0', sizeof(keybytes));
		for (i = 0; i < 8 && i < len; i++)
			keybytes[i] = *cp++ << 1;
		key[0] = keybytes[0] << 24 | keybytes[1] << 16
		    | keybytes[2] << 8 | keybytes[3];
		key[1] = keybytes[4] << 24 | keybytes[5] << 16
		    | keybytes[6] << 8 | keybytes[7];
		
		/*
		 * Set parity on key
		 */
		(void)auth_parity(key);

		/*
		 * Now set key in.
		 */
		auth_setkey(keyno, key);
		break;
	
	default:
		/* Oh, well */
		return 0;
	}

	return 1;
}
