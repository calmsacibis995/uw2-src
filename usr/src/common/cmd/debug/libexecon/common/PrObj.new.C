#ident	"@(#)debugger:libexecon/common/PrObj.new.C	1.5"

#include "Iaddr.h"
#include "ProcObj.h"
#include "Process.h"
#include "Instr.h"
#include "global.h"

ProcObj::ProcObj(int pflags) : instr(this)
{
	flags = pflags;
	verbosity = vmode;
	pc = lopc = hipc = 0;
	dot = 0;
	current_srcfile = 0;
	last_sym = 0;
	epoch = 0;
	hw_watch = 0;
	sw_watch = 0;
	latestbkpt = 0;
	latestexpr = 0;
	latestflt = latestsig = latesttsc = 0;
	hoppt = destpt = dynpt = 0;
	foreignlist = 0;
#ifdef DEBUG_THREADS
	threadpt = startpt = 0;
#endif
	cur_frame = top_frame = 0;
	saved_gregs = 0;
	saved_fpregs = 0;
}

ProcObj::~ProcObj()
{
	delete saved_gregs;
	delete saved_fpregs;
}

pid_t
ProcObj::pid()
{
	return process()->pid();
}

Program *
ProcObj::program()
{
	return process()->program();
}

EventTable *
ProcObj::events()
{
	return process()->events();
}

// null base class versions

Process *
ProcObj::process()
{
	return 0;
}
