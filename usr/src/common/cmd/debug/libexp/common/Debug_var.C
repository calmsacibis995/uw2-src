/*
 * $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)debugger:libexp/common/Debug_var.C	1.16"

#include "Interface.h"
#include "Itype.h"
#include "str.h"
#include "Debug_var.h"
#include "TYPE.h"
#include "Frame.h"
#include "Reg.h"
#include "Language.h"
#include "ProcObj.h"
#include "Process.h"
#include "Thread.h" // %thread_change
#include "Parser.h"
#include "utility.h" // %line, %file, %func
#include "global.h" // %num_lines, %num_bytes, %global_path, %follow
#include "Event.h" // %thisevent, %lastevent
#include "Proglist.h" // %program, %proc, %thread
#include "edit.h" // %mode
#include "Input.h" // %prompt
#include "Program.h"
#include "Vector.h"
#include <string.h>


#ifndef __cplusplus
// C++ 1.2 does not support pure virtual functions
debug_var_class Debug_var::var_class() { return null_var; };
int Debug_var::read_value(void * , int ) {return 0;};
int Debug_var::write_value(void * , int ) {return 0;};
#endif

#ifdef DEBUG
int debugflag = 0;
#endif

Debug_var_table debug_var_table;

Fund_type
Debug_var::fund_type(void)
{
	return ft_int;
}

int 
Debug_var::size() 
{
	return TYPE(fund_type()).size();
}

int
Debug_var::isnull() 
{
	return frame == 0;
}

//
// Debugger variable flavors:
//

class Debug_var_register: public Debug_var
{
private:
	RegRef the_reg;
public:
	Debug_var_register(RegRef reg) {the_reg = reg;};
	Fund_type fund_type(void) {return regtype(the_reg);};
	// size: use the default

	debug_var_class var_class() {return reg_var;};
	int read_value(void * value, int byte_count);  // see below
	int write_value(void * value, int byte_count); // see below
};

int 
Debug_var_register::read_value(void * value, int )
{
	Stype stype;
	TYPE(fund_type()).get_Stype(stype);
	Itype * iptr = (Itype *)value;
	if (frame->readreg(the_reg, stype, *iptr) == 0)
	{
		printe(ERR_read_reg, E_ERROR, pobj->obj_name());
		return 0;
	}
	return 1;
};

int 
Debug_var_register::write_value(void * value, int )
{
	Stype stype;
	TYPE(fund_type()).get_Stype(stype);
	if (frame->writereg(the_reg, stype, *(Itype*)value) == 0)
	{
		printe(ERR_write_reg, E_ERROR, pobj->obj_name());
		return 0;
	}
	return 1;
};

enum dbg_vmode {
	dv_rdwr,	// read/write
	dv_rdonly,	// read only
	dv_rdonly_gui,  // read only for gui, else read/write
};

class Debug_var_debug: public Debug_var
{
private:
	dbg_vtype which_var;
	dbg_vmode prot_mode;
	char * value(int report_error); 
		// int values are returned as the pointer itself
public:
	Debug_var_debug(dbg_vtype var, dbg_vmode mode) 
		{which_var = var; prot_mode = mode; }

        // See below.
	Fund_type fund_type(void);
	int       size(void);
	int       isnull(void);
	debug_var_class var_class() {return debug_var;};
	int       read_value(void * value, int byte_count);
	int       write_value(void * value, int byte_count);
};

Fund_type 
Debug_var_debug::fund_type(void)
{
	switch (which_var)
	{
	case Loc_v:
		return ft_pointer;
#ifdef DEBUG
	case Debug_v: 
#endif
	case Result_v:
		return ft_int;
	case List_line_v:
	case Line_v:
		return ft_long;
	case Lastevent_v:
	case Num_bytes_v:
	case Num_lines_v:
	case Frame_v:
	case Thisevent_v:
		return ft_uint;
	case Db_lang_v:
	case File_v:
	case Follow_v:
	case Func_v:
	case Glob_path_v:
	case Lang_v:
	case List_file_v:
	case Mode_v:
	case Path_v:
	case Proc_v:
	case Program_v:
	case Prompt_v:
	case Redir_v:
	case Verbose_v:
	case Wait_v:
		return ft_string;
	case Thread_v:
	case Thread_change_v:
#ifdef DEBUG_THREADS
		return ft_string;
#else
		return ft_none;
#endif
	case NoType_v:
	default:
		return ft_none;
	}
}

int 
Debug_var_debug::isnull(void)
{
	switch (which_var)
	{
#ifdef DEBUG
	case Debug_v:
#endif
	case Db_lang_v:
	case Follow_v:
	case Glob_path_v:
	case Lang_v:
	case Lastevent_v:
	case Mode_v:
	case Num_bytes_v:
	case Num_lines_v:
	case Result_v:
	case Prompt_v:
	case Redir_v:
	case Thisevent_v:
	case Verbose_v:
	case Wait_v:
		return 0;
	case Thread_change_v:
#ifdef DEBUG_THREADS
		return 0;
#else
		return 1;
#endif
	case List_file_v:
	case List_line_v:
	case Path_v:
	case Proc_v:
	case Program_v:
		return pobj == 0;
	case Thread_v:
#ifdef DEBUG_THREADS
		return pobj == 0;
#else
		return 1;
#endif
	case File_v:
	case Func_v:
	case Line_v:
	case Frame_v:
	case Loc_v:
	{
		if (pobj == 0 || frame == 0)
			return 1;
		Execstate state = pobj->get_state();
		return state == es_running
		|| state == es_stepping
		|| state == es_none
		|| state == es_dead;
	}
	case NoType_v:
	default:
		return 1;
	}
}

int 
Debug_var_debug::size(void)
{
	switch (fund_type())
	{
	case ft_int:
	case ft_uint:
	case ft_long:
	case ft_pointer:
		return Debug_var::size();
	case ft_string:
		{
		char * s = value(0);
		return strlen(s)+1;
		}
	default:
		return 0;
	}
}

static const char * yes_no_table[]  = {"no", "yes", 0};
static const char * wait_table[]    = {"background", "foreground", 0};

static const char * follow_table[]  = {"none", "procs", "all", 0};

static const char * verbose_table[] = {"quiet", "source", "events", 
					"reason", "all", 0};
static const char * table_to_5[]    = {"0", "1", "2", "3", "4", "5", 0};
// "5" is available only for internal use = high verbosity

#ifdef DEBUG_THREADS
static const char * tchange_table[] = {"ignore", "announce", "stop", 0 };
#endif

inline const char *
debug_enum_string(int value, const char * table[])
{
	// assumes value is in range
	return table[value];
}

static int
debug_enum_value(const char * string, const char * table[])
{
	int entry = 0;
	while (table[entry])
	{
		if (strcmp(table[entry], string) == 0) 
			return entry;
		entry++;
	}
	return -1;
}

inline
char *
null_check(char * s)
{
	if (s) 
		return s;
	else
		return "";
}

char * // int values are returned as the pointer itself
Debug_var_debug::value(int report_error)
{
	char *fname, *filename;
	long line;
	switch(which_var)
	{
#ifdef DEBUG
	case Debug_v:
		return (char *) debugflag;
#endif
	case Db_lang_v:	
		return null_check((char *)language_name(
			current_context_language(pobj)));
	case Func_v:	
		if (!pobj)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return "";
		}
		current_loc(pobj, pobj->curframe(), filename, fname, line);
		return null_check(fname);
	case File_v:	
		if (!pobj)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return "";
		}
		current_loc(pobj, pobj->curframe(), filename, fname, line);
		return null_check(filename);
	case Follow_v:
		return (char *)debug_enum_string(follow_mode, follow_table);
	case Glob_path_v: 
		return null_check(global_path);
	case Lang_v:      
		return null_check((char *)language_name(
			current_user_language()));
	case Lastevent_v:	
		return (char *)m_event.last_event();
	case List_file_v:	
		if (!pobj)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return "";
		}
		return null_check(pobj->curr_src());
	case Line_v:
		if (!pobj)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		current_loc(pobj, pobj->curframe(), filename, fname, line);
		return (char *)line;
	case List_line_v:
		if (!pobj)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		line = pobj->current_line();
		return (char *)line;
#ifdef DEBUG_THREADS
	case Thread_v:
	{
		Thread	*t = proglist.current_thread();
		if (!t)
		{
			if (report_error) 
			{
				if (!proglist.current_process())
					printe(ERR_no_proc, E_WARNING);
			}
			return "";
		}
		else 
			return null_check((char *)t->obj_name());
	}
#endif
	case Mode_v:	
		return null_check((char *)get_mode());
	case Num_bytes_v:
		return (char *)num_bytes;
	case Num_lines_v:
		return (char *)num_line;
	case Path_v:
		{
			if (!pobj)
			{
				if (report_error) printe(ERR_no_proc, E_ERROR);
				return "";
			}
			return null_check((char *)pobj->program()->src_path());
		}
	case Frame_v:	
		if (!pobj)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		return (char *)curr_frame(pobj);
	case Loc_v:	
		if (!pobj || pobj->get_state() == es_dead)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return "";
		}
		return (char *)pobj->curframe()->pc_value();
	case Program_v:
	{
		Program	*p = proglist.current_program();
		if (!p)
		{
			if (report_error) printe(ERR_no_proc, E_WARNING);
			return "";
		}
		else 
			return null_check((char *)p->prog_name());
	}
	case Redir_v:	
		return (char *)debug_enum_string(redir_io, yes_no_table);
	case Proc_v:
	{
		Process	*p = proglist.current_process();
		if (!p)
		{
			if (report_error) printe(ERR_no_proc, E_WARNING);
			return "";
		}
		else 
			return null_check((char *)p->obj_name());
	}
	case Prompt_v:	
		return (char *)Pprompt;
	case Result_v:
		return (char *)cmd_result;
	case Thisevent_v:
		return (char *)m_event.this_event();
#ifdef DEBUG_THREADS
	case Thread_change_v:	
		return (char *)debug_enum_string(thr_change, tchange_table);
#endif
	case Verbose_v:	
		return (char *)debug_enum_string(vmode, verbose_table);
	case Wait_v:	
		return (char *)debug_enum_string(wait_4_proc, wait_table);
	default:	
		return "";
	}
}

int       
Debug_var_debug::read_value(void * value, int byte_count)
{
	char * s = this->value(1);
	switch (fund_type())
	{
	case ft_int:
	case ft_uint:
	case ft_long:
	case ft_pointer:
		memcpy(value, &s, byte_count);
		return byte_count;
	case ft_string:
		{
		memcpy(value, s, byte_count);
		return byte_count;
		}
	default:
		return 0;
	}
}

static Vector str_vector;

// values are not null terminated

inline static char *
string_value(void * value, int byte_count)
{
	char	zero = 0;
	str_vector.clear();
	str_vector.add(value, byte_count);
	str_vector.add((void*)&zero, 1);
	return((char *)str_vector.ptr());
}

int       
Debug_var_debug::write_value(void * value, int byte_count)
{
// The string is not null-terminated.
	char	*name;
#define CVALUE string_value(value, byte_count)
#define IVALUE *(int *)value
	if ((prot_mode == dv_rdonly) ||
		((get_ui_type() == ui_gui) && 
		(prot_mode == dv_rdonly_gui)))
	{
		printe(ERR_debug_var_set, E_ERROR,
			debug_var_table.Lookup(this));
		return 0;
	}
	switch(which_var) {
#ifdef DEBUG
	case Debug_v:
		debugflag = IVALUE;
		break;
#endif
	case Follow_v:
		{
		int enum_value;
		enum_value = debug_enum_value(CVALUE, follow_table);
		if (enum_value < 0)
			printe(ERR_debug_var_val, E_ERROR, "%follow");
		else
			follow_mode = enum_value;
		break;
		}
	case Func_v:
		if (!pobj)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		if (pobj->get_state() == es_running
		|| pobj->get_state() == es_stepping)
		{
			printe(ERR_invalid_op_running, E_ERROR, pobj->obj_name());
			return 0;
		}
		set_curr_func(pobj, CVALUE);
		break;
	case Frame_v:
		if (!pobj)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		if (pobj->get_state() == es_running
		|| pobj->get_state() == es_stepping)
		{
			printe(ERR_invalid_op_running, E_ERROR, pobj->obj_name());
			return 0;
		}
		set_frame(pobj, IVALUE);
		break;
	case Lang_v:
		set_language(CVALUE);
		break;
	case Mode_v: 
		(void)set_mode(CVALUE);
		break;
	case Num_bytes_v:
		if (IVALUE < 0)
			printe(ERR_invalid_num, E_ERROR);
		else
			num_bytes = IVALUE;
		break;
	case Num_lines_v: 
		if (IVALUE < 0)
			printe(ERR_invalid_num, E_ERROR);
		else
			num_line = IVALUE;
		break;
	case List_file_v:
		if (!pobj)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		pobj->set_current_stmt(CVALUE, 1);
		break;
	case List_line_v:
	{
		char *fname;
		long lval = *(long *)value;
		if (!pobj)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		if (lval <= 0)
			printe(ERR_debug_var_val, E_WARNING, "%list_line");
		fname = pobj->curr_src();
		pobj->set_current_stmt(fname, lval);
		break;
	}
#ifdef DEBUG_THREADS
	case Thread_v:
	{
		Thread	*t;

		name = CVALUE;
		if (!name)
			break;
		t = proglist.find_thread(name);
		if (!t)
			printe(ERR_no_match, E_ERROR, name);
		else
			proglist.set_current(t, 1);
		break;
		
	}
#endif
	case Proc_v:
	{
		Process	*proc;

		name = CVALUE;
		if (!name)
			break;
		proc = proglist.find_proc(name);
		if (!proc)
			printe(ERR_no_match, E_ERROR, name);
		else
			proglist.set_current(proc, 1);
		break;
		
	}
	case Program_v:
	{
		Program	*prog;

		name = CVALUE;
		if (!name)
			break;
		prog = proglist.find_prog(name);
		if (!prog)
			printe(ERR_no_match, E_ERROR, name);
		else
			proglist.set_current(prog, 1);
		break;
		
	}
	case Path_v:
		if (!pobj)
		{
			printe(ERR_no_proc, E_ERROR);
				return 0;
		}
		// set_path deletes the current string
		name = CVALUE;
		pobj->program()->set_path(makestr(name));
		break;
	case Glob_path_v:
		delete global_path;
		name = CVALUE;
		global_path = makestr(name);
		pathage++;	// to invalidate cached source files
		break;
	case Prompt_v:
		// Should reclaim space, but first value is not 
		// dynamically allocated.
		// delete Pprompt;
		name = CVALUE;
		Pprompt = makestr(name);
		break;
	case Verbose_v:
		{
		int enum_value;
		name = CVALUE;
		enum_value = debug_enum_value(name, verbose_table);
		if (enum_value < 0)
			enum_value = debug_enum_value(name, table_to_5);
		if (enum_value < 0)
			printe(ERR_debug_var_val, E_ERROR, "%verbose");
		else
			vmode = enum_value;
		}
		break;
	case Redir_v:
		{
		int enum_value;
		name = CVALUE;
		enum_value = debug_enum_value(name, yes_no_table);
		if (enum_value < 0)
			enum_value = debug_enum_value(name, table_to_5);
		if (enum_value < 0 || enum_value > 1)
			printe(ERR_debug_var_val, E_ERROR, "%redir");
		else
			redir_io = enum_value;
		break;
		}
#ifdef DEBUG_THREADS
	case Thread_change_v:
		{
			int enum_value;
			name = CVALUE;
			enum_value = debug_enum_value(name, tchange_table);
			if (enum_value < 0)
				enum_value = debug_enum_value(name, table_to_5);
			if (enum_value < TCHANGE_IGNORE || enum_value > TCHANGE_STOP)
				printe(ERR_debug_var_val, E_ERROR, "%thread_change");
			else
				thr_change = enum_value;
			break;
		}
#endif
	case Wait_v:
		int enum_value;
		name = CVALUE;
		enum_value = debug_enum_value(name, yes_no_table);
		if (enum_value < 0)
			enum_value = debug_enum_value(name, wait_table);
		if (enum_value < 0)
			enum_value = debug_enum_value(name, table_to_5);
		if (enum_value < 0 || enum_value > 1)
			printe(ERR_debug_var_val, E_ERROR, "%wait");
		else
			wait_4_proc = enum_value;
		break;
	default:
		printe(ERR_internal, E_ERROR, "Debug_var_debug::write",	
			__LINE__);
		break;
	}
	return byte_count;
}

class Debug_var_user: public Debug_var
{
private:
	char * current_value;
	int    current_size;
	int    current_alloc;
public:
	Debug_var_user() {current_value = 0; current_size = current_alloc = 0;};

        // See below.
	Fund_type fund_type(void) 	{return ft_string;};
	int       size(void) 		{return current_size;}
	int       isnull(void) 		{return current_value == 0;};
	debug_var_class var_class()     {return user_var;};
	int       read_value(void * value, int )
			{
				memcpy(value, current_value, current_size);
				return current_size;
			};
	int       write_value(void * value, int byte_count);
};

int       
Debug_var_user::write_value(void * value, int byte_count)
{
	// Ensure value is *null-terminated* string
	if (!current_value 
		|| (byte_count+1) > current_alloc)
	{
		delete current_value;
		current_value = new char[byte_count+1];
		current_alloc = byte_count + 1;
	}
	memcpy(current_value, value, byte_count);
	if (current_value[byte_count-1] != 0)
	{
		current_value[byte_count] = 0;
		current_size = byte_count+1;
	}
	else
		current_size = byte_count;
	return byte_count;
}

// C++ 2.1 workaround
extern RegAttrs regs[];

Debug_var_table::Debug_var_table()
{
#ifdef DEBUG
	Enter("%debug",     new Debug_var_debug(Debug_v, dv_rdwr));
#endif
	Enter("%db_lang",     new Debug_var_debug(Db_lang_v, dv_rdonly));
	Enter("%file",        new Debug_var_debug(File_v, dv_rdonly));
	Enter("%follow",      new Debug_var_debug(Follow_v, dv_rdwr));
	Enter("%frame",       new Debug_var_debug(Frame_v, dv_rdwr));
	Enter("%func",        new Debug_var_debug(Func_v, dv_rdwr));
	Enter("%global_path", new Debug_var_debug(Glob_path_v, dv_rdwr));
	Enter("%lang",        new Debug_var_debug(Lang_v, dv_rdwr));
	Enter("%lastevent",   new Debug_var_debug(Lastevent_v, dv_rdonly));
	Enter("%line",        new Debug_var_debug(Line_v, dv_rdonly));
	Enter("%list_file",   new Debug_var_debug(List_file_v, dv_rdwr));
	Enter("%list_line",   new Debug_var_debug(List_line_v, dv_rdwr));
	Enter("%loc",         new Debug_var_debug(Loc_v, dv_rdonly));
	Enter("%mode",        new Debug_var_debug(Mode_v, dv_rdwr));
	Enter("%num_bytes",   new Debug_var_debug(Num_bytes_v, dv_rdwr));
	Enter("%num_lines",   new Debug_var_debug(Num_lines_v, dv_rdwr));
	Enter("%path",        new Debug_var_debug(Path_v, dv_rdwr));
	Enter("%proc",        new Debug_var_debug(Proc_v, dv_rdonly_gui));
	Enter("%program",     new Debug_var_debug(Program_v, dv_rdonly_gui));
	Enter("%prompt",      new Debug_var_debug(Prompt_v, dv_rdwr));
	Enter("%redir",       new Debug_var_debug(Redir_v, dv_rdonly_gui));
	Enter("%result",      new Debug_var_debug(Result_v, dv_rdonly));
	Enter("%thisevent",   new Debug_var_debug(Thisevent_v, dv_rdonly));
#ifdef DEBUG_THREADS
	Enter("%thread",      new Debug_var_debug(Thread_v, dv_rdonly_gui));
	Enter("%thread_change", new Debug_var_debug(Thread_change_v, dv_rdonly_gui));
#endif
	Enter("%verbose",     new Debug_var_debug(Verbose_v, dv_rdonly_gui));
	Enter("%wait",        new Debug_var_debug(Wait_v, dv_rdwr));

	// Enter register debug variables.
	RegAttrs * a_reg = regs;
	while (a_reg->ref != REG_UNK)
	{
		Enter(a_reg->name, new Debug_var_register(a_reg->ref));
		a_reg++;
	}
};

class DV_table_entry
{
public:
	char * name;
	Debug_var * var;
};

void
Debug_var_table::Enter(const char * name, Debug_var * var)
{
	// Assumes that name has not already been entered.
	DV_table_entry * new_entry = new DV_table_entry;
	new_entry->name = str(name);
	new_entry->var = var;
	DV_table_entry * insert_before = (DV_table_entry*)table.first();
	while (insert_before && strcmp(name, insert_before->name) > 0)
		insert_before = (DV_table_entry*)table.next();
	table.insert((void *)new_entry);
};

Debug_var *
Debug_var_table::Enter(const char * name)
{
	// Assumes that name has not already been entered.
	// create a new user defined variable
	// name[0] = $ (else error: illegal name)
	if (name[0] != '$')
	{
		printe(ERR_internal, E_ERROR, "Debug_var_table::Enter",
			__LINE__);
		return 0;
	};

	Debug_var_user * user_var = new Debug_var_user();
	Enter(name, user_var); 
	return user_var;
};

Debug_var *
Debug_var_table::Lookup(const char * name)
{
	// This adds all names to the hash table, even if the lookup fails.
	char * name_ptr = str(name);
	DV_table_entry * entry = (DV_table_entry *) table.first();
	while (entry)
	{
		if (entry->name == name_ptr) return entry->var;
		if (strcmp(name, entry->name) < 0 ) break;
		entry = (DV_table_entry *) table.next();
	}
	// Add $ names on lookup; the value is null until assignment
	if (name[0] == '$')
		return Enter(name);
	else
		return 0;
};

char *
Debug_var_table::Lookup(Debug_var * var)
{
	DV_table_entry * entry = (DV_table_entry *) table.first();
	while (entry)
	{
		if (var == entry->var) return entry->name;
		entry = (DV_table_entry *) table.next();
	}
	return 0;
};

Debug_var *
Debug_var_table::First()
{
	DV_table_entry * entry = (DV_table_entry*)table.first();
	if (!entry) 
		return 0;
	else if (entry->var->var_class() == user_var && entry->var->isnull()) 
		return Next();
	else 
		return entry->var;
}

Debug_var *
Debug_var_table::Next()
{
	DV_table_entry * entry = (DV_table_entry*)table.next();
	while (entry && entry->var->var_class() == user_var 
	&& entry->var->isnull())
		entry = (DV_table_entry*)table.next();
	if (entry) return entry->var;
	else       return 0;
}
