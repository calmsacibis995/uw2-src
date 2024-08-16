/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)xdm:getinp.c	1.9"
#endif

#ifdef TESTGUI

/*
 *  getinp.c - the original content is replaced by this test program!
 *
 *	tstgui elimates the xdm dependencies and only deals with
 *	the GUI part interface. To create tstgui, include
 *	`#define USE_TESTGUI' in xdm:Imakefile, and then run
 *	`make Makefile' and `make tstgui'.
 *
 *	Note that, enable USE_TESTGUI in Imakefile only force more
 *	debugging messages from: dtlogin.c, error.c, greet.c, server.c,
 *	and nondesktop.c.
 */

#include "dtlogin.h"
#include "dm.h"

extern Display *	InitGreet(struct display *);
extern void		XdmMainLoop(struct display *, unsigned long);
extern LoginInfo	login_info;

Boolean			doit = True;
void			Doit(int, char **, struct display * d);

char *	backgroundPixmap = "/usr/X/lib/pixmaps/usl128.xpm";
char *	companyLogoPixmap = "/usr/X/lib/pixmaps/Nlogo.xpm";
int	loginProblem = 0;
int	show_mne = 1;	/* > 0 -> YES */

int
main(int argc, char **argv)
{
	struct display d;
	Display *	dpy;

	if (argc == 3)
		loginProblem = argv[2][0] == 'n' ? NOHOME : BADSHELL;
	else
	{
		printf("Valid inputs are s = success, f = fail,\
 i = inactive/idleweeks,\n\t\t\
 e = expired, m = mandatory, p = pflag a= aged\n\n"); 
		printf("2nd parameter in command line can set loginProblem\n");
		printf("to NOHOME if it's `n' otherwise, it means BADSHELL\n");
		printf("\n");

		if (argc == 1)
			exit(1);
	}

printf("%s, Dummy ManageSession\n", __FILE__);
	d.pingInterval = 0;
	d.name = "unix:0";
	dpy = InitGreet(&d);

	while(doit)
	{
		XdmMainLoop(&d, GREET_DONE_BIT);
		Doit(argc, argv, &d);
	}
	printf("%s - I got what I want, so...\n", __FILE__);
}

void
Doit(int argc, char **argv, struct display * d)
{
	char	action;
	int	op = -1;

/* See session.c:ManageSession() for op code from invoke() */
	if (argc == 1)
		action = '?';
	else
		action = argv[1][0];

	switch(action)
	{
		case 's':
			op = LOGIN_SUCCESS;
			printf("LOGIN_SUCCESS\n");
			doit = False;
			break;
		case 'f':
printf("%s: Simulating LOGIN_FAIL\n", __FILE__);
			op = LOGIN_FAIL;
			ClearTextFields(login_info);
			PopupDialog(LOGIN_FAILED, LOGIN_FAILED_M, RING_BELL);
			break;
		case 'i':
printf("%s: Simulating INACTIVE\n", __FILE__);
			op = INACTIVE; /* same as IDLEWEEKS */
			PopupDialog(PASSWORD_AGED, PASSWORD_AGED_M, RING_BELL);
			break;
		case 'e':
printf("%s: Simulating EXPIRED\n", __FILE__);
			op = EXPIRED;
			PopupDialog(ACCOUNT_AGED, ACCOUNT_AGED_M, RING_BELL);
			break;
		case 'm':
printf("%s: Simulating MANDATORY\n", __FILE__);
			op = MANDATORY;
			argv[1][0] = 'p';
		case 'p':
			if (op == -1)
			{
printf("%s: Simulating PFLAG\n", __FILE__);
				op = PFLAG;
				argv[1][0] = 'a';
			}
		case 'a':
			if (op == -1)
			{
printf("%s: Simulating AGED\n", __FILE__);
				op = PFLAG;
				op = AGED;
				argv[1][0] = 'm';
			}
			NewPasswd(LOGIN_ID, op);
			XdmMainLoop(d, NEW_PASSWD_DONE_BIT);
                        if (CHK_STATUS(NEW_PASSWD_STORED_BIT))
                        {
printf("%s (%d): got new passwd\n", __FILE__, __LINE__);
				doit = False;
                        }
                        else
                        {
				Arg	args[1];
printf("%s (%d): didn't get new passwd, so try again\n", __FILE__, __LINE__);
				DPY_LOGIN_ID_M;
				ClearTextFields(login_info);
			}
			break;
		case '?':
		default:
printf("Valid inputs are s = success, f = fail, i = inactive/idleweeks,\n\t\t\
 e = expired, m = mandatory, p = pflag a= aged\n"); 
			exit(1);
			break;
	}
}

Debug()
{
}

LogError()
{
	printf("LogError\n");
}

RegisterCloseOnFork()
{
	printf("%s: Dummy RegisterCloseOnFork()\n", __FILE__);
}

SecureDisplay()
{
	printf("%s: Dummy SecureDisplay()\n", __FILE__);
}

UnsecureDisplay()
{
	printf("%s: Dummy UnsecureDisplay()\n", __FILE__);
}

ClearCloseOnFork()
{
	printf("%s: Dummy ClearCloseOnFork()\n", __FILE__);
}

SessionPingFailed()
{
	printf("%s: Dummy SessionPingFailed()\n", __FILE__);
}

PingServer()
{
	printf("%s: Dummy PingServer()\n", __FILE__);
}

int
GetPasswdMinLen(void)
{
	printf("%s: Dummy GetPasswdMinLen\n", __FILE__);
	return(6);
}

int
StorePasswd(void)
{
#define SZ	4

	static int	index = 0;
	static int	stuff[] = {
				TOO_SHORT, CIRC, SPECIAL_CHARS,
				DF3CHARS };
	static char *	msg[] = {
				"TOO_SHORT", "CIRC", "SPECIAL_CHARS",
				"DF3CHARS" };

	int		ret_val;

	ret_val = stuff[index];
printf("%s: Dummy StorePasswd - simulate %s - %d\n", __FILE__, msg[index], index);

	if (index + 1 == SZ)
		index = 0;
	else
		index++;

	return(ret_val);
}

#endif /* TESTGUI */
