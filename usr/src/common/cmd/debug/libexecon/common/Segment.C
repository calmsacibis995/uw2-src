#ident	"@(#)debugger:libexecon/common/Segment.C	1.4"

#include "Segment.h"
#include "Procctl.h"
#include "Itype.h"
#include "Symtab.h"
#include "Dyn_info.h"
#include <string.h>

Segment::Segment(Procctl *pctl, Symnode *sym, 
	Iaddr lo, long sz, long b, int flags)
{
	symnode = sym;
	access = pctl;
	loaddr = lo;
	hiaddr = loaddr + sz;
	base = b;
	prot_flags = flags;
}

int
Segment::read( Iaddr addr, void * buffer, int len )
{
	long	offset;

	if (!access)
		return -1;
	offset = addr - loaddr + base;
	return access->read( offset, buffer, len );
}

static int	size[] = {	
	0,
	sizeof(Ichar),
	sizeof(Iint1),
	sizeof(Iint2),
	sizeof(Iint4),
	sizeof(Iuchar),
	sizeof(Iuint1),
	sizeof(Iuint2),
	sizeof(Iuint4),
	sizeof(Isfloat),
	sizeof(Idfloat),
	sizeof(Ixfloat),
	sizeof(Iaddr),
	sizeof(Ibase),
	sizeof(Ioffset),
	0
};

int
stype_size( Stype stype )
{
	return size[stype];
}

int
Segment::read( Iaddr addr, Stype stype, Itype & itype )
{
	long	offset;

	if (!access)
		return -1;
	offset = addr - loaddr + base;
	return(access->read( offset, &itype, size[stype] ));
}

Symnode::Symnode( const char * s, Iaddr ss_base )
{
	pathname = new(char[strlen(s) + 1]);
	strcpy( (char *)pathname, s );
	sym.ss_base = ss_base;
	dyn_info = 0;
	sym.symtable = 0;
	brtbl_lo = 0;
	brtbl_hi = 0;
}

Symnode::~Symnode()
{
	delete (void *)pathname;
	delete dyn_info;
}
