#ident	"@(#)debugger:libutil/common/cancel_sig.C	1.6"
#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Parser.h"
#include "Proctypes.h"
#include "Interface.h"
#include <signal.h>

int
cancel_sig(Proclist *procl, IntList *sigs)
{
	int	single = 1;
	int	ret = 1;
	ProcObj	*pobj;
	plist	*list;
	sig_ctl	set;

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
	if (!sigs || !sigs->first())
	{
		prfillset(&set.signals);
	}
	else
	{
		do {
			praddset(&set.signals, sigs->val());
		} while(sigs->next());
	}
	do
	{
		Process	*proc = pobj->process();
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE|E_OFF_LWP|E_SUSPENDED)
			|| !proc->stop_all() )
		{
			ret = 0;
			continue;
		}
		if (!pobj->cancel_sigs(&set))
			ret = 0;
		if (!proc->restart_all())
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	return ret;
}
