#ident	"@(#)debugger:libutil/common/set_onstop.C	1.3"

#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Process.h"
#include "Interface.h"
#include "Parser.h"
#include "Event.h"


int
set_onstop( Proclist * procl, Node *cmd )
{
	ProcObj		*pobj;
	plist		*list;
	Onstop_e	*eptr;
	int		eid;
	int		success = 0;
	int		ret = 1;

	if (procl)
	{
		list = proglist.proc_list(procl);
	}
	else
	{
		list = proglist.proc_list(proglist.current_program());
	}
	pobj = list->p_pobj;
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	eid = m_event.new_id();
	for (; pobj; list++, pobj = list->p_pobj)
	{	
		Process	*proc = pobj->process();
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}
		eptr = new Onstop_e(eid, list->p_type, cmd, pobj);
		if (eptr->get_state() == E_ENABLED)
		{
			pobj->add_event((Event *)eptr);
			m_event.add((Event *)eptr);
			success++;
		}
		else
		{
			ret = 0;
			delete eptr;
		}
		if (!proc->restart_all())
			ret = 0;
	}
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
