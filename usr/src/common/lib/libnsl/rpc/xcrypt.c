/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/xcrypt.c	1.1.7.1"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * xcrypt.c
 * Hex encryption/decryption and utility routines
 */

#include <stdio.h>
#include <sys/types.h>
#include "trace.h" 
#include <rpc/des_crypt.h>
#include <memory.h>

extern char *malloc();

extern char hex[];	/* forward */
static char hexval();

/*
 * Encrypt a secret key given passwd
 * The secret key is passed and returned in hex notation.
 * Its length must be a multiple of 16 hex digits (64 bits).
 */
xencrypt(secret, passwd)
	char *secret;
	char *passwd;
{
	char key[8];
	char ivec[8];
	char *buf;
	int err;
	int len;

	trace1(TR_xencrypt, 0);
	len = strlen(secret) / 2;
	buf = malloc((unsigned)len);
	hex2bin(len, secret, buf);
	passwd2des(passwd, key);
	memset(ivec, 0, 8);

	err = cbc_crypt(key, buf, len, DES_ENCRYPT | DES_HW, ivec);
	if (DES_FAILED(err)) {	
		free(buf);
		trace1(TR_xencrypt, 1);
		return (0);
	}
	bin2hex(len, (unsigned char *) buf, secret);
	free(buf);
	trace1(TR_xencrypt, 1);
	return (1);
}

/*
 * Decrypt secret key using passwd
 * The secret key is passed and returned in hex notation.
 * Once again, the length is a multiple of 16 hex digits
 */
xdecrypt(secret, passwd)
	char *secret;
	char *passwd;
{
	char key[8];
	char ivec[8];
	char *buf;
	int err;
	int len;

	trace1(TR_xdecrypt, 0);
	len = strlen(secret) / 2;
	buf = malloc((unsigned)len);

	hex2bin(len, secret, buf);
	passwd2des(passwd, key);	
	memset(ivec, 0, 8);

	err = cbc_crypt(key, buf, len, DES_DECRYPT | DES_HW, ivec);
	if (DES_FAILED(err)) {
		free(buf);
		trace1(TR_xdecrypt, 1);
		return (0);
	}
	bin2hex(len, (unsigned char *) buf, secret);
	free(buf);
	trace1(TR_xdecrypt, 1);
	return (1);
}

/*
 * Turn password into DES key
 */
passwd2des(pw, key)
	char *pw;
	char *key;
{
	int i;

	trace1(TR_passwd2des, 0);
	memset(key, 0, 8);
	for (i = 0; *pw; i = (i+1) % 8) {
		key[i] ^= *pw++ << 1;
	}
	des_setparity(key);
	trace1(TR_passwd2des, 1);
}

/*
 * Hex to binary conversion
 */
static
hex2bin(len, hexnum, binnum)
	int len;
	char *hexnum;
	char *binnum;
{
	int i;

	trace2(TR_hex2bin, 0, len);
	for (i = 0; i < len; i++) {
		*binnum++ = 16 * hexval(hexnum[2 * i]) +
					hexval(hexnum[2 * i + 1]);	
	}
	trace1(TR_hex2bin, 1);
}

/*
 * Binary to hex conversion
 */
static
bin2hex(len, binnum, hexnum)
	int len;
	unsigned char *binnum;
	char *hexnum;
{
	int i;
	unsigned val;

	trace2(TR_bin2hex, 0, len);
	for (i = 0; i < len; i++) {
		val = binnum[i];
		hexnum[i*2] = hex[val >> 4];
		hexnum[i*2+1] = hex[val & 0xf];
	}
	hexnum[len*2] = 0;
	trace1(TR_bin2hex, 1);
}

static char hex[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

static char
hexval(c)
	char c;
{
	trace1(TR_hexval, 0);
	if (c >= '0' && c <= '9') {
		trace1(TR_hexval, 1);
		return (c - '0');
	} else if (c >= 'a' && c <= 'z') {
		trace1(TR_hexval, 1);
		return (c - 'a' + 10);
	} else if (c >= 'A' && c <= 'Z') {
		trace1(TR_hexval, 1);
		return (c - 'A' + 10);
	} else {
		trace1(TR_hexval, 1);
		return (-1);
	}
}
