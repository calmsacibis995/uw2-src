/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/mach.h	1.4"

#if (! defined(__MACH_INCLUDED__))

#define __MACH_INCLUDED__



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
extern boolean mach_debug ;
#endif


/*
 *	Current module state.
 */

#include "stdenv.h"

extern SIBool
mach_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
								SIScreenRec *si_screen_p, 
								struct mach_screen_state *screen_state_p)
;

extern void
mach_print_initialization_failure_message(
    const int status,
	const SIScreenRec *si_screen_p)
;


#endif
