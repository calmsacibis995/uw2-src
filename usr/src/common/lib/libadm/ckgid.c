/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckgid.c	1.2.5.5"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <grp.h>
#include <unistd.h>

extern long	strtol();
extern void	*calloc(), 
		*realloc(),
		free(),
		puterror(),
		puthelp(),
		putprmpt();
extern int	getinput(),
		puttext();
extern int	check_quit();

#define PROMPT \
	gettxt("uxadm:91", "Enter the name of an existing group")
#define MESG \
	gettxt("uxadm:92", "Please enter the name of an existing group.")
#define ALTMESG \
	gettxt("uxadm:93", "Please enter one of the following group names:\\n\\t")
#define MALSIZ	64

#define DELIM1 '/'
#define BLANK ' '

static char *
setmsg(disp)
int	disp;
{
	struct group
		*grpptr;
	int	count, n, m;
	char	*msg;
	char	*altmesg = ALTMESG;

	if(disp == 0)
		return(MESG);

	m = MALSIZ;
	n = strlen(altmesg);
	if (n >= m) {
		m += MALSIZ;
	}
	if ((msg = (char *) calloc(m, sizeof(char))) == NULL)
		return(msg);
	(void) strcpy(msg, altmesg);

	setgrent();
	count = 0;
	while(grpptr = getgrent()) {
		n += strlen(grpptr->gr_name) + 2;
		while(n >= m) {
			m += MALSIZ;
			if ((msg = (char *) realloc(msg, m*sizeof(char))) == NULL )
				return(msg);
		}
		if(count++)
			(void) strcat(msg, ", ");
		(void) strcat(msg, grpptr->gr_name);
	}
	endgrent();
	return(msg);
}

int
ckgid_dsp()
{
	struct group *grpptr;

	/* if display flag is set, then list out group file */
	if (ckgrpfile() == 1)
		return(1);
	setgrent();
	while (grpptr = getgrent())
		(void) printf("%s\n", grpptr->gr_name);
	endgrent();
	return(0);
}

int
ckgid_val(grpnm)
char	*grpnm;
{
	int	valid;

	setgrent ();
	valid = (getgrnam(grpnm) ? 0 : 1);
	endgrent ();
	return(valid);
}

int
ckgrpfile() /* check to see if group file there */
{
	struct group *grpptr;

	setgrent ();
	if (!(grpptr = getgrent())) {
		endgrent ();
		return(1);
	}
	endgrent ();
	return(0);
}

void
ckgid_err(disp, error)
char	*error;
{
	char	*msg;

	msg = setmsg(disp);
	puterror(stdout, msg, error);
	if(disp)
		free(msg);
}

void
ckgid_hlp(disp, help)
char	*help;
{
	char	*msg;

	msg = setmsg(disp);
	puthelp(stdout, msg, help);
	if(disp)
		free(msg);
}

int
ckgid(gid, disp, defstr, error, help, prompt)
char	*gid;
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
			(void) strcpy(gid, defstr);
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
	} else if(ckgid_val(input)) {
		if(!defmesg)
			defmesg = setmsg(disp);
		puterror(stderr, defmesg, error);
		goto start;
	}
	(void) strcpy(gid, input);
	if(disp && defmesg)
		free(defmesg);
	return(0);
}
