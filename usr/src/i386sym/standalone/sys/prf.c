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

#ident	"@(#)stand:i386sym/standalone/sys/prf.c	1.1"

#include <sys/types.h>

static void prf(const char *, uint *);
static void printn(ulong, int);

extern int putchar(int);

/*
 * int
 * printf(const char *, ...)
 * 	Scaled down version of C Library printf.
 *
 * Calling/Exit State:
 *	A unknown number of arguments are on the stack, corresponding
 *	to the format specifiers within the format string.  Use the
 *	address of the format string arg to find the address of the
 *	remaining argument list, to be passed along with the format
 *	string to prf(), which does all the work.
 *
 *	Always returns zero.  
 *
 * Remarks:
 * 	Used to print diagnostic information directly on console tty.
 * 	Since it is not interrupt driven, all system activities are
 * 	suspended.  Printf should not be used for chit-chat.
 *
 * 	One additional format: %b is supported to decode error registers.
 * 	Usage is:  printf("reg=%b\n", regval, "<base><arg>*");
 * 	Where <base> is the output base expressed as a control character,
 * 	e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * 	characters, the first of which gives the bit number to be inspected
 * 	(origin 1), and the next characters (up to a control character, i.e.
 * 	a character <= 32), give the name of the register.  Thus
 *		printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * 	would produce output: *	reg=2<BITTWO,BITONE>
 */
/*VARARGS1*/
int
printf(const char *fmt, ...)
{
	uint *x1 = (uint *)(&fmt + 1);

	prf(fmt, x1);
	return (0);
}

/*
 * static void
 * prf(const char *, uint *)
 * 	The real workhorse behind the printf function.
 *
 * Calling/Exit State:
 *	A unknown number of arguments are on the stack, corresponding
 *	to the format specifiers within the format string.  The first
 *	arg address of the format string to parse and dictate what
 *	data to display on the console.  The second arg is the address
 *	of a list of values to use for replacing the format directives
 *	with real values upon output; assume there is one per directive.
 *
 *	No return value.
 *
 * Remarks:
 *	This function assumes that (sizeof(int) == sizeof(long)).
 */
static void
prf(const char *fmt, uint *adx)
{
	int b, c, i;
	char *s, ch;
	int any;

loop:
	while ((c = *fmt++) != '%') {
		if(c == '\0')
			return;
		(void)putchar(c);
	}
again:
	c = *fmt++;
	switch (c) {

	case 'l':
		goto again;
	case 'x': case 'X':
		b = 16;
		goto number;
	case 'd': case 'D':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o': case 'O':
		b = 8;
number:
		printn(*(ulong *)adx, b);
		break;
	case 'c':
		ch = *(char *)adx;
		if (ch &= 0x7f)
			(void)putchar(ch);
		break;
	case 'b':
		b = *(int *)adx++;
		s = *(char **)adx;
		printn((ulong)b, *s++);
		any = 0;
		if (b) {
			(void)putchar('<');
			while ((i = *s++) != 0) {
				if (b & (1 << (i-1))) {
					if (any)
						(void)putchar(',');
					any = 1;
					for (; (c = *s) > 32; s++)
						(void)putchar(c);
				} else
					for (; *s > 32; s++)
						;
			}
			(void)putchar('>');
		}
		break;

	case 's':
		s = *(char **)adx;
		while ((c = *s++) != 0)
			(void)putchar(c);
		break;
	default:
		(void)putchar(c);
		goto loop;
	}
	adx++;
	goto loop;
}

/*
 * static void
 * printn(ulong, int)
 *	prints a number n in base b.
 *
 * Calling/Exit State:
 *	Assumes the resulting string will be <= 20 characters
 *	and that the radix arg is > 0.
 *
 *	Displays the resulting string on the console.
 *
 * 	No return value.
 *
 * Description:
 *	Determine the characters to print from least to most
 *	significant, depositing them in a buffer so they can
 *	be printed in the correct left-to-right readable form
 *	once the entire string has been determined.
 *
 * 	We don't use recursion to avoid deep kernel stacks.
 */
static void
printn(ulong n, int b)
{
	char prbuf[20];
	char *cp;

	if (b == 10 && (int)n < 0) {
		(void)putchar('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789ABCDEF"[n%b];
		n /= b;
	} while (n);
	do
		(void)putchar(*--cp);
	while (cp > prbuf);
}
