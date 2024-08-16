/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)builtinext:server/builtin/aux_thread.c	1.2"

#include <stdio.h>
#include <stdlib.h>
#include <thread.h>
#include <synch.h>
#include <unistd.h>

#include "BIserver.h"
#include "aux_thread.h"

/****************************************************************************
 *	Forward Declarations
 */
static int	TryAuxThreadTakeOver(void);
static int	AuxThreadShouldRelinquish(void);

/****************************************************************************
 *	Define global/static variables and #defines, and
 *	Declare externally referenced variables
 */
thread_t	AuxThreadId = -1L;
int		AuxThreadCanRun = 0;
int		AuxThreadHasStarted = 0;
int		ThrDebug = 0;

static mutex_t	aux_thread_mutex;
static cond_t	thr_switch_cond;
static int	AuxThreadActive;
static int	AuxThreadSleep;
static int	AuxThreadTimedOut;
static int	(*ShouldRunFunc)(void *);
static void	(*DispatchFunc)(void *);
static void *	client_data;

#define	MASK_AUX_THREAD_SIG(HOW) do {		\
	sigset_t tmp_sigset;			\
	sigemptyset(&tmp_sigset);		\
	sigaddset(&tmp_sigset, AUX_THREAD_SIG);	\
	thr_sigsetmask(HOW, &tmp_sigset, NULL);	\
} while (0)

/****************************************************************************
 *
 *		PRIVATE FUNCTIONS
 */

/****************************************************************************
 * aux_sighandler(int)
 *	Signal catch routine for AUX_THREAD_SIG signal. Its sole
 *	purpose is to force the discontinuation of a sleep, if the
 *	aux_thread is blocked. 
 */
static void
aux_sighandler(int unused_signum)
{
    fprintf(stderr, "Got Signal in Aux thread\n");
}

/****************************************************************************
 * AuxThreadLoop- work loop for the aux thread.
 */
static void
AuxThreadLoop(void * unused_arg)
{
    MASK_AUX_THREAD_SIG(SIG_UNBLOCK);

    while(1)
    {
	AuxThreadTimedOut = 1;
	sleep(5);
	fprintf(stderr, "aux thread woke up...\n");
	if (AuxThreadTimedOut && AuxThreadCanRun)
	{
	    fprintf(stderr, "aux thread running...\n");
	    AuxThreadActive = 1;
	    do
	    {
		(*DispatchFunc)(client_data);
	    } while (!AuxThreadShouldRelinquish());
	    fprintf(stderr, "aux thread relinquishing...\n");
	}
    }
}

/****************************************************************************
 * AuxThreadShouldRelinquish-
 */
static int
AuxThreadShouldRelinquish(void)
{
    mutex_lock(&aux_thread_mutex);
    if (!AuxThreadSleep)
    {
	mutex_unlock(&aux_thread_mutex);
	return(0);
    }
    AuxThreadActive = 0;
    cond_signal(&thr_switch_cond);
    AuxThreadSleep = 0;
    mutex_unlock(&aux_thread_mutex);
    return(1);
}

/****************************************************************************
 *
 *		PUBLIC FUNCTIONS
 */

/***************************************************************************
 * InitAuxThread-
 */
void
AuxThreadInit(void (*dispatch_func)(void *),
	      void * data)
{
#define	STACK_SIZE	64*4096
    void *		stackaddr;

    DispatchFunc	= dispatch_func;
    client_data		= data;

    if (AuxThreadHasStarted)
    	return;
    fprintf(stderr, "Starting up Aux Thread\n");
    if (getenv("THREAD_DEBUG"))
    	ThrDebug = 1;
    (void)mutex_init(&aux_thread_mutex, USYNC_THREAD, NULL);
    (void)cond_init(&thr_switch_cond, USYNC_THREAD, NULL); /* to FALSE */
    stackaddr = (void *)malloc(STACK_SIZE);
    sigset(AUX_THREAD_SIG, aux_sighandler);
    MASK_AUX_THREAD_SIG(SIG_BLOCK);
    thr_create(stackaddr,			/* stack ptr */
	       STACK_SIZE,			/* stack size */
	       (void *(*)(void *))AuxThreadLoop,/* func */
	       NULL,				/* argp */
	       (THR_BOUND | THR_SUSPENDED),	/* flags */
	       &AuxThreadId);			/* ptr to thr id */
    AuxThreadActive = 0;
    AuxThreadSleep = 0;
    AuxThreadHasStarted = 1;
    thr_continue(AuxThreadId);
}

/***************************************************************************
 * AuxThreadInterchange-
 */
static void
AuxThreadInterchange(void)
{
    if (AuxThreadId == -1)	/* aux_thread never init'ed */
	return;
    mutex_lock(&aux_thread_mutex);
    AuxThreadSleep = 1;
    while (AuxThreadActive)
    {
	thr_kill(AuxThreadId, AUX_THREAD_SIG);
	cond_wait(&thr_switch_cond, &aux_thread_mutex);
    }
    mutex_unlock(&aux_thread_mutex);
}

static int
AuxThreadIsMe()
{
    if (thr_self() == AuxThreadId)
    	return(1);
    return(0);
}

enter_pseudo_server_mode(char *file, int line)
{
    if (AuxThreadHasStarted && AuxThreadCanRun && !AuxThreadIsMe()) {
	mutex_lock(&aux_thread_mutex);
	AuxThreadCanRun = 0;
	if (ThrDebug)
	    fprintf(stderr, "Enter pseudo-server mode, aux thread cannot run, cur_fd = %d, file = %s, line = %d\n", BIGlobal.cur_c_s_fd, file, line);
	if (AuxThreadActive) {
	    fprintf(stderr, "The Aux Thread is active, interchange, id = %d\n", thr_self());
	    mutex_unlock(&aux_thread_mutex);
	    AuxThreadInterchange();
	}
	else
	    mutex_unlock(&aux_thread_mutex);
    }
}

leave_pseudo_server_mode(char *file, int line)
{
    if (AuxThreadHasStarted && IN_CLIENT_MODE() && !AuxThreadIsMe()) {
	if (ThrDebug)
	    fprintf(stderr, "Leave pseudo-server mode, aux thread can run, cur_fd = %d, file = %s, line = %d\n", BIGlobal.cur_c_s_fd, file, line);
	mutex_lock(&aux_thread_mutex);
	AuxThreadTimedOut = 0;
	AuxThreadCanRun = 1;
	mutex_unlock(&aux_thread_mutex);
    }
}

enter_server_mode(char *file, int line)
{
    if (AuxThreadHasStarted && AuxThreadCanRun && !AuxThreadIsMe()) {
	mutex_lock(&aux_thread_mutex);
	AuxThreadCanRun = 0;
	if (ThrDebug)
	    fprintf(stderr, "Enter server mode, aux thread cannot run, cur_fd = %d, file = %s, line = %d\n", BIGlobal.cur_c_s_fd, file, line);
	if (AuxThreadActive) {
	    fprintf(stderr, "The Aux Thread is active, interchange, id = %d\n", thr_self());
	    mutex_unlock(&aux_thread_mutex);
	    AuxThreadInterchange();
	}
	else
	    mutex_unlock(&aux_thread_mutex);
    }
}

leave_server_mode(char *file, int line)
{
    if (AuxThreadHasStarted && !AuxThreadIsMe()) {
	if (ThrDebug)
	    fprintf(stderr, "Leave server mode, aux thread can run, cur_fd = %d, file = %s, line = %d\n", BIGlobal.cur_c_s_fd, file, line);
	mutex_lock(&aux_thread_mutex);
	AuxThreadTimedOut = 0;
	AuxThreadCanRun = 1;
	mutex_unlock(&aux_thread_mutex);
    }
}
