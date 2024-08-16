/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mhs/decrypt.c	1.2"
/*
 * NW-MAIL LOGIN
 */
static char SccsID[] = "@(#)mhslogin.c	1.4 92/09/2116:48:04 92/10/3015:44:59";
/*
(C) Unpublished Copyright of Univel, Inc.  All Rights Reserved.

No part of this file may be duplicated, revised, translated,  
localized or modified in any manner or compiled, linked or uploaded or
downloaded to or  from any computer system without the prior written consent
of Novell, Inc.
*/

#include        <stdio.h>
#include        <string.h>
#include	<stdlib.h>
#include	<ctype.h>

char mask[]  = "Y7^%8*(!Pdzxc%^&$#@zdslo012.,";
char mask1[] = "ZCB8&%#khec0-+][{';/@DKzaqrsm";

unsigned char rol(unsigned char x)
{
	unsigned char hb;

	hb = x >> 7;
	x <<= 1;
	x |= hb;
	return x;
}

unsigned char ror(unsigned char x)
{
	unsigned char lb;

	lb = x << 7;
	x >>= 1;
	x |= lb;
	return x;
}

char *decrypt_passwd(unsigned char *enc, int len)
{
	char *unenc,*p;
	int offs = 0;
	int masklen = sizeof(mask);

	p = unenc = malloc(len+1);
	memcpy(unenc,enc,len);
	*(unenc+len) = 0;
	while(len) {
		*unenc = rol(*unenc);
		*unenc ^= mask1[offs];
		*unenc = rol(*unenc);
		*unenc ^= mask[offs];
		*unenc = ror(*unenc);
		*unenc = ror(*unenc);
		unenc++;
		offs++;
		if (offs == masklen) offs = 0;
		len--;
	}
	return p;
}

int tohex(char c)
{
	int val = 0;
	char *xdigits = "0123456789ABCDEF";

	while(*xdigits) {
		if (toupper(c) == *xdigits) return val;
		val++;
		xdigits++;
	}
}

void ascii_to_hex(unsigned char *dest, char *src)
{
	unsigned char x;

	while(*src) {
		*dest++ = (tohex(*src++) << 4) | tohex(*src++);
	}

	*dest = '\0';
}

int main(int argc, char **argv)
{
	char *passwd;
	unsigned char unpasswd[40];

	ascii_to_hex(unpasswd,argv[1]);
	passwd = decrypt_passwd(unpasswd,strlen((char *)unpasswd));
	printf("%s\n", passwd);
}
