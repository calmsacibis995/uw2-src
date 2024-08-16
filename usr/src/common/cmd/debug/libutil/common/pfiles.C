#ident	"@(#)debugger:libutil/common/pfiles.C	1.5"
#include "utility.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "global.h"
#include "Symbol.h"
#include "Source.h"

int
print_files( Proclist * procl )
{
	int	single = 1;
	ProcObj	*pobj;
	plist	*list;
	int	ret = 1;
	Source	source;

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
		Symbol	file = pobj->first_file();
		for(; !file.isnull(); file = pobj->next_file())
		{
			if (!file.isSourceFile())
			{
				continue;
			}
			if (file.source(source))
				printm(MSG_source_file, file.name());
		}
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	return ret;
}
