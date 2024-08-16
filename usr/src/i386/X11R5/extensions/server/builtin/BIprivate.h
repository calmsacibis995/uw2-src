/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)builtinext:server/builtin/BIprivate.h	1.11"

#ifndef BIPRIVATE_H
#define BIPRIVATE_H

#include <stdio.h>		/* always needed for DPRINT macros */
#include <sys/time.h>

#include "BIserver.h"

/* Info about each connection (may be multiple connections per "client").
 * Fields with # indicate their use for server-to-client fd's only.
 */
struct _BIFdInfo {
    int		other_fd;	/* fd on other side of connection	*/
    void *	ptr;		/* # ClientPtr				*/
    char *	buf;		/* # buffer writes to internal client	*/
    char *	rptr;		/* # to read out of buf			*/
    char *	wptr;		/* # to write into buf			*/
    char *	end;		/* # end of buf				*/
    char	client_info;	/* # _BIClientInfo index		*/
    u_char	flags;		/* Flags:				*/
#define CLIENT_TO_SERVER		( 1 << 0 )
#define FORCE_CLOSE			( 1 << 1 )
};

#define IFD(FD)			BIGlobal.fds[FD]
#define CLIENT_INFO(S_C_FD)	IFD(S_C_FD)->client_info
#define MAP(FD)			IFD(FD)->other_fd
#define DATA_AVAIL(S_C_FD)	( IFD(S_C_FD)->rptr != IFD(S_C_FD)->wptr )
#define CUR_FD_READY()		( (MAP(BIGlobal.cur_c_s_fd) < 0) || \
    (IS_SUSPENDED(CLIENT_INFO(MAP(BIGlobal.cur_c_s_fd))) && \
     HAS_EVENT(CLIENT_INFO(MAP(BIGlobal.cur_c_s_fd))))  || \
    (!IS_SUSPENDED(CLIENT_INFO(MAP(BIGlobal.cur_c_s_fd))) && \
     DATA_AVAIL(MAP(BIGlobal.cur_c_s_fd))) )


/* Info about each builtin client */

struct _BIClientInfo {
    char *		str_buf;		/* argv & env strings */
    char **		argv;
    char **		envp;			/* environ ptr */
    struct _XtAppStruct*app;			/* XtAppContext */
    struct _Client *	surrogate_client;
    void		(*(*atexit_funcs))(void);
    int			(*XErrorFunc)();
    int			(*XIOErrorFunc)();
    void *		dl_handle;
    void *		display;
    void		(*MainLoop)(void *, int);
    void		(*CleanUp)(void *);
    int *		ic_s_c_fds;
    char		num_ic_fds;
    char		atexit_func_cnt;
    char		sched_mask;
    unsigned char	flags;
#define EXITING_B		( 1 << 0 )
#define FORCE_EXIT_B		( 1 << 1 )
#define SUSPENDED_B		( 1 << 2 )	/* for "event" or timeout */
#define HAS_EVENT_B		( 1 << 3 )
};

#define ICLIENT(N)		BIGlobal.clients[N]
#define STR_BUF(N)		ICLIENT(N).str_buf
#define ARGV(N)			ICLIENT(N).argv
#define ARGV0(N)		ARGV(N)[0]
#define SURROGATE_CLIENT(N)	ICLIENT(N).surrogate_client
#define XERROR_FUNC(N)		ICLIENT(N).XErrorFunc
#define XIOERROR_FUNC(N)	ICLIENT(N).XIOErrorFunc
#define DL_HANDLE(N)		ICLIENT(N).dl_handle
#define DISPLAY(N)		ICLIENT(N).display
#define SCHED_MASK(N)		ICLIENT(N).sched_mask
#define IC_FLAGS(N)		ICLIENT(N).flags
#define IS_EXITING(N)		( IC_FLAGS(N) & EXITING_B )
#define FORCE_EXIT_IS_SET(N)	( IC_FLAGS(N) & FORCE_EXIT_B )
#define IS_SUSPENDED(N)		( IC_FLAGS(N) & SUSPENDED_B )
#define HAS_EVENT(N)		( IC_FLAGS(N) & HAS_EVENT_B )


#define ARE_READY_BUILTINS()	BIGlobal.readymask
#define IN_UPPER_SERVER_MODE()	!IN_NORMAL_SERVER_MODE()
#define NumClients		ARE_BUILTINS()
#define CurClient		BIGlobal.cur_client
#define IN_CLIENT_MODE()	( CurClient >= 0 )
#define IN_SERVER_MODE()	!IN_CLIENT_MODE()
#define CLIENT_FD(CLIENT)	( ((OsCommPtr)CLIENT->osPrivate)->fd )
#define MAX_ICLIENTS		2

#define ClientMask(N)		( 1 << N )
#define Unschedule(N)		BIGlobal.readymask &= ~ClientMask(N)
#define TurnOffScheduling(N)	SCHED_MASK(N) = 0
#define TurnOnScheduling(N)	SCHED_MASK(N) = ClientMask(N)

/* These macros are used to make the transition between client and server
 * modes.  TRANSITION_CALL is used to transition to server mode (ie, calling
 * Dispatch to enter UPPER_SERVER_MODE).  CALL_CLIENT_CODE is used to
 * transition to client mode (ie, calling client's main, work proc or
 * MainLoop).
 */
#define TRANSITION_CALL(ID, FUNC)					\
				do {					\
				    int save	= CurClient;		\
				    CurClient	= ID;			\
				    FUNC;				\
				    CurClient	= save;			\
				} while(0)

#define CALL_CLIENT_CODE(ID, FUNC)					\
				do {					\
				    int save	= CurClient;		\
				    CurClient	= ID;			\
				    Unschedule(ID);			\
				    TurnOffScheduling(ID);		\
				    LEAVE_SERVER_MODE();		\
				    FUNC;				\
				    ENTER_SERVER_MODE();		\
				    CurClient	= save;			\
				    TurnOnScheduling(ID);		\
				} while(0)

#define BIRemoveHandlers() \
    RemoveBlockAndWakeupHandlers(BIBlockHandler, BIWakeupHandler, NULL)

extern int	BuiltinProcRunClient(struct _Client * client);
extern void	BuiltinSendExecEvent(struct _Client * client);
extern void	BuiltinSendExitEvent(struct _Client * client, int exit_code);
extern void	BIBlockHandler(void *, void *, void *);
extern int	BICallTerminatingSigHandler(int ic);
extern void	BISigICExiting(int ic);
extern void	BIWakeupHandler();
extern void	(*libc_dlsym(char * name))();

#define NO_THREAD
#ifndef NO_THREAD
#define ENTER_PSEUDO_SERVER_MODE() enter_pseudo_server_mode(__FILE__, __LINE__);
#define LEAVE_PSEUDO_SERVER_MODE() leave_pseudo_server_mode(__FILE__, __LINE__);
#define ENTER_SERVER_MODE() enter_server_mode(__FILE__, __LINE__);
#define LEAVE_SERVER_MODE() leave_server_mode(__FILE__, __LINE__);
#else
#define AUX_PROC()
#define ENTER_PSEUDO_SERVER_MODE()
#define LEAVE_PSEUDO_SERVER_MODE()
#define ENTER_SERVER_MODE()
#define LEAVE_SERVER_MODE()
#endif /* ifndef NO_THREAD */

#ifdef BI_DEBUG
extern int BIDebug;
#endif
#if BI_DEBUG >= 1
#define _BI_DPRINT(n) (BIDebug < n) ? 1 : fprintf
#else
#define _BI_DPRINT(n) (1) ? 1 : fprintf
#endif
#define BI_DPRINT1 _BI_DPRINT(1)
#define BI_DPRINT3 _BI_DPRINT(3)
#define BI_DPRINT4 _BI_DPRINT(4)
#define BI_DPRINT5 _BI_DPRINT(5)
#define BI_DPRINT7 _BI_DPRINT(7)

#if BI_DEBUG >= 7
#define BI_PROCESS7(OPCODE) if (BIDebug >= 7 ) { \
	char oPnUM[32], oPnAME[1024]; \
	sprintf(oPnUM, "%d", OPCODE); \
	XGetErrorDatabaseText(NULL, "XRequest", oPnUM, "", oPnAME, 1024); \
	fprintf(stderr, "Processing message type %s (%s)\n", oPnUM, oPnAME); \
	} else
#else
#define BI_PROCESS7(OPCODE)
#endif /* BI_DEBUG >= 7 */

#endif /* BIPRIVATE_H */
