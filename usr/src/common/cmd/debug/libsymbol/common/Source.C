#ident	"@(#)debugger:libsymbol/common/Source.C	1.2"
#include	"Source.h"
#include	"Lineinfo.h"

Source::Source()
{
	lineinfo = 0;
	ss_base = 0;
}

Source::Source(const Source& source )
{
	lineinfo = source.lineinfo;
	ss_base = source.ss_base;
}

Source&
Source::operator=( const Source & source )
{
	lineinfo = source.lineinfo;
	ss_base = source.ss_base;
	return *this;
}

void
Source::pc_to_stmt( Iaddr pc, long & line, int slide, Iaddr *stmt_start )
{
	int		first,middle,last;
	int		found;
	long		entrycount;
	LineEntry *	pcinfo;

	if (stmt_start)
		*stmt_start = 0;
	if ( lineinfo == 0 )
	{
		line = 0;
		return;
	}
	entrycount = lineinfo->entrycount;
	pcinfo = lineinfo->addrpart;
	pc -= ss_base;
	found = 0;
	first = 0;
	last = (int)entrycount - 1;
	if (pc < pcinfo[first].addr)
	{
		if (slide > 0)
		{
			line = pcinfo[first].linenum;
			if (stmt_start)
				*stmt_start = pcinfo[first].addr + ss_base;
		}
		else 
			line = 0;

		return;
	}
	else if (pc > pcinfo[last].addr)
	{
		if (slide < 0)
		{
			line = pcinfo[last].linenum;
			if (stmt_start)
				*stmt_start = pcinfo[last].addr + ss_base;
		}
		else 
			line = 0;
		return;
	}
	while (first <= last)
	{
		middle = ( first + last ) / 2;
		if ( pcinfo[middle].addr == pc )
		{
			found = 1;
			break;
		}
		else if ( pcinfo[middle].addr > pc )
		{
			last = middle - 1;
		}
		else
		{
			first = middle + 1;
		}
	}
	if ( found )
	{
		line = pcinfo[middle].linenum;
		if (stmt_start)
			*stmt_start = pc + ss_base;
	}
	else if (slide == 0)
	{
		line = 0;
	}
	else if ( slide < 0 )
	{
		line = pcinfo[last].linenum;
		if (stmt_start)
			*stmt_start = pcinfo[last].addr + ss_base;
	}
	else 
	{
		line = pcinfo[last+1].linenum;
		if (stmt_start)
			*stmt_start = pcinfo[last+1].addr + ss_base;
	}
	return;
}

void
Source::stmt_to_pc( long line, Iaddr & pc, int slide )
{
	int		first, middle, last;
	int		found;
	long		entrycount;
	LineEntry 	*stmtinfo;

	if ( lineinfo == 0 )
	{
		pc = 0;
		return;
	}
	entrycount = lineinfo->entrycount;
	stmtinfo = lineinfo->linepart;
	found = 0;
	first = 0;
	last = (int)entrycount - 1;
	if (line < stmtinfo[first].linenum)
	{
		if (slide > 0)
			pc = stmtinfo[first].addr + ss_base;
		else 
			pc = 0;

		return;
	}
	else if (line > stmtinfo[last].linenum)
	{
		if (slide < 0)
			pc = stmtinfo[last].addr + ss_base;
		else 
			pc = 0;
		return;
	}
	while (first <= last)
	{
		middle = ( first + last ) / 2;
		if ( stmtinfo[middle].linenum == line )
		{
			found = 1;
			break;
		}
		else if ( stmtinfo[middle].linenum > line )
		{
			last = middle - 1;
		}
		else
		{
			first = middle + 1;
		}
	}
	if ( found )
	{
		// find first entry for that line number here
		pc = stmtinfo[middle].addr + ss_base;
	}
	else if (slide == 0)
	{
		pc = 0;
	}
	else if ( slide < 0 )
	{
		// find first entry for that line number here
		pc = stmtinfo[last].addr + ss_base;
	}
	else
	{
		pc = stmtinfo[last+1].addr + ss_base;
	}
	return;
}
