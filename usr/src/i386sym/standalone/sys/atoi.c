/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/sys/atoi.c	1.1"

/*
 * int
 * atoi(char *)
 * 	decimal ascii to integer conversion.
 *
 * Calling/Exit State:
 *	"p" is an ascii string containing a decimal number,
 *	possibly proceeded by whitespace and a +/- sign.
 *
 *	Returns the internal integer representation of the number.
 */
int
atoi(char *p)
{
	int n;
	int f;

	n = 0;
	f = 0;
	for (;;p++) {
		switch (*p) {
		case ' ':
		case '\t':
			continue;
		case '-':
			f++;
			p++;
			break;
		case '+':
			p++;
			break;
		}
		break;
	}
	while (*p >= '0' && *p <= '9')
		n = n*10 + *p++ - '0';
	return (f? -n: n);
}

/*
 * int
 * stoi(char **)
 * 	decimal or hex ascii to integer conversion.
 *
 * Calling/Exit State:
 *	"sp" is an ascii string pointer containing either
 *	a decimal number, or hexidecimal number.  Hex numbers
 *	are prefixed with "0x".
 *
 *	Updates "sp" to the character after the number string.
 *
 *	Returns the internal integer representation of the number.
 */
int
stoi (char **sp)
{
	char	*np = *sp;
	int	val = 0;

	if ((*np == '0') && (*(np + 1) == 'x')) {
		np += 2;
		for (;;) {
			if ((*np >= '0') && (*np <= '9'))
				val = (val * 16) + (int)(*np - '0');
			else if ((*np >= 'a') && (*np <= 'f'))
				val = (val * 16) + (int)(*np - ('a' - 10));
			else
				break;
			np++;
		}
	}
	else {
		while ((*np >= '0') && (*np <= '9'))
			val = val * 10 + *np++ - '0';
	}

	*sp = np;

	return (val);
}
