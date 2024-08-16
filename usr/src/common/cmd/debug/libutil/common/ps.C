#ident	"@(#)debugger:libutil/common/ps.C	1.12"

#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Program.h"
#include "global.h"
#include "Interface.h"
#include "Procctl.h"
#include "Proctypes.h"
#include "Thread.h"
#include "Parser.h"
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>

#define NAME_LEN	10	// length of program name for output

static void
print_status(ProcObj *pobj)
{
	const char	*pname;
	char		*func_name = 0;
	char		*file = 0;
	long		line = 0;
	Execstate	state;
	const char	*current = " ";	// assume non-current
	char		location[16];
	Msg_id		msg;
	int		stopped = 0;

	// add '*' to denote current object
	if (pobj == proglist.current_object())
		current = "*";

	pname = pobj->prog_name();
	state = pobj->get_state();
#ifdef DEBUG_THREADS
	if (pobj->is_suspended())
	{
		stopped = 1;
		if (state == es_corefile || state == es_core_off_lwp)
			msg = MSG_threads_ps_core_suspend;
		else
			msg = MSG_threads_ps_suspend;

	}
	else
#endif
	{
		switch(state) 
		{
		case es_corefile:
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_core;
	#else
			msg = MSG_ps_core;
	#endif
			stopped = 1;
			break;
		case es_halted:
		case es_stepped:
		case es_breakpoint:
		case es_signalled:
		case es_syscallent:
		case es_syscallxit:
		case es_watchpoint:
		case es_procstop:
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_stopped;
	#else
			msg = MSG_ps_stopped;
	#endif
			stopped = 1;
			break;
	#ifdef DEBUG_THREADS
		case es_core_off_lwp:
			msg = MSG_threads_ps_core_off_lwp;
			stopped = 1;
			break;
		case es_off_lwp:
			msg = MSG_threads_ps_off_lwp;
			stopped = 1;
			break;
	#endif
		case es_running:
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_running;
	#else
			msg = MSG_ps_running;
	#endif
			break;
		case es_stepping:
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_stepping;
	#else
			msg = MSG_ps_stepping;
	#endif
			break;
		default: 
	#ifdef DEBUG_THREADS
			msg = MSG_threads_ps_unknown;
	#else
			msg = MSG_ps_unknown;
	#endif
			break;
		}
	}
#ifdef DEBUG_THREADS
	thread_t	tid;
	if (pobj->obj_type() == pobj_thread)
		tid = ((Thread *)pobj)->thread_id();
	else
		tid = (thread_t)-1;
#endif
	if (stopped)
	{
		current_loc(pobj, 0, file, func_name, line);
		if (!func_name)
			func_name = "";

		if (line && file)
		{
			// use basename
			char	*fptr = strrchr(file, '/');
			if (fptr)
				file = fptr + 1;
			sprintf(location, "%.10s@%d", file, line);
		}
		else
		{
			sprintf(location, "%#x", pobj->pc_value());
		}
#ifdef DEBUG_THREADS
		printm(msg, current, pname, pobj->obj_name(),
			pobj->pid(), (Iaddr)tid, func_name, location, 
			pobj->program()->command());
#else
		printm(msg, current, pname, pobj->obj_name(),
			pobj->pid(), func_name, location, 
			pobj->program()->command());
#endif
	}
	else
#ifdef DEBUG_THREADS
		printm(msg, current, pname, pobj->obj_name(),
			pobj->pid(), (Iaddr)tid,
			pobj->program()->command());
#else
		printm(msg, current, pname, pobj->obj_name(),
			pobj->pid(), pobj->program()->command());
#endif
}

int 
proc_status(Proclist *procl)
{
	ProcObj	*pobj;
	plist	*list;

	if (procl)
	{
		list = proglist.proc_list(procl);
	}
	else
	{
		list = proglist.all_procs();
	}
	pobj = list++->p_pobj;
	if (!pobj)
	{
		printe(ERR_no_proc, E_WARNING);
		return 0;
	}
	sigrelse(SIGINT);
#ifdef DEBUG_THREADS
	printm(MSG_threads_ps_header);
#else
	printm(MSG_ps_header);
#endif

	for(; pobj;  pobj = list++->p_pobj)
	{
		if (prismember(&interrupt, SIGINT))
			break;
		print_status(pobj);
	}
	sighold(SIGINT);
	return 1;
}
