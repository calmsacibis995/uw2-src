/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3.h	1.6"
#if (! defined(__S3_INCLUDED__))

#define __S3_INCLUDED__



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

#include "s3_regs.h"
#include "s3_options.h"
#include "s3_state.h"
#ifdef DELETE
#include "s3_arc.h"
#include "s3_asm.h"
#include "s3_bitblt.h"
#include "s3_cursor.h"
#include "s3_fill.h"
#include "s3_font.h"
#include "s3_gs.h"
#include "s3_line.h"
#include "s3_points.h"
#include "s3_sline.h"
#include "s3_spans.h"
#include "s3_cmap.h"
#endif

/***
 ***	Constants.
 ***/

/*
 * Initialization flags.
 */
#define S3_INITIALIZE_S3_CHIPSET_DETECTION_FAILED		(1 << 0)
#define S3_INITIALIZE_CHIPSET_STEP_DETECTION_FAILED 	(1 << 1)
#define S3_INITIALIZE_BUS_KIND_DETECTION_FAILED 		(1 << 2)
#define S3_INITIALIZE_DETECT_MEMORY_SIZE_FAILED 		(1 << 3)
#define S3_INITIALIZE_PARSE_USER_OPTIONS_FAILED 		(1 << 4)
#define S3_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND 	(1 << 5)
#define S3_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND	 		(1 << 6)
#define S3_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED 		(1 << 7)
#define S3_INITIALIZE_UNSUPPORTED_DEPTH					(1 << 8)
#define S3_INITIALIZE_UNSUPPORTED_RESOLUTION			(1 << 9)
#define S3_INITIALIZE_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION (1 << 10)
#define S3_UNABLE_TO_SWITCH_CONSOLE_DRIVER_MODE			(1 << 11)
#define S3_INITIALIZE_BOGUS_BUS_WIDTH			 		(1 << 12)
#define S3_INITIALIZE_UNABLE_TO_MAP_DISPLAY_MEMORY		(1 << 13)
#define S3_INITIALIZE_UNSUPPORTED_MEMORY_SIZE				(1 << 14)
#define S3_INITIALIZE_UNSUPPORTED_VIDEO_BOARD			(1 << 15)
#define S3_INITIALIZE_MODE_NOT_SUPPORTED_BY_DAC			(1 << 16)
#define S3_INITIALIZE_PANNING_TO_VIRTUAL_DIMENSIONS_INIT_FAILURE	(1 << 17)
#define S3_INITIALIZE_DAC_NOT_RECOGNIZED 		(1 << 18)

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/
enum s3_display_mode 
{
#define DEFINE_MODE(MODENAME, MODE_FLAGS, INTERLACE_RETRACE_START,\
				WIDTH, HEIGHT, DEPTH,\
				REFRESH_RATE, CLOCK, MONITOR_NAME, \
				H_TOTAL, H_DISP_END, H_BLANK_START, H_BLANK_END,\
				H_SYNC_START, H_SYNC_END,\
				V_TOTAL, V_RETRACE_START, V_RETRACE_END, V_DISPLAY_END,\
				V_BLANK_START, V_BLANK_END,\
				PRESET_ROW_SCAN, MAX_SCAN_LINE, LOGICAL_SCREEN_OFFSET,\
				CRTC_MODE_CONTROL, LINE_COMPARE, NUM_SCREEN_PAGES, \
				PHYSICAL_WIDTH_IN_BYTES, PHYSICAL_HEIGHT_IN_BYTES,\
				INSTALLED_MEMORY_IN_BYTES) \
		MODENAME
#include "s3_modes.def"
#undef DEFINE_MODE
};

struct s3_display_mode_table_entry
{
	enum s3_display_mode display_mode;
	
	int mode_flags;
	int	interlace_retrace_start;
	int display_width;
	int display_height;
	int	display_depth;
	int display_refresh_rate;
	int clock_frequency;
	char *monitor_name_p;
	int h_total;
	int h_disp_end;
	int	h_blank_start;
	int	h_blank_end;
	int h_sync_start;
	int h_sync_end;
	int v_total;
	int v_retrace_start;
	int v_retrace_end;
	int	v_display_end;
	int	v_blank_start;
	int	v_blank_end;
	int	preset_row_scan;
	int	max_scan_line;
	int	logical_screen_offset;
	int	crtc_mode_control;
	int	line_compare;
	int	num_screen_pages;
	int	physical_width;
	int	physical_height;
	int installed_memory;
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
extern boolean s3_debug ;
#endif


/*
 *	Current module state.
 */

extern int 
s3_clock_init_func_standard(struct generic_screen_state *state_p)
;

extern int 
s3_clock_uninit_func_standard(struct generic_screen_state *state_p)
;

extern SIBool
s3_screen_control(SIint32 screen_number, SIint32 control_request)
;

extern SIBool
s3_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
								SIScreenRec *si_screen_p, 
								struct s3_screen_state *screen_state_p)
;

extern void
s3_print_initialization_failure_message(
    const int status,
	const SIScreenRec *si_screen_p)
;


#endif
