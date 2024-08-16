#ident	"@(#)debugger:libexecon/i386/Seg_mach.C	1.9"

#include "Seglist.h"
#include "Segment.h"
#include "Iaddr.h"
#include "Process.h"
#include "Interface.h"
#include "Object.h"
#include "ELF.h"
#include "Dyn_info.h"
#include "Proctypes.h"
#include "Procctl.h"
#include "Rtl_data.h"
#include <unistd.h>
#include <elf.h>
#include <sys/auxv.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <link.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

// machine dependent Seglist routines

Rtl_data::Rtl_data(const char *interp)
{
	DPRINT(DBG_SEG, ("Rtl_data::Rtl_data(this == %#x, interp == %s)\n", this, interp));
	r_debug_addr = rtld_addr = 0;
	ld_so = new(char[strlen(interp) + 1]);
	strcpy((char *)ld_so, interp);
}

// Find and read r_debug structure in dynamic linker - assumes live
// process.  Cannot use DT_DEBUG entry since we are stopped
// on process startup and dynamic linker hasn't had a chance to
// set the correct value yet.
int
Rtl_data::find_r_debug( Process *proc, Proclive *pctl )
{
	int		fd;  
	ELF		*obj;
	Iaddr		addr;

	DPRINT(DBG_SEG, ("Rtl_data::find_r_debug(this == %#x, proc == %#x, pctl == %#x)\n", this, proc, pctl));
	if (!r_debug_addr)
	{
		if ((fd = open( ld_so, O_RDONLY )) < 0)
			return 0;
		if (((obj = (ELF *)find_object(fd, 0)) == 0) ||
			( obj->file_format() != ff_elf ) ||
			( obj->find_symbol( "_r_debug", addr ) == 0 ) ||
			((rtld_addr = rtld_base(proc, pctl)) == 0))
		{
			close(fd);
			return 0;
		}
		r_debug_addr = addr + rtld_addr;
		close(fd);
	}
	// Must always reread to get up to date contents
	if (pctl->read( r_debug_addr, (char *)&rdebug, sizeof(r_debug))
		!= sizeof(r_debug))
	{
		r_debug_addr = 0;
		return 0;
	}
	return 1;
}

// Find link_map using address from process dynamic section.
// Used for core files.
int
Rtl_data::find_link_map( int fd, const char *exec_name, Seglist *slist,
	Iaddr &laddr, Process *proc)
{
	Object		*obj;
	Iaddr		addr;
	Segment		*seg;
	Elf_Dyn 	dyn;
	r_debug 	debug;
	Sectinfo	sinfo;

	// get addr of _DYNAMIC
	if ((obj = find_object(fd, exec_name)) == 0)
		return 0;

	if (!obj->getsect(s_dynamic, &sinfo))
	{
		return 0;	// not dynamically linked
	}
	addr = sinfo.vaddr;

	// walk list, find DT_DEBUG entry

	if ( (seg = slist->find_segment(addr)) == 0 ) 
	{
		printe(ERR_no_segment, E_ERROR, addr, proc->obj_name());
		return 0;
	}
	do 
	{
		if ( seg->read( addr, &dyn, sizeof dyn ) != sizeof dyn ) 
		{
			printe(ERR_proc_read, E_ERROR,
				proc->obj_name(), addr);
			return 0;
		}
		if ( dyn.d_tag == DT_DEBUG ) 
		{
			break;
		}
		addr += sizeof dyn;
	} while ( dyn.d_tag != DT_NULL );

	// get r_debug struct
	if ((addr = dyn.d_un.d_val) == 0)
	{
		return 0;
	}
	if ( (seg = slist->find_segment(addr)) == 0 ) 
	{
		printe(ERR_no_segment, E_ERROR, addr, proc->obj_name());
		return 0;
	}
	if ( seg->read( addr, &debug, sizeof debug ) != sizeof debug ) 
	{
		printe(ERR_proc_read, E_ERROR, proc->obj_name(), addr);
		return 0;
	}
	laddr = (Iaddr)debug.r_map;
	return 1;
}

// Find base address of dynamic linker.  For processes
// just created, we look at auxv entries on stack.
// Initial stack looks like this:
// ----------------------------------------
// |	unspecified	|   High Addresses |
// ----------------------------------------
// |	argument, 	|		   |
// |    environment,    |		   |
// |	etc. strings	|		   |
// ----------------------------------------
// |    null auxv entry |		   |
// ----------------------------------------
// |	Auxv entries	|		   |
// |    2 words each    |		   |
// ----------------------------------------
// |	0 word		|		   |
// ----------------------------------------
// |	Environment ptrs|		   | 
// ----------------------------------------
// |	0 word		|		   |
// ----------------------------------------
// |	Argument ptrs   |   4(%esp)	   |
// ----------------------------------------
// |	Argument count  |   0(%esp)	   |
// ----------------------------------------
// |	unspecified	|   Low Addresses  |
// ----------------------------------------
//
// We can't do that for forked or grabbed processes,
// since we don't know where on the stack we are.
// Instead we use the /proc memory map, get file descriptors
// for each read-only segment, and compare their device
// and inode with that of ld.so

Iaddr
Rtl_data::rtld_base(Process *proc, Proclive *pctl)
{
	DPRINT(DBG_SEG, ("Rtl_data::rtld_base(this == %#x, proc == %#x, pctl == %#x)\n", this, proc, pctl));

	if ( rtld_addr != 0 )
		return rtld_addr;

	if (proc->is_grabbed())
	{
		struct	stat	ldso_buf;
		int		num_segs;
		map_ctl		*map;

		DPRINT(DBG_SEG, ("Rtl_data::rtld_base - grabbed proc\n"));
		if (!pctl || stat(ld_so, &ldso_buf) == -1)
			return 0;
		DPRINT(DBG_SEG, ("rtld_base: %s dev %d, ino %d\n", ld_so, ldso_buf.st_dev, ldso_buf.st_ino));

		if ((map = pctl->seg_map(num_segs)) == 0)
			return 0;
		for (int i = 0; i < num_segs; i++, map++)
		{
			int		fd;
			struct stat	mem_buf;
			if ((map->pr_mflags &
				(MA_READ|MA_WRITE|MA_EXEC)) ==
					(MA_READ|MA_EXEC))
			{
				// check only text segments
				if ((fd = pctl->open_object((
					Iaddr)map->pr_vaddr,
					0)) == -1)
					continue;
				if (fstat(fd, &mem_buf) == -1)
				{
					close(fd);
					continue;
				}
				close(fd);

				DPRINT(DBG_SEG, ("rtld_base: segment at %#x dev %d, ino %d\n", map->pr_vaddr, mem_buf.st_dev, mem_buf.st_ino));
				if ((mem_buf.st_dev == ldso_buf.st_dev)
					&& (mem_buf.st_ino ==
						ldso_buf.st_ino))
					break;
			}
		}
		if (i >= num_segs )
		{
			return 0;
		}
		return (Iaddr)map->pr_vaddr;
	}

	// created process - right after startup
	Iaddr base = 0;
	Iaddr argp = proc->getreg(REG_ESP);

	long argc;
	if ( pctl->read( argp, &argc, sizeof(long) ) 
		!= sizeof(long) ) 
	{
		printe(ERR_proc_read, E_ERROR, proc->obj_name(), argp);
		return 0;
	}
	DPRINT(DBG_SEG, ("Rtl_data::find_r_debug - argp == %#x, argc == %d\n", argp, argc));

	argp += sizeof(long) * (1 + argc + 1); // skip argc, argv

	// argp now points at beginning of envp array (null terminated) 

	long envp;
	// find NULL at end of envp array
	do {		
		if ( pctl->read( argp, &envp, sizeof(long) ) 
			!= sizeof(long) ) 
		{
			printe(ERR_proc_read, E_ERROR, 
				proc->obj_name(), argp);
			return 0;
		}
		argp += sizeof(long);
	} while ( envp != 0 );

	DPRINT(DBG_SEG, ("Rtl_data::find_r_debug - auxv == %#x\n", argp));
	auxv_t auxv;
	// read auxv entries until find base addr
	do {
		if ( pctl->read( argp, &auxv, sizeof auxv ) !=
			sizeof auxv ) 
		{
			printe(ERR_proc_read, E_ERROR, 
				proc->obj_name(), argp);
			return 0;
		}
		if ( auxv.a_type == AT_BASE ) 
		{
			base = auxv.a_un.a_val;
			break;
		}
		argp += sizeof auxv;
	} while ( auxv.a_type != AT_NULL );

	rtld_addr = base;
	DPRINT(DBG_SEG, ("Rtl_data::find_r_debug - base == %#x\n", base));
	return base;
}

// Find address of dynamic linker function that brackets
// addition or deletion of libraries. debug sets a breakpoint
// on this function to keep address space up to date.
Iaddr
Seglist::rtl_addr( Proclive *pctl )
{
	DPRINT(DBG_SEG, ("Seglist::rtl_addr(this == %#x, pctl == %#x)\n", this , pctl));
	if (!rtl_data || !rtl_data->find_r_debug(proc, pctl))
	{
		return 0;
	}
	else
	{
		if (rtl_data->rdebug.r_brk > rtl_data->rtld_addr)
			// already relocated
			return rtl_data->rdebug.r_brk;
		else
			return rtl_data->rdebug.r_brk + rtl_data->rtld_addr;
	}
}

// Add segments for shared library text and data.  We must
// go through link_map and segment list and keep the 2 in sync,
// since shared libraries may come and go.
// We assume all segments for a given library are contiguous.
// Live processes only.
int
Seglist::build_dynamic( Proclive *pctl )
{
	Segment 	*seg;
	Segment 	*seg2;
	long		offset;
	link_map	map_node;
	int		found;
	char		buf[PATH_MAX+1];
	int		fd;

	DPRINT(DBG_SEG, ("Seglist::build_dynamic(this == %#x, pctl == %#x)\n", this , pctl));
	if (!rtl_data || !rtl_data->find_r_debug( proc, pctl ))
		return 1;
	buf[PATH_MAX] = '\0';
	// skip segments for a.out
	seg = first_segment;
	while ( (seg != 0 ) && (seg->symnode != 0) &&
		(seg->symnode->sym.ss_base == 0) )
	{
		seg = seg->next();
	}
	offset = (long)rtl_data->rdebug.r_map;
	// traverse the link_map to look for dynamic shared objects
	while ( offset != 0 )
	{
		if ( pctl->read( offset, &map_node, sizeof(link_map) ) 
			!= sizeof(link_map) )
		{
			printe(ERR_proc_read, E_ERROR, 
				proc->obj_name(), offset);
			return 0;
		}
		if ( map_node.l_name == 0 )
		{
			// l_name == 0 for the a.out
			offset = (long)map_node.l_next;
			continue;
		}
		else if ( pctl->read( (long)map_node.l_name, 
			buf, PATH_MAX ) <= 0 )
		{
			printe(ERR_proc_read, E_ERROR, 
				proc->obj_name(), offset);
			return 0;
		}
		// Delete all shared library segments until we find
		// one that applies to this library.  If we 
		// have such segments, it means that something
		// is missing from the link_map, so a library
		// was deleted.
		while ( (seg != 0) && (seg->symnode != 0) &&
			(strcmp( seg->symnode->pathname, buf )
			!= 0) )
		{
			seg2 = seg;
			seg = seg->next();
			if (seg2 == first_segment)
				first_segment = seg;
			if (seg2 == last_segment)
				last_segment = (Segment *)seg2->prev();
			seg2->unlink();
			delete seg2;
		}
		found = 0;
		while ( (seg != 0) && (seg->symnode != 0) &&
			!strcmp( seg->symnode->pathname, buf ) )
		{
			found = 1;
			seg = seg->next();
		}
		if ( !found )
		{
			if ((fd = open( buf, O_RDONLY )) < 0)
				return 0;
			add( fd, (Procctl *)pctl, buf, 0, 
				map_node.l_addr);
			close(fd);
		}
		offset = (long) map_node.l_next;
	}
	// delete left over segments
	while ( seg != 0 )
	{
		seg2 = seg;
		seg = seg->next();
		if (seg2 == first_segment)
			first_segment = seg;
		if (seg2 == last_segment)
			last_segment = (Segment *)seg2->prev();
		seg2->unlink();
		delete seg2;
	}
	return 1;
}

// Is dynamic linker actively adding or removing a library?
int
Seglist::buildable( Proclive *pctl )
{
	if (!rtl_data || !rtl_data->find_r_debug(proc, pctl))
	{
	DPRINT(DBG_SEG, ("Seglist::buildable(this == %#x, pctl == %#x) -not dynamic\n", this , pctl));
		return 0;
	}
	DPRINT(DBG_SEG, ("Seglist::buildable(this == %#x, pctl == %#x) rdebug.r_state == %d\n", this , pctl, rtl_data->rdebug.r_state));
	return (rtl_data->rdebug.r_state == 0); // really RT_CONSISTENT
}

// is addr in one of the text segments
int
Seglist::in_text( Iaddr addr )
{
	Segment *	seg;

	if ( addr == 0 )
		return 0;
	for ( seg = first_segment; seg != 0 ; seg = seg->next() ) 
	{
		if ( ((seg->prot_flags & (SEG_WRITE|SEG_EXEC)) ==
			SEG_EXEC) && 
			(addr >= seg->loaddr) && 
			( addr <= seg->hiaddr) )
			return 1;
	}
	return 0;
}

// Add shared library text segments.  Used for core files.
int
Seglist::add_dynamic_text( Procctl *textctl, 
	const char *executable_name )
{
	Iaddr	addr;
	Segment	*seg;
	Procctl	*pctl;

	// get symtabs for dynamic libraries
	
	if (!rtl_data ||
		!rtl_data->find_link_map(textctl->get_fd(), 
			executable_name, this, addr, proc))
		return 1;

	// for each map entry
	link_map map;
	while (addr) 
	{
		if ( (seg = find_segment(addr)) == 0 ) 
		{
			printe(ERR_no_segment, E_ERROR, addr, 
				proc->obj_name());
			return 0;
		}
		if ( seg->read( addr, &map, sizeof map ) != sizeof map ) 
		{
			printe(ERR_proc_read, E_ERROR, 
				proc->obj_name(), addr);
			return 0;
		}
		addr = (Iaddr)map.l_next;
		Iaddr nameaddr = (Iaddr)map.l_name;
		char name[PATH_MAX];
		if ( !nameaddr ) 
			continue;
		if ( (seg = find_segment(nameaddr)) == 0 ) 
		{
			printe(ERR_no_segment, E_ERROR, 
				nameaddr, proc->obj_name());
			return 0;
		}
		if (seg->read( nameaddr, name, sizeof name) 
			<= 0 ) 
		{
			printe(ERR_proc_read, E_ERROR, 
				proc->obj_name(), nameaddr);
			return 0;
		}
		int	fd;
		if ((fd = open ( name, O_RDONLY )) < 0)
		{
			printe(ERR_shlib_open, E_ERROR, name,
				strerror(errno));
			return 0;
		}
		pctl = new Procctl();
		if (!pctl->open(fd))
			return 0;
		add( fd, pctl, name, 1, map.l_addr );
		close(fd);
	}
	return 1;
}

// setup machine specific dynamic information for segment
void
Seglist::add_dyn_info(Symnode *sym, Object *obj)
{
	Sectinfo	pltinfo;
	Sectinfo	gotinfo;
	Dyn_info	*dyn;

	if (!obj->getsect(s_plt, &pltinfo) ||
		!obj->getsect(s_got, &gotinfo))
		return;
	dyn = new Dyn_info;
	dyn->pltaddr = pltinfo.vaddr + sym->sym.ss_base;
	dyn->pltsize = pltinfo.size;
	dyn->gotaddr = gotinfo.vaddr + sym->sym.ss_base;
	sym->dyn_info = dyn;
}
