#ident	"@(#)debugger:libexecon/common/PrObj.sym.C	1.4"

#include <stdlib.h>
#include "ProcObj.h"
#include "Process.h"
#include "Seglist.h"
#include "Symtab.h"
#include "Source.h"
#include "Interface.h"
#include "Instr.h"
#include "str.h"

// several routines declare a symbol just to return a null symnol;
// having one instance avoids all those calls to the Symbol constructor
// and destructor
static Symbol null_symbol;

// keep track of most recent function, symtab and source line
// to save needing to look them up constantly.
int
ProcObj::find_cur_src(Iaddr use_pc)
{
	Symbol	func;
	Source	source;
	Iaddr	pcval;

	if (use_pc == (Iaddr)-1)
		pcval = pc;
	else
		pcval = use_pc;
	dot = pcval;	// used for disassembly
	if (!is_core())
		seglist->update_stack(process()->proc_ctl());

	if ((pcval >= lopc) && (pcval < hipc))
	{
		// still in address range of current function
		if (!last_src.isnull())
		{
			if (!last_src.source(source))
			{
				printe(ERR_internal, E_ERROR,
					"ProcObj::find_cur_src()", __LINE__);
				last_src.null();
				return 0;
			}
			current_srcfile = last_src.name();
			source.pc_to_stmt( pcval, currstmt.line );
		}
		return 1;
	}
	if ((last_sym = seglist->find_symtab(pcval)) == 0)
	{
		current_srcfile = 0;
		lopc = hipc = 0;
		last_src.null();
		currstmt.unknown();
		return 1;
	}
	func = last_sym->find_entry( pcval );
	if (func.isnull())
	{
		// this can occur if you have partial debugging
		// information for an object
		current_srcfile = 0;
		lopc = hipc = 0;
		last_src.null();
		currstmt.unknown();
		return 1;
	}
	lopc = func.pc(an_lopc);
	hipc = func.pc(an_hipc);
	if (!last_sym->find_source(pcval, last_src))
	{
		// this can occur if you have partial debugging
		// information for an object
		current_srcfile = 0;
		last_src.null();
		currstmt.unknown();
		return 1;
	}
	current_srcfile = last_src.name();
	if (!last_src.source(source))
	{
		last_src.null();
		currstmt.unknown();
		return 0;
	}
	source.pc_to_stmt( pcval, currstmt.line );
	return 1;
}

// name of executable or shared object containing a given pc
const char *
ProcObj::object_name( Iaddr addr )
{
	return seglist->object_name( addr );
}

Symtab *
ProcObj::find_symtab( Iaddr addr )
{
	if ((addr >= lopc) && (addr < hipc) && last_sym)
		return last_sym;
	return seglist->find_symtab( addr );
}

Symtab *
ProcObj::find_symtab( const char *name )
{
	return seglist->find_symtab( name );
}

Symbol
ProcObj::find_entry( Iaddr addr )
{
	Symtab *symtab;

	if ((symtab = find_symtab(addr)) == 0)
	{
		return null_symbol;
	}
	return(symtab->find_entry( addr ));
}

Symbol
ProcObj::find_symbol( Iaddr addr )
{
	Symtab *symtab;

	if ((symtab = find_symtab(addr)) == 0)
	{
		return null_symbol;
	}
	return(symtab->find_symbol( addr ));
}

Symbol
ProcObj::find_scope( Iaddr addr )
{
	Symtab *symtab;

	if ((symtab = find_symtab(addr)) == 0)
	{
		return null_symbol;
	}
	return(symtab->find_scope( addr ));
}

int
ProcObj::find_source( const char * name, Symbol & symbol )
{
	return seglist->find_source( name, symbol );
}

Symbol
ProcObj::find_global(const char * name )
{
	return seglist->find_global( name );
}

int
ProcObj::find_next_global(const char *name, Symbol &sym)
{
	return seglist->find_next_global(name, sym);
}

Dyn_info *
ProcObj::get_dyn_info(Iaddr addr)
{
	return seglist->get_dyn_info(addr);
}

void
ProcObj::set_current_stmt( const char * filename, long line )
{
	current_srcfile = str( filename );
	currstmt.line = line;
}

//
// get symbol name
// checks for presence of COFF static shared libraries
//
char *
ProcObj::symbol_name( Symbol symbol )
{
	char	*name;
	int	offset;
	Iaddr	addr, newaddr;
	Symbol	newsymbol;

	name = symbol.name();
	// if there are no static shared libs, 
	// return symbol name unchanged
	if ( seglist->has_stsl() == 0 || name == 0)
		return name;

	// if name is ".bt?" get real name of function 
	// from the branch table.
	if ( (name[0] == '.') && (name[1] == 'b') && (name[2] == 't') ) 
	{
		offset  = atoi(name+3);	// offset in branch table
		addr = symbol.pc(an_lopc);
		newaddr = instr.fcn2brtbl( addr, offset);
		newsymbol = find_entry(newaddr);
		if ( newsymbol.isnull() == 0 )
			name = newsymbol.name();
	}	
	return name;
}	

Symbol
ProcObj::first_file()
{
	return seglist->first_file();
}

Symbol
ProcObj::next_file()
{
	return seglist->next_file();
}

int
ProcObj::find_stmt( Stmt & stmt, Iaddr addr )
{
	Symtab *	symtab;
	Symbol		symbol;
	Source		source;

	if ((addr >= lopc) && (addr < hipc) && last_sym)
	{
		symtab = last_sym;
		if (last_src.isnull())
		{
			stmt.unknown();
			return 1;
		}
		if ( last_src.source(source) == 0 )
		{
			stmt.unknown();
			printe(ERR_internal, E_ERROR,
				"ProcObj::find_stmt()", __LINE__);
			return 0;
		}
	}
	else
	{
		if (((symtab = seglist->find_symtab(addr)) == 0) ||
			( symtab->find_source(addr, symbol) == 0 ) ||
			( symbol.source(source) == 0 ))
		{
			stmt.unknown();
			return 1;
		}
	}
	source.pc_to_stmt(addr, stmt.line);
	return 1;
}

// If addr is beginning address of a function, return address
// of first line past function prolog (1st real statement).
// If we have no symbolic information, the addr is not the
// beginning of a function, or the function has no executable
// statements, just return addr.
// If we have no line information, use the disassembler's
// fcn_prolog mechanism.

Iaddr
ProcObj::first_stmt(Iaddr addr)
{
	Source	source;
	Symtab	*symtab;
	Symbol	symbol, func;
	Iaddr	naddr, hi;
	long	line;
	int	tmp;
	Iaddr	tmp2;

	find_cur_src();
	if ((addr >= lopc) && (addr < hipc))
	{
		// within range of current function
		if (addr != lopc)
			// not at beginning of function
			return addr;
		if (last_src.isnull())
		{
			return instr.fcn_prolog(addr, tmp, tmp2, 0);
		}
		if (last_src.source(source) == 0)
		{
			printe(ERR_internal, E_ERROR, 
				"ProcObj::first_statement", __LINE__);
			return addr;
		}
		hi = hipc;
	}
	else
	{
		if ((symtab = find_symtab(addr)) == 0)
			return addr;
		func = symtab->find_entry( addr );

		if (func.isnull() || (addr != func.pc(an_lopc)))
			// no symbol info or not at beginning
			return addr;

		hi = func.pc(an_hipc);
		if ((symtab->find_source(addr, symbol) == 0)
			|| symbol.isnull() 
			|| (symbol.source(source) == 0))
		{
			return instr.fcn_prolog(addr, tmp, tmp2, 0);
		}
	}
	// we have line info
	// find next statement past beginning
	source.pc_to_stmt(addr+1, line, 1, &naddr); 
	if ((naddr == 0) || (naddr >= hi)) 
		// no such statement within function
		return instr.fcn_prolog(addr, tmp, tmp2, 0);
	return naddr;
}
