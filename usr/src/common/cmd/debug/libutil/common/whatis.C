#ident	"@(#)debugger:libutil/common/whatis.C	1.3"

#include "Parser.h"
#include "utility.h"
#include "Expr.h"
#include "Proglist.h"
#include "ProcObj.h"
#include "Proctypes.h"
#include "Interface.h"
#include "global.h"
#include <signal.h>

int
whatis(Proclist *procl, const char *exp) 
{
	plist		*list;
	ProcObj  	*pobj;
	int		ret = 1;
	
	if (procl)
	{
		list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		list = 0;
		pobj = proglist.current_object();
	}

	int multiple = list && list->p_pobj;

	sigrelse(SIGINT);
	do
	{
		if (prismember(&interrupt, SIGINT))
			break;

		if (multiple)
			printm(MSG_print_header, pobj->obj_name());
		Expr	expr((char *)exp, pobj, 0, 1);
		if (!expr.print_type(pobj))
			ret = 0;
	} while (multiple && ((pobj = list++->p_pobj) != 0));

	sighold(SIGINT);
	return ret;
}
