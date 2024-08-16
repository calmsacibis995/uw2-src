/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckdate.c	1.4.5.9"
#ident  "$Header: $"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <nl_types.h>
#include <langinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

extern void	putprmpt(),
		puterror(),
		puthelp();
extern int	getinput(),
		puttext();
extern int	check_quit();

#define MSGSIZ	256

#define PROMPT_ID "uxadm:50"
#define PROMPT	"Enter the date"
#define MESG_ID "uxadm:51"
#define MESG	"Please enter a date. Format is <%s>."

#define DATEFILE "/tmp/ckdate.%u"
#define DATEMSK "DATEMSK"

static char datemsk[PATH_MAX + sizeof(DATEMSK)] = DATEMSK "=";
static char datefile[PATH_MAX + 1] = "";
static int  ckdate_call;

static char	*p_ndigit(),
		*p_date(),
		*p_eday(),
		*p_dlm();

static void
setmsg(msg, fmt)
char	*msg, *fmt;
{
	if( fmt == NULL || !strcmp(fmt, "%x"))
		fmt = nl_langinfo(D_FMT);
	(void) sprintf(msg, gettxt(MESG_ID, MESG), fmt);
}

int
ckdate_err(fmt, error)
char	*fmt, *error;
{
	char	defmesg[MSGSIZ];

	if(fmt && fmtcheck(fmt) == 1)
		return(4);
	setmsg(defmesg, fmt);
	puterror(stdout, defmesg, error);
	return(0);
}

int
ckdate_hlp(fmt, help)
char	*fmt, *help;
{
	char	defmesg[MSGSIZ];

	if(fmt && fmtcheck(fmt) == 1)
		return(4);
	setmsg(defmesg, fmt);
	puthelp(stdout, defmesg, help);
	return(0);
}

/*
*	A little state machine that checks out the format to
*	make sure it is acceptable.
*		return value 1: NG
*		return value 0: OK
*/
int
fmtcheck(fmt)
char	*fmt;
{
	int	percent = 0;

	if ( !fmt )
		return(percent);

	while(*fmt) {
		switch(*fmt++) {
		 	case '%':
				if(percent == 0)
					percent = 1;
				else
					percent = 0;
				break;
			case 'a': /* previous state must be "%" */
			case 'A':
			case 'b':
			case 'B':
			case 'C':
			case 'd':
			case 'D':
			case 'e':
			case 'h':
			case 'j':
			case 'm':
			case 'n':
			case 't':
			case 'U':
			case 'w':
			case 'W':
			case 'x':
			case 'y':
			case 'Y':
				if(percent == 1)
					percent = 0;
				break;
			case 'E':
			if (percent == 1) {
				percent = 0;
				switch (*fmt++) {
					case 'C':
					case 'x':
					case 'y':
					case 'Y':
						break;
					default:
						return(1);
				}
				break;
			}
			case 'O':
			if (percent == 1) {
				percent = 0;
				switch (*fmt++) {
					case 'd':
					case 'e':
					case 'm':
					case 'M':
					case 'U':
					case 'w':
					case 'W':
					case 'y':
						break;
					default:
						return(1);
				}
				break;
			}
			default:
			if (percent == 1)
				return(1);
		}
	}
	return(percent);
}

int
make_template(fmt)
char *fmt;
{
	FILE *df;

	sprintf(datefile, DATEFILE, (unsigned long)getpid());
	df=fopen(datefile, "w");
	if (df == NULL)
		return(1);
	fprintf(df, "%s\n", fmt);
	fclose(df);
	strcat(datemsk, datefile);
	if (putenv(datemsk) != 0)
		return(1);
	return(0);
}

int
wipe_template(void)
{
	int	rtn = 0;

	if (datefile[0] != '\0') {
		if (unlink(datefile) != 0)
			rtn = 1;
		datemsk[sizeof(DATEMSK)] = '\0';
		datefile[0] = '\0';
	}
	return rtn;
}

int
ckdate_val(fmt, input)
char *fmt, *input;
{
	int  rtn = 0;

	if (datefile[0] == '\0') { /* this is for the stupid valtools */
		if((fmt != NULL) && (fmtcheck(fmt) == 1))
			return(4);
		if(fmt == NULL)
			fmt = nl_langinfo(D_FMT);
				/* Default date format for the current locale */
		if (make_template(fmt) == 1)
			return(5);
	}

	if (getdate(input) == NULL)
		if (getdate_err == 7 || getdate_err == 8)
			rtn = 1;
		else 
			rtn = 5;

	if (! ckdate_call)
		wipe_template();
	return rtn;
}

ckdate(date, fmt, defstr, error, help, prompt)
char *date;
char *fmt;
char *prompt;
char *defstr, *error, *help;
{
	char	defmesg[MSGSIZ];
	char	input[128];
	char	*ept, end[128];
	char	*defprompt = NULL;

	ept = end;
	*ept = '\0';

	if (fmt == NULL)
		fmt = nl_langinfo(D_FMT);
			/* Default date format for the current locale */
	else if (fmtcheck(fmt) == 1)
		return(4);

	setmsg(defmesg, fmt);
	(void) sprintf(ept, "[?,q]");
	
	if(!prompt)
		prompt = gettxt(PROMPT_ID, PROMPT);
	else if(strchr(prompt, '~'))
		defprompt = gettxt(PROMPT_ID, PROMPT);

start:
	putprmpt(stderr, prompt, NULL, defstr, defprompt);
	if(getinput(input))
		return(1);

	if(!strlen(input)) {
		if(defstr) {
			(void) strcpy(date, defstr);
			return(0);
		}
		puterror(stderr, defmesg, error);
		goto start;
	} else if(!strcmp(input, "?")) {
		puthelp(stderr, defmesg, help);
		goto start;
	} else if (!check_quit(input)) {
		return(3);
	} else if(ckdate_val(fmt, input)) {
		puterror(stderr, defmesg, error);
		goto start;
	}
	(void) strcpy(date, input);
	return(0);
}
