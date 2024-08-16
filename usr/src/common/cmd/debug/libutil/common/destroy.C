#ident	"@(#)debugger:libutil/common/destroy.C	1.4"

#include "utility.h"
#include "Procctl.h"
#include "Interface.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Process.h"
#include "Parser.h"
#include <errno.h>
#include <string.h>
#include <signal.h>

int
destroy_process(Process *proc, int announce)
{
	if ( proc == 0 )
	{
		return 0;
	}
	if (!proc->destroy(1))
		return 0;
	if (announce)
		printm(MSG_proc_killed, proc->obj_name());
	return 1;
}


int
destroy_process(Proclist *procl, int announce)
{
	int 	single = 1;
	int 	current_changed = 0;
	Process	*proc;
	ProcObj	*pobj;
	plist	*list;
	int	ret = 1;

	// we operate only on processes
	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl, 1);
		pobj = list++->p_pobj;
		if (pobj)
			proc = pobj->process();
		else 
			proc = 0;
	}
	else
	{
		proc = proglist.current_process();
	}
	if (!proc)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	do
	{
		if (proc == proglist.current_process())
			current_changed = 1;

		if (!destroy_process(proc, announce))
		{
			ret = 0;
			continue;
		}
	}
	while(!single && ((pobj = list++->p_pobj) != 0) &&
		((proc = pobj->process()) != 0));

	if (current_changed)
		proglist.reset_current(1);
	proglist.prune();
	return ret;
}

void
destroy_all()
{
	// we operate only on processes
	// excludes core files
	plist		*list = proglist.all_live(1);

	for (Process *p = (Process *)list++->p_pobj; p; 
		p = (Process *)list++->p_pobj)
	{
		if (p->is_child()) 
		{
			destroy_process(p, 0);
		} 
		else 
		{
			release_process(p, 1);
		}
	}
	proglist.prune();
}
