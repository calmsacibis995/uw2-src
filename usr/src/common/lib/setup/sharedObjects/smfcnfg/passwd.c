/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:sharedObjects/smfcnfg/passwd.c	1.2"
#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>

static unsigned char binarystr[128];


static unsigned char
convertNibble (unsigned char c)
{
	if (c >= '0' && c <= '9')
		return (c & 0xf);

	c = (unsigned char)tolower ((int)c);

	if (c >= 'a' && c <= 'f')
		return ((c & 0xf) + 9);
	else
		return (0);

}	/*  End  convertNibble () */



static char mask[]  = "Y7^%8*(!Pdzxc%^&$#@zdslo012.,";
static char mask1[] = "ZCB8&%#khec0-+][{';/@DKzaqrsm";

static unsigned char
rol (unsigned char x)
{
	unsigned char hb;

	hb = x >> 7;
	x <<= 1;
	x |= hb;
	return (x);
}



static unsigned char
ror (unsigned char x)
{
	unsigned char lb;

	lb = x << 7;
	x >>= 1;
	x |= lb;
	return (x);
}



static char *
decrypt_passwd (unsigned char *enc, int len)
{

	char	*p;
	char	*unenc;
	int	offs = 0;
	int	masklen = sizeof(mask);

	p = unenc = (char *)enc;

	while (len)
	{
		*unenc = rol (*unenc);
		*unenc ^= mask1[offs];
		*unenc = rol (*unenc);
		*unenc ^= mask[offs];
		*unenc = ror (*unenc);
		*unenc = ror (*unenc);
		unenc++;
		offs++;

		if (offs == masklen)
			offs = 0;
		len--;
	}
	return (p);
}





/*
	This routine will encrypt a user specified password and return it in
	a new buffer.
*/
static unsigned char *
encrypt_passwd (char *passwd)
{
	unsigned char *p;
	static unsigned char enc[129];
	int offs = 0;
	int masklen = sizeof(mask);

	(void) strncpy ((char *)enc, passwd, sizeof(enc));
	p = enc;

	while (*p)
	{
		*p = rol (*p);
		*p = rol (*p);
		*p ^= mask[offs];
		*p = ror (*p);
		*p ^= mask1[offs];
		*p = ror (*p);
		p++;
		offs++;

		if (offs == masklen)
			offs = 0;
	}
	return (enc);
}


char *
decryptPasswd (char *passwd)
{
	int len = 0;
	unsigned char c1, c2;
	unsigned char *dest = binarystr;


	while (*passwd)
	{
		c1 = *passwd++;
		c2 = *passwd++;
		*dest++ = (convertNibble (c1) << 4) + convertNibble (c2);
		len++;
	}

	return (decrypt_passwd (binarystr, len));

}	/*  End  decryptPasswd () */


unsigned char *
encryptPasswd (char *passwd)
{
	unsigned char *enc;
	int i, len;
	static unsigned char encstr[256];

	enc = encrypt_passwd (passwd);
	len = strlen (passwd);

	for (i = 0; i < len ; i++)
	{
		(void) sprintf ((char *)&encstr[2*i], "%02x",enc[i]);
	}
	return (encstr);
}

