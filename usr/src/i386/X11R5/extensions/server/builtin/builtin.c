/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)builtinext:server/builtin/builtin.c	1.30"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#include <thread.h>
#include <sys/filio.h>
#include <sys/priocntl.h>
#include <sys/tspriocntl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/stropts.h>
#include <sys/wait.h>

#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"

#include "windowstr.h"
#include "opaque.h"
#include "osstruct.h"
#include "selection.h"
#include "../os/osdep.h"

#define BUILTIN_SERVER_		/* don't want Xlib structures */
#include "builtinstr.h"
#include "aux_thread.h"
#include "BIprivate.h"

/****************************************************************************
 *	Forward Declarations
 */
static void		AddICFd(int s_c_fd);
static void		CallDispatch(int c_s_fd, long sec, long usec);
static void		CallMainLoop(int ic);
static int		CheckClose(int fd);
static int		FindIClient(int fd);
static void		FreeIClient(int ic);
static void		FreeInternalFd(int fd);
static void		IClientReset(int ic);
static void		NewInternalFd(int fd);
static void		RemoveICFd(int s_c_fd);

extern jmp_buf			startagain;
extern Bool			ClientIsAsleep();
extern fd_set			ClientsWithInput;
extern int			(*ProcVector[])();
extern ConnectionInputPtr	FreeInputs;
extern ConnectionInputPtr	AllocateInputBuffer();
extern void			Dispatch(void);
extern ClientPtr		NextAvailableClient();
extern void			SendErrorToClient();

/****************************************************************************
 *	Define global/static variables and #defines, and
 *	Declare externally referenced variables
 */
#ifdef BI_DEBUG
int BIDebug = 0;
#endif

static struct _BIClientInfo BuiltinClients[MAX_ICLIENTS];

struct _BIGlobal BIGlobal = {
    NULL,				/* connection fds */
    BuiltinClients,			/* clients */
    0L,					/* poll_addr */
    0L,					/* last_poll_addr */
    -1L,				/* dispatch_tv_sec (unset) */
    -1L,				/* dispatch_tv_usec (unset) */
    -1,					/* dummy_pipe */
    -1,					/* cur_c_s_fd */
    -1,					/* cur_client */
    0,					/* num_clients */
    0,					/* ready mask */
    0,					/* num_force_exit */
    0,					/* num_force_close */
};

#define ENVP(N)			ICLIENT(N).envp
#define ATEXIT_FUNCS(N)		ICLIENT(N).atexit_funcs
#define ATEXIT_FUNC_CNT(N)	ICLIENT(N).atexit_func_cnt
#define NUM_IC_FDS(N)		ICLIENT(N).num_ic_fds	/* num alloced */
#define IC_FDS(N)		ICLIENT(N).ic_s_c_fds
#define MAIN_LOOP(N)		ICLIENT(N).MainLoop
#define CLEANUP(N)		ICLIENT(N).CleanUp
#define SET_IC_EXITING(N)	IC_FLAGS(N) |= EXITING_B
#define SET_FORCE_EXIT(N)	IC_FLAGS(N) |= FORCE_EXIT_B
#define SET_SUSPENDED(N)	IC_FLAGS(N) |= SUSPENDED_B
#define CLR_SUSPENDED(N)	IC_FLAGS(N) &= ~SUSPENDED_B
#define SET_HAS_EVENT(N)	IC_FLAGS(N) |= HAS_EVENT_B
#define CLR_HAS_EVENT(N)	IC_FLAGS(N) &= ~HAS_EVENT_B

/* writes, selects, etc before dispatching */
static u_int	OpsBeforeDispatch = (u_int)UINT_MAX;
static u_int	OpCnt;		/* cur cnt of writes, selects, etc */
static int	NumFds;		/* size of BIGlobal.fds array */
static int	s_mwm_fd = -1;
static int	(*DefaultXErrorHandler)();
static int	(*DefaultXIOErrorHandler)();

#define CLIENT(S_C_FD)		( *((ClientPtr *)&IFD(S_C_FD)->ptr) )
#define BUF(S_C_FD)		IFD(S_C_FD)->buf
#define RPTR(S_C_FD)		IFD(S_C_FD)->rptr
#define WPTR(S_C_FD)		IFD(S_C_FD)->wptr
#define END(S_C_FD)		IFD(S_C_FD)->end
#define FD_FLAGS(FD)		IFD(FD)->flags

#define IS_INTERNAL(FD)		( ((unsigned)(FD) < NumFds) && IFD(FD))
#define IS_CLIENT_TO_SERVER(FD)	( FD_FLAGS(FD) & CLIENT_TO_SERVER )
#define IS_SERVER_TO_CLIENT(FD) !IS_CLIENT_TO_SERVER(FD)
#define SET_AS_CLIENT(FD)	FD_FLAGS(FD) |= CLIENT_TO_SERVER
#define SET_FORCE_CLOSE(S_C_FD)	FD_FLAGS(S_C_FD) |= FORCE_CLOSE
#define FORCE_CLOSE_IS_SET(S_C_FD) ( FD_FLAGS(S_C_FD) & FORCE_CLOSE )

/* 	***  Scheduling stuff  ***	*/
#define Schedule(N)		BIGlobal.readymask |= SCHED_MASK(N)
#define IsScheduled(N)		( BIGlobal.readymask & ClientMask(N) )

#define ScheduleFD(S_C_FD)		Schedule(CLIENT_INFO(S_C_FD))
#define UnscheduleFD(S_C_FD)		Unschedule(CLIENT_INFO(S_C_FD))
#define TurnOnSchedulingFD(S_C_FD)	TurnOnScheduling(CLIENT_INFO(S_C_FD))
#define TurnOffSchedulingFD(S_C_FD)	TurnOffScheduling(CLIENT_INFO(S_C_FD))
#define IsScheduledFD(S_C_FD)		IsScheduled(CLIENT_INFO(S_C_FD))

/*	***  Other macros  ***		*/

#define MAX_FUNCS 32	/* API states max of 32.  Make this dynamic (maybe) */
#define IN_STACKED_MODE()	( IN_UPPER_SERVER_MODE() && IN_CLIENT_MODE() )
#define AddHandlers() \
    RegisterBlockAndWakeupHandlers(BIBlockHandler, BIWakeupHandler, NULL)

extern void FlushIfCriticalOutputPending();
extern void ProcessInputEvents();
#define CheckAndFlush() do { \
				if (*checkForInput[0] != *checkForInput[1]) \
				{ \
				    ProcessInputEvents(); \
				    FlushIfCriticalOutputPending(); \
				} \
			    } while(0)

/****************************************************************************
 *
 *		PRIVATE FUNCTIONS
 */
/****************************************************************************
 * AddICFd-
 */
static void
AddICFd(int s_c_fd)
{
    int *	s_c_fds;
    int		ic = CLIENT_INFO(s_c_fd);

    for (s_c_fds = IC_FDS(ic);
	 s_c_fds < IC_FDS(ic) + NUM_IC_FDS(ic); s_c_fds++)
    {
	if (*s_c_fds < 0)
	{
	    *s_c_fds = s_c_fd;
	    return;
	}
    }

    /* No spare slot so grow list of fds */
    IC_FDS(ic) = (int *)
	Xrealloc(IC_FDS(ic), (NUM_IC_FDS(ic) + 1) * sizeof(int));

    IC_FDS(ic)[NUM_IC_FDS(ic)] = s_c_fd;
    NUM_IC_FDS(ic)++;
}

#ifndef NO_THREAD
/***************************************************************************
 * AuxThreadDispatch-
 */
static void
AuxThreadDispatch(void * unused_data)
{
    PokeDispatch();
}
#endif

/***************************************************************************
 * CallDispatch- save necessary state, set state for UPPER_SERVER_MODE and
 * call server's Dispatch func.
 */
static void
CallDispatch(int c_s_fd, long sec, long usec)
{
    /* Could simply save away all of BIGlobal but there may be things that
     * shouldn't be restored (like num_clients).  Currently saving most of
     * BIGlobal.
     */
    long	save_sec	= BIGlobal.dispatch_tv_sec;
    long	save_usec	= BIGlobal.dispatch_tv_usec;
    int		save_fd		= BIGlobal.cur_c_s_fd;

    BIGlobal.dispatch_tv_sec	= sec;
    BIGlobal.dispatch_tv_usec	= usec;
    BIGlobal.cur_c_s_fd		= c_s_fd;
    OpCnt			= 0L;

    /* We don't need to transition into SERVER_MODE because we are
     * already in PSEUDO_SERVER_MODE
     */
    TRANSITION_CALL(-1, Dispatch());

    BIGlobal.cur_c_s_fd		= save_fd;
    BIGlobal.dispatch_tv_sec	= save_sec;
    BIGlobal.dispatch_tv_usec	= save_usec;
}

/***************************************************************************
 * CallMainLoop- set CurClient to indicate entering CLIENT_MODE and call
 *	client's MainLoop.
 */
static void
CallMainLoop(int ic)
{
    /* Guard against calling MainLoop blindly before it's registered */
    if (!MAIN_LOOP(ic))
	return;

    BI_DPRINT5(stderr, "Calling %s's MainLoop\n", ARGV0(ic));
    CALL_CLIENT_CODE(ic, MAIN_LOOP(ic)(DISPLAY(ic), 1));
    BI_DPRINT5(stderr, "Returned from %s's MainLoop\n", ARGV0(ic));
#ifdef BI_DEBUG
    if (IS_INTERNAL(IC_FDS(ic)[0]) && DATA_AVAIL(IC_FDS(ic)[0]))
	fprintf(stderr, "Data still avail after MainLoop (%d, %s)\n",
		IC_FDS(ic)[0], ARGV0(ic));
#endif
}

/***************************************************************************
 * CheckClose- check that close of fd is valid in this mode.  To guard
 *	against the server closing CLIENT_TO_SERVER fd or a client closing a
 *	SERVER_TO_CLIENT fd or a client fd not belonging to it.
 *	Note: Called with INTERNAL fd.
 */
static int
CheckClose(int fd)
{
    if (IN_SERVER_MODE())
    {
	return(IS_CLIENT_TO_SERVER(fd));

    } else
    {
	int		ic;
	int *		s_c_fds;

	if (IS_SERVER_TO_CLIENT(fd))
	    return(1);
	if (MAP(fd) < 0)
	    return(0);

	ic = CLIENT_INFO(MAP(fd));
    
	for (s_c_fds = IC_FDS(ic);
	     s_c_fds < IC_FDS(ic) + NUM_IC_FDS(ic); s_c_fds++)
	    if ((*s_c_fds >= 0) && (MAP(*s_c_fds) == fd))
		return(0);
	return(1);
    }
}

/****************************************************************************
 * CreateDummyPipe- create fifo and return fd to it.  Use O_NDELAY since we
 * don't want reads/writes to block on a "dummy" pipe.
 */
static void
CreateDummyPipe(int * dummy_pipe)
{
    if (dummy_pipe == NULL)
	return;

    if (*dummy_pipe == -1)
    {
	const char * file = "/tmp/.XFifo";

	if (((mknod(file, S_IFIFO | 0644, 0) == 0) || (errno == EEXIST)) &&
	    ((*dummy_pipe = open(file, O_RDWR | O_NDELAY)) >= 0))
	{
	    (void)unlink(file);
	} else
	    *dummy_pipe = open("/dev/spx", O_RDWR | O_NDELAY);	/* may be -1 */
    }
}

/***************************************************************************
 * FindIClient- try to match 'fd' with the fd from any i-c's surrogate.
 */
static int
FindIClient(int fd)
{
    if (fd >= 0)
    {
	int i;

	for (i = 0; i < NumClients; i++)
	    if (CLIENT_FD(SURROGATE_CLIENT(i)) == fd)
		return(i);
    }
    return(-1);
}

static void
FreeIClient(int ic)
{
    int	i;
    int	fd;

    /* Don't try FUNC_CNT == 0 since this gets dec'ed to -1 */
    if (ATEXIT_FUNCS(ic))
	Xfree((void *)ATEXIT_FUNCS(ic));
    Xfree(STR_BUF(ic));
    Xfree(ARGV(ic));
    if (ENVP(ic))
	Xfree(ENVP(ic));
    if(IC_FDS(ic))
	Xfree(IC_FDS(ic));
    dlclose(DL_HANDLE(ic));

    /* Run thru all internal fd's and adjust client_info index */
    for (fd = 0; fd < NumFds; fd++)
	if (IS_INTERNAL(fd) && IS_SERVER_TO_CLIENT(fd) &&
	    ((int)CLIENT_INFO(fd) > ic))
	    CLIENT_INFO(fd)--;

    /* Shift other i-c's left and zero-out remainder. */
    NumClients--;
    for (i = ic; i < NumClients; i++)
    {
	ICLIENT(i) = ICLIENT(i + 1);

	if (SCHED_MASK(i))
	    SCHED_MASK(i) = ClientMask(i);

	if (IsScheduled(i+1))
	{
	    Schedule(i);
	    Unschedule(i+1);

	} else
	    Unschedule(i);
    }
    memset(&ICLIENT(i), 0, sizeof(ICLIENT(0)));
}

static void
FreeInternalFd(int fd)
{
    if (BUF(fd))
	Xfree(BUF(fd));
    if (MAP(fd) >= 0)
	MAP(MAP(fd)) = -1;
    Xfree(IFD(fd));
    IFD(fd) = NULL;
}

static void
IClientReset(int ic)
{
    int *	s_c_fds;

    /* Close down CLIENT_TO_SERVER side connection.  This will be detected in
     * close() and SERVER_TO_CLIENT close will be flagged.
     */
    for (s_c_fds = IC_FDS(ic);
	 s_c_fds < IC_FDS(ic) + NUM_IC_FDS(ic); s_c_fds++)
	if (IS_INTERNAL(*s_c_fds))
	{
	    if (IS_INTERNAL(MAP(*s_c_fds)))
		close(MAP(*s_c_fds));		/* CLIENT_TO_SERVER */
	    CLIENT_INFO(*s_c_fds) = -1;		/* no more client info */
	}
    BISigICExiting(ic);			/* clean up signal stuff */
    FreeIClient(ic);			/* dec's NumClients */
    if (NumClients == 0)
	s_mwm_fd = -1;			/* to be sure */

    /* Reset state and return to top of Dispatch. */
    BIGlobal.cur_client		= -1;
    BIGlobal.cur_c_s_fd		= -1;
    BIGlobal.dispatch_tv_sec	= -1L;
    BIGlobal.dispatch_tv_usec	= -1L;
    LEAVE_PSEUDO_SERVER_MODE();
    ENTER_SERVER_MODE();
    longjmp(startagain, 1);
}

static void
NewInternalFd(int fd)
{
    if (fd >= NumFds)
    {
	BIGlobal.fds = (struct _BIFdInfo **)
	    Xrealloc(BIGlobal.fds, (fd + 1) * sizeof(struct _BIFdInfo *));
	memset(BIGlobal.fds + NumFds, 0,
	       (fd + 1 - NumFds) * sizeof(struct _BIFdInfo *));
	NumFds = fd + 1;
    }
    IFD(fd) = (struct _BIFdInfo *)Xalloc(sizeof(struct _BIFdInfo));
    memset(IFD(fd), 0, sizeof(struct _BIFdInfo));
    MAP(fd) = -1;
}

/***************************************************************************
 * RemoveICFd- this fd is being closed so delete it from i-c list of fds.
 */
static void
RemoveICFd(int s_c_fd)
{
    int		ic = CLIENT_INFO(s_c_fd);
    int *	ic_fds = IC_FDS(ic);
    int		i, j;

    for (i = 0, j = 0; i < NUM_IC_FDS(ic); i++)
	if (ic_fds[i] != s_c_fd)
	    ic_fds[j++] = ic_fds[i];
    if (j < NUM_IC_FDS(ic))
	ic_fds[j] = -1;
}

/****************************************************************************
 * XErrHandler-
 */
#include <X11/Xlib.h>		/* for Display & XErrorEvent */
static int
XErrHandler(Display * dpy, XErrorEvent * errEvent)
{
    int ic = (MAP(dpy->fd) >= 0) ? CLIENT_INFO(MAP(dpy->fd)) : CurClient;
    /* DefaultXErrorHandler has been initialized */
    int (*func)() = XERROR_FUNC(ic) ? XERROR_FUNC(ic) : DefaultXErrorHandler;
    BI_DPRINT1(stderr, "Calling %s's XError handler (0x%x)\n",
	       ARGV0(ic), func);
    return((*func)(dpy, errEvent));
}

/****************************************************************************
 * XIOErrHandler-
 */
static int
XIOErrHandler(Display * dpy, XErrorEvent * errEvent)
{
    int ic = (MAP(dpy->fd) >= 0) ? CLIENT_INFO(MAP(dpy->fd)) : CurClient;
    /* DefaultXIOErrorHandler has been initialized */
    int (*func)() =
	XIOERROR_FUNC(ic) ? XIOERROR_FUNC(ic) : DefaultXIOErrorHandler;
    BI_DPRINT1(stderr, "Calling %s's XIOError handler (0x%x)\n",
	       ARGV0(ic), func);
    return((*func)(dpy, errEvent));
}

/****************************************************************************
 *
 *		PUBLIC FUNCTIONS
 */

void
(*libc_dlsym(char * name))()
{
    static void *	libc_handle = NULL;
    void		(*func)();

    if (libc_handle == NULL)
	if (!(libc_handle = dlopen("/usr/lib/libc.so.1", RTLD_LAZY)))
	{
	    fprintf(stderr, "%s\n", dlerror());
	    return(NULL);
	}
    func = (void (*)())dlsym(libc_handle, name);
    if (func == NULL)
	fprintf(stderr, "%s\n", dlerror());
    return(func);
}

/****************************************************************************
 * BuiltinProcRunClient-
 */
int
BuiltinProcRunClient(ClientPtr client)
{
    int		argc;
    char *	argv[100];
    char *	p;
    char	name[PATH_MAX];
    char	bname[PATH_MAX];
    int		size;
    int		(*client_main)(int, char **);
    int		cur_client = NumClients;
    int		retval;
    xReply	reply;
    REQUEST(xBuiltinRunClientReq);

    REQUEST_AT_LEAST_SIZE(xBuiltinRunClientReq);

    /* Request contains gid, uid, and cwd, then argv and env strings */
    p = (char *)&stuff[1];
    ENTER_PSEUDO_SERVER_MODE();
    if (NumClients == 0)	/* ie, 1st time only */
    {
	uid_t		uid;
	gid_t		gid;
	struct passwd *	pw_ent;

	gid = *(gid_t *)p;
	p += sizeof(gid_t);		/* jump over gid */
	uid = *(uid_t *)p;
	p += sizeof(uid_t);		/* jump over uid */

	BI_DPRINT1(stderr, "Setting gid to %d\n", gid);
	if (setgid(gid))
	    fprintf(stderr,
		    "BuiltinProcRunClient: failed to setgid(%d) (errno=%d)\n",
		    gid, errno);

	if (!(pw_ent = getpwuid(uid)) || initgroups(pw_ent->pw_name, gid))
	    fprintf(stderr, "BuiltinProcRunClient: failed to set groups\n");

	BI_DPRINT1(stderr, "Setting uid to %d\n", uid);
	if (setuid(uid))
	    fprintf(stderr,
		    "BuiltinProcRunClient: failed to setuid(%d) (errno=%d)\n",
		    uid, errno);

	BI_DPRINT1(stderr, "Setting cwd to %s\n", p);
	if (chdir((const char *)p))
	    fprintf(stderr,
		    "BuiltinProcRunClient: failed to chdir(%s) (errno=%d)\n",
		    p, errno);
    } else
    {
	p += sizeof(gid_t) + sizeof(uid_t);	/* jump over gid & uid */
    }
    p += strlen(p) + 1;		/* jump over cwd */

    /* dup remainder of req buf since it contains argv and env strings. */
    size = (stuff->length << 2) - (p - (char *)stuff);
    STR_BUF(cur_client) = (char *)Xalloc(size);
    memcpy(STR_BUF(cur_client), p, size);

    /* args are stored in request as [len(1), argv(1), len(2), argv(2), ...] */
    for (argc = 0, p = STR_BUF(cur_client); *p; argc++)
    {
	argv[argc] = p + 1;			/* point to this arg */
	p += *p + 1;				/* next arg length */
	*(argv[argc] - 1) = '\0';		/* terminate previous arg */
    }
    p++;	/* Last arg is already terminated.  Jump over terminator */

    /* Alloc array of pointers for argv */
    size = argc * sizeof(char *);
    ARGV(cur_client) = (char **)Xalloc(size);
    memcpy(ARGV(cur_client), argv, size);

    BI_DPRINT1(stderr, "Processing builtin client '%s'\n", ARGV0(cur_client));

    /* Process env var's.  Do this only the 1st time (for now) and only if
     * there are any env var's 
     */
    if ((NumClients == 0) && *(ushort *)p)
    {
	char *	prev;

	/* Fix up env strings in req buf and then putenv */
	prev = p + sizeof(ushort);
	p += *(ushort *)p + sizeof(ushort);
	do
	{
	    int len = *(ushort *)p;	/* save length */
	    *p = '\0';			/* terminate previous string */
	    if (putenv(prev))
		fprintf(stderr,
			"BuiltinProcRunClient: putenv failed on '%s'\n", prev);
	    prev = p + sizeof(ushort);
	    p += len + sizeof(ushort);	/* jump to next env var len */
	} while (*(ushort *)p);
	p += sizeof(ushort);		/* jump over terminator */
    }

    if (NumClients == 0)
    {
	/* Register Block and Wakeup Handlers (once) */
	if (!AddHandlers())
	    return(BadName);

	/* Create dummy pipe used to avoid longjmp in catch_xsig (xwin_io.c) */
	CreateDummyPipe(&BIGlobal.dummy_pipe);
	if (BIGlobal.dummy_pipe < 0)
	    return(BadName);
    }

    /* Save client ptr of "launching" client for kill processing */
    SURROGATE_CLIENT(cur_client) = client;

    /* The convention is to dlopen basename of argv[0] with ".so" appended */
    strcpy(name, argv[0]);
    strcpy(bname, basename(name));
    if (!(DL_HANDLE(cur_client) =
	  dlopen(strcat(bname, ".so"), (RTLD_LAZY | RTLD_GLOBAL))))
    {
	fprintf(stderr, "%s\n", dlerror());
	retval = BadName;
    err:
	BIRemoveHandlers();
	Xfree((char *)ARGV(cur_client));
	/* Should Xfree STR_BUF but what about env? */
	return(retval);
    }
	
    if (!(client_main = (int (*)())dlsym(DL_HANDLE(cur_client), "main")))
    {
	fprintf(stderr, "%s\n", dlerror());
	if (dlclose(DL_HANDLE(cur_client)))
	    fprintf(stderr, "%s\n", dlerror());
	retval = BadName;
	goto err;
    }
#ifdef BI_DEBUG
    if ((p = getenv("OpsBeforeDispatch")))
	OpsBeforeDispatch = atoi((const char *)p);
#endif

    NumClients++;				/* Update NumClients */

#ifndef NO_THREAD
    if (!getenv("NO_AUX_THREAD"))
	AuxThreadInit(AuxThreadDispatch, NULL);
#endif

    /* Set up reply and send it to the surrogate now */
    reply.generic.type			= X_Reply;
    reply.generic.length		= 0;
    reply.generic.sequenceNumber	= client->sequence;
    WriteToClient(client, sizeof(xReply), (char *)&reply);
    /*
     * Call ic's main()
     *
     * Requests may be stacked (ie, dtm starts mwm) so save CurClient.
     */
    BI_DPRINT1(stderr, "Calling %s's main\n", ARGV0(cur_client));
    CALL_CLIENT_CODE(cur_client, (void)client_main(argc, argv));
#ifdef BI_DEBUG
    if (ICLIENT(cur_client).MainLoop == NULL)
	fprintf(stderr, "MainLoop not registered by %s\n", ARGV0(cur_client));
#endif
    /* If surrogate has exited during main(), can't return
     * client->noClientException since client may no longer be valid.  Just
     * return success since there's no point in returning failure.
     * Otherwise, return the usual.
     */
    if (SURROGATE_CLIENT(cur_client))
    {
	retval = client->noClientException;
	Schedule(cur_client);
    } else
	retval = Success;
    LEAVE_PSEUDO_SERVER_MODE();
    return(retval);
}

/****************************************************************************
 * BuiltinDispatch-
 */
int
BuiltinDispatch(void)
{
    int ic;

    /* Return immediately if there has been a dispatchException.  This means
     * returning back to NORMAL_SERVER_MODE if we're in UPPER_SERVER_MODE.
     */
    if (dispatchException)
	return(IN_UPPER_SERVER_MODE());

    if (IN_UPPER_SERVER_MODE())
    {
	/* Return from UPPER_SERVER_MODE if data available for cur_fd */
	if (CUR_FD_READY())
	    return(1);

	/* In UPPER_SERVER_MODE, consider "stacking" mwm on top of current
	 * client.  If there's data for mwm, call its main loop. Otherwise,
	 * return now.  We're sure that mwm isn't cur-client with data
	 * because that's caught above.
	 */
	if ((s_mwm_fd >= 0) && DATA_AVAIL(s_mwm_fd) &&
	    !IS_SUSPENDED(CLIENT_INFO(s_mwm_fd)))
	{
#ifdef BI_DEBUG
	    fprintf(stderr, "Stacking %s on top of %s\n",
		    ARGV0(CLIENT_INFO(s_mwm_fd)),
		    ARGV0(CLIENT_INFO(MAP(BIGlobal.cur_c_s_fd))));
#endif
	    CallMainLoop(CLIENT_INFO(s_mwm_fd));

	    /* cur_c_s_fd may have become ready.  If so, return from
	     * UPPER_SERVER_MODE now.
	     */
	    if (CUR_FD_READY())
		return(1);
	}

	if ((BIGlobal.dispatch_tv_sec > 0L) ||	/* implies */
	    (BIGlobal.dispatch_tv_usec > 0L) ||	/*  UPPER_SERVER_MODE */
	    dispatchException)
	    return(1);

	return(0);	/* Remain in UPPER_SERVER_MODE */
    }

    /* NORMAL_SERVER_MODE: While events avail for internal client(s),
     * dispatch them
     */
    while (ARE_READY_BUILTINS())
	for (ic = 0; ic < NumClients; ic++)
	    if (IsScheduled(ic))
	    {
		CallMainLoop(ic);

		if (dispatchException)
		    return(0);
	    }
    return(0);
}

/****************************************************************************
 * BIAttendClient- called by AttendClient (connection.c) when client is woken
 * up.
 */
void
BIAttendClient(int fd)
{
    if (IS_INTERNAL(fd) && IS_SERVER_TO_CLIENT(fd) && (CLIENT_INFO(fd) >= 0))
	SET_HAS_EVENT(CLIENT_INFO(fd));
}

/****************************************************************************
 * BIForceClose- run thru all fd's looking for a SERVER_TO_CLIENT fd that
 *	should be closed.  Only do one per call since this is being done in
 *	"background" (BlockHandler).  This is called and executes in
 *	SERVER_MODE.
 */
void
BIForceClose()
{
    int fd;

    for (fd = 0; fd < NumFds; fd++)
	if (IS_INTERNAL(fd) && IS_SERVER_TO_CLIENT(fd) &&
	    FORCE_CLOSE_IS_SET(fd))
	{
	    BI_DPRINT1(stderr, "Forcing close on %d\n", fd);
	    BIGlobal.num_force_close--;
	    FD_FLAGS(fd) &= ~FORCE_CLOSE;
	    if (CLIENT(fd))
		CloseDownClient(CLIENT(fd));
	    break;
	}
}

int
BIIsServerFd(int s_c_fd)
{
    return(IS_INTERNAL(s_c_fd) && IS_SERVER_TO_CLIENT(s_c_fd));
}

/****************************************************************************
 * BIRegisterClient-
 */
void
BIRegisterClient(int c_s_fd, void * dpy,
		 void (*MainLoop)(void *, int), void (*CleanUp)(void *))
{
    int ic = CLIENT_INFO(MAP(c_s_fd));

    BI_DPRINT1(stderr, "%s registering MainLoop\n", ARGV0(ic));

    DISPLAY(ic)		= dpy;
    MAIN_LOOP(ic)	= MainLoop;
    CLEANUP(ic)		= CleanUp;

    /* process events generated during init: */
    if (!dispatchException)
	CallMainLoop(ic);
}

/****************************************************************************
 * BIWakeupHandler- called after WaitForSomething wakes up.
 *	Only interested in timeout (result == 0).  Call ic's MainLoops if
 *	WaitForSomething timed out.  Should really only call MainLoops if
 *	timeout belongs to ic but it's easier to just call all MainLoops.
 */
void
BIWakeupHandler(pointer data, unsigned long result, pointer pReadmask)
{
    int ic;

    if (!dispatchException && (result == 0) && IN_NORMAL_SERVER_MODE())
	for (ic = 0; ic < NumClients; ic++)
	    CallMainLoop(ic);
}

/****************************************************************************
 * XSetErrorHandler- from lib/X/XErrHndlr.c
 */
XErrorHandler XSetErrorHandler(handler)
    register XErrorHandler handler;
{
    static int	(*(*real_func)())() = NULL;
    int		(*prev)();

    if (real_func == NULL)	/* ie, first-time */
    {
	real_func = (int (*(*)())())
	    dlsym(DL_HANDLE(CurClient), "XSetErrorHandler");
	if (real_func == NULL)
	{
	    fprintf(stderr, "%s\n", dlerror());
	    return(NULL);		/* NOTE: fix this */
	}
	DefaultXErrorHandler = (*real_func)(XErrHandler);
    }
    /* DefaultXErrorHandler has been initialized */
    prev = XERROR_FUNC(CurClient) ?
	XERROR_FUNC(CurClient) : DefaultXErrorHandler;
    XERROR_FUNC(CurClient) = handler ? handler : DefaultXErrorHandler;
    return(prev);
}

/****************************************************************************
 * XSetIOErrorHandler- from lib/X/XErrHndlr.c
 */
XIOErrorHandler XSetIOErrorHandler(handler)
    register XIOErrorHandler handler;
{
    static int	(*(*real_func)())() = NULL;
    int		(*prev)();

    if (real_func == NULL)	/* ie, first-time */
    {
	real_func = (int (*(*)())())
	    dlsym(DL_HANDLE(CurClient), "XSetIOErrorHandler");
	if (real_func == NULL)
	{
	    fprintf(stderr, "%s\n", dlerror());
	    return(NULL);		/* NOTE: fix this */
	}
	DefaultXIOErrorHandler = (*real_func)(XIOErrHandler);
    }
    /* DefaultXIOErrorHandler has been initialized */
    prev = XIOERROR_FUNC(CurClient) ?
	XIOERROR_FUNC(CurClient) : DefaultXIOErrorHandler;
    XIOERROR_FUNC(CurClient) = handler ? handler : DefaultXIOErrorHandler;
    return(prev);
}

/****************************************************************************
 *	Override some X and Xt functions
 */
int
_XConnectDisplay (display_name, fullnamep, dpynump, screenp,
		  auth_namep, auth_namelenp, auth_datap, auth_datalenp)
    char *display_name;
    char **fullnamep;			/* RETURN */
    int *dpynump;			/* RETURN */
    int *screenp;			/* RETURN */
    char **auth_namep;			/* RETURN */
    int *auth_namelenp;			/* RETURN */
    char **auth_datap;			/* RETURN */
    int *auth_datalenp;			/* RETURN */
{
    int			c_s_fd = -1;
    int			s_c_fd = -1;
    OsCommPtr		oc;
    extern int		ConnectionTranslation[];
    extern int		GrabInProgress;
    extern fd_set	AllSockets;
    extern fd_set	AllClients;
    extern char *	display;

    ENTER_PSEUDO_SERVER_MODE();

    /* Create "client to server" connection for internal client */
    CreateDummyPipe(&c_s_fd);
    if (c_s_fd < 0)
    {
	LEAVE_PSEUDO_SERVER_MODE();
	return(-1);
    }
    NewInternalFd(c_s_fd);
    SET_AS_CLIENT(c_s_fd);
    BI_DPRINT1(stderr, "Fd from %s to server is %d\n",
	       ARGV0(CurClient), c_s_fd);

    /* Create "server to client" connection */
    CreateDummyPipe(&s_c_fd);
    if (s_c_fd < 0)
    {
	LEAVE_PSEUDO_SERVER_MODE();
	return(-1);
    }
    NewInternalFd(s_c_fd);
    CLIENT_INFO(s_c_fd) = CurClient;
    MAP(s_c_fd) = c_s_fd;
    MAP(c_s_fd) = s_c_fd;
    AddICFd(s_c_fd);
    BI_DPRINT1(stderr, "Fd from server to %s is %d\n",
	       ARGV0(CurClient), s_c_fd);

    /* Create a new client */
    oc					= (OsCommPtr)xalloc(sizeof(OsCommRec));
    oc->input				= (ConnectionInputPtr)NULL;
    oc->output				= (ConnectionOutputPtr)NULL;
    oc->conn_time			= 0;
    oc->fd				= s_c_fd;
    CLIENT(s_c_fd)			= NextAvailableClient(oc);
    ConnectionTranslation[s_c_fd]	= CLIENT(s_c_fd)->index;
    IgnoreClient(CLIENT(s_c_fd));
    FD_CLR(s_c_fd, &ClientsWithInput);
#ifdef NOT_YET
    if (GrabInProgress)
    {
	FD_SET(s_c_fd, &SavedAllClients);
	FD_SET(s_c_fd, &SavedAllSockets);
    } else
#endif
    {
	FD_SET(s_c_fd, &AllClients);
	FD_SET(s_c_fd, &AllSockets);
    }

    *fullnamep = (char *)Xalloc(6);
    strcpy(*fullnamep, ":");
    strcat(*fullnamep, display);
    *dpynump = 0;
    *screenp = 0;
    *auth_namelenp = 0;
    *auth_datalenp = 0;
    *auth_namep = NULL;
    *auth_datap = NULL;

    /* Keep track of connection to window manager (mwm) so that mwm can be
     * stacked on top of other clients waiting for replies from it.
     * NOTE: need better way to detect window manager client.
     */
    if (s_mwm_fd < 0)
    {
	char argv0[PATH_MAX];
	strcpy(argv0, ARGV0(CLIENT_INFO(s_c_fd)));
	if (strcmp(basename(argv0), "mwm") == 0)
	    s_mwm_fd = s_c_fd;
    }
    LEAVE_PSEUDO_SERVER_MODE();
    return(c_s_fd);
}

/***************************************************************************
 *
 *	Override libc functions (read, write, select, etc)
 */
int
atexit(void (*func)(void))
{
    ENTER_PSEUDO_SERVER_MODE();
    if (IN_CLIENT_MODE())
    {
	if (ATEXIT_FUNC_CNT(CurClient) >= MAX_FUNCS) {
	    LEAVE_PSEUDO_SERVER_MODE();
	    return(-1);
	}
	if (ATEXIT_FUNCS(CurClient) == NULL)
	{
	    ATEXIT_FUNCS(CurClient) =
		(void (*(*))())Xalloc(MAX_FUNCS * sizeof(void (*)()));
	    if (ATEXIT_FUNCS(CurClient) == NULL) {
		LEAVE_PSEUDO_SERVER_MODE();
		return(-1);
	    }
	}
	ATEXIT_FUNCS(CurClient)[ATEXIT_FUNC_CNT(CurClient)++] = func;
	LEAVE_PSEUDO_SERVER_MODE();
	return(0);
    } else
    {
	static int (*real_func)() = NULL;

	if (real_func == NULL)		/* ie, first-time */
	    if ((real_func = (int (*)())libc_dlsym("atexit")) == NULL) {
		LEAVE_PSEUDO_SERVER_MODE();
		return(-1);
	    }

	LEAVE_PSEUDO_SERVER_MODE();
	return(real_func(func));
    }
}

close(int fd)
{
    int ret_val;

    ENTER_PSEUDO_SERVER_MODE();
    if (IS_INTERNAL(fd))
    {
	if (CheckClose(fd))
	{
#ifdef BI_DEBUG
	    fprintf(stderr, "CheckClose failed: %s closing %d\n",
		    IN_CLIENT_MODE() ? ARGV0(CurClient) : "server", fd);
#endif
	    LEAVE_PSEUDO_SERVER_MODE();
	    return(-1);
	} else
	{
	    ret_val = _close(fd);
	    if (IS_SERVER_TO_CLIENT(fd))
	    {
		/* Remove this from list for this client */
		if (CLIENT_INFO(fd) >= 0)
		    RemoveICFd(fd);
		if (fd == s_mwm_fd)
		    s_mwm_fd = -1;
	    } else
	    {
		/* Client is closing it's side of the connection.  Mark need
		 * to close server's side.
		 */
		if ((MAP(fd) >= 0) && !FORCE_CLOSE_IS_SET(MAP(fd)))
		{
		    BI_DPRINT3(stderr, "Setting FORCE_CLOSE on %s\n",
			       ARGV0(CLIENT_INFO(MAP(fd))));
		    SET_FORCE_CLOSE(MAP(fd));
		    BIGlobal.num_force_close++;
		}
	    }
	    FreeInternalFd(fd);
	}
    } else
    {
	int ic;

	ret_val = _close(fd);

	if (IN_SERVER_MODE() && (ic = FindIClient(fd)) >= 0)
	{
	    /* Server is closing connection to surrogate process so force an
	     * exit on corresponding i-c.  (Note: SERVER_MODE includes NORMAL
	     * and UPPER server mode).
	     * NOTE: not handling the case where UPPER_SERVER_MODE & !cur_fd
	     */
#ifdef BI_DEBUG
	    if (IN_UPPER_SERVER_MODE() &&
		(CLIENT_INFO(BIGlobal.cur_c_s_fd) != ic))
		fprintf(stderr,
			"Server closing %s's surrogate while %s running\n",
			ARGV0(ic),
			ARGV0(CLIENT_INFO(MAP(BIGlobal.cur_c_s_fd))));
#endif
	    if (!IS_EXITING(ic) && !FORCE_EXIT_IS_SET(ic))
	    {
		BIGlobal.num_force_exit++;
		SET_FORCE_EXIT(ic);
		SURROGATE_CLIENT(ic) = NULL;	 /* indicate it's gone */
		BI_DPRINT3(stderr, "Setting FORCE_EXIT on %s\n", ARGV0(ic));
	    }
	}
    }
    LEAVE_PSEUDO_SERVER_MODE();
    return(ret_val);
}

int
execvp(const char * file, char * const * argv)
{
    ENTER_PSEUDO_SERVER_MODE();
    if (IN_CLIENT_MODE())
    {
	char	file_name[PATH_MAX];
	char	argv_name[PATH_MAX];
	strcpy(file_name, file);
	strcpy(argv_name, ARGV0(CurClient));
	if (strcmp(basename(argv_name), basename(file_name)) == 0)
	{
	    int	ic = CurClient;

	    TurnOffScheduling(ic);
	    Unschedule(ic);

	    if (SURROGATE_CLIENT(ic))
		BuiltinSendExecEvent(SURROGATE_CLIENT(ic));

	    IClientReset(ic);
	    /* NOTREACHED */
	    return(0);
	}
    }
    /* not "else" since i-c can exec others besides itself */
    LEAVE_PSEUDO_SERVER_MODE();
    return(_execvp(file, argv));
}

static void
do_exit(int code, int underbar_exit)
{
    int	ic = CurClient;
#ifdef BI_DEBUG
    if (IN_STACKED_MODE())
	fprintf(stderr, "%s exiting while stacked on %s\n",
		ARGV0(ic), ARGV0(CLIENT_INFO(MAP(BIGlobal.cur_c_s_fd))));
#endif
    if (!IS_EXITING(ic))
    {
	SET_IC_EXITING(ic);		/* flag that this i-c is exiting */

	TurnOffScheduling(ic);
	Unschedule(ic);

	/* Close down the surrogate client when corresponding i-c exits. */
	if (SURROGATE_CLIENT(ic))
	    BuiltinSendExitEvent(SURROGATE_CLIENT(ic), code);

	BI_DPRINT3(stderr, "%s is exiting\n", ARGV0(ic));
    }
    if (!underbar_exit)
	/* Call atexit funcs, if any */
	while(--ATEXIT_FUNC_CNT(ic) >= 0)
	    (*ATEXIT_FUNCS(ic)[ATEXIT_FUNC_CNT(ic)])();

    if (CLEANUP(ic))
	CLEANUP(ic)(DISPLAY(ic));

    IClientReset(ic);
    /*NOTREACHED*/
}

void
_exit(int code)
{
    ENTER_PSEUDO_SERVER_MODE();
    if (IN_CLIENT_MODE())
	do_exit(code, 1);
	/*NOTREACHED*/
    else
    {
	static void (*real_func)(int) = NULL;
	if (real_func == NULL)
	    real_func = (void (*)(int))libc_dlsym("_exit");
	real_func(code);
	/*NOTREACHED*/
    }
}

void
exit(int code)
{
    ENTER_PSEUDO_SERVER_MODE();
    if (IN_CLIENT_MODE())
	do_exit(code, 0);
	/*NOTREACHED*/
    else
    {
	static void (*real_func)(int) = NULL;
	if (real_func == NULL)
	    real_func = (void (*)(int))libc_dlsym("exit");
	real_func(code);
	/*NOTREACHED*/
    }
}

pid_t
fork(void)
{
    pid_t pid = _fork1();

    /* Reset scheduling class to time-share in child process */
    if (pid == 0)
    {
	static pcparms_t parms = { -1, { NULL, }, };
	if (parms.pc_cid == -1)		/* ie, first time */
	{
	    pcinfo_t info;
	    strcpy (info.pc_clname, "TS");
	    (void)priocntl(0, 0, PC_GETCID, &info);
	    parms.pc_cid = info.pc_cid;
	    ((tsparms_t *)parms.pc_clparms)->ts_uprilim =
		((tsparms_t *)parms.pc_clparms)->ts_upri = TS_NOCHANGE;
	}
	(void)priocntl(P_PID, P_MYID, PC_SETPARMS, &parms);
    }
    return(pid);
}

ioctl(int fd, int cmd, void *arg1, void *arg2, void *arg3, void *arg4)
{
    ENTER_PSEUDO_SERVER_MODE();
    if (IS_INTERNAL(fd) && IS_CLIENT_TO_SERVER(fd) &&
	((cmd == FIONREAD) || (cmd == I_NREAD)))
    {
	CheckAndFlush();

	*((int *) arg1) = WPTR(MAP(fd)) - RPTR(MAP(fd));
	LEAVE_PSEUDO_SERVER_MODE();
	return(0);
    }
#ifdef BI_DEBUG	
    if (IS_INTERNAL(fd))
	if (IS_SERVER_TO_CLIENT(fd))
	    fprintf(stderr,
		    "ioctl: called with SERVER_TO_CLIENT fd (%d)\n", fd);
	else if (((cmd >> 8) & 0xff) != 'S')
	    fprintf(stderr, "ioctl: called by '%s' with cmd 0x%x\n",
		    ARGV0(CurClient), cmd);
#endif
    LEAVE_PSEUDO_SERVER_MODE();
    return(_ioctl(fd, cmd, arg1, arg2, arg3, arg4));
}

/*
 * open- override this in SERVER_MODE so that close-on-exec can be set on
 *	the fd.  This is because the server didn't used to do exec's but
 *	no it does due to builtin clients doing exec.
 */
int
open(const char *path, int oflag, ...)
{
    va_list	ap;
    int		fd;

    ENTER_PSEUDO_SERVER_MODE();
    va_start(ap, oflag);
    fd = _open(path, oflag, va_arg(ap, mode_t));

    if ((fd >= 0) && IN_SERVER_MODE())
	if(fcntl(fd, F_SETFD, 1) == -1)
	    fprintf(stderr, "Failed to set close-on-exec on %d (errno=%d)\n",
		fd, errno);
    va_end(ap);
    LEAVE_PSEUDO_SERVER_MODE();
    return(fd);
}

/***************************************************************************
 * check_read- common code for read & readv before doing actual reading.
 */
static int
check_read(int c_s_fd)
{
#ifdef BI_DEBUG
    /* Server should not be doing read on i-c fd */
    if (IS_SERVER_TO_CLIENT(c_s_fd))
    {
	fprintf(stderr, "check_read: called with SERVER_TO_CLIENT fd\n");
	return(-1);
    }
#endif
    /* Fail the read if server side has shutdown */
    if (MAP(c_s_fd) < 0)
    {
	BI_DPRINT1(stderr, "check_read: connection to %s has been shut down.\n",
		   ARGV0(CurClient));
	return(-1);
    }
    CheckAndFlush();

    /* Do a dispatch pass only if nothing avail for i-c to read */
    if (!DATA_AVAIL(MAP(c_s_fd)))
    {
	CallDispatch(c_s_fd, -1L, -1L);
	if (MAP(c_s_fd) < 0)
	{
	    fprintf(stderr,
		    "check_read: in %s read() - MAP(fd) < 0 after Dispatch\n",
		    ARGV0(CurClient));
	    return(-1);
	}
	if (!DATA_AVAIL(MAP(c_s_fd)))
	{
	    fprintf(stderr,
		    "check_read: no data avail for %s after Dispatch\n",
		    ARGV0(CurClient));
	    return(-1);
	}
    }
    return(0);
}

/***************************************************************************
 * do_read- satisfy read by internal client.
 */
static
do_read(int c_s_fd, char * buf, int size)
{
    int s_c_fd = MAP(c_s_fd);
    int len = min(size, WPTR(s_c_fd) - RPTR(s_c_fd));

    (void)memcpy(buf, RPTR(s_c_fd), len);
    RPTR(s_c_fd) += len;
    if (WPTR(s_c_fd) == RPTR(s_c_fd))
	WPTR(s_c_fd) = RPTR(s_c_fd) = BUF(s_c_fd);
    return(len);
}

ssize_t
read(int fd, void * buf, unsigned int size)
{
    int len;

    /* do normal read() for "external" clients */
    if (!IS_INTERNAL(fd))
	return(_read(fd, buf, size));

    ENTER_PSEUDO_SERVER_MODE();
    if (check_read(fd)) {
	LEAVE_PSEUDO_SERVER_MODE();
	return(-1);
    }
    len = do_read(fd, buf, size);
    BI_DPRINT7(stderr, "Reading from internal fd: %d, nbytes: %d\n", fd, len);
    LEAVE_PSEUDO_SERVER_MODE();
    return(len);
}

int
readv(int fd, const struct iovec * iov, int iovcnt)
{
    int len;
    int i;

    /* do normal readv() for "external" clients */
    if (!IS_INTERNAL(fd))
	return(_readv(fd, iov, iovcnt));

    ENTER_PSEUDO_SERVER_MODE();
    if (check_read(fd)) {
	LEAVE_PSEUDO_SERVER_MODE();
	return(-1);
    }

    len = 0;
    for (i = 0; i < iovcnt; i++)
    {
	len += do_read(fd, iov[i].iov_base, iov[i].iov_len);
	if (WPTR(MAP(fd)) == RPTR(MAP(fd)))
	    break;
    }
    BI_DPRINT7(stderr, "Readving from internal fd: %d, nbytes: %d\n", fd, len);
    LEAVE_PSEUDO_SERVER_MODE();
    return(len);
}

_abi_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    static int (*real_select)() = NULL;

    ENTER_PSEUDO_SERVER_MODE();
    if (IN_CLIENT_MODE())
    {
	/* There is no easy way to do this for multiple internal fd's, but,
	 * luckily, there really is very little reason to have multiple
	 * internal fd's (this would be multiple connections to the same
	 * server).  Motif clients establish a second connection, but send
	 * one message and close the connection.  So, we will assume that
	 * there is only one fd and streamline the code.
	 */
	int s_c_fd;

	CheckAndFlush();

	/* See if current i-c is looking for data from the server.
	 * If not, call the "normal" select.
	 */
	if (NUM_IC_FDS(CurClient) &&
	    ((s_c_fd = IC_FDS(CurClient)[0]) >= 0) &&
	    (MAP(s_c_fd) >= 0) && FD_ISSET(MAP(s_c_fd), readfds))
	{
	    if (DATA_AVAIL(s_c_fd))
	    {
		CLEARBITS(readfds->fds_bits);
		FD_SET(MAP(s_c_fd), readfds);
		LEAVE_PSEUDO_SERVER_MODE();
		return(1);
	    }

	    FD_CLR(MAP(s_c_fd), readfds);
	    if (!ANYSET(readfds->fds_bits) && timeout &&
		(timeout->tv_sec == 0) && (timeout->tv_usec == 0))
	    {
		LEAVE_PSEUDO_SERVER_MODE();
		return(0);
	    }

	    if (timeout)
		/* Don't set SUSPENDED so will return when data avail */
		CallDispatch(MAP(s_c_fd), timeout->tv_sec, timeout->tv_usec);
	    else
		CallDispatch(MAP(s_c_fd), -1L, -1L);

	    if (DATA_AVAIL(s_c_fd))
	    {
		CLEARBITS(readfds->fds_bits);
		FD_SET(MAP(s_c_fd), readfds);
		LEAVE_PSEUDO_SERVER_MODE();
		return(1);
	    }
	    if (!ANYSET(readfds->fds_bits))
	    {
		LEAVE_PSEUDO_SERVER_MODE();
		return(0);
	    }
	}
    }
    /* Not "else" since above 'if' can fall thru */

    if (real_select == NULL)
	real_select = (int (*)())libc_dlsym("_abi_select");
    LEAVE_PSEUDO_SERVER_MODE();
    return(real_select(nfds, readfds, writefds, exceptfds, timeout));
}

select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    return(_abi_select(nfds, readfds, writefds, exceptfds, timeout));
}

/***************************************************************************
 * sleep- set SUSPENDED so that full time expires.  Otherwise, will return
 * early if data becomes available.  Note: no HAS_EVENT to clear.
 */
unsigned
sleep(unsigned seconds)
{
    if (IN_SERVER_MODE())
	return(_sleep(seconds));

    SET_SUSPENDED(CurClient);
    CallDispatch(MAP(IC_FDS(CurClient)[0]), seconds, 0);
    CLR_SUSPENDED(CurClient);
    return(0);
}

/***************************************************************************
 * wait-
 */
static void (*prev_child_disp)(int);

static void
handler(int sig)
{
    int ic;

    for (ic = 0; ic < NumClients; ic++)
	if (IS_SUSPENDED(ic))
	{
	    SET_HAS_EVENT(ic);
	    break;
	}

    /* Restore sig disp for ALRM and CHLD in SERVER_MODE */
    TRANSITION_CALL(-1, (void)sigset(SIGCHLD, prev_child_disp));
}

static int
check_wait(int * stat_loc)
{
    pid_t pid;

    /* Begin critical region.  Set sig disp for SIGCLD in SERVER_MODE */
    prev_child_disp = sigset(SIGCLD, handler);
    return( ((pid = waitpid((pid_t)-1, stat_loc, WNOHANG)) == 0) ? -1 : pid );
}

pid_t
wait(int * stat_loc)
{
    if (IN_CLIENT_MODE())
    {
	pid_t pid;

	TRANSITION_CALL(-1, pid = check_wait(stat_loc));
	if (pid != -1)
	    return(pid);

	else if (!HAS_EVENT(CurClient))
	{
	    SET_SUSPENDED(CurClient);
	    CallDispatch(MAP(IC_FDS(CurClient)[0]), -1L, -1L);
	    CLR_SUSPENDED(CurClient);
	    CLR_HAS_EVENT(CurClient);
	}
    }
    return(_wait(stat_loc));
}

ssize_t
write(int fd, const void * buf, size_t nbyte)
{
    if (!IS_INTERNAL(fd))
	return(_write(fd, buf, nbyte));	/* "Normal" write if not i-c */

    ENTER_PSEUDO_SERVER_MODE();
    BI_DPRINT7(stderr, "Writing to internal fd: %d, nbytes: %d\n", fd, nbyte);

    /* Data from server to i-c's gets buffered */
    if (IS_SERVER_TO_CLIENT(fd))
    {
	if (END(fd) - WPTR(fd) < nbyte)
	{
	    u_int wdiff, rdiff, bufsize;

	    wdiff = WPTR(fd) - BUF(fd);
	    rdiff = RPTR(fd) - BUF(fd);
	    bufsize = max((END(fd) - BUF(fd)) * 2, nbyte + WPTR(fd) - BUF(fd));
	    BUF(fd) = (char *)Xrealloc(BUF(fd), bufsize);
	    WPTR(fd) = BUF(fd) + wdiff;
	    RPTR(fd) = BUF(fd) + rdiff;
	    END(fd) = BUF(fd) + bufsize;
	}
	(void)memcpy(WPTR(fd), buf, nbyte);	/* do the "write" */

	/* This is an OR op so redundant scheduling doesn't matter */
	if (CLIENT_INFO(fd) >= 0)
	    Schedule(CLIENT_INFO(fd));
#ifdef BI_DEBUG
	if (RPTR(fd) == WPTR(fd))
	    BI_DPRINT5(stderr, "Scheduling fd %d (%s)\n",
		       fd, ARGV0(CLIENT_INFO(fd)));
#endif
	WPTR(fd) += nbyte;
	LEAVE_PSEUDO_SERVER_MODE();
	return(nbyte);
    }

    /* else write to CLIENT_TO_SERVER fd */
    if ((MAP(fd) >= 0) && CLIENT(MAP(fd)))
    {
	/* Data from i-c's to server gets processed "in-line" */

	ClientPtr client = CLIENT(MAP(fd));

	/* (Taken from ReadRequestFromClient) */
	/* NOTE: should transition to server mode here (?) */

#define MAJOROP ((xReq *)client->requestBuffer)->reqType

	OsCommPtr oc = (OsCommPtr)client->osPrivate;
	register ConnectionInputPtr oci = oc->input;
	int fd = oc->fd;				/* SERVER_TO_CLIENT */
	register unsigned int gotnow;
	int result;
	char * oldbuf;

	if (!oci)
	{
	    oci = FreeInputs;
	    if (oci)
	    {
		FreeInputs = oci->next;
	    }
	    else if (!(oci = AllocateInputBuffer()))
	    {
		/* YieldControlDeath(); */
		LEAVE_PSEUDO_SERVER_MODE();
		return(-1);
	    }
	    oc->input = oci;
	}
	else
	{
	    if ((oci->bufptr < oci->buffer) ||
		(oci->bufptr > oci->buffer + oci->bufcnt))
		oci->bufptr = oci->buffer;
	    else
		oci->bufptr += oci->lenLastReq;
	    oci->lenLastReq = 0;
	}
	if (oci->bufptr == oci->buffer + oci->bufcnt) {
	    oldbuf = oci->buffer;
	    oci->bufptr = oci->buffer = (char *)buf;
	    oci->bufcnt = nbyte;
	}
	else {
	    (void)memcpy(oci->buffer + oci->bufcnt, buf, nbyte);
	    oci->bufcnt += nbyte;
	}
	do {
	    gotnow = oci->buffer + oci->bufcnt - oci->bufptr;
	    if ((gotnow < sizeof(xReq)) ||
		(gotnow < (((xReq *) oci->bufptr)->length << 2)))
	    {
		BI_DPRINT5(stderr, "ReadRequestFromClient Failed\n");
		if (oci->buffer == buf)
		    oci->buffer = oldbuf;
		(void)memmove(oci->buffer, oci->bufptr, gotnow);
		oci->bufcnt = gotnow;
		oci->bufptr = oci->buffer;
		break;
	    }
	    client->requestBuffer = (pointer) oci->bufptr;
	    BI_PROCESS7(MAJOROP);
	    client->sequence++;
	    oci->lenLastReq = ((xReq *)oci->bufptr)->length << 2;
	    result = client->requestVector[MAJOROP](client);

	    if (result != Success) {
		if (client->noClientException != Success)
		    CloseDownClient(client);
		else
		    SendErrorToClient(client, MAJOROP,
				      MinorOpcodeOfRequest(client),
				      client->errorValue, result);
	    }

	    /* Transition to UPPER_SERVER_MODE waiting for client to wake up
	     * if this request put client to sleep (eg, opening a font).
	     * 
	     * Don't need to Unschedule or TurnOffScheduling since we see
	     * client as still running so it won't be rescheduled.
	     */
	    if (ClientIsAsleep(client))
	    {
		SET_SUSPENDED(CLIENT_INFO(fd));
		CallDispatch(MAP(fd), -1L, -1L);
		CLR_SUSPENDED(CLIENT_INFO(fd));
		CLR_HAS_EVENT(CLIENT_INFO(fd));
	    }

	    oci->bufptr += oci->lenLastReq;
	    oci->lenLastReq = 0;
	} while (oci->bufptr < oci->buffer + oci->bufcnt);
	if (!(oci->bufptr < oci->buffer + oci->bufcnt)) {
	    if (oci->buffer == buf)
		oci->buffer = oldbuf;
	    oci->bufptr = oldbuf;
	    FD_CLR(fd, &ClientsWithInput);
	    oci->bufcnt = 0;
	}
	if (++OpCnt >= OpsBeforeDispatch)
	    CallDispatch(MAP(fd), 0L, 0L);	/* don't block */

	LEAVE_PSEUDO_SERVER_MODE();
	return(nbyte);
    }
    LEAVE_PSEUDO_SERVER_MODE();
    return(-1);
}

int
writev(int fd, const struct iovec *iov, int iovcnt)
{
    const struct iovec *vec;
    int			len, i;

    if (!IS_INTERNAL(fd))
	return(_writev(fd, iov, iovcnt));

    if ((iovcnt <= 0) || (iovcnt > 16))
    {
	fprintf(stderr, "writev: %s passed invalid iovcnt (%d)\n",
		ARGV0(CurClient), iovcnt);
	errno = EINVAL;
	return(-1);
    }
    len = 0;
    for (vec = iov; vec < iov + iovcnt; vec++)
    {
#ifdef BI_DEBUG
	if (vec->iov_len < 0)
	{
	    fprintf(stderr, "writev: %s passed negative iov_len\n",
		    ARGV0(CurClient));
	    errno = EINVAL;
	    return(-1);
	}
#endif
	if ( (i = write(fd, vec->iov_base, vec->iov_len)) < 0 ) {
	    return(-1);
	}
	len += i;
    }
    return(len);
}
