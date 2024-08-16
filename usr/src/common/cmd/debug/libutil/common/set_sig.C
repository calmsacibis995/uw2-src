#ident	"@(#)debugger:libutil/common/set_sig.C	1.4"

#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Event.h"
#include <signal.h>


int
set_signal( Proclist * procl, sigset_t sigs, Node *cmd, int ignore, int quiet )
{
	ProcObj	*pobj;
	plist	*list;
	Sig_e	*eptr;
	int	eid;
	int	success = 0;
	int	ret = 1;

	if (procl)
	{
		list = proglist.proc_list(procl, 0);
	}
	else
	{
		list = proglist.proc_list(proglist.current_program(),0);
	}
	pobj = list->p_pobj;
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	if (cmd)
	{
		eid = m_event.new_id();
	}

	if (get_ui_type() == ui_gui)
		quiet = 0;

	for (; pobj; list++, pobj = list->p_pobj)
	{	
		Process	*proc = pobj->process();
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}
		if (!cmd)
		// no event - just set signals
		{
			if (ignore)
			{
				if (!pobj->ignore_sigs(&sigs, list->p_type))
					ret = 0;
			}
			else
			{
				if (!pobj->catch_sigs(&sigs, list->p_type))
					ret = 0;
			}
		}
		else
		{
			eptr = new Sig_e(sigs, eid, list->p_type, quiet, cmd, 
				pobj);
			if (eptr->get_state() == E_ENABLED)
			{
				m_event.add((Event *)eptr);
				pobj->add_event((Event *)eptr);
				success++;
			}
			else
			{
				ret = 0;
				delete(eptr);
			}
		}
		if (!proc->restart_all())
			ret = 0;
	}
	if (cmd)
	{
		if (success)
		{
			printm(MSG_event_assigned, eid);
			return ret;
		}
		else
		{
			m_event.dec_id();
			return 0;
		}
	}
	else
		return ret;
}

