/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/devices/turbopro/apt_init.c	1.2"

/***
 ***	NAME
 ***
 ***		apt_init.c : ati pro turbo card init functions
 ***
 ***	SYNOPSIS
 ***
 ***		
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles the initialization and other interface
 ***	issues required from the SI server.
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	AUTHORS
 ***
 ***	HISTORY
 ***
 ***/

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean apt_init_debug = 0;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sidep.h"
#include "g_state.h"
#include "m64_state.h"
#include "m64.h"

#include "apt_msg.h"

/***
 ***	Constants.
 ***/
#define DEFAULT_APT_INIT_COMPATIBILITY_STRING_OPTION_SIZE	1024

#define DEFAULT_APT_INIT_COMPATIBILITY_CHIPSET_NAME 		"APT_INIT"

#define DEFAULT_APT_INIT_COMPATIBILITY_IDENT_STRING			\
	"ATI graphics pro turbo (mach64) board."

#define DEFAULT_APT_INIT_COMPATIBILITY_INFO_FORMAT			\
	"%s %s %dx%d %gx%g %d %dK"

#define DEFAULT_APT_INIT_COMPATIBILITY_INFO_TO_VENDOR_LIB	\
	" options-variable=" "DISPLIB_OPTIONS"					\
	" options-variable=" LIBRARY_NAME "_OPTIONS"

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

/*
 * Display Module initialization
 */
function SIBool
DM_InitFunction(SIint32 virtual_terminal_file_descriptor,
				SIScreenRec *si_screen_p)
{
	struct m64_screen_state *screen_state_p;
	int initialization_status;
	
#if (defined(__DEBUG__))
	extern void apt_init_debug_control(boolean);
	
	apt_init_debug_control(TRUE);
	if (apt_init_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(apt_init) DM_InitFunction {\n"
			"\tvirtual_terminal_file_descriptor = %ld\n"
			"\tsi_screen_p = %p\n"
			"\tscreen number = %d\n"
			"}\n",
		   virtual_terminal_file_descriptor,
		   (void *) si_screen_p,
		   (int) si_screen_p->cfgPtr->screen);
	}
#endif /* __DEBUG__ */
  
	(void) fprintf(stderr, APT_INIT_STARTUP_MESSAGE);

	/*
	 * Allocate space for the screen state and tuck away the pointer.
	 */
	screen_state_p = 
		allocate_and_clear_memory(sizeof(struct m64_screen_state));
	si_screen_p->vendorPriv = (void *) screen_state_p;

	/*
	 * Fill in the VT fd as this is information is not present in the
	 * configuration information.
	 */
	screen_state_p->generic_state.screen_virtual_terminal_file_descriptor = 
		virtual_terminal_file_descriptor;
	
	/*
	 * Fill in the generic screen pointer too.
	 */
	generic_current_screen_state_p = (void*) screen_state_p;
	

	/*
	 * Call the chipset specific initialization function.
	 */
	initialization_status = m64_initialize_display_library(
		virtual_terminal_file_descriptor, si_screen_p, 
		(struct m64_screen_state *) screen_state_p);

	if (initialization_status)
	{
#if (defined(__DEBUG__))
		if (apt_init_debug)
		{
			(void) fprintf(debug_stream_p, "(apt_init) chipset init "
					"failed.\n");
		}
		
#endif /* __DEBUG__ */
		m64_print_initialization_failure_message(
				initialization_status, si_screen_p);
#if (defined(USE_KD_GRAPHICS_MODE))
		if (!ioctl(virtual_terminal_file_descriptor, KDSETMODE,KD_TEXT0))
		{
			(void) fprintf(stderr,
				APT_INIT_UNABLE_TO_RESTORE_TEXTMODE_MESSAGE);
		}
#endif
		return (SI_FAIL);
	}
#if 0
#if (defined(__DEBUG__))
	if (apt_init_debug)
	{
		ASSERT(screen_state_p->register_state.generic_state.
			register_dump_state != NULL); 
		(*screen_state_p->register_state.generic_state.
		register_dump_state)((struct generic_screen_state *)screen_state_p,0);
		return(SI_FAIL);
	}
#endif
#endif

	/*
	 * Force a switch of the virtual terminal.
	 */
	ASSERT(si_screen_p->funcsPtr->si_vt_restore);

	(*si_screen_p->funcsPtr->si_vt_restore)();


	return (SI_SUCCEED);
}


/*
 * Backward compatibility code for use with SI version 1.0 core
 * servers.
 */
extern SIFunctions apt_init_compatibility_display_functions;

/*
 * An old style initialization entry point  for the R4 server.
 */

STATIC SIBool
apt_init_compatibility_init(int vt_fd, SIConfigP old_cfg_p, SIInfoP	info_p,
	ScreenInterface **routines_pp)
{
	SIScreenRec *si_screen_p;	/* new structure to pass down */
	int video_ram_size;
	int vfreq;
	char model_name[DEFAULT_APT_INIT_COMPATIBILITY_STRING_OPTION_SIZE];
	char *vfreq_p;
	float monitor_width, monitor_height;
	int disp_w, disp_h;
	int depth;
	SIConfigP config_p;
	char monitor_name[DEFAULT_APT_INIT_COMPATIBILITY_STRING_OPTION_SIZE];
	char	*virtual_dimension_string_p;
	
#if (defined(__DEBUG__))
	if (apt_init_debug)
	{
		(void) fprintf(debug_stream_p,
			"(apt_init_compatibility_init)\n"
			"{\n"
			"\tvt_fd = %d\n"
			"\tconfig_p = %p\n"
			"\tinfo_p = %p\n"
			"\troutines_pp = %p\n"
			"}\n",
			vt_fd, (void *) old_cfg_p, (void *) info_p, (void *) routines_pp);
	}
#endif

	/*
	 * Code added for maintaining compatibility with 
	 * R4 SI.
	 */

	si_screen_p = allocate_and_clear_memory(sizeof(SIScreenRec));

	si_screen_p->flagsPtr = info_p;
	si_screen_p->funcsPtr = *routines_pp =
		&apt_init_compatibility_display_functions;
	
	config_p = si_screen_p->cfgPtr = 
		allocate_and_clear_memory(sizeof(SIConfig));

	/*
	 * Copy out the old fields.
	 */

	config_p->resource = old_cfg_p->resource;
	config_p->class = old_cfg_p->class;
	config_p->visual_type = old_cfg_p->visual_type;
	config_p->info = old_cfg_p->info;
	config_p->display = old_cfg_p->display;
	config_p->displaynum = old_cfg_p->displaynum;
	config_p->screen = old_cfg_p->screen;
	config_p->device = old_cfg_p->device;

	/*
	 * Parse the information field to more information to fill into
	 * the new SIConfig structure.
	 */

	config_p->chipset = DEFAULT_APT_INIT_COMPATIBILITY_CHIPSET_NAME;
	
	config_p->IdentString =
		DEFAULT_APT_INIT_COMPATIBILITY_IDENT_STRING;

	if (sscanf(config_p->info,
		DEFAULT_APT_INIT_COMPATIBILITY_INFO_FORMAT,
		model_name, monitor_name, &disp_w, &disp_h,
		&monitor_width, &monitor_height, &depth,
		&video_ram_size) != 8)
	{
		/*
		 * Something went wrong in the parsing of information.
		 */

		(void) fprintf(stderr,
			APT_INIT_COMPATIBILITY_BAD_INFO_STRING_MESSAGE,
			config_p->info);
		return (SI_FAIL);
	}

	if (monitor_height < 0.0 || monitor_width < 0.0)
	{
		(void) fprintf(stderr,
			   APT_INIT_COMPATIBILITY_BAD_MONITOR_DIMENSIONS_MESSAGE,
			   monitor_width, monitor_height);
		return (SI_FAIL);
	}
	
	/*
	 * Get the modes vertical refresh frequency.
	 */

	vfreq = 0;
	if (vfreq_p = strchr(monitor_name, '_'))
	{
		*vfreq_p++ = 0;
		(void) sscanf(vfreq_p, "%d", &vfreq);
	}
	else
	{
		(void) fprintf(stderr,
			APT_INIT_COMPATIBILITY_BAD_MONITOR_NAME_MESSAGE, monitor_name);
			return (SI_FAIL);
	}
	
	/*
	 * Fill in fields.
	 */
	config_p->info = NULL;		/* look only in the standard places */
	
	config_p->model = strdup(model_name);
	config_p->videoRam = video_ram_size; /* display memory size	*/
	config_p->virt_w = disp_w;
	config_p->virt_h = disp_h;
	config_p->depth = depth;
	config_p->disp_w = disp_w;
	config_p->disp_h = disp_h;

	/*
	 * At this point virtual and display dimensions are the same.
	 * In R4 env, we cannot specify virtual display in the config file
	 * itself, so it is provided through an env variable
	 * For more details on setting virtual display, read README.M64
	 */
	if ((virtual_dimension_string_p = getenv("M64_VIRTUAL_DISPLAY")) != NULL)
	{
		SIint32	virt_w, virt_h;

		if (sscanf(virtual_dimension_string_p,"%ldx%ld\n",&virt_w,&virt_h) != 2)
		{
			/* 
			 * scan failed, leave the original virtual dimensions alone.
			 */
			(void) fprintf (stderr, 
			APT_INIT_COMPATIBILITY_INVALID_VIRTUAL_DISPLAY_STRING_MESSAGE);
		}
		else 
		{
			config_p->virt_w = virt_w;
			config_p->virt_h = virt_h;
		}
	}

	config_p->monitor_info.width = monitor_width;
	config_p->monitor_info.height = monitor_height;
	config_p->monitor_info.hfreq = 31.5 *  1024;
	config_p->monitor_info.vfreq = vfreq;
	config_p->monitor_info.model = strdup(monitor_name);

	info_p->SIxppin = config_p->virt_w / monitor_width;
	info_p->SIyppin = config_p->virt_h / monitor_height;
	
	/*
	 * initialize via the R5 entry point.
	 */
	if(DM_InitFunction(vt_fd, si_screen_p) != SI_SUCCEED)
	{
		return(SI_FAIL);
	}
	return (SI_SUCCEED);
}

SIFunctions	
apt_init_compatibility_display_functions = {
		apt_init_compatibility_init,
};

SIFunctions *DisplayFuncs = &apt_init_compatibility_display_functions;
