/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64.c	1.15"

/***
 ***	NAME
 ***
 ***		m64.c : Mach64 display library initialization entry point.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m64.h"
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles the interface between the board level
 ***	Mach64 library/SI  and the chipset level functionality.
 ***	Code for detection of board configuration, initialization of the 
 ***	chipset state and the register state. Initializes the default values 
 ***	for the mach64 registers. Mode releated checks are done to check if 
 *** 	the requested mode is supported by the library and the present 
 ***	board configuration.
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***	SEE ALSO
 ***		m64_state.c
 ***		m64_regs.c
 ***		g_omm.c
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	AUTHORS
 ***
 ***	HISTORY
 ***		First Version : Wed Jun 29 1994
 ***/

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

/*
 * System specific
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/kd.h>
#include <sys/errno.h>

#include <sys/inline.h>
#include "sidep.h"
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

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean m64_debug = 0;
#endif

PRIVATE

/***
 ***	Private declarations.
 ***/


/***
 ***	Includes.
 ***/
#include "generic.h"
#include "g_omm.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_state.h"
#include "m64_gbls.h"
#include "m64_mischw.h"
#include "m64_gs.h"
#include "m64_cmap.h"
#include "m64_cursor.h"

#undef PRIVATE

/*
 *For memory mapping register and framebuffer access
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "lfb_map.h"

/***
 ***	Constants.
 ***/
#define M64_INITIALIZE_CHIPSET_DETECTION_FAILED					(0x1 << 0U)
#define M64_INITIALIZE_VIDEO_MEMORY_SIZE_DETECTION_FAILED		(0x1 << 1U)
#define M64_INITIALIZE_BUS_KIND_DETECTION_FAILED				(0x1 << 2U)
#define M64_INITIALIZE_DAC_KIND_DETECTION_FAILED				(0x1 << 3U)
#define M64_INITIALIZE_MMAP_FAILED								(0x1 << 4U)
#define M64_INITIALIZE_MMAP_DEVICE_OPEN_FAILED					(0x1 << 5U)
#define M64_INITIALIZE_PARSE_USER_OPTIONS_FAILED				(0x1 << 6U)
#define M64_INITIALIZE_MEMORY_KIND_DETECTION_FAILED				(0x1 << 7U)
#define M64_INITIALIZE_KD_MODE_SWITCH_FAILED					(0x1 << 8U)
#define M64_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED				(0x1 << 9U)
#define M64_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND			(0x1 << 10U)
#define M64_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED				(0x1 << 11U)
#define M64_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC				(0x1 << 12U)
#define M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL				(0x1 << 13U)
#define M64_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE		(0x1 << 14U)

/*
 * Bit definitions for the flags field of the mode table entry.
 */
#define	INTERLACE												(0x1 << 0U)
#define NON_INTERLACE											(0x1 << 1U)

/***
 ***	Macros.
 ***/

#if (defined(USE_SYSI86))

/*
 * For the sysi86() call
 */
#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>

/*
 * v86.h doesn't exist in ESMP
 */
#ifndef SI86IOPL					/* ESMP */
#include <sys/v86.h>
#endif

#if (defined(__STDC__))

/*
 * Rename the function to what is present in the system library.
 */
#define sysi86 _abi_sysi86

/*
 * prototype.
 */
extern int sysi86(int, ...);

#endif /* STDC */

#if (!defined(SET_IOPL))

#ifdef SI86IOPL					/* ESMP */
#define SET_IOPL(iopl) _abi_sysi86(SI86IOPL, (iopl)>>12)
#else  /* SVR-4.2 */
#define SET_IOPL(iopl) _abi_sysi86(SI86V86, V86SC_IOPL, (iopl))
#endif
#endif /* SET_IOPL */
#endif /* USE_SYSI86 */

/***
 ***	Types.
 ***/
enum m64_display_mode
{
#define DEFINE_DISPLAY_MODE(MODENAME,DESCRIPTION, FLAGS,\
 	WIDTH, HEIGHT, REFRESH_RATE, CLOCK,\
 	HORZ_DISPLAY_TIME, HORZ_SYNC_START, HORZ_SYNC_END, HORZ_TOTAL,\
 	VERT_DISPLAY_TIME, VERT_SYNC_START, VERT_SYNC_END, VERT_TOTAL)\
	M64_##MODENAME
#include "m64_modes.def"
#undef DEFINE_DISPLAY_MODE
};

struct m64_display_mode_table_entry 
{
	enum m64_display_mode	display_mode;
	char	*mode_description;

	unsigned int flags;

	int width;				/* Horizontal displayed width */
	int height;				/* Vertical displayed width */
	int refresh_rate;		/* Vertical refresh rate. */
	int clock_frequency; 	/* Clock frequency required in KHZ */
	
	/* 
	 * The following fields are specified in dots.
	 */
	unsigned int	horizontal_active_display;	
	unsigned int horizontal_sync_start;
	unsigned int horizontal_sync_end;
	unsigned int horizontal_total;

	/*
	 * The following parameters are specified in lines.
	 */
	unsigned int vertical_active_display;
	unsigned int vertical_sync_start;
	unsigned int vertical_sync_end;
	unsigned int vertical_total;
};

/***
 ***	Variables.
 ***/
extern void m64__vt_switch_in__();
extern void m64__vt_switch_out__();
extern int  m64__initialize__(SIScreenRec *, struct m64_options_structure *);

/*
 * Table of known mode values.
 */
STATIC struct m64_display_mode_table_entry m64_display_mode_table[] =
{
#define DEFINE_DISPLAY_MODE(MODENAME, DESCRIPTION, FLAGS, \
 	WIDTH, HEIGHT, REFRESH_RATE, CLOCK,\
 	HORZ_DISPLAY_TIME, HORZ_SYNC_START, HORZ_SYNC_END, HORZ_TOTAL,\
 	VERT_DISPLAY_TIME, VERT_SYNC_START, VERT_SYNC_END, VERT_TOTAL)\
	{\
		M64_##MODENAME, DESCRIPTION, FLAGS,\
		WIDTH, HEIGHT, REFRESH_RATE, CLOCK,\
	 	HORZ_DISPLAY_TIME, HORZ_SYNC_START, HORZ_SYNC_END, HORZ_TOTAL,\
		VERT_DISPLAY_TIME, VERT_SYNC_START, VERT_SYNC_END, VERT_TOTAL\
	}
#include "m64_modes.def"
#undef DEFINE_DISPLAY_MODE
};

/***
 *** 	Functions.
 ***/

#if (defined(USE_KD_GRAPHICS_MODE))
/*
 * m64_set_virtual_terminal_mode
 *
 * Set the virtual terminal mode to KD_GRAPHICS or KD_TEXT.
 */
STATIC int
m64_set_virtual_terminal_mode(SIint32 virtual_terminal_file_descriptor, 
	int mode_type)
{

	ASSERT(mode_type == KD_GRAPHICS || mode_type == KD_TEXT0);
	
	/*
	 * Inform the kernel that the console is moving to "graphics
	 * mode". 
	 */

	if (ioctl(virtual_terminal_file_descriptor, KDSETMODE,
			  mode_type) == -1)
	{
		return (0);
	}
	return (1);
}
#endif /* USE_KD_GRAPHICS_MODE */

STATIC int
m64_get_board_configuration( SIConfigP config_p,
	struct m64_screen_state *screen_state_p,
	struct m64_options_structure *m64_options_p )
{
	int initialization_status = 0;
	unsigned long	scratch_reg0;
	unsigned long 	config_chip_id;
	unsigned long	config_cntl;
	unsigned long	config_stat0;
	unsigned long	mem_cntl;

	/*
	 * Try to detect the chip. See page 2.1 of programmers manual for details.
	 */
	scratch_reg0 = inl(M64_IO_REGISTER_SCRATCH_REG0);
	outl (M64_IO_REGISTER_SCRATCH_REG0, 0x55555555);
	if (inl(M64_IO_REGISTER_SCRATCH_REG0) != 0x55555555)
	{
		initialization_status |= M64_INITIALIZE_CHIPSET_DETECTION_FAILED;
	}
	else
	{
		outl (M64_IO_REGISTER_SCRATCH_REG0, 0xAAAAAAAA);
		if (inl(M64_IO_REGISTER_SCRATCH_REG0) != 0xAAAAAAAA)
		{
			initialization_status |= M64_INITIALIZE_CHIPSET_DETECTION_FAILED;
		}
	}
	outl (M64_IO_REGISTER_SCRATCH_REG0, scratch_reg0);
	if (initialization_status) return (initialization_status);
	

	/*
	 * Get the product class and revision number.
	 */
	config_chip_id = inl( M64_IO_REGISTER_CONFIG_CHIP_ID );
	screen_state_p->chipset_class = 
		( config_chip_id & CONFIG_CHIP_ID_CFG_CHIP_CLASS)  >> 
		CONFIG_CHIP_ID_CFG_CHIP_CLASS_SHIFT;
	screen_state_p->chipset_revision = 
		(config_chip_id & CONFIG_CHIP_ID_CFG_CHIP_REV) >> 
		CONFIG_CHIP_ID_CFG_CHIP_REV_SHIFT;

	/*
	 * Get the size of the installed memory on the board.
	 */
	if (! screen_state_p->video_memory_size )
	{
		mem_cntl = inl(M64_IO_REGISTER_MEM_CNTL);
		switch (mem_cntl & MEM_CNTL_MEM_SIZE)
		{
			case  MEM_CNTL_MEM_SIZE_512KB :
				screen_state_p->video_memory_size = 512*1024;
			break;
			case  MEM_CNTL_MEM_SIZE_1MB :
				screen_state_p->video_memory_size = 1024*1024;
			break;
			case  MEM_CNTL_MEM_SIZE_2MB :
			case  MEM_CNTL_MEM_SIZE_4MB :
			case  MEM_CNTL_MEM_SIZE_6MB :
			case  MEM_CNTL_MEM_SIZE_8MB :
				screen_state_p->video_memory_size = 
					(((mem_cntl & MEM_CNTL_MEM_SIZE) * 2 ) - 2) * 1024 * 1024;
			break;
			default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
			break;
		}
		if ((screen_state_p->video_memory_size >> 10) != config_p->videoRam)
		{
			(void) fprintf(stderr,M64_MEMORY_SIZE_OVERRIDE_MESSAGE,
				screen_state_p->video_memory_size >> 10,
				config_p->videoRam);
			screen_state_p->video_memory_size = config_p->videoRam << 10U;
		}

		/*
		 * We are in trouble if video memory size is 0 at this point.
		 */
		if (!screen_state_p->video_memory_size)
		{
			initialization_status |= 
				M64_INITIALIZE_VIDEO_MEMORY_SIZE_DETECTION_FAILED;
			return (initialization_status);
		}
	}

	/* 
	 * Get the card ID.
	 */
	config_stat0 = inl(M64_IO_REGISTER_CONFIG_STAT0);
	screen_state_p->card_id = (config_stat0 & CONFIG_STAT0_CFG_INIT_CARD_ID) >>
		CONFIG_STAT0_CFG_INIT_CARD_ID_SHIFT;

	/*
	 * Get the bus kind.
	 */
	if (! screen_state_p->bus_kind)
	{
		switch (config_stat0 & CONFIG_STAT0_CFG_BUS_TYPE)
		{
			case CONFIG_STAT0_CFG_BUS_TYPE_ISA :
				screen_state_p->bus_kind = M64_BUS_KIND_ISA;
				break;
			case CONFIG_STAT0_CFG_BUS_TYPE_EISA :
				screen_state_p->bus_kind = M64_BUS_KIND_EISA;
				break;
			case CONFIG_STAT0_CFG_BUS_TYPE_VLB :
				screen_state_p->bus_kind = M64_BUS_KIND_VLB;
				break;
			case CONFIG_STAT0_CFG_BUS_TYPE_PCI :
				screen_state_p->bus_kind = M64_BUS_KIND_PCI;
				break;
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				initialization_status |= 
					M64_INITIALIZE_BUS_KIND_DETECTION_FAILED;
				return (initialization_status);
		}
	}


	/*
	 * Memory aperture related details.
	 * Get the aperture size and position in physical address space.
	 */
	config_cntl = inl( M64_IO_REGISTER_CONFIG_CNTL);

	if (m64_options_p->memory_aperture_type == 
		M64_OPTIONS_MEMORY_APERTURE_TYPE_USE_BUILTIN_DEFAULT)
	{
		if (screen_state_p->bus_kind == M64_BUS_KIND_ISA)
		{
			/*CONSTANTCONDITION*/
			ASSERT(0);
		}
		else
		{
			/* 
			 * VLB/EISA/PCI buses. Enable large apertures.
			 */
			if ((screen_state_p->video_memory_size > 4096*1024) ||
				(m64_options_p->memory_aperture_size == 
				M64_OPTIONS_MEMORY_APERTURE_SIZE_USE_8MB_APERTURE))
			{
				/* 8 MB aperture. */
				screen_state_p->linear_aperture_location = 
					((config_cntl & CONFIG_CNTL_CFG_MEM_AP_LOC) >> 
					 (CONFIG_CNTL_CFG_MEM_AP_LOC_SHIFT + 1)) * 8 * 1024 * 1024;
				screen_state_p->linear_aperture_size = 8 * 1024 * 1024;
				screen_state_p->aperture_kind = 
					M64_APERTURE_KIND_BIG_LINEAR_APERTURE;
			}
			else
			{
				/* 4MB aperture.*/
				ASSERT(m64_options_p->memory_aperture_size == 
					M64_OPTIONS_MEMORY_APERTURE_SIZE_USE_4MB_APERTURE);

				screen_state_p->linear_aperture_location = 
					((config_cntl & CONFIG_CNTL_CFG_MEM_AP_LOC) >> 
					 CONFIG_CNTL_CFG_MEM_AP_LOC_SHIFT ) * 4 * 1024 * 1024;
				screen_state_p->linear_aperture_size = 4 * 1024 * 1024;
				screen_state_p->aperture_kind = 
					M64_APERTURE_KIND_BIG_LINEAR_APERTURE;
			}
		}
	}
	else if (m64_options_p->memory_aperture_type == 
		M64_OPTIONS_MEMORY_APERTURE_TYPE_SMALL_DUAL_PAGED)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		 
	}
	else if (m64_options_p->memory_aperture_type == 
		M64_OPTIONS_MEMORY_APERTURE_TYPE_BIG_LINEAR)
	{
		if ((screen_state_p->video_memory_size > 4096*1024) ||
			(m64_options_p->memory_aperture_size == 
			M64_OPTIONS_MEMORY_APERTURE_SIZE_USE_8MB_APERTURE))
		{
			/* 8 MB aperture. */
			screen_state_p->linear_aperture_location = 
				((config_cntl & CONFIG_CNTL_CFG_MEM_AP_LOC) >> 
				 (CONFIG_CNTL_CFG_MEM_AP_LOC_SHIFT + 1)) * 8 * 1024 * 1024;
			screen_state_p->linear_aperture_size = 8 * 1024 * 1024;
			screen_state_p->aperture_kind = 
				M64_APERTURE_KIND_BIG_LINEAR_APERTURE;
		}
		else
		{
			/* 4MB aperture.*/
			ASSERT(m64_options_p->memory_aperture_size == 
				M64_OPTIONS_MEMORY_APERTURE_SIZE_USE_4MB_APERTURE);

			screen_state_p->linear_aperture_location = 
				((config_cntl & CONFIG_CNTL_CFG_MEM_AP_LOC) >> 
				 CONFIG_CNTL_CFG_MEM_AP_LOC_SHIFT ) * 4 * 1024 * 1024;
			screen_state_p->linear_aperture_size = 4 * 1024 * 1024;
			screen_state_p->aperture_kind = 
				M64_APERTURE_KIND_BIG_LINEAR_APERTURE;
		}
	}

	/*
	 * Determine the dac kind. 
	 */
	if (!screen_state_p->dac_state_p)
	{
		int		return_value;

		return_value = m64_dac_detect_dac(screen_state_p);

		if (m64_options_p->dac_name == M64_OPTIONS_DAC_NAME_AUTO_DETECT)
		{
			if (return_value)
			{
				return (M64_INITIALIZE_DAC_KIND_DETECTION_FAILED);
			}
		}
		else
		{
			char	*detected_dac_name;

			detected_dac_name = allocate_memory(
				strlen(screen_state_p->dac_state_p->dac_name) + 1);
			strcpy(detected_dac_name, screen_state_p->dac_state_p->dac_name);

			screen_state_p->dac_state_p = NULL;

			switch (m64_options_p->dac_name)
			{
				case  M64_OPTIONS_DAC_NAME_ATI68875 	:
					break;
				case  M64_OPTIONS_DAC_NAME_BT476 		:
					break;
				case  M64_OPTIONS_DAC_NAME_BT481 		:
					break;
				case  M64_OPTIONS_DAC_NAME_STG1700 		:
					break;
				case  M64_OPTIONS_DAC_NAME_SC15021		:
					break;

				case  M64_OPTIONS_DAC_NAME_STG1702 		:
					screen_state_p->dac_state_p = 
						&(m64_dac_state_table[M64_DAC_STG1702]);
					break;

				case  M64_OPTIONS_DAC_NAME_ATI68860 	:
					screen_state_p->dac_state_p = 
						&(m64_dac_state_table[M64_DAC_ATI68860]);
					break;

				default :
					/*CONSTANTCONDITION*/
					ASSERT(0);
					break;
			}

			if (strcmp(detected_dac_name, 
				screen_state_p->dac_state_p->dac_name) != 0)
			{
				(void) fprintf(stderr,
					M64_DAC_OVERRIDE_MESSAGE,
					detected_dac_name,
					screen_state_p->dac_state_p->dac_name);
			}

			free_memory(detected_dac_name);
		}
	}

	/*
	 * Determine the clock chip kind.
	 */
	if (!screen_state_p->clock_chip_p)
	{
		if (m64_options_p->clock_chip_name == 
			M64_OPTIONS_CLOCK_CHIP_NAME_USE_BUILTIN_DEFAULT)
		{
			if (m64_clock_detect_clock_chip(screen_state_p))
			{
				return (M64_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED);
			}
		}
		/*
		 * otherwise allow for option based clock chip overrides.
		 * first we have to get the list of clock chips supported.
		 */
	}

	/*
	 * Determine the memory kind.
	 */
	 if (!screen_state_p->memory_kind)
	 {
		switch (config_stat0 & CONFIG_STAT0_CFG_MEM_TYPE)
		{
			case  CONFIG_STAT0_CFG_MEM_TYPE_DRAM_256Kx4 :
				screen_state_p->memory_kind = M64_MEMORY_KIND_DRAM_256Kx4;
				break;
			case  CONFIG_STAT0_CFG_MEM_TYPE_VRAM_256Kx4 :
				screen_state_p->memory_kind = M64_MEMORY_KIND_VRAM_256Kx4;
				break;
			case  CONFIG_STAT0_CFG_MEM_TYPE_VRAM_256Kx16S :
				screen_state_p->memory_kind = M64_MEMORY_KIND_VRAM_256Kx16S;
				break;
			case  CONFIG_STAT0_CFG_MEM_TYPE_DRAM_256Kx16 :
				screen_state_p->memory_kind = M64_MEMORY_KIND_DRAM_256Kx16;
				break;
			case  CONFIG_STAT0_CFG_MEM_TYPE_DRAM_256Kx16G :
				screen_state_p->memory_kind = M64_MEMORY_KIND_DRAM_256Kx16G;
				break;
			case  CONFIG_STAT0_CFG_MEM_TYPE_VRAM_256Kx4E :
				screen_state_p->memory_kind = M64_MEMORY_KIND_VRAM_256Kx4E;
				break;
			case  CONFIG_STAT0_CFG_MEM_TYPE_VRAM_256Kx16SE :
				screen_state_p->memory_kind = M64_MEMORY_KIND_VRAM_256Kx16SE;
				break;
			default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
				initialization_status |= 
					M64_INITIALIZE_MEMORY_KIND_DETECTION_FAILED;
				return(initialization_status);
		}
	}
	return(initialization_status);
}

STATIC int
m64_memory_map_registers_and_framebuffer(struct m64_screen_state
	 *screen_state_p)
{
	struct	lfbmap_struct	map; /* arguments for the MAP ioctl */
	struct	lfbmap_unit_struct	map_unit;
	int mmap_fd;

	mmap_fd = open(DEFAULT_M64_MMAP_DEVICE_PATH,O_RDWR);

	if (mmap_fd == -1)
	{
		perror("open");
		return (M64_INITIALIZE_MMAP_DEVICE_OPEN_FAILED);
	}

	map_unit.map_area_index = 0;
	map_unit.map_start_address_p = screen_state_p->linear_aperture_location;
	map_unit.map_length = screen_state_p->linear_aperture_size;
	
	map.map_count = 1;			/* one area to be mapped */
	map.map_struct_list_p = &map_unit;

	if (ioctl(mmap_fd, MAP_SETBOUNDS, &map) < 0)
	{
		perror("ioctl");
		(void) close(mmap_fd);
		return  -1;
	}

	screen_state_p->video_frame_buffer_base_address = 
		(void *)mmap(0, screen_state_p->linear_aperture_size,
			 PROT_READ|PROT_WRITE, MAP_SHARED,
			 mmap_fd, screen_state_p->linear_aperture_location);


	if ((caddr_t)screen_state_p->video_frame_buffer_base_address == 
		(caddr_t) -1)
	{
		close(mmap_fd);
		perror("mmap");
		return (M64_INITIALIZE_MMAP_FAILED);
	}

	screen_state_p->mmap_file_descriptor = mmap_fd;

	screen_state_p->register_mmap_area_base_address = (unsigned long *)
		((char *)screen_state_p->video_frame_buffer_base_address +
		(screen_state_p->linear_aperture_size == 4*1024*1024  ?
			M64_REGISTER_SET_OFFSET_IN_BYTES_FOR_4MB_APERTURE :
			M64_REGISTER_SET_OFFSET_IN_BYTES_FOR_8MB_APERTURE ));

	return 0;
}

function void
m64_save_registers(struct m64_screen_state *screen_state_p, 
	unsigned long *registers_p)
{
	volatile unsigned long 	*physical_base_p = 
		screen_state_p->register_mmap_area_base_address;
	volatile unsigned long	*memory_base_p = registers_p;
	int		i;

	/*
	 * Blind copy. A lot of unwanted area is copied but still the 
	 * code is very simple.
	 */
	for (i = 0; i < M64_REGISTER_SET_SIZE_IN_BYTES/sizeof(long); i++)
	{
		*memory_base_p++ = *physical_base_p++;
	}
	return;
}

STATIC int
m64_check_display_mode_feasibility(SIConfigP config_p, 
	struct m64_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	int						number_of_visuals;
	struct	generic_visual 	*visuals_p;
	struct m64_dac_state	*dac_state_p = screen_state_p->dac_state_p;

	/*
	 * Can the chipset support this mode, basically check memory requirements
	 * etc. 
	 */

	/*
	 * Memory requirements check first.
	 */
	if ((screen_state_p->video_memory_size * config_p->depth ) < 
		(config_p->virt_w * config_p->virt_h * config_p->depth))
	{
		/*
		 * Not enough memory to handle this mode.
		 */
		initialization_status |= 
			M64_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE;
	}

	/*
	 * Check if the clock chip can support the required clock frequency.
	 */
	 if(screen_state_p->clock_chip_p->check_support_for_frequency(
		screen_state_p->clock_frequency))
	{
		initialization_status |= M64_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED;
	}

	/*
	 * Add a seperate function in misc_hw.c if this becomes too complex.
	 * Check if the dac can support : 
	 * 1. The requested mode.
	 * 2. The list of visuals that are requested. We now check only for the
	 *    default visual since SI cannot handle multiple visuals anyway.
	 */



	if (screen_state_p->clock_frequency > dac_state_p->max_frequency)
	{
		initialization_status |= M64_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC;
	}
	visuals_p = screen_state_p->generic_state.screen_visuals_list_p;
	number_of_visuals = screen_state_p->generic_state.screen_number_of_visuals;
	ASSERT(visuals_p && (number_of_visuals > 0));
	initialization_status |= M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
	switch (visuals_p->dac_flags)
	{
		/*
		 * For static visuals we only fake staticness.
		 */
		case M64_VISUAL_PSEUDO_COLOR :
		case M64_VISUAL_STATIC_COLOR :
		case M64_VISUAL_STATIC_GRAY :
		case M64_VISUAL_GRAY_SCALE :
			if(((screen_state_p->generic_state.screen_depth == 4) &&
					(dac_state_p->visuals_supported & 
					M64_DAC_4_BIT_PSEUDO_COLOR_SUPPORTED)) ||
			 	((screen_state_p->generic_state.screen_depth == 8) &&
					(dac_state_p->visuals_supported & 
					M64_DAC_8_BIT_PSEUDO_COLOR_SUPPORTED)))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_TRUE_COLOR_16_555 :
			if((dac_state_p->visuals_supported & 
					M64_DAC_16_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
					M64_DAC_16_BIT_5_5_5_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_TRUE_COLOR_16_565 :
			if((dac_state_p->visuals_supported & 
					M64_DAC_16_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
					M64_DAC_16_BIT_5_6_5_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_DIRECT_COLOR_16_555 :
			if((dac_state_p->visuals_supported & 
					M64_DAC_16_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
					M64_DAC_16_BIT_5_5_5_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_DIRECT_COLOR_16_565 :
			if((dac_state_p->visuals_supported & 
					M64_DAC_16_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
					M64_DAC_16_BIT_5_6_5_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_TRUE_COLOR_24_RGB :
			if((dac_state_p->visuals_supported & 
					M64_DAC_24_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_24_bit_flags & 
					M64_DAC_24_BIT_R_G_B_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_DIRECT_COLOR_24_RGB :
			if((dac_state_p->visuals_supported & 
					M64_DAC_24_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_24_bit_flags & 
					M64_DAC_24_BIT_R_G_B_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_TRUE_COLOR_32_ARGB :
			if((dac_state_p->visuals_supported & 
					M64_DAC_32_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
					M64_DAC_32_BIT_A_R_G_B_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_DIRECT_COLOR_32_ARGB :
			if((dac_state_p->visuals_supported & 
					M64_DAC_32_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
					M64_DAC_32_BIT_A_R_G_B_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_TRUE_COLOR_32_RGBA :
			if((dac_state_p->visuals_supported & 
					M64_DAC_32_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
					M64_DAC_32_BIT_R_G_B_A_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_DIRECT_COLOR_32_RGBA :
			if((dac_state_p->visuals_supported & 
					M64_DAC_32_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
					M64_DAC_32_BIT_R_G_B_A_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_TRUE_COLOR_32_ABGR :
			if((dac_state_p->visuals_supported & 
					M64_DAC_32_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
					M64_DAC_32_BIT_A_B_G_R_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case M64_VISUAL_DIRECT_COLOR_32_ABGR :
			if((dac_state_p->visuals_supported & 
					M64_DAC_32_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
					M64_DAC_32_BIT_A_B_G_R_SUPPORTED))
			{
				initialization_status &= 
					~M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
	}

	/*
	 * Check if the monitor can take the display mode.
	 */

	return (initialization_status);
}

/*
 * m64_compute_initial_register_values
 * Computes the initial values of the registers for the requested mode 
 * and memory configuration. This function also provides reasonable defaults 
 * to all registers that will be used in the X server graphics mode.
 */ 
STATIC
m64_compute_initial_register_values(SIConfigP config_p, 
	struct m64_options_structure *options_p,
	struct m64_screen_state *screen_state_p)
{
	int		initialization_status = 0;
	volatile unsigned long	*registers_p = 
		screen_state_p->register_state.registers;
	volatile unsigned long	*saved_registers_p	= 
		screen_state_p->register_state.saved_registers;
	unsigned long	tmp;
	
	/*
	 * First get the appropriate display mode entry.
	 */
	if (options_p->crtc_parameters) 
	{
		struct m64_display_mode_table_entry	*user_display_mode_p;

		user_display_mode_p = (struct m64_display_mode_table_entry *)
			allocate_and_clear_memory(sizeof(
			struct m64_display_mode_table_entry));

		if(sscanf(options_p->crtc_parameters, 
			"%d %u %u %u %u %u %u %u %u %u", 
			&user_display_mode_p->clock_frequency,
			&user_display_mode_p->horizontal_active_display,	
			&user_display_mode_p->horizontal_sync_start,
			&user_display_mode_p->horizontal_sync_end,
			&user_display_mode_p->horizontal_total,
			&user_display_mode_p->vertical_active_display,
			&user_display_mode_p->vertical_sync_start,
			&user_display_mode_p->vertical_sync_end,
			&user_display_mode_p->vertical_total,
			&user_display_mode_p->flags) == 10)

		{
			user_display_mode_p->mode_description = 
				M64_DEFAULT_USER_MODE_DESCRIPTION;
			user_display_mode_p->width = config_p->disp_w;
			user_display_mode_p->height = config_p->disp_h;
			user_display_mode_p->refresh_rate = config_p->monitor_info.vfreq;

			screen_state_p->display_mode_p = user_display_mode_p;
		}
		else
		{
			free_memory(user_display_mode_p);
			(void) fprintf(stderr,
				M64_INITIALIZE_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION_MESSAGE,
				options_p->crtc_parameters);
		}
	}

	/*
	 * Neither the crtc parameters nor the board level module has initialized
	 * this field. Search through the list of available modes and find a match.
	 */
	if (!screen_state_p->display_mode_p)
	{
		enum m64_display_mode mode_count;
		struct m64_display_mode_table_entry *entry_p;

		for(mode_count = M64_MODE_NULL + 1,
			entry_p = &(m64_display_mode_table[M64_MODE_NULL + 1]);
			mode_count < M64_MODE_COUNT; 
			++mode_count, ++entry_p)
		{
			double refresh_difference = /* make a positive value */
				(config_p->monitor_info.vfreq > entry_p->refresh_rate) ?
				(config_p->monitor_info.vfreq - entry_p->refresh_rate) :
				(entry_p->refresh_rate - config_p->monitor_info.vfreq);
			/*
			 * look for a matching entry in the modetable :
			 */
			if ((config_p->disp_w == entry_p->width) &&
				(config_p->disp_h == entry_p->height) &&
				(refresh_difference < DEFAULT_M64_EPSILON))
			{
				/*
				 * Found a match.
				 */
				screen_state_p->display_mode_p = entry_p;
				break;
			}
		}
	}

	/*
	 * If display_mode_p is Null, we dont have the requested mode.
	 */
	if (!screen_state_p->display_mode_p)
	{
		initialization_status |= M64_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND;
		return (initialization_status);
	}

	/*
	 * The pixel port on the STG1702 dac is 16 bit wide.
	 * Hence, in 32 bit pixel modes, two clocks are required
	 * for a single pixel to be latched. For this reason, the 
	 * horizontal parameters and the clock frequency have to be 
	 * doubled.
	 */
	if( strcmp(screen_state_p->dac_state_p->dac_name, "M64_DAC_STG1702") == 0
		&& screen_state_p->generic_state.screen_depth == 32)
	{
		screen_state_p->display_mode_p->horizontal_active_display <<= 1;
		screen_state_p->display_mode_p->horizontal_sync_start <<= 1;
		screen_state_p->display_mode_p->horizontal_sync_end <<= 1;
		screen_state_p->display_mode_p->horizontal_total <<= 1;
		screen_state_p->display_mode_p->clock_frequency <<= 1;
	}

	screen_state_p->clock_frequency = 
		screen_state_p->display_mode_p->clock_frequency;

	/*
	 * 	Setup and control registers.
	 */
	/* 
	 *  bus_cntl: disable interrupts.
	 */
	registers_p[M64_REGISTER_BUS_CNTL_OFFSET] = 
		(saved_registers_p[M64_REGISTER_BUS_CNTL_OFFSET] & 
		~BUS_CNTL_BUS_FIFO_ERR_INT_EN) & BUS_CNTL_BITS;

	/*
	 * Crtc register values.
	 */
	tmp = saved_registers_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET]  & 
		CRTC_GEN_CNTL_BITS;
	tmp &= ~CRTC_GEN_CNTL_CRTC_PIX_BY_2_EN;
	tmp &= ~CRTC_GEN_CNTL_CRTC_PIX_WID;
	switch (config_p->depth)
	{
		case 4:
			tmp |= CRTC_GEN_CNTL_CRTC_PIX_WID_4;
			tmp |= CRTC_GEN_CNTL_CRTC_BYTE_PIX_ORDER;
			break;
		case 8:
		case 24:
			tmp |= CRTC_GEN_CNTL_CRTC_PIX_WID_8;
			break;
		case 16:
			if (options_p->dac_16_bit_color_mode == 
				M64_OPTIONS_DAC_16_BIT_COLOR_MODE_555)
			{
				tmp |= CRTC_GEN_CNTL_CRTC_PIX_WID_15;
			}
			else
			{
				tmp |= CRTC_GEN_CNTL_CRTC_PIX_WID_16;
			}
			break;
		case 32:
			/*
	 		 * The pixel port on the STG1702 dac is 16 bit wide.
			 * Hence, the pixel depth from CRTC point of view is
			 * set to 16. All the pixels will be covered by doubling
			 * the CRTC pitch in the CRTC_OFF_PITCH register.
			 */
			if( strcmp(screen_state_p->dac_state_p->dac_name, "M64_DAC_STG1702")
				== 0 && screen_state_p->generic_state.screen_depth == 32)
			{
				tmp |= CRTC_GEN_CNTL_CRTC_PIX_WID_16;
			}
			else
			{
				tmp |= CRTC_GEN_CNTL_CRTC_PIX_WID_32;
			}
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}
	registers_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] = tmp;

	/*
	 * There is a lot of flicker and screen disturbance with DRAM
	 * cards. 
	 */
	if ((screen_state_p->memory_kind == M64_MEMORY_KIND_DRAM_256Kx4) ||
		(screen_state_p->memory_kind ==  M64_MEMORY_KIND_DRAM_256Kx16))
	{
		/*
		 * The values are a result of an experiment on the developement 
		 * machine with the ATI Xpression card, for 8 bit modes.
		 * OKI 60ns 256KBx4 Dram.
		 */
		registers_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] &= 
			~CRTC_GEN_CNTL_CRTC_FIFO_LWM;

		if (options_p->crtc_display_fifo_low_watermark)
		{
			tmp = options_p->crtc_display_fifo_low_watermark;
		}
		else
		{
			switch (screen_state_p->generic_state.screen_displayed_width)
			{
				case 1280:
					tmp = 8;
				break;
				case 1152:
					tmp = 7;
				break;
				case 1024:
				case 800:
				case 640:
					tmp = 5;
				break;
				default:
					tmp = 0xf;
				break;
			}
			if (screen_state_p->generic_state.screen_depth > 8)
			{
				if ((tmp * (screen_state_p->generic_state.screen_depth / 8)) 
					< 0xf)
				{
					tmp *= (screen_state_p->generic_state.screen_depth / 8) ;
				}
				else
				{
					tmp = 0xf;
				}
			}
		}

		tmp = (tmp & 0xf) << CRTC_GEN_CNTL_CRTC_FIFO_LWM_SHIFT;

		registers_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] |= tmp; 
	}

	registers_p[M64_REGISTER_CRTC_H_SYNC_STRT_WID_OFFSET] =
		(screen_state_p->display_mode_p->horizontal_sync_start >> 3U) |
		(((screen_state_p->display_mode_p->horizontal_sync_end - 
		screen_state_p->display_mode_p->horizontal_sync_start) >> 3U) <<
		CRTC_H_SYNC_STRT_WID_CRTC_H_SYNC_WID_SHIFT);

	registers_p[M64_REGISTER_CRTC_H_TOTAL_DISP_OFFSET] =
		(screen_state_p->display_mode_p->horizontal_total >> 3U) |
		(((screen_state_p->display_mode_p->horizontal_active_display >> 3U) 
		- 1 ) << CRTC_H_TOTAL_DISP_CRTC_H_DISP_SHIFT);

	registers_p[M64_REGISTER_CRTC_INT_CNTL_OFFSET] = 
		(saved_registers_p[M64_REGISTER_CRTC_INT_CNTL_OFFSET] & 
		~CRTC_INT_CNTL_CRTC_VLINE_INT_EN & ~CRTC_INT_CNTL_CRTC_VBLANK_INT_EN) &
		CRTC_INT_CNTL_BITS;

	/*
	 * The pixel port on the STG1702 dac is 16 bit wide.
	 * Hence, the pixel depth from CRTC point of view is
	 * set to 16. All the pixels will be covered by doubling
	 * the CRTC pitch in the CRTC_OFF_PITCH register.
	 */
	if( strcmp(screen_state_p->dac_state_p->dac_name, "M64_DAC_STG1702") == 0
		&& screen_state_p->generic_state.screen_depth == 32)
	{
		registers_p[M64_REGISTER_CRTC_OFF_PITCH_OFFSET] = 
			((unsigned)((config_p->virt_w * 2) / 8)) << 
			CRTC_OFF_PITCH_CRTC_PITCH_SHIFT;
	}
	else
	{
		registers_p[M64_REGISTER_CRTC_OFF_PITCH_OFFSET] = 
			((unsigned)(config_p->virt_w / 8)) << 
			CRTC_OFF_PITCH_CRTC_PITCH_SHIFT;
	}

	registers_p[M64_REGISTER_CRTC_V_SYNC_STRT_WID_OFFSET] = 
		screen_state_p->display_mode_p->vertical_sync_start |
		((screen_state_p->display_mode_p->vertical_sync_end - 
		screen_state_p->display_mode_p->vertical_sync_start) <<
		CRTC_V_SYNC_STRT_WID_CRTC_V_SYNC_WID_SHIFT);

	registers_p[M64_REGISTER_CRTC_V_TOTAL_DISP_OFFSET] =
		(screen_state_p->display_mode_p->vertical_total) |
		((screen_state_p->display_mode_p->vertical_active_display - 1 ) << 
		CRTC_V_TOTAL_DISP_CRTC_V_DISP_SHIFT);

	/*
	 * dac_cntl register.
	 */
	registers_p[M64_REGISTER_DAC_CNTL_OFFSET] = 
		((saved_registers_p[M64_REGISTER_DAC_CNTL_OFFSET]  & 
		~DAC_CNTL_DAC_TYPE) & DAC_CNTL_BITS) | 
		(screen_state_p->dac_state_p->dac_id << DAC_CNTL_DAC_TYPE_SHIFT);
	
	/*
	 * gen_test_cntl register.
	 */
	registers_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] = 
		(saved_registers_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] & 
		GEN_TEST_CNTL_BITS) & ~GEN_TEST_CNTL_GEN_CUR_EN;

	/*
	 * Leave blocked writes at the bios default.
	 * Override in case the * user wants to do so.
	 */
	if (options_p->enable_blocked_memory_write != 
		M64_OPTIONS_ENABLE_BLOCKED_MEMORY_WRITE_DEFAULT)
	{
		if (options_p->enable_blocked_memory_write == 
			M64_OPTIONS_ENABLE_BLOCKED_MEMORY_WRITE_YES)
		{
			registers_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] |= 
				GEN_TEST_CNTL_GEN_BLOCK_WR_EN;
		}
		else
		{
			registers_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] &= 
				~GEN_TEST_CNTL_GEN_BLOCK_WR_EN;
		}
	}

	if (options_p->cursor_type == M64_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR)
	{
		registers_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] |= 
			GEN_TEST_CNTL_GEN_CUR_EN;
	}
	
	/*
	 * Initialize the h/w cursor width and height to 0. just in case we dont
	 * use it, we dont want to see a blob on the screen.
	 */
	registers_p[M64_REGISTER_CUR_HORZ_VERT_OFF_OFFSET] = 
		CUR_MAX_HORZ_OFFSET_VALUE |
		(CUR_MAX_VERT_OFFSET_VALUE << CUR_HORZ_VERT_OFF_CUR_VERT_OFFSET_SHIFT);

	/*
	 * GUI Engine trajactory register. dst_off_pitch.
	 */
	registers_p[M64_REGISTER_DST_OFF_PITCH_OFFSET] = 
		(screen_state_p->generic_state.screen_physical_width << 3 ) 
		<< DST_OFF_PITCH_DST_PITCH_SHIFT;
	
	return(initialization_status);
}

STATIC SIBool
m64_virtual_terminal_save(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	int		i;
	char	*dst_p;
	char 	*src_p;
	size_t	physical_width_in_bytes;
	SIBool	return_value = SI_SUCCEED;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

	
	/*
	 * Wait for the graphics engine to finish with all its commands.
	 */
	M64_WAIT_FOR_GUI_ENGINE_IDLE();

	/*
	 * Save the contents of offscreen memory if necessary.
	 * Allocate the space for saving the video memory in system memory space.
	 */
	if (screen_state_p->vt_switch_save_lines > 0)
	{
		if (generic_current_screen_state_p->screen_contents_p == NULL)
		{
			generic_current_screen_state_p->screen_contents_p =
				allocate_memory((screen_state_p->vt_switch_save_lines *
					screen_state_p->generic_state.screen_physical_width * 
					screen_state_p->generic_state.screen_depth) >> 3);
		}
	
		/*
		 * if the allocation succeeded, we store the framebuffer.
		 */
		if (generic_current_screen_state_p->screen_contents_p != NULL) 
		{
			/*
			 * Save the first vt_switch_save_lines of the frame buffer memory.
			 */
			src_p = (char *)screen_state_p->video_frame_buffer_base_address;
			dst_p = (char *)generic_current_screen_state_p->screen_contents_p;
			physical_width_in_bytes = screen_state_p->framebuffer_stride;

			for (i = 0; i < screen_state_p->vt_switch_save_lines; i++)
			{
				(void) memcpy((void*)dst_p, (void*)src_p, 
					physical_width_in_bytes);
				src_p += physical_width_in_bytes;
				dst_p += physical_width_in_bytes;
			}
		}
		else
		{
			(void) fprintf(stderr, M64_MEMORY_ALLOCATION_FAILED_MESSAGE);
		}
	}
	
	/*
	 * Inform each concerned module that we are about to lose control
	 * of the VT.
	 */
	m64__vt_switch_out__();

	/*
	 * Wait for the graphics engine to finish with all its commands.
	 */
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
	
	/*
	 * Restore the chipset state.
	 */
	if ((screen_state_p->register_state.generic_state.register_put_state)
		((struct generic_screen_state *) screen_state_p, 
		REGISTER_PUT_STATE_SAVED_STATE))
	{
#if (defined(__DEBUG__))
		if (m64_debug)
		{
			(void) fprintf(debug_stream_p, "(m64_virtual_terminal_save)"
				"register_put_state call failed.\n");
		}
#endif
		return(SI_FAIL);
	}

	/*
	 * Finally program the mem_cntl.
	 * Done at the end because it may disable apertures.
	 */
	register_base_address_p[M64_REGISTER_MEM_CNTL_OFFSET] = 
		screen_state_p->register_state.saved_mem_cntl &  MEM_CNTL_BITS;
	
#if (defined(USE_KD_GRAPHICS_MODE))
	if (!m64_set_virtual_terminal_mode(screen_state_p->generic_state.
		screen_virtual_terminal_file_descriptor,KD_TEXT0))
	{
		return (SI_FAIL);
	}
#endif

	return (return_value);
}


STATIC SIBool
m64_virtual_terminal_restore()
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	int		i;
	char	*dst_p;
	char 	*src_p;
	size_t	physical_width_in_bytes;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

#if (defined(USE_KD_GRAPHICS_MODE))
	if (!m64_set_virtual_terminal_mode(screen_state_p->generic_state.
		screen_virtual_terminal_file_descriptor,KD_GRAPHICS))
	{
		return (SI_FAIL);
	}
#endif
	/*
	 * Initialize the chipset and the dac registers that are to be
	 * programmed for a mode switch.
	 */
	if ((*screen_state_p->register_state.generic_state.register_put_state)
		((struct generic_screen_state *) screen_state_p,
		REGISTER_PUT_STATE_XSERVER_STATE))
	{
#if (defined(__DEBUG__))
		if (m64_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(m64_virtual_terminal_restore) register "
						   "init failed.\n");
		}
#endif
		return (SI_FAIL);
	}

	/*
	 * Finally program the mem_cntl but do it before calling module
	 * specific vt restore initializations.
	 */
	register_base_address_p[M64_REGISTER_MEM_CNTL_OFFSET] = 
		screen_state_p->register_state.mem_cntl &  MEM_CNTL_BITS;

	M64_WAIT_FOR_GUI_ENGINE_IDLE();
	/*
	 * Restore the first vt_switch_save lines of the frame buffer
	 * memory that is saved in system memory.
	 */
	if (generic_current_screen_state_p->screen_contents_p != NULL)
	{
		dst_p = (char *)screen_state_p->video_frame_buffer_base_address;
		src_p = (char *)generic_current_screen_state_p->screen_contents_p;
		physical_width_in_bytes = screen_state_p->framebuffer_stride;

		for (i = 0; i < screen_state_p->vt_switch_save_lines; i++)
		{
			(void) memcpy((void*)dst_p, (void*)src_p, physical_width_in_bytes);
			src_p += physical_width_in_bytes;
			dst_p += physical_width_in_bytes;
		}
	}

	/*
	 * Inform other interested modules that we are about to regain
	 * control of our VT.
	 */
	m64__vt_switch_in__();

	return (SI_SUCCEED);
}

STATIC SIBool
m64_restore_screen(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (m64_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_restore_screen) restore_screen called.\n");
	}
#endif	

	/*
	 * Last time around, we do not want to save the x server screen.
	 */
	screen_state_p->vt_switch_save_lines = 0;

	/*
	 * We dont care about the return value at this point.
	 */
	(void) m64_virtual_terminal_save();

	/*
	 * Unmap the linear address space and close the fd.
	 */
	if (munmap(screen_state_p->video_frame_buffer_base_address,
		screen_state_p->linear_aperture_size) == -1)
	{
		perror("munmap");
		return (SI_FAIL);
	}

	(void)close(screen_state_p->mmap_file_descriptor);

	return (SI_SUCCEED);
}

/* 
 * m64_initialize_display_library
 */
function int
m64_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
	SIScreenRec *si_screen_p, struct m64_screen_state *screen_state_p)
{

	SIFlagsP flags_p;
	SIFunctions saved_generic_functions;
	int initialization_status = 0;	/* error flags returned to caller */
	char options_string[M64_DEFAULT_OPTION_STRING_SIZE];
	int options_string_size = sizeof(options_string);
	struct m64_options_structure *m64_options_p;
	SIConfigP config_p = si_screen_p->cfgPtr;
	
#if (defined(__DEBUG__))

	extern void m64_debug_control(boolean);
	
	m64_debug_control(TRUE);

	if (m64_debug)
	{
		(void) fprintf (debug_stream_p,
			"(m64_initialize_display_library){\n"
			"\tvirtual_terminal_descriptor = %d\n"
			"\tsi_screen_p = 0x%x\n"
			"\tm64_screen_state_p = 0x%x\n"
			"}\n",
			virtual_terminal_file_descriptor,
			(unsigned) si_screen_p,
			(unsigned) screen_state_p);
	}

#endif /* __DEBUG__ */


	ASSERT(screen_state_p != NULL);
	ASSERT(si_screen_p != NULL);

	flags_p = si_screen_p->flagsPtr;

	(void) fprintf(stderr, M64_MESSAGE_STARTUP);
	
	/*
	 * Initialize the generic data structure.
	 */

	if (initialization_status = 
		generic_initialize_display_library(virtual_terminal_file_descriptor,
		   si_screen_p, (struct generic_screen_state *) screen_state_p))
	{

#if (defined(__DEBUG__))
		if (m64_debug)
		{
			(void) fprintf(debug_stream_p, 
						   "generic initialization failed.\n");
		}
#endif /* __DEBUG__ */

		return (initialization_status);

	}

	/*
	 * Tuck away a copy of the generic screen functions.
	 */

	saved_generic_functions = *si_screen_p->funcsPtr;
	/*
	 * Convenient macro.
	 */

#define APPEND_STRING(destination, source_string_p, free_size)	\
	{															\
		const char *source_p = source_string_p;					\
		int source_size = strlen(source_p) + 1;					\
		if (free_size < source_size)							\
		{														\
			(void) fprintf(stderr, 								\
				M64_MESSAGE_BUFFER_SIZE_EXCEEDED,				\
				destination, source_p);							\
			return (SI_FAIL);									\
		}														\
		else													\
		{														\
			strncat(destination, source_p, free_size);			\
			free_size -= source_size;							\
		}														\
	}

	/*
	 * User option handling.  Null terminate the buffer first.
	 */

	options_string[0] = '\0';
	
	/*
	 * Add a file parsing option if the standard options file is
	 * present.
	 */

	if (access(M64_DEFAULT_STANDARD_OPTION_FILE_NAME, F_OK | R_OK) == 0)
	{
		APPEND_STRING(options_string,
					  M64_DEFAULT_PARSE_STANDARD_OPTIONS_FILE_COMMAND, 
					  options_string_size);
	}

	/*
	 * Add parsing options for the user specified option string.
	 */

	APPEND_STRING(options_string, config_p->info, options_string_size);
	APPEND_STRING(options_string, " ", options_string_size);

	/*
	 * Add parsing options for the standard environment option variables 
	 */
	APPEND_STRING(options_string,
				  M64_DEFAULT_PARSE_STANDARD_ENVIRONMENT_VARIABLE_COMMAND,
				  options_string_size);
	
	/*
	 * parse user configurable options.
	 */
	if (!(m64_options_p = 
		  m64_options_parse(NULL, options_string)))
	{
		
#if (defined(__DEBUG__))
		if (m64_debug)
		{
			(void) fprintf(debug_stream_p, 
						   "parsing of user options failed.\n"); 
		}
#endif /* __DEBUG__ */
		
		initialization_status |=
			M64_INITIALIZE_PARSE_USER_OPTIONS_FAILED;

		return (initialization_status);

	}
	else
	{
		/*
		 * Save the options for later persual by code.
		 */
		screen_state_p->options_p = m64_options_p;
	}

	/*
	 * Set the SDD version number as requested.
	 */
	if (m64_options_p->si_interface_version !=
		M64_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE)
	{
		switch (m64_options_p->si_interface_version)
		{
		case M64_OPTIONS_SI_INTERFACE_VERSION_1_0:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version = DM_SI_VERSION_1_0;
			break;
		case M64_OPTIONS_SI_INTERFACE_VERSION_1_1:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version = DM_SI_VERSION_1_1;
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}
	}

#if (defined(USE_KD_GRAPHICS_MODE))
	if (!m64_set_virtual_terminal_mode(virtual_terminal_file_descriptor,
		KD_GRAPHICS))
	{
		initialization_status |= M64_INITIALIZE_KD_MODE_SWITCH_FAILED;
		return (initialization_status);
	}
#endif

#if (defined(USE_SYSI86))

	/*
	 * Note that we need this for exactly one register. CONFIG_CNTL
	 * which cannot be memory mapped in any way.
	 * The X server needs access to many I/O addresses.  We use the
	 * `sysi86()' call to get access to these.  Another alternative to
	 * this approach would be to have a kernel driver which
	 * allows the X server process access to the ATI registers.
	 */
	if (SET_IOPL(PS_IOPL) < 0)
	{
		perror(M64_CANNOT_ENABLE_REGISTER_ACCESS_MESSAGE);
		free_memory(m64_options_p);
		return (SI_FAIL);
	}
#endif /* USE_SYSI86 */
	
	/*
	 * Get the board configuration. This includes amount of memory,
	 * bus type, dac kind etc...
	 */
	initialization_status |= m64_get_board_configuration( config_p,
		screen_state_p, m64_options_p);
	if (initialization_status)
	{
#if (defined(__DEBUG__))
		if (m64_debug)
		{
			(void) fprintf(debug_stream_p, 
				"(m64_initialize_display_library)"
				"\tFailed to get board data.\n");
		}
#endif /* __DEBUG__ */
		return(initialization_status);
	}

	/*
	 * Override the dac maximum frequency if specified.
	 */
	if (m64_options_p->dac_max_frequency)
	{
		screen_state_p->dac_state_p->max_frequency = 
			m64_options_p->dac_max_frequency;
		(void) fprintf(stderr, M64_DAC_MAX_FREQUENCY_OVERRIDE_MESSAGE,
			screen_state_p->dac_state_p->max_frequency, 
			m64_options_p->dac_max_frequency);
	}

	/*
	 * Allocate space for the register states fields. This consists of the
	 * save state and current state pointers.
	 */
	screen_state_p->register_state.saved_registers = (unsigned long *)
		allocate_and_clear_memory(M64_REGISTER_SET_SIZE_IN_BYTES);
	screen_state_p->register_state.registers = (unsigned long *)
		allocate_and_clear_memory(M64_REGISTER_SET_SIZE_IN_BYTES);

	/*
	 * Save the value of the config control register. Note that this 
	 * register is io mapped and cant be memory mapped.  Also save the
	 * value of the mem_cntl register.
	 */
	screen_state_p->register_state.saved_config_cntl = 
		inl(M64_IO_REGISTER_CONFIG_CNTL) & 0xFFFF7;
	screen_state_p->register_state.saved_mem_cntl = 
		inl(M64_IO_REGISTER_MEM_CNTL) & 0x7FFF7;

	/*
	 * Memory map the framebuffer. Allow register access by 
	 * 1. enabling the linear memory aperture in config control register.
	 *    disable vga apertures.
	 * 2. Disabling the memory boundary in the mem_cntl register.
	 */
	if (screen_state_p->aperture_kind == M64_APERTURE_KIND_BIG_LINEAR_APERTURE)
	{
		m64_memory_map_registers_and_framebuffer(screen_state_p);
		screen_state_p->register_state.config_cntl = 
			screen_state_p->register_state.saved_config_cntl & 
			~(CONFIG_CNTL_CFG_MEM_VGA_AP_EN | CONFIG_CNTL_CFG_MEM_AP_SIZE);
		screen_state_p->register_state.config_cntl |= 
			(screen_state_p->linear_aperture_size == 4 * 1024 * 1024 ? 
				CONFIG_CNTL_CFG_MEM_AP_SIZE_4MB	: 
				CONFIG_CNTL_CFG_MEM_AP_SIZE_8MB);
	}
	else
	{
		/* WE DONT SUPPORT SMALL DUAL PAGED APERTURES AS OF NOW. */
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return(SI_FAIL);
	}

#ifdef DELETE
	screen_state_p->register_state.mem_cntl = (~MEM_CNTL_MEM_BNDRY_EN) & 
		screen_state_p->register_state.saved_mem_cntl; 
#endif
	screen_state_p->register_state.mem_cntl = 
		screen_state_p->register_state.saved_mem_cntl & 
		~(MEM_CNTL_MEM_BNDRY_EN | MEM_CNTL_MEM_SIZE);
	switch(screen_state_p->video_memory_size)
	{
		case 512*1024:
			screen_state_p->register_state.mem_cntl |= MEM_CNTL_MEM_SIZE_512KB;
			break;
		case 1024*1024:
			screen_state_p->register_state.mem_cntl |= MEM_CNTL_MEM_SIZE_1MB;
			break;
		case 2048*1024:
		case 4096*1024:
		case 6144*1024:
		case 8192*1024:
			screen_state_p->register_state.mem_cntl |= 
				((screen_state_p->video_memory_size /(1024*1024)) + 2) / 2;
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

	outl(M64_IO_REGISTER_CONFIG_CNTL, 
		screen_state_p->register_state.config_cntl);
	outl(M64_IO_REGISTER_MEM_CNTL , screen_state_p->register_state.mem_cntl);

	/*
	 * Determine screen physical dimensions.
	 */
	screen_state_p->generic_state.screen_physical_width = 
		(config_p->virt_w > config_p->disp_w) ?
		config_p->virt_w : config_p->disp_w;
	screen_state_p->generic_state.screen_physical_height = 
		(int) (screen_state_p->video_memory_size << 3U) /
		(screen_state_p->generic_state.screen_physical_width * config_p->depth);

	if (screen_state_p->video_memory_size == 4096*1024)
	{
		screen_state_p->generic_state.screen_physical_height -= 2;
	}

	/*
	 * Save the original register state.
	 */
	(void) m64_save_registers(screen_state_p,
		screen_state_p->register_state.saved_registers);

	/* 
	 * Program default initial values for the registers.
	 */
	 initialization_status |= m64_compute_initial_register_values(config_p, 
		m64_options_p,screen_state_p);
	 if (initialization_status) return (initialization_status);


	/*
	 * How many lines of the screen does the user want saved?
	 * Limit this to the max video memory lines available.
	 */
	screen_state_p->vt_switch_save_lines =
		(((m64_options_p->vt_switch_save_lines <= 0) ||
				(m64_options_p->vt_switch_save_lines >
				screen_state_p->generic_state.screen_physical_height)) ? 
			screen_state_p->generic_state.screen_physical_height : 
			m64_options_p->vt_switch_save_lines);

	/*
	 * Set the loop timeout value.
	 */
	m64_graphics_engine_loop_timeout_count =
		m64_options_p->graphics_engine_loop_timeout_count;

	ASSERT(m64_graphics_engine_loop_timeout_count > 0);
	
	/*
	 * Set the micro delay count.
	 */
	m64_graphics_engine_micro_delay_count =
		m64_options_p->graphics_engine_micro_delay_count;

	ASSERT(m64_graphics_engine_micro_delay_count > 0);

	/*
	 * We cannot pass the framebuffer pointer up for small apertures.
	 * Si has no idea of the select page mechanism. Moreover 
	 * core server seems to have a bug in the 16bit drawing modes when
	 * directly accessing the framebuffer memory.
	 */
	if ((m64_options_p->framebuffer_access_for_core_server ==
			M64_OPTIONS_FRAMEBUFFER_ACCESS_FOR_CORE_SERVER_YES) &&
		(screen_state_p->aperture_kind == 
			M64_APERTURE_KIND_BIG_LINEAR_APERTURE))
	{
		flags_p->SIfb_pbits = screen_state_p->video_frame_buffer_base_address;
		flags_p->SIfb_width = 
			screen_state_p->generic_state.screen_physical_width;
	}
	else
	{
		flags_p->SIfb_pbits = SI_FB_NULL; 
		flags_p->SIfb_width = 0;
	}

	si_screen_p->funcsPtr->si_vt_save = m64_virtual_terminal_save;
	si_screen_p->funcsPtr->si_vt_restore = m64_virtual_terminal_restore;
	si_screen_p->funcsPtr->si_restore = m64_restore_screen;
	
	/*
	 * Initialize the off-screen memory manager.
	 */
	 {
		struct omm_initialization_options omm_options; 

		/*
		 * Create the initialization string for the off-screen memory
		 * manager if required.
		 */

		char *buffer_p;

		char *cursor_named_allocation_string_p = 
			m64_cursor_make_named_allocation_string(si_screen_p, m64_options_p);

		/* 
		 * Add one for the comma which will seperate each named allocate
		 * string
		 */

		buffer_p = (char *)allocate_memory(
			DEFAULT_M64_OMM_INITIALIZATION_STRING_LENGTH + 1 +
			strlen(cursor_named_allocation_string_p) + 1 +
			strlen(m64_options_p->omm_named_allocation_list));

		/*
		 *See if the user has supplied any additional named allocate
		 *requests
		 */

		if (m64_options_p->omm_named_allocation_list &&
			m64_options_p->omm_named_allocation_list[0] != '\0')
		{
			sprintf(buffer_p, DEFAULT_M64_OMM_INITIALIZATION_FORMAT ",%s,%s",
					DEFAULT_M64_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height,
					screen_state_p->generic_state.screen_depth,
					0,0,0,
					cursor_named_allocation_string_p,
					m64_options_p->omm_named_allocation_list);
		}
		else
		{
			sprintf(buffer_p, DEFAULT_M64_OMM_INITIALIZATION_FORMAT ",%s",
					DEFAULT_M64_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height,
					screen_state_p->generic_state.screen_depth,
					0,0,0,
					cursor_named_allocation_string_p);
		}
		
		omm_options.total_width =
		 screen_state_p->generic_state.screen_physical_width;

		omm_options.total_height =
		   screen_state_p->generic_state.screen_physical_height;

		omm_options.total_depth = screen_state_p->generic_state.screen_depth;

		omm_options.horizontal_constraint = 
			(m64_options_p->omm_horizontal_constraint ?
				m64_options_p->omm_horizontal_constraint : 
				(1U << screen_state_p->pixels_per_long_shift));

		omm_options.vertical_constraint = 
		   (m64_options_p->omm_vertical_constraint ?
				m64_options_p->omm_vertical_constraint :
				DEFAULT_M64_OMM_VERTICAL_CONSTRAINT);

		omm_options.neighbour_list_increment = 
			m64_options_p->omm_neighbour_list_increment;

		omm_options.full_coalesce_watermark = 
			m64_options_p->omm_full_coalesce_watermark;

		omm_options.hash_list_size = m64_options_p->omm_hash_list_size;

		omm_options.named_allocations_p = buffer_p;

		 (void)omm_initialize(&omm_options);

		 free_memory(buffer_p);
	}

	/*
	 * Let each module initialize itself.
	 */
	m64__initialize__(si_screen_p, m64_options_p);

	/*
	 * We have all information about the requested display mode , physical
	 * screen dimensions and video memory installed. Check if the 
	 * chipset/dac/clock/monitor combination can support these for display
	 * and drawing purposes. This is done here because by this time
	 * the individual module initialization also has gone through including
	 * the number of visuals we need to support etc.
	 */
	initialization_status |= 
		m64_check_display_mode_feasibility(config_p,screen_state_p);
	if (initialization_status) return (initialization_status);

	if(screen_state_p->generic_state.screen_current_graphics_state_p)
	{
		int count;

		for(count = 0; count <
			screen_state_p->generic_state.screen_number_of_graphics_states; 
			count ++)
		{
			struct m64_graphics_state *graphics_state_p = 
				(struct m64_graphics_state *) screen_state_p->generic_state.
				screen_graphics_state_list_pp[count];
					   
			ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
			 (struct generic_graphics_state *) graphics_state_p));
	
			ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

			/*
			 * Save the generic function table
			 */

			graphics_state_p->generic_si_functions = saved_generic_functions;
		}
	}
	
	/*
	 * Verbose startup messages.
	 */
	if (m64_options_p->verbose_startup > 0)
	{
		unsigned long	*registers_p = 
			screen_state_p->register_state.registers;

		(void) fprintf(stderr, M64_VERBOSE_STARTUP_PROLOGUE);
		
		(void) fprintf(stderr,
			M64_VERBOSE_STARTUP_SDD_PARAMETERS_MESSAGE,
			screen_state_p->generic_state.screen_server_version_number,
			screen_state_p->generic_state.screen_sdd_version_number,
			((m64_options_p->si_interface_version ==
				M64_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE) ? 
				M64_VERBOSE_STARTUP_AUTO_CONFIGURED :
				M64_VERBOSE_STARTUP_USER_SPECIFIED)); 

		(void) fprintf(stderr,
			M64_VERBOSE_STARTUP_BOARD_CONFIGURATION_MESSAGE,
			config_p->model,
			screen_state_p->card_id,
			screen_state_p->chipset_class,
			screen_state_p->chipset_revision,
			screen_state_p->dac_state_p->dac_name,
			(m64_options_p->dac_name == M64_OPTIONS_DAC_NAME_AUTO_DETECT ?
				M64_VERBOSE_STARTUP_AUTO_DETECTED :
				M64_VERBOSE_STARTUP_USER_SPECIFIED),
			m64_bus_kind_to_description[screen_state_p->bus_kind],
			screen_state_p->clock_chip_p->clock_chip_name_p,
			(m64_options_p->clock_chip_name == 
				M64_OPTIONS_CLOCK_CHIP_NAME_USE_BUILTIN_DEFAULT ?
				M64_VERBOSE_STARTUP_BUILTIN_DEFAULT :
				M64_VERBOSE_STARTUP_USER_SPECIFIED),
			m64_aperture_kind_to_description[screen_state_p->aperture_kind],
			(m64_options_p->memory_aperture_type == 
				M64_OPTIONS_MEMORY_APERTURE_TYPE_USE_BUILTIN_DEFAULT ?
				M64_VERBOSE_STARTUP_BUILTIN_DEFAULT :
				M64_VERBOSE_STARTUP_USER_SPECIFIED),
			screen_state_p->video_memory_size / 1024,
			m64_mem_kind_to_description[screen_state_p->memory_kind]);

		(void) fprintf(stderr,
			M64_VERBOSE_STARTUP_SCREEN_PARAMETERS_MESSAGE,
			screen_state_p->generic_state.screen_physical_width,
			screen_state_p->generic_state.screen_physical_height,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height,
			screen_state_p->generic_state.screen_displayed_width,
			screen_state_p->generic_state.screen_displayed_height,
			screen_state_p->generic_state.screen_depth);

		(void) fprintf(stderr,
			M64_VERBOSE_STARTUP_GRAPHICS_ENGINE_PARAMETERS_MESSAGE,
			m64_graphics_engine_micro_delay_count,
			m64_graphics_engine_loop_timeout_count,
			((registers_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] & 
				GEN_TEST_CNTL_GEN_BLOCK_WR_EN) == 0 ? "disabled" : "enabled"),
			m64_options_p->enable_blocked_memory_write == 
				M64_OPTIONS_ENABLE_BLOCKED_MEMORY_WRITE_DEFAULT ?
				M64_VERBOSE_STARTUP_BUILTIN_DEFAULT :
				M64_VERBOSE_STARTUP_USER_SPECIFIED);

		(void) fprintf(stderr,
			LINEAR_FRAME_BUFFER_PARAMETERS_MESSAGE,
			(screen_state_p->linear_aperture_location >> 20),
			(screen_state_p->linear_aperture_size >> 20),
			m64_options_p->memory_aperture_size == 
				M64_OPTIONS_MEMORY_APERTURE_SIZE_USE_4MB_APERTURE ?
				M64_VERBOSE_STARTUP_BUILTIN_DEFAULT :
				M64_VERBOSE_STARTUP_USER_SPECIFIED);

		(void) fprintf(stderr, M64_VERBOSE_STARTUP_EPILOGUE);
	}

	config_p->IdentString = M64_DEFAULT_IDENT_STRING;
	return (initialization_status);
}

function void
m64_print_initialization_failure_message( const int status,
	const SIScreenRec *si_screen_p)
{
	struct m64_screen_state					*screen_state_p;
	struct m64_display_mode_table_entry 	*entry_p; 
	int										count;

	screen_state_p = (struct m64_screen_state *)si_screen_p->vendorPriv;

	if(status & M64_INITIALIZE_CHIPSET_DETECTION_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_CHIPSET_DETECTION_FAILED_MESSAGE);
	}
	if(status & M64_INITIALIZE_VIDEO_MEMORY_SIZE_DETECTION_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_VIDEO_MEMORY_SIZE_DETECTION_FAILED_MESSAGE);
	}
	if(status & M64_INITIALIZE_BUS_KIND_DETECTION_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_BUS_KIND_DETECTION_FAILED_MESSAGE);
	}
	if(status & M64_INITIALIZE_DAC_KIND_DETECTION_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_DAC_KIND_DETECTION_FAILED_MESSAGE);
	}
	if(status & M64_INITIALIZE_MMAP_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_MMAP_FAILED_MESSAGE);
	}
	if(status & M64_INITIALIZE_MMAP_DEVICE_OPEN_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_MMAP_DEVICE_OPEN_FAILED_MESSAGE);
	}
	if(status & M64_INITIALIZE_PARSE_USER_OPTIONS_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_PARSE_USER_OPTIONS_FAILED_MESSAGE);
	}
	if(status & M64_INITIALIZE_MEMORY_KIND_DETECTION_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_MEMORY_KIND_DETECTION_FAILED_MESSAGE);
	}
	if(status & M64_INITIALIZE_KD_MODE_SWITCH_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_KD_MODE_SWITCH_FAILED_MESSAGE);
	}
	if (status & M64_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED)
	{
		(void) fprintf (stderr,
			M64_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED_MESSAGE);
	}

	if (status & M64_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND)
	{
		(void) fprintf (stderr, 
			M64_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND_MESSAGE,
			si_screen_p->cfgPtr->disp_w, si_screen_p->cfgPtr->disp_h,
			si_screen_p->cfgPtr->depth, 
			(int)si_screen_p->cfgPtr->monitor_info.vfreq);

		/*
		 * List available modes.
		 */
		for(count = M64_MODE_NULL + 1,
			entry_p = &(m64_display_mode_table[M64_MODE_NULL + 1]);
			count < M64_MODE_COUNT; count ++, entry_p++)
		{
			/*
			 * Check if the clock/dac combination can support this mode.
			 */
	 		if(!screen_state_p->clock_chip_p->check_support_for_frequency(
				entry_p->clock_frequency) && (entry_p->clock_frequency <= 
				screen_state_p->dac_state_p->max_frequency))
			{
				(void) fprintf(stderr, "\t\t%s\n",entry_p->mode_description);
			}
		}
	}

	if(status & M64_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED)
	{
		(void)fprintf(stderr, 
			M64_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED_MESSAGE,
			screen_state_p->clock_frequency/1000,
			screen_state_p->clock_frequency%1000);
	}
	if(status & M64_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC)
	{
		(void) fprintf(stderr,
			M64_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC_MESSAGE,
			screen_state_p->clock_frequency/1000,
			screen_state_p->clock_frequency%1000,
			screen_state_p->dac_state_p->max_frequency/1000,
			screen_state_p->dac_state_p->max_frequency%1000,
			screen_state_p->dac_state_p->dac_name);
	}
	if (status & M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL)
	{
		(void) fprintf(stderr,
			M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL_MESSAGE);
	}

	if (status & M64_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE)
	{
		(void) fprintf(stderr,
			M64_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE_MESSAGE,
			screen_state_p->video_memory_size / 1024,
			si_screen_p->cfgPtr->disp_w, 
			si_screen_p->cfgPtr->disp_h,
			si_screen_p->cfgPtr->depth);
	}
}
