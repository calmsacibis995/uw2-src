#ident	"@(#)debugger:libutil/common/send_sig.C	1.4"
#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Procctl.h"

// release list of threads and processes - if -pprocid
// is given, we use process level routine instead of parsing
// list into individual threads (i.e. p1== p1, not p1.1,p1.2,p1.3)

int
send_signal( Proclist * procl, int signo )
{
	ProcObj	*pobj;
	plist	*list;
	int	ret = 1;
	int	single = 1;

	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl, 1);
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
		if (!pobj->state_check(E_CORE|E_DEAD|E_OFF_LWP|E_SUSPENDED))
		{
			ret = 0;
			continue;
		}
		if (!pobj->proc_ctl()->kill(signo))
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	return ret;
}
