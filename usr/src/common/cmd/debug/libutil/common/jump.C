#ident	"@(#)debugger:libutil/common/jump.C	1.6"

#include "utility.h"
#include "Iaddr.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "Frame.h"
#include "Symbol.h"
#include "Tag.h"
#include "global.h"

// change program counter 

int
jump(Proclist *procl, Location *location)
{
	Iaddr	addr;
	ProcObj	*pobj;
	plist	*list;
	Symbol	func;
	int	ret = 1;
	int	single = 1;

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
		if (!pobj->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}

		if ( get_addr( pobj, location, addr, st_func, func ) == 0 )
		{
			ret = 0;
			continue;
		}

		func = pobj->find_entry(addr);

		while(!func.isnull() && (!IS_ENTRY(func.tag())))
			func = func.parent();
		if (!func.isnull())
		{
			Iaddr	lo, hi, pc;
			if (func.tag() == t_label)
				func = func.parent();
			pc = pobj->pc_value();
			lo = func.pc(an_lopc);
			hi = func.pc(an_hipc);
	
			if (pc < lo || pc >= hi)
				printe(ERR_non_local_jump, E_WARNING,
					pobj->obj_name());
		}
		if (!pobj->set_pc(addr))
		{
			ret = 0;
			continue;
		}
		if (get_ui_type() == ui_gui)
		{
			char	*filename;
			char	*funcname;
			long	line;

			if (!current_loc(pobj, pobj->topframe(), filename, 
				funcname, line) || !filename)
			{
				printm(MSG_jump, (unsigned long)pobj,
					addr, "", 0);
			}
			else 
				printm(MSG_jump, (unsigned long)pobj,
					addr, filename, line);
		}
		else
		{
			pobj->show_current_location(1, es_none, vmode);
		}
	}
	while(!single && ((pobj = list++->p_pobj) != 0));

	return ret;
}
