/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Seglist_h
#define Seglist_h
#ident	"@(#)debugger:libexecon/common/Seglist.h	1.10"

#include "Iaddr.h"
#include "Symbol.h"

class NameEntry;
class Object;
class Procctl;
class Proccore;
class Proclive;
class Process;
class Segment;
class Symnode;
class Symtab;
struct Dyn_info;

// Each Seglist is shared among all the threads in a process.
// For each Seglist we maintain two lists: a list of address ranges
// (segments) and a list of objects with their associated symbol
// tables (symnodes).  Many segments may be associated with a
// single symnode.

struct Rtl_data;	// opaque to clients

class Seglist {
	char		static_loaded;
	char		uses_rtld;
	short		_has_stsl;
	Process		*proc;
	Segment		*mru_segment;  // most recently used
	Symbol		current_file;
	Symnode		*symnode_file;
	Symnode		*symnode_global;
	Rtl_data	*rtl_data;
	Iaddr		start;
	Symnode		*symlist;
	Segment		*first_segment;
	Segment		*last_segment;
	Segment		*stack_segment;
	Symnode		*add( int fd, Procctl *, const char *path, 
				int text_only, Iaddr base );
	int		add_static_shlib( Object *, Procctl *, int text_only );
	int		add_dynamic_text( Procctl *, const char *exec_name );

	Symnode		*add_symnode( Object *, const char * path, 
				Iaddr base );
	void		add_dyn_info(Symnode *, Object *);
	int		build_dynamic( Proclive * );
	int		build_static( Proclive *, const char *exec_name );
	int		get_brtbl( const char *objname );
	void 		add_name(Symnode *, Iaddr addr, long size);
	void 		add_segment(Segment *);
public:
			Seglist( Process *);
			~Seglist();
	int		setup( int fd, const char *exec_name);
	int		buildable( Proclive * );
	int		build( Proclive *, const char *exec_name );
	Iaddr		rtl_addr( Proclive * );
	int		readproto( Procctl *txt, Proccore *,
				const char *exec_name);
	Symtab		*find_symtab( Iaddr );
	Symtab		*find_symtab( const char * objname);
	const char	*object_name( Iaddr );
	Segment		*find_segment( Iaddr );
	int		find_source( const char * , Symbol & );
	int		find_next_global(const char *, Symbol &);
	Symbol		first_file();
	Symbol		next_file();
	Symbol		find_global( const char * );
	int		print_map(Procctl *);
	int		has_stsl() { return _has_stsl; }
	int		in_stack( Iaddr );
	Iaddr		end_stack();
	int		in_text( Iaddr );
	void		update_stack( Proclive * );
	Iaddr		start_addr() { return start; };
	Dyn_info	*get_dyn_info(Iaddr pc);
};

#endif

// end of Seglist.h
