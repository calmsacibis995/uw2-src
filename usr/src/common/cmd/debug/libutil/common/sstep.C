#ident	"@(#)debugger:libutil/common/sstep.C	1.3"

#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "global.h"
#include "List.h"

int
single_step( Proclist * procl, int clearsig, int cnt, int talk, 
	int level, int where, int wait )
{
	ProcObj	*pobj;
	plist	*list;
	int	single = 1;
	int	ret = 1;

	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		pobj = proglist.current_object();
	}
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	do
	{
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE|E_OFF_LWP|E_SUSPENDED))
		{
			ret = 0;
			continue;
		}
		if (level == STEP_STMT)
		{
			if (!pobj->stmt_step( where, clearsig, 
				0, cnt, talk ))
			{
				ret = 0;
				continue;
			}
		}
		else
		{
			if (!pobj->instr_step( where, clearsig, 
				cnt, talk ))
			{
				ret = 0;
				continue;
			}
		}
		if (wait == WAIT)
		{
			pobj->set_wait();
			waitlist.add(pobj);
		}
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	if ((wait == WAIT) && !waitlist.isempty())
		wait_process();
	return ret;
}
