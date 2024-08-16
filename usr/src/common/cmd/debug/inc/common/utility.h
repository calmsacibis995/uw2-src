/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef utility_h
#define utility_h
#ident	"@(#)debugger:inc/common/utility.h	1.18"


#include "Iaddr.h"
#include "Parser.h"
#include "TSClist.h"
#include "Proctypes.h"
#include "Severity.h"
#include "Symbol.h"
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

/* modes passed to symbols */
#define	SYM_LOCALS	0x1
#define	SYM_FILES	0x2
#define	SYM_GLOBALS	0x4
#define	SYM_BUILT_IN	0x10
#define SYM_USER	0x20
#define	SYM_VALUES	0x40
#define	SYM_TYPES	0x80

/* modes passed to create */
#define	DEFAULT_IO	0
#define	DIRECT_IO	1
#define	REDIR_IO	2

/* type of symbol calling function expects to get back from */
/* find_symbol or get_addr - needed for type checking */
enum Symtype {
	st_notags, 	// only ids and typedefs - no tags
	st_tagnames, 	// only tag names (struct, union, enum, class)
	st_usertypes,	// tags and typedefs
	st_func, 	// functions, entry points and labels
	st_object,	// variables and parameters
	st_any,		// anything matches
};

class	ProcObj;
class	Process;
class	Frame;
class	Program;
class	IntList;
class	Rvalue;
struct	Location;
class	Exp;
class	Node;
class	Symbol;
class	Vector;
struct	Proclist;
struct	Filelist;
struct	StopEvent;

extern int	create_process(const char *, int redir, int follow,
			int on_exec, Location *);
extern int	input_pty( const char *proc, const char *pty, 
			char *input, int nonewline );
extern int	grab_process( Filelist *, char *, int follow );
extern int 	grab_core( char * adotout, char * core );
extern int	dispose_process( ProcObj * );
extern int	proc_status( Proclist * );
extern void	destroy_all();
extern void	db_exit( int );
extern void	parse_ps_args(char *);
extern int	inform_processes(int, siginfo_t *, ucontext_t *);
extern int	rename_prog(const char *, const char *);

extern int	symbols(Proclist *, const char *, const char *, 
			const char *, int);
extern int	functions(const char *, const char *, int);

extern int	run( Proclist *, int, Location *, int );
extern int	stop_process(Proclist *);
extern void	wait_process();

extern int	single_step( Proclist *, int, int, int, int, int, int );
extern int	resume( Proclist * );

extern int	print_map( Proclist * );
extern int	printregs( Proclist * );
extern int	set_val( Proclist *, char *, Exp *, int verbose );
extern int	print_expr( Proclist *, char *, Exp *, int verbose, int brief );
extern int	print_special(dbg_vtype, ProcObj *, char *);
extern int	whatis(Proclist *, const char *);

extern char	*set_path( const char *, const char * );
extern int	set_curr_func( ProcObj *, const char * );
extern int	set_frame( ProcObj *, int );
extern int	count_frames( ProcObj *);
extern int	curr_frame( ProcObj * );
extern int	cancel_sig( Proclist *, IntList * );
extern int	send_signal( Proclist *, int );

extern int	dump_raw(Proclist *, Location *, int );
extern int	disassem_cnt(Proclist *, Location *, unsigned int cnt,
			int func);

extern int	print_source( ProcObj *, Location );

extern int 	list_src(Proclist *, int cnt, Location *, const char *, int direction);

extern long	first_exec_line( ProcObj * );
extern int	jump(Proclist *, Location *);
extern int	set_jump_addr(ProcObj *, Iaddr);
extern int	suffix_path( char * );
extern int	pc_current_src( ProcObj * );
extern int	get_curr_pc( ProcObj * , Iaddr * );

extern int 	current_loc( ProcObj *, Frame *, char *&file, char *&func, 
			long &line);

#ifndef __cplusplus
overload destroy_process;
overload get_addr;
#endif
extern int	destroy_process(Proclist *, int announce = 1);
extern int	destroy_process(Process *, int announce = 1);

extern int	print_stack(Proclist *, int how_many, int first, 
			Iaddr pc, Iaddr sp);
extern int	rval_true(Rvalue &, int &);
extern int	set_stop( Proclist *, StopEvent *, Node *, int, int);
extern int	set_onstop( Proclist *, Node *);
extern int	set_syscall( Proclist *, IntList *, Systype, Node *, int, int);
extern int	set_signal( Proclist *, sigset_t, Node *, int, int);
extern int	change_event( int event, Proclist *, int count, Systype,
			int set_quiet, void *event_expr, Node *cmd);
extern Symbol	find_symbol(const char *, const char *, ProcObj *, Iaddr, Symtype);
extern int	resolve_overloading(ProcObj *, const char *func, Symbol &, Vector **);

extern int	get_addr( ProcObj *&, Location *, Iaddr &, Symtype, 
			Symbol &, Severity msg = E_ERROR, 
			Vector **v = 0 );
extern int	get_addr(ProcObj *&, Location *, Iaddr &, Symbol &, const char *&,
			Severity msg, Vector **v = 0);
extern char	*find_fcn(ProcObj *, char *, char *, long &);
extern int	release_process( ProcObj *, int);
extern int	release_proclist( Proclist *, int);

// gui only commands
extern int	pending_sigs( Proclist *);
extern int	print_path( Proclist *, const char * );
extern int	print_files( Proclist * );

extern void	new_handler();
extern void	debugtty();
extern void	restore_tty();

#ifdef __cplusplus
extern "C" {
#endif
extern int	debug_open(const char *path, int oflag, mode_t = 0666);
extern int	debug_dup(int filedes);
extern FILE	*debug_fopen(const char *path, const char *type);
#ifdef __cplusplus
}
#endif

extern int	ksh_fc(char *);

// information returned in vector from get_addr for overloaded functions
struct Overload_data
{
	Symbol	function;
	Iaddr	address;
	char	*expression;

		Overload_data() { address = 0; expression = 0; }
};
	
#endif

/* end of utility.h */
