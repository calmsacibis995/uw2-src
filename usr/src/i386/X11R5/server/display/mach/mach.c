/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/mach.c	1.10"

/***
 ***	NAME
 ***
 ***		mach.c : flagship source for the mach display library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "mach.h"
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles the initialization sequence for the
 ***	MACH display library.
 ***
 ***	RETURNS
 *** 	
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

/*
 * System specific
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/kd.h>
#include <sys/errno.h>

/*
 * Inline functions
 */
#include <sys/inline.h>

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

/*
 * SI related
 */
#include "sidep.h"

/*
 * SDD related.
 */
#include "stdenv.h"
#include "generic.h"

#include "m_bitblt.h"
#include "m_colormap.h"
#include "m_cursor.h"
#include "m_fill.h"
#include "m_font.h"
#include "m_gs.h"
#include "m_line.h"
#include "m_opt.h"
#include "m_points.h"
#include "m_regs.h"
#include "m_scanline.h"
#include "m_spans.h"
#include "m_state.h"

/***
 ***	Constants.
 ***/

/*
 * Initialization flags.
 */
#define MACH_INITIALIZE_ATI_CHIPSET_DETECTION_FAILED	(1 << 0)
#define MACH_INITIALIZE_ATI_MACH8_DETECTION_FAILED		(1 << 1)
#define MACH_INITIALIZE_ATI_MACH32_DETECTION_FAILED		(1 << 2)
#define MACH_INITIALIZE_PARSE_USER_OPTIONS_FAILED		(1 << 3)
#define MACH_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND	(1 << 4)
#define MACH_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND		(1 << 5)
#define MACH_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED		(1 << 6)
#define MACH_INITIALIZE_BOARD_IS_INCAPABLE_OF_HI_RES_MODE (1 << 7)
#define MACH_INITIALIZE_MODE_IS_NOT_SUPPORTED_BY_DAC	(1 << 8)



/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

enum mach_display_mode 
{
#define DEFINE_MODE(MODENAME, MODE_DESCRIPTION, WIDTH, HEIGHT,\
				REFRESH_RATE, CLOCK, MONITOR_NAME, H_TOTAL, H_DISP,\
				H_SYNC_STRT, H_SYNC_WID, V_TOTAL, V_DISP,\
				V_SYNC_STRT, V_SYNC_WID, DISP_CNTL, CLOCK_SEL)\
		MODENAME
#include "m_modes.def"
#undef DEFINE_MODE
};

struct mach_display_mode_table_entry
{
	enum mach_display_mode display_mode;
	
	int display_width;
	int display_height;
	int display_refresh_rate;
	int clock_frequency;
	char *monitor_name_p;
	int h_total;
	int h_disp;
	int h_sync_strt;
	int h_sync_wid;
	int v_total;
	int v_disp;
	int v_sync_strt;
	int v_sync_wid;
	int disp_cntl;
	int clock_sel;

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
export boolean mach_debug = FALSE;
#endif


/*
 *	Current module state.
 */

#include "stdenv.h"

PRIVATE

/***
 ***	Private declarations.
 ***/

extern void mach__initialize__(SIScreenRec *si_screen_p,
							   struct mach_options_structure *options_p);
extern void mach__vt_switch_out__(void);
extern void mach__vt_switch_in__(void);

/***
 ***	Includes.
 ***/
#undef PRIVATE
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "m_asm.h"
#include "g_omm.h"

/***
 ***	Constants.
 ***/

/*
 * Sync Polarities that can be specified in modedb-string option
 */
#define MACH_MODEDB_POSITIVE_SYNC 0
#define MACH_MODEDB_NEGATIVE_SYNC 1

/*
 * Flags field in the modedb-string option
 */
#define MACH_DEFAULT_MODEDB_FLAGS_STRING_SIZE	256
#define MACH_DEFAULT_MODEDB_FORMAT_STRING_SIZE	50

/**
 **
 ** Programming the CRTC registers.
 ** 
 ** Tables of register values and associated clocks.
 ** 
 **
 **/

STATIC const struct mach_display_mode_table_entry
mach_display_mode_table[] = 
{
#define DEFINE_MODE(MODENAME, MODE_DESCRIPTION, WIDTH, HEIGHT,\
				REFRESH_RATE, CLOCK, MONITOR_NAME, H_TOTAL, H_DISP,\
				H_SYNC_STRT, H_SYNC_WID, V_TOTAL, V_DISP,\
				V_SYNC_STRT, V_SYNC_WID, DISP_CNTL, CLOCK_SEL)\
	{\
		 MODENAME,\
		 WIDTH, HEIGHT, REFRESH_RATE, CLOCK, MONITOR_NAME,\
		 H_TOTAL, H_DISP, H_SYNC_STRT, H_SYNC_WID,\
		 V_TOTAL, V_DISP, V_SYNC_STRT, V_SYNC_WID,\
		 DISP_CNTL, CLOCK_SEL\
	}
#include "m_modes.def"					
#undef DEFINE_MODE
	
};

STATIC const char *const mach_display_mode_descriptions[] =
{
	
#define DEFINE_MODE(MODENAME, MODE_DESCRIPTION, WIDTH, HEIGHT,\
				REFRESH_RATE, CLOCK, MONITOR_NAME, H_TOTAL, H_DISP,\
				H_SYNC_STRT, H_SYNC_WID, V_TOTAL, V_DISP,\
				V_SYNC_STRT, V_SYNC_WID, DISP_CNTL, CLOCK_SEL)\
		MODE_DESCRIPTION

#include "m_modes.def"

#undef DEFINE_MODE

};

		
/***
 ***	Macros.
 ***/

/***
 *** 	Functions.
 ***/


/*
 * Detecting an 8514 chip : see the Supplement to the Programmers
 * Guide to the Mach8 Extended Register Specification.
 */

STATIC boolean
mach_detect_8514(void)
{
	/*
	 * TODO: the RAMDAC based test.
	 */
	
	MACH_RESET_GRAPHICS_ENGINE();
	
	/* write to the ERR_TERM register */
	outw(MACH_REGISTER_ERR_TERM, 0x5555);

	MACH_WAIT_FOR_ENGINE_IDLE();

	/* read the value back */
	if (inw(MACH_REGISTER_ERR_TERM) != 0x5555)
	{
		(void) fprintf(stderr, MACH_BAD_READ_FROM_8514_REGISTER_MESSAGE);
		return (0);
	}
	
	/* repeat the process ... */
	outw(MACH_REGISTER_ERR_TERM, 0xAAAA);

	MACH_WAIT_FOR_ENGINE_IDLE();

	if (inw(MACH_REGISTER_ERR_TERM) != 0xAAAA)
	{
		(void) fprintf(stderr, MACH_BAD_READ_FROM_8514_REGISTER_MESSAGE);
		return (0);
	}

	/*
	 * All's well : 8514 detected.
	 */
	return (1);
}

/*
 * Detecting an ATI chip : returns bios revision number.
 */

STATIC boolean
mach_detect_ati(unsigned char *bios_major_revision_p, 
				unsigned char *bios_minor_revision_p,
				unsigned char *asic_revision_p)
{
	unsigned int rom_addr_1;
	int mem_fd;
	char *mem_p;
	
	if (!mach_detect_8514())
	{
		/*
		 * !8514 -- can't be a ATI MACH(8,32) chip.
		 */
#if (defined(__DEBUG__))
		
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, "(mach)\tdetection of 8514 "
					"failed.\n");
		}
#endif /* __DEBUG__ */

		return FALSE;
	}

	/*
	 * Attempt to read the ROM.
	 */
	mem_fd = open("/dev/pmem", O_RDONLY);
	if (mem_fd < 0)
	{
		perror(MACH_CANNOT_OPEN_DEV_PMEM_MESSAGE);
		return FALSE;
	}
	
	mem_p = mmap(0, DEFAULT_MACH_DEV_PMEM_MMAP_SIZE, PROT_READ,
				 MAP_SHARED, mem_fd, 0);
	if (!mem_p)
	{
		perror(MACH_CANNOT_MMAP_DEV_PMEM_MESSAGE);
		(void) close(mem_fd);
		return FALSE;
	}
	
	if (strncmp(mem_p + DEFAULT_MACH_BIOS_SIGNATURE_OFFSET,
				 DEFAULT_MACH_BIOS_SIGNATURE,
				 sizeof(DEFAULT_MACH_BIOS_SIGNATURE) - 1))
	{
		(void) fprintf(stderr, MACH_BIOS_SIGNATURE_MISMATCH_MESSAGE);
		(void) munmap(mem_p, DEFAULT_MACH_DEV_PMEM_MMAP_SIZE);
		(void) close(mem_fd);
		return FALSE;
	}

#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p, 
"(mach)\n"
"{\n"
"\tBios signature test passes.\n"
"\tsignature = %-*s\n"
"}\n",
					   sizeof(DEFAULT_MACH_BIOS_SIGNATURE) - 1,
					   mem_p + DEFAULT_MACH_BIOS_SIGNATURE_OFFSET);
	}
#endif

	*bios_major_revision_p =  
		*((unsigned char *) 
		  ((void *) (mem_p + DEFAULT_MACH_BIOS_MAJOR_REVISION_OFFSET)));
	
	*bios_minor_revision_p =  
		*((unsigned char *)
		  ((void *) (mem_p + DEFAULT_MACH_BIOS_MINOR_REVISION_OFFSET)));
	
	*asic_revision_p =  
		*((unsigned char *)
		  ((void *) (mem_p + DEFAULT_MACH_ASIC_REVISION_OFFSET)));
	
	(void) munmap(mem_p, DEFAULT_MACH_DEV_PMEM_MMAP_SIZE);
	(void) close(mem_fd);
	
	
	/* read from rom addr 1 */
	rom_addr_1 = inw(MACH_REGISTER_ROM_ADDR_1);
	/* write pattern to the same register */
	outw(MACH_REGISTER_ROM_ADDR_1, 0x5555);

	MACH_WAIT_FOR_ENGINE_IDLE();

	if (inw(MACH_REGISTER_ROM_ADDR_1) != 0x5555)
	{
		(void) fprintf(stderr, MACH_BAD_READ_FROM_MACH8_REGISTER_MESSAGE);
		(void) munmap(mem_p, DEFAULT_MACH_DEV_PMEM_MMAP_SIZE);
		(void) close(mem_fd);
		return FALSE;
	}

	outw(MACH_REGISTER_ROM_ADDR_1, 0x2A2A);

	MACH_WAIT_FOR_ENGINE_IDLE();

	if (inw(MACH_REGISTER_ROM_ADDR_1) != 0x2A2A)
	{
		(void) fprintf(stderr, MACH_BAD_READ_FROM_MACH8_REGISTER_MESSAGE);
		(void) munmap(mem_p, DEFAULT_MACH_DEV_PMEM_MMAP_SIZE);
		(void) close(mem_fd);
		return FALSE;
	}
	
	/* restore the saved value of the register */
	outw(MACH_REGISTER_ROM_ADDR_1, rom_addr_1);

	MACH_WAIT_FOR_ENGINE_IDLE();

	/*
	 * All's well, MACH 8 detected.
	 */
	return TRUE;
	
}

STATIC boolean
mach_detect_mach8(void)
{

	/*
	 * Look for an ATI-38800-1 chip.
	 */
	outw(MACH_REGISTER_DEST_X, 0xAAAA);
	
	MACH_WAIT_FOR_ENGINE_IDLE();
	
	if (inw(MACH_REGISTER_R_SRC_X) != 0xAAAA)
	{
		/*
		 * Is a mach8 chip.
		 */
		return(1);
	}
	
	/*
	 * repeat the test with a different pattern.
	 */
	outw(MACH_REGISTER_DEST_X, 0x5555);
	
	MACH_WAIT_FOR_ENGINE_IDLE();
	
	if (inw(MACH_REGISTER_R_SRC_X) != 0x5555)
	{
		/*
		 * Is a mach8 chip.
		 */
		return(1);
	}
	
	return (0);
	
}

STATIC boolean
mach_detect_mach32(void)
{
	/*
	 * Look for the existence of the EXT FIFO STATUS register.
	 */
	MACH_WAIT_FOR_ENGINE_IDLE();
	
	if ((inw(MACH_REGISTER_EXT_FIFO_STATUS) == 0xFFFF))
	{
		perror(MACH_CANNOT_ACCESS_EXTENDED_FIFO_STATUS_REGISTER_MESSAGE);
		return (0);
	}
	
	MACH_RESET_GRAPHICS_ENGINE();
	
	/*
	 * Write test pattern to the SRC_X register
	 */
	outw(MACH_REGISTER_SRC_X, 0x555);
	
	MACH_WAIT_FOR_ENGINE_IDLE();
	
	/*
	 * Read it back from R_SRC_X
	 */
	if ((inw(MACH_REGISTER_R_SRC_X) != 0x555))
	{
#if (defined(__DEBUG__))
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, 
						   "(mach32) bad read from R_SRC_X register.\n");
		}
#endif
		return (0);
	}
	/*
	 * Repeat ...
	 */
	outw(MACH_REGISTER_SRC_X, 0x2AA);
	
	MACH_WAIT_FOR_ENGINE_IDLE();
	
	if ((inw(MACH_REGISTER_R_SRC_X) != 0x2AA))
	{
#if (defined(__DEBUG__))
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, 
						   "(mach32) bad read from R_SRC_X register.\n");
		}
#endif
		return (0);
	}
	
	return (1);

}

#if (defined(USE_KD_GRAPHICS_MODE))
/*
 * mach_set_virtual_terminal_mode
 *
 * Set the virtual terminal mode to KD_GRAPHICS or KD_TEXT.
 */
STATIC int
mach_set_virtual_terminal_mode(SIint32
							   virtual_terminal_file_descriptor, 
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



/*
 * Function:
 *			mach_parse_modedb_string
 * Returns:		
 *			1 on success
 *			0 on failure
 */

STATIC boolean
mach_parse_modedb_string(char *modedb_p, 
						struct mach_screen_state *screen_state_p)
{
	unsigned short clock_sel;
	float clock_frequency;
	float tmp_clock_frequency;
	int clock_divide;
	int clock_found;
	int clock_count;
	int HTotal, HDisplay, HSyncStart, HSyncEnd, 
		VTotal, VDisplay, VSyncStart, VSyncEnd;
	int is_interlaced = 0, 
		hsync_polarity = MACH_MODEDB_POSITIVE_SYNC, 
		vsync_polarity = MACH_MODEDB_POSITIVE_SYNC;
	int v_sync_wid;
	int memory_required;
	int ret;
	char format_string[MACH_DEFAULT_MODEDB_FORMAT_STRING_SIZE];
	char *flags_string_p;
	char *flag_p;
	char *tmp_flag_p;
	char *saved_modedb_p;


	ASSERT(modedb_p != NULL);
	ASSERT(screen_state_p != NULL);

	flags_string_p = (char *)allocate_and_clear_memory(sizeof(char) * 
							MACH_DEFAULT_MODEDB_FLAGS_STRING_SIZE);

	sprintf(format_string, "%s%%%ds", "%f %d %d %d %d %d %d %d %d ", 
							MACH_DEFAULT_MODEDB_FLAGS_STRING_SIZE);
	ret = sscanf(modedb_p,
		format_string,
		&clock_frequency,
	 	&HDisplay,&HSyncStart,&HSyncEnd, &HTotal,
	 	&VDisplay,&VSyncStart,&VSyncEnd, &VTotal, flags_string_p);

	if (ret	!= 10)
	{
			if(ret != 9)
			{
				(void) fprintf(stderr,
				MACH_INITIALIZE_IGNORING_MALFORMED_MODEDB_STRING_OPTION_MESSAGE,
				modedb_p);
				return(0);
			}
	}
	else
	{
		/*
		 * Copy the modedb string as it is going to be modified in place.
		 */
		saved_modedb_p = strdup(modedb_p);

		flags_string_p = strstr(modedb_p, flags_string_p);

		flag_p = strtok(flags_string_p, " \t");

		while(flag_p != NULL)
		{
			/*
			 * Convert to uppercase.
			 */
			tmp_flag_p = flag_p;
			while(*tmp_flag_p)
			{
				*tmp_flag_p = toupper(*tmp_flag_p);
				tmp_flag_p++;
			}

			ASSERT(*tmp_flag_p == NULL);

			if (strcmp(flag_p, "INTERLACE")==0 )
			{
				is_interlaced = 1;
			}
			else if (strcmp(flag_p, "-HSYNC")==0)
			{
				hsync_polarity = MACH_MODEDB_NEGATIVE_SYNC;
			}
			else if (strcmp(flag_p, "-VSYNC")==0)
			{
				vsync_polarity = MACH_MODEDB_NEGATIVE_SYNC;
			}
			else if((strcmp(flag_p, "+HSYNC")==0) ||
					(strcmp(flag_p, "+VSYNC")==0))
			{
				/*EMPTY*/
				;
			}
			else
			{
				(void) fprintf(stderr,
				MACH_INITIALIZE_IGNORING_MALFORMED_MODEDB_STRING_OPTION_MESSAGE,
				saved_modedb_p);
				return(0);
			}

			flag_p = strtok(NULL, " \t");

		}
	}

#if (defined(__DEBUG__))
    if (mach_debug)
    {
        (void) fprintf(debug_stream_p,
            "(mach_parse_modedb_string) \n"
			"{\n"
			"\tclock_frequency = %11.5f MHz \n"
			"\tHDisplay = %d HSyncStart = %d HSyncEnd = %d HTotal = %d\n"
			"\tVDisplay = %d VSyncStart = %d VSyncEnd = %d VTotal = %d\n"
			"\t%s\tHSYNC: %s\tVSYNC: %s\n"
			"}\n",
			clock_frequency,
			HDisplay, HSyncStart, HSyncEnd, HTotal,
			VDisplay, VSyncStart, VSyncEnd, VTotal,
			(is_interlaced)?"I":"NI", 
			(hsync_polarity)?"NEGATIVE":"POSITIVE",
			(vsync_polarity)?"NEGATIVE":"POSITIVE");
	}
#endif
	
	if( !((HDisplay == screen_state_p->generic_state.screen_displayed_width) 
		&& (VDisplay == screen_state_p->generic_state.screen_displayed_height)))
	{
			(void) fprintf(stderr, 
			MACH_INITIALIZE_MODEDB_RESOLUTION_MISMATCH_MESSAGE,
			screen_state_p->generic_state.screen_displayed_width,
			screen_state_p->generic_state.screen_displayed_height,
			HDisplay, VDisplay);
			(void) fprintf(stderr,
				MACH_INITIALIZE_OVERRIDING_DISPLAY_SIZE_IN_CONFIG_FILE_MESSAGE);

			screen_state_p->generic_state.screen_displayed_width  = HDisplay ;
			screen_state_p->generic_state.screen_displayed_height = VDisplay ;
	}

	ASSERT(HDisplay == screen_state_p->generic_state.screen_displayed_width);
	ASSERT(VDisplay == screen_state_p->generic_state.screen_displayed_height);

	/*
	 * Check if the 'configured' memory size can support the mode
	 * requested.
	 */
	memory_required  = HDisplay * VDisplay *
						screen_state_p->generic_state.screen_depth;

	if( (screen_state_p->video_memory_size << 3) < memory_required )
	{
			(void) fprintf(stderr, 
			MACH_INITIALIZE_MODEDB_INSUFFICIENT_MEMORY_SIZE_MESSAGE,
			screen_state_p->video_memory_size >> 10, HDisplay, VDisplay,
						screen_state_p->generic_state.screen_depth);

			(void) fprintf(stderr,
						MACH_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE);
			return(0);
	}

	/*
	 * Refreshrate = 
	 * 		clock frequency/(horizontal framelength * vertical framelength)
	 * Horizontal scan frequency =
	 * 		clock frequency/horizontal framelength	 
	 */		

#if (defined(__DEBUG__))
    if (mach_debug)
    {
        (void) fprintf(debug_stream_p,
            "(mach_parse_modedb_string) \n"
			"{\n"
			"\tresolution = %dx%dx%d\n"
			"\trefresh rate = %11.5f Hz\n"
			"\thorizontal scan frequency = %11.5f KHz\n"
			"}\n",
 			HDisplay, VDisplay, screen_state_p->generic_state.screen_depth,
	  		((clock_frequency*1000000)/((float)HTotal * VTotal)),
			((clock_frequency*1000)/(float)HTotal));
	}
#endif

	/*
	 * Initialize CLOCK_SEL register value.
	 * Always PASS_THROUGH is enabled.
	 * Later on, OR it with the other fields namely CLK_SEL (clock index)
	 * and CLK_DIV (to divide the clock programmed to get the required 
	 * clock).
	 *
	 */

	clock_sel = 0;
	clock_divide = 1;
	clock_found = 0;

	while( clock_divide <= 2 && (clock_found == 0))
	{
		
		tmp_clock_frequency = clock_frequency * 100 * clock_divide ;

		for (clock_count = 0;
			 clock_count <
			 DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES &&
			 screen_state_p->clock_chip_p->clock_frequency_table[clock_count] !=
			 (int) tmp_clock_frequency;
			 clock_count ++)
		{
			;
		}

		if (clock_count !=
			DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES)
		{
			clock_found = 1 ;
			clock_sel |= (clock_count << MACH_CLOCK_SEL_CLK_SEL_SHIFT) ; 

			if( clock_divide == 2)
			{
				clock_sel |= MACH_CLOCK_SEL_CLK_DIV_2;
			}
			/*
			 * The clock frequency as seen by the DAC.
			 */
			screen_state_p->clock_frequency = (int) (tmp_clock_frequency/
													clock_divide) ;

		}

		clock_divide++ ;
	}

	if (!clock_found)
	{

		(void) fprintf(stderr, 
		MACH_INITIALIZE_MODEDB_CANNOT_FIND_REQUESTED_CLOCK_FREQUENCY_MESSAGE,
		screen_state_p->clock_chip_p->clock_chip_name_p, clock_frequency);

		(void) fprintf(stderr,
		MACH_INITIALIZE_MODEDB_SUPPORTED_CLOCK_FREQUENCIES_MESSAGE);

		for (clock_count = 0;
		 	clock_count <
		 	DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES;
		 	clock_count ++)
		{

			if (screen_state_p->clock_chip_p->clock_frequency_table[clock_count]
				>= 0)
			{

				(void) fprintf(stderr, "%d.%d  ", 
				screen_state_p->clock_chip_p->clock_frequency_table[clock_count]
				/100 ,
				screen_state_p->clock_chip_p->clock_frequency_table[clock_count]
				%100 );
			}
			else
			{
				(void) fprintf(stderr, "%d  ",
				screen_state_p->clock_chip_p->clock_frequency_table[clock_count]
				);
			}
		}

		(void) fprintf(stderr, "\n");
		(void) fprintf(stderr,
					MACH_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE);

#if (defined(__DEBUG__))
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(mach_parse_modedb_string) missed clock "
						   "freq %11.5fMHz\n",
						   clock_frequency);
		}
#endif

		return(0);
	}

	ASSERT(clock_found);

	/*
	 * All the validity/feasibility checks on the input 
	 * modedb parameters are done already. Now, convert them 
	 * to register values. 
	 */
	/*
	 * NOTE : 
	 *	All the CRT timing parameters( register values) are inclusive.
	 *
	 * Horizontal register values are in the units of eight pixels.
	 */

	screen_state_p->register_state.h_total = (((HTotal >> 3) - 1) & (0xFF));

	screen_state_p->register_state.h_disp = (((HDisplay >> 3) - 1) & (0xFF));

	screen_state_p->register_state.h_sync_strt = (((HSyncStart >> 3) - 1) & 
													(0xFF));

	screen_state_p->register_state.h_sync_wid = (((HSyncEnd - HSyncStart) >> 3) 
												& (MACH_H_WIDTH));
	screen_state_p->register_state.h_sync_wid |= (hsync_polarity << 
													MACH_H_POLARITY_SHIFT);

	/*
	 * Vertical parameters are specified with respect to the Y counter.
	 * We always set Y counter to SKIP BIT 2 (in the register DISP_CNTL). 
	 * Use the formula given in 'Programmer's guide to the mach32 regs'
	 * to calculate the value to program (approx. multiplied by 2).
	 * " (((linear << 1) & 0xFFF8) | (linear & 0x3) | ((linear & 0x80) >> 5)) "
	 *
	 * (i.e.,) When the Y counter value reaches the value programmed 
	 * into the register, the apropriate action takes place. Skipping
	 * a bit (any bit) means that the counter counts twice as fast as
	 * when it is linear (approx).
	 */

	screen_state_p->register_state.v_total = 
		((((VTotal << 1) & 0xFFF8) | (VTotal & 0x3) | ((VTotal & 0x80) >> 5))
		- 1) & 0xFFF;

	screen_state_p->register_state.v_disp = 
		((((VDisplay << 1) & 0xFFF8) | (VDisplay & 0x3) | ((VDisplay & 0x80) 
		>> 5)) - 1) & 0xFFF;

	screen_state_p->register_state.v_sync_strt = 
		((((VSyncStart << 1) & 0xFFF8) | (VSyncStart & 0x3) | ((VSyncStart & 
		0x80) >> 5)) - 1) & 0xFFF;

	v_sync_wid = VSyncEnd - VSyncStart;
	screen_state_p->register_state.v_sync_wid = 
		(((v_sync_wid << 1) & 0xFFF8) | (v_sync_wid & 0x3) | ((v_sync_wid & 
		0x80) >> 5)) & MACH_V_WIDTH;
	screen_state_p->register_state.v_sync_wid |= (vsync_polarity << 
													MACH_V_POLARITY_SHIFT);

	/*
	 * Always,
	 * Y_CONTROL is set to SKIP_BIT_2,
	 * Single Scan and
	 * 8514 CRT controller enabled.
	 *
	 */
	screen_state_p->register_state.disp_cntl = MACH_Y_CONTROL_SKIP_2 | 
												MACH_ENABLE_DISPLAY_ENABLE |
												(is_interlaced << 
												MACH_INTERLACE_SHIFT) |
												0x0001 ;
												/* Reserved bit 0 */

	screen_state_p->register_state.clock_sel = clock_sel ;	

	return(1);

}


/*
 * mach_get_mode
 *
 * set crtc registers according to a given mode.  Returns an error
 * code if a suitable mode was not found.
 */
STATIC int
mach_get_mode(SIConfigP config_p,
			  struct mach_options_structure *options_p,
			  struct mach_screen_state *screen_state_p)
{
	
	int initialization_status = 0;
	
	unsigned short h_total, h_disp, h_sync_strt, h_sync_wid;
	unsigned short v_total, v_disp, v_sync_strt, v_sync_wid;
	unsigned short disp_cntl, clock_sel;
	
	const struct mach_display_mode_table_entry *entry_p = 
		&(mach_display_mode_table[MACH_DISPLAY_MODE_NULL + 1]);

	const struct mach_clock_chip_table_entry *clock_chip_table_p;
	
	int mode_count = 0;
	int clock_count = 0;

	/*
	 * Select the clock chip if the board level code has not already
	 * supplied it.
	 */
	if (screen_state_p->clock_chip_p == NULL)
	{

		/*
		 * We cannot automagically determine the clock chip name.
		 * This information would be kept in the configuration file.
		 */
		switch (options_p->clock_chip_name)
		{
		  case MACH_OPTIONS_CLOCK_CHIP_NAME_18810 :
			  clock_chip_table_p =
				  &(mach_clock_chip_table[MACH_CLOCK_CHIP_18810]);
			  break;

		  case MACH_OPTIONS_CLOCK_CHIP_NAME_18810_2 :
			  clock_chip_table_p =
				  &(mach_clock_chip_table[MACH_CLOCK_CHIP_18810_2]);
			  break;

		  case MACH_OPTIONS_CLOCK_CHIP_NAME_18811_0 :
			  clock_chip_table_p =
				  &(mach_clock_chip_table[MACH_CLOCK_CHIP_18811_0]);
			  break;

		  case MACH_OPTIONS_CLOCK_CHIP_NAME_18812_0 :
			  clock_chip_table_p =
				  &(mach_clock_chip_table[MACH_CLOCK_CHIP_18812_0]);
			  break;

		  case MACH_OPTIONS_CLOCK_CHIP_NAME_18811_1 :
			  clock_chip_table_p =
				  &(mach_clock_chip_table[MACH_CLOCK_CHIP_18811_1]);
			  break;

		  default:
			  initialization_status |=
				  MACH_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED;
			  return (initialization_status);
		}

		screen_state_p->clock_chip_p = clock_chip_table_p;

	}
	
#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p, 
					   "(mach_get_mode)\tclock chip \"%s\".\n",
					   mach_clock_chip_kind_dump
					   [clock_chip_table_p->clock_chip_kind]);
		 

	}
#endif

	if (options_p->modedb_string == NULL || 
		mach_parse_modedb_string(options_p->modedb_string, screen_state_p) == 0)
	{

		if (options_p->crtc_parameters && 
			(sscanf(options_p->crtc_parameters, 
				   "%hi %hi %hi %hi %hi %hi %hi %hi %hi %hi",
				   &h_total, &h_disp, &h_sync_strt, &h_sync_wid,
				   &v_total, &v_disp, &v_sync_strt, &v_sync_wid,
				   &disp_cntl, &clock_sel)) == 10)
		{
			
	#if (defined(__DEBUG__))
			if (mach_debug)
			{
				(void) fprintf(debug_stream_p,
							   "(mach_get_mode) user option display mode {}\n");
			}
	#endif

			/*
			 * Load registers.
			 */

			screen_state_p->register_state.h_total = h_total;
			screen_state_p->register_state.h_disp = h_disp;
			screen_state_p->register_state.h_sync_strt = h_sync_strt;
			screen_state_p->register_state.h_sync_wid = h_sync_wid;

			screen_state_p->register_state.v_total = v_total;
			screen_state_p->register_state.v_disp = v_disp;
			screen_state_p->register_state.v_sync_strt = v_sync_strt;
			screen_state_p->register_state.v_sync_wid = v_sync_wid;

			screen_state_p->register_state.disp_cntl = disp_cntl;

			screen_state_p->register_state.clock_sel = clock_sel;

		}
		else
		{
			if (options_p->crtc_parameters)
			{
				(void) fprintf(stderr,
						 MACH_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION_MESSAGE, 
							   options_p->crtc_parameters);
			}
				
			/*
			 * Look for a mode entry which matches the information specifed.
			 */
			for(mode_count = MACH_DISPLAY_MODE_NULL + 1,
				entry_p =
				&(mach_display_mode_table[MACH_DISPLAY_MODE_NULL + 1]);
				mode_count < MACH_DISPLAY_MODE_COUNT; 
				mode_count ++, entry_p++)
			{
				double refresh_difference = /* make a positive value */
					(config_p->monitor_info.vfreq >
					 entry_p->display_refresh_rate) ? 
						 (config_p->monitor_info.vfreq -
						  entry_p->display_refresh_rate) :
						 (entry_p->display_refresh_rate - 
						  config_p->monitor_info.vfreq);
				
				/*
				 * look for a matching entry in the modetable :
				 */
				if ((config_p->disp_w == entry_p->display_width) &&
					(config_p->disp_h == entry_p->display_height) &&
					(refresh_difference < DEFAULT_MACH_EPSILON) &&
					((entry_p->monitor_name_p) ? /* NULL will match any name */
					 !strcmp(entry_p->monitor_name_p,
							 config_p->monitor_info.model) : TRUE))
				{
					/*
					 * Look for the appropriate clock index.
					 */
					initialization_status &=
							~MACH_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND;

					for (clock_count = 0;
						 clock_count <
						 DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES &&
						 clock_chip_table_p->clock_frequency_table[clock_count] !=
						 entry_p->clock_frequency;
						 clock_count ++)
					{
						;
					}

					if (clock_count ==
						DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES)
					{
						/*
						 * Try another mode.
						 */
						initialization_status |=
							MACH_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND;

	#if (defined(__DEBUG__))
						if (mach_debug)
						{
							(void) fprintf(debug_stream_p,
										   "(mach_get_mode) missed clock "
										   "freq %d.%d\n",
										   entry_p->clock_frequency/100,
										   entry_p->clock_frequency%100);
						}
	#endif
						continue;
					}

					break;
				}

			}

			if (mode_count == MACH_DISPLAY_MODE_COUNT)
			{
				initialization_status |=
					MACH_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND; 
				return (initialization_status);
			}

			/*
			 * The clock frequency seen by the DAC depends on the divide
			 * ratio set in the CLOCKSEL register.
			 */
			screen_state_p->clock_frequency =
				(entry_p->clock_sel & MACH_CLOCK_SEL_CLK_DIV_2) ?
				(entry_p->clock_frequency >> 1) :
				(entry_p->clock_frequency); 
			
	#if (defined(__DEBUG__))
			if (mach_debug)
			{
				(void) fprintf(debug_stream_p, 
	"(mach_get_mode) table selected display mode =\n"
	"{\n"
	"\tdisplay_width = %d\n"
	"\tdisplay_height = %d\n"
	"\tmonitor_name = \"%s\"\n"
	"\tclock index = %d\n"
	"\tchip clock frequency = %d.%d Mhz\n"
	"\toutput clock frequency = %d.%d Mhz\n"
	"}\n",
							   entry_p->display_width,
							   entry_p->display_height,
							   entry_p->monitor_name_p,
							   clock_count,
							   (entry_p->clock_frequency / 100), 
							   (entry_p->clock_frequency % 100),
							   (screen_state_p->clock_frequency / 100),
							   (screen_state_p->clock_frequency % 100));
			}
	#endif	

			/*
			 * Load registers.
			 */

			screen_state_p->register_state.h_total = entry_p->h_total;
			screen_state_p->register_state.h_disp = entry_p->h_disp;
			screen_state_p->register_state.h_sync_strt = entry_p->h_sync_strt;
			screen_state_p->register_state.h_sync_wid = entry_p->h_sync_wid;
			
			screen_state_p->register_state.v_total = entry_p->v_total;
			screen_state_p->register_state.v_disp = entry_p->v_disp;
			screen_state_p->register_state.v_sync_strt = entry_p->v_sync_strt;
			screen_state_p->register_state.v_sync_wid = entry_p->v_sync_wid;
			
			screen_state_p->register_state.disp_cntl = entry_p->disp_cntl;
			
			screen_state_p->register_state.clock_sel = 
				(entry_p->clock_sel & ~MACH_CLOCK_SEL_CLK_SEL) |
					((clock_count << MACH_CLOCK_SEL_CLK_SEL_SHIFT) &
					 MACH_CLOCK_SEL_CLK_SEL);
			
		}
	}

#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_get_mode) crtc parameters\n"
"{\n"
"\th_total = %hx\n"
"\th_disp = %hx\n"
"\th_sync_strt = %hx\n"
"\th_sync_wid = %hx\n"
"\tv_total = %hx\n"
"\tv_disp = %hx\n"
"\tv_sync_strt = %hx\n"
"\tv_sync_wid = %hx\n"
"\tdisp_cntl = %hx\n"
"\tclock_sel = %hx\n"
"}\n",
					   screen_state_p->register_state.h_total, 
					   screen_state_p->register_state.h_disp, 
					   screen_state_p->register_state.h_sync_strt, 
					   screen_state_p->register_state.h_sync_wid,
					   screen_state_p->register_state.v_total, 
					   screen_state_p->register_state.v_disp, 
					   screen_state_p->register_state.v_sync_strt, 
					   screen_state_p->register_state.v_sync_wid,
					   screen_state_p->register_state.disp_cntl, 
					   screen_state_p->register_state.clock_sel);
	}
#endif

	/*
	 * check if the user has requested a particular VRAM fifo
	 * trigger depth.
	 */
	if (options_p->vram_fifo_depth != -1)
	{
		screen_state_p->register_state.clock_sel =
			(screen_state_p->register_state.clock_sel &
			 (~MACH_CLOCK_SEL_VFIFO_DEPTH)) |
				 ((options_p->vram_fifo_depth <<
				  MACH_CLOCK_SEL_VFIFO_DEPTH_SHIFT) &
				  (MACH_CLOCK_SEL_VFIFO_DEPTH));
	}

	/*
	 * Allow the user to override the monitor sync type.
	 */
	if (options_p->monitor_sync_type ==
		MACH_OPTIONS_MONITOR_SYNC_TYPE_COMPOSITE_SYNC)
	{
		screen_state_p->register_state.clock_sel |=
			MACH_CLOCK_SEL_COMPOSITE_SYNC;
	}
	else
	{
		screen_state_p->register_state.clock_sel &=
			(~MACH_CLOCK_SEL_COMPOSITE_SYNC);
	}

#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(mach_get_mode) clock_sel = 0x%hx\n",
					   screen_state_p->register_state.clock_sel);
		
	}
#endif
	
	screen_state_p->register_state.crt_offset_lo = 0;
	screen_state_p->register_state.crt_offset_hi = 0;

	if (options_p->crtc_start_offset)
	{
		screen_state_p->register_state.crt_offset_lo =
			(options_p->crtc_start_offset / config_p->depth) &
				0x0000FFFF;
		screen_state_p->register_state.crt_offset_hi =
			((options_p->crtc_start_offset / config_p->depth)
			 & 0x00FF0000) >> 16;
	}

	/*
	 * The max waitstates register :
	 */
	
	screen_state_p->register_state.max_waitstates =
		inw(MACH_REGISTER_MAX_WAITSTATES);
	
	if (options_p->horizontal_line_draw_optimizations == 
		MACH_OPTIONS_HORIZONTAL_LINE_DRAW_OPTIMIZATIONS_ENABLED)
	{
		screen_state_p->register_state.max_waitstates &= 
			~MACH_USE_HORIZONTAL_LINE_DRAW_OPTIMIZATIONS;
	}
	
#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(mach_get_mode) max_waitstates = 0x%hx\n",
					   screen_state_p->register_state.max_waitstates);
	}
#endif
	
	/*
	 * Check if the DAC can support the selected display mode.
	 */
	if (mach_dac_check_display_mode_feasibility(screen_state_p) ==
		FALSE)
	{
		initialization_status |=
			MACH_INITIALIZE_MODE_IS_NOT_SUPPORTED_BY_DAC;
	}
	
	return (initialization_status);

}

/*
 * Program the MACH 8 CRTC registers according to the given mode.
 */
STATIC int
mach8_set_registers(SIConfigP config_p, 
					struct mach_options_structure *options_p,
					struct mach_screen_state *screen_state_p)
{
	int initialization_status;
	unsigned int monitor_alias;

	/*
	 * save the clock sel that is currently present.
	 */
	screen_state_p->register_state.saved_clock_sel =
		inw(MACH_REGISTER_CLOCK_SEL);

	if ((initialization_status = mach_get_mode(config_p, options_p,
											   screen_state_p)))
	{
		return (initialization_status);
	}

	/*
	 * Save the monitor alias.
	 */
	monitor_alias = ((screen_state_p->register_state.subsys_stat &
					  MACH_MONITOR_ID) >> MACH_MONITOR_ID_SHIFT);

	if (config_p->depth == 4)
	{
		/*
		 * The CRT pitch is fixed at 1024 for the MACH8 except for the
		 * 1280x1024x4 mode.  
		 */
		if (config_p->disp_w == 1280 && config_p->disp_h == 1024)
		{
			
			/*
			 * Check the hi-res boot bit of the config status 2 register.
			 */
	
			if ((screen_state_p->register_state.config_status_2 &
				 MACH8_HI_RES_BOOT) &&
				(screen_state_p->clock_chip_p->clock_chip_kind >=
				 MACH_CLOCK_CHIP_18810_2))
			{
				screen_state_p->register_state.crt_pitch = (1280/8);
				
				screen_state_p->register_state.ge_pitch = (1280/8);
			
				screen_state_p->register_state.ext_ge_config |=
					MACH_EXT_GE_CONFIG_Z1280 |
					MACH_EXT_GE_CONFIG_ALIAS_ENA | 
				    (monitor_alias & MACH_EXT_GE_CONFIG_MONITOR_ALIAS); 
				
			}
			else
			{
#if (defined(__DEBUG__))
				if (mach_debug)
				{
					(void) fprintf(debug_stream_p,
								   "(mach) 1280x1024 mode is not "
								   "supported on this board.\n");
				}
#endif
				screen_state_p->register_state.ext_ge_config = 
					MACH_EXT_GE_CONFIG_ALIAS_ENA | (monitor_alias &
									  MACH_EXT_GE_CONFIG_MONITOR_ALIAS);
			
				return (initialization_status |
						MACH_INITIALIZE_BOARD_IS_INCAPABLE_OF_HI_RES_MODE);
			}
		}
		else /* lower res mode */
		{
			screen_state_p->register_state.ext_ge_config |=
				MACH_EXT_GE_CONFIG_Z1280 |
				MACH_EXT_GE_CONFIG_ALIAS_ENA | 
				(monitor_alias & MACH_EXT_GE_CONFIG_MONITOR_ALIAS);
			screen_state_p->register_state.ge_pitch = (1024/8);
			screen_state_p->register_state.crt_pitch = (1024/8);
			
		}
		/*
		 * The read and write masks.
		 */
		screen_state_p->register_state.wrt_mask = 0x0FU;
		screen_state_p->register_state.rd_mask = 0x0FU;

	}
	else
	{
		/*
		 * 8 bit per pixel modes.
		 */
		screen_state_p->register_state.crt_pitch = (1024 / 8);
		screen_state_p->register_state.ge_pitch = (1024 / 8);
		screen_state_p->register_state.ext_ge_config =
			MACH_EXT_GE_CONFIG_ALIAS_ENA | 
			(monitor_alias & MACH_EXT_GE_CONFIG_MONITOR_ALIAS);
		/*
		 * The read and write masks.
		 */
		screen_state_p->register_state.wrt_mask = 0xFFU;
		screen_state_p->register_state.rd_mask =  0xFFU;
	}
	
	screen_state_p->register_state.ge_offset_lo = 0;
	screen_state_p->register_state.ge_offset_hi = 0;
	
	/*
	 * Reset the scissors.
	 */
	screen_state_p->register_state.ext_scissor_t = 
	screen_state_p->register_state.ext_scissor_l = 0;

	screen_state_p->register_state.ext_scissor_r = config_p->virt_w-1;
	screen_state_p->register_state.ext_scissor_b = config_p->virt_h-1;

	return (initialization_status);
	
}

/*
 * Program the MACH32 CRTC registers according to the given mode.
 */

STATIC int
mach32_set_registers(SIConfigP config_p, 
						  struct mach_options_structure *options_p,
						  struct mach_screen_state *screen_state_p)
{
	int initialization_status = 0;
	int max_pitch;
	
	
	/*
	 * read in whatever is readable.
	 */
	screen_state_p->register_state.misc_cntl =
		(inw(MACH_REGISTER_R_MISC_CNTL) & /* certain bits of misc cntl */
		 (MACH_R_MISC_CNTL_PIXEL_DELAY | /* need to be masked */
		  MACH_R_MISC_CNTL_BLANK_ADJUST |
		  MACH_R_MISC_CNTL_ROM_PAGE_SEL));
	screen_state_p->register_state.ext_ge_config =
		inw(MACH_REGISTER_R_EXT_GE_CONFIG);
	screen_state_p->register_state.saved_clock_sel =
		inw(MACH_REGISTER_CLOCK_SEL);
	
	/*
	 * Set CRTC parameters according to mode.
	 */

	if ((initialization_status = 
		 mach_get_mode(config_p, options_p, screen_state_p)))
	{
		return (initialization_status);
	}

	screen_state_p->register_state.ext_ge_config &=
		~MACH_EXT_GE_CONFIG_PIXEL_WIDTH;
	
	switch (screen_state_p->generic_state.screen_depth)
	{
	case 4 :
		screen_state_p->register_state.ext_ge_config |=
			MACH_EXT_GE_CONFIG_PIXEL_WIDTH_4;
		break;
	case 8 :
		screen_state_p->register_state.ext_ge_config |=
			MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8;
		break;
	case 16:
		screen_state_p->register_state.ext_ge_config |=
			MACH_EXT_GE_CONFIG_PIXEL_WIDTH_16;
		break;
	case 24:
		screen_state_p->register_state.ext_ge_config |=
			MACH_EXT_GE_CONFIG_PIXEL_WIDTH_24;
		break;
	default :
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}

	ASSERT(screen_state_p->generic_state.screen_physical_width != 0);
	
	max_pitch = screen_state_p->generic_state.screen_physical_width;
	
	if (max_pitch > 1024)
	{
		/*
		 * In these cases when we need to deviate from the standard
		 * pitches.
		 */
		screen_state_p->register_state.crt_pitch = (max_pitch/8);
		screen_state_p->register_state.ge_pitch = (max_pitch/8);
	}
	else
	{
		screen_state_p->register_state.crt_pitch = (1024/8);
		screen_state_p->register_state.ge_pitch = (1024/8);
	}
	
	/*
	 * Reset the scissors.
	 */

	screen_state_p->register_state.ext_scissor_t = 
	screen_state_p->register_state.ext_scissor_l = 0;

	screen_state_p->register_state.ext_scissor_r = config_p->virt_w-1;
	screen_state_p->register_state.ext_scissor_b = config_p->virt_h-1;

	/*
	 * The read and write masks.
	 */

	screen_state_p->register_state.wrt_mask = (unsigned short)
		((1 << screen_state_p->generic_state.screen_depth) - 1);
	screen_state_p->register_state.rd_mask = (unsigned short)
		((1 << screen_state_p->generic_state.screen_depth) - 1);

	/*
	 * Hardware cursor registers.
	 */
	
	screen_state_p->register_state.cursor_offset_lo = 0;
	screen_state_p->register_state.cursor_offset_hi = 0;
	screen_state_p->register_state.horz_cursor_posn = 0;
	screen_state_p->register_state.vert_cursor_posn = 0;
	screen_state_p->register_state.horz_cursor_offset = 0;
	screen_state_p->register_state.vert_cursor_offset = 0;
	screen_state_p->register_state.cursor_color_0 = 0;
	screen_state_p->register_state.cursor_color_1 = 0;
	screen_state_p->register_state.ext_cursor_color_0 = 0;
	screen_state_p->register_state.ext_cursor_color_1 = 0;
	
	/*
	 * Get the contents of the linear memory aperture registers.
	 */

	/*
	 * Forcifully set the video memory to SHARED.
	 */

	screen_state_p->register_state.mem_bndry = 0x0;

	screen_state_p->register_state.mem_cfg =
		inw(MACH_REGISTER_MEM_CFG);
	
	/*
	 * Overscan registers.
	 */
	screen_state_p->register_state.horz_overscan = 0;
	screen_state_p->register_state.vert_overscan = 0;
	screen_state_p->register_state.overscan_color_8 = 0;
	screen_state_p->register_state.overscan_blue_24 = 0;
	screen_state_p->register_state.overscan_green_24 = 0;
	screen_state_p->register_state.overscan_red_24 = 0;

#if (defined(EXTRA_FUNCTIONALITY))
	if (options_p->overscan_color)
	{
		unsigned short red_component, green_component, blue_component;
		extern int OsLookupColor(int, unsigned char *,  unsigned long,
								 unsigned short *, unsigned short *,
								 unsigned short *);	/* from the OS layer */
		

#if (defined(OSLOOKUPCOLOR_IS_CALLABLE))
		if (OsLookupColor(screen_state_p->generic_state.
						  screen_config_p->screen,
						  (unsigned char *) options_p->overscan_color,
						  strlen(options_p->overscan_color),
						  &red_component, &green_component,
						  &blue_component))
		{
			
			screen_state_p->register_state.overscan_blue_24 =
				blue_component;
			screen_state_p->register_state.overscan_green_24 =
				green_component;
			screen_state_p->register_state.overscan_red_24 =
				red_component;
			if (screen_state_p->generic_state.screen_depth < 8)
			{
				/*
				 * Place the color looked into the default colormap
				 * at the very end.
				 */
				unsigned short *color_values_p;
				/*
				 * Get an index to program this color into.
				 */
				int max_index =
					screen_state_p->generic_state.
					screen_visuals_list_p->si_visual_p->SVcmapsz - 1;
				
				color_values_p =
					screen_state_p->generic_state.
					screen_colormaps_pp[0]->rgb_values_p + 
					3 * max_index;

				/*
				 * save in colormap.
				 */

				*color_values_p++ = red_component;
				*color_values_p++ = green_component;
				*color_values_p = blue_component;

				/*
				 * Set register value.
				 */

				screen_state_p->register_state.overscan_color_8 =
					max_index;
			}
		}
		else
		{
			(void) fprintf(stderr, MACH_UNKNOWN_OVERSCAN_COLOR_NAME_MESSAGE,
						   options_p->overscan_color);
		}
#endif /* OSLOOKUPCOLOR_IS_CALLABLE */
#if (defined(NUMERIC_OVERSCAN_SPECIFICATION))
		if (screen_state_p->generic_state.screen_depth > 8)
		{
			/*
			 * 24 bit mode.
			 */
			if (sscanf(options_p->overscan_color, "%i %i %i",
					   &red_component, &green_component,
					   &blue_component) == 3)
			{
				screen_state_p->register_state.overscan_blue_24 =
					blue_component; 
				screen_state_p->register_state.overscan_green_24 = 
					green_component;
				screen_state_p->register_state.overscan_red_24 =
					red_component;
				screen_state_p->register_state.horz_overscan =
					options_p->overscan_h;
				screen_state_p->register_state.vert_overscan =
					options_p->overscan_v;
			}
			else 
			{
				(void) fprintf(stderr,
				   MACH_CANNOT_PARSE_HI_COLOR_OVERSCAN_COLOR_OPTION_MESSAGE,
							   options_p->overscan_color);
			}
		}
		else 
		{
			if (sscanf(options_p->overscan_color, "%i",
					   &red_component) == 1)
			{
				screen_state_p->register_state.overscan_color_8 =
					red_component;
				screen_state_p->register_state.horz_overscan =
					options_p->overscan_h;
				screen_state_p->register_state.vert_overscan =
					options_p->overscan_v;
			}
			else
			{
				(void) fprintf(stderr,
				   MACH_CANNOT_PARSE_OVERSCAN_COLOR_OPTION_MESSAGE, 
							   options_p->overscan_color);
			}
		}
#endif /* NUMERIC_OVERSCAN_SPECIFICATION */
		/*
		 * Fill in the overscan registers.
		 */
		screen_state_p->register_state.horz_overscan =
			options_p->overscan_h;
		screen_state_p->register_state.vert_overscan =
			options_p->overscan_v;

	}
#endif /* EXTRA_FUNCTIONALITY */

	return (initialization_status);
}

#if (defined(UNUSED_FUNCTIONALITY))

/*
 * mach_screen_control
 *
 * Handle switching between screens.
 */
STATIC SIBool
mach_screen_control(SIint32 screen_number, SIint32 control_request)
{
	return (SI_SUCCEED);
}

#endif /* UNUSED_FUNCTIONALITY */

/**
 **		Virtual terminal functionality.
 **/

/*
 * mach_save_screen_contents
 *
 */
STATIC void
mach_save_screen_contents(void *contents_p, int vt_switch_save_lines)
{
	struct mach_screen_state *screen_state_p =
		(struct mach_screen_state *) generic_current_screen_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	
#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_save_screen_contents)\n"
"{\n"
"\tcontents_p = %p\n"
"\tvt_switch_save_lines = %d\n"
"}\n",
					   contents_p,
					   vt_switch_save_lines);
	}
#endif

	if (vt_switch_save_lines <= 0)
	{
		return;
	}
	
	/*
	 * Do a screen to memory blit.
	 */
	
	ASSERT(screen_state_p->generic_state.screen_contents_p);
	
	if (screen_state_p->generic_state.screen_contents_p)
	{
		unsigned short dp_config =
			screen_state_p->dp_config_flags |
			MACH_DP_CONFIG_FG_COLOR_SRC_BLIT |
			MACH_DP_CONFIG_MONO_SRC_ONE |
			MACH_DP_CONFIG_ENABLE_DRAW;

		MACH_STATE_SET_FG_ROP(screen_state_p, MACH_MIX_FN_PAINT);

		/*
		 * Since we modify the clip registers : mark clip as invalid.
		 */
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p,
			 0, 0,
             screen_state_p->generic_state.screen_physical_width,
			 screen_state_p->generic_state.screen_physical_height);

		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIDEO_MEMORY;
		
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* deviation from SI's clip */

		MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

		mach_asm_transfer_helper(
				 screen_state_p->generic_state.screen_contents_p,
				 (screen_state_p->generic_state.screen_physical_width *
				  screen_state_p->generic_state.screen_depth) >> 3,
				 (screen_state_p->generic_state.screen_physical_width *
				  screen_state_p->generic_state.screen_depth) >>
					  screen_state_p->pixtrans_width_shift,
				 vt_switch_save_lines,
				 0, 0,
				 screen_state_p->generic_state.screen_physical_width,
				 vt_switch_save_lines,
				 screen_state_p->screen_read_pixels_p,
				 screen_state_p->pixtrans_register,
				 MACH_ASM_TRANSFER_FROM_VIDEO_MEMORY);
	}
	
	return;
}


STATIC void
mach_restore_screen_contents(void *contents_p, int vt_switch_save_lines)
{
	struct mach_screen_state *screen_state_p =
		(struct mach_screen_state *) generic_current_screen_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));

#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_restore_screen_contents)\n"
"{\n"
"\tcontents_p = %p\n"
"\tvt_switch_save_lines = %d\n"
"}\n",
					   contents_p,
					   vt_switch_save_lines);
	}
#endif

	if (vt_switch_save_lines <= 0)
	{
		return;
	}
	
	/*
	 * Restore screen bits.
	 */
	
	ASSERT(screen_state_p->generic_state.screen_contents_p);
	
	if (screen_state_p->generic_state.screen_contents_p)
	{
		unsigned short dp_config =
			screen_state_p->dp_config_flags |
			MACH_DP_CONFIG_FG_COLOR_SRC_HOST |
		    MACH_DP_CONFIG_MONO_SRC_ONE |
			MACH_DP_CONFIG_ENABLE_DRAW |
			MACH_DP_CONFIG_WRITE;

		MACH_STATE_SET_FG_ROP(screen_state_p, MACH_MIX_FN_PAINT);
		MACH_STATE_SET_WRT_MASK(screen_state_p, ~0U);
		
		/*
		 * Set clip registers : mark clip as full-screen.
		 */
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIDEO_MEMORY;
		
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p,
			 0, 0,
             screen_state_p->generic_state.screen_physical_width,
			 screen_state_p->generic_state.screen_physical_height);
		
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* deviation from SI's clip */

		MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

		mach_asm_transfer_helper(
				 screen_state_p->generic_state.screen_contents_p,
				 (screen_state_p->generic_state.screen_physical_width *
				  screen_state_p->generic_state.screen_depth) >> 3,
				 (screen_state_p->generic_state.screen_physical_width *
				  screen_state_p->generic_state.screen_depth) >>
					  screen_state_p->pixtrans_width_shift,
			     vt_switch_save_lines,
				 0, 0,
				 screen_state_p->generic_state.screen_physical_width,
                 vt_switch_save_lines,
				 screen_state_p->screen_write_pixels_p,
				 screen_state_p->pixtrans_register,
				 MACH_ASM_TRANSFER_TO_VIDEO_MEMORY);
	}
	
	return;
}

/*
 * mach_virtual_terminal_save
 *
 * Save as much of the register state as possible and restore CRTC
 * registers. 
 */
STATIC SIBool
mach_virtual_terminal_save(void)
{

	struct mach_screen_state *screen_state_p = 
		(struct mach_screen_state *) generic_current_screen_state_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	
	
#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p, "(mach) VT save called.\n");
	}
#endif
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	/*
	 * Save the contents of offscreen memory if necessary.
	 */

	if (generic_current_screen_state_p->screen_contents_p == NULL &&
		screen_state_p->vt_switch_save_lines > 0)
	{
		generic_current_screen_state_p->screen_contents_p =
			allocate_memory((screen_state_p->vt_switch_save_lines *
					 screen_state_p->generic_state.screen_physical_width * 
					 screen_state_p->generic_state.screen_depth) >> 3);
	}
	
	mach_save_screen_contents(generic_current_screen_state_p->
		      screen_contents_p, screen_state_p->vt_switch_save_lines);
	

	/*
	 * Inform each concerned module that we are about to lose control
	 * of the VT.
	 */
	mach__vt_switch_out__();
	
	/*
	 * Colormaps are kept in memory and so don't need to be saved.
	 */
	
	/*
	 * Restore the DAC to VGA mode.
	 */
	ASSERT(((struct mach_screen_state *)generic_current_screen_state_p)->
		   register_state.generic_state.dac_uninit);
   
	(((struct mach_screen_state *)
	  generic_current_screen_state_p)->register_state.generic_state.dac_uninit)
		(generic_current_screen_state_p);

	/*
	 * We don't know the state of the chipset when it returns.
	 */
	screen_state_p->current_graphics_engine_mode =
		MACH_GE_MODE_NULL_MODE;
		
	MACH_WAIT_FOR_ENGINE_IDLE();
	
	MACH_PASSTHROUGH_VGA(screen_state_p->register_state.saved_clock_sel);
	
	outb(MACH_REGISTER_MEM_BNDRY, 
			screen_state_p->register_state.saved_mem_bndry);
	/*
	 * Restore previous CRT state.
	 */
	/* CONSTANTCONDITION */
	MACH_LOCK_SHADOW_SET(MACH_SHADOW_REGISTER_SET_1);

	/* CONSTANTCONDITION */
	MACH_LOCK_SHADOW_SET(MACH_SHADOW_REGISTER_SET_2);

	return (SI_SUCCEED);
}

/*
 * mach_virtual_terminal_restore
 *
 * Program the primary CRTC set with the required parameters.
 */
STATIC SIBool
mach_virtual_terminal_restore()
{
	struct mach_screen_state *screen_state_p = 
		(struct mach_screen_state *) generic_current_screen_state_p;

	struct generic_colormap *colormap_p;
	unsigned short *rgb_p;
	int i;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));

#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p, "(mach) VT restore called.\n");
	}
#endif
	
	MACH_WAIT_FOR_ENGINE_IDLE();
	
	/* CONSTANTCONDITION */
	MACH_UNLOCK_SHADOW_SET(MACH_SHADOW_REGISTER_SET_1);
	/* CONSTANTCONDITION */
	MACH_UNLOCK_SHADOW_SET(MACH_SHADOW_REGISTER_SET_2);
	/* CONSTANTCONDITION */
	MACH_SELECT_SHADOW_SET(MACH_PRIMARY_REGISTER_SET);
	
	MACH_DISABLE_CRT_CONTROLLER(screen_state_p->register_state.disp_cntl);

	MACH_PASSTHROUGH_VGA_ADVFUNC_CNTL(screen_state_p->
									  register_state.advfunc_cntl); 
	MACH_PASSTHROUGH_VGA(screen_state_p->register_state.clock_sel);

	if ((screen_state_p->chipset_kind == MACH_CHIPSET_ATI_38800) ||
		 (screen_state_p->chipset_kind == MACH_CHIPSET_ATI_68800))
	{
#if (defined(__DEBUG__))
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, 
"(mach_virtual_terminal_restore)\n"
"{\n"
"\treprogramming CRT.\n"
"}\n"
						   );
		}
#endif

		ASSERT(screen_state_p->register_state.
			   generic_state.register_put_state);
		
		if (!(*screen_state_p->register_state.generic_state.register_put_state)
			((struct generic_screen_state *) screen_state_p))
		{
#if (defined(__DEBUG__))
			if (mach_debug)
			{
				(void) fprintf(debug_stream_p,
							   "(mach_virtual_terminal_restore) register "
							   "init failed.\n");
			}
#endif
			return (SI_FAIL);
		}

		if (!screen_state_p->register_state.generic_state.dac_init)
		{
			return (SI_FAIL);
		}
		
		
		if (!(*screen_state_p->register_state.generic_state.dac_init)
			((struct generic_screen_state *) screen_state_p))
		{
#if (defined(__DEBUG__))
			if (mach_debug)
			{
				(void) fprintf(debug_stream_p,
							   "(mach_virtual_terminal_restore) dac "
							   "init failed.\n");
			}
#endif
			return (SI_FAIL);
		}
		
	}
	else						/* Unknown chipset */
	{
#if (defined(__DEBUG__))
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, 
						   "(mach) vt restore : unknown chipset.\n");
		}
#endif
		return (SI_FAIL);
	}
	
	/*
	 * switch to ATI mode.
	 */

	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	/*
	 * Restore screen bits.
	 */
	if (generic_current_screen_state_p->screen_contents_p)
	{
	
		mach_restore_screen_contents(generic_current_screen_state_p->
			screen_contents_p, screen_state_p->vt_switch_save_lines);
	}

	/*
	 * reprogram the colormap too ... since SI does not currently
	 * support multiple colormaps, we reprogram colormap index 0.
	 *
	 * If the screen is in TrueColor mode, we may not have a
	 * programmable colormap.
	 */
	
	colormap_p =
		screen_state_p->generic_state.screen_colormaps_pp[0];

	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP, colormap_p));
	
	rgb_p = (colormap_p) ? colormap_p->rgb_values_p : NULL;

	ASSERT(!colormap_p || rgb_p != NULL); 
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL, colormap_p->visual_p));

	/*
	 * restore the colormap if there is a way to do this.
	 */

	if (colormap_p && colormap_p->visual_p->set_color_method_p)
	{
		/*
		 * program the number of entries in the colormap.
		 */
		for (i = 0; i < colormap_p->si_colormap.sz; i++)
		{
			(*colormap_p->visual_p->set_color_method_p)
				(colormap_p->visual_p,
				 i,
				 rgb_p);
			rgb_p += 3;
		}
	}
	
	/*
	 * Inform other interested modules that we are about to regain
	 * control of our VT.
	 */
	mach__vt_switch_in__();
	
	/*
	 * Enable the CRT controller etc.
	 */

	MACH_ENABLE_CRT_CONTROLLER(screen_state_p->register_state.disp_cntl);

	MACH_PASSTHROUGH_8514_ADVFUNC_CNTL(screen_state_p->
									   register_state.advfunc_cntl); 
	MACH_PASSTHROUGH_8514(screen_state_p->register_state.clock_sel);

	/*
	 * Reprogram the CRTC and GE pitches.
	 */

	outw(MACH_REGISTER_CRT_PITCH, screen_state_p->register_state.crt_pitch);
	outw(MACH_REGISTER_GE_PITCH, screen_state_p->register_state.ge_pitch);

	return (SI_SUCCEED);
}


#if (defined(UNUSED_FUNCTIONALITY))

/*
 * Restore 640x480 8 bit per pixel parameters to shadow set.
 */

STATIC void
mach_restore_default_shadow_set_parameters()
{

	outw(MACH_REGISTER_CRT_PITCH, 128);
	outw(MACH_REGISTER_GE_PITCH, 128);

	outb(MACH_REGISTER_H_TOTAL, 0x63);
	outb(MACH_REGISTER_H_DISP, 0x4F);
	outb(MACH_REGISTER_H_SYNC_STRT, 0x52);
	outb(MACH_REGISTER_H_SYNC_WID, 0x2C);

	outw(MACH_REGISTER_V_TOTAL, 0x418);
	outw(MACH_REGISTER_V_DISP, 0x3BF);
	outw(MACH_REGISTER_V_SYNC_STRT, 0x3D6);
	outw(MACH_REGISTER_V_SYNC_WID, 0x22);

	outw(MACH_REGISTER_CLOCK_SEL, 0x50);

	MACH_DELAY();
	
	outw(MACH_REGISTER_DISP_CNTL, 0x23);
	
}
#endif /* UNUSED_FUNCTIONALITY */

/*
 * Restore screen :
 *
 * Put back contents of video memory, put back contents of the
 * colormap, enable the shadow sets.
 */
STATIC SIBool
mach_restore_screen(void)
{
	struct mach_screen_state *screen_state_p =
		(struct mach_screen_state *) generic_current_screen_state_p;
	
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	

#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(mach_restore_screen) called.\n");
	}
#endif	

	/*
	 * Switch to ATI context.
	 */

	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	/*
	 * Inform each concerned module that we are about to lose control
	 * of the VT.
	 */

	mach__vt_switch_out__();
	
	/*
	 * Restore the DAC to VGA mode.
	 */

	ASSERT(((struct mach_screen_state *)generic_current_screen_state_p)->
		   register_state.generic_state.dac_uninit);
   
	if (!(((struct mach_screen_state *)
		   generic_current_screen_state_p)->
		  register_state.generic_state.dac_uninit))
	{
		/*
		 *  A problem was detected at INIT time ...
		 */
		return (SI_FAIL);
	}
		
	(((struct mach_screen_state *)
	  generic_current_screen_state_p)->register_state.generic_state.dac_uninit)
		(generic_current_screen_state_p);


	MACH_PASSTHROUGH_VGA(screen_state_p->register_state.saved_clock_sel);
	
	outb(MACH_REGISTER_MEM_BNDRY, 
			screen_state_p->register_state.saved_mem_bndry);
	
	/* CONSTANTCONDITION */
	MACH_LOCK_SHADOW_SET(MACH_SHADOW_REGISTER_SET_1);
	/* CONSTANTCONDITION */
	MACH_LOCK_SHADOW_SET(MACH_SHADOW_REGISTER_SET_2);
	/* CONSTANTCONDITION */
	MACH_SELECT_SHADOW_SET(MACH_PRIMARY_REGISTER_SET);
	
	return (SI_SUCCEED);
	
}

/* 
 * mach_initialize_display_library
 * 
 * Initialize the chipset specific SDD code.  This code will parse the
 * user specified options and configure its behaviour accordingly.  
 */

function SIBool
mach_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
								SIScreenRec *si_screen_p, 
								struct mach_screen_state *screen_state_p)
{
	SIConfigP config_p;
	
	unsigned int initialization_status = 0;
	unsigned char bios_major_revision = '?', bios_minor_revision = '?';
	unsigned char chipset_asic_revision= '?';
	int screen_pitch, screen_height;
	SIFunctions saved_generic_functions;
	
	struct mach_options_structure *mach_options_p = NULL;
	char mach_options_string[DEFAULT_MACH_STRING_OPTION_SIZE];
	
#if (defined(__DEBUG__))
	extern void mach_debug_control(boolean);
	
	mach_debug_control(TRUE);
#endif /* __DEBUG__ */

	config_p = si_screen_p->cfgPtr;
	
#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p, "(mach)\t"
				"mach_initialize_display_library called.\n");
	}
#endif /* __DEBUG__ */
	
	ASSERT(screen_state_p != NULL);
	ASSERT(si_screen_p != NULL);

	(void) fprintf(stderr, DEFAULT_MACH_STARTUP_MESSAGE);
	
	/*
	 * Initialize the generic data structure.
	 */

	initialization_status = 
		generic_initialize_display_library(virtual_terminal_file_descriptor,
		   si_screen_p, (struct generic_screen_state *) screen_state_p);
	
	if (initialization_status)
	{
#if (defined(__DEBUG__))
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, "(mach)\tgeneric initialization "
					"failed.\n"); 
		}
#endif /* __DEBUG__ */

		return (SI_FAIL);

	}

	/*
	 * Tuck away a copy of the generic screen functions.
	 */

	saved_generic_functions = *si_screen_p->funcsPtr;

	/*
	 * User option handling.  Null terminate the buffer first.
	 */

	mach_options_string[0] = '\0';
	
	/*
	 * Add a file parsing option if the standard options file is
	 * present.
	 */

	if (access(DEFAULT_MACH_STANDARD_OPTION_FILE_NAME, F_OK | R_OK) == 0)
	{
		(void) strcpy(mach_options_string,
					  DEFAULT_MACH_PARSE_STANDARD_OPTIONS_FILE_COMMAND);
	}

	/*
	 * Add parsing options for the user specified option string.
	 */

	(void) strcat(mach_options_string, config_p->info);
	(void) strcat(mach_options_string, " ");

	/*
	 * Add parsing options for the standard environment option variables 
	 */

	(void) strcat(mach_options_string,
				  DEFAULT_MACH_PARSE_STANDARD_ENVIRONMENT_VARIABLES_COMMAND);
	
	/*
	 * parse user configurable options.
	 */

	if (!(mach_options_p = 
		  mach_options_parse(NULL, mach_options_string)))
	{
		
#if (defined(__DEBUG__))
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, "(mach)\tparsing of user options "
					"failed.\n"); 
		}
#endif /* __DEBUG__ */
		
		initialization_status |=
			MACH_INITIALIZE_PARSE_USER_OPTIONS_FAILED;

		return (initialization_status);

	}
	else
	{

		/*
		 * Save the options for later persual by code.
		 */

		screen_state_p->options_p = mach_options_p;

	}
	
	/*
	 * Set the SDD version number as requested.  By default this is
	 * handled in "generic.c".
	 */

	if (mach_options_p->si_interface_version !=
		MACH_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE)
	{

		switch (mach_options_p->si_interface_version)
		{

		case MACH_OPTIONS_SI_INTERFACE_VERSION_1_0:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version =
					DM_SI_VERSION_1_0;
			break;

		case MACH_OPTIONS_SI_INTERFACE_VERSION_1_1:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version = DM_SI_VERSION_1_1;
			break;

		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;

		}
	}

	
	/*
	 * Essential stuff for controlling register accesses.
	 */

	/*
	 * Set the loop timeout value.
	 */

	mach_graphics_engine_loop_timeout_count =
		mach_options_p->graphics_engine_loop_timeout_count;

	ASSERT(mach_graphics_engine_loop_timeout_count > 0);
	
	/*
	 * Set the fifo blocking factor.
	 */

	mach_graphics_engine_fifo_blocking_factor =
		mach_options_p->graphics_engine_fifo_blocking_factor;

	ASSERT(mach_graphics_engine_fifo_blocking_factor > 0);

	/*
	 * Set the micro delay count.
	 */

	mach_graphics_engine_micro_delay_count =
		mach_options_p->graphics_engine_micro_delay_count;

	ASSERT(mach_graphics_engine_micro_delay_count > 0);

#if (defined(USE_KD_GRAPHICS_MODE))

	if(!mach_set_virtual_terminal_mode(virtual_terminal_file_descriptor,
								   KD_GRAPHICS))
	{
		perror(MACH_CANNOT_SET_GRAPHICS_MODE_MESSAGE); 
		free_memory(mach_options_p);
		return (SI_FAIL);
	}

#endif	/* USE_KD_GRAPHICS_MODE */

#if (defined(USE_SYSI86))

	/*
	 * The X server needs access to many I/O addresses.  We use the
	 * `sysi86()' call to get access to these.  Another alternative to
	 * this approach would be to have a kernel driver which
	 * allows the X server process access to the ATI registers.
	 */

	if (SET_IOPL(PS_IOPL) < 0)
	{
		perror(MACH_CANNOT_ENABLE_REGISTER_ACCESS_MESSAGE);
		free_memory(mach_options_p);
		return (SI_FAIL);
	}

#endif /* USE_SYSI86 */

#if (defined(USE_ATI_DRIVER))

	/*
	 * Open a driver and enable access to the IO ports.
	 */

	if (open(DEFAULT_MACH_SPECIAL_DEVICE_NAME, O_RDONLY) != -1)
	{
		perror(MACH_CANNOT_OPEN_SPECIAL_DEVICE_MESSAGE);
	}

#endif

	/*
	 * Attempt to detect the chipset.
	 */

	if ((mach_options_p->chipset_name ==
		 MACH_OPTIONS_CHIPSET_NAME_AUTO_DETECT) &&
		mach_detect_ati(&bios_major_revision, &bios_minor_revision,
						&chipset_asic_revision) == FALSE)
	{

#if (defined(__DEBUG__))

		if (mach_debug)
		{
			
			(void) fprintf(debug_stream_p, "(mach)\tdetection of ATi chipset "
					"failed.\n");
		}

#endif
		
		initialization_status |= MACH_INITIALIZE_ATI_CHIPSET_DETECTION_FAILED;
		
	}
	else
	{

		/*
		 * The user specified the chipset OR auto-detection detected
		 * an ATI chipset.
		 */

		screen_state_p->bios_major_revision = bios_major_revision;
		screen_state_p->bios_minor_revision = bios_minor_revision;

		/*
		 * Override the chipset_revision if the user specified these. 
		 */

		if (mach_options_p->chipset_revision)
		{
			(void) sscanf(mach_options_p->chipset_revision, "%c",
						  &chipset_asic_revision);
		}

		screen_state_p->chipset_asic_revision =
			chipset_asic_revision;

#if (defined(__DEBUG__))

		if (mach_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach)\n"
"{\n"
"\tbios revision = %d.%d.\n"
"\tasic revision = %c\n"
"}\n",
						   screen_state_p->bios_major_revision,
						   screen_state_p->bios_minor_revision,
						   screen_state_p->chipset_asic_revision);
		
		}
#endif

	}

	/*
	 * Get the video memory size.
	 */
	
	if (mach_options_p->video_memory_dimensions)
	{
		
		if (sscanf(mach_options_p->video_memory_dimensions, "%ix%i",
				   &screen_pitch, &screen_height) != 2)
		{
			(void) fprintf(stderr,
						   MACH_CANNOT_PARSE_SCREEN_DIMENSIONS_MESSAGE,
						   mach_options_p->video_memory_dimensions);
			/*
			 * Reset to default.
			 */

			screen_pitch = (config_p->disp_w > 1024) ?
				config_p->disp_w : 1024;
			screen_pitch = (config_p->virt_w > screen_pitch) ?
				config_p->virt_w : screen_pitch;
			screen_height = 1024;
			if (config_p->depth == 16)
			{
				screen_height = 768;
			}
		}
		
	}
	else
	{
		/*
		 * Reset to defaults.
		 */

		screen_pitch = (config_p->disp_w > 1024) ? config_p->disp_w :
			1024;
		screen_pitch = (config_p->virt_w > screen_pitch) ?
			config_p->virt_w : screen_pitch;
		screen_height = (config_p->videoRam *  1024) / screen_pitch;

		/*
		 * Do proper computation for 16 bits per pixel.
		 */
		if (config_p->depth == 16)
		{
			screen_height = (config_p->videoRam * 512) / screen_pitch;
		}

	}		

	screen_state_p->generic_state.screen_physical_width =
		screen_pitch;

	screen_state_p->generic_state.screen_physical_height =
		screen_height;
	
	/*
	 * Note what the user has specified as the configured memory size.
	 */

	screen_state_p->video_memory_size = 
		config_p->videoRam * 1024;
		
#if (defined(__DEBUG__))

	if (mach_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_initialize_display_library)\n"
"{\n"
"\tscreen_pitch = %d\n"
"\tscreen_height = %d\n"
"}\n",
					   screen_pitch,
					   screen_height);
	}

#endif

	if (((mach_options_p->chipset_name == 
		  MACH_OPTIONS_CHIPSET_NAME_AUTO_DETECT) && 
		 (mach_detect_mach32() == TRUE)) ||
		(mach_options_p->chipset_name == MACH_OPTIONS_CHIPSET_NAME_MACH_32))
	{
		int dac_kind;
		
		screen_state_p->chipset_kind =
			MACH_CHIPSET_ATI_68800; 

		/*
		 * Read the status registers.
		 */

		screen_state_p->register_state.config_status_1 = 
			inw(MACH_REGISTER_CONFIG_STATUS_1);
		screen_state_p->register_state.config_status_2 = 
			inw(MACH_REGISTER_CONFIG_STATUS_2);
		screen_state_p->register_state.subsys_stat =
			inw(MACH_REGISTER_SUBSYS_STATUS);
		/*
		 * JUN 6:
		 * Save the memory boundary register, as we are going to
		 * program it to 'video memory SHARED status'.
		 *
		 */
		screen_state_p->register_state.saved_mem_bndry =
			inb(MACH_REGISTER_MEM_BNDRY);

		/*
		 * detect the DAC.
		 */

		if (mach_options_p->dac_name ==
			MACH_OPTIONS_DAC_NAME_AUTO_DETECT)
		{
			dac_kind = (screen_state_p->register_state.config_status_1 &
						MACH32_DAC_TYPE) >> MACH32_DAC_TYPE_SHIFT; 
		}
		else
		{
			/*
			 * Select user supplied dac name.  User supplied dac-names
			 * are a superset of the auto-detectable names.
			 */

			switch (mach_options_p->dac_name)
			{
			case MACH_OPTIONS_DAC_NAME_ATI_68830 :
				dac_kind = MACH_DAC_ATI_68830;
				break;

			case MACH_OPTIONS_DAC_NAME_ATT_20C491:
			case MACH_OPTIONS_DAC_NAME_SIERRA_SC11_48X :
				dac_kind = MACH_DAC_SIERRA_SC11;
				break;

			case MACH_OPTIONS_DAC_NAME_ATI_68875_CFN:
				dac_kind = MACH_DAC_ATI_68875_CFN;
				break;
				
			case MACH_OPTIONS_DAC_NAME_ATI_68875_BFN : /* synonym */
			case MACH_OPTIONS_DAC_NAME_TI_TLC_34075 :
				dac_kind = MACH_DAC_TI_TLC_34075;
				break;


			case MACH_OPTIONS_DAC_NAME_BT_478 :
			case MACH_OPTIONS_DAC_NAME_IMS_G178J_80Z: /* synonym */
				dac_kind = MACH_DAC_BT_478;
				break;

			case MACH_OPTIONS_DAC_NAME_BT_481 :
				dac_kind = MACH_DAC_BT_481;
				break;

			case MACH_OPTIONS_DAC_NAME_BT_476: 
			case MACH_OPTIONS_DAC_NAME_IMS_G176J_80Z : /* synonym */
				dac_kind = MACH_DAC_IMS_G176J_80Z;
				break;
				
			default :
				dac_kind = MACH_DAC_UNKNOWN;
				break;

			}
		}
		
		ASSERT(dac_kind >= MACH_DAC_ATI_68830 && dac_kind < MACH_DAC_UNKNOWN);

		screen_state_p->dac_kind = dac_kind;
		
		/*
		 * determine the video memory organization.
		 */

		screen_state_p->mem_kind = (enum mach_mem_kind)
			((screen_state_p->register_state.config_status_1 &
			 MACH32_MEM_TYPE) >> MACH32_MEM_TYPE_SHIFT);

		ASSERT(screen_state_p->mem_kind >= MACH_MEM_KIND_DRAM_256Kx4
			   && screen_state_p->mem_kind < MACH_MEM_KIND_COUNT);
			   
		/*
		 * Determine the physical memory size
		 */

		screen_state_p->register_state.misc_options =
			inw(MACH_REGISTER_MISC_OPTIONS);
	
		switch (screen_state_p->register_state.misc_options &
				MACH_MISC_OPTIONS_MEM_SIZE_ALIAS)
		{
		case MACH_MISC_OPTIONS_512_KB :
			screen_state_p->physical_video_memory_size = 512 * 1024;
			break;
			
		case MACH_MISC_OPTIONS_1_MB :
			screen_state_p->physical_video_memory_size = 1024 * 1024;
			break;

		case MACH_MISC_OPTIONS_2_MB :
			screen_state_p->physical_video_memory_size = 2048 * 1024;
			break;

		case MACH_MISC_OPTIONS_4_MB :
			screen_state_p->physical_video_memory_size = 4096 * 1024;
			break;

		default:
			screen_state_p->physical_video_memory_size = -1;
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
			
		}
		
		
		/*
		 * determine the bus connection.
		 */

		screen_state_p->bus_kind = (enum mach_bus_kind)
			((screen_state_p->register_state.config_status_1 &
			 MACH32_BUS_TYPE) >> MACH32_BUS_TYPE_SHIFT);
		
		ASSERT(screen_state_p->bus_kind >=0 &&
			   screen_state_p->bus_kind < MACH_BUS_KIND_COUNT);
		
		/*
		 * determine the bus width.
		 */

		switch (mach_options_p->io_bus_width)
		{
		case MACH_OPTIONS_IO_BUS_WIDTH_AUTO_DETECT :

			/*
			 * If the bus kind is ISA or EISA, look for an 8 bit bus.
			 * Otherwise the IO bus is assumed to be 16 bits wide (for
			 * the local buses).
			 */

			screen_state_p->bus_width = 
				((screen_state_p->bus_kind == MACH_BUS_KIND_ISA || 
				  screen_state_p->bus_kind == MACH_BUS_KIND_EISA) &&
				 !(screen_state_p->register_state.config_status_2 &
				  MACH32_ISA_16_ENA)) ? MACH_BUS_WIDTH_8 :
				 MACH_BUS_WIDTH_16;
			screen_state_p->pixtrans_width =
				(screen_state_p->bus_width == MACH_BUS_WIDTH_8) ? 8 :
				16;
			
			break;
			
		case MACH_OPTIONS_IO_BUS_WIDTH_16_BIT :
			screen_state_p->bus_width = MACH_BUS_WIDTH_16;
			screen_state_p->pixtrans_width = 16;
			break;

		case MACH_OPTIONS_IO_BUS_WIDTH_8_BIT :
			screen_state_p->bus_width = MACH_BUS_WIDTH_8;
			screen_state_p->pixtrans_width = 8;
			break;

		default :
			screen_state_p->bus_width = MACH_BUS_WIDTH_UNKNOWN;
			break;

		}

#if (defined(__DEBUG__))
		
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, 
"(mach)\n"
"{\n"
"\tmach32 detected\n"
"\tdac kind = %s\n"
"\tvideo_memory_kind = %s\n"
"\tmemory_size = %d\n"
"\tbus_kind = %s\n"
"\tbus_width = %s\n"
"}\n",
					mach_dac_kind_to_dac_kind_dump[screen_state_p->dac_kind],
					mach_mem_kind_to_mem_kind_dump[screen_state_p->mem_kind],
					screen_state_p->video_memory_size,
					mach_bus_kind_to_bus_kind_dump[screen_state_p->bus_kind],
					mach_bus_width_to_bus_width_dump
					[screen_state_p->bus_width]); 
		}

#endif

		/*
		 * Fill in CRTC registers according to requested mode.
		 */

		initialization_status |= 
			mach32_set_registers(config_p, mach_options_p, 
									  screen_state_p);
		
	}
	else if (((mach_options_p->chipset_name == 
			   MACH_OPTIONS_CHIPSET_NAME_AUTO_DETECT) && 
			  (mach_detect_mach8() == TRUE)) ||
			 (mach_options_p->chipset_name == 
			  MACH_OPTIONS_CHIPSET_NAME_MACH_8))
	{

		screen_state_p->chipset_kind = MACH_CHIPSET_ATI_38800; 
		screen_state_p->dac_kind = MACH_DAC_IMS_G176J_80Z;

		/*
		 * read in the status registers.
		 */

		screen_state_p->register_state.config_status_1 =
			inw(MACH_REGISTER_CONFIG_STATUS_1);
		screen_state_p->register_state.config_status_2 =
			inw(MACH_REGISTER_CONFIG_STATUS_2);
		screen_state_p->register_state.subsys_stat =
			inw(MACH_REGISTER_SUBSYS_STATUS);
		
		screen_state_p->physical_video_memory_size =
			(screen_state_p->register_state.config_status_1 &
			 MACH8_MEM_INSTALLED)? (1024*1024) : (512*1024);
		
		/*
		 * determine the bus width
		 */

		switch (mach_options_p->io_bus_width)
		{

		case MACH_OPTIONS_IO_BUS_WIDTH_AUTO_DETECT :
			screen_state_p->bus_width =
				(screen_state_p->register_state.config_status_1 &
				 MACH8_BUS_16) ? MACH_BUS_WIDTH_16 : MACH_BUS_WIDTH_8;
			screen_state_p->pixtrans_width =
				(screen_state_p->bus_width == MACH_BUS_WIDTH_8) ? 8 :
				16;
			
			break;
			
		case MACH_OPTIONS_IO_BUS_WIDTH_16_BIT :
			screen_state_p->pixtrans_width = 16;
			screen_state_p->bus_width = MACH_BUS_WIDTH_16;
			break;

		case MACH_OPTIONS_IO_BUS_WIDTH_8_BIT :
			screen_state_p->pixtrans_width = 8;
			screen_state_p->bus_width = MACH_BUS_WIDTH_8;
			break;

		default :
			screen_state_p->bus_width = MACH_BUS_WIDTH_UNKNOWN;
			break;

		}

		/*
		 * determine the bus kind : the mach8 does not support the
		 * EISA bus, or the 32 bit Microchannel.
		 */

		screen_state_p->bus_kind = 
			(screen_state_p->register_state.config_status_1 & MACH8_MC_BUS) ? 
				MACH_BUS_KIND_UC_16 : MACH_BUS_KIND_ISA;

#if (defined(__DEBUG__))

		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, 
"(mach)\n"
"{\n"
"\tmach8 detected\n"
"\tdac kind = %s\n"
"\tvideo size = %d\n"
"\tbus_width = %s\n"
"\tbus_kind = %s\n"
"}\n",
						   mach_dac_kind_to_dac_kind_dump
						   [screen_state_p->dac_kind],
						   screen_state_p->video_memory_size,
						   mach_bus_width_to_bus_width_dump
						   [screen_state_p->bus_width],
						   mach_bus_kind_to_bus_kind_dump
						   [screen_state_p->bus_kind]); 
		}
#endif 

		/*
		 * Fill in CRTC registers according to requested mode.
		 */

		initialization_status |= 
			mach8_set_registers(config_p, mach_options_p, 
									 screen_state_p);

	}
	else
	{

#if (defined(__DEBUG__))
		if (mach_debug)
		{
			(void) fprintf(debug_stream_p, "(mach)\tunknown ATi chipset.\n");
		}
#endif /* __DEBUG__ */

		screen_state_p->chipset_kind = MACH_CHIPSET_ATI_UNKNOWN; 
		screen_state_p->dac_kind = MACH_DAC_UNKNOWN;

		initialization_status |=
			(MACH_INITIALIZE_ATI_MACH8_DETECTION_FAILED|
			 MACH_INITIALIZE_ATI_MACH32_DETECTION_FAILED);
		
	}

	/*
	 * The time has come to fill in state.
	 */

	/*
	 * How many lines of the screen does the user want saved?
	 * Limit this to the max video memory lines available.
	 */

	screen_state_p->vt_switch_save_lines =
		(((mach_options_p->vt_switch_save_lines < 0) ||
		  (mach_options_p->vt_switch_save_lines >
		   screen_state_p->generic_state.screen_physical_height)) ? 
		 screen_state_p->generic_state.screen_physical_height : 
		 mach_options_p->vt_switch_save_lines);
	
	/*
	 * check for coordinates exceeding the graphics engine coordinate
	 * space.
	 */

	if (screen_state_p->vt_switch_save_lines > 
		DEFAULT_MACH_MAX_ATI_COORDINATE)
	{
		screen_state_p->vt_switch_save_lines =
			DEFAULT_MACH_MAX_ATI_COORDINATE;
	}
	

#if (defined(__DEBUG__))
	if (mach_debug)
	{
		(void) fprintf(debug_stream_p, 
"(mach)\tvt save size = %d.\n",
					   screen_state_p->vt_switch_save_lines);

	}
#endif
	
	si_screen_p->funcsPtr->si_vt_save = mach_virtual_terminal_save;
	si_screen_p->funcsPtr->si_vt_restore = mach_virtual_terminal_restore;
	si_screen_p->funcsPtr->si_restore = mach_restore_screen;
	
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
			mach_cursor_make_named_allocation_string(si_screen_p,
			 mach_options_p);

		/* 
		 * Add one for the comma which will seperate each named allocate
		 * string
		 */
		buffer_p = (char *)allocate_memory(
				DEFAULT_MACH_OMM_INITIALIZATION_STRING_LENGTH + 1 +
				strlen(cursor_named_allocation_string_p) + 1 +
				strlen(mach_options_p->omm_named_allocation_list));
		/*
		 *See if the user has supplied any additional named allocate
		 *requests
		 */
		if (mach_options_p->omm_named_allocation_list &&
			mach_options_p->omm_named_allocation_list[0] != '\0')
		{
			sprintf(buffer_p, DEFAULT_MACH_OMM_INITIALIZATION_FORMAT ",%s,%s",
					DEFAULT_MACH_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height,
					screen_state_p->generic_state.screen_depth,
					0,0,0,
					cursor_named_allocation_string_p,
					mach_options_p->omm_named_allocation_list);
		}
		else
		{
			sprintf(buffer_p, DEFAULT_MACH_OMM_INITIALIZATION_FORMAT ",%s",
					DEFAULT_MACH_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height,
					screen_state_p->generic_state.screen_depth,
					0,0,0,
					cursor_named_allocation_string_p);

		}
		
		omm_options.total_width =
		 screen_state_p->generic_state.screen_physical_width;

		omm_options.total_height =
		   (screen_state_p->generic_state.screen_physical_height > 
			DEFAULT_MACH_MAX_ATI_COORDINATE) ? 
			DEFAULT_MACH_MAX_ATI_COORDINATE  :
			screen_state_p->generic_state.screen_physical_height;

		omm_options.total_depth =
		 screen_state_p->generic_state.screen_depth;
		omm_options.horizontal_constraint = 
				(mach_options_p->omm_horizontal_constraint ?
				 mach_options_p->omm_horizontal_constraint : 
				 screen_state_p->pixtrans_width);
		omm_options.vertical_constraint = 
			   (mach_options_p->omm_vertical_constraint ?
			    mach_options_p->omm_vertical_constraint :
				DEFAULT_MACH_OMM_VERTICAL_CONSTRAINT);
		omm_options.neighbour_list_increment = 
			mach_options_p->omm_neighbour_list_increment;
		omm_options.full_coalesce_watermark = 
			mach_options_p->omm_full_coalesce_watermark;
		omm_options.hash_list_size = 
			mach_options_p->omm_hash_list_size;
		omm_options.named_allocations_p = buffer_p;

		 (void)omm_initialize(&omm_options);

		 free_memory( buffer_p );
	}
	
	/*
	 * Let each module initialize itself.
	 */
	mach__initialize__(si_screen_p, mach_options_p);

	if(screen_state_p->generic_state.screen_current_graphics_state_p)
	{
		int count;

		for(count = 0; count <
			screen_state_p->generic_state.screen_number_of_graphics_states; 
			count ++)
		{

			struct mach_graphics_state *graphics_state_p = 
				(struct mach_graphics_state *)
					screen_state_p->generic_state.
						screen_graphics_state_list_pp[count];
					   
			ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
									 (struct generic_graphics_state *)
									 graphics_state_p));
	
			ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
									 graphics_state_p));

			/*
			 * Save the generic function table
			 */

			graphics_state_p->generic_si_functions =
				saved_generic_functions;
		}

	}
	
	/*
	 * Verbose startup options
	 */

	if (mach_options_p->verbose_startup ==
		MACH_OPTIONS_VERBOSE_STARTUP_YES)
	{
		(void) fprintf(stderr,
					   MACH_VERBOSE_STARTUP_PROLOGUE);
		
		(void) fprintf(stderr,
					   MACH_VERBOSE_STARTUP_SDD_PARAMETERS_MESSAGE,
					   screen_state_p->generic_state.
					   screen_server_version_number,
					   screen_state_p->generic_state.
					   screen_sdd_version_number,
					   ((mach_options_p->si_interface_version ==
						 MACH_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE) ? 
						MACH_VERBOSE_STARTUP_AUTO_CONFIGURED :
						MACH_VERBOSE_STARTUP_USER_SPECIFIED)); 

		(void) fprintf(stderr,
					   MACH_VERBOSE_STARTUP_BOARD_CONFIGURATION_MESSAGE,
					   mach_chipset_kind_to_chipset_description[
								screen_state_p->chipset_kind],
					   ((mach_options_p->chipset_name ==
						 MACH_OPTIONS_CHIPSET_NAME_AUTO_DETECT) ?
						MACH_VERBOSE_STARTUP_AUTO_DETECTED : 
						MACH_VERBOSE_STARTUP_USER_SPECIFIED),
					   screen_state_p->bios_major_revision,
					   screen_state_p->bios_minor_revision,
					   screen_state_p->chipset_asic_revision,
					   mach_dac_kind_to_dac_description[
								screen_state_p->dac_kind],
					   ((mach_options_p->dac_name ==
						 MACH_OPTIONS_DAC_NAME_AUTO_DETECT) ?
						MACH_VERBOSE_STARTUP_AUTO_DETECTED :
						MACH_VERBOSE_STARTUP_USER_SPECIFIED),
					   mach_mem_kind_to_description[screen_state_p->
													mem_kind],
					   mach_bus_kind_to_description[screen_state_p->
													bus_kind],
					   mach_bus_width_to_description[screen_state_p->
													 bus_width],
					   ((mach_options_p->io_bus_width ==
						 MACH_OPTIONS_IO_BUS_WIDTH_AUTO_DETECT) ?
						MACH_VERBOSE_STARTUP_AUTO_DETECTED :
						MACH_VERBOSE_STARTUP_USER_SPECIFIED),
					   screen_state_p->clock_chip_p->clock_chip_name_p,
					   screen_state_p->video_memory_size / 1024,
					   screen_state_p->physical_video_memory_size / 1024);

		(void) fprintf(stderr,
					   MACH_VERBOSE_STARTUP_GRAPHICS_ENGINE_PARAMETERS_MESSAGE,
					   mach_graphics_engine_fifo_blocking_factor,
					   ((screen_state_p->register_state.clock_sel &
						 MACH_CLOCK_SEL_VFIFO_DEPTH) >> 
						MACH_CLOCK_SEL_VFIFO_DEPTH_SHIFT),
					   mach_graphics_engine_micro_delay_count,
					   mach_graphics_engine_loop_timeout_count);

		(void) fprintf(stderr,
					   MACH_VERBOSE_STARTUP_SCREEN_PARAMETERS_MESSAGE,
					   screen_state_p->generic_state.screen_physical_width,
					   screen_state_p->generic_state.screen_physical_height,
					   ((mach_options_p->video_memory_dimensions) ?
						MACH_VERBOSE_STARTUP_USER_SPECIFIED : 
						MACH_VERBOSE_STARTUP_AUTO_DETECTED),
					   screen_state_p->generic_state.screen_virtual_width,
					   screen_state_p->generic_state.screen_virtual_height,
					   screen_state_p->generic_state.screen_displayed_width,
					   screen_state_p->generic_state.screen_displayed_height,
					   screen_state_p->generic_state.screen_depth);

		(void) fprintf(stderr,
					   MACH_VERBOSE_STARTUP_EPILOGUE);
		
	}
	
	config_p->IdentString = DEFAULT_MACH_IDENT_STRING;
	
	return (initialization_status);
	
}


/*
 *	mach_print_initialization_failure_message
 *
 *	Diagnose the cause for initialization failure from the
 * 	initialization status flag.
 *
 */

function void
mach_print_initialization_failure_message(
    const int status,
	const SIScreenRec *si_screen_p)
{

	int count = 0;

	const struct mach_display_mode_table_entry *entry_p = 
		&(mach_display_mode_table[MACH_DISPLAY_MODE_NULL + 1]);

	if (status & MACH_INITIALIZE_ATI_CHIPSET_DETECTION_FAILED)
	{
		(void) fprintf(stderr,
					   MACH_COULD_NOT_FIND_ATI_CHIPSET_MESSAGE);
	}
	else if ((status & MACH_INITIALIZE_ATI_MACH8_DETECTION_FAILED) ||
		   (status & MACH_INITIALIZE_ATI_MACH8_DETECTION_FAILED))
	{
		(void) fprintf(stderr,
					   MACH_COULD_NOT_DETECT_MACH8_32_MESSAGE);
	}
	
	if (status & MACH_INITIALIZE_PARSE_USER_OPTIONS_FAILED)
	{
		(void) fprintf(stderr,
					   MACH_PARSE_OF_USER_OPTIONS_FAILED_MESSAGE);
	}
	
	if (status & MACH_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND)
	{
		(void) fprintf(stderr,
					   MACH_CANNOT_FIND_REQUESTED_DISPLAY_MODE_MESSAGE,
					   si_screen_p->cfgPtr->disp_w,
					   si_screen_p->cfgPtr->disp_h,
					   si_screen_p->cfgPtr->monitor_info.vfreq,
					   si_screen_p->cfgPtr->monitor_info.model);
				
		/* 
		 * Print supported modes 
		 */

		for(count = MACH_DISPLAY_MODE_NULL + 1,
			entry_p =
			&(mach_display_mode_table[MACH_DISPLAY_MODE_NULL + 1]);
			count < MACH_DISPLAY_MODE_COUNT; 
			count ++, entry_p++)
		{
			(void) fprintf(stderr, "%5dx%-5d\t%-15.15s\t\"%s\"\n",
						   entry_p->display_width,
						   entry_p->display_height,
						   entry_p->monitor_name_p,
						   mach_display_mode_descriptions[count]);
			
						   
		}
	}
	
	if (status & MACH_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED)
	{
		(void) fprintf(stderr, MACH_UNRECOGNIZED_CLOCK_CHIP_MESSAGE);
	}
	
	if (status & MACH_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND)
	{
		(void) fprintf(stderr,
					   MACH_CANNOT_FIND_REQUESTED_CLOCK_FREQUENCY_MESSAGE,
					   ((struct mach_screen_state *) 
						generic_current_screen_state_p)->
					   clock_chip_p->clock_chip_name_p);
	}
	
	if (status & MACH_INITIALIZE_BOARD_IS_INCAPABLE_OF_HI_RES_MODE)
	{
		(void) fprintf(stderr,
					   MACH_CANNOT_INITIALIZE_BOARD_TO_MODE_MESSAGE,
					   si_screen_p->cfgPtr->disp_w,
					   si_screen_p->cfgPtr->disp_h,
					   si_screen_p->cfgPtr->depth);
	}

	if (status & MACH_INITIALIZE_MODE_IS_NOT_SUPPORTED_BY_DAC)
	{
		(void) fprintf(stderr,
					   MACH_CANNOT_INITIALIZE_DAC_MESSAGE);
	}
	
	
	return;
}
