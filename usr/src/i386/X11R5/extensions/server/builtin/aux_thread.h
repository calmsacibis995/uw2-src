/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)builtinext:server/builtin/aux_thread.h	1.3"
#ifndef AUX_THREAD_H_
#define AUX_THREAD_H_

#define	AUX_THREAD_SIG	SIGCONT

extern thread_t	AuxThreadId;
extern int	AuxThreadCanRun;
extern int	AuxThreadHasStarted;
extern int	ThrDebug;

extern void AuxThreadInit(void (*dispatch_func)(void *), void * data);

#endif /* AUX_THREAD_H_ */
