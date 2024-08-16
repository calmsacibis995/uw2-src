/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckuid.c	1.2.5.5"
#ident  "$Header: $"

#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern long	strtol();
extern void	*calloc(), 
		*realloc(),
		free(),
		putprmpt(),
		puterror(),
		puthelp();
extern int	getinput(),
		puttext();
extern int	check_quit();

#define PROMPT \
	gettxt("uxadm:88", "Enter the login name of an existing user")
#define MESG \
	gettxt("uxadm:89", "Please enter the login name of an existing user.")
#define ALTMESG \
	gettxt("uxadm:90", "Please enter one of the following login names:\\n\\t")
#define MALSIZ	64

#define DELIM1 '/'
#define BLANK ' '

static char *
setmsg(disp)
int	disp;
{
	struct passwd
		*pwdptr;
	int	count, n, m;
	char	*msg;
	char	*altmsg = ALTMESG;

	if(disp == 0)
		return(MESG);

	m = MALSIZ;
	n = strlen(altmsg);
	if (n >= m) {
		m += MALSIZ;
	}
	if ((msg = (char *) calloc(m, sizeof(char))) == NULL)
		return(msg);
	(void) strcpy(msg, altmsg);

	setpwent();
	count = 0;
	while(pwdptr = getpwent()) {
		n += strlen(pwdptr->pw_name) + 2;
		while(n >= m) {
			m += MALSIZ;
			if ((msg = (char *) realloc(msg, m*sizeof(char))) == NULL)
				return(msg);
		}
		if(count++)
			(void) strcat(msg, ", ");
		(void) strcat(msg, pwdptr->pw_name);
	}
	endpwent();
	return(msg);
}

int
ckuid_dsp()
{
	struct passwd *pwdptr;

	/* if display flag is set, then list out passwd file */
	if (ckpwdfile() == 1)
		return(1);
	setpwent();
	while(pwdptr = getpwent())
		(void) printf("%s\n", pwdptr->pw_name);
	endpwent();
	return(0);
}

int
ckuid_val(usrnm)
char	*usrnm;
{
	int	valid;

	setpwent ();
	valid = (getpwnam(usrnm) ? 0 : 1);
	endpwent ();
	return(valid);
}

int
ckpwdfile() /* check to see if passwd file there */
{
	struct passwd *pwdptr;

	setpwent ();
	if (!(pwdptr = getpwent())) {
		endpwent ();
		return(1);
	}
	endpwent ();
	return(0);
}

void
ckuid_err(disp, error)
short	disp;
char	*error;
{
	char	*msg;

	msg = setmsg(disp);
	puterror(stdout, msg, error);
	if(disp)
		free(msg);
}

void
ckuid_hlp(disp, help)
char	*help;
{
	char	*msg;

	msg = setmsg(disp);
	puthelp(stdout, msg, help);
	if(disp)
		free(msg);
}

int
ckuid(uid, disp, defstr, error, help, prompt)
char	*uid;
short	disp;
char	*prompt;
char	*defstr, *error, *help;
{
	char	*defmesg,
		input[128];
	char	*defprompt = NULL;

	defmesg = NULL;
	if(!prompt)
		prompt = PROMPT;
	else if(strchr(prompt, '~'))
		defprompt = PROMPT;

start:
	putprmpt(stderr, prompt, NULL, defstr, defprompt);
	if(getinput(input)) {
		if(disp && defmesg)
			free(defmesg);
		return(1);
	}

	if(!strlen(input)) {
		if(defstr) {
			if(disp && defmesg)
				free(defmesg);
			(void) strcpy(uid, defstr);
			return(0);
		}
		if(!defmesg)
			defmesg = setmsg(disp);
		puterror(stderr, defmesg, error);
		goto start;
	} else if(!strcmp(input, "?")) {
		if(!defmesg)
			defmesg = setmsg(disp);
		puthelp(stderr, defmesg, help);
		goto start;
	} else if (!check_quit(input)) {
		if(disp && defmesg)
			free(defmesg);
		return(3);
	} else if(ckuid_val(input)) {
		if(!defmesg)
			defmesg = setmsg(disp);
		puterror(stderr, defmesg, error);
		goto start;
	}
	(void) strcpy(uid, input);
	if(disp && defmesg)
		free(defmesg);
	return(0);
}
