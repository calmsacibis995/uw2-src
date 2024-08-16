/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/encodefile.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)encodefile.c	1.3 'attmail mail(1) command'"
#ifdef MAIN	/* used to build standalone version */
# include <stdio.h>
# include <string.h>
#else
#include "mail.h"
#endif
/*
    NAME
	encode_file_to_* - change encoding of a FILE* from raw to base64 or quoted-printable

    SYNOPSIS
	long encode_file_to_base64(FILE *in, FILE *out, int portablenewline)
	long encode_file_to_quoted_printable(FILE *in, FILE *out)
	long encode_string_to_quoted_printable(string *in, string *out, int qp_for_hdr)

    DESCRIPTION
	Copy from one stream to another, changing the encoding to either "base64"
	or "quoted-printable". Return the number of bytes written.

#ifdef MAIN
    NAME
	encodebody - change encoding of stdin to base64 or quoted-printable

    SYNOPSIS
	encodebody [-q] < input > output

    DESCRIPTION
	encodebody encodes the input to base64 or, if -q is specified, quoted printable.
	The converted stream is written to standard output.
#endif
*/

static int getc_expand_nl(in, portablenewline)
FILE *in;
int portablenewline;
{
    static int in_crlf = 0;
    int c;
    if (in_crlf)
	{
	in_crlf = 0;
	return '\n';
	}
    c = getc(in);
    if ((c == '\n') && portablenewline)
	{
	in_crlf = 1;
	return '\r';
	}
    return c;
}

static char base64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

long encode_file_to_base64(in, out, portablenewline)
FILE *in;
FILE *out;
int portablenewline;
{
    int encoding = 1;
    unsigned int c[3];
    int quadcount = 0;
    long ret = 0;

    while (encoding)
	{
	int converted = 0;
	int i;

	/* read 3 characters to be converted into 4 */
	for (i = 0; i < 3; i++)
	    {
	    int ch = getc_expand_nl(in, portablenewline);
	    if (ch == EOF)
		c[i] = 0;
	    else
		{
		c[i] = ch;
		converted++;
		}
	    }
	if (converted == 0)
	    encoding = 0;

	else	/* convert the characters */
	    {
	    (void) putc(base64[c[0] >> 2], out);
	    (void) putc(base64[((c[0] & 03) << 4) | ((c[1] & 0xF0) >> 4)], out);
	    ret += 2;

	    if (converted > 1)
		{
		(void) putc(base64[((c[1] & 0xF) << 2) | ((c[2] & 0xC0) >> 6)], out);
		ret++;
		}

	    if (converted > 2)
		{
		(void) putc(base64[c[2] & 0x3F], out);
		ret++;
		}

	    switch (converted)
		{
		case 1:
		    (void) putc('=', out);
		    ret++;
		    /* FALLTHROUGH */

		case 2:
		    (void) putc('=', out);
		    ret++;
		    break;
		}
	    }

	/* add a newline once in a while */
	if (quadcount++ > 16)
	    {
	    (void) putc('\n', out);
	    ret++;
	    quadcount = 0;
	    }
	}

    /* add a final newline */
    if (quadcount > 0)
	{
	(void) putc('\n', out);
	ret++;
	}

    return ret;
}

/* which characters absolutely have to be encoded? */
/* 0 == no encoding. 1 == encode */
static char encode_map[128] =
    {
    /* nul soh stx etx eot enq ack bel bs  ht  nl  vt  np  cr  so  si  */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* dle dc1 dc2 dc3 dc4 nak syn etb can em  sub esc fs  gs  rs  us  */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* sp   !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /  */
	0,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?  */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,
    /*  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O  */
	1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*  P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _  */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,
    /*  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o  */
	1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~  del */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1
    };

static const char hexdigit[] = "0123456789ABCDEF";

/* convert the input stream into quoted printable. Keep lines < ~72 characters. */
long encode_file_to_quoted_printable(in, out)
FILE *in;
FILE *out;
{
    int c;
    int linelength = 0;
    int lastchar = '\n';
    long ret = 0;

    while ((c = getc(in)) != EOF)
	{
	/* Output newlines as newlines. However, if the previous character was a */
	/* blank, output a soft newline first so that there is no trailing white */
	/* space on the line. Such white space can be wiped out by some gateways. */
	if (c == '\n')
	    {
	    if ((lastchar == ' ') || (lastchar == '\t'))
		{
		(void) putc('=', out);
		(void) putc('\n', out);
		ret += 2;
		}
	    (void) putc(c, out);
	    ret++;
	    linelength = 0;
	    }

	/* encoded characters print as =XX, where XX is the hex value of the character */
	else if ((c > 127) || encode_map[c])
	    {
	    (void) putc('=', out);
	    (void) putc(hexdigit[c % 16], out);
	    (void) putc(hexdigit[c / 16], out);
	    ret += 3;
	    linelength += 3;
	    }

	/* normal characters output as themselves */
	else
	    {
	    (void) putc(c, out);
	    ret++;
	    linelength++;
	    }

	/* output soft newlines for long lines */
	if (linelength > 72)
	    {
	    linelength = 0;
	    (void) putc('=', out);
	    (void) putc('\n', out);
	    ret += 2;
	    }
	lastchar = c;
	}

    /* if a newline wasn't just output, end the stream with a soft newline */
    if (linelength > 0)
	{
	(void) putc('=', out);
	(void) putc('\n', out);
	ret += 2;
	}

    return ret;
}

/* convert the string into quoted printable. */
long encode_string_to_quoted_printable(in, out, qp_for_hdr)
string *in;
string *out;
int qp_for_hdr;
{
    int linelength = 0;
    int lastchar = '\n';
    long ret = 0;
    const unsigned char *p = (unsigned char*)s_to_c(in);
    const unsigned char *endp = p + s_curlen(in);

    for ( ; p < endp; p++)
	{
	int c = *p;
	/* Output newlines as newlines. However, if the previous character was a */
	/* blank, output a soft newline first so that there is no trailing white */
	/* space on the line. Such white space can be wiped out by some gateways. */
	if (c == '\n')
	    {
	    if ((lastchar == ' ') || (lastchar == '\t'))
		{
		s_putc(out, '=');
		s_putc(out, '\n');
		ret += 2;
		}
	    s_putc(out, c);
	    ret++;
	    linelength = 0;
	    }

	/* encoded characters print as =XX, where XX is the hex value of the character */
	else if ((c > 127) || encode_map[c] || (qp_for_hdr && (c == '?')))
	    {
	    s_putc(out, '=');
	    s_putc(out, hexdigit[c % 16]);
	    s_putc(out, hexdigit[c / 16]);
	    ret += 3;
	    linelength += 3;
	    }

	/* normal characters output as themselves */
	else
	    {
	    s_putc(out, c);
	    ret++;
	    linelength++;
	    }

	/* output soft newlines for long lines */
	if (linelength > 72)
	    {
	    linelength = 0;
	    ret += 2;
	    s_putc(out, '=');
	    s_putc(out, '\n');
	    if (qp_for_hdr)
		{
		s_putc(out, ' ');
		ret++;
		linelength++;
		}
	    }
	lastchar = c;
	}

    /* if a newline wasn't just output, end the stream with a soft newline */
    if ((linelength > 0) && !qp_for_hdr)
	{
	s_putc(out, '=');
	s_putc(out, '\n');
	ret += 2;
	}

    return ret;
}

#ifdef MAIN
main(argc, argv)
int argc;
char **argv;
{
    if ((argc == 2) && (strcmp(argv[1], "-q") == 0))
	encode_file_to_quoted_printable(stdin, stdout);
    else
	encode_file_to_base64(stdin, stdout, 0);
    return 0;
}
#endif /* MAIN */
