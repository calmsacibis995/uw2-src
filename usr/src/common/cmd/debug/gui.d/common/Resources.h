/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _RESOURCES_H
#define _RESOURCES_H

#ident	"@(#)debugger:gui.d/common/Resources.h	1.1"

#include "ResourcesP.h"
#include "Windows.h"

class Resources {
	RESOURCE_TOOLKIT_SPECIFICS
private:
	const char	*configuration;
	const char	*config_file;
	Action		output_action;
	Action		event_action;
	int		event_level;
	int		command_level;
	int		symbol_types;
#ifdef DEBUG_THREADS
	int		thread_action;
#endif
public:
			Resources();
			~Resources() {}
	void		initialize();
	const char	*get_config() { return configuration; }
	const char	*get_config_file() { return config_file; }
	Action		get_output_action() { return output_action; }
	Action		get_event_action() { return event_action; }
	int		get_command_level() { return command_level; }
	int		get_event_level() { return event_level; }
	int		get_symbol_types() { return symbol_types; }
#ifdef DEBUG_THREADS
	int		get_thread_action() { return thread_action; }
#endif
};

extern Resources	resources;

#endif
