/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef TSClist_h
#define TSClist_h
#ident	"@(#)debugger:inc/common/TSClist.h	1.4"

#include "Ev_Notify.h"
#include "Proctypes.h"
#include <sys/syscall.h>

// special system call exits - always trapped by debug
#ifdef DEBUG_THREADS
#define	SPECIAL_EXIT(I) 	(((I) == SYS_fork) || \
				    ((I) == SYS_vfork) ||\
				    ((I) == SYS_exec) ||\
				    ((I) == SYS_execve) ||\
				    ((I) == SYS_lwpcreate))
#else
#define	SPECIAL_EXIT(I) 	(((I) == SYS_fork) || \
				    ((I) == SYS_vfork) ||\
				    ((I) == SYS_exec) ||\
				    ((I) == SYS_execve))
#endif

class	ProcObj;
struct	TSCevent;

// modes = entry or exit or both
#define TSC_entry	1
#define TSC_exit	2


class TSClist {
	sys_ctl		entrymask;
	sys_ctl		exitmask;
	TSCevent	*_events;
	TSCevent	*lookup(int);
	TSCevent	*find(int);
public:
			TSClist();
			~TSClist();
	int		add(int sys, int mode, Notifier,
				void *, ProcObj *);
	int		remove(int sys, int mode, Notifier, 
				void *, ProcObj *);
	sys_ctl		*tracemask(int mode = TSC_entry);
	NotifyEvent 	*events(int sys, int mode = TSC_entry);
};

enum Systype {
	NoSType = 0,
	Entry,
	Exit,
	Entry_exit
};

struct syscalls
{
	const char	*name;
	int		entry;
};

extern syscalls systable[];

#endif
// end of TSClist.h
