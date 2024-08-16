#ident	"@(#)debugger:libol/common/Resources.C	1.3"

// Read resource data base and initialize resource
// class so rest of gui can access the values

#include "Toolkit.h"
#include "Resources.h"
#include "Proclist.h"
#include "Windows.h"
#include "Syms_pane.h"

#include <stddef.h>
#include <string.h>
#include <ctype.h>

Resources	resources;

// converts name in place
static char *
cvt_to_upper(char *str)
{
	register char	*ptr = str;
	while(*ptr)
	{
		*ptr = toupper(*ptr);
		ptr++;
	}
	return str;
}

struct res_action {
	const char 	*name;
	int		action;
};

// list must be sorted alphabetically
static const res_action  acts[] =
{
	{ "BEEP",	(int)A_beep },
	{ "MESSAGE",	(int)A_message },
	{ "NONE",	(int)A_none },
	{ "RAISE",	(int)A_raise },
	{ 0,		(int)A_none },
};

// list must be sorted alphabetically
static const res_action	levs[] = 
{
	{ "PROCESS",	PROCESS_LEVEL },
	{ "PROGRAM",	PROGRAM_LEVEL },
#ifdef DEBUG_THREADS
	{ "THREAD",	THREAD_LEVEL },
#endif
	{ 0,		0 },
};

// list must be sorted alphabetically
static const res_action	stypes[] = 
{
	{ "DEBUGGER",	SYM_debugger },
	{ "FILE",	SYM_file },
	{ "GLOBAL",	SYM_global },
	{ "LOCAL",	SYM_local },
	{ "USER",	SYM_user },
	{ 0,		0 },
};

#ifdef DEBUG_THREADS

// list must be sorted alphabetically
static const res_action	tacts[] = 
{
	{ "BEEP",	TCA_beep },
	{ "NONE",	0 },
	{ "STOP",	TCA_stop },
	{ 0,		0 },
};
#endif

static int
str_to_act(const res_action *tab, int defact, char *str, char *resource)
{
	const res_action	*a;

	str = cvt_to_upper(str);

	for(a = tab; a->name; a++)
	{
		int	i = strcmp(str, a->name);
		if (i == 0)
			return a->action;
		else if (i < 0)
			break;
	}
	display_msg(E_ERROR, GE_resource, str, resource);
	return defact;
}

static int
str_to_types(const res_action *tab, int deftype, char *str, char *resource)
{
	char			*next;
	const res_action	*s;
	int			type = 0;
	int			none = 0;

	str = cvt_to_upper(str);
	next = str;
	while(next && *next)
	{
		str = next;
		next = strchr(str, ',');
		if (next)
		{
			*next = 0;
			next++;
		}
		for(s = tab; s->name; s++)
		{
			int	i = strcmp(str, s->name);
			// "NONE" illegal with other types
			if (i == 0)
			{
				if (strcmp(str, "NONE") == 0)
				{
					if (type != 0)
					{
						display_msg(E_ERROR,
							GE_resource_none,
							resource);
						return deftype;
					}
					none = 1;
				}
				else if (none)
				{
					display_msg(E_ERROR,
						GE_resource_none,
						resource);
					return deftype;
				}
				else
					type |= s->action;
				break;
			}
			else if (i < 0)
			{
				display_msg(E_ERROR, GE_resource, str,
					resource);
				return deftype;
			}
		}
	}
	return type;
}

struct DebugResources {
	char	*config_string;
	char	*config_file;
	Boolean	iconic;
	char	*output_action;
	char	*event_action;
	char	*command_level;
	char	*event_level;
	char	*symbols;
#ifdef DEBUG_THREADS
	char	*thread_action;
#endif
};

static XtResource	resrc[] = 
{
	{ "config_desc", "Config_desc", XtRString, sizeof(String), 
		offsetof(DebugResources, config_string),
		XtRString, (String)0 },
	{ "config_file", "Config_file", XtRString, sizeof(String), 
		offsetof(DebugResources, config_file),
		XtRString, (String)0 },
	{ "iconic", "Iconic", XtRBoolean, sizeof(Boolean),
		offsetof(DebugResources, iconic), XtRImmediate,
		(caddr_t)False },
	{ "output_action", "Output_action", XtRString, sizeof(String), 
		offsetof(DebugResources, output_action),
		XtRString, (String)0 },
	{ "event_action", "Event_action", XtRString, sizeof(String), 
		offsetof(DebugResources, event_action),
		XtRString, (String)0 },
	{ "command_level", "Command_level", XtRString, sizeof(String), 
		offsetof(DebugResources, command_level),
		XtRString, (String)0 },
	{ "event_level", "Event_level", XtRString, sizeof(String), 
		offsetof(DebugResources, event_level),
		XtRString, (String)0 },
	{ "symbols", "Symbols", XtRString, sizeof(String), 
		offsetof(DebugResources, symbols),
		XtRString, (String)0 },
#ifdef DEBUG_THREADS
	{ "thread_action", "Thread_action", XtRString, sizeof(String), 
		offsetof(DebugResources, thread_action),
		XtRString, (String)0 },
#endif
};

static XrmOptionDescRec options[] = 
{
	{"-config", "config_file", XrmoptionSepArg, NULL },
	{"-iconic", "*iconic", XrmoptionNoArg, (caddr_t)"True" },
	{"-output_action", "output_action", XrmoptionSepArg, NULL },
	{"-event_action", "event_action", XrmoptionSepArg, NULL },
	{"-command_level", "command_level", XrmoptionSepArg, NULL },
	{"-event_level", "event_level", XrmoptionSepArg, NULL },
	{"-symbols", "symbols", XrmoptionSepArg, NULL },
#ifdef DEBUG_THREADS
	{"-thread_action", "thread_action", XrmoptionSepArg, NULL },
#endif
};

Resources::Resources()
{
	configuration = 0;
	config_file = 0;
	output_action = A_raise;
	event_action = A_beep;
	event_level = PROGRAM_LEVEL;
	symbol_types = SYM_local;
#ifdef DEBUG_THREADS
	thread_action = TCA_beep|TCA_stop;
	command_level = THREAD_LEVEL;
#else
	command_level = PROCESS_LEVEL;
#endif
}

void 
Resources::initialize()
{
	DebugResources	dbg;

	XtGetApplicationResources(base_widget, &dbg, resrc,
		XtNumber(resrc), NULL, 0);
	config_file = dbg.config_file;
	configuration = dbg.config_string;
	if (dbg.output_action)
		output_action = (Action)str_to_act(acts, 
			(int)output_action, dbg.output_action,
			"output_action");
	if (dbg.event_action)
		event_action = (Action)str_to_act(acts, 
			(int)event_action, dbg.event_action,
			"event_action");
	if (dbg.command_level)
		command_level = str_to_act(levs, command_level, 
			dbg.command_level, "command_level");
	if (dbg.event_level)
		event_level = str_to_act(levs, event_level, 
			dbg.event_level, "event_level");
	if (dbg.symbols)
		symbol_types = str_to_types(stypes, symbol_types,
			dbg.symbols, "symbols");
#ifdef DEBUG_THREADS
	if (dbg.thread_action)
		thread_action = str_to_types(tacts, thread_action, 
				dbg.thread_action, "thread_action");
#endif
}

XrmOptionDescRec *
Resources::get_options(int &noptions)
{
	noptions = sizeof(options)/sizeof(XrmOptionDescRec);
	return options;
}
