/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)xidlelock:xidlelock.c	1.2"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/errno.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <X11/extensions/xidle.h>
#include "xidlelock.h"

#include <unistd.h>			
#include <string.h>			
#include <locale.h>			
extern char *gettxt();

#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define PRINT_DEBUG_MSG(x,a,b)	fprintf(stderr, x,a,b); fflush(stderr)
#else
#define PRINT_DEBUG_MSG(x,a,b)
#endif

extern int errno;

#define DEFAULT_LOCKER         "/usr/X/bin/xlock" 
#define SHORT_BEEP				50	/* Beep duration */
#define LONG_BEEP				200	/* Beep duration */
#define MIN_TIME				2	/* minimum number of minutes */
#define MAX_TIME				60	/* maximum number of minutes */
#define DEFAULT_TIME			0	/* number of minutes */
#define NOTIFY_TIME				30	/* number of seconds between notify */
									/* add locker execution */

/*
 * Valid resources 
 */
static XtResource resources[] = {
	{	"notify", "Notify",XtRBoolean, sizeof(Boolean),
		XtOffset(XidlelockApplicationDataPtr,notify), XtRString, "TRUE"
	},
	{	"locker", "Locker",XtRString, sizeof(char *),
		XtOffset(XidlelockApplicationDataPtr,locker), XtRString, DEFAULT_LOCKER
	},
	{	"timeout", "Timeout",XtRInt, sizeof(int),
		XtOffset(XidlelockApplicationDataPtr, timeout),
		XtRImmediate, (caddr_t)DEFAULT_TIME 
	},
};

/*
 * Valid command line options
 */
static XrmOptionDescRec options[] = {
	{	"-notify","*notify",XrmoptionNoArg,	"FALSE"},
	{	"-locker","*locker",XrmoptionSepArg,NULL},
	{	"-timeout","*timeout",XrmoptionSepArg,NULL},
};


/*
 *  Function declarations
 */

static void	Syntax();
static void	CheckExclusive (Display *, char *);
static int	CheckXidleExtension(Display * );
static int	GetLastEventTime(Display *dp, int *lastEventTime);
static void load_resources();
void reload_resources(int);


/*
 * Report the syntax for calling xidlelock and exit.
 */
static void  Syntax()
{
  fprintf (stderr,"%s\n",GetStr( TXT_usage));
  fprintf (stderr,GetStr( TXT_timeout),MIN_TIME,MAX_TIME);
  fprintf (stderr,"%s\n",GetStr( TXT_locker));
  fprintf (stderr,"%s\n",GetStr( TXT_notify));
  fprintf (stderr,"%s\n",GetStr( TXT_defaults));
  fprintf (stderr,GetStr( TXT_def_timeout));
  fprintf (stderr,GetStr( TXT_def_locker),DEFAULT_LOCKER);
  fprintf (stderr,"%s\n",GetStr( TXT_def_notify));
  exit (1);
}

/*
 *  Function for finding out whether another XidleLock or xautolock is 
 *	already running.
 */
static void  
CheckExclusive ( Display*  dp, char * prog_name)
{
	pid_t		*returned_pid;	
	pid_t		pid;	
	Atom		property;
	Atom		actual_type;
	int			actual_format;
	unsigned	long    nitems; 
	unsigned	long    bytes_after;
	int			itmp;


	/*
	 * Get the current contents of the XAUTOLOCK_SEMAPHORE_PID
	 * atom, if any.
	 */
	XGrabServer (dp);
	property = XInternAtom (dp, "XAUTOLOCK_SEMAPHORE_PID", False);
	XGetWindowProperty (dp, DefaultRootWindow(dp), property, 0L, 2L, False,
						XA_ATOM, &actual_type, &actual_format, 
						&nitems, &bytes_after,(unsigned char **)&returned_pid);

	if (actual_type == XA_ATOM)
	{
		/*
		 * Send a SIGHUP to the current running version, if still executing,
		 * so that it will reread it's user changeable resources.
		 */
		if ((itmp = kill(*returned_pid, SIGHUP)) == 0) 
		{
      		fprintf (stderr,GetStr(TXT_already), *returned_pid);
				(void) exit (1);
    	}
		else if ((errno != ESRCH) && (itmp != -1 )) 
		{
      		fprintf (stderr,GetStr(TXT_exclusive));
				(void) exit (1);
		}
	}
	/*
	 * No one running so set property to my PID
	 */
	pid = getpid ();
	XChangeProperty (dp, DefaultRootWindow(dp), property, XA_ATOM, 8,
				PropModeReplace, (unsigned char *)&pid, sizeof (pid));
	XUngrabServer (dp);
	XFree ((char*) returned_pid);
}


/*
 * Globals
 */
Display*		dp; 
char			command[256];		/* locker command to execute */
int				time_limit;			/* idle time limit */
int				notify_lock;		/* notify user or not */
int				reload = 0;			/* reload resources when set */
XidlelockApplicationData	data;	/* contains resource values at startup */

/*
 *  Main function
 */
int main(int argc,char* argv[])
{
	int				locker_pid = 0;		/* child pid */
	int				i,x; 
	int				sleepCount;
	int				lastEventTime;  
	int				status;
	char			c;
	int				Debug;
    Widget			toplevel;
	char			**argvv;
	XtAppContext	app_con;

	setlocale(LC_ALL, "");
	/*
	 * Keep the extra resources ( -xrm ) stuff to a reasonable amount
	 */
	if ( argc > 20 )
		Syntax();

	/*
	 * Copy all the user arguments into our arg buffer.  The
	 * display variable is set to "unix:0" as argument 1 so that
	 * we restrict this client to local machine only. Also remove 
	 * any user specified display argument.
	 */
	if (( argvv = (char **)malloc((argc + 3) * sizeof(char *))) == NULL )
	{
		fprintf(stderr,GetStr(TXT_malloc));
		fprintf(stderr,"\n");
		exit(1);
	}
	x = 0;
	argvv[x++] = argv[0];
	for ( i = 1; i < argc; i++ )
	{
		argvv[x] = argv[i];
		x++;
	}
	argvv[x] = NULL;
	argc = x;


	/*
	 *	Let toolkit deal with initial resources and command line options
	 */

    toplevel = XtAppInitialize (&app_con, "xidlelock", options,
					 				XtNumber(options), &argc, argvv, NULL,
									NULL,0);

    if (argc != 1) Syntax();

	/*
	 * Load the initial resources
	 */
	XtGetApplicationResources(toplevel, &data, resources,
								XtNumber(resources), NULL,0 );

	dp = XtDisplay(toplevel);

	/*
	 * Ensure that the Xidle server extensions exists
	 */
	if ( CheckXidleExtension(dp)  != True )
	{
		fprintf(stderr,GetStr(TXT_ext));
		fprintf(stderr,"\n");
		fprintf(stderr,GetStr(TXT_exec));
		fprintf(stderr,"\n");
		exit(1);
	}
	/*
	 * Only one xidlelock or xautolock active at a time.
	 */
	CheckExclusive (dp, argv[0]);

	XSync (dp, 0);   
	if ((fcntl(ConnectionNumber(dp),F_SETFD,1)) == -1 )
    {
		fprintf(stderr,GetStr(TXT_child));
		exit(1);
	}

#ifdef DEBUG
time_limit=20;
#endif

	/*
	 * Setup the SIGHUP handler
	 */
	load_resources();
	sigset(SIGHUP, reload_resources);

	lastEventTime = 0;
	sleepCount = time_limit;
	/*
	*  Main wait loop.
	*/
	for(;;)
	{
		if ( reload )
		{
			load_resources();
			lastEventTime = 0;
		}
		if ( lastEventTime >= time_limit )
			sleepCount = time_limit;
		else
		{
			sleepCount = time_limit -  lastEventTime;
			sleepCount = ( sleepCount ) ? sleepCount : 1 ;
		}
		PRINT_DEBUG_MSG("sleepCount %d \n",sleepCount,0);
		PRINT_DEBUG_MSG("notify_lock %x \n",notify_lock,0);
		sleep(sleepCount);
		if ( reload )
		{
			continue;
		}
			
		/*
		 *  Find out the time of the last event
		 */
		PRINT_DEBUG_MSG("Wakeup from sleep \n",0,0);
	    while (GetLastEventTime(dp, &lastEventTime) != True) 
		{
			/*
		 	* Couldn't get to the server. Must be grabbed.
		 	* So delay and try again.
		 	*/
			sleep(1);
		}
		PRINT_DEBUG_MSG("lastEventTime = %d\n",lastEventTime,0);
		/*
		 *   - Ring the bell, if we were asked to and are about to lock.
		 */

		if (lastEventTime >= time_limit )
		{
			PRINT_DEBUG_MSG("notify_lock test\n",0,0);
			if (notify_lock == True)
			{
				PRINT_DEBUG_MSG("notify_lock \n",0,0);
				sighold(SIGHUP);
				XBell (dp, SHORT_BEEP);
				XFlush(dp), sleep(1); 
				XBell (dp, SHORT_BEEP);
				XFlush(dp), sleep(1); 
				XBell (dp, SHORT_BEEP);
				XFlush(dp), sleep(1); 
				XBell (dp, LONG_BEEP);
				XFlush(dp);
				XBell (dp, SHORT_BEEP);
				XFlush(dp); 
				sigrelse(SIGHUP);
				PRINT_DEBUG_MSG("sleep(NOTIFY_TIME) = %d\n",NOTIFY_TIME,0);
				sleep(NOTIFY_TIME -3);
			}
			if ( reload )
			{
				continue;
			}
	    	while (GetLastEventTime(dp, &lastEventTime) != True) 
			{
				/*
		 		* Couldn't get to the server. Must be grabbed.
		 		* So delay and try again later.
		 		*/
				sleep(1);
			}
			PRINT_DEBUG_MSG("lastEventTime after notify sleep = %d\n",
															lastEventTime,0);
			/*
		 	*   - Start up a locker if the time limit has been reached.
		 	*/
	
			if (lastEventTime >= NOTIFY_TIME )
			{
				PRINT_DEBUG_MSG("Forking \n",0,0);
				switch (locker_pid = fork ())
				{
					case -1 :
						fprintf(stderr,GetStr(TXT_fork));
						exit(1);
					case 0 :
						sigignore(SIGHUP);
						(void) close (ConnectionNumber (dp));
 						system (command);
						exit (0);
					default :
						while ((wait (&status)) != locker_pid){}
						PRINT_DEBUG_MSG("Wakeup from wait \n",0,0);
						lastEventTime = 0;
						break;
				}
			}
		}
	}
}

static int
CheckXidleExtension(Display * dp)
{

    int first_event, first_error;
	int major_opcode,event;

	if (XidleQueryExtension(dp, &first_event, &first_error) != 0) 
    {
		return(True);
	}
	return(False);
}

static int
GetLastEventTime(Display *dp, int *lastEventTime)
{
	Time      idleTime;
	int	retval = False;

	sighold(SIGHUP);
    if (XGetIdleTime(dp, &idleTime)) 
	{
		PRINT_DEBUG_MSG("idltTime = %u\n",idleTime,0);
		*lastEventTime = (idleTime/1000);
		retval = True;
    }
	sigrelse(SIGHUP);
	return(retval);
}
void
reload_resources(int signo)
{
	sighold(SIGHUP);
	reload = 1;
	PRINT_DEBUG_MSG("In the SIGHUP handler routine\n",0,0);
	sigrelse(SIGHUP);
}


static void
load_resources()
{
	int argc = 1;
	XrmValue value;
	char *str_type[20];
	char 	env[256];
	char *tst;
	XrmDatabase db;

	sighold(SIGHUP);
	reload = 0;
	/*
	 * Retrieve the application specific resources.
	 * We create a new shell and application context
	 * so that we get the updated resources.
	 */


    tst = getenv("HOME");
	strcpy(env,tst);
	strcat(env,"/.Xdefaults");
	db = XrmGetFileDatabase(env);
	
	/*
	 * Get the timeout interger resource
	 */
	XrmGetResource(db,"xidlelock.timeout","XIdlelock.timeout", str_type,&value);
	if ( value.addr != NULL )
		time_limit = atoi(value.addr);
	else
		time_limit = data.timeout;

	/*
	 * Get the notify flag resource
	 */
	XrmGetResource(db,"xidlelock.notify","XIdlelock.notify", str_type,&value);

	if ( value.addr != NULL )
		if ( strcmp(value.addr,"true") == 0 )
			notify_lock = True;
		else
			notify_lock = False;
	else
		notify_lock = data.notify;
	
	/*
	 * Get the command string resource
	 */
	XrmGetResource(db,"xidlelock.locker","XIdlelock.locker", str_type,&value);
	if ( value.addr != NULL )
		strcpy(command,value.addr);
	else
		strcpy(command,data.locker);

	XrmDestroyDatabase(db);
	sigrelse(SIGHUP);

	if (time_limit == 0)
	{
		exit(1);
	}
	else if (time_limit < MIN_TIME)
	{
		fprintf (stderr,GetStr(TXT_min), MIN_TIME);
		time_limit = MIN_TIME * 60;
	}
	else if (time_limit > MAX_TIME)
	{
		fprintf (stderr,GetStr(TXT_max), MAX_TIME);
		time_limit = MAX_TIME * 60;
	}
	else
	{
		time_limit = time_limit * 60;
	}

	if ( notify_lock == True )
		time_limit = time_limit - NOTIFY_TIME;

	PRINT_DEBUG_MSG("load_resources:    command = %s \n",command,0);
	PRINT_DEBUG_MSG("load_resources:    notify = %d \n",notify_lock,0);
	PRINT_DEBUG_MSG("load_resources:    timeout = %d \n",time_limit,0);
}

/* GetStr
 *
 * Get an internationalized string.  String id's contain both the filename:id
 * and default string, separated by the FS_CHR character.
 */
char *
GetStr (char *idstr)
{
    char	*sep;
    char	*str;

    sep = strchr (idstr, FS_CHR);
    *sep = 0;
    str = gettxt (idstr, sep + 1);
    *sep = FS_CHR;

    return (str);
}	/* End of GetStr () */
