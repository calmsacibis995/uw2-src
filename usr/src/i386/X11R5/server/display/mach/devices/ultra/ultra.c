/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/ultra.c	1.11"

/***
 ***	NAME
 ***
 ***		ultra.c : Initialization for the linear frame buffer driver
 ***					  for the MACH display library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "ultra.h"
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles the initialization and other interface
 ***	issues required by the SI server and interfaces with the
 ***	chipset layer's initialization code.
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
#include "mach.h"
#include "l_opt.h"

/***
 ***	Constants.
 ***/

#if (defined(__DEBUG__))
#define LFB_SCREEN_STATE_STAMP\
	(('L' << 0) + ('F' << 1) + ('B' << 2) + ('S' << 3) +\
	 ('C' << 4) + ('R' << 5) + ('E' << 6) + ('E' << 7) +\
	 ('N' << 8) + ('S' << 9) + ('T' << 10) + ('A' << 11) +\
	 ('T' << 12))
#endif

/***
 ***	Macros.
 ***/

#define LFB_CURRENT_SCREEN_STATE_DECLARE()\
	struct lfb_screen_state *const screen_state_p = \
	(struct lfb_screen_state *) generic_current_screen_state_p

#define LFB_ALL_PLANES_ENABLED_MASK(screen_state_p)\
	((1 << (screen_state_p)->mach_state.generic_state.screen_depth) - 1) 

/***
 ***	Types.
 ***/

/*
 * The Linear Frame Buffer Module state structure
 */

struct lfb_screen_state 
{
	/*
	 * Chipset level module state 
	 */

	struct	mach_screen_state mach_state;
	
	boolean	is_lfb_enabled;

	/*
	 * Pointers to lfb modules state
	 */

	void *lfb_graphics_state_p;
	void *lfb_points_state_p;
	void *arc_state_p;

	/*
	 * lfb specific user options
	 */

	struct lfb_options_structure *options_p ;
		
	/*
	 * Pointer to the ATI frame buffer: 
	 */

	void *frame_buffer_p;

	/*
	 * frame buffer location and length measured in bytes
	 */

	unsigned int frame_buffer_address;
	unsigned int frame_buffer_length;
	
	/*
	 * The file descriptor for use with the MMAP call.
	 */

	int mmap_fd;
	
	/*
	 * Length of one screen line in bytes
	 */

	unsigned int frame_buffer_stride;

	/*
	 * This can be used if the frame buffer
	 * stride is a power of 2
	 */
	int is_frame_buffer_stride_a_power_of_two;
	
	unsigned int frame_buffer_stride_shift;
	unsigned int pixels_per_word_shift;

	/*
	 * The memory aperture interface on ATI is controlled by
	 * the register MEM_CFG.  This register needs to be
	 * saved and restored as and when we VT switch in / out.
	 */

	unsigned short saved_mem_cfg;

	/*APR20
	 *Pointer  to chipset layer
	 *si_restore function
	 */
	SIBool (*screen_restore_function_p)();

	/* JUN 16
	 * Pointer to chipset layer si_vt_save and si_vt_restore
	 * functions.
	 */
	SIBool (*virtual_terminal_save_function_p)();

	SIBool (*virtual_terminal_restore_function_p)();

#if defined(__DEBUG__)
	int stamp;
#endif

}; 

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
export boolean lfb_debug = FALSE;
#endif

/*
 *	Current module state.
 */

/*
 * global pointer to lfb state
 */
export struct lfb_state * lfb_current_state_p;

PRIVATE

/*
 * mman.h also defines the PRIVATE and PUBLIC
 * constants, therefore undefine them 
 */

#if defined(PRIVATE)
#undef PRIVATE
#endif

#if defined(PUBLIC)
#undef PUBLIC
#endif

/***
 ***	Private declarations.
 ***/

extern void lfb__pre_initialize__(SIScreenRec *si_screen_p,
							   struct lfb_options_structure *options_p);
extern void lfb__post_initialize__(SIScreenRec *si_screen_p,
							   struct lfb_options_structure *options_p);
extern	int close(int file_desc);

/***
 ***	Includes.
 ***/

#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>
#include <unistd.h>


/*
 * Bring in the declarations for the memory map driver.
 */
#include "lfb_map.h"

#include "l_globals.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/


/*
 * lfb_restore_screen:
 *
 * Function unmaps the frame buffer and
 * calls the chipset level restore_screen function
 */

STATIC SIBool
lfb_restore_screen(void)
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(screen_state_p->is_lfb_enabled);

#if defined(__DEBUG__)
		if (lfb_debug)
		{
			(void)fprintf(debug_stream_p,
"(lfb_restore_screen)\n{"
"}\n");
		}
#endif
	
	(void) munmap(screen_state_p->frame_buffer_p,
				  screen_state_p->frame_buffer_length);

	ASSERT(screen_state_p->screen_restore_function_p);

#if defined(__DEBUG__)
	if (lfb_debug)
	{
		(void) fprintf(debug_stream_p,
"(lfb_restore_screen)\n{"
"\t#Calling chipset level restore_screen\n"
"}\n");
	}
#endif


	/*
	 * Replace the old value of the mem_cfg register.
	 */

	outb(MACH_REGISTER_MEM_CFG, screen_state_p->saved_mem_cfg);

	return (*screen_state_p->screen_restore_function_p)();

}

/*
 * lfb_virtual_terminal_restore
 *
 */

STATIC SIBool
lfb_virtual_terminal_restore(void)
{

	LFB_CURRENT_SCREEN_STATE_DECLARE();

#if defined(__DEBUG__)
	if (lfb_debug)
	{
		(void) fprintf(debug_stream_p,
"(lfb_virtual_terminal_restore)\n{"
"}\n");
	}
#endif
	
	/*
	 * Memory map the frame buffer.  The mapping is shared so that
	 * other tools can read the same frame buffer too.
	 */

	screen_state_p->frame_buffer_p = 
		mmap(0, screen_state_p->frame_buffer_length, 
			 PROT_READ|PROT_WRITE, MAP_SHARED,
			 screen_state_p->mmap_fd, screen_state_p->frame_buffer_address);

	if ((caddr_t) screen_state_p->frame_buffer_p == (caddr_t) -1)
	{

#if defined(__DEBUG__)
		if (lfb_debug)
		{
			(void)fprintf(debug_stream_p,
"(lfb_virtual_terminal_restore)\n"
"{\n"
"\t# mmap device failed\n" 
"\tdevice = %s\n"
"\terrno = %d\n"
"}\n",
					MAP_DEVICE_FILE_PATH,
					errno);
		}
#endif

		perror(LFB_CANNOT_MEMORY_MAP_FRAME_BUFFER_MESSAGE);

		return (FALSE);

	}

	return (*screen_state_p->virtual_terminal_restore_function_p)();

}


/*
 * 
 * Function called everytime we vt_save in case lfb is enabled.
 * This precisely, unmaps the framebuffer and calls the chipset
 * layer vt_save.
 */

STATIC SIBool
lfb_virtual_terminal_save()
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(screen_state_p->is_lfb_enabled);

#if defined(__DEBUG__)
	if (lfb_debug)
	{
		(void) fprintf(debug_stream_p,
"(lfb_virtual_terminal_save)\n{"
"}\n");
	}
#endif
	
	/*
	 * Unmap the frame buffer.	
	 */

	(void) munmap(screen_state_p->frame_buffer_p,
				  screen_state_p->frame_buffer_length);
					 
	screen_state_p->frame_buffer_p = NULL;

	ASSERT(screen_state_p->virtual_terminal_save_function_p);

#if defined(__DEBUG__)
	if (lfb_debug)
	{
		(void) fprintf(debug_stream_p,
"(lfb_virtual_terminal_save)\n{"
"\t#Calling chipset level restore_screen\n"
"}\n");
	}
#endif

	/*
	 * Replace the old value of the mem_cfg register.
	 */

	outb(MACH_REGISTER_MEM_CFG, screen_state_p->saved_mem_cfg);

	return (*screen_state_p->virtual_terminal_save_function_p)();

}

/*
 * lfb_initialize_lfb_state
 * 
 * helper function : returns TRUE on success otherwise FALSE.
 *
 */

STATIC boolean
lfb_initialize_lfb_state(SIScreenRec *si_screen_p,
						 struct lfb_options_structure *lfb_options_p)
{
	struct	lfbmap_struct	map; /* arguments for the MAP ioctl */
	struct	lfbmap_unit_struct	map_unit;

	int	lfb_physical_address;	/* physical address of frame buffer in
								 * units of MB */
	int	lfb_length;				/* length of the frame buffer in bytes */
	int	tmp;					/* temporary */
	unsigned short mem_cfg;		/* temporary */
	int system_physical_memory_size; /* amount of core memory */
	
	boolean is_lfb_enabled = FALSE;

	int	mmap_fd;				/* return from the MMAP call */

	LFB_CURRENT_SCREEN_STATE_DECLARE();
	
	ASSERT(lfb_options_p != NULL);
	
	/*
	 * Save the values of the MEM_CFG register for later
	 * restoration.  This needs to be done irrespective of whether the
	 * user has asked for LFB code and/or the configuration supports
	 * the code. 
	 */

	screen_state_p->saved_mem_cfg =
		screen_state_p->mach_state.register_state.mem_cfg;
	
	/*
	 * Check if the configuration can support LFB operations.  Allow
	 * the user to override this.
	 */
	
	if (lfb_options_p->use_linear_frame_buffer ==
		LFB_OPTIONS_USE_LINEAR_FRAME_BUFFER_YES)
	{
		is_lfb_enabled = TRUE;
	}
	else if (lfb_options_p->use_linear_frame_buffer ==
			 LFB_OPTIONS_USE_LINEAR_FRAME_BUFFER_NO)
	{
		is_lfb_enabled = FALSE;
	}
	else if (lfb_options_p->use_linear_frame_buffer ==
			 LFB_OPTIONS_USE_LINEAR_FRAME_BUFFER_AUTO_CONFIGURE)
	{

		/*
		 * LFB operations need a mach32 AND a local bus configuration.
		 */
		
		if ((screen_state_p->mach_state.chipset_kind == 
			 MACH_CHIPSET_ATI_68800) &&
			((screen_state_p->mach_state.bus_kind == 
			  MACH_BUS_KIND_EISA) ||
			 (screen_state_p->mach_state.bus_kind == 
			  MACH_BUS_KIND_LOCAL_386SX) ||
			 (screen_state_p->mach_state.bus_kind == 
			  MACH_BUS_KIND_LOCAL_486SX) ||
			 (screen_state_p->mach_state.bus_kind == 
			  MACH_BUS_KIND_LOCAL_486)))
		{
			is_lfb_enabled = TRUE;
		}
		else 
		{
			is_lfb_enabled = FALSE;
		}

		/*
		 * Try and access the "/dev/lfb" kernel driver.
		 */

		if (access(MAP_DEVICE_FILE_PATH, (R_OK | W_OK)) != 0)
		{
			is_lfb_enabled = FALSE;
		}
		
	}
#if (defined(__DEBUG__))
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}
#endif

	/*
	 * Return if the linear frame buffer usage has been found
	 * infeasible.
	 */
	
	if (is_lfb_enabled == FALSE)
	{
		return (FALSE);
	}

	(void) fprintf(stderr, DEFAULT_LFB_STARTUP_MESSAGE);

	/*
	 * Check if the user has supplied lfb physical address.
	 */

	if (lfb_options_p->frame_buffer_physical_address  !=  
		LFB_OPTIONS_FRAME_BUFFER_PHYSICAL_ADDRESS_DEFAULT)
	{
		lfb_physical_address = 
			lfb_options_p->frame_buffer_physical_address;
	}
	else
	{ 

		/*
		 * Try to determine the address from the MEM_CFG register
		 */

		mem_cfg = 
			screen_state_p->mach_state.register_state.mem_cfg;

		lfb_physical_address = 
			(mem_cfg >> MACH_MEM_CFG_MEM_APERTURE_LOC_SHIFT);


		/*
		 * Get the amount of physical memory on the system.
		 */
		
		if ((system_physical_memory_size = sysi86(SI86MEM)) == -1)
		{
			perror(LFB_CANNOT_DETERMINE_SYSTEM_MEMORY_SIZE_MESSAGE);
			return (FALSE);
		}

		/*
		 * Convert to units of MB
		 */

		system_physical_memory_size /= (1024*1024);
		
		if (lfb_physical_address <= system_physical_memory_size)
		{

	#if defined(__DEBUG__)
			if (lfb_debug)
			{
				(void)fprintf(debug_stream_p,
	"(lfb_initialize_lfb_state)\n{"
	"\t#Unable to determine frame buffer location\n" 
	"}\n");
			}
	#endif

			/*
			 * If the aperture location overlaps with the system memory,
			 * we cannot do much. 
			 * return indicating failure.
			 */
			
			(void) fprintf(stderr,
					   LFB_ADDRESS_CLASH_DETECTED_WITH_SYSTEM_MEMORY_MESSAGE,
						   system_physical_memory_size, 
						   lfb_physical_address);

			return (FALSE);

		}

	}
	/*
	 * Get the frame buffer size.
	 */

	if (lfb_options_p->frame_buffer_size == 
		LFB_OPTIONS_FRAME_BUFFER_SIZE_DEFAULT)
	{
		
		/*
		 * Auto-detect the frame buffer size.
		 */

		tmp = (screen_state_p->mach_state.register_state.misc_options & 
			   MACH_MISC_OPTIONS_MEM_SIZE_ALIAS) >>
				   MACH_MISC_OPTIONS_MEM_SIZE_ALIAS_SHIFT;

		/*
		 * Allowed values for tmp
		 * 0  : 512 KB
		 * 1  : 1   MB
		 * 2  : 2   MB
		 * 3  : 4   MB
		 */

		if (tmp < 0 || tmp > 3)
		{
			(void) fprintf(stderr,
						   LFB_BAD_MEMORY_SIZE_DETECTED_MESSAGE);
			return (FALSE);
		}
	
		lfb_length =  (1 << tmp) * 512 * 1024; /* in bytes*/

	}
	else
	{
		
		/*
		 * The user specified the frame buffer size -- convert to 
		 * number of bytes.
		 */

		lfb_length = lfb_options_p->frame_buffer_size * 
			(1024 * 1024);

	}


	/*
	 * program the MEM_CFG register as required.  Note that we use 4
	 * MB pages always.
	 */

	mem_cfg = 
		(lfb_physical_address << MACH_MEM_CFG_MEM_APERTURE_LOC_SHIFT) |
		MACH_MEM_CFG_MEM_APERTURE_SEL_4_MB |
		MACH_MEM_CFG_MEM_APERTURE_PAGE_0;

#if defined(__DEBUG__)
	if (lfb_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_initialize_lfb_state)\n"
"{\n"
"\t# Programming memory aperture registers\n"
"\tmem_cfg = 0x%x\n"
"}\n",
					  mem_cfg);
	}
#endif

	/*
	 * Place the new values of the memory configuration register into
	 * the shadow copy of the values.  The call to VT Restore will do
	 * the needful.
	 */

	screen_state_p->mach_state.register_state.mem_cfg = mem_cfg;

	/*
	 * Open mmap device and try to memory map the frame buffer
	 */

	/*
	 * Save the map device file descriptor in lfb_state to be
	 * used when we map, every time we vt_restore. Note that,
	 * we are unmapping frame buffer everytime we vt switch out
	 * (vt_save).
	 */ 

	screen_state_p->mmap_fd = mmap_fd = 
		open(MAP_DEVICE_FILE_PATH, O_RDWR);

	if (mmap_fd == -1)
	{
	
#if defined(__DEBUG__)
		if (lfb_debug)
		{
			(void)fprintf(debug_stream_p,
"(lfb_initialize_lfb_state)\n"
"{\n"
"\t# Unable to open mmap device\n" 
"\tdevice = %s\n"
"}\n",
						  MAP_DEVICE_FILE_PATH);
			
		}
#endif
		(void) fprintf(stderr,
					   LFB_CANNOT_OPEN_FRAME_BUFFER_DEVICE_MESSAGE,
					   MAP_DEVICE_FILE_PATH);
		
		perror(LIBRARY_NAME ": ");
		
		return (FALSE);

	}

	/*
	 * Before mmap  we should notify the mmap driver the physical
	 * memory location through the SET_BOUNDS ioctl command
	 */

	map_unit.map_area_index = 0;
	map_unit.map_start_address_p = 
		lfb_physical_address * (1024*1024);	/* convert to a byte address */
	map_unit.map_length = lfb_length;
	
	map.map_count = 1;			/* one area to be mapped */
	map.map_struct_list_p = &map_unit;

	if (ioctl(mmap_fd, MAP_SETBOUNDS, &map) < 0)
	{

#if defined(__DEBUG__)
		if (lfb_debug)
		{
			(void)fprintf(debug_stream_p,
"(lfb_initialize_lfb_state)\n"
"{\n"
"\t# Unable to SET_BOUNDS on mmap device\n" 
"\tdevice = %s\n"
"\terrno = %d\n"
"}\n",
					MAP_DEVICE_FILE_PATH,
					errno);
		}
#endif

		perror(LFB_SET_MAP_BOUNDS_FAILED_MESSAGE);
		
		(void) close(mmap_fd);

		return (FALSE);

	}

	/*
	 * Save state.
	 */

	screen_state_p->frame_buffer_address = 
		lfb_physical_address * (1024*1024);	/* convert to a byte address */

	screen_state_p->frame_buffer_length  = lfb_length;

	screen_state_p->frame_buffer_stride  = 
		(generic_current_screen_state_p->screen_physical_width *
		generic_current_screen_state_p->screen_depth) / 8;

	/*
	 * A small optimization replacing multiplication with shifts.
	 */

	if (screen_state_p->frame_buffer_stride == 1024)
	{
		screen_state_p->is_frame_buffer_stride_a_power_of_two = TRUE;
		screen_state_p->frame_buffer_stride_shift = 10;
	}
	else
	{
		screen_state_p->is_frame_buffer_stride_a_power_of_two = FALSE;
		screen_state_p->frame_buffer_stride_shift = 0;  
	}

#if defined(__DEBUG__)
	if (lfb_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_initialize_lfb_state)\n"
"{\n"
"\tframe_buffer_address = %dMB\n"
"\tframe_buffer_length = %dMB\n"
"\tframe_buffer_stride = %d\n"
"\tframe_buffer_stride_shift = %d\n"
"}\n",
					  screen_state_p->frame_buffer_address >> 20,
					  screen_state_p->frame_buffer_length >> 20,
					  screen_state_p->frame_buffer_stride,
					  screen_state_p->frame_buffer_stride_shift);
	}
#endif

	/*
	 * Determine the number of pixels per processor word.
	 */

	switch (screen_state_p->mach_state.generic_state.screen_depth)
	{
	case 4:
		screen_state_p->pixels_per_word_shift = 3;
		break;
	case 8:
		screen_state_p->pixels_per_word_shift = 2;
		break;	
	case 16:
		screen_state_p->pixels_per_word_shift = 1;
		break;
	case 24:

		/*
		 * We can't support LFB for 24 bit deep frame buffers.
		 */
		is_lfb_enabled = FALSE;
		break;
		
	default:
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}/*switch*/
	
	/*
	 * We have to set the si_restore field to a local
	 * restore function which will unmap the 
	 * frame buffer as well as call the chipset level
	 * screen_restore function.
	 * So save the chipset level layer function for
	 * future use.
	 */

	screen_state_p->screen_restore_function_p =
		si_screen_p->funcsPtr->si_restore;
	si_screen_p->funcsPtr->si_restore = lfb_restore_screen;

	/* 
	 * Patch our local virtual_terminal_save and 
	 * virtual_terminal_restore functions into si structure after
	 * saving chipset level save and restore functions in the 
	 * lfb_state. This will help us unmap framebuffer everytime we 
	 * vt_save before calling chipset level vt_save AND map it
	 * everytime we vt_restore before chipset level vt_restore
	 * is called.
	 *
	 */

	screen_state_p->virtual_terminal_save_function_p =
		si_screen_p->funcsPtr->si_vt_save;
	si_screen_p->funcsPtr->si_vt_save = lfb_virtual_terminal_save;

	screen_state_p->virtual_terminal_restore_function_p =
		si_screen_p->funcsPtr->si_vt_restore;
	si_screen_p->funcsPtr->si_vt_restore = lfb_virtual_terminal_restore;

	return is_lfb_enabled;

}

/*
 * Display Module initialization
 */

function SIBool
DM_InitFunction(SIint32 virtual_terminal_file_descriptor,
				SIScreenRec *si_screen_p)
{
	struct lfb_screen_state *screen_state_p;
	struct lfb_options_structure *lfb_options_p;
	SIConfigP config_p;
	int initialization_status;
	char lfb_options_string[DEFAULT_LFB_STRING_OPTION_SIZE];
	struct stat stat_buffer;
	
	
#if (defined(__DEBUG__))
	extern void lfb_debug_control(boolean);
#endif
	
#if (defined(__DEBUG__))
	lfb_debug_control(TRUE);

	if (lfb_debug)
	{
		(void) fprintf(debug_stream_p, 
"(lfb)\n"
"{\n"
"\t#DM_InitFunction\n"
"\tvirtual_terminal_file_descriptor = %ld\n"
"\tsi_screen_p = %p\n"
"\tscreen number = %d\n"
"}\n",
					   virtual_terminal_file_descriptor,
					   (void *) si_screen_p,
					   (int) si_screen_p->cfgPtr->screen);
	}
#endif /* __DEBUG__ */
  
	/*
	 * parse lfb specific options
   	 */

	config_p = si_screen_p->cfgPtr;

	/*
	 * User option handling.  Null terminate the buffer first.
	 */

	lfb_options_string[0] = '\0';
	
	/*
	 * The add a file parsing option if the standard options file is
	 * present.
	 */

	if (access(DEFAULT_LFB_STANDARD_OPTION_FILE_NAME, F_OK | R_OK) == 0)
	{
		(void) strcpy(lfb_options_string,
					  DEFAULT_LFB_PARSE_STANDARD_OPTIONS_FILE_COMMAND);
	}

	/*
	 * Add parsing options for the user specified option string.
	 */

	(void) strcat(lfb_options_string, config_p->info2vendorlib);
	(void) strcat(lfb_options_string, " ");

	/*
	 * Add parsing options for the standard environment option variables 
	 */

	(void) strcat(lfb_options_string,
				  DEFAULT_LFB_PARSE_STANDARD_ENVIRONMENT_VARIABLES_COMMAND);
	
	/*
	 * parse options.
	 */

	lfb_options_p = lfb_options_parse(NULL, lfb_options_string);

	if (lfb_options_p == NULL)
	{
#if (defined(__DEBUG__))
		if (lfb_debug)
		{
			(void) fprintf(debug_stream_p, "parsing of user options "
					"failed.\n"); 
		}
#endif /* __DEBUG__ */
		return (SI_FAIL);
	}

	/*
	 * Allocate space for the screen state and tuck away the pointer.
	 */

	screen_state_p =
		allocate_and_clear_memory(sizeof(struct lfb_screen_state));
	si_screen_p->vendorPriv = (void *) screen_state_p;

	/*
	 * Fill in the VT fd as this is information is not present in the
	 * configuration information.
	 */
	screen_state_p->mach_state.generic_state.
		screen_virtual_terminal_file_descriptor = 
			virtual_terminal_file_descriptor;
	
	/*
	 * Fill in the generic screen pointer too.
	 */
	generic_current_screen_state_p = (struct generic_screen_state *)
		screen_state_p;

	/*
	 * Fill in the options pointer.
	 */
	screen_state_p->options_p = lfb_options_p;
	
	/*
	 * Call the board level pre initialize functions
	 */

	lfb__pre_initialize__(si_screen_p, lfb_options_p);
	
	/*
	 * Call the chipset specific initialization function.
	 */

	initialization_status = 
		mach_initialize_display_library(virtual_terminal_file_descriptor,
										si_screen_p, 
										&(screen_state_p->mach_state));

	if (initialization_status)
	{
#if (defined(__DEBUG__))
		if (lfb_debug)
		{
			(void) fprintf(debug_stream_p, "(lfb) chipset init "
					"failed.\n");
		}
		
#endif /* __DEBUG__ */
		mach_print_initialization_failure_message(initialization_status,
												  si_screen_p);
		
		return (SI_FAIL);
	}
	
	/*
	 * do module specific initialization
	 */

	screen_state_p->is_lfb_enabled =
		lfb_initialize_lfb_state(si_screen_p, lfb_options_p);


#if (defined(__DEBUG__))
	if (lfb_debug)
	{
		(void) fprintf(debug_stream_p,
"(DM_InitFunction)\n"
"{\n"
"\t# LFB interface : %s\n"
"}\n",
					   boolean_to_dump[screen_state_p->is_lfb_enabled]);
	}
#endif

	/*
	 * post initialize all lfb drawing modules
	 */

	lfb__post_initialize__(si_screen_p, lfb_options_p);


	/*
	 * Mark the state structure as having been initialiazed
	 */

	STAMP_OBJECT(LFB_SCREEN_STATE, screen_state_p);
	
	/*
	 * Force a switch of the virtual terminal.
	 */
	ASSERT(si_screen_p->funcsPtr->si_vt_restore);

	(*si_screen_p->funcsPtr->si_vt_restore)();

	/*
	 * Blank the screen
	 */

	MACH_STATE_SET_FG_ROP(&(screen_state_p->mach_state),
						  MACH_MIX_FN_PAINT);
	MACH_STATE_SET_WRT_MASK(&(screen_state_p->mach_state),
							((1 << screen_state_p->mach_state.
							  generic_state.screen_depth) - 1));
	MACH_STATE_SET_FRGD_COLOR(&(screen_state_p->mach_state), 0);
	MACH_STATE_SET_DP_CONFIG(&(screen_state_p->mach_state),
							 screen_state_p->mach_state.dp_config_flags |
							 MACH_DP_CONFIG_WRITE |
							 MACH_DP_CONFIG_ENABLE_DRAW |
							 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR);
#if (defined(__DEBUG__))
	MACH_STATE_SET_ATI_CLIP_RECTANGLE((&(screen_state_p->mach_state)),
		0, 0,
		screen_state_p->mach_state.generic_state.screen_physical_width,
		 ((screen_state_p->mach_state.generic_state.screen_physical_height >
		   DEFAULT_MACH_MAX_ATI_COORDINATE) ?
		  DEFAULT_MACH_MAX_ATI_COORDINATE : 
		  screen_state_p->mach_state.generic_state.screen_physical_height));
	screen_state_p->mach_state.generic_state.screen_current_clip =
		GENERIC_CLIP_NULL;
	MACH_STATE_SET_FLAGS(&(screen_state_p->mach_state),
						 MACH_INVALID_CLIP);
#endif

	MACH_WAIT_FOR_FIFO(6);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);
	outw(MACH_REGISTER_CUR_X, 0);
	outw(MACH_REGISTER_CUR_Y, 0);
	outw(MACH_REGISTER_DEST_X_START, 0);
	outw(MACH_REGISTER_DEST_X_END,
		 screen_state_p->mach_state.generic_state.screen_physical_width);
	outw(MACH_REGISTER_DEST_Y_END,
		 ((screen_state_p->mach_state.generic_state.screen_physical_height >
		   DEFAULT_MACH_MAX_ATI_COORDINATE) ?
		  DEFAULT_MACH_MAX_ATI_COORDINATE : 
		  screen_state_p->mach_state.generic_state.screen_physical_height));
								/* blit starts */

	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);

}

/*
 * Backward compatibility code for use with SI version 1.0 core
 * servers.
 */

extern SIFunctions lfb_compatibility_display_functions;

/*
 * An old style initialization entry point.
 */

STATIC SIBool
lfb_compatibility_init(
	int	vt_fd,
	SIConfigP old_cfg_p,
	SIInfoP	info_p,
	ScreenInterface **routines_pp)
{
	SIScreenRec *si_screen_p;	/* new structure to pass down */
	int video_ram_size;
	int vfreq;
	char model_name[DEFAULT_LFB_STRING_OPTION_SIZE];
	char *vfreq_p;
	float monitor_width, monitor_height;
	int disp_w, disp_h;
	int depth;
	SIConfigP config_p;
	char monitor_name[DEFAULT_LFB_STRING_OPTION_SIZE];
	char	*virtual_dimension_string_p;

#if (defined(__DEBUG__))
	 extern void lfb_debug_control(boolean);
#endif

#if (defined(__DEBUG__))

	lfb_debug_control(TRUE);
	
	if (lfb_debug)
	{
		(void) fprintf(debug_stream_p,
"(lfb_compatibility_init)\n"
"{\n"
"\tvt_fd = %d\n"
"\tconfig_p = %p\n"
"\tinfo_p = %p\n"
"\troutines_pp = %p\n"
"}\n",
					   vt_fd, (void *) old_cfg_p, (void *) info_p,
					   (void *) routines_pp);
	}
#endif

	/*
	 * Code added for maintaining compatibility with 
	 * R4 SI.
	 */

	si_screen_p = allocate_and_clear_memory(sizeof(SIScreenRec));

	si_screen_p->flagsPtr = info_p;
	si_screen_p->funcsPtr = *routines_pp =
		&lfb_compatibility_display_functions;
	
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

	config_p->chipset = DEFAULT_LFB_COMPATIBILITY_CHIPSET_NAME;
	
	config_p->IdentString =
		DEFAULT_LFB_COMPATIBILITY_IDENT_STRING;

	if (sscanf(config_p->info,
			   DEFAULT_LFB_COMPATIBILITY_INFO_FORMAT,
			   model_name, monitor_name, &disp_w, &disp_h,
			   &monitor_width, &monitor_height, &depth,
			   &video_ram_size) != 8)
	{
		/*
		 * Something went wrong in the parsing of information.
		 */

		(void) fprintf(stderr,
					   DEFAULT_LFB_COMPATIBILITY_BAD_INFO_STRING_MESSAGE,
					   config_p->info);
		return (SI_FAIL);
	}

	if (monitor_height < 0.0 || monitor_width < 0.0)
	{
		(void) fprintf(stderr,
			   DEFAULT_LFB_COMPATIBILITY_BAD_MONITOR_DIMENSIONS_MESSAGE,
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
					   DEFAULT_LFB_COMPATIBILITY_BAD_MONITOR_NAME_MESSAGE,
					   monitor_name);
		return (SI_FAIL);
	}
	
	/*
	 * Fill in fields.
	 */
	config_p->info = NULL;
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
	 * For more details on setting virtual display, read README.mach
	 */
	if ((virtual_dimension_string_p = getenv("ULTRA_VIRTUAL_DISPLAY")) != NULL)
	{
		SIint32	virt_w, virt_h;

		if (sscanf(virtual_dimension_string_p,"%ix%i\n",&virt_w,&virt_h) != 2)
		{
			/* 
			 * scan failed, leave the original virtual dimensions alone.
			 */
			(void) fprintf (stderr, 
			DEFAULT_ULTRA_INIT_COMPATIBILITY_INVALID_VIRTUAL_DISPLAY_STRING_MESSAGE);
		}
		else 
		{
			config_p->virt_w = virt_w;
			config_p->virt_h = virt_h;
		}
	}

	config_p->monitor_info.width = monitor_width;
	config_p->monitor_info.height = monitor_height;
	config_p->monitor_info.hfreq = 31.5 * 1024;
	config_p->monitor_info.vfreq = vfreq;
	config_p->monitor_info.model = strdup(monitor_name);

	info_p->SIxppin = config_p->virt_w / monitor_width;
	info_p->SIyppin = config_p->virt_h / monitor_height;
	
	/*
	 * Initialize via the R5 entry point.
	 */

	if (DM_InitFunction(vt_fd, si_screen_p) != SI_SUCCEED)
	{
		return (SI_FAIL);
	}

	return (SI_SUCCEED);
}

SIFunctions	
lfb_compatibility_display_functions = {
		lfb_compatibility_init,
};

SIFunctions *DisplayFuncs = &lfb_compatibility_display_functions;
