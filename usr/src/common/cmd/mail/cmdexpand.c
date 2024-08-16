/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/cmdexpand.c	1.6.6.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)cmdexpand.c	1.12 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	cmdexpand - expand mail surrogate command string

    SYNOPSIS
	string *cmdexpand(Msg *pmsg, Recip *r, string *instr, char **lbraslist, char **lbraelist,
		string *outstr)

    DESCRIPTION
	 cmdexpand() will expand the mail surrogate command.
	 It will make the following changes to the string:

		%D	  ->	local domain name
		%L	  ->	local system name
		%U	  ->	local uname
		%X	  ->	smarter host
		\1 - \9	  ->	pointers into orig and recip
		%l	  ->	content length
		%H	  ->	header length
		%n	  ->	recipient name
		%R	  ->	return path to originator
		%C	  ->	content type
		%c	  ->	content type
		%S	  ->	subject
		%x	  ->	x is [a-z], Mgetenv(%x)
		%x	  ->	Mgetenv(%x)
		%{DNODOT} ->	local domain name w/o leading dot
		%{xyz}	  ->	Mgetenv(xyz)
		%[xyz]	  ->	value of first line of first header named xyz
		\x	  ->	x
		%%	  ->	%
*/

#define BS '\\'

string *cmdexpand(pmsg, r, instr, lbraslist, lbraelist, outstr)
Msg *pmsg;
Recip *r;
string *instr;
char **lbraslist;
char **lbraelist;
string *outstr;
{
    static const char pn[] = "cmdexpand";
    register char *ip;
    register char *brap;
    Hdrs	*hptr;
    register int i;

    if (!instr)
	return 0;

    ip = s_to_c(instr);
    outstr = s_reset(outstr);

    Dout(pn, 7, "instr = '%s'\n", s_to_c(instr));
    for ( ; *ip; ip++)
	{
	switch (*ip)
	    {
	    case '%':
		switch (*++ip)
		    {
		    case 'C':	/* output "text" or "binary" */
			outstr = s_append(outstr, (pmsg->binflag == C_Text) ? Text : Binary);
			break;

		    case 'c':	/* output contents of Content-Type: header */
			if ((hptr = pmsg->phdrinfo->hdrs[H_CTYPE]) !=
					(Hdrs *)NULL) {
				outstr = s_append(outstr, s_to_c(hptr->value));
			}
			break;

		    case 'l':	/* output contents of Content-Length: header */
			if ((hptr = pmsg->phdrinfo->hdrs[H_CLEN]) !=
					(Hdrs *)NULL) {
				outstr = s_append(outstr, s_to_c(hptr->value));
			} else
				outstr = s_append(outstr, "0");
			break;

		    case 'H':	/* output length of header */
			{
			char buf[40]; /* large enough to hold a long */
			sprintf(buf, "%ld", sizeheader(pmsg));
			outstr = s_append(outstr, buf);
			}
			break;

		    case 'D':	/* output domain string */
			outstr = s_append(outstr, maildomain());
			break;

		    case 'L':	/* output local system name */
			outstr = s_append(outstr, thissys);
			break;

		    case 'n':	/* output recipient name */
			outstr = s_append(outstr, s_to_c(r->name));
			break;

		    case 'O':	/* output original version of recipient name */
			outstr = s_append(outstr, s_to_c(recip_parent(r)->name));
			break;

		    case 'R':	/* output return path to sender */
			outstr = s_append(outstr, s_to_c(pmsg->Rpath));
			break;

		    case 'U':	/* output local uname */
			outstr = s_append(outstr, mailsystem(1));
			break;

		    case 'X':	/* output SMARTERHOST value */
			outstr = s_append(outstr, Mgetenv("SMARTERHOST"));
			break;

		    case 'S':	/* output contents of Subject: header */
			if ((hptr = pmsg->phdrinfo->hdrs[H_SUBJ]) !=
					(Hdrs *)NULL) {
				outstr = s_append(outstr, s_to_c(hptr->value));
			}
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
			    if (*d) outstr = s_append(outstr, d+1);
			    }

			else
			    outstr = s_append(outstr, Mgetenv(s_to_c(s)));
			s_free(s);
			}
			break;

		    case '[':	/* value of first line of first header named xyz */
			{
			string *s = s_new();
			register Hdrs *hdr;
			while (*++ip)	/* grab name between the {} */
			    if (*ip == ']')
				break;
			    else
				s_putc(s, *ip);
			if (!*ip) ip--;
			s_putc(s, ':');
			s_terminate(s);
			/* loop through headers ... */
			for (hdr = pmsg->phdrinfo->hdrhead; hdr; hdr = hdr->next)
			    /* ... looking for a header of the right name */
			    if (cascmp(s_to_c(s), hdr->name) == 0)
				{
				char *p = s_to_c(hdr->value);
				/* copy the value up to the newline */
				for (; *p; p++)
				    if (*p == '\n')
					break;
				    else
					s_putc(outstr, *p);
				break;
				}
			s_free(s);
			}
			break;

		    default:	/* use %x value from mailcnfg */
			if (Islower(*ip))
			    {
			    char x[3];
			    x[0] = '%';
			    x[1] = *ip;
			    x[2] = '\0';
			    outstr = s_append(outstr, Mgetenv(x));
			    }

			else
			    s_putc(outstr, *ip);
			break;
		    }
		break;

	    case BS:
		switch (*++ip)
		    {
		    default:	/* \x -> \x */
			s_putc(outstr, BS);
			s_putc(outstr, *ip);
			break;

		    /* \1 - \0 becomes braslist[0] - braslist[9] */
		    case '1': case '2': case '3': case '4': case '5':
		    case '6': case '7': case '8': case '9': case '0':
			i = (*ip == '0') ? 9 : (*ip - '1');
			if (lbraslist[i])
			    for (brap = lbraslist[i]; brap < lbraelist[i]; brap++)
				s_putc(outstr, *brap);
			break;
		    }
		break;

	    default:
		s_putc(outstr, *ip);
	    }
	}

    s_terminate(outstr);
    Dout(pn, 7, "outstr = '%s'\n", s_to_c(outstr));
    return outstr;
}
