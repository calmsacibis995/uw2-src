/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailcomp.c	1.6.4.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mailcomp.c	1.10 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	mailcompile - compile mail surrogate regular expressions

    SYNOPSIS
	char *mailcompile(string *pattern, int *retnbra)

    DESCRIPTION
	 mailcompile() will compile the regular expression and
	 return the compiled version. Before passing the regular
	 expression on to r_recomp(), it will make the following
	 changes to the pattern:

		%D	   ->	local domain name
		%L	   ->	local system name
		%U	   ->	local uname
		%X	   ->	smarter host
		%[CcHlnRS] ->	disallowed
		%x	   ->	x is [a-z], Mgetenv(%x)
		%{DNODOT}  ->	local domain name w/o leading dot
		%{xyz}	   ->	Mgetenv(xyz)
		%[xyz]	   ->	disallowed
		%%	   ->	%

    RETURNS
	A pointer to the compiled expression will be returned.
	The number of bracketed expressions will be stored
	into retnbra.

	If the compilation failed, NULL will be returned.
*/

#define BS '\\'

static void nomagic ARGS((string *outpattern, char *str));

re_re *mailcompile(inpattern, retnbra)
string *inpattern;
int *retnbra;
{
    static const char pn[] = "mailcompile";
    string *outpattern = s_new();
    register char *ip = s_to_c(inpattern);
    re_re *regex;
    unsigned char re_map[256];
    int i;

    Dout(pn, 5, "inpattern = '%s'\n", s_to_c(inpattern));
    for ( ; *ip; ip++)
	switch (*ip)
	    {
	    case '%':
		switch (*++ip)
		    {
		    case 'D':	/* local domain name */
			nomagic(outpattern, maildomain());
			break;

		    case 'L':	/* local system name */
			nomagic(outpattern, thissys);
			break;

		    case 'U':	/* uname */
			nomagic(outpattern, mailsystem(1));
			break;

		    case 'X':
			nomagic(outpattern, Mgetenv("SMARTERHOST"));
			break;

		    case 'C': 	/* ignore */
		    case 'c': 	/* ignore */
		    case 'H':	/* ignore */
		    case 'l':	/* ignore */
		    case 'n':	/* ignore */
		    case 'R': 	/* ignore */
		    case 'S':	/* ignore */
			break;

		    case '{':	/* DNODOT: output domain name without leading dot */
			{	/* otherwise: output value from mailcnfg file */
			string *s = s_new();
			while (*++ip)	/* grab name between the {} */
			    if (*ip == '}')
				break;
			    else
				s_putc(s, *ip);
			if (!*ip) ip--;
			s_terminate(s);
			if (strcmp(s_to_c(s), "DNODOT") == 0)
			    {
			    char *d = maildomain();
			    if (*d) nomagic(outpattern, d+1);
			    }

			else
			    nomagic(outpattern, Mgetenv(s_to_c(s)));
			s_free(s);
			}
			break;

		    case '[':	/* ignore header name */
			{
			while (*++ip)	/* skip over name between the {} */
			    if (*ip == ']')
				break;
			if (!*ip) ip--;
			}
			break;

		    default:	/* use %x value from mailcnfg */
			if (Islower(*ip))
			    {
			    char x[3];
			    x[0] = '%';
			    x[1] = *ip;
			    x[2] = '\0';
			    nomagic(outpattern, Mgetenv(x));
			    }

			else
			    s_putc(outpattern, *ip);
			break;
		    }
		break;

	    case BS:
		s_putc(outpattern, BS);
		s_putc(outpattern, *++ip);
		break;

	    default:	/* regular character */
		s_putc(outpattern, *ip);
	    }

    s_terminate(outpattern);
    ip = s_ptr_to_c(outpattern);
    Dout(pn, 5, "outpattern = '%s'\n", s_to_c(outpattern));

    /* set up regular expression mapping */
    for (i = 0; i < 256; i++)
	re_map[i] = (char)i;
    /* Map upper case letters to lower case. */
    /* This works even on EBCDIC systems. */
    for (i = 'A'; i <= 'Z'; i++)
	re_map[i] = tolower(i);
    regex = re_recomp(s_to_c(outpattern), ip, re_map);

    Dout(pn, 5, "regular expression compilation %s\n", (regex ? "succeeded" : "failed"));
    s_free(outpattern);
    if (retnbra)
	{
	*retnbra = regex ? re_paren(regex) : 0;
	if (*retnbra > RE_NBRAK)
	    *retnbra = RE_NBRAK;
	}
    return regex;
}

/*
    append a string to given pattern, making certain that all
    magic characters become non-magic.

	\	->	\\
	.	->	\.
	*	->	\*
	[	->	\[
	?	->	\?
	+	->	\+
	|	->	\|
*/
static void nomagic(outpattern, str)
register string *outpattern;
register char *str;
{
    for ( ; *str; str++)
	switch (*str)
	    {
	    case BS:
	    case '.':
	    case '*':
	    case '[':
	    case '?':
	    case '+':
	    case '|':
		s_putc(outpattern, BS);
		/* FALLTHROUGH */
	    default:
		s_putc(outpattern, *str);
		break;
	    }
}

/*
    NAME
	re_error() - error routine for re_re*() functions

    SYNOPSIS
	void re_error(char *msg)

    DESCRIPTION
	This routine is provided for the re_re*() functions.
	It will be called by them if a problem arises
	during compilation of a pattern. We log the message
	and return, permitting the re_recomp() function
	to also return.
*/

void re_error(msg)
char *msg;
{
    Tout("re_error", "regular expression error message '%s'\n", msg);
}
