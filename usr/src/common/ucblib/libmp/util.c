#ident	"@(#)ucb:common/ucblib/libmp/util.c	1.3"
#ident	"$Header: $"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/* 	Portions Copyright(c) 1988, Sun Microsystems Inc.	*/
/*	All Rights Reserved					*/


/* LINTLIBRARY */

#include <stdio.h>
#include <mp.h>

extern char *malloc();

_mp_move(a,b) 
	MINT *a,*b;
{	
	int i,j;

	xfree(b);
	b->len = a->len;
	if ((i = a->len) < 0) {
		i = -i;
	}
	if (i == 0) {
		return;
	}
	b->val = xalloc(i,"_mp_move");
	for(j = 0;j < i; j++) {
		b->val[j]=a->val[j];
	}
}


/* ARGSUSED */
/* VARARGS */
short *
xalloc(nint,s) 
	int nint;	
	char *s;
{	
	short *i;

	i = (short *) malloc((unsigned) 
	    sizeof(short)*((unsigned)nint + 2)); /* ??? 2 ??? */
#ifdef DEBUG
	(void) fprintf(stderr, "%s: %o\n",s,i);
#endif
	if (i == NULL) {
		_mp_fatal("mp: no free space");
	}
	return(i);
}


_mp_fatal(s) 
	char *s;
{
	(void) fprintf(stderr,"%s\n",s);
	(void) fflush(stdout);
	sleep(2);
	abort();
}


xfree(c) 
	MINT *c;
{
#ifdef DBG
	(void) fprintf(stderr, "xfree ");
#endif
	if (c->len != 0) {
		free((char *) c->val);
		c->len = 0;
	}
}



_mp_mcan(a) 
	MINT *a;
{	
	int i,j;

	if ((i = a->len) == 0) {
		return;
	}
	if (i <0) {
		i = -i;
	}
	for (j = i; j > 0 && a->val[j-1] == 0; j--)
		;
	if (j == i) {
		return;
	}
	if (j == 0) {	
		xfree(a);
		return;
	}
	if (a->len > 0) {
		a->len = j;
	} else {
		a->len = -j;
	}
}


MINT *
itom(n)
	int n;
{	
	MINT *a;

	a = (MINT *) malloc((unsigned) sizeof(MINT));
	if (n > 0) {	
		a->len = 1;
		a->val = xalloc(1,"itom1");
		*a->val = n;
	} else if ( n < 0) {	
		a->len = -1;
		a->val = xalloc(1,"itom2");
		*a->val = -n;
	} else {	
		a->len = 0;
	}
	return(a);
}


	
mcmp(a,b) 
	MINT *a,*b;
{	
	MINT c;
	int res;

	_mp_mcan(a);
	_mp_mcan(b);
	if (a->len != b->len) {
		return(a->len - b->len);
	}
	c.len = 0;
	msub(a,b,&c);
	res = c.len;
	xfree(&c);
	return(res);
}

/*
 * Convert hex digit to binary value
 */
static int
xtoi(c)
	char c;
{
	if (c >= '0' && c <= '9') {
		return(c - '0');
	} else if (c >= 'a' && c <= 'f') {
		return(c - 'a' + 10);
	} else if (c >= 'A' && c <= 'F') {
		return(c - 'A' + 10);
	} else {
		return(-1);
	}
}



/*
 * Convert hex key to MINT key
 */
MINT *
xtom(key)
	char *key;
{	
	int digit;
	MINT *m = itom(0);
	MINT *d;
	MINT *sixteen;

	sixteen = itom(16);	
	for (; *key; key++) {
		digit = xtoi(*key);
		if (digit < 0) {
			return(NULL);
		}
		d = itom(digit);
		mult(m,sixteen,m);
		madd(m,d,m);
		mfree(d);
	}
	mfree(sixteen);
	return(m);
}

static char
itox(d)
	short d;
{
	d &= 15;
	if (d < 10) {
		return('0' + d);
	} else {
		return('a' - 10 + d);
	}
}

/*
 * Convert MINT key to hex key
 */
char *
mtox(key)
	MINT *key;
{
	MINT *m = itom(0);
	MINT *zero = itom(0);
	short r;
	char *p;
	char c;
	char *s;
	char *hex;
	int size;

#	define BASEBITS	(8*sizeof(short) - 1)

	if (key->len >= 0) {
		size = key->len;
	} else {	
		size = -key->len; 
	}
	hex = malloc((unsigned) ((size * BASEBITS + 3)) / 4 + 1);
	if (hex == NULL) {
		return(NULL);
	}
	_mp_move(key,m);
	p = hex;
	do {
		sdiv(m,16,m,&r);
		*p++ = itox(r);
	} while (mcmp(m,zero) != 0);	
	mfree(m);
	mfree(zero);

	*p = 0;
	for (p--, s = hex; s < p; s++, p--) {
		c = *p;
		*p = *s;
		*s = c;
	}
	return(hex);
}

/*
 * Deallocate a multiple precision integer
 */
void
mfree(a)
	MINT *a;
{
	xfree(a);
	free((char *)a);
}
