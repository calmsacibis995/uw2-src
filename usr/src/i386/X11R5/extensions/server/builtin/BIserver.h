/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)builtinext:server/builtin/BIserver.h	1.10"

#ifndef BISERVER_H
#define BISERVER_H

#include <sys/types.h>

struct _BIGlobal {			/* + = global but private to builtin */
    struct _BIFdInfo ** fds;		/* +builtin connection fd info */
    struct _BIClientInfo * clients;	/* +builtin client info */
    ulong	poll_addr;		/*  the real addr of poll() */
    ulong	last_poll_addr;		/*  captured in catch_xsig */
    long	dispatch_tv_sec;	/*  dispatch time (secs) */ 
    long	dispatch_tv_usec;	/*  dispatch time (micro secs) */ 
    int		dummy_pipe;		/*  to avoid longjmp in catch_xsig */
    int		cur_c_s_fd;		/*  ic fd while in UPPER_SERVER_MODE */
    char	cur_client;		/* +current client executing */
    u_char	num_clients;		/*  number of clients built-in */
    u_char	readymask;		/* +mask of ready clients */
    u_char	num_force_exit;		/* +when surrogate goes away */
    u_char	num_force_close;	/* +when CLIENT_TO_SERVER closes */
};
extern struct _BIGlobal BIGlobal;
extern int		BuiltinDispatch(void);

#define ARE_BUILTINS()		BIGlobal.num_clients
#define IN_NORMAL_SERVER_MODE()	( BIGlobal.cur_c_s_fd < 0 )

#endif /* BISERVER_H */
