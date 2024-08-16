/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:main.c	1.22"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Auto_Authenticator/main.c,v 1.36.4.2 1995/02/09 22:38:52 plc Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ifndef NOIDENT
#ident	"xauto:main.c	1.0"
#endif
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <utmp.h>

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#include <Xm/Protocols.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>


#include <Dt/Desktop.h>

#include "main.h"
#include "dtFuncs.h"
#include "sniffproc.h"
#include <sys/sap_app.h>
#include "nwconfig.h"
#include <fcntl.h>    

#define MAGIC 666
#define OPT_STR_LEN 4

/****************************************************************
		 extern functions 
******************************************************************/
Widget postDialog (Widget parent, char *title, char *serverName, char *userName);
extern char	*GetStr (char *idstr);
Widget		TopLevel;
char *gettty(dev_t pr_ttydev);
void newvt(char *cmdString);
char * testproc( int pid );
void LoadDeviceTable(void);

/****************************************************************
		 globals 
******************************************************************/
static char *tokenName = {"nuc_xauto_panel"};
int sendDeskTopSyncReq = False;
int sendDeskTopOpenReq = False;
char *folderSpec = NULL;
char *serverName;
char *userName = NULL;
int pid = -1;
int uid = -1;
char *displayName = NULL;
int fd1;
#ifdef DEBUG
char debug_buf[256];
#endif

/****************************************************************
	forward declaration
*****************************************************************/
void addHomeToEnv(int uid);

/****************************************************************
		 HELP VARIABLES 
******************************************************************/
HelpText AppHelp = {
    TXT_appHelp, HELP_FILE, HELP_SECT,
};

/**********************************************************
	options
*********************************************************/
typedef struct {
	Boolean		debug;
} RscTable;
static RscTable		rsc_table;

static XtResource  myResources[] = {
	{ "svDebug", "Debug", XmRBoolean, sizeof(Boolean),
	  XtOffsetOf(RscTable, debug),
	  XmRImmediate, (XtPointer)False },
};

/***************************************
	main program starts here
****************************************/
void 
main ( int argc, char **argv)
{
    Widget panel;
    int i;
    XtAppContext	AppContext;
    int ttydev;
    int fd;
    char devName[256] = {"/dev/"};
    pid_t child;
    int ppid = -1;
    int tpid;
    int tuid;
    char **argvv;
    int x;
    int remoteDisplay = True;
    int singleLogin = False;
    char *textPtr;
    struct  utsname  machine;
    char name[256];
    struct passwd 	*pwd;
    extern int optind;
    extern char *optarg;
    int c;
    int reRun = False;
    int authenticated = False;
    int ac;
    char **av;
    char displayEnv[256];
    int userLoggedIn = False;
    struct  utmp *utmpp;
    char *cp;
    char * serverUserName;

	/*
	 * Preset the pid and uid to the instantiator
	 */
	pid = getpid();
	uid = getuid();

	/*
	 * Need to be root, filepriv or package prototype
	 * allows us this privilege.
	 */
	setuid(0);

#ifdef DEBUG
	fd1 = open("/dev/osm",O_WRONLY);
	print_debug("xauto called\n");
	sprintf(debug_buf,"arg cnt = %d",argc);
	print_debug(debug_buf);
	for(i=0;i < argc;i++)
	{
		print_debug(argv[i]);
	}
#endif
	XtSetLanguageProc(NULL,NULL,NULL);

	/*
	 * Pick all the arguments, old-style command line had server name
	 * ,user name first.  Adjustment of ac and av allow for
	 * continued support of that format.
	 */
	if ( argc > 1 && argv[1][0] != '-' )
	{
		ac = argc - 2;
		av = &argv[2];

		serverName = argv[SERVER_NAME_ARG_POS];
		if ( argc > 2 )
		{
			userName = argv[USER_NAME_ARG_POS];
			pwd = getpwnam (userName);
			if ( pwd != NULL )
				uid =  pwd->pw_uid;
		}
	}
	else
	{
		ac = argc;
		av = argv;
	}
	while ( (c = getopt(ac, av, "c:s:u:d:f:p:i:raq")) != -1 ) 
	{
        switch(c ) 
		{
        	case 'c':
				/*
				 * ONLY dtm uses this option.  Comes from
			 	 * nuc.cdb, user has double clicked on a 
				 * NetWare Server icon.
				 */
				folderSpec = optarg;
				if ((cp = strrchr(optarg,'/')))
				{
					cp++;
					serverName = strdup(cp);
					if ((cp = strchr(serverName,'.')))
						*cp = NULL;
				}
				else
            		serverName = optarg;
				/*
				 * Use the preset "uid" of xauto to get the user name
				 */
				pwd = getpwuid(uid);
				if ( pwd != NULL )
					userName = strdup(pwd->pw_name);

				/*
				 * Get the display env variable from the preset pid
				 */
    			displayName = (char *)testproc( pid );
					
				sendDeskTopOpenReq = True;
				reRun = True;
            	break;
        	case 's':
            	serverName = optarg;
            	break;
        	case 'u':
            	userName = optarg;
            	break;
        	case 'd':
            	displayName = optarg;
            	break;
        	case 'f':
            	folderSpec = optarg;
				sendDeskTopSyncReq = True;
            	break;
        	case 'p':
				pid = (pid_t) atoi(optarg);
            	break;
        	case 'i':
            	uid = atoi(optarg);
				pwd = getpwuid(uid);
				if ( pwd == NULL )
					uid = -1;
            	break;
        	case 'r':
            	reRun = True;
            	break;
        	case 'a':
            	authenticated = True;
            	break;
        	case 'q':
            	singleLogin = True;
            	break;
        }
    }
	/*
 	 * single login option
 	 */
	if ( singleLogin == True )
	{
		if ( isUserAuthenticated(serverName,uid) == True ||
				SLAuthenticateUidRequest(serverName, uid) == 0 )
		{
			exit(1);
		}
		exit(0);
	}

	/*
	 * If we were given a folder name argument
	 * and reRun is false then setup for a rerun 
	 * of xauto.
	 */
	if ( folderSpec != NULL && reRun == False )
	{
		char *buf;
		
		/*
		 * Need displayName from the orginal caller not
		 * the reRun instance.
		 */
		if ( displayName == NULL )
    		displayName = (char *)testproc( pid );

		i = strlen(serverName) + OPT_STR_LEN 
				+ strlen(userName) + OPT_STR_LEN
				+ 16 + OPT_STR_LEN         /* pid */ 
				+ 16 + OPT_STR_LEN         /* uid */ 
				+ strlen(folderSpec) + OPT_STR_LEN
				+ strlen(displayName) + OPT_STR_LEN
				+ OPT_STR_LEN              /* -r */ 
				+ 16                       /* /usr/X/bin/xauto */ 
				+ 1;                       /* NULL */ 
		if ((buf = (char *)malloc(i)) == NULL)
			exit(1);
		sprintf(buf,"/usr/X/bin/xauto -s %s -u %s -p %d -i %d -f %s -d %s -r",
						serverName, userName,
						pid,uid,folderSpec,displayName);

		child = fork();
	    if (child == (pid_t)-1) 
		{
        	exit(1);
    	}
    	if (child == 0) 
		{
	        execl("/bin/sh", "sh", "-c", buf, (char *)0);
            exit(0);
        }
        exit(MAGIC);
    }

	/*
	 * If Auto-Authenticator is turned off then set remoteDisplay
	 * false, disallows remote display of login panel.
	 */
	if ( NWCMGetParam(tokenName, NWCP_BOOLEAN, &remoteDisplay) != NWCM_SUCCESS )
	{
		remoteDisplay = False;
	}

	/*
	 * If uid is not valid, I can't NetWare log in this
	 * user, unknown to Unix, so fall thru and display sorry
	 * message
	 */
	if ( uid == -1 )
	{
		displayName = NULL; /* stops the login panel display */
		pid = -1; 			/* stops the vt-flip nwlogin exec */
		                    /*  and the ttydev write */
	}
	/*
	 * Setup a home dir for xauth
	 */
	if ( uid != -1 )
		addHomeToEnv(uid);

	/*
	 * lpNet calls xauto without checking the user's login status 
	 * which can cause a different user to get a login panel to 
	 * authenticate for a print job he/she  doesn't own.
	 *
	 * If lpNet is the parent of xauto 
	 * then
	 *      If the user is still logged in
	 *      then
	 *         set pid to the pid in utmp
	 *         sniff env for display
	 *            ( xdm - started then we get a displayName )
	 *          If NOT displayName 
	 *          then 
	 *              If ut_line == "console"
	 *              then
	 *                 if X is active and owned by user 
	 *                 then
	 *                    set displayName = unix:0
	 */
	if ( reRun != True )
	{
		ppid = GetParentPid(pid);
		cp = sniff_fname(ppid);
		if ( cp && (strcmp("lpNet",cp) == NULL))
		{
    		while ((utmpp = getutent()) != NULL) 
			{
				if (!strncmp(userName, utmpp->ut_user, sizeof(utmpp->ut_user)))
				{
					userLoggedIn = True;
					pid = utmpp->ut_pid;
    				displayName = (char *)testproc( pid );
					if ( displayName == NULL )
					{
#ifdef CONSOLE_OWNER_ONLY
						if ( strcmp(utmpp->ut_line,"console") == NULL)
#endif
						{
							if ( isXRunning("unix:0") == True )
							{
								xauto_ps ("X",NULL,&tuid);
								if ( tuid == uid )
									displayName = "unix:0";
							}
						}
					}
					break;
				}
    		}
		}
		if (cp)
			free(cp);
	}

	/*
 	 * Try to figure out display name from the
 	 * pid's process stack if it was not passed
 	 */
	if ( pid != -1 && displayName == NULL )
    	displayName = (char *)testproc( pid );

	/*
	 * Don't allow remote login panel display if
	 * remoteDisplay == False
	 */
	if ( displayName != NULL && remoteDisplay == False && 
	             isDisplayLocal(displayName) == False)
	{
		displayName = NULL;
	}

	/*
	 * See if the user is logged into the local system.
	 */
	if ( userLoggedIn == False )
	{
		setutent();
   		while ((utmpp = getutent()) != NULL) 
		{
			if (!strncmp(userName, utmpp->ut_user, sizeof(utmpp->ut_user)))
			{
				userLoggedIn = True;
				break;
   			}
		}
	}

	/*
	 * User has to be logged into the local system before
	 * a login panel can be displayed for that person.
	 */
	if ( userLoggedIn != True )
	{
		displayName = NULL; /* stops the login panel display */
	}
	if ( displayName != NULL && isXRunning(displayName) == True )
    {
		strcpy(displayEnv,"DISPLAY=");
		strcat(displayEnv,displayName);
		putenv(displayEnv);

		/*
		 * Motif wants to open it's internationalization catalog 
		 * file and if not found via the environment variables it 
		 * attempts to look in the current working directory.  
		 * ( I searched the whole disk and could not find the file either )
		 * This can cause a recursive auto-authentication because cwd
		 * in within NUCFS. 
		 * Solve this problem by changing the current working dir to
		 * /tmp and hope it does not inject a new problem.
		 */
		chdir("/tmp");

		XtSetLanguageProc(NULL, NULL, NULL);
   		TopLevel = XtAppInitialize(
					&AppContext,        /* app_context_return	*/
					APPNAME,            /* application_class	*/
					NULL,               /* options		*/
					0,                  /* num_options		*/
					NULL,               /* argc_in_out		*/
					NULL,               /* argv_in_out		*/
					(String *) NULL,    /* fallback_resources	*/
					(ArgList) NULL,     /* args			*/
					(Cardinal) 0        /* num_args		*/
    				);
		/* 
		 * Now get application resources... 
		 */
		XtGetApplicationResources( TopLevel, (XtPointer)&rsc_table,
					myResources, XtNumber(myResources), NULL, 0);

		/*
	 	 * Try single login for this dude
	 	 */
		if ( uid != -1 )
		{
			if ( authenticated == False &&  
					isUserAuthenticated(serverName,uid) != True &&
					SLAuthenticateUidRequest(serverName, uid) == 0 )
			{
				sendDeskTopOpenReq = True;
				reSpawnForOpen();
				exit(1);
			}
		}
		if ( authenticated == True || 
						isUserAuthenticated(serverName,uid) == True)
		{
			sendDeskTopOpenReq = True;
			sendOpenFolderReq(TopLevel);
			exit(1);
		}
	   	/*
		 * create the prompt dialog container
   		 */
		setuid (uid);  /* be the user now */
		panel = postDialog (TopLevel,GetStr(TXT_appTitle), serverName,
						 userName);

		/*
		 * Only one panel for a given server and user
		 */
		serverUserName = malloc(strlen(serverName) + strlen(userName) + 1);
		strcpy(serverUserName,serverName);
		strcat(serverUserName,userName);
		allowOnlyOne(panel,serverUserName);

		XtManageChild(panel);
		setInputFocus();
		XtAppMainLoop(AppContext);
	}
    else
    {
		setlocale(LC_ALL, "");
        LoadDeviceTable();
        ttydev = sniff_ttydev(pid);
        displayName = (char *)gettty(ttydev);
		/*
		 * Flip vt and display and run nwlogin for the user
		 */
        if ( userLoggedIn == True && ( strncmp(displayName,"vt",2) == NULL 
                            || strcmp(displayName,"/dev/console") == NULL 
                            || strcmp(displayName,"console") == NULL ))
        {
            char buf[512];
			/*
	 	 	 * Try single login for this dude
	 	 	 */
			if ( uid != -1 )
			{
				if ( SLAuthenticateUidRequest(serverName, uid) == 0 )
				{
					exit(1);
				}
			}
			if (isUserAuthenticated(serverName, uid) == True)
				exit(1);
			setuid (uid);  /* be the user now */
            sprintf(buf,"/usr/bin/nwlogin -s %s",serverName);
            newvt (buf);
        }
		/*
		 * Send to the pid's device, probably a remote terminal
		 */
        else if ( strncmp(displayName,"?",1) != NULL 
		                               && strlen(displayName) > 2 )
        {
            strcat(devName,displayName);
            fd = open(devName,O_WRONLY);
			/*
	 	 	 * Try single login for this dude
	 	 	 */
			if ( uid != -1 )
			{
				if ( SLAuthenticateUidRequest(serverName, uid) == 0 )
				{
					exit(1);
				}
			}
			if (isUserAuthenticated(serverName, uid) == True)
				exit(1);
			setuid (uid);  /* be the user now */
			textPtr = GetStr(TXT_nwLoginMsg);
        	write(fd,textPtr,strlen(textPtr));
        	write(fd,"\n",1);
        	close(fd);
		}
		/*
		 * Can't find a device, send message to /dev/console and /dev/osm
		 */
		else
		{
	
			/*
	 	 	 * Try single login for this dude
	 	 	 */
			if ( uid != -1 )
			{
				if ( SLAuthenticateUidRequest(serverName, uid) == 0 )
				{
					exit(1);
				}
			}
            fd = open("/dev/console",O_WRONLY);
            fd1 = open("/dev/osm",O_WRONLY);
			if (isUserAuthenticated(serverName, uid) == True)
				exit(1);
			setuid (uid);  /* be the user now */
			textPtr = GetStr(TXT_nwLoginMsg);
        	write(fd,textPtr,strlen(textPtr));
        	write(fd1,textPtr,strlen(textPtr));

			textPtr = GetStr(TXT_serverName);
        	write(fd,textPtr,strlen(textPtr));
        	write(fd1,textPtr,strlen(textPtr));
        	write(fd,serverName,strlen(serverName));
        	write(fd1,serverName,strlen(serverName));
        	write(fd,"  ",1);
        	write(fd1,"  ",1);

			textPtr = GetStr(TXT_login);
        	write(fd,textPtr,strlen(textPtr));
        	write(fd1,textPtr,strlen(textPtr));
        	write(fd,userName,strlen(userName));
        	write(fd1,userName,strlen(userName));
        	write(fd,"\n",1);
        	write(fd1,"\n",1);
        	close(fd);
        	close(fd1);
		}
    }
} /* End of main () */

/*
 * Routine to add the HOME env variable to this process's env.
 * Needed when the X-server is started with -auth.  The
 * .Xauthority file is found via $HOME, absense of a $HOME
 * env variable will result in a connection failure, client
 * not authroized to connect.
 */
char *home = "HOME=";
void
addHomeToEnv(int uid)
{
	char *homeDir;
	struct passwd 	*pwd;

	/*
	 * Create the HOME env string
	 */
	if ((pwd = getpwuid(uid)) == NULL)
		return;
	homeDir = (char *)malloc(strlen(home) + strlen(pwd->pw_dir) + 1 );
	if (homeDir == NULL)
		return;
	strcpy(homeDir,home);
	strcat(homeDir,pwd->pw_dir);
	putenv(homeDir);
}
allowOnlyOne(Widget shell,char *appID)
{
    static Window   another_window;
    Display *display;

    display = XtDisplay(shell);
	XtRealizeWidget(shell);
    another_window = DtSetAppId(display,
				XtWindow(shell),
                appID);
    if (another_window != None) 
	{
        XMapWindow(display, another_window);
        XRaiseWindow(display, another_window);
        XFlush(display);
        exit(0);
    }
}
/*
 * Routine: reSpawnForOpen
 *
 * Purpose: exec xauto with a folder spec so that an open folder
 *          request is sent to DTM.
 *          
 */
reSpawnForOpen()
{
	int i;
	pid_t child;

	if ( sendDeskTopOpenReq == True)
	{
		char *buf;

		i = strlen(serverName) + OPT_STR_LEN 
				+ strlen(userName) + OPT_STR_LEN
				+ 16 + OPT_STR_LEN         /* pid */ 
				+ 16 + OPT_STR_LEN         /* uid */ 
				+ strlen(folderSpec) + OPT_STR_LEN
				+ strlen(displayName) + OPT_STR_LEN
				+ OPT_STR_LEN              /* -r */ 
				+ OPT_STR_LEN              /* -a */ 
				+ 16                       /* /usr/X/bin/xauto */ 
				+ 1;                       /* NULL */ 
		if ((buf = (char *)malloc(i)) == NULL)
			exit(1);
		sprintf(buf,"/usr/X/bin/xauto -s %s -u %s -p %d -i %d -f %s -d %s -r -a"
						,serverName, userName,
						pid,uid,folderSpec,displayName);

		child = fork();
	    if (child == (pid_t)-1) 
		{
        	exit(1);
    	}
    	if (child == 0) 
		{
	        execl("/bin/sh", "sh", "-c", buf, (char *)0);
            exit(0);
        }
        exit(1);
	}
}
#ifdef DEBUG
print_debug(char *buf)
{
	write(fd1,buf,strlen(buf));
	write(fd1,"\n",1);
}
#endif
