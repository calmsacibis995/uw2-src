/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/getsurr.c	1.5.4.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)getsurr.c	1.12 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	getsurr - read the next field of the surrogate file

    SYNOPSIS
	getsurr(FILE *fp, string *buf, int firstfield)

    DESCRIPTION
	Read the next field from the surrogate file and store it into
	buf. These fields are surrounded by single quotes. Within the
	single quotes, \' will permit nesting.

	If firstfield is set, then this is the first field of a set
	of fields. An EOF is permitted here, as is the # comment
	character, plus the special

		Defaults:
		Translate-Defaults:

	lines which are used to reset the default settings of the
	transport and translation return codes, via setsurg_rc().

	The resulting field is stored into buf.

    RETURN VALUES
	-1	Unnatural EOF or other error
	 0	Natural EOF
	 1	Regular return
*/

static int getfield();

int getsurr (fp, buf, firstfield)
FILE *fp;
string *buf;
int  firstfield;
{
	static const char pn[] = "getsurr";
	int	ch;

	s_terminate(buf);

	for (;;) {
		/* Skip leading whitespace and blank lines */
		while (((ch = fgetc(fp)) != EOF) && Isspace(ch))
			;

		switch (ch) {
		case EOF:
			/* OK to hit EOF here */
			return (firstfield == TRUE ? 0 : -1);

		case '\'':
			return getfield(fp, buf);

		case 'd': case 'D':
		case 't': case 'T':
			/* If default return code states redefined, handle here */
			if (firstfield == TRUE) {
			    char lbuf[256];
			    string *s;
			    int Ch = toupper(ch);
			    int len = (Ch == 'D' ? 8 : 18);
			    char *str = (Ch == 'D' ? "efaults:" : "ranslate-defaults:");

			    ch = tolower(ch);
			    if (fgets(lbuf, sizeof(lbuf), fp) == (char *)NULL) {
				return (-1);
			    }

			    if (casncmp (lbuf, str, len) != 0) {
				return (-1);
			    }

			    Tout(pn,"---------- Next '%s' entry ----------\n", mailsurr);

			    s = s_copy(lbuf+len);
			    if (setsurg_rc(s, DEFAULT, (Ch == 'D' ? t_transport_cmd : t_translate_cmd),
			                   (int*)0, (int*)0, (int*)0, (int*)0, (string*)0) == (char *)NULL) {
			        s_free(s);
				return (-1);
			    }
			    s_free(s);
			    continue;
			}
			return (-1);

		case '#':
			if (firstfield == TRUE) {
				/* If we find a '#' before anything else on */
				/* the line, assume it's a comment indicator */
				/* and flush through the newline */
				while (((ch = fgetc(fp)) != '\n') && (ch != EOF))
					;
				if (ch == EOF) {
					return(0);
				}
				continue;
			}
			/* FALLTHROUGH */

		default:
			/* Trouble in River City... */
			return (-1);
		}
	}
}

static int getfield(fp, buf)
FILE *fp;
string *buf;
{
	int	ch;

	while ((ch = fgetc(fp)) != '\'') {
		switch (ch) {
		case EOF:
			/* Bad to hit EOF here */
			s_terminate(buf);
			return (-1);

		case '\n':
			/* Eat unescaped newline plus following whitespace */
			while (((ch = fgetc(fp)) != EOF) && Isspace(ch))
				;

			if (ch == EOF) {
				s_terminate(buf);
				return (-1);
			}
			(void) ungetc(ch, fp);
			break;

		case '\\':
			/* Next char escaped. Take it regardless */
			ch = fgetc(fp);
			if (ch == EOF) {
				s_terminate(buf);
				return (-1);
			}
			/* FALLTHROUGH */

		default:
			s_putc(buf, ch);
			break;
		}
	}

	s_terminate(buf);
	return (1);
}
