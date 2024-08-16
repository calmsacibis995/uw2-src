/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)builtinext:server/builtin/app.c	1.11"

#include "Xt/IntrinsicI.h"	/* (sic), must get it from src tree */
#include "Shell.h"
#include "StringDefs.h"

#include <dlfcn.h>

#include "BIprivate.h"

#define APP(N)	ICLIENT(N).app

/****************************************************************************
 *	Forward Declarations
 */

/****************************************************************************
 *	Define global/static variables and #defines, and
 *	Declare externally referenced variables
 */

/****************************************************************************
 *
 *		PRIVATE FUNCTIONS
 */

/****************************************************************************
 * CallWorkProc- Call any work procs that ic's have.  Return whether a work
 *	proc was called.
 */
static Boolean
CallWorkProc(int ic)
{
    XtAppContext app = APP(ic);

    /* From Xt/NextEvent.c:CallWorkProc */
    register WorkProcRec *w = app->workQueue;
    Boolean delete;

    if (w == NULL) return FALSE;

    app->workQueue = w->next;

    /* Enter client mode and call work proc */
    CALL_CLIENT_CODE(ic, delete = (*(w->proc)) (w->closure));

    if (delete) {
	Xfree(w);			/* NEW */
	/* w->next = freeWorkRecs;	/* ORIGINAL */
	/* freeWorkRecs = w;		/* ORIGINAL */
    }
    else {
	w->next = app->workQueue;
	app->workQueue = w;
    }
    /* End of NextEvent.c:CallWorkProc code */

    return(1);
}

/****************************************************************************
 * CloseDisplay-
 */
static void
CloseDisplay(int ic)
{
    static void (*real_func)(Display *) = NULL;

    if (DISPLAY(ic) == NULL)
	return;

    if (real_func == NULL)	/* ie, first-time */
    {
	real_func = (void (*)(Display *))dlsym(DL_HANDLE(ic), "XCloseDisplay");
	if (real_func == NULL)
	{
	    fprintf(stderr, "%s\n", dlerror());
	    return;
	}
    }
    CALL_CLIENT_CODE(ic, real_func((Display *)DISPLAY(ic)));
}

/***************************************************************************
 * BackgroundTasks-
 */
static int
BackgroundTasks(void)
{
    int ic;

    if (BIGlobal.num_force_close)
    {
	BIForceClose();
	return(1);
    }

    if (BIGlobal.num_force_exit)
    {
	/* run thru all i-c's looking for one that should be forced to 	
	 * exit.  Only do one per call since we're called by the
	 * BlockHandler.
	 */
	for (ic = 0; ic < NumClients; ic++)
	{
	    if (!FORCE_EXIT_IS_SET(ic))
		continue;

	    IC_FLAGS(ic) &= ~FORCE_EXIT_B;
	    BIGlobal.num_force_exit--;
	    if (!IS_EXITING(ic))
	    {
		BI_DPRINT1(stderr, "Forcing builtin %s to exit\n", ARGV0(ic));
		if (!BICallTerminatingSigHandler(ic))
		{
#ifdef BI_DEBUG
		    fprintf(stderr,
			    "Can't find sig handler to force %s to exit\n",
			    ARGV0(ic));
#endif
		    /* No handler was found so close CLIENT_TO_SERVER side and
		     * exit on behalf of client.
		     */
		    CloseDisplay(ic);
		    CALL_CLIENT_CODE(ic, exit(0));
		}
		return(1);		/* not reached */
	    }
	}
    }

    for (ic = 0; ic < NumClients; ic++)
	if (CallWorkProc(ic))
	    return(1);
    return(0);
}

/****************************************************************************
 *
 *		PUBLIC FUNCTIONS
 */

/***************************************************************************
 * BIBlockHandler- builtin block handler is used to:
 *	1. Run things in background including
 *	    a. process force_close (close server to client connection
 *		after ic has closed its side).
 *	    b. process force_exit (surrogate client has gone away so
 *		force ic to exit).
 *	    c. process ic work procs.
 *	2. Alter "timeout", if necessary.
 */
void
BIBlockHandler(void * data, void * pTimeout, void * pReadMask)
{
    extern long *		checkForInput[2];
    static struct timeval	waittime;
    struct timeval **		wt = (struct timeval **)pTimeout;

#define DONT_WAIT()	do {						\
				if (*wt == NULL)			\
				    *wt = &waittime;			\
				(*wt)->tv_sec = (*wt)->tv_usec = 0L;	\
			    } while(0)

    if (IN_NORMAL_SERVER_MODE() && BackgroundTasks())
    {
	/* If any background task was run, don't wait.  There may be output
	 * to to be flushed or calling a work proc may have generated data
	 * for client(s).
	 */
	if ((*wt == NULL) || (*wt)->tv_sec || (*wt)->tv_usec)
	    DONT_WAIT();
	return;
    }

    /* No background tasks have been run.  Return right away if:
     * 1.  there are no builtin clients
     * 2.  there's PendingInput (OK, so we know WaitForSomething() looks for
     *     PendingInput right after calling BlockHandler.)
     * 3.  wait-time is alreay 0
     */
    if (!ARE_BUILTINS())
    {
	BIRemoveHandlers();
	return;
    }
    if ((*checkForInput[0] != *checkForInput[1]) ||
	(*wt && ((*wt)->tv_sec == 0) && ((*wt)->tv_usec == 0)))
	return;

    if (IN_NORMAL_SERVER_MODE())
    {
	struct timeval *nextAppTime;
	int		i;

	if (ARE_READY_BUILTINS())
	{
	    DONT_WAIT();
	    return;
	}

	/* Handle i-c timers and work procs.  If timer has expired or (after
	 * calling work procs) there are any work procs left, make timeout 0.
	 * Else use lesser of remaining time and wt (if any).
	 */
	nextAppTime = NULL;
	for (i = 0; i < NumClients; i++)
	{
	    XtAppContext	app = APP(i);
	    struct timeval *	next;

	    if (app->timerQueue && (next = &app->timerQueue->te_timer_value) &&
		(!nextAppTime || (nextAppTime->tv_sec > next->tv_sec) ||
		 ((nextAppTime->tv_sec == next->tv_sec) &&
		  (nextAppTime->tv_usec > next->tv_usec))))
	    {
		nextAppTime = next;
	    }
	}
	if (nextAppTime)
	{
	    struct timeval now;
	    gettimeofday(&now, NULL);

	    if ((nextAppTime->tv_sec < now.tv_sec) ||
		((nextAppTime->tv_sec == now.tv_sec) &&
		 (nextAppTime->tv_usec <= now.tv_usec)))
	    {
		DONT_WAIT();
		return;
	    }

	    /* Compute remaining time and use it if less than wt */
	    if (nextAppTime->tv_usec > now.tv_usec)
	    {
		waittime.tv_sec = nextAppTime->tv_sec - now.tv_sec;
		waittime.tv_usec = nextAppTime->tv_usec - now.tv_usec;

	    } else
	    {
		waittime.tv_sec = nextAppTime->tv_sec - now.tv_sec - 1;
		waittime.tv_usec =
		    1000000 + nextAppTime->tv_usec - now.tv_usec;
	    }
	    if (*wt == NULL)
	    {
		*wt = &waittime;

	    } else if ((waittime.tv_sec < (*wt)->tv_sec) ||
		       ((waittime.tv_sec == (*wt)->tv_sec) &&
			(waittime.tv_usec < (*wt)->tv_usec)))
	    {
		(*wt)->tv_sec	= waittime.tv_sec;
		(*wt)->tv_usec	= waittime.tv_usec;
	    }
	}
    } else		/* UPPER_SERVER_MODE */
    {
	if (CUR_FD_READY())
	{
	    DONT_WAIT();
	    return;
	}

	/* If client specified timeout, use it if less than "wt" */
	if ((BIGlobal.dispatch_tv_sec >= 0L) ||
	    (BIGlobal.dispatch_tv_usec >= 0L))
	{
	    if (*wt == NULL)
	    {
		*wt = &waittime;
		waittime.tv_sec	= BIGlobal.dispatch_tv_sec;
		waittime.tv_usec= BIGlobal.dispatch_tv_usec;

	    } else if ((BIGlobal.dispatch_tv_sec < (*wt)->tv_sec) ||
		       ((BIGlobal.dispatch_tv_sec == (*wt)->tv_sec) &&
			(BIGlobal.dispatch_tv_usec < (*wt)->tv_usec)))
	    {
		(*wt)->tv_sec	= BIGlobal.dispatch_tv_sec;
		(*wt)->tv_usec	= BIGlobal.dispatch_tv_usec;
	    }
	}
    }
}

/****************************************************************************
 * XtCreateApplicationContext-
 */
XtAppContext
XtCreateApplicationContext()
{
    static XtAppContext	(*real_func)() = NULL;

    if (real_func == NULL)	/* ie, first-time */
    {
	real_func = (XtAppContext (*)())
	    dlsym(DL_HANDLE(CurClient), "XtCreateApplicationContext");
	if (real_func == NULL)
	{
	    fprintf(stderr, "%s\n", dlerror());
	    return(NULL);
	}
    }
    APP(CurClient) = real_func();
    return(APP(CurClient));
}

void
XtToolkitInitialize()
{
    static void (*real_func)() = NULL;

    if (real_func == NULL)	/* ie, first-time */
    {
	real_func = (void (*)())
	    dlsym(DL_HANDLE(CurClient), "XtToolkitInitialize");
	if (real_func == NULL)
	{
	    fprintf(stderr, "%s\n", dlerror());
	    return;
	}
	real_func();
    }
    /* else ignore subsequent calls to XtToolkitInitialize */
}

BISetDisLevel(XtAppContext app)
{
    app->dispatch_level = 0;
}
