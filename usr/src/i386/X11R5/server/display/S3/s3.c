/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3.c	1.18"

/***
 ***	NAME
 ***
 ***		s3.c : flagship source for the S3 display library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s3.h"
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles the interface between the board level
 ***	S3 library/SI  and the chipset level functionality.
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
/* Based on the number 9 Inc code */
/* Copyright (c) 1992, Number Nine Computer Corp.  All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Number Nine Computer Corp not be used 
 * in advertising or publicity pertaining to distribution of the software 
 * without specific, written prior permission.  Number Nine Computer Corp 
 * makes no representations about the suitability of this software for any 
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * NUMBER NINE COMPUTER CORP DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, 
 * IN NO EVENT SHALL NUMBER NINE COMPUTER CORP BE LIABLE FOR ANY SPECIAL, 
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING 
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

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
export boolean s3_debug = FALSE;
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
#undef PRIVATE
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "g_omm.h"

#include "s3_arc.h"
#include "s3_asm.h"
#include "s3_bitblt.h"
#include "s3_cursor.h"
#include "s3_fill.h"
#include "s3_font.h"
#include "s3_gs.h"
#include "s3_line.h"
#include "s3_options.h"
#include "s3_points.h"
#include "s3_regs.h"
#include "s3_sline.h"
#include "s3_spans.h"
#include "s3_state.h"
#include "s3_cmap.h"
/***
 ***	Constants.
 ***/
/*
 * These would be the values for the mode_flags field.
 */
#define S3_MODE_FLAG_801_CHIPSET_SUPPORTED		(1 << 0U)
#define S3_MODE_FLAG_928_CHIPSET_SUPPORTED		(1 << 1U)
#define DEFAULT_CRTC_PARAMETERS_STRING_LENGTH	256
#define DEFAULT_INTERLACED_FLAG_SIZE			50

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/
struct s3_supported_cards_table_entry
{
	char *vendor_name;
	char *model_name;
	enum s3_clock_chip_kind	clock_kind;
	enum s3_dac_kind	dac_kind;
};

/***
 *** Externs.
 ***/
extern void s3__initialize__(SIScreenRec *si_screen_p,
							   struct s3_options_structure *options_p);
extern void s3__vt_switch_out__(void);
extern void s3__vt_switch_in__(void);

/***
 ***	Variables.
 ***/
STATIC boolean is_memory_size_mismatch = FALSE;

/* 
 * List of supported cards.
 */
#define NUMBER_OF_SUPPORTED_VIDEO_CARDS	13
#define DEFINE_SUPPORTED_S3_VIDEO_CARDS()\
	DEFINE_CARD("ELSA","Winner1000",S3_CLOCK_CHIP_ICD_2061A,S3_DAC_SC15025),\
	DEFINE_CARD("Number 9","9GXE",S3_CLOCK_CHIP_ICD_2061A,S3_DAC_BT485),\
	DEFINE_CARD("Metheus","Premier928",S3_CLOCK_CHIP_CHRONTEL_CH9294_VERSION_G,S3_DAC_ATT20C490),\
	DEFINE_CARD("Metheus","Premier928_4M",S3_CLOCK_CHIP_CHRONTEL_CH9294_VERSION_G,S3_DAC_BT485),\
	DEFINE_CARD("Actix","GraphicsEngineUltra",S3_CLOCK_CHIP_CHRONTEL_CH9204_VERSION_C,S3_DAC_W82C490),\
	DEFINE_CARD("Diamond","STEALTHPRO",S3_CLOCK_CHIP_ICD_2061A,S3_DAC_SS2410),\
	DEFINE_CARD("STB","PowergraphX24",S3_CLOCK_CHIP_ICD_2061A,S3_DAC_ATT20C490),\
	DEFINE_CARD("Metheus","PremierS3",S3_CLOCK_CHIP_CHRONTEL_CH9204_VERSION_C,S3_DAC_ATT20C491),\
	DEFINE_CARD("Nth Graphics","NthS3Advantage",S3_CLOCK_CHIP_AV9194_07,S3_DAC_ATT20C491),\
	DEFINE_CARD("Orchid","Fahrenheit1280Plus",S3_CLOCK_CHIP_CHRONTEL_CH9294_VERSION_G,S3_DAC_ATT20C490),\
	DEFINE_CARD("Actix","GraphicsEngine32",S3_CLOCK_CHIP_CHRONTEL_CH9204_VERSION_C,S3_DAC_ATT20C491),\
	DEFINE_CARD("2Max","TrueSpeed",S3_CLOCK_CHIP_AV9194_07,S3_DAC_ATT20C491),\
	DEFINE_CARD("UNSUPPORTED","unsupported model ",0,0)

STATIC const struct s3_supported_cards_table_entry
s3_supported_video_cards_table[] = 
{
#define DEFINE_CARD(VENDOR_NAME,MODEL_NAME,CLOCK_CHIP,DAC_CHIP)\
	{VENDOR_NAME, MODEL_NAME, CLOCK_CHIP, DAC_CHIP}
	DEFINE_SUPPORTED_S3_VIDEO_CARDS()
#undef DEFINE_CARD
};

/**
 **
 ** Programming the CRTC registers.
 ** 
 ** Tables of register values and associated clocks.
 **
 **/

STATIC const struct s3_display_mode_table_entry
s3_display_mode_table[] = 
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
				{ \
					MODENAME, MODE_FLAGS, INTERLACE_RETRACE_START, WIDTH,\
					HEIGHT, DEPTH, REFRESH_RATE, CLOCK, MONITOR_NAME,\
					H_TOTAL, H_DISP_END, H_BLANK_START, H_BLANK_END,\
					H_SYNC_START, H_SYNC_END,\
					V_TOTAL, V_RETRACE_START, V_RETRACE_END, V_DISPLAY_END,\
					V_BLANK_START, V_BLANK_END,\
					PRESET_ROW_SCAN, MAX_SCAN_LINE, LOGICAL_SCREEN_OFFSET,\
					CRTC_MODE_CONTROL, LINE_COMPARE, NUM_SCREEN_PAGES, \
					PHYSICAL_WIDTH_IN_BYTES, PHYSICAL_HEIGHT_IN_BYTES,\
					INSTALLED_MEMORY_IN_BYTES\
				}
#include "s3_modes.def"					
#undef DEFINE_MODE
};

STATIC const char *const s3_display_mode_descriptions[] =
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
				PHYSICAL_WIDTH_IN_BYTES, PHYSICAL_HEIGHT_IN_BYTES, \
				INSTALLED_MEMORY_IN_BYTES) \
		#MODENAME

#include "s3_modes.def"
#undef DEFINE_MODE
};

/***
 *** 	Functions.
 ***/

/*
 * The clock init function for the standard types of clock chips
 * which provide a way of programming them by choosing an index
 * from the extended mode control register. (CR42)
 */
function int 
s3_clock_init_func_standard(struct generic_screen_state *state_p)
{
	struct s3_screen_state	*screen_state_p = 
						(struct s3_screen_state *)state_p;
	/*
	 * Program to select the external clock.
	 */
	outb(VGA_GENERAL_REGISTER_MISC_WR,
		screen_state_p->register_state.standard_vga_registers.
		standard_vga_general_registers.misc_out &  0xEF);

	return(TRUE);
}
/*
 * The clock uninit function for the clock chips that allow programming
 * through choosing an index from the extended mode control register.
 * (CR42)
 */
function int 
s3_clock_uninit_func_standard(struct generic_screen_state *state_p)
{
	struct s3_screen_state	*screen_state_p = 
						(struct s3_screen_state *)state_p;
	outb(VGA_GENERAL_REGISTER_MISC_WR,
		screen_state_p->register_state.saved_standard_vga_registers.
		standard_vga_general_registers.misc_out &  0xEF);

	return(TRUE);
}
/*
 * The init and uninit functions for ICD 2061A chip.
 * The code has been imported from the XFree86 source code 
 * -* no questions asked *-
 */
#define CRYSTAL_FREQUENCY       (14318180L * 2)
#define MIN_VCO_FREQUENCY       50000000L
#define MAX_POST_SCALE          285000000L

#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))

#define MAX_NUMERATOR           130
#define MAX_DENOMINATOR         MIN(129 , CRYSTAL_FREQUENCY / 400000)
#define MIN_DENOMINATOR         MAX(3, CRYSTAL_FREQUENCY / 2000000)

STATIC void
icd2061ASetClock(clock_value)
register long clock_value;
{
	register long	index;
	register char	iotemp;
	int 			select;

	S3_CURRENT_SCREEN_STATE_DECLARE();

	select = (clock_value >> 22) & 3;

	/* 
	 * Set clock input to 11 binary 
	 */
	iotemp = inb(VGA_GENERAL_REGISTER_MISC_RD);
	outb(VGA_GENERAL_REGISTER_MISC_WR, iotemp | 0x0C);

#ifdef DELETE /* seems to cause problems on the diamond board.*/
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,0);
#endif

	S3_READ_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_MODE_CTL_INDEX,iotemp);
	iotemp &= 0xF0;

#define CLOCK(x) outb(screen_state_p->vga_crtc_data, iotemp | (x))
#define C_DATA  2
#define C_CLK   1
#define C_BOTH  3
#define C_NONE  0

	/* 
	 * Program the IC Designs ICD2061A frequency generator 
	 */
	CLOCK(C_NONE);

	/* 
	 * Unlock sequence 
	 */
	CLOCK(C_DATA);
	CLOCK(C_DATA);
	for (index = 0; index < 6; index++)
	{
	  CLOCK(C_BOTH);	/* clock high */
	  CLOCK(C_BOTH);	/* clock high */
	  CLOCK(C_DATA);	/* clock low */
	  CLOCK(C_DATA);	/* clock low */
	}
	CLOCK(C_NONE);	/* clock low, data low */
	CLOCK(C_NONE);	/* clock low, data low */
	CLOCK(C_CLK);	/* clock high */
	CLOCK(C_CLK);	/* clock high */
	CLOCK(C_NONE);	/* clock low, data low */
	CLOCK(C_NONE);	/* clock low, data low */
	CLOCK(C_CLK);	/* clock high */
	CLOCK(C_CLK);	/* clock high */

	/* 
 	 * Program the 24 bit value into REG0 
 	 */
	for (index = 0; index < 24; index++)
	{
		/* 
		   * Clock in the next bit 
		   */
		clock_value >>= 1;
		if (clock_value & 1)
		{
			CLOCK(C_CLK);		/* clock high */
			CLOCK(C_CLK);		/* clock high */
			CLOCK(C_NONE);	/* clock low */
			CLOCK(C_NONE);	/* clock low */
			CLOCK(C_DATA);	/* data high */
			CLOCK(C_DATA);	/* data high */
			CLOCK(C_BOTH);
			CLOCK(C_BOTH);
		 }
		else
		{
			CLOCK(C_BOTH);
			CLOCK(C_BOTH);
			CLOCK(C_DATA);
			CLOCK(C_DATA);
			CLOCK(C_NONE);
			CLOCK(C_NONE);
			CLOCK(C_CLK);
			CLOCK(C_CLK);
		}
	}

	CLOCK(C_BOTH);
	CLOCK(C_BOTH);
	CLOCK(C_DATA);
	CLOCK(C_DATA);
	CLOCK(C_BOTH);
	CLOCK(C_BOTH);

	/* 
	 * Select the CLOCK in the frequency synthesizer 
	 */
	CLOCK(C_NONE | select);
	CLOCK(C_NONE | select);
#undef CLOCK
}
/* 
 * Number theoretic function - GCD (Greatest Common Divisor) 
 */
STATIC long
icd2061AGCD(a, b)
long a, b;
{
	long c = a % b;
	while (c)
	  a = b, b = c, c = a % b;
	return b;
}
STATIC long
icd2061ACalcClock(frequency, select)
register long   frequency;  /* in Hz */
int select;
{
	register long	index;
	long			temp;
	long			min_m, min_n, min_diff;
	long			diff;
	long			clock_p, clock_n, clock_m;

	/* 
	 * Index register frequency ranges for ICD2061A chip 
	 */
	static long  vclk_range[16] = {
		0, /* should be MIN_VCO_FREQUENCY, but that causes problems. */
		51000000,
		53200000,
		58500000,
		60700000,
		64400000,
		66800000,
		73500000,
		75600000,
		80900000,
		83200000,
		91500000,
		100000000,
		120000000,
		MAX_POST_SCALE,
		0,
	};
	
	min_diff = 0xFFFFFFF;
	min_n = 1;
	min_m = 1;
	
	/* 
	*Calculate 18 bit clock value 
	*/
	clock_p = 0;
	if (frequency < MIN_VCO_FREQUENCY)
	clock_p = 1;
	if (frequency < MIN_VCO_FREQUENCY / 2)
	clock_p = 2;
	if (frequency < MIN_VCO_FREQUENCY / 4)
	clock_p = 3;
	frequency <<= clock_p;

	for (clock_n = 4; clock_n <= MAX_NUMERATOR; clock_n++)
	{
		index = CRYSTAL_FREQUENCY / (frequency / clock_n);

		if (index > MAX_DENOMINATOR)
			index = MAX_DENOMINATOR;

		if (index < MIN_DENOMINATOR)
			index = MIN_DENOMINATOR;

		for (clock_m = index - 3; clock_m < index + 4; clock_m++)
			if (clock_m >= MIN_DENOMINATOR && clock_m <= MAX_DENOMINATOR)
			{
				diff = (CRYSTAL_FREQUENCY / clock_m) * clock_n - frequency;

				if (diff < 0)
					diff = -diff;

				if (min_m * icd2061AGCD(clock_m, clock_n) / 
					icd2061AGCD(min_m, min_n) == clock_m &&
					min_n * icd2061AGCD(clock_m, clock_n) / 
					icd2061AGCD(min_m, min_n) == clock_n)
						if (diff > min_diff)
							diff = min_diff;

				if (diff <= min_diff)
				{
					min_diff = diff;
					min_m = clock_m;
					min_n = clock_n;
				}
			}
	}
	clock_m = min_m;
	clock_n = min_n;
	
	/* 
	 * Calculate the index 
	 */
	temp = (((CRYSTAL_FREQUENCY / 2) * clock_n) / clock_m) << 1;

	for ( index = 0; vclk_range[index + 1] < temp && index < 15; index++)
		;

	/* 
	 * Pack the clock value for the frequency snthesizer 
	 */
	temp = (((long)clock_n - 3) << 11) + ((clock_m - 2) << 1)
	+ (clock_p << 8) + (index << 18) + ((long)select << 22);
	
	return temp;
}
STATIC int
s3_clock_init_func_icd_2061(struct generic_screen_state *state_p)
{
	register long 	clock_value; 	
	register long	frequency;

	struct s3_screen_state	*screen_state_p = 
						(struct s3_screen_state *)state_p;

	frequency = screen_state_p->clock_frequency * 1000;

#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p,
					"(s3_clock_init_func_icd_2061a){\n"
					"frequency=%ld \n"
					"}\n",
					frequency);
	}
#endif

	clock_value = icd2061ACalcClock(frequency,2);

	/*
	 * Is there any way to figure out if the programming succeeded?
	 */
	icd2061ASetClock(clock_value);
	icd2061ASetClock(clock_value);
	icd2061ASetClock(clock_value);
	

	/*
	 * Select the clock.
	 */
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_MODE_CTL_INDEX,
		((screen_state_p->register_state.s3_system_control_registers.
			mode_control & 0xF0)|0x02));
	
	return(1);
}
STATIC int
s3_clock_uninit_func_icd_2061(struct generic_screen_state *state_p)
{
	struct s3_screen_state	*screen_state_p = 
		(struct s3_screen_state *)state_p;

	/*
	 * Restore back the clock value.
	 */
	outb(VGA_GENERAL_REGISTER_MISC_WR,
		screen_state_p->register_state.saved_standard_vga_registers.
		standard_vga_general_registers.misc_out &  0xEF);
	return(1);
}


/*
 * Init/Uninit routines for the clock built in Ti3025 DAC.
 * This is a programmable PLL clock generator.
 *
 * The following function calculates N, M and P parameters from
 * the clock frequency given taking into consideration the constraints
 * given below.
 *
 *	F_VCO = F_REF * ((M+2) * 8) / (N+2)
 *	where,
 *		F_REF / (N+2) > 1 MHz.	
 *		110 MHz <= F_VCO <= 220 MHz. and
 *		N, M should be minimized.
 *
 *	F_PLL = F_VCO / (2^P)
 *
 *	These parameters are then programmed thru the registers
 *	PLL control register and Pixel clock PLL data register.
 */	


#define TI3025_CRYSTAL_FREQUENCY       (14318180UL * 8UL)
#define TI3025_MIN_VCO_FREQUENCY       110000000UL
#define TI3025_MAX_VCO_FREQUENCY       220000000UL

#define MAX_CLOCK_N          	14 
#define MIN_CLOCK_M				3


STATIC long
ti3025_calc_gcd(a, b)
long a, b;
{
	long c = a % b;
	while (c)
		a = b, b = c, c = a % b;
	return b;
}

							/* in Hz */
STATIC int
ti3025_calc_clock_parameters(register long frequency, 
							unsigned char *n,
							unsigned char *m,
							unsigned char *p)
{

	register long   index;
	long         	min_m, min_n, min_diff;
	long		  	clock_n, clock_m, clock_p;
	long         	diff;

	min_n = 3;
	min_m = 3;

	clock_p = 0;
	if (frequency < TI3025_MIN_VCO_FREQUENCY)
		clock_p = 1;
	if (frequency < TI3025_MIN_VCO_FREQUENCY / 2)
		clock_p = 2;
	if (frequency < TI3025_MIN_VCO_FREQUENCY / 4)
		clock_p = 3;

	frequency <<= clock_p;

 	min_diff =  (frequency * MAX_CLOCK_N) / TI3025_CRYSTAL_FREQUENCY ;

    min_diff = (TI3025_CRYSTAL_FREQUENCY * min_diff) / MAX_CLOCK_N -
                                                            frequency;
	for (clock_n = MAX_CLOCK_N; clock_n >= 3; clock_n--)
	{
		index = (frequency * clock_n) / TI3025_CRYSTAL_FREQUENCY ;

		if (index < MIN_CLOCK_M)
			index =  MIN_CLOCK_M;

		for (clock_m = index - 3; clock_m < index + 4; clock_m++)
		{
			if( clock_m >= MIN_CLOCK_M)
			{
				diff = (TI3025_CRYSTAL_FREQUENCY * clock_m) / clock_n - 
																frequency;
				if (diff < 0)
					diff = -diff;

				if (min_m * ti3025_calc_gcd(clock_m, clock_n) / 
					ti3025_calc_gcd(min_m, min_n) == clock_m &&
					min_n * ti3025_calc_gcd(clock_m, clock_n) / 
					ti3025_calc_gcd(min_m, min_n) == clock_n)
				{
					if (diff > min_diff)
						diff = min_diff;
				}
				
				if (diff <= min_diff)
				{
					min_diff = diff;
					min_m = clock_m;
					min_n = clock_n;
				}
			}
		}
	}
	clock_m = min_m;
	clock_n = min_n;

	*n	= clock_n - 2;
	*m	= clock_m - 2;
	*p	= clock_p ;

	return (1);
}


STATIC int
s3_clock_init_func_ti3025(struct generic_screen_state *state_p)
{
	register long	frequency;
	unsigned char	n, m, p ;

	struct s3_screen_state	*screen_state_p = 
						(struct s3_screen_state *)state_p;

	frequency = screen_state_p->clock_frequency * 1000;

#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p,
					"(s3_clock_init_func_ti3025){\n"
					"frequency=%ld \n"
					"}\n",
					frequency);
	}
#endif


	ti3025_calc_clock_parameters( frequency, &n, &m, &p ); 

	S3_WRITE_TI_DAC_REGISTER( TI3025_PLL_CONTROL, 0 );
	S3_WRITE_TI_DAC_REGISTER( TI3025_PIXEL_CLOCK_PLL_DATA, n );
	S3_WRITE_TI_DAC_REGISTER( TI3025_PIXEL_CLOCK_PLL_DATA, m );
	S3_WRITE_TI_DAC_REGISTER( TI3025_PIXEL_CLOCK_PLL_DATA, p | 
								PCLK_PLL_DATA_P_VALUE_PCLKOUT);
	S3_WRITE_TI_DAC_REGISTER( TI_INPUT_CLOCK_SELECT, TI_ICLK_PCLK_PLL);

	return(1);
}

STATIC int
s3_clock_uninit_func_ti3025(struct generic_screen_state *state_p)
{
	struct s3_screen_state	*screen_state_p = 
		(struct s3_screen_state *)state_p;

	S3_WRITE_TI_DAC_REGISTER( TI_INPUT_CLOCK_SELECT, TI_ICLK_CLK0);
	/*
	 * Restore back the clock value.
	 */
	outb(VGA_GENERAL_REGISTER_MISC_WR,
		screen_state_p->register_state.saved_standard_vga_registers.
		standard_vga_general_registers.misc_out &  0xEF);
	return(1);
}


/**
 **
 ** Clock Chip tables.
 **
 **/
STATIC const struct s3_clock_chip_table_entry
s3_clock_chip_table[] = 
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, INIT_FUNC, UNINIT_FUNC, NUM_FREQ,\
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)\
	{\
		NAME,\
		INIT_FUNC, UNINIT_FUNC,NUM_FREQ,\
		{ f0, f1, f2, f3, f4, f5, f6, f7,\
		  f8, f9, f10, f11, f12, f13, f14, f15 }\
	}
#include "s3_clks.def"
#undef DEFINE_CLOCK_CHIP						
};  
#if (defined(USE_KD_GRAPHICS_MODE))
/*
 * s3_set_virtual_terminal_mode
 *
 * Set the virtual terminal mode to KD_GRAPHICS or KD_TEXT.
 */
STATIC int
s3_set_virtual_terminal_mode(SIint32
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
 * This function checks if panning to virtual screen boundary is 
 * possible for the given combination of display and virtual 
 * dimensions. The catch is to check if both of them have the
 * same screen map for the given depth. In such a case 
 * hardware panning is feasible.
 */
STATIC int
s3_check_if_panning_is_feasibile(SIConfigP config_p, 
	struct s3_screen_state  *screen_state_p)
{
	const struct s3_display_mode_table_entry *entry_p ; 
	int mode_count;
	int	initialization_status = 0;

	screen_state_p->is_panning_to_virtual_screen_feasible  = FALSE;

	if (config_p->virt_w < config_p->disp_w ||
		config_p->virt_h < config_p->disp_h)
	{
		/*
		 * No panning if virtual dimensions are less than physical
		 * dimensions.
		 */
		return(initialization_status);
	}
	if (config_p->virt_w == config_p->disp_w &&
		config_p->virt_h == config_p->disp_h)
	{
		/*
		 * No panning if virtual dimensions are same as the physical
		 * dimensions.
		 */
		return(initialization_status);
	}
	/*
	 * First get a display mode corresponding to the virtual 
	 * dimensions.
	 */
	for(mode_count = S3_DISPLAY_MODE_NULL + 1,
		entry_p =
		&(s3_display_mode_table[S3_DISPLAY_MODE_NULL + 1]);
		mode_count < S3_DISPLAY_MODE_COUNT; 
		++mode_count, ++entry_p)
	{
		if ((config_p->virt_w == entry_p->display_width) &&
			(config_p->virt_h == entry_p->display_height) &&
			(config_p->depth == entry_p->display_depth) &&
			(screen_state_p->video_memory_size == 
					entry_p->installed_memory))
		{
			if ((screen_state_p->display_mode_p->physical_width
				== entry_p->physical_width) &&
				(screen_state_p->display_mode_p->physical_height
				== entry_p->physical_height) &&
				(screen_state_p->display_mode_p->display_depth
				== entry_p->display_depth))
			{
				screen_state_p->is_panning_to_virtual_screen_feasible = TRUE;
				break;
			}
		}
	}
	if (mode_count == S3_DISPLAY_MODE_COUNT)
	{
		initialization_status |=
			S3_INITIALIZE_PANNING_TO_VIRTUAL_DIMENSIONS_INIT_FAILURE;
	}
	return(initialization_status);
}

/*
 * Function:
 *			s3_gen_crtc_parameters_string(char *, SIConfigP)
 * Input:
 *			modeDB string 
 *			Also, screen depth and video memory size are taken from SIConfigP.
 *			
 * Output:
 *			CRTC parameters string in the format of crtc-parameters option
 *
 * Returns:		
 *			Pointer to crtc parameters string on success  and 
 *			NULL on failure.
 *
 */

STATIC char *
s3_gen_crtc_parameters_string(char *modeDB_p, SIConfigP config_p)
{
	float clk_freq;
	int HTotal, HDisplay, HSyncStart, HSyncEnd, 
		VTotal, VDisplay, VSyncStart, VSyncEnd;
	int x, y;
	char *is_interlaced;
	char *tmp_is_interlaced;
	char *crtc_parameters;
	unsigned char CRTC[27];
	unsigned int 
		val_h_total,
		val_h_d_end,
		val_s_h_blank,
		val_e_h_blank,
		val_s_h_sy_p,
		val_e_h_sy_p,
		val_v_total,
		val_vrs,
		val_vre,
		val_vde,
		val_svb,
		val_evb,
		val_p_r_scan,
		val_max_s_ln,
		val_screen_offset,
		val_crt_md,
		val_lcm, 
		val_interlace_retrace_start;
	int
		screen_depth,
	    installed_memory,
		memory_required,
	    num_scr_pgs,
	    physical_width,
	    physical_height,
	    horz_scan_freq,
	    refresh_rate;

/*
 * Removed for now. MODE NAME within quotes in the beginning of the 
 * modedb-string option is not allowed. Option of type string
 * having double quotes within is not suupported by genoptions.
 *
 */
#ifdef DELETE
	/*
	 * Skip leading white space
	 */
    while (*modeDB_p && isspace(*modeDB_p))
    {
                modeDB_p ++;
    }

	/*
	 * Skip the Name field within double quotes, if present.
	 */
	if(*modeDB_p=='"')
	{
		modeDB_p++;
		/*
		 * Look for matching double-quote.
		 */
		while(*modeDB_p++ != '"')
		{
			;	
		}
	}
#endif

	is_interlaced = (char *)allocate_and_clear_memory(sizeof(char) * 
							DEFAULT_INTERLACED_FLAG_SIZE);

	if( sscanf(modeDB_p,
		"%f %d %d %d %d %d %d %d %d %s", 
		&clk_freq,
	 	&HDisplay,&HSyncStart,&HSyncEnd, &HTotal,
	 	&VDisplay,&VSyncStart,&VSyncEnd, &VTotal, is_interlaced) != 10)
	{
			if( sscanf(modeDB_p,
				"%f %d %d %d %d %d %d %d %d", 
				&clk_freq,
				&HDisplay,&HSyncStart,&HSyncEnd, &HTotal,
				&VDisplay,&VSyncStart,&VSyncEnd, &VTotal) != 9)
				{
						(void) fprintf(stderr,
						S3_INITIALIZE_IGNORING_MALFORMED_MODEDB_STRING_OPTION_MESSAGE,
						modeDB_p);
						return NULL;
				}
	}

	x = HDisplay;
	y = VDisplay;

#if (defined(__DEBUG__))
    if (s3_debug)
    {
        (void) fprintf(debug_stream_p,
            "(s3_gen_crtc_parameters_string) \n\
			clock_frequency = %f \n\
			HDisplay = %d HSyncStart = %d HSyncEnd = %d HTotal = %d\n\
			VDisplay = %d VSyncStart = %d VSyncEnd = %d VTotal = %d\n",
			clk_freq,
			HDisplay, HSyncStart, HSyncEnd, HTotal,
			VDisplay, VSyncStart, VSyncEnd, VTotal);
	}
#endif

	/*
	 * Supported Resolutions, Screen depths and Video memory sizes
	 *	are hard coded.
	 *
	 */
	if( !(((x==640)&&(y==480)) || ((x==800)&&(y==600)) || 
			((x==1024)&&(y==768)) || ((x==1280)&&(y==1024))) )
	{
			fprintf(stderr, 
			S3_INITIALIZE_MODEDB_UNSUPPORTED_RESOLUTION_MESSAGE,
			x, y);
			(void) fprintf(stderr,
						S3_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE);
			return NULL;
	}

	if( !((x==config_p->disp_w)&&(y==config_p->disp_h)) )
	{
			fprintf(stderr, 
			S3_INITIALIZE_MODEDB_RESOLUTION_MISMATCH_MESSAGE,
			config_p->disp_w, config_p->disp_h, x, y);
			(void) fprintf(stderr,
						S3_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE);
			return NULL;
	}

	screen_depth = config_p->depth;

	if( screen_depth != 4 && screen_depth != 8)
	{
			fprintf(stderr, 
			S3_INITIALIZE_MODEDB_UNSUPPORTED_DEPTH_MESSAGE,
			screen_depth);
			(void) fprintf(stderr,
						S3_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE);
			return NULL;
	}

	installed_memory = config_p->videoRam;
	
	memory_required  = x*y*screen_depth;

	if( (installed_memory << 13) < memory_required )
	{
			fprintf(stderr, 
			S3_INITIALIZE_MODEDB_INSUFFICIENT_MEMORY_SIZE_MESSAGE,
			installed_memory, x, y, screen_depth);
			(void) fprintf(stderr,
						S3_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE);
			return NULL;
	}

	switch(installed_memory)
	{
		case  512:
		case 1024:
		case 2048:
		case 3072:
		case 4096:

			break;
		default  :
	
			fprintf(stderr, 
			S3_INITIALIZE_MODEDB_UNSUPPORTED_VIDEO_MEMORY_SIZE_MESSAGE,
			installed_memory);
			(void) fprintf(stderr,
						S3_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE);
			return NULL;
			/*NOTREACHED*/
	}

	tmp_is_interlaced = is_interlaced ;
	/*
	 * Convert the string to uppercase.
	 */
	while( *tmp_is_interlaced)
	{
		*tmp_is_interlaced = toupper(*tmp_is_interlaced);
		tmp_is_interlaced++;
	}

	if( strcmp(is_interlaced,"INTERLACE") == 0 )
	{
		switch(HDisplay)
		{
		case 800 :
			val_interlace_retrace_start = 0x32;
			break;
		case 1024:
			val_interlace_retrace_start = 0x4F;
			break;
		case 1280:
			val_interlace_retrace_start = 0x62;
			break;
		default :
			fprintf(stderr, 
			S3_INITIALIZE_MODEDB_UNSUPPORTED_INTERLACED_MODE_MESSAGE,
			x, y);
			(void) fprintf(stderr,
						S3_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE);
			return NULL;
			/*NOTREACHED*/
		}
		VDisplay/=2;
		VSyncStart/=2;
		VSyncEnd/=2;
		VTotal/=2;
	}
	else 
		val_interlace_retrace_start = 0x0;

#if (defined(__DEBUG__))
    if (s3_debug)
    {
        (void) fprintf(debug_stream_p,
            "(s3_gen_crtc_parameters_string) val_interlace_retrace_start = %x\n", 
			val_interlace_retrace_start);
	}
#endif

	/*
	 * CRTC Controller
	 */
	/*
	 * Except CRTC[25] and CRTC[26],
	 * the array CRTC holds the CRTC register values of index specified
	 * by the corresponding subscript. 
	 * 		CRTC[25] is the value of extended horizontal overflow reg(CR-5D).
	 *		CRTC[26] is the value of extended vertical overflow reg(CR-5E).
	 */
	CRTC[0]  = (HTotal >> 3) - 5;
	CRTC[1]  = (HDisplay >> 3) - 1;
	CRTC[2]  = (HSyncStart >> 3) -1;
	CRTC[3]  = ((HSyncEnd >> 3) & 0x1F) | 0x80;
	CRTC[4]  = (HSyncStart >> 3);
	CRTC[5]  = (((HSyncEnd >> 3) & 0x20 ) << 2 )
	    | (((HSyncEnd >> 3)) & 0x1F);
	CRTC[6]  = (VTotal - 2) & 0xFF;
	CRTC[7]  = (((VTotal -2) & 0x100) >> 8 )
	    | (((VDisplay -1) & 0x100) >> 7 )
	    | ((VSyncStart & 0x100) >> 6 )
	    | (((VSyncStart) & 0x100) >> 5 )
	    | 0x10
	    | (((VTotal -2) & 0x200)   >> 4 )
	    | (((VDisplay -1) & 0x200) >> 3 )
	    | ((VSyncStart & 0x200) >> 2 );
	CRTC[8]  = 0x00;
	CRTC[9]  = ((VSyncStart & 0x200) >>4) | 0x40;
	CRTC[10] = 0x00;
	CRTC[11] = 0x00;
	CRTC[12] = 0x00;
	CRTC[13] = 0x00;
	CRTC[14] = 0x00;
	CRTC[15] = 0x00;
	CRTC[16] = VSyncStart & 0xFF;
	CRTC[17] = (VSyncEnd & 0x0F) | 0x20;
	CRTC[18] = (VDisplay -1) & 0xFF;
	CRTC[19] = 0x80; /*vga256InfoRec.virtualX >> 4;   */

	switch(installed_memory)
	{
	case  512:
		if( HDisplay==800 && screen_depth==8 )
		{
			ASSERT(y==600);
			physical_width  =   800;
			physical_height =   655;
		}
		else if( HDisplay==640 && screen_depth==8 )
		{
			ASSERT(y==480);
			physical_width  =   1024;
			physical_height =   512;
		}
		else
		{
			ASSERT(screen_depth==4);

			physical_width  =   512;
			physical_height =   1024;
		}
		break;
	case 1024:
		physical_width = physical_height = 1024;
		break;
	case 2048:
		if( HDisplay==1280 && screen_depth==8 )
		{
			physical_width  =   1280;
			physical_height =   1638;
			CRTC[19]		= 	0xa0;
		}
		else
		{
			physical_width  =   1024;
			physical_height =   2048;
		}
		break;
	case 3072:
		if( HDisplay==1280 && screen_depth==8 )
		{
			physical_width  =   1280;
			physical_height =   2457;
			CRTC[19]		= 	0xa0;
		}
		else
		{
			physical_width  =   1024;
			physical_height =   3072;
		}
		break;
	case 4096:
		if( HDisplay==1280 && screen_depth==8 )
		{
			physical_width  =   1280;
			physical_height =   3276;
			CRTC[19]		= 	0xa0;
		}
		else
		{
			physical_width  =   1024;
			physical_height =   4096;
		}
		break;
	}

	CRTC[20] = 0x00;
	CRTC[21] = VSyncStart & 0xFF;
	CRTC[22] = (VSyncStart +1) & 0xFF;

	CRTC[23] = 0xC3;

	if( val_interlace_retrace_start != 0)
	{
		if(HDisplay==1024||HDisplay==1280)
		{
			CRTC[23] = 0xE3;
		}
	}


	CRTC[24] = 0xFF;
	CRTC[25]  = 
	    ((((HTotal 	  >> 3) - 5) & 0x100) >> 8) |
	    ((((HDisplay   >> 3) - 1) & 0x100) >> 7) |
	    ((((HSyncStart >> 3) - 1) & 0x100) >> 6) |
	    ((( HSyncStart >> 3) 	 & 0x100) >> 4);

	CRTC[26]  = (((VTotal -2) & 0x400) >> 10 )
	    | (((VDisplay -1) & 0x400) >> 9 )
	    | ((VSyncStart & 0x400) >> 8 )
	    | ((VSyncStart & 0x400) >> 6 )
	    | 0x40;

#if (defined(__DEBUG__))
    if (s3_debug)
    {
		int i;
		for(i=0;i<25;i++)
		{
        	(void) fprintf(debug_stream_p,
							" CR-%x %x\n", i, CRTC[i]);
		}
    	(void) fprintf(debug_stream_p,
            			" CR-5D %x\n CR-5E %x\n", 
						CRTC[25], CRTC[26]);
	}
#endif

	if(screen_depth==4)
		num_scr_pgs = 1;
	else if(screen_depth==8)
		num_scr_pgs = 0;

	/*
	 * Refreshrate = 
	 * 		clock frequency/(horizontal framelength * vertical framelength)
	 * Horizontal scan frequency =
	 * 		clock frequency/horizontal framelength	 
	 */		

	refresh_rate =  (clk_freq*1000000)/(HTotal * VTotal) + 0.5 ;

	horz_scan_freq =  (clk_freq*1000)/(float)HTotal + 0.5 ;

#if (defined(__DEBUG__))
    if (s3_debug)
    {
        (void) fprintf(debug_stream_p,
            "(s3_gen_crtc_parameters_string) \n\
			refresh_rate = %d\n\
			horz_scan_freq = %d\nresolution = %dx%d\n",
 			refresh_rate, horz_scan_freq, x, y);
	}
#endif

	/*
	 * Computation of ACTUAL crtc parameter values
	 * from the register values by taking into consideration
	 * all the overflow bits.
	 */

	val_h_total	= CRTC[0]  + ((CRTC[25] & 0x01) ? (1<< 8) :0);
	val_h_d_end	= CRTC[1]  + ((CRTC[25] & 0x02) ? (1<< 8) :0);
	val_s_h_blank	= CRTC[2]+ ((CRTC[25] & 0x04) ? (1<< 8) :0);
	val_e_h_blank	=(CRTC[3] & 0x1f) + ((CRTC[5] & 0x80) ? (1<<5) :0);
	val_s_h_sy_p 	= CRTC[4] + ((CRTC[25] & 0x10) ? (1<< 8) :0);
	val_e_h_sy_p 	= (CRTC[5]  & 0x1f);
	val_v_total	= CRTC[6]  + ((CRTC[7]  & 0x01) ? (1<< 8) :0)
	    + ((CRTC[7]  & 0x20) ? (1<< 9) :0)
	    + ((CRTC[26] & 0x01) ? (1<<10) :0);
	val_vrs    	= CRTC[16]      + ((CRTC[7]  & 0x04) ? (1<< 8) :0)
	    + ((CRTC[7]  & 0x80) ? (1<< 9) :0)
	    + ((CRTC[26] & 0x10) ? (1<<10) :0);
	val_vre     	= (CRTC[17] 	& 0x0f);
	val_vde    	= CRTC[18]      + ((CRTC[7]  & 0x02) ? (1<< 8) :0)
	    + ((CRTC[7]  & 0x40) ? (1<< 9) :0)
	    + ((CRTC[26] & 0x02) ? (1<<10) :0);

	val_svb    	= CRTC[21]      + ((CRTC[7]  & 0x08) ? (1<< 8) :0)
	    + ((CRTC[9]  & 0x20) ? (1<< 9) :0)
	    + ((CRTC[26] & 0x04) ? (1<<10) :0);

	val_evb		= CRTC[22];
	val_p_r_scan	= (CRTC[8]  & 0x1f);
	val_max_s_ln	= (CRTC[9]  & 0x1f);
	val_screen_offset	= CRTC[19];
	val_crt_md 	= CRTC[23];
	val_lcm    	= CRTC[24]      + ((CRTC[7]  & 0x10) ? (1<< 8) :0)
	    + ((CRTC[9]  & 0x40) ? (1<< 9) :0)
	    + ((CRTC[26] & 0x40) ? (1<<10) :0);


	/*
	 * CRTC parameters in hexadecimal :\
     *      h-total h-disp-end h-blank-start h-blank-end h-sync-start \
     *      h-sync-end v-total v-retrace-start v-retrace-end v-display-end \
     *      v-blank-start v-blank-end preset-row-scan max-scan-line \
     *      logical-screen-offset clock-frequency interlace_retrace_start \
     *      crtc_mode-control line-compare num-screen-pages physical_width\
     *      physical_height
	 */

	/*
	 * CRTC parameter entry has been succesfully created.
	 * Allocate space for crtc_parameters and copy in the new crtc parameter
	 * 		string. 
	 */

	crtc_parameters = (char *) allocate_memory( sizeof(char) *
								DEFAULT_CRTC_PARAMETERS_STRING_LENGTH);
	
	sprintf(crtc_parameters,   "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x "\
							"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x %d 0x%x 0x%x 0x%x "\
							"0x%x %d %d",
		val_h_total,	val_h_d_end,	val_s_h_blank,	val_e_h_blank,
		val_s_h_sy_p,	val_e_h_sy_p,
		val_v_total,	val_vrs,		val_vre,		val_vde,
		val_svb,		val_evb,
		val_p_r_scan,	val_max_s_ln,	val_screen_offset,
		(int)(clk_freq*1000), 			val_interlace_retrace_start,
		val_crt_md,		val_lcm,		num_scr_pgs,
		physical_width, 				physical_height);

	return crtc_parameters;

}


/*
 * s3_get_mode
 *
 * Updates the in memory values of the VGA crtc registers. 
 * Updates the in memory values of the S3 extended VGA registers.
 * Returns an error code if a suitable mode was not found.
 */
STATIC int
s3_get_mode(SIConfigP config_p,
		  struct s3_options_structure *options_p,
		  struct s3_screen_state *screen_state_p)
{
	
	int option_line_crtc_scan_failed = 1;
	int initialization_status = 0;

	char *crtc_parameters=NULL;
	unsigned int interlace_retrace_start;
	unsigned int clock_frequency;
	unsigned int h_total,h_disp_end,h_blank_start,h_blank_end;
	unsigned int h_sync_start,h_sync_end; 
	unsigned int v_total,v_retrace_start,v_retrace_end,v_display_end;
	unsigned int v_blank_start,v_blank_end;
	unsigned int preset_row_scan,max_scan_line,logical_screen_offset;
	unsigned int crtc_mode_control,line_compare;
	unsigned int physical_width, physical_height, num_screen_pages;

	struct vga_crt_controller_register_state *vga_crtc_registers;
	const struct s3_display_mode_table_entry *entry_p = 
		&(s3_display_mode_table[S3_DISPLAY_MODE_NULL + 1]);

	/* 
	 * The way we would hunt for entries for a muxable dac like bt485
	 * for 16 bit ,from the mode table, is very different from 8 bit
	 * modes.
	 */
	boolean	is_mux_dac_and_16_bit = FALSE;
	
	int mode_count = 0;
	int clock_count = 0;
		
	/*
	 * We cannot automagically determine the clock chip name.
	 * This information would be kept in the configuration file.
	 * See if an option has been set. If not then take the board
	 * level library's specification. If neither is there then it
	 * is an error condition. An option will override any board
	 * level settings.
	 */
	switch (options_p->clock_chip_name)
	{
	  case S3_OPTIONS_CLOCK_CHIP_NAME_CHRONTEL_CH9204B : 
		  screen_state_p->clock_chip_p =
		  &(s3_clock_chip_table[S3_CLOCK_CHIP_CHRONTEL_CH9204_VERSION_B]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_CHRONTEL_CH9204C : 
		  screen_state_p->clock_chip_p =
		  &(s3_clock_chip_table[S3_CLOCK_CHIP_CHRONTEL_CH9204_VERSION_C]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_CHRONTEL_CH9294G : 
		  screen_state_p->clock_chip_p =
		  &(s3_clock_chip_table[S3_CLOCK_CHIP_CHRONTEL_CH9294_VERSION_G]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_ICD_2061A : 
		  screen_state_p->clock_chip_p =
			  &(s3_clock_chip_table[S3_CLOCK_CHIP_ICD_2061A]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_ICD_2061 : 
		  screen_state_p->clock_chip_p =
			  &(s3_clock_chip_table[S3_CLOCK_CHIP_ICD_2061]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_APPROXIMATE_VALUES:
		  screen_state_p->clock_chip_p =
			  &(s3_clock_chip_table[S3_CLOCK_CHIP_APPROXIMATE_VALUES]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_AV9194_56:
		  screen_state_p->clock_chip_p =
			  &(s3_clock_chip_table[S3_CLOCK_CHIP_AV9194_56]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_AV9194_07:
		  screen_state_p->clock_chip_p =
			  &(s3_clock_chip_table[S3_CLOCK_CHIP_AV9194_07]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_AV9194_11:
		  screen_state_p->clock_chip_p =
			  &(s3_clock_chip_table[S3_CLOCK_CHIP_AV9194_11]);
		  break;
	  case S3_OPTIONS_CLOCK_CHIP_NAME_TI_3025:
		  screen_state_p->clock_chip_p =
			  &(s3_clock_chip_table[S3_CLOCK_CHIP_TI_3025]);
		  break;
	  default:
		  /*
		   * Check if the vendor level code has intialized the clock 
		   * the clock chip information.
		   */
		  if (screen_state_p->clock_chip_p == NULL)
		  {
			  (void) fprintf(stderr,
					S3_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED_MESSAGE); 
			  initialization_status |=
					S3_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED;
			  return (initialization_status);
		  }
		break;
	}

	/*
	 * We have a different way of figuring out the 16 bit mode entries
	 * for a dac that multiplexes for 16bpp. As of now the dacs that
     * can multiplex that we know of are BT485 and TI3025/3020.
     */
    if ((screen_state_p->dac_kind == S3_DAC_BT485 ||
         screen_state_p->dac_kind == S3_DAC_TI3020 ||
         screen_state_p->dac_kind == S3_DAC_TI3025) &&
         config_p->depth == 16)
	{
		is_mux_dac_and_16_bit = TRUE;
	}
	
#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s3_get_mode)\tclock chip \"%s\".\n",
			s3_clock_chip_kind_dump
		   	[screen_state_p->clock_chip_p->clock_chip_kind]);
	}
#endif

	/*
	 * Check for crtc parameters as library options.
	 * Options override the def files.
	 */

	/*
	 * NOTE:
	 *		modedb-string option has higher priority than crtc_parameters
	 *		option.
	 */


	if (options_p->modedb_string) 
	{
		crtc_parameters = s3_gen_crtc_parameters_string(
							options_p->modedb_string, config_p);
	}

	if(crtc_parameters == NULL)
	{
		crtc_parameters = options_p->crtc_parameters;
	}

	if(crtc_parameters)
	{
		if(sscanf(crtc_parameters, 
		   "%i %i %i %i %i %i %i %i %i %i %i \
			%i %i %i %i %i %i %i %i %i %i %i ",
			&h_total,&h_disp_end,&h_blank_start,&h_blank_end,
			&h_sync_start,&h_sync_end, 
			&v_total,&v_retrace_start,&v_retrace_end,&v_display_end,
			&v_blank_start,&v_blank_end,
			&preset_row_scan,&max_scan_line,&logical_screen_offset,
			&clock_frequency, &interlace_retrace_start, &crtc_mode_control, 
			&line_compare, &num_screen_pages,&physical_width, 
			&physical_height) == 22)
		{
			/*
			 * We need to initialize the screen_state's display_mode_p
			 * field. The hardware panning logic and initialize the pointer. 
			 * Allocate some space, fill in appropriate values
			 * makes use of this.
			 */
			struct s3_display_mode_table_entry *disp_entry_p = 
				allocate_and_clear_memory( 
				sizeof(struct s3_display_mode_table_entry));

			disp_entry_p->display_width = config_p->disp_w;
			disp_entry_p->display_height = config_p->disp_h;
			disp_entry_p->display_depth = config_p->depth;
			disp_entry_p->interlace_retrace_start = interlace_retrace_start;
			disp_entry_p->display_refresh_rate;
			disp_entry_p->h_total = h_total;
			disp_entry_p->h_disp_end = h_disp_end;
			disp_entry_p->h_blank_start = h_blank_start;
			disp_entry_p->h_blank_end = h_blank_end;
			disp_entry_p->h_sync_start = h_sync_start;
			disp_entry_p->h_sync_end = h_sync_end;
			disp_entry_p->v_total = v_total;
			disp_entry_p->v_retrace_start = v_retrace_start;
			disp_entry_p->v_retrace_end = v_retrace_end;
			disp_entry_p->v_display_end = v_display_end;
			disp_entry_p->v_blank_start = v_blank_start;
			disp_entry_p->v_blank_end = v_blank_end;
			disp_entry_p->preset_row_scan = preset_row_scan;
			disp_entry_p->max_scan_line = max_scan_line;
			disp_entry_p->logical_screen_offset = logical_screen_offset;
			disp_entry_p->crtc_mode_control = crtc_mode_control;
			disp_entry_p->line_compare = line_compare;
			disp_entry_p->num_screen_pages = num_screen_pages;
			disp_entry_p->physical_width = physical_width;
			disp_entry_p->physical_height = physical_height;
			disp_entry_p->clock_frequency = clock_frequency;

			screen_state_p->display_mode_p = disp_entry_p;

			option_line_crtc_scan_failed = 0;
		}
		else
		{
			(void) fprintf(stderr,
				S3_INITIALIZE_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION_MESSAGE, 
				options_p->crtc_parameters);
		}
	}
			
	if (option_line_crtc_scan_failed)
	{
		/*
		 * check if the vendor level code has already initialized any
		 * mode tables. If not then check the mode table for any
		 * matching mode data.
		 */
		if (!screen_state_p->display_mode_p)
		{
			/* 
			 * get the matching mode entry if the following criteria is set.
			 * 1. depth is 8/4 bit.
			 * 2. if depth is 8/4 bit dac is not a mux type dac.
			 */
			if (is_mux_dac_and_16_bit == FALSE)
			{
				/*
				 * Look for a mode entry which matches the information specifed.
				 */
				for(mode_count = S3_DISPLAY_MODE_NULL + 1,
					entry_p =
					&(s3_display_mode_table[S3_DISPLAY_MODE_NULL + 1]);
					mode_count < S3_DISPLAY_MODE_COUNT; 
					++mode_count, ++entry_p)
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
						(config_p->depth == entry_p->display_depth) &&
						(screen_state_p->video_memory_size == 
								entry_p->installed_memory) &&
						(refresh_difference < DEFAULT_S3_EPSILON) &&
						((entry_p->monitor_name_p) ? 
						/* NULL will match any name */
						 !strcmp(entry_p->monitor_name_p,
								 config_p->monitor_info.model) : TRUE))
					{
						/*
						 * Found a match.
						 */
						screen_state_p->display_mode_p = entry_p;
						break;
					}
				}

				if (mode_count == S3_DISPLAY_MODE_COUNT)
				{
					initialization_status |=
						S3_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND; 
					return (initialization_status);
				}
			}
			else
			{
				int	memory_required = 1024*1024; /* Minimum for any mode */
				ASSERT(config_p->depth == 16);
				ASSERT(screen_state_p->dac_kind == S3_DAC_BT485);

				/*
				 * We need minimum 2MB for 1024x768 and 3MB for 1280x1024. 
				 * Check that here and return failure in case of problems.
				 */
				if(config_p->virt_w == 1024 && config_p->virt_h== 768)
				{
					memory_required = 2048*1024;
				}
				else if(config_p->virt_w == 1280 && config_p->virt_h== 1024)
				{
					memory_required = 3072*1024;
				}
				if (screen_state_p->video_memory_size < memory_required)
				{
					initialization_status |=
						S3_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND; 
					return (initialization_status);
				}

				/*
				 * Handle dacs that multiplex for 16 bit seperately.
				 * Basically we avoid repeating the mode table entries
				 * for 16 bit and derive the values from the 8 bit 
				 * modes itself. Look for a matching 8 bit mode entry.
				 */
				for(mode_count = S3_DISPLAY_MODE_NULL + 1,
					entry_p =
					&(s3_display_mode_table[S3_DISPLAY_MODE_NULL + 1]);
					mode_count < S3_DISPLAY_MODE_COUNT; 
					++mode_count, ++entry_p)
				{
					double refresh_difference = /* make a positive value */
						(config_p->monitor_info.vfreq >
						 entry_p->display_refresh_rate) ? 
							 (config_p->monitor_info.vfreq -
							  entry_p->display_refresh_rate) :
							 (entry_p->display_refresh_rate - 
							  config_p->monitor_info.vfreq);
					if ((config_p->disp_w == entry_p->display_width) &&
						(config_p->disp_h == entry_p->display_height) &&
						(8 == entry_p->display_depth) &&
						(screen_state_p->video_memory_size == 
								entry_p->installed_memory) &&
						(refresh_difference < DEFAULT_S3_EPSILON) &&
						((entry_p->monitor_name_p) ? 
						 !strcmp(entry_p->monitor_name_p,
								 config_p->monitor_info.model) : TRUE))
					{
						/*
						 * Found a match.
						 */
						screen_state_p->display_mode_p = entry_p;
						break;
					}
				}

				if (mode_count == S3_DISPLAY_MODE_COUNT)
				{
					initialization_status |=
						S3_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND; 
					return (initialization_status);
				}
			}
		}
		else
		{
			/*
			 * The vendor level code has given some mode information.
			 * use it.
			 */
			entry_p = screen_state_p->display_mode_p;
		}

		h_total = 		entry_p->h_total;
		h_disp_end = 	entry_p->h_disp_end;
		h_blank_start = entry_p->h_blank_start;
		h_blank_end = 	entry_p->h_blank_end;
		h_sync_start = 	entry_p->h_sync_start;
		h_sync_end = 	entry_p->h_sync_end;
		v_total = 		entry_p->v_total;
		v_retrace_start = entry_p->v_retrace_start;
		v_retrace_end = entry_p->v_retrace_end;
		v_display_end = entry_p->v_display_end;
		v_blank_start = entry_p->v_blank_start;
		v_blank_end = 	entry_p->v_blank_end;
		preset_row_scan = entry_p->preset_row_scan;
		max_scan_line = entry_p->max_scan_line;
		crtc_mode_control = entry_p->crtc_mode_control;
		line_compare = entry_p->line_compare;
		logical_screen_offset = entry_p->logical_screen_offset;
		num_screen_pages = entry_p->num_screen_pages;
		physical_width = entry_p->physical_width;
		physical_height = entry_p->physical_height;
		clock_frequency = entry_p->clock_frequency;
		interlace_retrace_start = entry_p->interlace_retrace_start;

		/* 
		 * we have got the mode entries from the 8 bit table for the 16 bit
		 * case. Munge the necessary paramteres.
		 */
		if (is_mux_dac_and_16_bit == TRUE)
		{
			logical_screen_offset <<= 1;
			physical_width <<= 1;
			physical_height >>= 1;
		}
	}

	/*
	 * Look for the appropriate clock index. In case of programmable clocks
	 * any frequency is selectable between a lower bound and an upper bound
	 * inclusive bothways. See the caveat in s3_clks.def file.
	 */
	clock_count = 0;
	if (screen_state_p->clock_chip_p->number_of_clock_frequencies == 0)
	{
#define MIN_CLOCK_FREQUENCY_BOUND	0
#define MAX_CLOCK_FREQUENCY_BOUND	1

		const int min = screen_state_p->clock_chip_p->
			clock_frequency_table[MIN_CLOCK_FREQUENCY_BOUND];
		const int max = screen_state_p->clock_chip_p->
			clock_frequency_table[MAX_CLOCK_FREQUENCY_BOUND];

		/*
		 * Check if the synthesizer can support the requested freq.
		 */
		if ((clock_frequency >= min ) && (clock_frequency <= max))
		{
			clock_count++;
		}
#undef MIN_CLOCK_FREQUENCY_BOUND
#undef MAX_CLOCK_FREQUENCY_BOUND
	}
	else
	{
		for (; clock_count <
			 screen_state_p->clock_chip_p->number_of_clock_frequencies &&
			 screen_state_p->clock_chip_p->clock_frequency_table[clock_count] !=
			 clock_frequency;
			 ++clock_count)
		{
			;
		}
	}

	if (clock_count ==
		screen_state_p->clock_chip_p->number_of_clock_frequencies)
	{
		/*
		 * Try another mode.
		 */
		initialization_status |=
			S3_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND;
		return(initialization_status);
	}
	else
	{
		screen_state_p->clock_frequency = clock_frequency;
	}

	/*
	 * First update the screen state parameters.
	 */
	switch (config_p->depth)
	{
		case 4 : screen_state_p->generic_state.screen_depth_shift = 2;
				screen_state_p->register_state.
					s3_enhanced_commands_registers.write_mask  = 0x0F;
				screen_state_p->register_state.
					s3_enhanced_commands_registers.read_mask  = 0x0F;
			break;
		case 8 : screen_state_p->generic_state.screen_depth_shift = 3;
				screen_state_p->register_state.
					s3_enhanced_commands_registers.write_mask  = 0xFF;
				screen_state_p->register_state.
					s3_enhanced_commands_registers.read_mask  = 0xFF;
				screen_state_p->register_state.s3_vga_registers.misc_1 |= 
					0x10;	
			break;
		case 16: screen_state_p->generic_state.screen_depth_shift = 4;
				screen_state_p->register_state.s3_system_extension_registers.
					extended_system_control_1 |= EX_SCTL_1_GE_PXL_LNGH_16;
				screen_state_p->register_state.
					s3_enhanced_commands_registers.write_mask  = 0xFFFF;
				screen_state_p->register_state.
					s3_enhanced_commands_registers.read_mask  = 0xFFFF;
				screen_state_p->register_state.
					s3_vga_registers.misc_1 |= 0x10;	
			break;
		case 32: 
			if ( screen_state_p->chipset_kind == 
					S3_CHIPSET_KIND_S3_CHIPSET_86C928)
			{
				screen_state_p->generic_state.screen_depth_shift = 5;
				screen_state_p->register_state.s3_system_extension_registers.
					extended_system_control_1 |= EX_SCTL_1_GE_PXL_LNGH_32;
				screen_state_p->register_state.
					s3_enhanced_commands_registers.write_mask  = 0xFFFFFFFF;
				screen_state_p->register_state.
					s3_enhanced_commands_registers.read_mask  = 0xFFFFFFFF;
			}
			else
			{
				initialization_status |= S3_INITIALIZE_UNSUPPORTED_DEPTH;
			}
			break;
		default:
			initialization_status |= S3_INITIALIZE_UNSUPPORTED_DEPTH;
			if(initialization_status)
			{
				return(initialization_status);
			}
			else
			{
				break;
			}
	}

	/*
	 * Update the screen state.
	 */
	screen_state_p->generic_state.screen_physical_width = 
		((physical_width << 3) >> 
		 screen_state_p->generic_state.screen_depth_shift);
	screen_state_p->generic_state.screen_physical_height = physical_height;

	/*
	 * update the necessary registers.
	 */
	screen_state_p->register_state.s3_vga_registers.mem_cfg = 0x89;
	if (options_p->s3_bus_width == S3_OPTIONS_S3_BUS_WIDTH_16_BIT)
	{
		screen_state_p->register_state.s3_vga_registers.mem_cfg |= 0x04;						
	}

	screen_state_p->register_state.s3_vga_registers.mem_cfg |= 
					(unsigned char )((num_screen_pages << 1) & 0x02);

	screen_state_p->register_state.s3_vga_registers.bkwd_compat_1 = 0x00; 
					/* bit8 reserved in 801 */
	screen_state_p->register_state.s3_vga_registers.bkwd_compat_2 = 0x20; 
					/* no border */
	screen_state_p->register_state.s3_vga_registers.bkwd_compat_3 = 0x00;

	screen_state_p->register_state.s3_vga_registers.crt_reg_lock  = 0x00; 
					/* unlock all */
	screen_state_p->register_state.s3_vga_registers.misc_1 |= 0x05;						

	if (options_p->s3_bus_width == S3_OPTIONS_S3_BUS_WIDTH_16_BIT)
	{
		screen_state_p->register_state.s3_vga_registers.misc_1 |= 0x80;						
	}

	/*
	 * Screen size.
	 * The following observations have been made:
	 * 1. When installed memory is 1 or 2 MB  for all resolutions 
	 *    choose bit 2 of advfunc control register to be 0. This
	 *    bit seems to be 'dont care' if the depth is 8.
	 * 2. Bits 7-6 of extended system control 1 have to be 00 for all
	 *    modes other than 1280x1024x8.
	 */
	screen_state_p->register_state.s3_system_extension_registers.
		extended_system_control_1 |= EX_SCTL_1_GE_SCR_W_1024_OR_2048;
	switch (config_p->virt_w)
	{
		/*CONSTANTCONDITION*/
		case 640:
			break;
		case 800:
			if((config_p->depth != 8) && (config_p->depth != 16))
			{
				screen_state_p->register_state.s3_enhanced_commands_registers.
					advfunc_control |= ADVFUNC_CNTL_SCRN_SIZE_800_OR_1024;
			}
			break;
		case 1024:
			if((config_p->depth != 8) && (config_p->depth != 16))
			{
				screen_state_p->register_state.s3_enhanced_commands_registers.
					advfunc_control |= ADVFUNC_CNTL_SCRN_SIZE_800_OR_1024;
			}
			break;
		case 1280:
			if(config_p->depth == 8)
			{
				screen_state_p->register_state.s3_system_extension_registers.
					extended_system_control_1 |= EX_SCTL_1_GE_SCR_W_1280;
			}
			if((config_p->depth != 8) && (config_p->depth != 16))
			{
				screen_state_p->register_state.s3_enhanced_commands_registers.
					advfunc_control |= ADVFUNC_CNTL_SCRN_SIZE_800_OR_1024;
			}
			break;
	}

	/*
	 * Initialize the S3 vga mode control register (CLOCK_SELECT)
	 */
	screen_state_p->register_state.s3_system_control_registers.
		mode_control |= (unsigned char)clock_count & 0x0F;

	/*CONSTANTCONDITION*/
	ASSERT(interlace_retrace_start >= 0);

	/*
	 * Htotal active ?, reserved in 801.
	 */
	screen_state_p->register_state.s3_vga_registers.
		data_execute_position = 0x00; 
	screen_state_p->register_state.s3_vga_registers.
		interlace_retrace_start = 0x00;
	if (interlace_retrace_start)
	{
		screen_state_p->register_state.s3_system_control_registers.
			mode_control |= MODE_CTL_CR42_INTERLACED;
		screen_state_p->register_state.s3_vga_registers.
			interlace_retrace_start = (unsigned char)interlace_retrace_start;
	}

	/*
	 * Load the crtc Register Values.
	 */
	vga_crtc_registers = &(screen_state_p->register_state.
		standard_vga_registers.standard_vga_crtc_registers);

	vga_crtc_registers->h_total	= (unsigned char)(h_total & 0xFF);
	vga_crtc_registers->h_d_end = (unsigned char)(h_disp_end & 0xFF);
	vga_crtc_registers->s_h_blank = (unsigned char) (h_blank_start & 0xFF);
	vga_crtc_registers->e_h_blank = (unsigned char) (h_blank_end & 0x1F);
	if (options_p->display_skew)
	{
		vga_crtc_registers->e_h_blank |= 
			((unsigned char)(options_p->display_skew) << 5U);
	}
	vga_crtc_registers->s_h_sy_p = (unsigned char) (h_sync_start & 0xFF);
	vga_crtc_registers->e_h_sy_p = (unsigned char) (
								((unsigned char)h_sync_end & 0x1F) |
								((unsigned char)(h_blank_end & 0x20) << 2U));
	if (options_p->horizontal_skew)
	{
		vga_crtc_registers->e_h_blank |= 
			((unsigned char)(options_p->horizontal_skew) << 5U);
	}
	vga_crtc_registers->v_total = (unsigned char) (v_total & 0xFF);
	vga_crtc_registers->preset_row_scan = 
							(unsigned char)(preset_row_scan & 0x1F);
	vga_crtc_registers->max_scan_lines = 
						(unsigned char) (
						(max_scan_line & 0x1FU) |
						((line_compare & 0x200U) >> 3U)|
						((v_blank_start & 0x200U) >> 4U));
	vga_crtc_registers->vert_ret_start = 
							(unsigned char)(v_retrace_start & 0xFF);
	/*
	 * disable the vertical retrace interrupt from occouring.
	 */
	vga_crtc_registers->vert_ret_end=
							(unsigned char)(v_retrace_end & 0x0F) | 0x20;
	vga_crtc_registers->vert_disp_end=
							(unsigned char)(v_display_end & 0xFF);
	vga_crtc_registers->screen_offset = 
							(unsigned char)(logical_screen_offset & 0xFF);
	vga_crtc_registers->start_vert_blank  = 
							(unsigned char)(v_blank_start & 0xFF);
	vga_crtc_registers->end_vert_blank = 
							(unsigned char)(v_blank_end & 0xFF);
	vga_crtc_registers->ovfl_reg = 
			(unsigned char) (
				((v_total & 0x100U) >> 8U) |
				((v_total & 0x200U) >> 4U) |
				((v_retrace_start & 0x100U) >> 6U) |
				((v_retrace_start & 0x200U) >> 2U) |
				((v_display_end & 0x100U) >> 7U) |
				((v_display_end & 0x200U) >> 3U) |
				((line_compare & 0x100U) >> 4U) |
				((v_blank_start & 0x100U) >> 5U));
	vga_crtc_registers->crtc_mode_control = (unsigned char)crtc_mode_control;
	vga_crtc_registers->line_cmp = (unsigned char)line_compare;

	/* 
	 * start address registers
	 */
	vga_crtc_registers->start_addr_h = 0;
	vga_crtc_registers->start_addr_l = 0;

	/*
	 * Initialize the Extended overflow registers (CR5D,CR5E).
	 */
	screen_state_p->register_state.s3_system_extension_registers.
		extended_horz_ovfl = (unsigned char )(
						((h_total & 0x100U) >> 8U) |
						((h_disp_end & 0x100U) >> 7U) | 
						((h_blank_start & 0x100U) >> 6U) | 
						((h_sync_start & 0x100U) >> 4U));
	screen_state_p->register_state.s3_system_extension_registers.
		extended_vert_ovfl = (unsigned char )(
						((v_total & 0x400U) >> 10U) |
						((v_display_end & 0x400U) >> 9U) |
						((v_blank_start & 0x400U) >> 8U) |
						((v_retrace_start & 0x400U) >> 6U));
	screen_state_p->register_state.s3_system_extension_registers.
		extended_system_control_2 |= (unsigned char) 
						((logical_screen_offset & 0x300U) >> 4U);
	/*
	 * check if we should support hardware panning.
	 * Make sure that screen_state_p->display_mode_p is 
	 * already initialized.
	 */
	initialization_status |= s3_check_if_panning_is_feasibile( config_p, 
															screen_state_p);
	return (initialization_status);
}
/*
 * Override any register default that might not suit the 801 chipset.
 * This is a good place to introduce the chipset stepping number specific 
 * overrides if any.
 */
STATIC int
s3_801_specific_set_registers(SIConfigP config_p,
					struct s3_options_structure *options_p,
					struct s3_screen_state *screen_state_p)
{
	int initialization_status = 0;

	/*
	 * S3C801 can't support more than 1280x1024 resolution.
	 */
	if ((screen_state_p->chipset_kind == 
			S3_CHIPSET_KIND_S3_CHIPSET_86C801) && (config_p->virt_w > 1280))
	{
		initialization_status |= S3_INITIALIZE_UNSUPPORTED_RESOLUTION;
		return(initialization_status);
	}
	/*
	 * Cogged from the x386 code. Could be wrong.
	 * screen_state_p->register_state.extended_mem_control_2 |= 0xA0;
	 */
	/*
	 * check if the user specified memory size is different from the
	 * one detected. In such a case override the detected size.
	 */
	if (is_memory_size_mismatch == TRUE)
	{
		switch(config_p->videoRam)
		{
			case 512:
				screen_state_p->register_state.s3_vga_registers.
					config_1 |= CONFIG_1_DISPLAY_MEMORY_SIZE_512;
				break;
			case 1024:
				screen_state_p->register_state.s3_vga_registers.
					config_1 |= CONFIG_1_DISPLAY_MEMORY_SIZE_1024;
				break;
			case 2048:
				screen_state_p->register_state.s3_vga_registers.
					config_1 |= CONFIG_1_DISPLAY_MEMORY_SIZE_2048;
				break;
			default:
				initialization_status |= S3_INITIALIZE_UNSUPPORTED_MEMORY_SIZE;
				break;
		}
	}
	else
	{
		screen_state_p->register_state.s3_vga_registers.config_1 = 
			screen_state_p->register_state.saved_s3_vga_registers.config_1;
	}
	if (initialization_status)
	{
		return(initialization_status);
	}

	/*
	 * This bit is reserved in 801 and is set to 1.
	 */
	screen_state_p->register_state.s3_enhanced_commands_registers.
		advfunc_control |=  0x02;

	/*
	 * This is magic. No documentation is available for programming
	 * these registers. But the free software code and the pmi files
	 * seem to touch these registers.
	 */
	screen_state_p->register_state.
		s3_extra_registers[S3_REGISTER_CR60_INDEX] = 0x3f;
	screen_state_p->register_state.
		s3_extra_registers[S3_REGISTER_CR61_INDEX] = 0x81;
	screen_state_p->register_state.
		s3_extra_registers[S3_REGISTER_CR62_INDEX] = 0x00;
	return(initialization_status);
}
/*
 * Override any register default that might not suit the 928 chipset
 * like some bits that are reserved in an 801 that are nolonger in a 928.
 * This is a good place to introduce the chipset stepping number specific 
 * overrides if any.
 */
STATIC int
s3_928_specific_set_registers(SIConfigP config_p,
					struct s3_options_structure *options_p,
					struct s3_screen_state *screen_state_p)
{
	int initialization_status = 0;

	if(options_p->vram_addressing_mode == 
		S3_OPTIONS_VRAM_ADDRESSING_MODE_PARALLEL) 
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_mem_control_1 |= EX_MCTL_1_PAR_VRAM;
	}

	/*
	 * check if the user specified memory size is different from the
	 * one detected. In such a case override the detected size.
	 */
	if (is_memory_size_mismatch == TRUE)
	{
		switch(config_p->videoRam)
		{
			case 512:
				screen_state_p->register_state.s3_vga_registers.
					config_1 |= CONFIG_1_DISPLAY_MEMORY_SIZE_512;
				break;
			case 1024:
				screen_state_p->register_state.s3_vga_registers.
					config_1 |= CONFIG_1_DISPLAY_MEMORY_SIZE_1024;
				break;
			case 2048:
				screen_state_p->register_state.s3_vga_registers.
					config_1 |= CONFIG_1_DISPLAY_MEMORY_SIZE_2048;
				break;
			case 3072:
				screen_state_p->register_state.s3_vga_registers.
					config_1 |= CONFIG_1_DISPLAY_MEMORY_SIZE_2048;
				break;
			case 4096:
				screen_state_p->register_state.s3_vga_registers.
					config_1 |= CONFIG_1_DISPLAY_MEMORY_SIZE_2048;
				break;
			default:
				initialization_status |= S3_INITIALIZE_UNSUPPORTED_MEMORY_SIZE;
				break;
		}
	}
	else
	{
		screen_state_p->register_state.s3_vga_registers.config_1 = 
			screen_state_p->register_state.saved_s3_vga_registers.config_1;
	}
	if (initialization_status)
	{
		return(initialization_status); 
	}

	/*
	 * This logic is from XFree86 code.
	 */
	screen_state_p->register_state.s3_vga_registers.data_execute_position = 
		(screen_state_p->register_state.standard_vga_registers.
			standard_vga_crtc_registers.h_total +
		screen_state_p->register_state.standard_vga_registers.
			standard_vga_crtc_registers.s_h_sy_p) / 2U;

	return(initialization_status);
}
/*
 * Provide reasonable initialization defaults for the s3/generic vga 
 * registers depending on the mode. 
 */
STATIC int
s3_set_registers(SIConfigP config_p, 
					struct s3_options_structure *options_p,
					struct s3_screen_state *screen_state_p)
{
	int initialization_status = 0;
	unsigned char	misc_output_register = 0;
	struct vga_general_register_state   *vga_general_registers = 
			&(screen_state_p->register_state.standard_vga_registers.
			standard_vga_general_registers);
	struct vga_sequencer_register_state   *vga_sequencer_registers = 
			&(screen_state_p->register_state.standard_vga_registers.
			standard_vga_sequencer_registers);
	struct vga_graphics_controller_register_state   *vga_gc_registers = 
			&(screen_state_p->register_state.standard_vga_registers.
			standard_vga_graphics_controller_registers);
	struct vga_attribute_controller_register_state   *vga_ac_registers = 
			&(screen_state_p->register_state.standard_vga_registers.
			standard_vga_attribute_controller_registers);

	/*
	 * Provide reasonable CRTC defaults depending on the mode.
	 */
	if ((initialization_status = s3_get_mode(config_p, options_p,
											   screen_state_p)))
	{
		return (initialization_status);
	}

	/*
	 * Provide defaults for the standard vga general, sequencer, 
	 * attribute controller and the graphics controller registers.
	 */
	misc_output_register |= (MISC_OUT_ENB_RAM | MISC_OUT_EXTERNAL_CLOCK);	
	/*
	 * If Color screen then program for accessing crtc address at
	 * 3d? address.
	 */
	if ( config_p->depth != 1 )
	{
		 misc_output_register |= MISC_OUT_COLOR_EMULATION;
	}
	if (options_p->hsync_polarity != S3_OPTIONS_HSYNC_POLARITY_POSITIVE)
	{
		misc_output_register |=  MISC_OUT_HSPBAR;
	}
	if (options_p->hsync_polarity != S3_OPTIONS_HSYNC_POLARITY_POSITIVE)
	{
		misc_output_register |= MISC_OUT_VSPBAR;
	}
	vga_general_registers->misc_out 		= 	misc_output_register;
	vga_general_registers->feature_control	=	0; 

	vga_sequencer_registers->reset_sync		=	0x03;
	vga_sequencer_registers->clk_mode		=	0x21;
	vga_sequencer_registers->enab_wr_plane	=	0x0F;
	vga_sequencer_registers->ch_font_sel	=	0x00;
	vga_sequencer_registers->mem_mode		=	0x0E;

	if (config_p->depth == 4)
	{
		vga_ac_registers->attr_mode_control 	= 	0x01; 
	}
	else
	{
		vga_ac_registers->attr_mode_control 	= 	0x41; 
	}
	vga_ac_registers->border_color			= 	0x00;
	vga_ac_registers->color_plane_enable	= 	0x0F;
	vga_ac_registers->horiz_pixel_panning 	= 	0x00;
	vga_ac_registers->pixel_padding 		= 	0x00;

	vga_gc_registers->set_reset 				= 	0x00;
	vga_gc_registers->enable_set_reset			= 	0x00;
	vga_gc_registers->color_cmp					= 	0x00;
	vga_gc_registers->raster_op_rotate_counter	=	0x00;
	vga_gc_registers->read_plane_select			= 	0x00;
	vga_gc_registers->graphics_controller_mode	= 	0x40;
	vga_gc_registers->mem_map_mode_control		=	0x05;
	vga_gc_registers->cmp_dont_care				=	0x0F;
	vga_gc_registers->bit_mask					=	0xFF;

	/*
	 * Initialize the S3 system control registers.
	 */
	screen_state_p->register_state.s3_system_control_registers.
		system_config |= (	SYS_CNFG_CPC_SEL |
							SYS_CNFG_WR_WAIT |
							SYS_CNFG_DEC_WAIT_3 |
							SYS_CNFG_ISA_DIS_NOWS );
	if ( options_p->wait_state_control == S3_OPTIONS_WAIT_STATE_CONTROL_NO )
	{
		screen_state_p->register_state.s3_system_control_registers.
			system_config &= ~SYS_CNFG_WR_WAIT;
	}
	if(options_p->enable_write_posting == S3_OPTIONS_ENABLE_WRITE_POSTING_YES)
	{
		screen_state_p->register_state.s3_system_control_registers.
			system_config |= SYS_CNFG_EWRT_POST;
	}
	if(options_p->decode_wait_control != 
		S3_OPTIONS_DECODE_WAIT_CONTROL_DEFAULT)
	{
		switch(options_p->decode_wait_control)
		{
			case 1:
				screen_state_p->register_state.s3_system_control_registers.
					system_config |= SYS_CNFG_DEC_WAIT_1;
			break;
			case 2:
				screen_state_p->register_state.s3_system_control_registers.
					system_config |= SYS_CNFG_DEC_WAIT_2;
			break;
			case 3:
				screen_state_p->register_state.s3_system_control_registers.
					system_config |= SYS_CNFG_DEC_WAIT_3;
			break;
		}
	}
	if(options_p->read_wait_control != 
		S3_OPTIONS_READ_WAIT_CONTROL_DEFAULT)
	{
		switch(options_p->read_wait_control)
		{
			case 1:
				screen_state_p->register_state.s3_system_control_registers.
					system_config |= SYS_CNFG_RD_WAIT_1;
			break;
			case 2:
				screen_state_p->register_state.s3_system_control_registers.
					system_config |= SYS_CNFG_RD_WAIT_2;
			break;
			case 3:
				screen_state_p->register_state.s3_system_control_registers.
					system_config |= SYS_CNFG_RD_WAIT_3;
			break;
		}
	}

	screen_state_p->register_state.s3_system_control_registers.
		extended_mode = 0x00;
#ifdef DELETE /* Does not look like it is required for the 640 mode., 
				other modes */
	if ( config_p->depth == 16 )
	{
		screen_state_p->register_state.s3_system_control_registers.
			extended_mode |= EXT_MODE_64K_CLR;
	}
#endif
	if (options_p->enable_alternate_ioport_address  == 
		S3_OPTIONS_ENABLE_ALTERNATE_IOPORT_ADDRESS_YES )
	{
		screen_state_p->register_state.s3_system_control_registers.
			extended_mode |= EXT_MODE_XEN;
	}
	screen_state_p->register_state.s3_system_control_registers.
		hw_cursor_mode = 0x00;
#ifdef DELETE
	if (options_p->cursor_type == S3_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR)
	{
		screen_state_p->register_state.s3_system_control_registers.
			hw_cursor_mode |= HGC_MODE_HWGC_ENB;
	}
	screen_state_p->register_state.s3_system_control_registers.
		hw_cursor_origin_x = 0x00;
	screen_state_p->register_state.s3_system_control_registers.
		hw_cursor_origin_y = 0x00;
	screen_state_p->register_state.s3_system_control_registers.
		hw_cursor_frgd_stack = 0x00;
	screen_state_p->register_state.s3_system_control_registers.
		hw_cursor_bkgd_stack = 0x00;
	screen_state_p->register_state.s3_system_control_registers.
		hw_cursor_start_address = 0x00;
	screen_state_p->register_state.s3_system_control_registers.
		hw_cursor_pattern_start_x_position = 0x00;
	screen_state_p->register_state.s3_system_control_registers.
		hw_cursor_pattern_start_y_position = 0x00;
#endif

	/*
	 * Initialize the S3 system extension registers.
	 */
	if(options_p->enable_split_transfers == 
		S3_OPTIONS_ENABLE_SPLIT_TRANSFERS_NO)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_system_control_2 |= EX_SCTL_2_DIS_SPXF;
	}
	if(options_p->enable_eprom_write == S3_OPTIONS_ENABLE_EPROM_WRITE_YES)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_system_control_2 |= EX_SCTL_2_ENB_ERW;
	}

	if(options_p->enable_nibble_swap == S3_OPTIONS_ENABLE_NIBBLE_SWAP_YES)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_mem_control_1 |= EX_MCTL_1_SWP_NBL;
	}
	if(options_p->enable_nibble_write_control == 
		S3_OPTIONS_ENABLE_NIBBLE_WRITE_CONTROL_YES)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_mem_control_1 |= EX_MCTL_1_ENB_NBLW;
	}
	
	if(options_p->rac_extra_prefetch > 0)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_mem_control_2 |= options_p->rac_extra_prefetch &
			EX_MCTL_2_RAC_EXT_PFTCH_MASK;
	}

	screen_state_p->register_state.s3_system_extension_registers.
		extended_dac_control = 0x00;
	if(options_p->cursor_type == S3_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_dac_control |= EX_DAC_CT_CURSOR_MODE_X11;
	}
	screen_state_p->register_state.s3_system_extension_registers.
		extended_sync_1 = 0x00;
	screen_state_p->register_state.s3_system_extension_registers.
		extended_sync_2 = 0x00;

	/*
	 * Disable linear addressing by default.
	 */
	if(options_p->limit_write_posting == S3_OPTIONS_LIMIT_WRITE_POSTING_YES)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_linear_addr_window_control |= LAW_CTL_LMT_WPE;
	}
	if(options_p->enable_read_ahead_cache == 
		S3_OPTIONS_ENABLE_READ_AHEAD_CACHE_YES)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_linear_addr_window_control |= LAW_CTL_ENB_RAC;
	}
	if(options_p->latch_isa_addr == S3_OPTIONS_LATCH_ISA_ADDR_YES)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_linear_addr_window_control |=  LAW_CTL_ISA_LAD;
	}
	if(options_p->serial_access_mode_control == 
		S3_OPTIONS_SERIAL_ACCESS_MODE_CONTROL_256)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_linear_addr_window_control |=  LAW_CTL_SAM_256;
	}
	if(options_p->ras_m_clk == S3_OPTIONS_RAS_M_CLK_6)
	{
		screen_state_p->register_state.s3_system_extension_registers.
			extended_linear_addr_window_control |= LAW_CTL_RAS_6_MCLK;
	}

	screen_state_p->register_state.s3_system_extension_registers.
		extended_linear_addr_window_pos = 0x00;

	/*
	 * S3 Enhanced commands registers.
	 */
	screen_state_p->register_state.s3_enhanced_commands_registers.
		advfunc_control |=  ADVFUNC_CNTL_ENB_EHFC_ENHANCED;
	/*
	 * defaults which might prove meaningful for all depths.
	 */
	screen_state_p->register_state.s3_enhanced_commands_registers.
		scissor_t = 0;
	screen_state_p->register_state.s3_enhanced_commands_registers.
		scissor_l = 0;
	screen_state_p->register_state.s3_enhanced_commands_registers.
		scissor_b = screen_state_p->
			generic_state.screen_physical_width;
	screen_state_p->register_state.s3_enhanced_commands_registers.
		scissor_r = screen_state_p->
			generic_state.screen_physical_height;
	screen_state_p->register_state.s3_enhanced_commands_registers.
		mult_misc = 0x00;
	if (options_p->fast_rmw == S3_OPTIONS_FAST_RMW_NO )
	{
		screen_state_p->register_state.s3_enhanced_commands_registers.
			mult_misc |= 0x40;
	}

	/*
	 * Call the chipset specific init routines.
	 */
	if (screen_state_p->chipset_kind == 
			S3_CHIPSET_KIND_S3_CHIPSET_86C801)
	{
		initialization_status |= s3_801_specific_set_registers(
					config_p, options_p, screen_state_p);
	}
	else
	{
		initialization_status |= s3_928_specific_set_registers(
					config_p, options_p, screen_state_p);
	}
	if (initialization_status)
	{
		return(initialization_status);
	}

	/*
	 * 20 apr 94 : Provision for user to supply info on new registers.
	 * initialize these crtc registers and their values.
	 */
	{
		char *new_regs_string = NULL;

		if (screen_state_p->options_p->register_values_string != NULL &&
			((new_regs_string = 
			strdup(screen_state_p->options_p->register_values_string))!=NULL)) 
		{
			struct new_s3_register_node *current_new_s3_regs_node_p;
			struct new_s3_register_node *tmp_new_s3_regs_node_p;
			char *tuple_string;
			/*
			 * Parse the tokens from the user specified string.
			 */
			while((tuple_string = strtok(new_regs_string,",")) != NULL)
			{
				tmp_new_s3_regs_node_p = (struct new_s3_register_node *)
					allocate_memory(sizeof(struct new_s3_register_node));
#if (defined(__DEBUG__))
					if (s3_debug)
					{
						(void) fprintf(debug_stream_p, 
							"\t(s3_get_mode) tuple = <%s>\n",tuple_string);
					}
#endif
				/*
				 * Get the 4 tuple. and the original value of the register.
				 */
				if(sscanf(tuple_string,"%i %i %i %i",
					&(tmp_new_s3_regs_node_p->index),
					&(tmp_new_s3_regs_node_p->mask),
					&(tmp_new_s3_regs_node_p->value),
					&(tmp_new_s3_regs_node_p->rbits)) != 4)
				{
					(void)fprintf(stderr,
						S3_PARSE_REGISTERS_FAILED_MESSAGE,tuple_string);
					free_memory(tmp_new_s3_regs_node_p);
				}
				else
				{
					S3_READ_CRTC_REGISTER(tmp_new_s3_regs_node_p->index,
						tmp_new_s3_regs_node_p->saved_value);
					tmp_new_s3_regs_node_p->next_p = NULL; 
					if (screen_state_p->register_state.
						new_s3_registers_list_head_p == NULL)
					{
						screen_state_p->register_state.
						new_s3_registers_list_head_p = tmp_new_s3_regs_node_p;
						current_new_s3_regs_node_p = tmp_new_s3_regs_node_p;
					}
					else
					{
						current_new_s3_regs_node_p->next_p = 
							tmp_new_s3_regs_node_p;
						current_new_s3_regs_node_p = tmp_new_s3_regs_node_p;
					}
				}
				new_regs_string = NULL;
			}
		}
	}

	if(s3_dac_check_display_mode_feasibility(screen_state_p) == FALSE)
	{
		initialization_status |= S3_INITIALIZE_MODE_NOT_SUPPORTED_BY_DAC;
	}
	return ( initialization_status);
}

#if (defined(UNUSED_FUNCTIONALITY))
/*
 * s3_screen_control
 *
 * Handle switching between screens.
 */
function SIBool
s3_screen_control(SIint32 screen_number, SIint32 control_request)
{
	return (SI_SUCCEED);
}
#endif

/**
 **		Virtual terminal functionality.
 **/

/*
 * s3_save_screen_contents
 *
 */
STATIC void
s3_save_screen_contents(void *contents_p, int vt_switch_save_lines)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_current_screen_state_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));

#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_save_screen_contents) {\n"
			"\tcontents_p = %p\n"
			"\tvt_switch_save_lines = %d\n"
			"}\n",
			contents_p,
			vt_switch_save_lines);
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));


	if (vt_switch_save_lines <= 0)
	{
		return;
	}


	/*
	 * Check if a clipping rectangle reprogramming is required. If yes, 
	 * set it such that drawing is allowed for the entire screen.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		GENERIC_CLIP_TO_VIDEO_MEMORY)
	{
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p,
			0,
			0,
			screen_state_p->generic_state.screen_physical_width,
			screen_state_p->generic_state.screen_physical_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIDEO_MEMORY;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	/*
	 * The Rop has to be GXCopy.
	 */
	S3_STATE_SET_FG_ROP(screen_state_p,S3_MIX_FN_N);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_CPU_DATA);

	S3_WAIT_FOR_FIFO(1);
	/*
	 * The read mask is all ones.
	 */
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
		(1 << screen_state_p->generic_state.screen_depth) -1);
	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_RD_MASK);

	/*
	 * Select Foreground Mix. Image transfer as against to stippling.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	
	/*
	 * Blit a rectangle of display memory to the cpu memory.
	 */
	ASSERT(screen_state_p->generic_state.screen_contents_p);
	if (screen_state_p->generic_state.screen_contents_p)
	{
		s3_asm_transfer_helper(
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
			S3_ASM_TRANSFER_FROM_VIDEO_MEMORY|S3_ASM_TRANSFER_THRO_PLANE);
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return;
}
STATIC void
s3_restore_screen_contents(void *contents_p, int vt_switch_save_lines)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_current_screen_state_p;
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));

#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_restore_screen_contents) {\n"
			"\tcontents_p = %p\n"
			"\tvt_switch_save_lines = %d\n"
			"}\n",
			contents_p,
			vt_switch_save_lines);
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (vt_switch_save_lines <= 0)
	{
		return;
	}

	if (screen_state_p->generic_state.screen_current_clip !=
		GENERIC_CLIP_TO_VIDEO_MEMORY)
	{
	  
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 
			0,
			0,
			screen_state_p->generic_state.screen_physical_width,
			screen_state_p->generic_state.screen_physical_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIDEO_MEMORY;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	/*
	 * The Rop has to be GXCopy.
	 */
	S3_STATE_SET_FG_ROP(screen_state_p,S3_MIX_FN_N);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_CPU_DATA);

	S3_WAIT_FOR_FIFO(1);
	/*
	 * The write mask is all ones.
	 */
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
		(1 << screen_state_p->generic_state.screen_depth) -1);
	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_WRT_MASK);

	/*
	 * Select Foreground Mix. Image transfer as against to stippling.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);

	/*
	 * Restore the screen contents from cpu memory to screen memory.
	 */
	ASSERT(screen_state_p->generic_state.screen_contents_p);
	if (screen_state_p->generic_state.screen_contents_p)
	{
		s3_asm_transfer_helper(
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
			S3_ASM_TRANSFER_TO_VIDEO_MEMORY | S3_ASM_TRANSFER_THRO_PLANE);
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return;
}
/*
 * Switching out of the x server mode.
 * s3_virtual_terminal_save
 *
 * Save as much of the register state as possible and 
 * and restore the chipset back to the vga state.
 */

STATIC SIBool
s3_virtual_terminal_save(void)
{

	struct s3_screen_state *screen_state_p = 
		(struct s3_screen_state *) generic_current_screen_state_p;
	struct standard_vga_state	*stdvga_regs;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));
	
	/*
	 * Save the contents of offscreen memory if necessary.
	 */

	if ((generic_current_screen_state_p->screen_contents_p == NULL) &&
		screen_state_p->vt_switch_save_lines )
	{
		generic_current_screen_state_p->screen_contents_p =
			allocate_memory((screen_state_p->vt_switch_save_lines *
					 screen_state_p->generic_state.screen_physical_width * 
					 screen_state_p->generic_state.screen_depth) >> 3);
	}
	
	s3_save_screen_contents(generic_current_screen_state_p->
		      screen_contents_p, screen_state_p->vt_switch_save_lines);
	
	/*
	 * Inform each concerned module that we are about to lose control
	 * of the VT.
	 */
	s3__vt_switch_out__();

	/*
	 * Colormaps are kept in memory and so don't need to be saved.
	 * But the dac needs to be uninitialized.
	 */
	ASSERT(screen_state_p->register_state.generic_state.dac_uninit != NULL);

	if (!(*screen_state_p->register_state.generic_state.dac_uninit)
		((struct generic_screen_state *) screen_state_p))
	{
#if (defined(__DEBUG__))
		if (s3_debug)
		{
			(void) fprintf(debug_stream_p,
			   "(s3_virtual_terminal_save) dac init failed.\n");
		}
#endif
		return (SI_FAIL);
	}

	/*
	 * Wait for the graphics engine to finish with all its commands.
	 */
	S3_WAIT_FOR_GE_IDLE();
	
	/*
	 * Switch back to VGA mode.
	 * color/mono and use the appropriate addresses.
	 * Here we have to determine if the original screen was 
	 * color/mono and use the appropriate addresses.
	 */
	stdvga_regs =  &screen_state_p->register_state.
		saved_standard_vga_registers;

	/*
	 * CRTC Reserved bits. Make them 0.
	 * bit 7 of horizontal blanking is reserved.
	 * bit 7 of preset row scan is reserved.
	 * bit 7,6 of cursor start scanline are reserved.
	 * bit 7 of cursor end scanline is reserved.
	 * bit 7 of underline location is reserved.
	 * bit 4 of mode control is reserved.
	 */
	stdvga_regs->standard_vga_crtc_registers.e_h_blank &= 0x7f;
	stdvga_regs->standard_vga_crtc_registers.preset_row_scan &= 0x7f;
	stdvga_regs->standard_vga_crtc_registers.cursor_start_scan_line &= 0x3f;
	stdvga_regs->standard_vga_crtc_registers.cursor_end_scan_line &= 0x7f;
	stdvga_regs->standard_vga_crtc_registers.under_line_loc &= 0x7f;
	stdvga_regs->standard_vga_crtc_registers.crtc_mode_control &= 0xef;

	/* 
	 * Attribute controller reserved bits.
	 * bit 4 of attribute mode control is reserved.
	 * bits 7,6 of color plane enable are reserved.
	 * bits 7,6,5,4 of horizontal pixel panning are reserved.
	 * bits 7,6,5,4 of pixel padding are reserved.
	 */
	stdvga_regs->standard_vga_attribute_controller_registers.
		attr_mode_control &= 0xef;
	stdvga_regs->standard_vga_attribute_controller_registers.
		color_plane_enable &= 0x3f;
	stdvga_regs->standard_vga_attribute_controller_registers.
		horiz_pixel_panning &= 0x0f;
	stdvga_regs->standard_vga_attribute_controller_registers.
		pixel_padding &= 0x0f;

	/*
	 * Force back to vga mode.
	 * lock enhanced registers.
	 */
	screen_state_p->register_state.saved_s3_enhanced_commands_registers.
		advfunc_control = 0x2;
	screen_state_p->register_state.saved_s3_system_control_registers.
		system_config &= (unsigned char )~SYS_CNFG_ENA_8514;

	/*
	 * Forcibly unlock the vga crtc registers.
	 */
	if(stdvga_regs->standard_vga_crtc_registers.vert_ret_end & 0x80)
	{
		stdvga_regs->standard_vga_crtc_registers.vert_ret_end &=  
			~0x80;
	}
	if(screen_state_p->register_state.saved_s3_vga_registers.crt_reg_lock
		& 0x30)
	{
		screen_state_p->register_state.saved_s3_vga_registers.
			crt_reg_lock &= ~0x30;
	}


	/*
	 * Restore the chipset state.
	 */
	if (!(screen_state_p->register_state.generic_state.register_put_state)
		((struct generic_screen_state *) screen_state_p))
	{
#if (defined(__DEBUG__))
		if (s3_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s3_virtual_terminal_save) register "
						   "init failed.\n");
		}
#endif
		return (SI_FAIL);
	}
	
#if (defined(USE_KD_GRAPHICS_MODE))
	if (!s3_set_virtual_terminal_mode(screen_state_p->generic_state.
		screen_virtual_terminal_file_descriptor,KD_TEXT0))
	{
		return (SI_FAIL);
	}
#endif
	return (SI_SUCCEED);
}
/*
 * s3_virtual_terminal_restore
 * VT switching into the X server. X server VT is now the active VT.
 * Assumptions.  DAC Programming is handled by the vendor level code.
 */
STATIC SIBool
s3_virtual_terminal_restore()
{
	struct s3_screen_state *screen_state_p = 
		(struct s3_screen_state *) generic_current_screen_state_p;
	struct generic_colormap *colormap_p;
	unsigned short *rgb_p;
	int		i;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));

#if (defined(USE_KD_GRAPHICS_MODE))
	if (!s3_set_virtual_terminal_mode(screen_state_p->generic_state.
		screen_virtual_terminal_file_descriptor,KD_GRAPHICS))
	{
		return (SI_FAIL);
	}
#endif
	/*
	 * Figure out the current addr base for the crtc register set.
	 */
	if (inb(VGA_GENERAL_REGISTER_MISC_RD) & 0x01)
	{
		/*
		 * Color.
		 */
		 screen_state_p->vga_crtc_address =
			VGA_CRTC_REGISTER_CRTC_ADR_COLOR;
		 screen_state_p->vga_crtc_data = 
			VGA_CRTC_REGISTER_CRTC_DATA_COLOR;
		 screen_state_p->vga_input_status_address =
			VGA_GENERAL_REGISTER_STATUS_1_COLOR;
	}
	else
	{
		/*
		 * Monochrome.
		 */
		 screen_state_p->vga_crtc_address = 
			VGA_CRTC_REGISTER_CRTC_ADR_MONO;
		 screen_state_p->vga_crtc_data =  
			VGA_CRTC_REGISTER_CRTC_DATA_MONO;
		 screen_state_p->vga_input_status_address =
			VGA_GENERAL_REGISTER_STATUS_1_MONO;
	}

	ASSERT(screen_state_p->register_state.
		   generic_state.register_put_state);
		
	/*
	 * Change mode.
	 * Reinitialize the register context to correspond to the xserver
	 * register state.
	 */
	if (!(*screen_state_p->register_state.generic_state.register_put_state)
		((struct generic_screen_state *) screen_state_p))
	{
#if (defined(__DEBUG__))
		if (s3_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s3_virtual_terminal_restore) register "
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
		if (s3_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s3_virtual_terminal_restore) dac "
						   "init failed.\n");
		}
#endif
		return (SI_FAIL);
	}

	/*
	 * Restore screen bits.
	 */
	if (generic_current_screen_state_p->screen_contents_p)
	{
		s3_restore_screen_contents(generic_current_screen_state_p->
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
	s3__vt_switch_in__();

	return (SI_SUCCEED);
}
/*
 * exiting the x server.
 * Restore screen :
 *
 * Put back contents of video memory, put back contents of the
 * colormap, enable the shadow sets.
 */
STATIC SIBool
s3_restore_screen(void)
{

	struct s3_screen_state *screen_state_p = 
		(struct s3_screen_state *) generic_current_screen_state_p;

#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(s3_restore_screen) restore_screen called.\n");
	}
#endif	

	/*
	 * Last time around, we do not want to save the x server screen.
	 */
	screen_state_p->vt_switch_save_lines = 0;

	/*
	 * We dont care about the return value at this point.
	 */
	(void) s3_virtual_terminal_save();

	if (S3_MEMORY_REGISTER_ACCESS_MODE_ANY_MMIO_OPERATION(screen_state_p))
	{
		(void)munmap(screen_state_p->mmio_base_address, 
			S3_MMIO_WINDOW_SIZE_IN_BYTES);
	}

	return(SI_SUCCEED);
}


STATIC SIBool
s3_screen_saver(SIBool flag)
{

#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p,
					   	"(s3_screen_saver) screen saver called.\n");
		(void) fprintf(debug_stream_p, "(s3_screen_saver){\n"
						"flag = %s}\n", (flag == SI_TRUE) ? "SI_TRUE" :
						"SI_FALSE");
	}
#endif	

	if(flag == SI_TRUE)
	{
		S3_TURN_SCREEN_ON();
	}
	else if(flag == SI_FALSE)
	{
		S3_TURN_SCREEN_OFF();
	}
#if (defined(__DEBUG__))
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}
#endif

	return (SI_SUCCEED);
}

STATIC void
s3_save_original_801_specific_state(struct s3_screen_state *screen_state_p)
{
	struct s3_register_state *register_state_p = 
					&screen_state_p->register_state;

	S3_READ_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR60_INDEX,
		register_state_p->saved_s3_extra_registers[S3_REGISTER_CR60_INDEX]);
	S3_READ_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR61_INDEX,
		register_state_p->saved_s3_extra_registers[S3_REGISTER_CR61_INDEX]);
	S3_READ_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR62_INDEX,
		register_state_p->saved_s3_extra_registers[S3_REGISTER_CR62_INDEX]);
	return;
}
/*
 * Function which saves the chipset state to be restored at the time
 * of relinquishing the virtual terminal. 
 */
STATIC void
s3_save_original_vga_state(struct s3_screen_state *screen_state_p)
{
	struct s3_register_state *register_state_p = 
					&screen_state_p->register_state;
	struct vga_sequencer_register_state *vga_sequencer_registers;
	unsigned char	*register_set;
	int		i;

	 /*
	  * Do this first to avoid any damage to the monitor.
	  * Turn off the screen.
	  */
	S3_TURN_SCREEN_OFF();

	/*
	 * First save the vga text mode register state.
	 */
	S3_RESET_AND_HOLD_SEQUENCER();

	/*
	 * Unlock all registers. 
	 */
	S3_UNLOCK_CRT_TIMING_REGISTERS();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();
	S3_UNLOCK_CLOCK_REGISTERS();

	 /*
	  * General registers. Note that the status registers are 
	  * read only.
	  */
	 register_state_p->saved_standard_vga_registers.
		standard_vga_general_registers.misc_out = 
			inb(VGA_GENERAL_REGISTER_MISC_RD);

	 register_state_p->saved_standard_vga_registers.
	 	standard_vga_general_registers.feature_control = 
			inb(VGA_GENERAL_REGISTER_FCR_RD);

	/*
	 * Sequencer Registers. These registers dont autoincrement.
	 */
	vga_sequencer_registers = &(register_state_p->
		saved_standard_vga_registers.standard_vga_sequencer_registers);
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_RST_SYNC_INDEX,
		vga_sequencer_registers->reset_sync); 
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX, 
		vga_sequencer_registers->clk_mode);
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_EN_WT_PL_INDEX, 
		vga_sequencer_registers->enab_wr_plane);
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_CH_FONT_SL_INDEX, 
		vga_sequencer_registers->ch_font_sel);
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_MEM_MODE_INDEX,
		vga_sequencer_registers->mem_mode);
	
	/*
	 * CRT Controller controller Registers. 
	 * These Registers dont autoincrement on a read. 
	 */
	register_set = (unsigned char *)&register_state_p->
		saved_standard_vga_registers.standard_vga_crtc_registers;

	for ( i = VGA_CRTC_REGISTER_H_TOTAL_INDEX; 
		 i <= VGA_CRTC_REGISTER_LCM_INDEX;
		 i++)
	{
		S3_READ_CRTC_REGISTER(i,*register_set);
		register_set++;
	}
	S3_READ_CRTC_REGISTER(VGA_CRTC_REGISTER_GCCL_INDEX,*register_set);
	register_set++;
	S3_READ_CRTC_REGISTER(VGA_CRTC_REGISTER_AT_CNTL_F_INDEX,*register_set);
	register_set++;
	S3_READ_CRTC_REGISTER(VGA_CRTC_REGISTER_AT_CNTL_I_INDEX,*register_set);

	/*
	 * Save the Graphics controller registers.
	 * These registers dont autoincrement on read.
	 */
	register_set = (unsigned char *)&register_state_p-> 
		saved_standard_vga_registers.
		standard_vga_graphics_controller_registers;
	for ( i = VGA_GRAPHICS_CONTROLLER_REGISTER_SET_RST_DT; 
		 i <= VGA_GRAPHICS_CONTROLLER_REGISTER_BIT_MASK;
		 i++)
	{
		S3_READ_GRAPHICS_CONTROLLER_REGISTER(i,*register_set);
		register_set++;
	}

	/*
	 * Save the Attribute controller registers.
	 */
	register_set = (unsigned char *)&register_state_p->
		saved_standard_vga_registers.
		standard_vga_attribute_controller_registers;
	for ( i = VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR00; 
		 i <= VGA_ATTRIBUTE_CNTL_REGISTER_PX_PAD;
		 i++)
	{
		S3_READ_ATTRIBUTE_CONTROLLER_REGISTER(i,*register_set);
		register_set++;
	}
	S3_CLEAR_ATTRIBUTE_CONTROLLER_FLIPFLOP();
	outb(VGA_ATTRIBUTE_CNTL_REGISTER_ADR,0x20);

	/*
	 * Save the dac mask.
	 */
	register_state_p->saved_standard_vga_registers.
		standard_vga_dac_registers.dac_mask = 
		inb(VGA_DAC_REGISTER_DAC_AD_MK);

	register_set = 
		(unsigned char *)&register_state_p->saved_s3_vga_registers;
	for ( i = S3_VGA_REGISTER_CHIP_ID_INDEX;
		  i <= S3_VGA_REGISTER_IL_RTSTART_INDEX;
		  i++)
	{
		S3_READ_CRTC_REGISTER(i,*register_set);
		register_set++;
	}

	register_set = (unsigned char *)&register_state_p->
			saved_s3_system_control_registers;
	for ( i = S3_SYSTEM_CONTROL_REGISTER_SYS_CNFG_INDEX;
		  i <= S3_SYSTEM_CONTROL_REGISTER_HWGC_DY_INDEX;
		  i++)
	{
		S3_READ_CRTC_REGISTER(i,*register_set);
		register_set++;
		/*
		 * Handle registers that are 16 bits wide.
		 */
		switch (i)
		{
			case S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_INDEX :
			case S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_INDEX :
			case S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_INDEX:	
				i++;
				S3_READ_CRTC_REGISTER(i,*register_set);
				register_set++;
			break;
		}
	}

	register_set = (unsigned char *)&register_state_p->
			saved_s3_system_extension_registers;
	for ( i = S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_1;
		  i <= S3_SYSTEM_EXTENSION_REGISTER_EXT_V_OVF;
		  i++)
		{
			S3_READ_CRTC_REGISTER(i,*register_set);
			register_set++;
			/*
			 * This is a word wide register.
			 */
			if ( i == S3_SYSTEM_EXTENSION_REGISTER_LAW_POS)
			{
				i++;
				S3_READ_CRTC_REGISTER(i,*register_set);
				register_set++;
			}
		}

	if (screen_state_p->chipset_kind ==	
			S3_CHIPSET_KIND_S3_CHIPSET_86C801)
	{
		s3_save_original_801_specific_state(screen_state_p);
	}

	/*
	 * Save the original vga palette.
	 * If dac kind is bt485 select page 0 before reading.
	 */
	if (screen_state_p->dac_kind == S3_DAC_BT485)
	{
		BT485_SELECT_REGISTER_SET(0);
	}
	register_set = register_state_p->saved_standard_vga_palette;
	outb(VGA_DAC_REGISTER_DAC_RD_AD,0);
	for (i = 0; i < S3_STANDARD_VGA_PALETTE_SIZE*3; i++)
	{
		volatile int j = s3_dac_access_delay_count;
		while (j--);
		*register_set++ = inb(VGA_DAC_REGISTER_DAC_DATA);
	}

	S3_START_SEQUENCER();
	S3_TURN_SCREEN_ON();

#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p, "(s3) Save of vga state over.\n");
	}
#endif
}
/*
 * Determine the type of S3 chipset by reading the Chipid
 * registers. Assumes that the registers have been unlocked
 * before calling this code.
 */
STATIC int
s3_get_chipset_kind(struct s3_screen_state *screen_state_p,
			struct s3_options_structure *s3_options_p)
{
	int status = 0;
	unsigned char chip_id_register_value;
	enum s3_chipset_kind detected_chipset_kind = 
		S3_CHIPSET_KIND_S3_CHIPSET_UNKNOWN;

	S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CHIP_ID_INDEX,
		chip_id_register_value);

	switch (chip_id_register_value & 0xF0)
	{
		case 0x80:
			detected_chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C911;
			if (chip_id_register_value == 0x82)
			{
				detected_chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C924;
			}
			break;
		case 0x90:
			detected_chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C928;
			break;
		case 0xA0:
			detected_chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C801;
			break;
		case 0xB0:
			detected_chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C928_PCI;
			break;
		default:
			detected_chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_UNKNOWN;
#ifdef DELETE
			status |= S3_INITIALIZE_S3_CHIPSET_DETECTION_FAILED;
#endif
	}

	if (s3_options_p->chipset_name == S3_OPTIONS_CHIPSET_NAME_AUTO_DETECT) 
	{
		screen_state_p->chipset_kind = detected_chipset_kind;
	}
	else if (s3_options_p->chipset_name == S3_OPTIONS_CHIPSET_NAME_86C911) 
	{
		screen_state_p->chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C911;
	}
	else if (s3_options_p->chipset_name == S3_OPTIONS_CHIPSET_NAME_86C924) 
	{
		screen_state_p->chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C924;
	}
	else if (s3_options_p->chipset_name == S3_OPTIONS_CHIPSET_NAME_86C928) 
	{
		screen_state_p->chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C928;
	}
	else if (s3_options_p->chipset_name == S3_OPTIONS_CHIPSET_NAME_86C801) 
	{
		screen_state_p->chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C801;
	}
	else if (s3_options_p->chipset_name == S3_OPTIONS_CHIPSET_NAME_86C928_PCI) 
	{
		screen_state_p->chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_86C928_PCI;
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		screen_state_p->chipset_kind = S3_CHIPSET_KIND_S3_CHIPSET_UNKNOWN;
#ifdef DELETE
		status |= S3_INITIALIZE_S3_CHIPSET_DETECTION_FAILED;
#endif
	}

	if (screen_state_p->chipset_kind == S3_CHIPSET_KIND_S3_CHIPSET_UNKNOWN)
	{
		status |= S3_INITIALIZE_S3_CHIPSET_DETECTION_FAILED;
	}

#if (defined (__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s3_get_chipset_kind) {\n"
			"\tDetected S3 chipset <%s>\n"
			"}\n",
			s3_chipset_kind_to_chipset_kind_dump[screen_state_p->
													chipset_kind]); 
	}
#endif

	/* 
	 * Unknown chipset kind is an error. No warnings. 
	 */
	if ((screen_state_p->chipset_kind != detected_chipset_kind) &&
		(screen_state_p->chipset_kind != S3_CHIPSET_KIND_S3_CHIPSET_UNKNOWN))
	{
		(void) fprintf(stderr, S3_CHIPSET_KIND_OVERRIDE_MESSAGE,
		s3_chipset_kind_to_chipset_kind_dump[detected_chipset_kind],
		s3_chipset_kind_to_chipset_kind_dump[screen_state_p->chipset_kind]);
	}

#if (defined(__DEBUG__))
	if (screen_state_p->chipset_kind == S3_CHIPSET_KIND_S3_CHIPSET_UNKNOWN)
	{
		if (s3_debug)
		{
			(void) fprintf(debug_stream_p, 
					"(s3)\tdetection of S3 chipset failed.\n");
		}
	}
#endif /* __DEBUG__ */
	return (status);
}
/*
 * Determine the S3 chipset stepping number from the Chipid/revision
 * register. Assumes that the register sets have been unlocked prior
 * to calling this code.
 */
STATIC int
s3_get_chipset_stepping_number(struct s3_screen_state *screen_state_p,
			struct s3_options_structure *s3_options_p)
{
	unsigned char chip_id_register_value;
	int	status = 0;
	enum s3_chipset_stepping_kind detected_chipset_step = 
		S3_STEP_KIND_INVALID_STEP;

	S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CHIP_ID_INDEX,
		chip_id_register_value);

	if (screen_state_p->chipset_kind == 
		S3_CHIPSET_KIND_S3_CHIPSET_86C801)
	{
		switch (chip_id_register_value & 0x0F)
		{
			case 0: /* a/b step */
				detected_chipset_step = S3_STEP_KIND_B_STEP;
				break;
			case 2: /* C Step HP - Whatever that Means - Memory? */
			case 3: /* C Step Toshiba  */
			case 4: /* C Step NEC  */
				detected_chipset_step = S3_STEP_KIND_C_STEP;
				break;
			case 5: /* D Step HP */
				detected_chipset_step = S3_STEP_KIND_D_STEP;
				break;
			default:
				detected_chipset_step = S3_STEP_KIND_INVALID_STEP;
#ifdef DELETE
				status |= S3_INITIALIZE_CHIPSET_STEP_DETECTION_FAILED;
#endif
				break;
		}
	}
	else  if (screen_state_p->chipset_kind == 
		S3_CHIPSET_KIND_S3_CHIPSET_86C928)
	{
		switch (chip_id_register_value & 0x0F)
		{
			case 0: /* Rev A-D HP */
			case 1:
				detected_chipset_step = S3_STEP_KIND_D_STEP;
				break;
			case 4: /* Rev E NEC */
				detected_chipset_step = S3_STEP_KIND_E_STEP;
				break;
			case 5: /* Rev G HP */
			case 6: /* Rev G HP */
				detected_chipset_step = S3_STEP_KIND_G_STEP;
				break;
			default:
				detected_chipset_step = S3_STEP_KIND_INVALID_STEP;
#ifdef DELETE
				status |= S3_INITIALIZE_CHIPSET_STEP_DETECTION_FAILED;
#endif
				break;
		}
	}
	else  if (screen_state_p->chipset_kind == 
			S3_CHIPSET_KIND_S3_CHIPSET_86C928_PCI)
	{
		switch (chip_id_register_value & 0x0F)
		{
			/* 
			 * No stepping information available for PCI. 
			 * Hence maintain that 0 is A step. 
			 */
			case 0: 
				detected_chipset_step = S3_STEP_KIND_A_STEP;
				break;
			default:
				detected_chipset_step = S3_STEP_KIND_INVALID_STEP;
				break;
		}
	}
	if ( s3_options_p->stepping_number == 
			S3_OPTIONS_STEPPING_NUMBER_AUTO_DETECT) 
	{
		screen_state_p->chipset_step = detected_chipset_step;
	}
	else if (s3_options_p->stepping_number == 
			S3_OPTIONS_STEPPING_NUMBER_B_STEP)
	{
		screen_state_p->chipset_step = S3_STEP_KIND_B_STEP;
	}
	else if (s3_options_p->stepping_number == 
			S3_OPTIONS_STEPPING_NUMBER_C_STEP)
	{
		screen_state_p->chipset_step = S3_STEP_KIND_C_STEP;
	}
	else if (s3_options_p->stepping_number == 
			S3_OPTIONS_STEPPING_NUMBER_D_STEP)
	{
		screen_state_p->chipset_step = S3_STEP_KIND_D_STEP;
	}
	else if (s3_options_p->stepping_number == 
			S3_OPTIONS_STEPPING_NUMBER_E_STEP)
	{
		screen_state_p->chipset_step = S3_STEP_KIND_E_STEP;
	}
	else if (s3_options_p->stepping_number == 
			S3_OPTIONS_STEPPING_NUMBER_G_STEP)
	{
		screen_state_p->chipset_step = S3_STEP_KIND_G_STEP;
	}
	else
	{
		screen_state_p->chipset_step = S3_STEP_KIND_INVALID_STEP;
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}
#if (defined (__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s3_get_chipset_stepping_number) {\n"
			"\tDetected S3 chipset with <%s>\n"
			"}\n",
			s3_step_kind_to_step_kind_dump[screen_state_p->chipset_step]); 
	}
#endif

	if (screen_state_p->chipset_step == S3_STEP_KIND_INVALID_STEP)
	{
		status |= S3_INITIALIZE_CHIPSET_STEP_DETECTION_FAILED;
	}
	/* 
	 * Unknown chipset step is an error. No warnings. 
	 */
	if ((screen_state_p->chipset_step != detected_chipset_step) &&
		(screen_state_p->chipset_step != S3_STEP_KIND_INVALID_STEP))
	{
		(void) fprintf(stderr, S3_CHIPSET_STEP_OVERRIDE_MESSAGE,
		s3_step_kind_to_step_kind_dump[detected_chipset_step],
		s3_step_kind_to_step_kind_dump[screen_state_p->chipset_step]);
	}

#if (defined(__DEBUG__))
	if (screen_state_p->chipset_step == S3_STEP_KIND_INVALID_STEP)
	{
		if (s3_debug)
		{
			(void) fprintf(debug_stream_p, 
				"(s3_get_chipset_stepping_number)"
				"{\n\tUnknown stepping number.\n\t}\n");
		}
	}
#endif /* __DEBUG__ */
	return status;
}
/*
 * Get the bus on which the s3 card is sitting by reading the 
 * configuration register 1 (CR36).
 */
STATIC int
s3_get_bus_kind( struct s3_screen_state *screen_state_p, 
			struct s3_options_structure *s3_options_p) 
{
	unsigned char 	config_register_1;
	int				status = 0;

	S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CNFG_REG1_INDEX,
		config_register_1);

	switch  (config_register_1 & 0x03)
	{
		case 0:
			screen_state_p->bus_kind = S3_BUS_KIND_EISA;
			break;
		case 1:
			screen_state_p->bus_kind = S3_BUS_KIND_LOCAL_386DX_486;
			break;
		case 3:
			screen_state_p->bus_kind = S3_BUS_KIND_ISA;
			break;
		default:
			screen_state_p->bus_kind = S3_BUS_KIND_UNKNOWN_BUS;
#ifdef DELETE
			status |= S3_INITIALIZE_BUS_KIND_DETECTION_FAILED;
#endif
			break;
	}
	/*
	 * No way to detect this bus. But chipset kind is 928 PCI. Hence
	 * assume pci bus.
	 */
	if (screen_state_p->chipset_kind == S3_CHIPSET_KIND_S3_CHIPSET_86C928_PCI)
	{
		screen_state_p->bus_kind = S3_BUS_KIND_PCI;
	}

	if ( screen_state_p->bus_kind == S3_BUS_KIND_UNKNOWN_BUS)
	{
			status |= S3_INITIALIZE_BUS_KIND_DETECTION_FAILED;
	}

	if (s3_options_p->s3_bus_width == S3_OPTIONS_S3_BUS_WIDTH_8_BIT)
	{
		screen_state_p->bus_width = S3_BUS_WIDTH_8;
		screen_state_p->pixtrans_width = 8;
	}
	else if (s3_options_p->s3_bus_width == S3_OPTIONS_S3_BUS_WIDTH_16_BIT)
	{
		screen_state_p->bus_width = S3_BUS_WIDTH_16;
		screen_state_p->pixtrans_width = 16;
	}
	else
	{
		screen_state_p->bus_width = S3_BUS_WIDTH_UNKNOWN;
		status |= S3_INITIALIZE_BOGUS_BUS_WIDTH;
	}
		
#if (defined(__DEBUG__))
		if (s3_debug)
		{
			(void) fprintf(debug_stream_p, 
				"(s3_get_bus_kind) {\n"
				"\tbus_kind = %s\n"
				"\tbus_width = %s\n"
				"}\n",
				s3_bus_kind_to_bus_kind_dump[screen_state_p->bus_kind],
				s3_bus_width_to_bus_width_dump[screen_state_p->bus_width]); 
		}
#endif
	return (status);
}
/*
 * Get the amount of video memory installed on the card by reading
 * configuration register 1 (CR36).
 */
STATIC int
s3_get_video_memory_size( struct s3_screen_state *screen_state_p, 
			struct s3_options_structure *s3_options_p, SIConfigP config_p)
{
	unsigned char 	config_register_1;
	int				status = 0;

	S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CNFG_REG1_INDEX,
		config_register_1);

	switch ( config_register_1 & 0xE0 )
	{
		case CONFIG_1_DISPLAY_MEMORY_SIZE_512:
			screen_state_p->video_memory_size = 512*1024;
			break;
		case CONFIG_1_DISPLAY_MEMORY_SIZE_1024:
			screen_state_p->video_memory_size = 1024*1024;
			break;
		case CONFIG_1_DISPLAY_MEMORY_SIZE_2048:
			screen_state_p->video_memory_size = 2*1024*1024;
			break;
		case CONFIG_1_DISPLAY_MEMORY_SIZE_3072:
			if ((screen_state_p->chipset_kind == 
					S3_CHIPSET_KIND_S3_CHIPSET_86C928) ||
				(screen_state_p->chipset_kind == 
					S3_CHIPSET_KIND_S3_CHIPSET_86C928_PCI))
			{
				screen_state_p->video_memory_size = 3*1024*1024;
			}
			else
			{
				status |= S3_INITIALIZE_DETECT_MEMORY_SIZE_FAILED;
			}
			break;
		case CONFIG_1_DISPLAY_MEMORY_SIZE_4096:
			if ((screen_state_p->chipset_kind == 
					S3_CHIPSET_KIND_S3_CHIPSET_86C928) ||
				(screen_state_p->chipset_kind == 
					S3_CHIPSET_KIND_S3_CHIPSET_86C928_PCI))
			{
				screen_state_p->video_memory_size = 4*1024*1024;
			}
			else
			{
				status |= S3_INITIALIZE_DETECT_MEMORY_SIZE_FAILED;
			}
			break;
		default:
			/* CONSTANTCONDITION */
			ASSERT(0);
			status |= S3_INITIALIZE_DETECT_MEMORY_SIZE_FAILED;
			break;
	}

	if ((screen_state_p->video_memory_size >> 10) != config_p->videoRam )
	{
		(void) fprintf(stderr,S3_MEMORY_SIZE_OVERRIDE_MESSAGE,
			screen_state_p->video_memory_size >> 10,
			config_p->videoRam);
		screen_state_p->video_memory_size = config_p->videoRam << 10U;
		is_memory_size_mismatch = TRUE;
		/* Allow override even if memory cannot be detected. PCI problem */
		status = 0;
	}
	return(status);
}
/* 
 * This function checks if the model specified by the config file is
 * supported by the display library. Returns 0 if supported.
 */
STATIC int
s3_check_support_for_video_board(struct s3_screen_state *screen_state_p,
	struct s3_options_structure *s3_options_p, char *modelname)
{
	int	status = 0;
	int	i ;

	/*
	 * If both clock and dac information are absent and if options for
	 * clock and dac are absent then check against the supported models.
	 * In case of no match return failure.
	 */
	if ((screen_state_p->clock_chip_p != NULL) &&
		(screen_state_p->dac_kind != S3_DAC_NULL))
	{
		return(status);
	}

	for ( i = 0; i < NUMBER_OF_SUPPORTED_VIDEO_CARDS; i++)
	{
#if (defined(__DEBUG__))
	if(s3_debug)
	{
		(void) fprintf(debug_stream_p,"(s3_initialize_display_library){\n"
			"Checking model %s from vendor %s\n}\n",
			s3_supported_video_cards_table[i].model_name,
			s3_supported_video_cards_table[i].vendor_name);
	}
#endif
		if(strcmp(modelname,s3_supported_video_cards_table[i].model_name) 
			== 0)
		{
			screen_state_p->clock_chip_p = &(s3_clock_chip_table[
				s3_supported_video_cards_table[i].clock_kind]);
			screen_state_p->dac_kind = 
				s3_supported_video_cards_table[i].dac_kind;
			return(status);
		}
	}

	/*
	 * Check if a combination of vendor level code and options 
	 * specify clock chip and dac. Otherwise signal an error.
	 */
	if (i == NUMBER_OF_SUPPORTED_VIDEO_CARDS)
	{
		if (((s3_options_p->clock_chip_name 
			== S3_OPTIONS_CLOCK_CHIP_NAME_UNKNOWN) &&
			(screen_state_p->clock_chip_p == NULL)) ||
			((s3_options_p->dac_name 
			== S3_OPTIONS_DAC_NAME_UNKNOWN_DAC) &&
			(screen_state_p->dac_kind == S3_DAC_NULL)))
		{
			status |= S3_INITIALIZE_UNSUPPORTED_VIDEO_BOARD;
		}
	}
	return(status);
}
/* 
 * s3_initialize_display_library
 * 
 * Initialize the S3 chipset specific SDD code.  This code will parse the
 * user specified options and configure its behaviour accordingly.  
 * There is no way for the chipset level code to determine type of dac 
 * on a S3 card. Hence it is wholly the responsibility of the board 
 * level code to do this. This function will also save the chipset
 * state that would have to be restored finally.
 */
function SIBool
s3_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
								SIScreenRec *si_screen_p, 
								struct s3_screen_state *screen_state_p)
{
	SIConfigP config_p;
	
	unsigned int initialization_status = 0;
	SIFunctions saved_generic_functions;
	
	struct s3_options_structure *s3_options_p = NULL;
	char s3_options_string[DEFAULT_S3_STRING_OPTION_SIZE];
	struct stat stat_buffer;
	int fd;

#if (defined(__DEBUG__))
	extern void s3_debug_control(boolean);
	
	s3_debug_control(TRUE);
#endif /* __DEBUG__ */

	config_p = si_screen_p->cfgPtr;
	
#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p, "(s3)\t"
				"s3_initialize_display_library called.\n");
	}
#endif /* __DEBUG__ */
	
	ASSERT(screen_state_p != NULL);
	ASSERT(si_screen_p != NULL);

	(void) fprintf(stderr, DEFAULT_S3_STARTUP_MESSAGE);


	/*
	 * Initialize the generic data structure.
	 */
	initialization_status = 
		generic_initialize_display_library(virtual_terminal_file_descriptor,
		   si_screen_p, (struct generic_screen_state *) screen_state_p);
	
	if (initialization_status)
	{
#if (defined(__DEBUG__))
		if (s3_debug)
		{
			(void) fprintf(stderr, 
			"(s3_initialize_display_library)\tgeneric initialization "
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
	 * User option Handling. Null terminate the buffer first.
	 */
	s3_options_string[0] = '\0';

	/*
	 * First check if the standard options file is present.
	 */
	if (!stat(DEFAULT_S3_STANDARD_OPTION_FILE_NAME,&stat_buffer))
	{
		(void) strcpy(s3_options_string,
			DEFAULT_S3_PARSE_STANDARD_OPTIONS_FILE_COMMAND);
	}

	/*
	 * Add parsing options for the user specified option string.
	 */
	(void) strcat(s3_options_string, config_p->info);
	(void) strcat(s3_options_string, " ");

	/*
	 * Add parsing options for the standard environment option variables.
	 */
	(void) strcat(s3_options_string,
		DEFAULT_S3_PARSE_STANDARD_ENVIRONMENT_VARIABLES_COMMAND);

	/* 
	 * Parse user configurable options.
	 */
	if (!(s3_options_p  = 
		s3_options_parse(NULL, s3_options_string)))
	{
		
#if (defined(__DEBUG__))
		if (s3_debug)
		{
			(void) fprintf(stderr, "(s3)\tparsing of user options "
					"failed.\n"); 
		}
#endif /* __DEBUG__ */
		
		initialization_status |= S3_INITIALIZE_PARSE_USER_OPTIONS_FAILED;

		return (initialization_status);
	}
	else
	{
		/*
		 * Save the options for later persual by code.
		 */
		screen_state_p->options_p = s3_options_p;
	}
	

	/*
	 * Determine if we can support this board. Check this based on
	 * the model name.
	 */
	initialization_status |= s3_check_support_for_video_board(
						screen_state_p,	s3_options_p, config_p->model);
	if (initialization_status)
	{
		return(initialization_status);
	}

	/*
	 * Set the SDD version number as requested.
	 */
	if (s3_options_p->si_interface_version !=
		S3_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE)
	{
		switch (s3_options_p->si_interface_version)
		{
		case S3_OPTIONS_SI_INTERFACE_VERSION_1_0:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version =
					DM_SI_VERSION_1_0;
			break;
		case S3_OPTIONS_SI_INTERFACE_VERSION_1_1:
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
 	s3_dac_access_delay_count = s3_options_p->dac_access_delay_count;

	/*
	 * Set the loop timeout value.
	 */
	s3_graphics_engine_loop_timeout_count =
		s3_options_p->graphics_engine_loop_timeout_count;

	ASSERT(s3_graphics_engine_loop_timeout_count > 0);
	
	/*
	 * Set the micro delay count.
	 */
	s3_graphics_engine_micro_delay_count =
		s3_options_p->graphics_engine_micro_delay_count;

	ASSERT(s3_graphics_engine_micro_delay_count > 0);

	s3_crtc_sync_loop_timeout_count = 
		s3_options_p->crtc_sync_timeout_count;

	ASSERT(s3_crtc_sync_loop_timeout_count > 0 );

	s3_graphics_engine_fifo_blocking_factor =
		s3_options_p->graphics_engine_fifo_blocking_factor;
	
	ASSERT(s3_graphics_engine_fifo_blocking_factor > 0);

#if (defined(USE_KD_GRAPHICS_MODE))
	if (!s3_set_virtual_terminal_mode(virtual_terminal_file_descriptor,
		KD_GRAPHICS))
	{
		initialization_status |= S3_UNABLE_TO_SWITCH_CONSOLE_DRIVER_MODE;
		return (initialization_status);
	}
#endif

#if (defined(USE_SYSI86))

	/*
	 * The X server needs access to many I/O addresses.  We use the
	 * `sysi86()' call to get access to these.  Another alternative to
	 * this approach would be to have a kernel driver which
	 * allows the X server process access to the ATI registers.
	 */
	if (SET_IOPL(PS_IOPL) < 0)
	{
		perror(S3_CANNOT_ENABLE_REGISTER_ACCESS_MESSAGE);
		free_memory(s3_options_p);
		return (SI_FAIL);
	}
#endif /* USE_SYSI86 */

	/*
	 * Figure out the current addr base for the crtc register set.
	 */
	if (inb(VGA_GENERAL_REGISTER_MISC_RD) & 0x01)
	{
		/*
		 * Color.
		 */
		 screen_state_p->vga_crtc_address = 
			VGA_CRTC_REGISTER_CRTC_ADR_COLOR;
		 screen_state_p->vga_crtc_data =
			VGA_CRTC_REGISTER_CRTC_DATA_COLOR;
		 screen_state_p->vga_input_status_address =
			VGA_GENERAL_REGISTER_STATUS_1_COLOR;
	}
	else
	{
		/*
		 * Monochrome.
		 */
		 screen_state_p->vga_crtc_address =
			VGA_CRTC_REGISTER_CRTC_ADR_MONO;
		 screen_state_p->vga_crtc_data =  
			VGA_CRTC_REGISTER_CRTC_DATA_MONO;
		 screen_state_p->vga_input_status_address =
			VGA_GENERAL_REGISTER_STATUS_1_MONO;
	}

	/*
	 * Allow access to the S3 VGA and S3 system registers.
	 */
	S3_UNLOCK_CRT_TIMING_REGISTERS();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();

	/*
	 * IMPORTANT: Call these functions in the same order.
	 * Chipset kind, chipset step, bus kind, video memory size.
	 */
	if ( ! screen_state_p->chipset_kind )
	{
		initialization_status |= s3_get_chipset_kind( screen_state_p, 
									s3_options_p);
		if (initialization_status)
		{
			return(initialization_status);
		}
	}

	/*
	 * Get the stepping number.
	 */
	if ( ! screen_state_p->chipset_step )
	{
		initialization_status |= s3_get_chipset_stepping_number(
									screen_state_p, s3_options_p);
		if (initialization_status)
		{
			return(initialization_status);
		}
	}

	/*
	 * Get the bus kind.
	 */
	if (!screen_state_p->bus_kind)
	{
		initialization_status |= s3_get_bus_kind(screen_state_p, 
										s3_options_p);
		if (initialization_status)
		{
			return(initialization_status);
		}
	}

	/*
	 * Get the size of the installed memory on the board.
	 */
	if ( ! screen_state_p->video_memory_size )
	{
		initialization_status |= s3_get_video_memory_size(
							screen_state_p, s3_options_p,config_p);
		if (initialization_status)
		{
			return(initialization_status);
		}
	}


	/*
	 * Determine the dac kind. There is no way to determine the dac kind
	 * except by looking into the board. See file s3_dacs.def for list
	 * of dacs supported by this chipset. If dac_kind is non zero then the
	 * vendor level code has initialized it.
	 */
	switch(s3_options_p->dac_name)
	{
		case S3_OPTIONS_DAC_NAME_ATT20C491:
			screen_state_p->dac_kind = S3_DAC_ATT20C491;
		break;
		case S3_OPTIONS_DAC_NAME_SC15025:
			screen_state_p->dac_kind = S3_DAC_SC15025;
		break;
		case S3_OPTIONS_DAC_NAME_SC11481_6_2_3:
			screen_state_p->dac_kind = S3_DAC_SC11481_486_482_483;
		break;
		case S3_OPTIONS_DAC_NAME_SC11485_7:
			screen_state_p->dac_kind = S3_DAC_SC11485_487;
		break;
		case S3_OPTIONS_DAC_NAME_SC11484_8:
			screen_state_p->dac_kind = S3_DAC_SC11484_488;
		break;
		case S3_OPTIONS_DAC_NAME_SC11489:
			screen_state_p->dac_kind = S3_DAC_SC11489;
		break;
		case S3_OPTIONS_DAC_NAME_SC11471_6:
			screen_state_p->dac_kind = S3_DAC_SC11471_476;
		break;
		case S3_OPTIONS_DAC_NAME_SC11478:
			screen_state_p->dac_kind = S3_DAC_SC11478;
		break;
		case S3_OPTIONS_DAC_NAME_IMSG171:
			screen_state_p->dac_kind = S3_DAC_IMSG171;
		break;
		case S3_OPTIONS_DAC_NAME_IMSG176:
			screen_state_p->dac_kind = S3_DAC_IMSG176;
		break;
		case S3_OPTIONS_DAC_NAME_IMSG178:
			screen_state_p->dac_kind = S3_DAC_IMSG178;
		break;
		case S3_OPTIONS_DAC_NAME_BT471_6:
			screen_state_p->dac_kind = S3_DAC_BT471_476;
		break;
		case S3_OPTIONS_DAC_NAME_BT478:
			screen_state_p->dac_kind = S3_DAC_BT478;
		break;
		case S3_OPTIONS_DAC_NAME_BT485KPJ110:
			screen_state_p->dac_kind = S3_DAC_BT485;
		break;
		case S3_OPTIONS_DAC_NAME_BT485KPJ135:
			screen_state_p->dac_kind = S3_DAC_BT485;

			s3_dac_kind_to_max_clock_table[screen_state_p->dac_kind] = 
															135000 ;
		break;
		case S3_OPTIONS_DAC_NAME_W82C478:
			screen_state_p->dac_kind = S3_DAC_W82C478;
		break;
		case S3_OPTIONS_DAC_NAME_W82C476:
			screen_state_p->dac_kind = S3_DAC_W82C476;
		break;
		case S3_OPTIONS_DAC_NAME_W82C490:
			screen_state_p->dac_kind = S3_DAC_W82C490;
		break;
		case S3_OPTIONS_DAC_NAME_TR9C1710:
			screen_state_p->dac_kind = S3_DAC_TR9C1710;
		break;
		case S3_OPTIONS_DAC_NAME_ATT20C490:
			screen_state_p->dac_kind = S3_DAC_ATT20C490;
		break;
		case S3_OPTIONS_DAC_NAME_SS2410:
			screen_state_p->dac_kind = S3_DAC_SS2410;
		break;
		case S3_OPTIONS_DAC_NAME_TI3020:
			screen_state_p->dac_kind = S3_DAC_TI3020;
		break;
		case S3_OPTIONS_DAC_NAME_TI3025:
			screen_state_p->dac_kind = S3_DAC_TI3025;
		break;
		default:
		  if (screen_state_p->dac_kind == NULL)
		  {
			  initialization_status |=
					S3_INITIALIZE_DAC_NOT_RECOGNIZED;
			  return (initialization_status);
		  }
		break;
	}
	ASSERT(screen_state_p->dac_kind >= S3_DAC_ATT20C491 && 
		screen_state_p->dac_kind < S3_DAC_UNKNOWN);


	/*
	 * Check if overriding of dac max frequency is required.
	 */
	if (s3_options_p->dac_max_frequency)
	{
		(void) fprintf(stderr,
		   S3_OVERRIDE_DAC_MAX_FREQUENCY_MESSAGE,
		   s3_dac_kind_to_max_clock_table[screen_state_p->dac_kind] / 1000,
		   s3_dac_kind_to_max_clock_table[screen_state_p->dac_kind] % 1000,
		   s3_options_p->dac_max_frequency / 1000,
		   s3_options_p->dac_max_frequency % 1000);

		   s3_dac_kind_to_max_clock_table[screen_state_p->dac_kind] = 
				s3_options_p->dac_max_frequency;
	}

	/*
	 * update the current state of the chipset.
	 */
	screen_state_p->register_state.current_chipset_state_kind = 
							CHIPSET_REGISTERS_NOT_IN_XSERVER_MODE;
	 
	/*
	 * Save the register state so that we could restore state 
	 * before relinquishing the virtual terminal.
	 */
	s3_save_original_vga_state(screen_state_p);
	
	/*
	 * The time has come to fill in state. Call the function which would
	 * initialize the register state with reasonable defaults.
	 */
	initialization_status |= s3_set_registers(config_p, s3_options_p, 
								screen_state_p);
	if ( initialization_status)
	{
		return(initialization_status);
	}

	/* 
	 * Determine the type of framebuffer/memory access the user has
	 * requested. Also check if the user wants to use alternate 
	 * io port addresses. By default use io/access for memory and
	 * enhanced registers.
	 */
	if (s3_options_p->enable_alternate_ioport_address == 
		S3_OPTIONS_ENABLE_ALTERNATE_IOPORT_ADDRESS_YES)
	{
		screen_state_p->enhanced_register_io_address_xor_value =  
			S3_ALTERNATE_IO_PORT_XOR_VALUE;
	}

	/*
	 * Get the types of register/memory access on the S3 card.
	 * enable only io writes by default,since this can coexist with any
	 * type of operation. mmio will be enabled as and when required.
	 */
	screen_state_p->use_mmio = 0;	
	screen_state_p->memory_register_access_mode |= S3_IO_ACCESS;

	if (s3_options_p->memory_and_register_access_mode & 
		S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_MMIO_READ_ENHANCED_REGS)
	{
		screen_state_p->memory_register_access_mode |=
 			S3_MMIO_READ_ENHANCED_REGS;
	}
	if (s3_options_p->memory_and_register_access_mode & 
		S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_MMIO_WRITE_ENHANCED_REGS)
	{
		screen_state_p->memory_register_access_mode |=
 			S3_MMIO_WRITE_ENHANCED_REGS;
	}
	if (s3_options_p->memory_and_register_access_mode & 
		S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS)
	{
		screen_state_p->memory_register_access_mode |=
 			S3_MMIO_READ_PIXTRANS;
	}
	if (s3_options_p->memory_and_register_access_mode & 
		S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS)
	{
		screen_state_p->memory_register_access_mode |=
 			S3_MMIO_WRITE_PIXTRANS;
	}
	if (s3_options_p->memory_and_register_access_mode & 
		S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_LFB_ACCESS)
	{
		screen_state_p->memory_register_access_mode |=
 			S3_LFB_ACCESS;
	}

	/*
	 * For any type of memory mapped io operation we need to mmap the
	 * vga framebuffer area starting from physical 0xa0000 to a length
	 * of 64KB
	 */
	if (S3_MEMORY_REGISTER_ACCESS_MODE_ANY_MMIO_OPERATION(screen_state_p))
	{
		if ((fd = open("/dev/pmem", O_RDWR)) < 0)
		{
#if (defined(__DEBUG__))
			if(s3_debug)
			{
				(void) fprintf(debug_stream_p,
					"(s3_initialize_display_library){\n"
					"cannot open /dev/pmem.\n"
					"}\n");
			}
#endif
			initialization_status |= 
				S3_INITIALIZE_UNABLE_TO_MAP_DISPLAY_MEMORY;
		}
		else
		{
			screen_state_p->mmio_base_address = (void *)mmap
				((caddr_t)0,S3_MMIO_WINDOW_SIZE_IN_BYTES,
				PROT_READ|PROT_WRITE, MAP_SHARED, fd, 
				(off_t)S3_MMIO_WINDOW_BASE_PHYSICAL_ADDRESS);
			(void) close(fd);
			if ((long)screen_state_p->mmio_base_address == -1)
			{
#if (defined(__DEBUG__))
				if(s3_debug)
				{
					(void) fprintf(debug_stream_p,
						"(s3_initialize_display_library){\n"
						"\tmmap call failed.\n" 
						"\tBase = 0x%x\n" 
						"\tSize = 0x%x\n"
						"}\n",
						S3_MMIO_WINDOW_BASE_PHYSICAL_ADDRESS,
						S3_MMIO_WINDOW_SIZE_IN_BYTES);
				}
#endif
				initialization_status |= 
					S3_INITIALIZE_UNABLE_TO_MAP_DISPLAY_MEMORY;
			}
		}
	}

	if ( initialization_status)
	{
		return(initialization_status);
	}

	/*
	 * How many lines of the screen does the user want saved?
	 * Limit this to the max video memory lines available.
	 */
	screen_state_p->vt_switch_save_lines =
		((s3_options_p->vt_switch_save_lines >
		  screen_state_p->generic_state.screen_physical_height) ? 
		 screen_state_p->generic_state.screen_physical_height : 
		 s3_options_p->vt_switch_save_lines);


#if (defined(__DEBUG__))
	if (s3_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s3)\tvt save size = %d.\n",
			screen_state_p->vt_switch_save_lines);

	}
#endif
	
	si_screen_p->funcsPtr->si_vt_save = s3_virtual_terminal_save;
	si_screen_p->funcsPtr->si_vt_restore = s3_virtual_terminal_restore;
	si_screen_p->funcsPtr->si_restore = s3_restore_screen;
	si_screen_p->funcsPtr->si_vb_onoff = s3_screen_saver;
	
	/*
	 * Initialize the off-screen memory manager.
     * CAVEATS: all the parameters passed to omm_initialize can be
     * changed/destroyed after the call.
     */
    {
        struct omm_initialization_options omm_options;
        /*
         * Create the initialization string for the off-screen memory
         * manager if required.
         */
        char *cursor_named_allocation_string_p =
            s3_cursor_make_named_allocation_string(si_screen_p, s3_options_p);

        char *buffer_p;

        /*
         * Add one for the comma which will seperate each named allocate
         * string
         */
        buffer_p = (char *)allocate_memory(
                DEFAULT_S3_OMM_INITIALIZATION_STRING_LENGTH + 1 +
                strlen(cursor_named_allocation_string_p) + 1);

        sprintf(buffer_p, DEFAULT_S3_OMM_INITIALIZATION_FORMAT ",%s",
                DEFAULT_S3_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
                screen_state_p->generic_state.screen_virtual_width,
                screen_state_p->generic_state.screen_virtual_height,
                screen_state_p->generic_state.screen_depth,
                0,0,0,cursor_named_allocation_string_p);

        omm_options.total_width =
         screen_state_p->generic_state.screen_physical_width;
        omm_options.total_height =
         screen_state_p->generic_state.screen_physical_height;
        omm_options.total_depth =
         screen_state_p->generic_state.screen_depth;
        omm_options.horizontal_constraint =
                (s3_options_p->omm_horizontal_constraint ?
                 s3_options_p->omm_horizontal_constraint :
                 screen_state_p->pixtrans_width);
        omm_options.vertical_constraint =
               (s3_options_p->omm_vertical_constraint ?
                s3_options_p->omm_vertical_constraint :
				DEFAULT_S3_OMM_VERTICAL_CONSTRAINT);

        omm_options.neighbour_list_increment =
            s3_options_p->omm_neighbour_list_increment;
        omm_options.full_coalesce_watermark =
            s3_options_p->omm_full_coalesce_watermark;
        omm_options.hash_list_size =
            s3_options_p->omm_hash_list_size;
        omm_options.named_allocations_p = buffer_p;

        (void) omm_initialize(&omm_options);

		free_memory( buffer_p );
    }
	/*
	 * Let each module initialize itself.
	 */
	s3__initialize__(si_screen_p, s3_options_p);


	if(screen_state_p->generic_state.screen_current_graphics_state_p)
	{
		int count;

		for(count = 0; count <
			screen_state_p->generic_state.screen_number_of_graphics_states; 
			count ++)
		{

			struct s3_graphics_state *graphics_state_p = 
				(struct s3_graphics_state *)
					screen_state_p->generic_state.
						screen_graphics_state_list_pp[count];
					   
			ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
									 (struct generic_graphics_state *)
									 graphics_state_p));
	
			ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
									 graphics_state_p));

			/*
			 * Save the generic function table
			 */
			graphics_state_p->generic_si_functions =
				saved_generic_functions;
		}

	}

	/*
	 * panning message.
	 */
	if (screen_state_p->is_panning_to_virtual_screen_feasible == TRUE)
	{
		(void) fprintf(stderr, 
			S3_INITIALIZE_ENABLING_PANNING_TO_VIRTUAL_DIMENSIONS_MESSAGE ,
			config_p->virt_w, config_p->virt_h);
	}
	/*
	 * Verbose startup options
	 */
	if (s3_options_p->verbose_startup)
	{
		(void) fprintf(stderr,
		   S3_VERBOSE_STARTUP_PROLOGUE);
		
#ifdef DELETE
		(void) fprintf(stderr, DEFAULT_S3_STARTUP_MESSAGE);
#endif

		(void) fprintf(stderr,
		   S3_VERBOSE_STARTUP_SDD_PARAMETERS_MESSAGE,
		   screen_state_p->generic_state.
			   screen_server_version_number,
		   screen_state_p->generic_state.
			   screen_sdd_version_number,
		   ((s3_options_p->si_interface_version ==
				S3_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE) ? 
				S3_VERBOSE_STARTUP_AUTO_CONFIGURED :
				S3_VERBOSE_STARTUP_USER_SPECIFIED)); 

		(void) fprintf(stderr,
			S3_VERBOSE_STARTUP_BOARD_CONFIGURATION_MESSAGE,
			s3_chipset_kind_to_chipset_kind_description[screen_state_p->
				chipset_kind],
			((s3_options_p->chipset_name ==
				S3_OPTIONS_CHIPSET_NAME_AUTO_DETECT) ? 
				S3_VERBOSE_STARTUP_AUTO_DETECTED :
				S3_VERBOSE_STARTUP_USER_SPECIFIED), 
			s3_step_kind_to_step_kind_description[screen_state_p->
				chipset_step],
			((s3_options_p->stepping_number ==
				S3_OPTIONS_STEPPING_NUMBER_AUTO_DETECT) ? 
				S3_VERBOSE_STARTUP_AUTO_DETECTED :
				S3_VERBOSE_STARTUP_USER_SPECIFIED), 
			s3_dac_kind_to_dac_description[screen_state_p->dac_kind],
			s3_bus_kind_to_bus_kind_description[screen_state_p->bus_kind],
			s3_bus_width_to_bus_width_description[screen_state_p->bus_width],
			s3_clock_chip_kind_description[screen_state_p->clock_chip_p->
				clock_chip_kind],
			screen_state_p->video_memory_size / 1024);

		(void) fprintf(stderr,
			S3_VERBOSE_STARTUP_GRAPHICS_ENGINE_PARAMETERS_MESSAGE,
			s3_graphics_engine_micro_delay_count,
			s3_graphics_engine_loop_timeout_count);

		(void) fprintf(stderr,
		   S3_VERBOSE_STARTUP_SCREEN_PARAMETERS_MESSAGE,
		   screen_state_p->generic_state.screen_physical_width,
		   screen_state_p->generic_state.screen_physical_height,
		   screen_state_p->generic_state.screen_virtual_width,
		   screen_state_p->generic_state.screen_virtual_height,
		   screen_state_p->generic_state.screen_displayed_width,
		   screen_state_p->generic_state.screen_displayed_height,
		   screen_state_p->generic_state.screen_depth);

		(void) fprintf(stderr,
		   S3_VERBOSE_STARTUP_EPILOGUE);
		
	}

	config_p->IdentString = DEFAULT_S3_IDENT_STRING;
	return (initialization_status);
}

/*
 * Print a list of supported modes on stderr.
 */
STATIC void
s3_list_available_modes(const SIScreenRec *si_screen_p)
{
	struct s3_screen_state						*screen_state_p;
	const struct s3_display_mode_table_entry 	*entry_p ; 
	int	 										count;
	boolean	is_mux_dac_and_16_bit = FALSE;

	screen_state_p = (struct s3_screen_state *)si_screen_p->vendorPriv;

	if (screen_state_p->dac_kind == S3_DAC_BT485 )
	{
		is_mux_dac_and_16_bit = TRUE;
	}

	/*
	 * First print 4/8 bit modes.
	 */
	for(count = S3_DISPLAY_MODE_NULL + 1,
		entry_p =
		&(s3_display_mode_table[S3_DISPLAY_MODE_NULL + 1]);
		count < S3_DISPLAY_MODE_COUNT; 
		count ++, entry_p++)
	{
		boolean print_entry = FALSE;
		boolean print_mux_16_entry = FALSE;

		if(entry_p->installed_memory == screen_state_p->video_memory_size)
		{
			/*
			 * Check if the present clock chip can support this mode.
			 */
			if (screen_state_p->clock_chip_p->number_of_clock_frequencies)
			{
				int	i;
				for ( i = 0; i < 
					screen_state_p->clock_chip_p->number_of_clock_frequencies;
					i++)
				{
					if(screen_state_p->clock_chip_p->clock_frequency_table[i] 
						== entry_p->clock_frequency)
					{
						print_entry = TRUE;
						break;
					}
				}
			}
			else
			{
				/*
				 * Programmable clock.
				 */
				if (screen_state_p->clock_chip_p->clock_frequency_table[0] <= 
					entry_p->clock_frequency &&
					screen_state_p->clock_chip_p->clock_frequency_table[1] >= 
					entry_p->clock_frequency)
				{
					print_entry = TRUE;
				}
			}

			/* 
			 * Check if the ramdac can take this clock frequency.
			 */
			if (s3_dac_kind_to_max_clock_table[screen_state_p->dac_kind] <
				entry_p->clock_frequency)
			{
					print_entry = FALSE;
			}

			/*
			 * Entries for mux dacs. no 1280 modes for now.
			 */
			if (entry_p->display_depth == 8 && is_mux_dac_and_16_bit == TRUE &&
				print_entry == TRUE)
			{
				/* Minimum required in any configuration. */
				int memory_required_for_16_bit = 1024*1024;

				/*
				 * We need minimum 2MB for 1024x768 and 3MB for 1280x1024. 
				 * Check that here and return failure in case of problems.
				 */
				if(entry_p->display_width == 1024 && 
				   entry_p->display_height == 768) 
				{
					memory_required_for_16_bit = 2048*1024; 
				} 
				else if(entry_p->display_width == 1280 && 
				   entry_p->display_height == 1024) 
				{
					memory_required_for_16_bit = 3072*1024;
				}
				ASSERT(screen_state_p->dac_kind == S3_DAC_BT485);
				if (screen_state_p->video_memory_size >= 
				memory_required_for_16_bit && entry_p->display_width != 1280)
				{
					print_mux_16_entry = TRUE;
				}
			}

			/*
			 * Print only 4/8 bit modes for mux dacs.
			 */
			if (entry_p->display_depth > 8 && is_mux_dac_and_16_bit == TRUE)
			{
				print_entry  = FALSE;
			}

			/*
			 * Print the entry.
			 */
			if (print_entry == TRUE)
			{
				(void) fprintf(stderr, 
					"%4d KB VRAM\t%4d x %-4dx %-2d\t%d%s HZ\t%-10.15s\t\n",
					entry_p->installed_memory>>10,
					entry_p->display_width,
					entry_p->display_height,
					entry_p->display_depth,
					entry_p->display_refresh_rate,
					(entry_p->interlace_retrace_start?"i":" "),
					entry_p->monitor_name_p);
			}

			if (print_mux_16_entry == TRUE)
			{
				(void) fprintf(stderr, 
					"%4d KB VRAM\t%4d x %-4dx %-2d\t%d%s HZ\t%-10.15s\t\n",
					entry_p->installed_memory>>10,
					entry_p->display_width,
					entry_p->display_height,
					16,
					entry_p->display_refresh_rate,
					(entry_p->interlace_retrace_start?"i":" "),
					entry_p->monitor_name_p);
			}
		}
	}

	return;
}
/*
 *	s3_print_initialization_failure_message
 *
 *	Diagnose the cause for initialization failure from the
 * 	initialization status flag.
 *
 */
function void
s3_print_initialization_failure_message(
    const int status,
	const SIScreenRec *si_screen_p)
{
	struct s3_screen_state						*screen_state_p;

	screen_state_p = (struct s3_screen_state *)si_screen_p->vendorPriv;

	if (status & S3_INITIALIZE_S3_CHIPSET_DETECTION_FAILED)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_S3_CHIPSET_DETECTION_FAILED_MESSAGE);
	}
	 
	if (status & S3_INITIALIZE_CHIPSET_STEP_DETECTION_FAILED)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_CHIPSET_STEP_DETECTION_FAILED_MESSAGE);
	}
	 
	if (status & S3_INITIALIZE_BUS_KIND_DETECTION_FAILED)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_BUS_KIND_DETECTION_FAILED_MESSAGE);
	}
	 
	if (status & S3_INITIALIZE_DETECT_MEMORY_SIZE_FAILED)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_DETECT_MEMORY_SIZE_FAILED_MESSAGE);
	}
	 
	if (status & S3_INITIALIZE_PARSE_USER_OPTIONS_FAILED)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_PARSE_USER_OPTIONS_FAILED_MESSAGE);
	}
	 
	if (status & S3_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND_MESSAGE,
			(long)(screen_state_p->video_memory_size >> 10),
			si_screen_p->cfgPtr->disp_w,
			si_screen_p->cfgPtr->disp_h,
			si_screen_p->cfgPtr->depth,
			si_screen_p->cfgPtr->monitor_info.vfreq,
			si_screen_p->cfgPtr->monitor_info.model);
			
		s3_list_available_modes(si_screen_p);
	}

	if (status & S3_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND_MESSAGE);
	}

	if (status & S3_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED_MESSAGE);
	}

	if (status & S3_INITIALIZE_DAC_NOT_RECOGNIZED)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_DAC_NOT_RECOGNIZED_MESSAGE);
	}
	 
	if (status & S3_INITIALIZE_UNSUPPORTED_DEPTH)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_UNSUPPORTED_DEPTH_MESSAGE,
			si_screen_p->cfgPtr->depth);
		s3_list_available_modes(si_screen_p);
	}
	 
	if (status & S3_INITIALIZE_UNSUPPORTED_RESOLUTION)
	{
		(void) fprintf(stderr,
			S3_INITIALIZE_UNSUPPORTED_RESOLUTION_MESSAGE,
			si_screen_p->cfgPtr->virt_w,
			si_screen_p->cfgPtr->virt_h,
			si_screen_p->cfgPtr->depth);
		s3_list_available_modes(si_screen_p);
	}
	 
	if (status & S3_INITIALIZE_BOGUS_BUS_WIDTH)
	{
		(void) fprintf(stderr, S3_INITIALIZE_BOGUS_BUS_WIDTH_MESSAGE);
	}
	if (status & S3_INITIALIZE_UNABLE_TO_MAP_DISPLAY_MEMORY)
	{
		(void) fprintf(stderr, 
			S3_INITIALIZE_UNABLE_TO_MAP_DISPLAY_MEMORY_MESSAGE);
	}
	if (status & S3_INITIALIZE_UNSUPPORTED_MEMORY_SIZE)
	{
		(void) fprintf(stderr, 
			S3_INITIALIZE_UNSUPPORTED_MEMORY_SIZE_MESSAGE,
			si_screen_p->cfgPtr->videoRam);
	}
	if (status & S3_INITIALIZE_UNSUPPORTED_VIDEO_BOARD)
	{
		int i;

		(void) fprintf(stderr, S3_INITIALIZE_UNSUPPORTED_VIDEO_BOARD_MESSAGE,
			si_screen_p->cfgPtr->model);
		(void) fprintf(stderr, "\t%-20s\t%s\n","VENDOR","MODEL");
		for ( i = 0; i < NUMBER_OF_SUPPORTED_VIDEO_CARDS - 1; i++)
		{
			(void) fprintf(stderr, "\t%-20s\t%s\n",
				s3_supported_video_cards_table[i].vendor_name, 
				s3_supported_video_cards_table[i].model_name); 
				
		}
	}
	if (status & S3_INITIALIZE_MODE_NOT_SUPPORTED_BY_DAC)
	{
		s3_list_available_modes(si_screen_p);
	}
	if (status & S3_INITIALIZE_PANNING_TO_VIRTUAL_DIMENSIONS_INIT_FAILURE)
	{
		(void) fprintf(stderr, 
			S3_INITIALIZE_PANNING_TO_VIRTUAL_DIMENSIONS_INIT_FAILURE_MESSAGE,
			si_screen_p->cfgPtr->disp_w,
			si_screen_p->cfgPtr->disp_h,
			si_screen_p->cfgPtr->virt_w,
			si_screen_p->cfgPtr->virt_h);
	}
	return;
}

