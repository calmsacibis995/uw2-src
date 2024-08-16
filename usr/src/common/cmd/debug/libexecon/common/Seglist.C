#ident	"@(#)debugger:libexecon/common/Seglist.C	1.20"

#include "Process.h"
#include "ProcObj.h"
#include "Link.h"
#include "Seglist.h"
#include "Symtab.h"
#include "Object.h"
#include "Dyn_info.h"
#include "Segment.h"
#include "Procctl.h"
#include "Interface.h"
#include "Symbol.h"
#include "Rtl_data.h"
#include "Tag.h"
#include "global.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

// Segment list processing is divided between common
// and machine dependent pieces.  The machine dependencies
// have mainly to do with shared library processing.

// Each Seglist is shared among all the threads in a process.
// For each Seglist we maintain two lists: a list of address ranges
// (segments) and a list of objects with their associated symbol
// tables (symnodes).  Many segments may be associated with a
// single symnode.

Seglist::Seglist( Process * process)
{
	proc = process;
	mru_segment = 0;
	first_segment = last_segment = stack_segment = 0;
	symlist = 0;
	static_loaded = 0;
	symnode_file = symnode_global = 0;
	uses_rtld = 0;
	_has_stsl = 0;
	rtl_data = 0;
	start = 0;
}

Seglist::~Seglist()
{
	Segment *seg = first_segment;
	Symnode	*sym = symlist;

	delete rtl_data;
	while (seg)
	{
		Segment	*cur;
		cur = seg;
		seg = seg->next();
		cur->unlink();
		delete cur;
	}
	while (sym)
	{
		Symnode	*cur;
		cur = sym;
		sym = sym->next();
		cur->unlink();
		delete cur;
	}
}

// Determine if executable is dynamically linked.  If so,
// find interpreter path and create an Rtl_data structure.
int
Seglist::setup( int fd, const char *exec_name )
{
	Object		*obj;
	Sectinfo	sinfo;

	DPRINT(DBG_SEG, ("Seglist::setup(this == %#x ...)\n", this));
	if ( fd == -1 )
	{
		return 0;
	}
	if ((obj = find_object(fd, exec_name)) == 0)
		return 0;

	if ((obj->getsect(s_dynamic, &sinfo) == 0) ||
		(obj->getsect(s_interp, &sinfo) == 0))
	{
		return 1;
	}
	rtl_data = new Rtl_data((char *)sinfo.data);
	uses_rtld = 1;
	return 1;
}

//
// build segments from a.out and static shared libs if there are any
// add a segment for the stack;
int
Seglist::build_static( Proclive *pctl, const char *executable_name )
{
	Segment	*seg;
	int 	fd;
	Object	*obj;
	Symnode	*sym;

	DPRINT(DBG_SEG, ("Seglist::build_static(this == %#x, pctl == %#x, exec_name == %s)\n", this, pctl, executable_name));
	if (( (fd = pctl->open_object(0, executable_name)) == -1 ) ||
		((obj = find_object(fd, 0)) == 0) ||
		((sym =  add( fd, (Procctl *)pctl, executable_name,
			0, 0L )) == 0 ) ||
		( add_static_shlib( obj, (Procctl *)pctl, 0 ) == 0 ))
	{
		if (fd >= 0)
			close(fd);
		return 0;
	}
	seg = new Segment( (Procctl *)pctl, sym, 0, 0, 0, 
		SEG_LOAD|SEG_READ|SEG_WRITE);
	stack_segment = seg;
	update_stack(pctl);
	add_segment(seg);
	close(fd);
	return 1;
}

//
// is addr in stack segment
//
int
Seglist::in_stack( Iaddr addr )
{
	if ( addr == 0 )
		return 0;
	if (!stack_segment ||
		((stack_segment->loaddr == 0) &&
		 (stack_segment->hiaddr == 0)) )
		return ( !in_text(addr) );
	else if ( (stack_segment->loaddr <= addr) &&
		 (addr < stack_segment->hiaddr) )
		return 1;
	return 0;
}

Iaddr
Seglist::end_stack()
{
	if (!stack_segment ||
		((stack_segment->loaddr == 0) &&
		 (stack_segment->hiaddr == 0)) )
		return 0;
	return stack_segment->hiaddr;
}

//
// update stack segment boundaries
//
void
Seglist::update_stack(Proclive *pctl)
{
	if (pctl->update_stack( stack_segment->loaddr, 
		stack_segment->hiaddr ) == 0)
		printe(ERR_sys_map, E_ERROR, strerror(errno));
}

//
// get static shared lib branch table addresses
//
int
Seglist::get_brtbl( const char * name )
{
	Symnode *symnode;
	Symbol	symbol;

	symnode = symlist;
	while ( symnode != 0) 
	{
		if (!strcmp(symnode->pathname, name))
			break;
		symnode = symnode->next();
	}
	if (symnode)
		symnode->sym.find_source("branchtab", symbol);
	if ( !symnode || symbol.isnull() ) 
	{
		printe(ERR_branch_tbl, E_ERROR, name);
		return 0;
	}
	symnode->brtbl_lo = symbol.pc(an_lopc);
	symnode->brtbl_hi = symbol.pc(an_hipc);
	return 1;
}

int
Seglist::build( Proclive *pctl, const char *exec_name )
{
	DPRINT(DBG_SEG, ("Seglist::build(this == %#x, pctl == %#x, exec_name == %s)\n", this, pctl, exec_name));
	if (!static_loaded)
	{
		// build static builds segment list for a.out
		// and static shared libraries
		if ( build_static( pctl, exec_name ) == 0 )
		{
			return 0;
		}
		else
			static_loaded = 1;
	}
	return build_dynamic( pctl );
}

int
Seglist::add_static_shlib( Object *obj, Procctl *pctl, int text_only )
{
	int		i;
	char		**stsl_names;
	int		fd;
	Sectinfo	sinfo;

	DPRINT(DBG_SEG, ("Seglist::add_static_shlib(this == %#x, obj == %#x, pctl == %#x, text_only == %d\n", this, obj, pctl, text_only));
	if (!obj)
		return 0;
	// if object has a .lib section, read list of shlib pathnames
	if (obj->getsect( s_lib, &sinfo) == 0)
	{
		return 1;
	}
	else if ( (stsl_names = obj->get_stsl_names()) == 0 )
	{
		return 0;
	}
	for ( i = 0; stsl_names[i] != 0; ++i )
	{
		Procctl	*nctl;
		if ((fd = open(stsl_names[i], O_RDONLY)) < 0)
		{
			printe(ERR_shlib_open, E_ERROR, stsl_names[i],
				strerror(errno));
			return 0;
		}
		if (pctl->get_type() == pt_live)
			nctl = pctl;
		else
		{
			nctl = new Procctl;
			if (!nctl->open(fd))
			{
				delete nctl;
				return 0;
			}
		}
		if (!add( fd, nctl, stsl_names[i], text_only, 0 ))
			return 0;
		if (!get_brtbl( stsl_names[i] ))
			return 0;
		_has_stsl++;
		close(fd);
	}
	return 1;
}
//
// read proto (post-mortem debugging)
// get segments from a.out, shared libs ( if any )
// and core file.
//
int
Seglist::readproto( Procctl *txtctl, Proccore *core,
	const char *executable_name )
{
	Segment 	*seg;
	int		i;
	Elf_Phdr 	*phdr;
	Object		*obj;
	int		textfd;
	Iaddr		lo, hi;

	DPRINT(DBG_SEG, ("Seglist::readproto(this == %#x, txtctl == %#x, core == %#x, executable_name == %s)\n", this, txtctl, core, executable_name));

	if ( !core || !txtctl )
	{
		printe(ERR_internal, E_ERROR, 
			"Seglist::readproto", __LINE__);
		return 0;
	}
	if ((textfd = txtctl->get_fd()) < 0)
	{
		return 0;
	}
	if ((obj = find_object(textfd, executable_name)) == 0)
		return 0;

	i = 0;
	for ( phdr = core->segment(i) ; phdr ;
			phdr = core->segment(++i) )
	{
		// add segment descriptors for data dumped to core file
		if ( phdr->p_memsz && phdr->p_filesz )
		{
			int	pflags;
			DPRINT(DBG_SEG, ("Seglist::readproto: add core segment, addr = %#x, offset = %#x, filesz = %d, flags = %d\n", phdr->p_vaddr, phdr->p_offset,phdr->p_filesz, phdr->p_flags));
			pflags = SEG_LOAD;
			if (phdr->p_flags & PF_W)
				pflags |= SEG_WRITE;
			if (phdr->p_flags & PF_X)
				pflags |= SEG_EXEC;
			if (phdr->p_flags & PF_R)
				pflags |= SEG_READ;
			seg = new Segment( core, 0, phdr->p_vaddr,
				phdr->p_filesz, phdr->p_offset,
				pflags);
			add_segment(seg);
		}
	}
	if (core->update_stack(lo, hi))
	{
		for (Segment *seg = first_segment; seg; 
			seg = seg->next())
		{
			if ((seg->hiaddr <= hi) && (seg->loaddr >= lo))
			{
				stack_segment = seg;
				break;
			}
		}
	}
	// add segment descriptors for a.out and static shlib text
	if (!add( textfd, txtctl, executable_name, 1, 0 ))
		return 0;
	if (!add_static_shlib( obj, txtctl, 1 ))
		return 0;
	return add_dynamic_text( txtctl, executable_name );
}

Segment *
Seglist::find_segment( Iaddr addr )
{
	Segment	*seg;

	if ( ( mru_segment != 0 ) && ( mru_segment->loaddr <= addr ) &&
		( mru_segment->hiaddr > addr ) )
	{
		return mru_segment;
	}
	for ( seg = first_segment ; seg != 0 ; seg = seg->next() )
	{
		if ( (addr >= seg->loaddr) && ( addr < seg->hiaddr) )
		{
			mru_segment = seg;
			return seg;
		}
	}
	return 0;
}

Symtab *
Seglist::find_symtab( Iaddr addr )
{
	Segment *seg;

	if ( ( mru_segment != 0 ) && ( mru_segment->loaddr <= addr ) &&
		( mru_segment->hiaddr > addr ) )
	{
		seg = mru_segment;
	}
	else
	{
		for (seg = first_segment; seg != 0; seg = seg->next())
		{
			if ((addr >= seg->loaddr) && 
				(addr < seg->hiaddr))
			{
				mru_segment = seg;
				break;
			}
		}
	}
	if (!seg || !seg->symnode)
		return 0;
	return &seg->symnode->sym;
}

// find machine specific dynamic information for symnode associated
// with addr
Dyn_info *
Seglist::get_dyn_info(Iaddr addr)
{
	Segment	*seg;

	if (((seg = find_segment(addr)) == 0) || !seg->symnode)
		return 0;
	return seg->symnode->dyn_info;
}

Symtab *
Seglist::find_symtab( const char *name )
{
	Symnode *symnode;

	for (symnode = symlist; symnode != 0; symnode = symnode->next())
	{
		if ((strcmp(name, symnode->pathname) == 0)
			|| (strcmp(name, 
			basename((char *)symnode->pathname)) == 0))
			return &symnode->sym;
			
	}
	return 0;
}

const char *
Seglist::object_name(Iaddr addr)
{
	Segment	*seg;

	if ( ( mru_segment != 0 ) && ( mru_segment->loaddr <= addr ) &&
		( mru_segment->hiaddr > addr ) )
	{
		seg = mru_segment;
	}
	else
	{
		for (seg = first_segment; seg != 0; seg = seg->next())
		{
			if ((addr >= seg->loaddr) && 
				(addr < seg->hiaddr))
			{
				mru_segment = seg;
				break;
			}
		}
	}
	if (!seg || !seg->symnode)
		return 0;
	return(seg->symnode->pathname);
}

int
Seglist::find_source( const char * name, Symbol & symbol )
{
	Symnode *symnode;

	for (symnode = symlist; symnode != 0; symnode = symnode->next())
	{
		if ( symnode->sym.find_source( name, symbol ) != 0 )
		{
			return 1;
		}
	}
	return 0;
}

int
Seglist::find_next_global(const char *name, Symbol &sym)
{
	Symnode		*symnode;

	if (sym.isnull())
		symnode_global = symnode = symlist;
	else
		symnode = symnode_global;

	if (symnode == 0)
		return 0;
	if (symnode->sym.find_next_global(name, sym))
		return 1;

	for (symnode = symnode->next(); symnode; symnode = symnode->next())
	{
		if (symnode->sym.find_next_global(name, sym))
		{
			symnode_global = symnode;
			return 1;
		}
	}
	symnode_global = 0;
	return 0;
}

Symbol
Seglist::first_file()
{
	Symnode *symnode;
	Symbol	first;

	symnode = symlist;
	while ( symnode != 0 )
	{
		first = symnode->sym.first_symbol();
		if ( !first.isnull() && (first.tag() == t_sourcefile))
		{
			symnode_file = symnode;
			current_file = first;
			return first;
		}
		else
		{
			symnode = symnode->next();
		}
	}
	symnode_file = 0;
	current_file.null();
	return first;
}

Symbol
Seglist::next_file()
{
	Symnode *symnode;
	Symbol	next;

	symnode = symnode_file;
	if ( symnode == 0 )
	{
		return next;
	}
	next = current_file.arc( an_sibling );
	if ( !next.isnull() )
	{
		current_file = next;
		return next;
	}
	symnode = symnode->next();
	while ( symnode != 0 )
	{
		next = symnode->sym.first_symbol();
		if ( !next.isnull() && (next.tag() == t_sourcefile))
		{
			symnode_file = symnode;
			current_file = next;
			return next;
		}
		else
		{
			symnode = symnode->next();
		}
	}
	symnode_file = 0;
	current_file.null();
	return next;
}

Symbol
Seglist::find_global( const char * name )
{
	Symnode *symnode;
	Symbol	symbol;

	symnode = symlist;
	while ( symnode != 0 )
	{
		symbol = symnode->sym.find_global(name);
		if ( symbol.isnull() )
		{
			symnode = symnode->next();
			continue;
		}
		if ( symnode->brtbl_lo > 0 ) 
		{
			//
			// static shared library
			//
			
			Iaddr addr = symbol.pc(an_lopc);
			if ( (addr >= symnode->brtbl_lo) &&
			    ( addr < symnode->brtbl_hi) ) 
			{
				addr = proc->instruct()->brtbl2fcn(addr);
				symbol = symnode->sym.find_entry(addr);
			}
		}
		symnode_global = symnode;
		return symbol;
	}
	return symbol;
}

// Add a segment entry for each loadable segment - if text_only
// set, skip writeable segments
Symnode *
Seglist::add( int fd, Procctl *pctl, const char * name, int text_only, 
	Iaddr base )
{
	Iaddr	addr;
	Segment	*seg;
	int	shared;
	Seginfo	*seginfo;
	int	i, count;
	Object	*obj;
	Symnode	*sym;

	DPRINT(DBG_SEG, ("Seglist::add(this == %#x, fd == %d, pctl == %#x, name == %s, text_only == %d, base == %#x )\n", this, fd, pctl, name, text_only, base));

	if ( fd < 0 )
	{
		return 0;
	}
	if (((obj = find_object(fd, 0)) == 0) ||
		( (seginfo = obj->get_seginfo( count, shared )) == 0 ))
	{
		return 0;
	}


	if ((sym = add_symnode( obj, name, base)) == 0)
	{
		return 0;
	}

	if ( !shared )
	{
		start = obj->start_addr();
	}
	for ( i = 0; i < count ; ++i )
	{
		if (!(seginfo[i].seg_flags & SEG_LOAD ))
		{
			continue;
		}
		addr = seginfo[i].vaddr;
		if ( shared )
		{
			addr += base;
		}
		if ( text_only && (seginfo[i].seg_flags & SEG_WRITE) ) 
		{
			// add name to anonymous segments from corefile
			add_name(sym, addr, seginfo[i].file_size);
			continue;
		}
		else
		{
			// if segment is not writable, assume it's text
			seg = new Segment(pctl, sym, addr, 
				seginfo[i].mem_size, seginfo[i].offset,
				seginfo[i].seg_flags);
			add_segment(seg);
		}
	}
	return sym;
}

Symnode *
Seglist::add_symnode( Object *obj, const char *name, Iaddr base)
{
	Symnode	*symnode, *prev;

	if (name == 0)
	{
		printe(ERR_internal, E_ERROR, 
			"Seglist::add_symnode", __LINE__);
		return 0;
	}
	prev = 0;
	for(symnode = symlist; symnode; 
		prev = symnode, symnode = symnode->next())
	{
		if ( strcmp( name, symnode->pathname ) == 0 )
		{
			// already an entry
			return symnode;
		}
	}
	symnode = new Symnode( name, base );
	if (prev)
		symnode->append(prev);
	else
		symlist = symnode;
	symnode->sym.symtable = obj->get_symtable();
	add_dyn_info(symnode, obj);
	return symnode;
}

static const char *modes[] = {
	"___", "__X", "_W_", "_WX",
	"R__", "R_X", "RW_","RWX"
};

// macros to truncate to previous page or round to next page
// /proc maintains segment list on page boundaries
#define PTRUNC(X) ((X) & ~(pagesize - 1))
#define PROUND(X) (((X) + pagesize - 1) & ~(pagesize - 1))

// Print process memory map.
// If core file, just use segment list; it doesn't change.
// If live process, ask /proc for updated list and use seglist
// to get corresponding pathnames.
int
Seglist::print_map(Procctl *pctl)
{
	Segment 	*seg;
	const char	*path;
	int		num;
	map_ctl		*map;

	if (!pctl)
		return 0;
	if (pctl->get_type() == pt_core)
	{
		DPRINT(DBG_SEG, ("Seglist::print_map - core file\n"));
		for (seg = first_segment; seg; seg = seg->next())
		{
			if (seg == stack_segment)
				path = "[STACK]";
			else if (!seg->symnode)
				path = "";
			else
				path = seg->symnode->pathname;
			DPRINT(DBG_SEG, ("Seglist::print_map, loaddr %#x hiaddr %#x, path %s\n", seg->loaddr, seg->hiaddr, path));
			printm(MSG_map, (Word) seg->loaddr, (Word) seg->hiaddr - 1,
				(Word) seg->hiaddr - seg->loaddr,
				modes[SEG_PROT_MASK(seg->prot_flags)], 
				path);
		}
		return 1;
	}
	DPRINT(DBG_SEG, ("Seglist::print_map - live object\n"));
	if (!pagesize)
		pagesize = sysconf(_SC_PAGESIZE);

	if ((map = ((Proclive *)pctl)->seg_map(num)) == 0)
		return 0;

	for(int i = 0; i < num; i++, map++)
	{
		int	pflags;
		Iaddr	lo, hi;

		lo = (Iaddr)map->pr_vaddr;
		hi = (Iaddr)map->pr_vaddr + map->pr_size;
		pflags = 0;
		if ((map->pr_mflags & MA_EXEC) != 0)
			pflags |= SEG_EXEC;
		if ((map->pr_mflags & MA_WRITE) != 0)
			pflags |= SEG_WRITE;
		if ((map->pr_mflags & MA_READ) != 0)
			pflags |= SEG_READ;
		for(seg = first_segment; seg; seg = seg->next())
		{
			Iaddr	lotrunc = PTRUNC(seg->loaddr);
			Iaddr	hiround = PROUND(seg->hiaddr);

			if (lo >= lotrunc && lo < hiround && 
				hi <= hiround)
			{
				break;
			}
		}
		if (seg == stack_segment)
			path = "[STACK]";
		else if (!seg || !seg->symnode)
			path = "";
		else
			path = seg->symnode->pathname;
		printm(MSG_map, (Word)lo, (Word)hi - 1, (Word)map->pr_size,
			modes[pflags], path);
	}
	return 1;
}

// find core file segment corresponding to entry from
// program header and add its symnode
void
Seglist::add_name(Symnode *sym, Iaddr addr, long size)
{
	Segment	*seg = first_segment;
	Iaddr	hi, lotrunc, hiround;

	hi = addr + size;

	if (!pagesize)
		pagesize = sysconf(_SC_PAGESIZE);

	for(; seg; seg = seg->next())
	{
		lotrunc = PTRUNC(seg->loaddr);
		hiround = PROUND(seg->hiaddr);

		if (addr >= lotrunc && addr < hiround && 
			hi <= hiround)
		{
			seg->symnode = sym;
			return;
		}
	}
}

// add a segment to end of list
void
Seglist::add_segment(Segment *nseg)
{
	if (last_segment)
		nseg->append(last_segment);
	else
		first_segment = nseg;
	last_segment = nseg;
}
