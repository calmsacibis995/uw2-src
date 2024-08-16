#ident	"@(#)debugger:libutil/common/callstack.C	1.11"

#include "Interface.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Proglist.h"
#include "Symtab.h"
#include "Source.h"
#include "Frame.h"
#include "Machine.h"
#include "Expr.h"
#include "Rvalue.h"
#include "Parser.h"
#include "global.h"
#include "Tag.h"
#include "Buffer.h"
#include "utility.h"
#include <signal.h>

static void show_call(ProcObj *, Frame *, int, Buffer *);

// print callstack - if pc or sp are non-0 we allow user to specify
// initial pc and/or sp

int
print_stack(Proclist *procl, int how_many, int first, Iaddr pc, Iaddr sp)
{
	int	count = 0;
	int	single = 1;
	int	ret = 1;
	ProcObj	*pobj;
	plist	*list;
	Buffer	*buf = buf_pool.get();

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
		buf_pool.put(buf);
		return 0;
	}
	sigrelse(SIGINT);
	do
	{
		Frame *frame;

		if (!pobj->state_check(E_RUNNING|E_DEAD))
		{
			ret = 0;
			continue;
		}


		printm(MSG_stack_header, pobj->obj_name(), pobj->prog_name());
		if (pc != 0 || sp != 0)
			frame = pobj->topframe(pc, sp);
		else
			frame = pobj->topframe();

		count = count_frames(pobj); // count is 1 less than
						// number of frames

		if (first >= 0) // start with first rather than top
		{
			while(first < count)
			{
				frame = frame->caller();
				count--; 
			}
		}
		if ( how_many == 0 )
			how_many = count+1;		// big enough

		while ( frame && how_many-- && 
			!prismember(&interrupt, SIGINT))
		{
			show_call(pobj, frame, count, buf);
			count--;
			frame = frame->caller();
		}
		if (!single)
			printm(MSG_newline);
		if (prismember(&interrupt, SIGINT))
			break;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));

	sighold(SIGINT);
	buf_pool.put(buf);
	return ret;
}

static void
show_call(ProcObj *pobj, Frame *frame, int count, Buffer *buf)
{
	Iaddr	pc = frame->pc_value();
	Symtab	*symtab = pobj->find_symtab(pc);
	char	*filename = 0;
	long	line = 0;
	int	i = 0;
	int	assumed = 0;
	int	nbytes = frame->nargwds(assumed) * sizeof(int);
	int	done = 0;


	if (symtab)
	{
		Symbol	symbol;
		Source	source;

		if (symtab->find_source(pc, symbol))
		{
			filename = symbol.name();
			if (symbol.source( source ))
				source.pc_to_stmt( pc, line );
		}

		Symbol	symbol2 = pobj->find_entry( pc );
		char	*name = pobj->symbol_name(symbol2);
		if ( !name || !*name )
			name = "?";

		printm(MSG_stack_frame, (unsigned long)count, name);
		if ( (nbytes > 0) && frame->caller() ) 
		{
			Symbol	arg = symbol2.child();
			Tag	tag = arg.tag();
			Rvalue	rval;
			while ( !arg.isnull() ) 
			{
				if (tag == t_argument ) 
				{
					char	*val;
					Expr expr( arg, pobj );
					if (frame->incomplete()
						|| !expr.eval( pobj, pc, frame )
					    	|| !expr.rvalue( rval ))
						val = "???" ;
					else
					{
						buf->clear();
						rval.print(buf, pobj,
						DEBUGGER_BRIEF_FORMAT);
						val = buf->size() ? (char *)*buf : "";
					}
					printm(MSG_stack_arg, (i++>0) ?
						", " : "",
						arg.name(), val);
					//
					// round to word boundary
					//
					if (!rval.isnull())
						done+=ROUND_TO_WORD(rval.type()->size());
					else
						done+=ROUND_TO_WORD(sizeof(long));
				}
				arg = arg.sibling();
				tag = arg.tag();
			}
		}
	}
	else
		printm(MSG_stack_frame, count, "?");

	if (frame->caller() && !frame->incomplete()) 
	{
		int	n = done/sizeof(int);
		int	first = 1;
		while ( done < nbytes ) 
		{
			if (first && assumed)
			{
				// number of args is a guess
				first = 0;
				printm( MSG_stack_arg3, (i++>0)?", ":"", 
					frame->argword(n++) );
			}
			else
				printm( MSG_stack_arg2, (i++>0)?", ":"", 
					frame->argword(n++) );
			done += sizeof(int);
		}
	}

	if ( filename && *filename && line) 
	{
		printm(MSG_stack_frame_end_1, filename, line);
	} 
	else 
	{
		printm(MSG_stack_frame_end_2, pc);
	}

}
