#ident	"@(#)debugger:libutil/common/change.C	1.9"

// Change event
// Delete old event and create a new one with the same event id.

#include "Interface.h"
#include "Proglist.h"
#include "Event.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Parser.h"
#include "TSClist.h"
#include <signal.h>

static int create_new(int, plist *, char, Systype, Node *, int, int, int,
	StopEvent *, sigset_t *, IntList *);

int
change_event( int event, Proclist *procl, int count, Systype systype,
	int set_quiet, void *event_expr, Node *cmd)
{
	Event		*eptr;
	int		found = 0;
	int		first = 1;
	plist		*list, *old_list;
	int		ret = 1;

	char		old_op;
	int		old_count;
	Systype		old_stype;
	Node		*old_cmd;
	int		old_quiet = 0;
	StopEvent	*old_stop;
	sigset_t	old_sigs;
	IntList		*old_sys;
	int		old_state;


	// do we need to delete old event?
	int delete_old = (procl || (systype != NoSType) || event_expr);

	// Go through all events with this id; if
	// we are deleting,  we must save the old information on
	// the event.  If we are not deleting,
	// change part of event that was specified.

	// proglist.make_list and add_list provide an external way to
	// create a list of ProcObj's.  Every make_list should be balanced
	// with a list_done
	if (!procl)
		old_list = proglist.make_list();
	eptr = m_event.events();
	if (!eptr)
	{
		printe(ERR_no_event_id, E_ERROR, event);
		if (!procl)
			proglist.list_done();
		return 0;
	}

	if (get_ui_type() == ui_gui)
		set_quiet = -1;

	for(; eptr; eptr = eptr->next())
	{
		if (eptr->get_id() != event ||
			eptr->get_state() == E_DELETED)
			continue;
		found++;
		old_op = eptr->get_type();
		if (first)
		{
			// check for syntax - must do here
			// since it's first time we
			// know type of event
			if (old_op == E_ONSTOP ||
				old_op == E_SIGNAL)
			{
				if (count >= 0)
				{
					printe(ERR_opt_change,
						E_ERROR, "-c");
					return 0;
				}
				if ((set_quiet >= 0) &&
					(old_op == E_ONSTOP))
				{
					printe(ERR_opt_change,
					E_ERROR,
					(set_quiet ? "-q" : "-v"));
					return 0;
				}
			}
			if ((systype != NoSType) &&
				(old_op != E_SCALL))
			{
				printe(ERR_opt_change, E_ERROR,
					"-ex");
				return 0;
			}
		}
		if (!eptr->get_obj())
		{
			printe(ERR_cannot_change, E_ERROR, event);
			if (!procl)
				proglist.list_done();
			return 0;
		}
		if (!delete_old)
		{
			Process	*proc = eptr->get_obj()->process();
			if (!eptr->get_obj()->state_check(E_RUNNING))
			{
				return 0;
			}
			if (!proc->stop_all())
				return 0;
			switch(old_op)
			{
			case E_ONSTOP:
			case E_SIGNAL:
				break;
			case E_SCALL:
				if (count >= 0)
				{
					((Sys_e *)eptr)->set_count(count);
					((Sys_e *)eptr)->reset_count();
				}
				break;
			case E_STOP:
				if (count >= 0)
				{
					((Stop_e *)eptr)->set_count(count);
					((Stop_e *)eptr)->reset_count();
				}
				break;
			}
			if (set_quiet == 1)
				eptr->set_quiet();
			else if (set_quiet == 0)
				eptr->set_verbose();
			if (cmd)
				eptr->set_cmd(cmd);
			if (!proc->restart_all())
				return 0;
		}
		else
		{
			if (!procl)
			{
				proglist.add_list(eptr->get_obj(), 
					eptr->get_level());
			}
			if (first)
			{
				first = 0;	
				old_cmd = eptr->get_cmd();
				old_state = eptr->get_state();
				switch(old_op)
				{
				case E_ONSTOP:	
					break;
				case E_STOP:
					old_count = ((Stop_e *)eptr)->get_count();
					old_stop = ((Stop_e *)eptr)->get_stop();
					break;
				case E_SIGNAL:
					old_sigs = *((Sig_e *)eptr)->get_sigs();
					break;
				case E_SCALL:
					old_stype = ((Sys_e *)eptr)->get_stype();
					old_sys = ((Sys_e *)eptr)->get_calls();
					old_count = ((Sys_e *)eptr)->get_count();
					break;
				}
			}
		}
	}
	if (!found)
	{
		if (!procl)
			proglist.list_done();
		printe(ERR_no_event_id, E_ERROR, event);
		return 0;
	}
	if (!delete_old)
	{
		if (get_ui_type() == ui_gui)
			printm(MSG_event_changed, event);
		return 1;
	}
	if (count >= 0)
		old_count = count;
	if (set_quiet != -1)
		old_quiet = set_quiet;
	if (cmd)
		old_cmd = cmd;
	if (systype != NoSType)
		old_stype = systype;
	if (event_expr)
	{
		IntList	*mask;
		switch(old_op)
		{
		default:
			break;
		case E_STOP:
			old_stop = (StopEvent *)event_expr;
			break;
		case E_SIGNAL:
		{
			mask = (IntList *)event_expr;

			premptyset(&old_sigs);
			if (mask->first())
			{
				do {
					praddset(&old_sigs, mask->val());
				} while(mask->next());
			}
			break;
		}
		case E_SCALL:
			old_sys = (IntList *)event_expr;
			break;
		}
	}
	else 
	{
		if (old_op == E_SCALL)
		{
			// copy IntList
			IntList	*tmp = old_sys;
			old_sys = new IntList;
			tmp->first();
			do
			{
				old_sys->add(tmp->val());
			} while(tmp->next());
		}
	}

	if (procl)
	{
		list = proglist.proc_list(procl);
	}
	else
	{
		list = old_list;
	}

	ret = create_new(event, list, old_op, old_stype, old_cmd, old_count, 
		old_quiet, old_state, old_stop, &old_sigs, old_sys);
	if (!procl)
		proglist.list_done();
	return ret;
}

static int 
create_new(int event, plist *list, char old_op, Systype old_stype,
	Node *old_cmd, int old_count, int old_quiet, int old_state,
	StopEvent *old_stop, sigset_t *old_sigs, IntList *old_sys)
{
	ProcObj	*pobj = list->p_pobj;
	int	ret = 1;
	int	new_event = 0;
	int	old_level;
	Event	*eptr;

	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}

	// create new event, with bogus number for now
	for (; pobj; list++, pobj = list->p_pobj)
	{	
		Process	*proc = pobj->process();
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}

		old_level = list->p_type;
		switch(old_op)
		{
		default:	break;
		case E_ONSTOP:
			eptr = new Onstop_e(-1, old_level, old_cmd, 
				pobj);
			break;
		case E_STOP:
			eptr = (Event *)new Stop_e(copy_tree(old_stop), 
				-1, old_level, old_quiet, old_count, 
				old_cmd, pobj);
			break;
		case E_SIGNAL:
			eptr = (Event *)new Sig_e(*old_sigs, -1, 
				old_level, old_quiet, old_cmd, pobj);
			break;
		case E_SCALL:
			eptr = (Event *)new Sys_e(old_sys, old_stype,
				-1, old_level, old_quiet, old_count,
				old_cmd, pobj);
			break;
		}
		if (eptr->get_state() != E_DELETED)
		{
			pobj->add_event(eptr);
			m_event.add(eptr);
			new_event++;
		}
		else
		{
			delete eptr;
			ret = 0;
		}
		if (!proc->restart_all())
			ret = 0;
	}

	if (ret)
	{
		// everything's ok, so now delete old event
		// and reset new event's number
		if (!m_event.event_op(event, M_Delete))
		{
			m_event.event_op(-1, M_Delete);
			return 0;
		}

		for(eptr = m_event.events(); eptr; 
			eptr = eptr->next())
		{
			if (eptr->get_id() == -1)
			{
				eptr->set_id(event);
				if (old_state == E_DISABLED ||
					old_state == E_DISABLED_INV)
					eptr->disable();
			}
		}

		if (get_ui_type() == ui_gui)
			printm(MSG_event_changed, event);
	}
	else if (new_event)
		m_event.event_op(-1, M_Delete);
	return ret;
}
