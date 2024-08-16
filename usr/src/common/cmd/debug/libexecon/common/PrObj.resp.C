#ident	"@(#)debugger:libexecon/common/PrObj.resp.C	1.20"

#include "Event.h"
#include "ProcObj.h"
#include "Process.h"
#include "Thread.h"
#include "EventTable.h"
#include "Interface.h"
#include "Proctypes.h"
#include "Procctl.h"
#include "Instr.h"
#include "Seglist.h"
#include "Machine.h"
#include "List.h"
#include "global.h"
#include <sys/syscall.h>
#include <sys/types.h>


int
ProcObj::respond_to_sus(int show, int showsrc, Execstate showstate)
{
	check_watchpoints();
	find_cur_src();
	check_onstop();
	if (show)
		return show_current_location(showsrc, showstate);
	return 1;
}

// We could be stopped because of an event in place for this
// signal or just because the disposition for the debugger
// for this signal is "catch."
int
ProcObj::respond_to_sig()
{
	NotifyEvent	*ne;
	int		found = 0;
	int		show = 0;
	EventTable	*tab = events();

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::respond_to_sig", __LINE__);
		return 0;
	}

	ne = tab->siglist.events(latestsig); 
	if (ne)
	{
		for (; ne; ne = ne->next())
		{
			int	i;
			if (ne->object != this)
				continue;
			if ((i = (*ne->func)(ne->thisptr)) 
				== NO_TRIGGER)
				continue;
			found++;
			if (i == TRIGGER_VERBOSE)
				show = 1;
		}
	}
	else
	{
		// no events - check signal disposition
		if (prismember(&sigset, latestsig))
		{
			found++;
			show = 1;
		}
	}
	if (found)
	{
		return respond_to_sus(show, 1, state);
	}
	else
	{
		if (!check_watchpoints())
			return restart(follow_add_nostart);
		check_onstop();
	}
	return 1;
}

// Certain system calls require special treatment (fork, exec,
// lwp create).  We must still check for events for those calls,
// however.

int
ProcObj::respond_to_tsc()
{
	int		found = 0;
	NotifyEvent	*ne;
	int		show = 0;
	int		ret_val = 1;
	EventTable	*tab = events();

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::respond_to_tsc", __LINE__);
		return 0;
	}

	ne = tab->tsclist.events(latesttsc, (state == es_syscallxit)
		? TSC_exit : TSC_entry);
	if (state == es_syscallxit)
	{
		switch(latesttsc)
		{
		case SYS_fork:
		case SYS_vfork:
#ifdef SYS_fork1
		case SYS_fork1:
		case SYS_forkall:
#endif
			if (FORK_FAILED())
			{
				if (!ne)
					return(restart(follow_add_nostart));
			}
			else
			{
				pid_t npid = SYS_RETURN_VAL();
				if (flags & L_IGNORE_FORK)
				{
					// process forked but we are 
					// ignoring children
					ret_val = release_child(npid);
					if (!ne)
						return(restart(follow_add_nostart));
				}
				else
				{
					// control new process in debugger
					ret_val = process()->control_new_proc(npid, this, latesttsc);
					if (!ne)
					{
						if (ret_val)
						{
							check_watchpoints();
							find_cur_src();
						}
						return ret_val;
					}
				}
			}
			break;
		case SYS_exec:
		case SYS_execve:
			if (EXEC_FAILED())
			{
				if (!ne)
					return(restart(follow_add_nostart));
			}
			else
			{
				ret_val = process()->control_new_prog();
				if (!ne)
					return ret_val;
			}
			break;
#ifdef DEBUG_THREADS
		case SYS_lwpcreate:
			if (!LWP_CREATE_FAILED() && (flags & L_THREAD))

			{
				lwpid_t	id;
				id = SYS_RETURN_VAL();
				ret_val = process()->control_new_lwp(id);
			}
			if (!ne)
				return(restart(follow_add_nostart));
			break;
#endif
		default:
			break;
		}
	}
	for (; ne; ne = ne->next())
	{
		int	i;

		if (ne->object != this)
			continue;
		if ((i = (*ne->func)(ne->thisptr)) == NO_TRIGGER)
			continue;
		found++;
		if (i == TRIGGER_VERBOSE)
			show = 1;
	}
	if (found)
	{
		if (!respond_to_sus(show, 1, state))
			return 0;
	}
	else
	{
		if (!check_watchpoints())
		{
			if (!restart(follow_add_nostart))
				return 0;
		}
		else
			check_onstop();
	}
	return ret_val;
}

int
ProcObj::respond_to_bkpt()
{
	NotifyEvent	*ne, *ne2;
	int		found = 0;
	int		show = 0;

	ne = latestbkpt->events(); 
	// must be careful since event code may delete
	// from middle of list
	while(ne)
	{
		int	i;

		ne2 = ne->next();

		if (ne->object != this || !ne->func)
		// special breakpoints like destpt and hoppt
		// don't have notifier functions
		{
			ne = ne2;
			continue;
		}
		i = (*ne->func)(ne->thisptr);
		switch(i)
		{
		case NO_TRIGGER:
			break;
		case TRIGGER_VERBOSE:
			show = 1;
			/* FALLTHROUGH */
		case TRIGGER_QUIET:
			found++;
			break;
		case TRIGGER_FOREIGN:
			// event triggered but context for
			// event was not this ProcObj; we keep
			// going
			if (flags & L_WAITING)
			{
				flags &= ~L_WAITING;
				waitlist.remove(this);
			}
			break;
		}
		ne = ne2;
	}
	if (found)
	{
		retaddr = 0;	// we can reset retaddr on next 
				// start - if we are ignoring
				// this bkpt, we don't want
				// to reset in case we were
				// stepping over this func
		find_cur_src();
		check_onstop();
		if (show)
			show_current_location( 1 );
		check_watchpoints();
		return 1;
	}
	return 0;
}

// Stop due to event triggered by a foreign process
int
ProcObj::stop_for_event(int mode)
{
	if (!stop())
		return 0;
	
	state = es_watchpoint;
	find_cur_src();

	check_onstop();
	if (mode == TRIGGER_VERBOSE)
		show_current_location( 1 );
	check_watchpoints();
	return 1;
}

int
ProcObj::check_watchpoints()
{
	NotifyEvent	*ne;
	int		show = 0;
	int		found = 0;
	EventTable	*tab = events();

	if (tab == 0)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::check_watchpoints",
			__LINE__);
		return 0;
	}

	ne = tab->watchlist;
	for (; ne; ne = ne->next())
	{
		int	i;

		if (ne->object != this)
			continue;
		i = (*ne->func)(ne->thisptr);
		switch(i)
		{
			default:
				break;
			case NO_TRIGGER:
				break;
			case TRIGGER_VERBOSE:
				show = 1;
				/* FALLTHROUGH */
			case TRIGGER_QUIET:
				found++;
				break;
			case TRIGGER_FOREIGN:
				if (flags & L_WAITING)
				{
					flags &= ~L_WAITING;
					waitlist.remove(this);
				}
				break;
		}
	}
	if (found)
	{

		find_cur_src();
		if (show)
			show_current_location( 1, es_watchpoint );
		return 1;
	}
	return 0;
}

// For each call to _rtld, we hit the dynpt breakpoint
// twice: the first time the library map is marked as inconsistent.
// We wait for the second time to add change our internal library map.
int
ProcObj::respond_to_dynpt(follower_mode mode)
{
	pc = instr.adjust_pc();
	if ( seglist->buildable( pctl ) )
	{
		seglist->build( pctl, ename );
	}
	return restart(mode);
}
