/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#ident	"@(#)mail:common/cmd/mail/mailproc/cmd.y	1.1.2.3"
#ident	"@(#)mail:common/cmd/mail/mailproc/cmd.y	1.1.2.2"
#ident "@(#)cmd.y	1.4 'attmail mail(1) command'"
/* from ferret SCCSid cmd.y 3.2 */
#include "../mail/libmail.h"
#include "mailproc.h"

static void appendfrom ARGS((Fromlist*, int));
static void append ARGS((const char*));
static const char *getheader ARGS((const char*));
static void reset_cmdbuf ARGS((void));

static char	cmdbuf[512];
static char	*cmdptr = cmdbuf;
static char	keepcodes[256];
static int	retcode;
static Fromlist	*null_list = NULL;

Msg	*Msgptr;

%}

%start line

%token COL MGR MGA TXR TXA TXT LFR FRM SUB DAT ALP EOL SQT DQT HDR

%%

line	:	start who COL test COL exit_code_spec COL action EOL
		{
			if (keepcodes[retcode] == 'k')
			{
				if (dflg)
					if (rflg)
						(void) fprintf(stderr, "SUCCESS\n");
					else
						(void) fprintf(stderr, "KEEPING\n");
				Msgptr->keep = 1;
			}
			else
			{
				if (dflg)
					(void) fprintf(stderr, "SUCCESS\n");
				Msgptr->keep = 0;
			}
			return P_MATCH;
		}
	|	null
		{
			return P_EOF;
		}
	|	error
		{
			return P_ERROR;
		}

who	:      	ALP
		{
			char *p = yytext;
			retcode = 0;

			/* parse string for command, if necessary */
			if (!Cmdptr->flp->user)
			{
				if (!pfrom(Cmdptr->flp, p, p+strlen(p)))
					return P_ERR_FROM;
			}
			if (dflg)
			{
				(void) fprintf(stderr, "Matching <");
				prtfrom(stderr, Cmdptr->flp, 1);
				(void) fprintf(stderr, "> with <");
				prtfrom(stderr, Msgptr->fhp, 1);
				(void) fprintf(stderr, ">\n");
			}

			if (!matchfp(Cmdptr->flp, Msgptr->fhp))
				return P_NOMATCH;
		}
	|	null
		{
			char *tmp;

			if (!null_list)
			{
				if ((null_list = new_Fromlist()) == NULL)
					return P_ERR_MEM;
				tmp = ".*";
				(void) pfrom(null_list, tmp, tmp+2);
			}
			(void) matchfp(null_list, Msgptr->fhp);
			retcode = 0;
		}


test	:	list
		{
			int ret;

			if (dflg)
				(void) fprintf(stderr, "Performing test '%s'\n",cmdbuf);
			if (*cmdbuf)
			{
				int ret;
				if (cmdbuf[0] == '~')
				{
					const char *p1 = strtok(cmdbuf+1, "~");
					const char *p2 = strtok((char*)0, "~");
					ret = matchstr(p1, p2);
					if (ret >= 0) ret = !ret;
				}
				else
				{
					ret = do_system(cmdbuf);
				}
				if (ret < 0)
				{
					if (dflg)
						(void) fprintf(stderr, "sh or regex error\n");
					return P_ERR_TSYS;
				}
				if (ret)
				{
					if (dflg)
						(void) fprintf(stderr, "CONTINUE\n");
					return P_NOMATCH;
				}
			}

			if (dflg)
				(void) fprintf(stderr, "SUCCESS\n");
			reset_cmdbuf();
		}

exit_code_spec :	list
		{
			if (dflg)
				(void) fprintf(stderr, "Using exit code list '%s'\n", cmdbuf);
			if (!parse_code_spec())
				return P_ERROR;
			reset_cmdbuf();
		}

action	:	MGR tok
		{
			if (dflg)
				(void) fprintf(stderr, "Writing message to %s\n", cmdbuf);
			if (!nflg && !mvmsg(Msgptr, cmdbuf, M_MSG, M_DIR, 1))
			{
				if (dflg)
					(void) fprintf(stderr, "CONTINUE\n");
				return P_NOMATCH;
			}
		}
	|	MGA tok
		{
			if (dflg)
				(void) fprintf(stderr, "Appending message to %s\n", cmdbuf);
			if (!nflg && !mvmsg(Msgptr, cmdbuf, M_MSG, M_APP, 1))
			{
				if (dflg)
					(void) fprintf(stderr, "CONTINUE\n");
				return P_NOMATCH;
			}
		}
	|	TXR tok
		{
			if (dflg)
				(void) fprintf(stderr, "Writing text to %s\n", cmdbuf);
			if (!nflg && !mvmsg(Msgptr, cmdbuf, M_TXT, M_DIR, 1))
			{
				if (dflg)
					(void) fprintf(stderr, "CONTINUE\n");
				return P_NOMATCH;
			}
		}
	|	TXA tok
		{
			if (dflg)
				(void) fprintf(stderr, "Appending text to %s\n",cmdbuf);
			if (!nflg && !mvmsg(Msgptr, cmdbuf, M_TXT, M_APP, 1))
			{
				if (dflg)
					(void) fprintf(stderr, "CONTINUE\n");
				return P_NOMATCH;
			}
		}
	|	list
		{
			if (dflg)
				(void) fprintf(stderr, "Performing action '%s'\n", cmdbuf);
			if (!nflg && *cmdbuf && ((retcode = do_system(cmdbuf)) < 0))
			{
				if (dflg)
					(void) fprintf(stderr, "sh error\n");
				return P_ERR_ASYS;
			}
			if (keepcodes[retcode] == 'c')
			{
				if (dflg)
					(void) fprintf(stderr, "CONTINUE\n");
				return P_NOMATCH;
			}
		}

list	:	null
	|	tok
	|	list tok
	|	list dqlist
	|	list sqlist
		;

tok	:	ALP
		{
			append(yytext);
		}
	|	FRM
		{
			appendfrom(Msgptr->fhp, 0);
		}
	|	LFR
		{
			appendfrom(Msgptr->fhp, 1);
		}
	|	SUB
		{
			append(quotechars(Msgptr->subject));
		}
	|	DAT
		{
			append(quotechars(Msgptr->date));
		}
	|	HDR
		{
			append(quotechars(getheader(yytext)));
		}

dqt:		DQT
		{
			if (Quoted == '"')
				Quoted = '\0';		/* terminate string */
			else if (!Quoted)
				Quoted = '"';		/* start string */
			/* else already in string, ignore */
			append("\"");
		}
sqt	:	SQT
		{
			if (Quoted == '\'')
				Quoted = '\0';		/* terminate string */
			else if (!Quoted)
				Quoted = '\'';		/* start string */
			/* else already in string, ignore */
			append("'");
		}

dqlist	:	dqt dql dqt
		;

dql	:	null
	|	sqt
	|	tok
	|	dql sqt
	|	dql tok
		;

sqlist	:	sqt sql sqt
		;

sql	:	null
	|	dqt
	|	tok
	|	sql dqt
	|	sql tok
		;

start:		{
			retcode = 0;
			reset_cmdbuf();
		}

null:		;

%%

void yyerror(msg)
const char *msg;
{
    if (dflg)
	(void) fprintf (stderr, "yyerror: %s\n", msg);
}

/* append a string to the command buffer */
static void append(s)
const char *s;
{
	(void) strcpy(cmdptr, s);
	cmdptr += strlen(s);
}

/* append a from address to the command buffer */
static void appendfrom(fhp, full)
Fromlist *fhp;
int full;
{
	sprtfrom(cmdptr, fhp, full);
	cmdptr += strlen(cmdptr);
}

/* pull out the contents of the given header from the message */
static	const char *getheader(name)
const char *name;
{
    static char buf[1024];
    int len = strlen(name) - 3;	/* subtract 3 for the %{} */

    if (fseek(Mailfp, Msgptr->startmsg, 0))
        {
	(void) fprintf (stderr, "Seek failed: %s\n", Strerror(errno));
        return "";
	}

    /* only read the header */
    while (ftell(Mailfp) < Msgptr->starttxt)
	{
	/* was something read? */
	if (!fgets(buf, sizeof(buf), Mailfp))
	    {
	    (void) fprintf (stderr, "Read failed: %s\n", Strerror(errno));
	    return "";
	    }

	/* did we find the header? */
	if ((casncmp(buf, name+2, len) == 0) && (buf[len] == ':'))
	    {
	    trimnl(buf + len + 1);
	    return skipspace(buf + len + 1);
	    }
	}

    /* header not found */
    return "";
}

static void reset_cmdbuf()
{
	cmdptr = cmdbuf;
	*cmdptr = '\0';
}

/* Look at a list such as *,3-5,99 and parse it, setting keepcodes appropriately. */
static int parse_code_list(list, type)
char *list;
char type;
{
	const char *numlist;
	if (!*list) return 1;
	if (list[1] != '=') return 0;
	for (numlist = strtok(list+2, ","); numlist && *numlist; numlist = strtok((char*)0, ","))
	{
		int r1, r2;
		const char *dashptr;
		if (numlist[0] == '*')
			continue;
		r1 = atoi(numlist);
		if (r1 < 0) r1 = 0;
		dashptr = strchr(numlist, '-');
		if (dashptr)
			r2 = atoi(dashptr+1);
		else
			r2 = r1;
		if (r2 > 255) r2 = 255;
		memset(keepcodes+r1, type, r2-r1+1);
	}
	return 1;
}

/* dump a representation of the exit code lists to the screen */
static void dumpstatlist(s)
const char *s;
{
    register int i, iend = 256;
#define NPERLINE 70
    fprintf(stderr, "The exit code list is:\n");
    for (i = 0; i < iend; i += NPERLINE)
	{
	register int j, jend = ((i+NPERLINE) >= iend) ? iend : (i+NPERLINE);
	for (j = i; j < jend; j += 10)
	    {
	    register int k, kend = ((j+10) >= iend) ? iend : (j+10);
	    (void) fprintf(stderr, "'%3d .", j);
	    for (k = j+6; k < kend; k++)
		putc(' ', stderr);
	    }
	putc('\n', stderr);
	for (j = i; j < jend; j++)
	    putc(s[j], stderr);
	putc('\n', stderr);
	putc('\n', stderr);
	}
}

/* Convert a S=...;K=...;C=...; list into the keepcodes array. */
/* Return 1 on success, 0 on parse error. */
static int parse_code_spec()
{
	const char *specptr;
	const char *spec;
	const char *slist = "";
	const char *clist = "";
	const char *klist = "";

	/* look for empty spec */
	cmdptr = (char*)skipspace(cmdbuf);
	if (!*cmdptr)
	{
		keepcodes[0] = 's';
		memset(keepcodes+1, 'c', sizeof(keepcodes)-1);
		if (dflg > 1) dumpstatlist(keepcodes);
		return 1;
	}

	for (spec = strtok(cmdptr, ";"); spec && *spec; spec = strtok((char*)0, ";"))
	{
		switch (spec[0])
		{
		case 's': case 'S': slist = spec; break;
		case 'c': case 'C': clist = spec; break;
		case 'k': case 'K': klist = spec; break;
		default: return 0;
		}
	}

	/* set the default */
	if (strchr(slist, '*'))
		memset(keepcodes, 's', sizeof(keepcodes));
	else if (strchr(klist, '*'))
		memset(keepcodes, 'k', sizeof(keepcodes));
	else if (strchr(clist, '*'))
		memset(keepcodes, 'c', sizeof(keepcodes));
	else
	{
		keepcodes[0] = 's';
		memset(keepcodes+1, 'c', sizeof(keepcodes)-1);
	}

	/* now look for number lists */
	if (!parse_code_list(clist, 'c')) return 0;
	if (!parse_code_list(klist, 'k')) return 0;
	if (!parse_code_list(slist, 's')) return 0;
	if (dflg > 1) dumpstatlist(keepcodes);
	return 1;
}
