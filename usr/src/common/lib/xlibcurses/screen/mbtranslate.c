/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/mbtranslate.c	1.3.2.3"
#ident  "$Header: mbtranslate.c 1.2 91/06/26 $"
#include	"curses_inc.h"



/*
**	Translate process code to byte-equivalent
**	Return the length of the byte-equivalent string
*/

/*
**	use wctomb() instead of _code2byte(code,bytes)
*/


/*
**	Translate a set of byte to a single process code
*/

/*
**	use mbtowc() instead of wchar_t _byte2code(bytes)
*/


/*
**	Translate a string of wchar_t to a byte string.
**	code: the input code string
**	byte: if not NULL, space to store the output string
**	n: maximum number of codes to be translated.
*/
char	*_strcode2byte(code,byte,n)
wchar_t	*code;
char	*byte;
int	n;
	{
	register char	*bufp;
	register wchar_t *endcode;
	static char	*buf;
	static int	bufsize;

	/* compute the length of the code string */
	if(n < 0)
		for(n = 0; code[n] != 0; ++n)
			;

	/* get space to store the translated string */
	if(!byte && (n*CSMAX+1) > bufsize)
		{
		if(buf)
			free(buf);
		bufsize = n*CSMAX+1;
		if((buf = malloc(bufsize*sizeof(char))) == NULL)
			bufsize = 0;
		}

	/* no space to do it */
	if(!byte && !buf)
		return NULL;

	/* start the translation */
	bufp = byte ? byte : buf;
	endcode = code+n;
	while(code < endcode && *code)
		{
		bufp += wctomb(bufp, *code & TRIM);
		++code;
		}
	*bufp = '\0';

	return byte ? byte : buf;
	}



/*
**	Translate a byte-string to a wchar_t string.
*/
wchar_t	*_strbyte2code(byte,code,n)
char	*byte;
wchar_t	*code;
int	n;
	{
	register char	*endbyte;
	register wchar_t *bufp;
	static wchar_t	*buf;
	static int	bufsize;

	if(n < 0)
		for(n = 0; byte[n] != '\0'; ++n)
			;

	if(!code && (n+1) > bufsize)
		{
		if(buf)
			free((char *)buf);
		bufsize = n+1;
		if((buf = (wchar_t *)malloc(bufsize*sizeof(wchar_t))) == NULL)
			bufsize = 0;
		}

	if(!code && !buf)
		return NULL;

	bufp = code ? code : buf;
	endbyte = byte+n;

	while(byte < endbyte && *byte)
		{
		register int	type, width, length;
		wchar_t		wchar;

		type = TYPE(*byte & 0377);
		width = cswidth[type];
		if(type == 1 || type == 2)
			width++;

		if(byte + width <= endbyte) {
			length = mbtowc(&wchar, byte, width);
			*bufp++ = wchar;
		}

		byte += width;
		}
	*bufp = 0;

	return code ? code : buf;
	}
