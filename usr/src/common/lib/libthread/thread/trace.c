/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/trace.c	1.2"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <trace.h>
#include <libthread.h>

#ifdef TRACE
/*
 * this file contains variables and functions used by the trace facility;
 * these are defined only when trace is in effect.
 */

/*
 * the following is used to determine if trace initialization has occurred
 */
boolean_t	_thr_trace_initialized = B_FALSE;	/* trace initialized? */

/*
 * the following are used for writing trace records
 */
pid_t           _thr_trace_pid;         /* current process ID         */
int             _thr_trace_forkcount;   /* count of pending fork()s   */
lwp_mutex_t     _thr_trace_lock;        /* protects global trace vars */
lwp_cond_t      _thr_trace_cond;        /* used during fork()s        */

/*
 * the following may be manipulated at run-time via environment variables
 */
long            _thr_trace_categories;  /* trace category bit pattern */
char            _thr_trace_dir[MAXPATHLEN]; /* trace file directory   */
int             _thr_trace_buf;         /* 'type' arg to setvbuf      */

/*
 * the following are used to track which categories of events to trace
 */
const int	_thr_trace_category_count = 9;	/* # of entries in _thr_event */
char		*_thr_event[] = {       /* maps ev1 to _thr_trace_categories */
			"thread",
			"mutex",
			"cond",
			"sema",
			"rwlock",
			"rmutex",
			"barrier",
			"barrier_spin",
			"spin",
			NULL };

/*
 * the following separates categories in the environment variable
 * THR_TRACE_EVENTS
 */
char *CATEGORY_DELIMITER = ":";

void    _thr_open_tracefile(thread_desc_t *tp);
void    _thr_trace_event(thread_desc_t *curtp, short ev1, short ev2,
                    short which, long a1, long a2, long a3, long a4, long a5);

/*
 * _thr_trace_init(void)
 *	Initializes trace for the process.  This includes reading trace
 *	variables from the environment, initializing global variables for
 *	the process, and opening a trace file for the initial LWP.
 *
 * Parameter/Calling State:
 *      _thr_trace_lock is held on entry and signal handlers are disabled; 
 *	called by _thr_trace_event() when first traceable event occurs.
 *
 *      Takes no arguments.
 *
 *      During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *      No return value, but trace variables are initialized and a trace
 *	file is opened for the initial LWP.  _thr_trace_lock is still held
 *	and signal handlers are still disabled on return.
 */
void
_thr_trace_init(void)
{

	int	ctr;
	char	*categories, *category, *choices, **choicesp;
	char	*tracedir, *buffering;

	/*
	 * initialize global variables
	 */
	_thr_trace_pid = getpid();
	_thr_trace_forkcount = 0;
	_thr_trace_categories = 0;

	/*
	 * determine which categories of events to trace; default is all 
	 */
	categories = getenv("THR_TRACE_EVENTS");
	if (categories == NULL) {
		/*
		 * THR_TRACE_EVENTS is not set in environment so trace
		 * all categories of events
		 */
		for (ctr = 0; ctr < _thr_trace_category_count; ctr++) {
			_thr_trace_categories |= (1 << ctr);
		}
	} else {
		/*
		 * THR_TRACE_EVENTS is set in environment so trace only
		 * requested categories
		 */
		category = strtok(categories, CATEGORY_DELIMITER);
		while (category != NULL) {
			for (choicesp = _thr_event, choices = *choicesp, ctr=0;
			     choices && strcmp(category, choices) != 0;
			     choicesp++, choices = *choicesp, ctr++) {
				PRINTF2("%s is not %s\n", category, choices);
			}
			if (choices == NULL) {
				PRINTF1("%s is not valid category\n", category);
			} else {
				_thr_trace_categories |= (1 << ctr);
				PRINTF1("tracing %s\n", _thr_event[ctr]);
			}
			category = strtok(NULL, CATEGORY_DELIMITER);
		}
	}

	/*
	 * determine directory in which to create trace files; default
	 * is the current directory
	 */
	tracedir = getenv("THR_TRACE_DIR");
	if (tracedir == NULL) {
		/*
		 * THR_TRACE_DIR is not set in environment so create
                 * trace files in current directory
                 */
		tracedir = getcwd(NULL, MAXPATHLEN);
		if (tracedir == NULL)
			_thr_panic("_thr_trace_init -- getcwd failed");
	}
	sprintf(_thr_trace_dir, "%s", tracedir);

	/*
	 * determine type of buffering to use for tracing; default is
	 * full buffering
	 */
	buffering = getenv("THR_TRACE_BUF");
	if (buffering == NULL) {
		/*
		 * THR_TRACE_BUF is NULL or not set in environment 
		 * so use full buffering
                 */
		_thr_trace_buf = _IOFBF;
	} else {
		/*
		 * THR_TRACE_BUF is non-NULL in environment so use
                 * no buffering
                 */
		_thr_trace_buf = _IONBF;
	}
		
	/*
	 * set up tracing for initial LWP
	 */
	PRINTF1("_thr_trace_init:  _thr_trace_dir = %s\n", _thr_trace_dir);
	PRINTF1("_thr_trace_init:  _thr_trace_pid = %d\n", _thr_trace_pid);
	PRINTF1("_thr_trace_init:  _thr_trace_categories = 0%x\n",
	   _thr_trace_categories);
	PRINTF1("_thr_trace_init:  _thr_trace_buf = %d\n", _thr_trace_buf);
	_thr_open_tracefile(curthread);
}


/*
 * _thr_open_tracefile(thread_desc_t *tp)
 *	Opens a trace file for the LWP on which tp is running, sets the
 *	desired buffering, and records the file pointer and buffer pointer
 *	in the LWP's private area.
 *
 * Parameter/Calling State:
 *      On entry signal handlers are disabled.
 *
 *      Argument is the thread currently running on the LWP for which a
 *	file is to be opened.
 *
 *      During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *      No return value, but a trace file is opened for the LWP on which
 *	tp is running, appropriate buffering is set up for tracing,
 *	and signal handlers are still disabled on return.
 */
void
_thr_open_tracefile(thread_desc_t *tp)
{
		char	newtracefile[MAXPATHLEN];
		int	fd;

                /*
		 * set creating thread's pid into LWP private area;
		 * this must be done first to prevent corruption in
		 * case a forkall() occurs during this function
		 */
                ((__lwp_desc_t *)tp->t_lwpp)->curpid = _thr_trace_pid;

                /*
		 * get a new buffer if buffering is enabled
		 */
		if (_thr_trace_buf == _IOFBF) {       /* buffering enabled */
                	((__lwp_desc_t *)tp->t_lwpp)->buf = 
			   (char *)malloc(BUFSIZ);
		} else {                              /* buffering disabled*/
			((__lwp_desc_t *)tp->t_lwpp)->buf = NULL;
		}

                /*
		 * open file to close-on-exec
		 */
                sprintf(newtracefile, "%s/tr.%.7x.%.3x", _thr_trace_dir,
                   _thr_trace_pid, ((__lwp_desc_t *)tp->t_lwpp)->lwp_id);
                fd = open(newtracefile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                if(fd == -1)				/* see Note 1 */
                        _thr_panic("_thr_trace_event -- open failed");
                (void) fcntl(fd, F_SETFD, 1);  /* close-on-exec */
		PRINTF1("_thr_open_tracefile:  opened trace file %s\n", 
		   newtracefile);

                /*
		 * set buffering
		 */
                ((__lwp_desc_t *)tp->t_lwpp)->tracefile = fdopen(fd, "w");
                if (((__lwp_desc_t *)tp->t_lwpp)->tracefile == NULL) {
                        _thr_panic("_thr_trace_event -- fdopen failed");
		}
                (void) setvbuf(((__lwp_desc_t *)tp->t_lwpp)->tracefile, 
		   ((__lwp_desc_t *)tp->t_lwpp)->buf, _thr_trace_buf, BUFSIZ);
}



/*
 * _thr_trace_event(thread_desc_t *curtp, short ev1, short ev2, short which,
 *    long a1, long a2, long a3, long a4, long a5)
 *	Records a trace event in the calling thread's LWP's trace file.
 *	If trace has not yet been initialized, calls _thr_trace_init().
 *	If fork() is in progress or has occurred, this function detects
 *	it and, if necessary (i.e., in the child process), opens a new 
 *	trace file before recording the event.
 *
 * Parameter/Calling State:
 *      On entry no locks are held.
 *
 *      Arguments are:
 *		curtp:  a pointer to the calling thread or 0 if the
 *			calling thread pointer is not known.
 *		ev1:    the category of event being recorded (see trace.h)
 *		ev2:    the event being recorded (see trace.h)
 *		which:  whether the only, first, or second call for the event
 *		a1:	first datum for this call of the event or 0 if n/a
 *		a2:	second datum for this call of the event or 0 if n/a
 *		a3:	third datum for this call of the event or 0 if n/a
 *		a4:	fourth datum for this call of the event or 0 if n/a
 *		a5:	fifth datum for this call of the event or 0 if n/a
 *
 *      During processing, _thr_trace_lock may be acquired if it is
 *	necessary to initialize trace or to handle fork().
 *
 * Return Values/Exit State:
 *      No return value, but a record of the indicated trace event is
 *	recorded in the trace file of the LWP on which curtp is running.
 */
void
_thr_trace_event(thread_desc_t *curtp, short ev1, short ev2, short which, 
	long a1, long a2, long a3, long a4, long a5)
{
	thread_desc_t	*cur;
	timestruc_t	curtime;
	int		rval;

	/*
	 * make sure we know calling thread's ID
	 */
	if (!curtp)
		cur = curthread;
	else
		cur = curtp;

	/*
	 * disable signal handlers to prevent preemption while
	 * accessing LWP-private data; this ensures we remain on
	 * the same LWP until we enable signal handlers again.
	 */
	_thr_sigoff(cur);
	
	/*
	 * make sure trace is initialized
	 */
	if (_thr_trace_initialized == B_FALSE) {
		LOCK_TRACE;
		if (_thr_trace_initialized == B_FALSE) {
			_thr_trace_init();
			_thr_trace_initialized = B_TRUE;
			/* 
		 	 * Initially all events are set to be traced.
		 	 * Upon initialization we may find out that
		 	 * this event is not being traced
		 	 */
			if (((1 << (ev1 - 1)) & _thr_trace_categories) == 0) {
				UNLOCK_TRACE;
				return;
			}
		}
		UNLOCK_TRACE;
	}

	/*
	 * check that fork() isn't in progress
	 */
	if (_thr_trace_forkcount > 0) {
		/*
		 * fork() or forkall() is occurring; wait till this
		 * is not the case before trying to write a trace record
		 */
		LOCK_TRACE;
		while (_thr_trace_forkcount > 0)
			TRACE_COND_WAIT;
		UNLOCK_TRACE;
	}

	/*
	 * check that fork() hasn't occurred
	 */
	while (_thr_trace_pid != ((__lwp_desc_t *)cur->t_lwpp)->curpid) {
		/*
		 * fork() has occurred; therefore we must set up a
		 * trace file and possibly a buffer for this LWP as
		 * if we were doing it for the first time.  This is
		 * done in a while-loop to protect against additional 
		 * forkall()s that may occur during processing.
		 */
		_thr_open_tracefile(cur);
	}

	/*
	 * Note that there is a window here in which fork() can occur
	 * that will result in the trace record being written to the
	 * parent's LWP's file instead of the child's.  This is not
	 * viewed as a serious enough problem (for tracing) to justify
	 * the overhead of closing this window.
	 */

	/* 
	 * get the current time and write the record
	 */
	rval = hrestime(&curtime);
	if (rval != 0) {
		_thr_panic("_thr_trace_event:  hrestime failed");
	}
	fprintf(((__lwp_desc_t *)cur->t_lwpp)->tracefile,
	    "%.9d:%.9d:%d:%d:%d:%.2d:%.2d:%.1d:%x:%x:%x:%x:%x\n",
	    curtime.tv_sec, curtime.tv_nsec, 
	    ((__lwp_desc_t *)cur->t_lwpp)->lwp_id, _thr_trace_pid, cur->t_tid, 
	    ev1, ev2, which, a1, a2, a3, a4, a5);
	_thr_sigon(cur);
}

#endif /* TRACE */
