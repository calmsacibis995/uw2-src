#ident	"@(#)debugger:libutil/common/functions.C	1.7"

// print out list of functions 
// only used by the gui
// 
// by default, prints all functions, global and local, for 
// all files in the current object (a.out, libc.so.1, etc.)
//
// if an object is given, it looks there instead
// if a filename is given, only the functions defined in that
// file are listed (local or global)
// if source_only is non-zero, only functions from files having
// statement information are listed

#include "utility.h"
#include "global.h"
#include "Symbol.h"
#include "Source.h"
#include "Symtab.h"
#include "ProcObj.h"
#include "Proctypes.h"
#include "Frame.h"
#include "Proglist.h"
#include "Interface.h"
#include "Tag.h"
#include <signal.h>
#include <string.h>
#include <stdlib.h>

class FuncList
{
	char		**list;
	int		count;
	int		size;
public:
			FuncList();
			~FuncList();
	void		add(const char * name, const char *location);
	void		printall();
	void		clear();
};

#define LISTENT	200

FuncList::FuncList()
{
	// initialize list with default number of entries
	// we never shrink list
	if ((list = (char **)calloc(LISTENT, 
		sizeof(char *))) == 0)
	{
		new_handler();
		return;
	}
	size = LISTENT;
	count = 0;
}

FuncList::~FuncList()
{
	clear();
	free(list);
}

void
FuncList::clear()
{
	register char	**lptr = list;
	while(*lptr)
	{
		delete(*lptr);
		*lptr = 0;
		lptr++;
	}
	count = 0;
}

void
FuncList::add(const char *name, const char *loc)
{
	char	*sym;
	int	len = strlen(name) + 1;

	if (loc)
		len += strlen(loc) + 1;
	sym = new char[len];
	if (loc)
		sprintf(sym, "%s@%s", loc, name);
	else
		strcpy(sym, name);
	
	if (count >= (size - 1))
	{
		// always leave a null entry at end
		if ((list = (char **)realloc((char *)list,
			(sizeof(char *) * (size + LISTENT)))) == 0)
		{
			new_handler();
		}
		memset(list+size, 0, sizeof(char *) * LISTENT);
		size += LISTENT;
	}
	list[count++] = sym;
}

int
fcmp(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}

void
FuncList::printall()
{
	char		*symentry, **lptr;
	char		*last = 0;

	if (!count)
		return;

	qsort(list, count, sizeof(char *), (int (*)(const void *,
		const void *))fcmp);

        lptr = list;
	symentry = *lptr;
	for ( ; symentry; symentry = *(++lptr))
	{
		if (prismember(&interrupt, SIGINT))
			break;
		if (last && (strcmp(last, symentry) == 0))
			continue;
		printm(MSG_function, symentry);
		last = symentry;
	}
}

static void 
search_file(Symbol fsymb, ProcObj *pobj, FuncList *funclist, int source_only)
{
	Source	source;
	char	*fname;
	char	*sname;

	if (source_only && !fsymb.source(source))
		return;
	fname = fsymb.name();
	Symbol	s = fsymb.child();
	for(; !s.isnull(); s = s.sibling())
	{
		if (prismember(&interrupt, SIGINT))
			break;
		Tag t = s.tag();
		switch(t)
		{
		case t_entry:
		case t_subroutine:
			sname = pobj->symbol_name(s);
			if (sname && *sname)
				funclist->add(sname, fname);
			break;
		case t_global_sub:
			if (source_only)
			{
				sname = pobj->symbol_name(s);
				if (sname && *sname)
					funclist->add(sname, 0);
			}
			break;
		default:
			break;
		}
	}
}

static void 
search_symtab(Symtab *stp, ProcObj *pobj, FuncList *funclist, int source_only)
{
	Symbol	sym = stp->first_symbol();
	for(; !sym.isnull(); sym = sym.sibling())
	{
		if (prismember(&interrupt, SIGINT) ||
			(!sym.isSourceFile()))
			break;
		search_file(sym, pobj, funclist, source_only);
	}

	// if not looking only for symbols for which we have debug info,
	// use global list to get globals
	if (!source_only)
	{
		char		*sname;
		NameEntry	*ne;
		ne = stp->first_global();
		for(; ne; ne = stp->next_global())
		{
			sym = stp->global_symbol(ne);
			if (!sym.isnull() && sym.isGlobalSub())
			{
				sname = pobj->symbol_name(sym);
				if (sname && *sname)
					funclist->add(sname, 0);
			}
		}
	}
}

int
functions(const char *filename, const char *object, int source_only)
{
	ProcObj		*pobj;
	static FuncList	*funclist;
	
	pobj = proglist.current_object();

	sigrelse(SIGINT);

	if (!funclist)
		funclist = new FuncList;
	else
		funclist->clear();
	if (filename)
	{
		Symbol	file;
		if (!pobj->find_source(filename, file))
		{
			printe(ERR_no_source_info, E_ERROR, filename);
			return 0;
		}
		search_file(file, pobj, funclist, source_only);
	}
	else
	{
		Symtab	*stp;

		if (!object)
			stp = pobj->find_symtab(pobj->curframe()->pc_value());
		else
			stp = pobj->find_symtab(object);
		if (!stp)
		{
			printe(ERR_no_sym_info, E_ERROR, pobj->obj_name());
			return 0;
		}
		search_symtab(stp, pobj, funclist, source_only);
	}
	funclist->printall();

	sighold(SIGINT);
	return 1;
}
