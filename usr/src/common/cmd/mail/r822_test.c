/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/r822_test.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)r822_test.c	1.3 'attmail mail(1) command'"
/*
**  Checkout program for rfc822-address parser
**  Takes address-lines from stdin up to EOF or empty line.
**  Writes results to stdout.
*/
#include "mail.h"
#include "r822.h"
#include "r822t.h"

/* implementation of r822_FILE infinite pushback FILE structure */
void init_r822_FILE(this, sfp)
r822_FILE *this;
FILE *sfp;
{
    this->fp = sfp;
    this->pushback = s_new();
}
void fini_r822_FILE(this)
r822_FILE *this;
{
    s_free(this->pushback);
}
r822_FILE *new_r822_FILE(sfp)
FILE *sfp;
{
    r822_FILE *this = New(r822_FILE);
    if (this) init_r822_FILE(this, sfp);
    return this;
}
void delete_r822_FILE(this)
r822_FILE *this;
{
    if (this)
	{
	fini_r822_FILE(this);
	free((char*)this);
	}
}

static void
prtroute(dp)
r822_domain *dp;
{
	r822_subthing *p;

	for (; dp; dp = dp->next)
	{
		fprintf(stdout, "{{%s}} ", s_to_c(dp->dotted));
		for (p = dp->subdom; p; p=p->next)
		{
			if (p->flags & r822_IS_DOMLIT) fputs("[", stdout);
			fputs(s_to_c(p->element), stdout);
			if (p->flags & r822_IS_DOMLIT) fputs("]", stdout);
			if (p->next) fputs(".", stdout);
		}
		if (dp->next) fputs(" --> ", stdout);
	}
	fputs("\n", stdout);
}


main(argc, argv)
int argc;
char **argv;
{
	r822_address *alist = 0, *ap;
	int flags=0, ch;
	string *line = s_new();

	while ((ch = getopt(argc, argv, "p,rw[@7")) != EOF)
	{
		switch (ch)
		{
		case 'p':
			flags |= r822_LOOSE_QPAIRS;
			fprintf(stdout, "LOOSE_QPAIRS   ");
			break;
		case ',':
			flags |= r822_STRICT_COMMA;
			fprintf(stdout, "STRICT_COMMAS   ");
			break;
		case 'r':
			flags |= r822_LOOSE_CRLF;
			fprintf(stdout, "LOOSE_CRLF   ");
			break;
		case 'w':
			flags |= r822_LOOSE_LWSP;
			fprintf(stdout, "LOOSE_LWSP   ");
			break;
		case '[':
			flags |= r822_LOOSE_DOMLIT;
			fprintf(stdout, "LOOSE_DOMLIT   ");
			break;
		case '@':
			flags |= r822_STRICT_COLON;
			fprintf(stdout, "STRICT_COLON   ");
			break;
		case '7':
			flags |= r822_STRICT_7BIT;
			fprintf(stdout, "STRICT_7BIT   ");
			break;
		default:
			fprintf(stdout, "Usage: %s [flags]\n\n", argv[0]);
			fprintf(stdout, "\t-p  (loose quoted pairs, rfc821 instead of rfc822)\n");
			fprintf(stdout, "\t-,  (strict commas, no phantom commas between addresses)\n");
			fprintf(stdout, "\t-r  (loose CRLF, take any of CR, LF, or CRLF as CRLF)\n");
			fprintf(stdout, "\t-w  (loose LWSP, take any isspace() instead of just SP,HT)\n");
			fprintf(stdout, "\t-[  (loose domain literals, rfc822 instead of rfc821)\n");
			fprintf(stdout, "\t-@  (strict colon, no unquoted GMS folder names)\n");
			fprintf(stdout, "\t-7  (strict 7bit, input gets top bit stripped)\n");
			fprintf(stdout, "\n");
			exit (1);
		}
	}

	for (;;)
	{
		int rc;
		r822_FILE *rfp = new_r822_FILE(stdin);
		s_reset(line);
		rc = r822_find_field_name((int (*) ARGS((void *)))r822_fgetc, (int (*) ARGS((int, void *)))r822_ungetc, rfp, flags, line);
		if (rc <= 0)
		{
			delete_r822_FILE(rfp);
			break;
		}
		s_reset(line);
		rc = r822_find_field_body((int (*) ARGS((void *)))r822_fgetc, (int (*) ARGS((int, void *)))r822_ungetc, rfp, flags, line);
		s_terminate(line);
		fprintf(stdout, "\n");
		fprintf(stdout, "#-#-#-#-#-#\n");
		fprintf(stdout, "-----------\n");
		fputs(s_to_c(line), stdout);
		r822_addr_parse(line, flags, &alist);
		r822_slash_options_all(alist);
		for (ap = alist; ap; ap = ap->next)
		{
			r822_subthing *sp;
			fprintf(stdout, "\n-----------\n");
			fprintf(stdout, "error :\t%s\n", s_to_c(ap->error));
			fprintf(stdout, "name  :\t%s\n", s_to_c(ap->name_phrase));
			fprintf(stdout, "(c)   :\t");
			if (ap->comment  &&  ap->comment->next)
			{
				fprintf(stdout, "(");
			}
			for (sp=ap->comment; sp; sp=sp->next)
			{
				fprintf(stdout, "(%s)", s_to_c(sp->element));
			}
			if (ap->comment  &&  ap->comment->next)
			{
				fprintf(stdout, ")");
			}
			fprintf(stdout, "\n");
			fprintf(stdout, "group :\t%s\n", s_to_c(ap->group_phrase));
			fprintf(stdout, "localp:\t%s\n", s_to_c(ap->local_part));
			for (sp=ap->options; sp; sp=sp->next)
			{
				fprintf(stdout, "option:\t%s", s_to_c(sp->element));
				if (sp->flags & r822_HAS_VALUE)
				{
					sp = sp->next;
					fprintf(stdout, " = '%s'", s_to_c(sp->element));
				}
				fprintf(stdout, "\n");
			}
			fprintf(stdout, "dstdom:");
			if (ap->domain_part)
			{
				prtroute(ap->domain_part);
			}
			else
				fprintf(stdout, "\n");
			fprintf(stdout, "route :");
			if (ap->route)
			{
				prtroute(ap->route);
			}
			fprintf(stdout, "\n");
		}
		delete_r822_address(alist);
		alist = 0;
		delete_r822_FILE(rfp);
	}
	s_free(line);
	return 0;
}
