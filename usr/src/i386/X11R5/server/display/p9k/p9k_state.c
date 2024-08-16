/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_state.c	1.2"
/***
 ***	NAME
 ***
 ***		p9k_state.c : chipset state for the Weitek p9000 chip.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9k_state.h"
 ***
 ***	DESCRIPTION
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
#include "g_state.h"

/***
 ***	Constants.
 ***/

#define	P9000_SCREEN_STATE_STAMP\
	(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + ('0' << 4) +\
	 ('_' << 5) + ('S' << 6) + ('C' << 7) + ('R' << 8) + ('E' << 9) +\
	 ('E' << 10) + ('N' << 11) + ('_' << 12) + ('S' << 13) + ('T' << 14) +\
	 ('A' << 15) + ('T' << 16) + ('E' << 17) + ('_' << 18) + ('S' << 19) +\
	 ('T' << 20) + ('A' << 21) + ('M' << 22) + ('P' << 23) + 0 )

/*
 * Minterm masks
 */

#define	P9000_FOREGROUND_COLOR_MASK			(0xFF00U)
#define	P9000_BACKGROUND_COLOR_MASK			(0xF0F0U)
#define	P9000_SOURCE_MASK					(0xCCCCU)
#define	P9000_DESTINATION_MASK				(0xAAAAU)

/*
 * Change handling.
 */

#define P9000_STATE_CHANGE_FOREGROUND_COLOR	(1 << 0U)
#define P9000_STATE_CHANGE_BACKGROUND_COLOR (1 << 1U)
#define P9000_STATE_CHANGE_PLANEMASK		(1 << 2U)

#ifdef DELETE

/*
 * @doc:enum drawing_operation_source:
 *
 * A drawing operation's source can come from 3 locations : the
 * foreground color register, the background color register and the
 * blit source.  The blit source can be the pattern registers, the
 * host CPU (used for pixel1 and pixel8 commands), or video memory
 * (used during the blit commands).
 *
 * @enddoc
 */

enum drawing_operation_source
{
	P9000_SOURCE_FOREGROUND_REGISTER,
	P9000_SOURCE_BACKGROUND_REGISTER,
	P9000_SOURCE_BLIT	/*CPU data, video memory data, pattern registers*/
};
#endif

/***
 ***	Macros.
 ***/

/*
 * Declare current register state
 */

#define P9000_CURRENT_REGISTER_STATE_DECLARE()		\
	struct p9000_register_state *register_state_p=	\
		screen_state_p->register_state_p

/*
 * Declare the current screen state.
 */

#define P9000_CURRENT_SCREEN_STATE_DECLARE()	\
	struct p9000_screen_state *screen_state_p =	\
		generic_screen_current_state_p

#define	P9000_MEMORY_BASE_DECLARE()					\
	volatile unsigned int * const p9000_base_p =	\
		 screen_state_p->base_p

#define	P9000_FRAMEBUFFER_DECLARE()							\
	volatile unsigned char * const p9000_framebuffer_p =	\
		 screen_state_p->framebuffer_p
	

#define	P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,graphics_state_p)	\
	((screen_state_p)->														\
	 fg_color_minterm_table_p[(graphics_state_p)->generic_state.			\
							  si_graphics_state.SGmode])

#define	P9000_STATE_CALCULATE_BG_MINTERM(screen_state_p,graphics_state_p)	\
	((screen_state_p)->														\
	 bg_color_minterm_table_p[(graphics_state_p)->generic_state.			\
							  si_graphics_state.SGmode])

#define	P9000_STATE_CALCULATE_BLIT_MINTERM(screen_state_p,graphics_state_p)	\
	((screen_state_p)->														\
	 blit_minterm_table_p[(graphics_state_p)->generic_state.				\
						  si_graphics_state.SGmode])

#define	P9000_STATE_SET_RASTER(register_state_p, raster_value)				 \
	{																		 \
		if ((register_state_p)->raster != (raster_value))					 \
		{																	 \
			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();							 \
			P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_RASTER, \
				((register_state_p)->raster = (raster_value)));				 \
		}																	 \
	}

/*
 * Macros to restore graphics state
 */

/*
 * @doc:P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP:
 *
 * Restore the hardware clip rectangle to the the clipping rectangle
 * requested by SI for the current drawing operation.
 *
 * @enddoc
 */

#define P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p)		\
	{																	\
		if (screen_state_p->clip_state != P9000_STATE_CLIP_TO_SI_CLIP)	\
		{																\
			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();						\
			P9000_REGISTER_SET_CLIPPING_RECTANGLE(						\
				screen_state_p->generic_state.screen_clip_left,			\
				screen_state_p->generic_state.screen_clip_top,			\
				screen_state_p->generic_state.screen_clip_right,		\
				screen_state_p->generic_state.screen_clip_bottom);		\
			screen_state_p->clip_state = P9000_STATE_CLIP_TO_SI_CLIP;	\
		}																\
	}


/*
 * @doc:P9000_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN:
 * 
 * Set the clipping rectangle to the virtual screen, if not already so.
 *
 * @enddoc
 */

#define P9000_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN(screen_state_p)	 \
	{																		 \
		if (screen_state_p->clip_state !=									 \
			P9000_STATE_CLIP_TO_VIRTUAL_SCREEN)								 \
		{																	 \
			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();							 \
			P9000_REGISTER_SET_CLIPPING_RECTANGLE(0, 0,						 \
				(screen_state_p->generic_state.screen_virtual_width - 1),	 \
				(screen_state_p->generic_state.screen_virtual_height - 1));	 \
			screen_state_p->clip_state = P9000_STATE_CLIP_TO_VIRTUAL_SCREEN; \
		}																	 \
	}

/*
 * @doc:P9000_STATE_SET_CLIP_RECTANGLE_TO_PHYSICAL_MEMORY:
 * 
 * Set the clip rectangle to all of video memory if not already so.
 * 
 * @enddoc
 */

#define P9000_STATE_SET_CLIP_RECTANGLE_TO_PHYSICAL_MEMORY(screen_state_p)	  \
	{																		  \
		if (screen_state_p->clip_state !=									  \
			P9000_STATE_CLIP_TO_PHYSICAL_MEMORY)							  \
		{																	  \
			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();							  \
			P9000_REGISTER_SET_CLIPPING_RECTANGLE(0, 0,						  \
				(screen_state_p->generic_state.screen_physical_width - 1),	  \
				(screen_state_p->generic_state.screen_physical_height - 1));  \
			screen_state_p->clip_state = P9000_STATE_CLIP_TO_PHYSICAL_MEMORY; \
		}																	  \
	}


/*
 * @doc:P9000_STATE_SYNCHRONIZE_REGISTERS:
 *
 * Synchronize the hardware registers with the SI specified values.
 *
 * @enddoc
 */

#define P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p, required_flags)	\
	{																		\
		if (screen_state_p->changed_state.flags & (required_flags))			\
		{																	\
			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();							\
			P9000_REGISTER_SET_FOREGROUND_COLOR(screen_state_p->			\
				changed_state.foreground_color);							\
			register_state_p->fground = 									\
				screen_state_p->changed_state.foreground_color;				\
			P9000_REGISTER_SET_BACKGROUND_COLOR(screen_state_p->			\
				changed_state.background_color);							\
			register_state_p->bground = 									\
				screen_state_p->changed_state.background_color;				\
			P9000_REGISTER_SET_PLANE_MASK(screen_state_p->					\
				changed_state.planemask);									\
			register_state_p->pmask = 										\
				screen_state_p->changed_state.planemask;					\
			screen_state_p->changed_state.flags = 0;						\
		}																	\
	}


#if (defined(__DEBUG__))

#define P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p, what)	\
	{																\
		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();						\
		ASSERT(P9000_READ_DRAWING_ENGINE_REGISTER(					\
				P9000_DRAWING_ENGINE_FGROUND) ==					\
			   register_state_p->fground);							\
		/*CONSTANTCONDITION*/										\
		if (what & P9000_STATE_CHANGE_FOREGROUND_COLOR)				\
		{															\
			ASSERT(register_state_p->fground ==						\
				   screen_state_p->changed_state.foreground_color);	\
		}															\
		ASSERT(P9000_READ_DRAWING_ENGINE_REGISTER(					\
				P9000_DRAWING_ENGINE_BGROUND) ==					\
			   register_state_p->bground);							\
		/*CONSTANTCONDITION*/										\
		if (what & P9000_STATE_CHANGE_BACKGROUND_COLOR)				\
		{															\
			ASSERT(register_state_p->bground ==						\
				   screen_state_p->changed_state.background_color);	\
		}															\
		ASSERT(P9000_READ_DRAWING_ENGINE_REGISTER(					\
				P9000_DRAWING_ENGINE_PMASK) ==						\
			   register_state_p->pmask);							\
		/*CONSTANTCONDITION*/										\
		if (what & P9000_STATE_CHANGE_PLANEMASK)					\
		{															\
			ASSERT(register_state_p->pmask ==						\
				   screen_state_p->changed_state.planemask);		\
		}															\
	}

#else

#define P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p, what) /* nothing */

#endif

#define P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p)\
	(screen_state_p)->clip_state = P9000_STATE_CLIP_NONE;
			
/***
 ***	Types.
 ***/

/*
 * Memory Configurations.
 */

#define DEFINE_MEMORY_CONFIGURATIONS()			\
	DEFINE_MEMORY_CONFIGURATION(1, 0),			\
	DEFINE_MEMORY_CONFIGURATION(2, 1),			\
	DEFINE_MEMORY_CONFIGURATION(3, 2),			\
	DEFINE_MEMORY_CONFIGURATION(4, 3),			\
	DEFINE_MEMORY_CONFIGURATION(5, 4),			\
	DEFINE_MEMORY_CONFIGURATION(COUNT, -1)
	
enum p9000_memory_configuration_kind
{
#define DEFINE_MEMORY_CONFIGURATION(TYPE, VALUE)\
	MEMORY_CONFIGURATION_##TYPE = VALUE
	
	DEFINE_MEMORY_CONFIGURATIONS()

#undef DEFINE_MEMORY_CONFIGURATION
};


/*
 * Clip states.
 */

#define DEFINE_CLIP_KINDS()						\
	DEFINE_CLIP(NONE),							\
	DEFINE_CLIP(TO_SI_CLIP),					\
	DEFINE_CLIP(TO_VIRTUAL_SCREEN),				\
	DEFINE_CLIP(TO_PHYSICAL_MEMORY),			\
	DEFINE_CLIP(COUNT)

enum p9000_state_clip_kind
{
#define DEFINE_CLIP(NAME)	P9000_STATE_CLIP_##NAME	
	DEFINE_CLIP_KINDS()
#undef DEFINE_CLIP
};


/*
 * @doc:struct p9000_screen_state:
 *
 * Weitek Power 9000 Screen State Structure.
 * 
 * @enddoc
 */ 

struct p9000_screen_state
{

	/*
	 * Inherit from the generic screen state
	 */

	struct generic_screen_state generic_state;

	/*
	 * Option structure pointer.
	 */
	
	struct p9000_options_structure *options_p;
	
	void *board_functions_p;

	/*
	 * Pointer to display mode table from which the mode selection
	 * is made.  This hook is provided for  the board level code
 	 * to provide new mode values
	 */

	void *display_mode_table_p;

	/*
	 * No of entries in the mode table 
	 */

	int display_mode_table_entry_count;

	/*
	 * Pointers to modules handling hardware  on board (other then the 
	 * p9000 chipset) like DAC, Clock etc.
	 * NOTE: If the board level layer is handling any of these modules
	 * then the corresponding fields will be patched by the board level layer 
	 * and the chipset level layer should step aside
	 */

	void *dac_state_p;

	void *clock_state_p;

	/*
	 * Information about the hardware modules that is visible
	 * to other modules
	 */

	struct p9000_dac_functions *dac_functions_p;
	struct p9000_clock_functions *clock_functions_p;
	
	/*
	 * Physical address at which the p9000 is memory
	 * mapped
	 */

	unsigned int  p9000_base_address;
	
	/*
	 * Address at which the dac is I/O mapped
	 */

	unsigned int  dac_base_address;

	unsigned int framebuffer_length;

	/*
	 * File descriptor of the open lfb device
	 */

	int mmap_device_file_descriptor;

	/*
	 * The frame buffer pointer for this screen.
	 */

	unsigned char *framebuffer_p;

	/*
	 * Base address of the p9000 memory map 
	 */

	void *base_p;

	/*
	 *Display mode entry pointer
	 */

	struct p9000_display_mode_table_entry *display_mode_p;

	/*
	 * Number of lines to save and restore during
	 * vt switches
	 */

	int vt_switch_save_lines_count;

	unsigned long graphics_engine_loop_timeout_count;

	unsigned short	pixels_per_word;
	
	unsigned short	pixels_per_word_shift;

	/*
	 * Minterm tables
	 * For three different sources
	 *  FG color register
	 *  BG color register
	 *  BLIT (Video memory, Host data, pattern registers
	 */

	const unsigned short *fg_color_minterm_table_p;
	const unsigned short *bg_color_minterm_table_p;
	const unsigned short *blit_minterm_table_p;

	
	/*
	 * Pointers to various module states
	 */

	struct p9000_register_state *register_state_p;

	void *arc_state_p;

	void *font_state_p;

	void *cursor_state_p;


	/*
	 * Flag is true if the current GE clipping rectangle
	 * corresponds to the graphics state clipping rectangle
	 */

	enum p9000_state_clip_kind clip_state;
	
	/*
	 * Structure for handling register changes requested by SI.
	 */

	struct 
	{
		/*
		 * Flags used for marking invalid components.
		 */

		unsigned int flags;
		
		/*
		 * new foreground color.
		 */

		unsigned int foreground_color;

		/*
		 * new background color.
		 */

		unsigned int background_color;

		/*
		 * new planemask.
		 */

		unsigned int planemask;

	} changed_state;
	
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
export enum debug_level p9000_state_debug = DEBUG_LEVEL_NONE;
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

#include "sidep.h"
#include "p9k_gs.h"
#include "p9k_gbls.h"
#include "p9k_regs.h"

/***
 ***	Constants.
 ***/

/*
 * Minterms to use when the drawing operation source  is
 * video memory or cpu data or pattern registers
 */

#define	MINTERM(expr)	(unsigned short)(expr)

STATIC const unsigned short 
p9000_state_blit_rop_to_minterms_table[16] =
{
MINTERM(0),												/* GXclear */
MINTERM(P9000_SOURCE_MASK & P9000_DESTINATION_MASK),	/* GXand */
MINTERM(P9000_SOURCE_MASK & ~P9000_DESTINATION_MASK),	/* GXandReverse */
MINTERM(P9000_SOURCE_MASK),								/* GXcopy*/
MINTERM(~P9000_SOURCE_MASK & P9000_DESTINATION_MASK ),	/* GXAndInverted*/ 
MINTERM(P9000_DESTINATION_MASK),						/* GXnoop */
MINTERM(P9000_SOURCE_MASK ^ P9000_DESTINATION_MASK),	/* GXxor*/
MINTERM(P9000_SOURCE_MASK | P9000_DESTINATION_MASK),	/* GXor*/
MINTERM(~P9000_SOURCE_MASK & ~P9000_DESTINATION_MASK),	/* GXnor*/
MINTERM(P9000_SOURCE_MASK  ^ ~P9000_DESTINATION_MASK),	/* GXequiv*/
MINTERM(~P9000_DESTINATION_MASK),						/* GXinvert*/
MINTERM(P9000_SOURCE_MASK | ~P9000_DESTINATION_MASK),	/* GXorReverse*/
MINTERM(~P9000_SOURCE_MASK),							/* GXcopyInverted */
MINTERM(~P9000_SOURCE_MASK | P9000_DESTINATION_MASK),	/* GXorInverted */
MINTERM(~P9000_SOURCE_MASK | ~P9000_DESTINATION_MASK),	/* GXnand */
MINTERM(~0)												/* GXset */
};

/*
 * Minterms with foreground register as source
 */

STATIC const unsigned short 
p9000_state_foreground_color_rop_to_minterms_table[16] =
{
MINTERM(0),														/* GXclear */
MINTERM(P9000_FOREGROUND_COLOR_MASK & P9000_DESTINATION_MASK),	/* GXand */
MINTERM(P9000_FOREGROUND_COLOR_MASK & ~P9000_DESTINATION_MASK),	/* GXandReverse */
MINTERM(P9000_FOREGROUND_COLOR_MASK), 							/* GXcopy*/
MINTERM(~P9000_FOREGROUND_COLOR_MASK & P9000_DESTINATION_MASK),	/* GXAndInverted*/ 
MINTERM(P9000_DESTINATION_MASK), 								/* GXnoop */
MINTERM(P9000_FOREGROUND_COLOR_MASK ^ P9000_DESTINATION_MASK), 	/* GXxor*/
MINTERM(P9000_FOREGROUND_COLOR_MASK | P9000_DESTINATION_MASK),	/* GXor*/
MINTERM(~P9000_FOREGROUND_COLOR_MASK & ~P9000_DESTINATION_MASK),/* GXnor*/
MINTERM(P9000_FOREGROUND_COLOR_MASK  ^ ~P9000_DESTINATION_MASK),/* GXequiv*/
MINTERM(~P9000_DESTINATION_MASK), 								/* GXinvert*/
MINTERM(P9000_FOREGROUND_COLOR_MASK | ~P9000_DESTINATION_MASK),	/* GXorReverse*/
MINTERM(~P9000_FOREGROUND_COLOR_MASK),							/* GXcopyInverted*/
MINTERM(~P9000_FOREGROUND_COLOR_MASK | P9000_DESTINATION_MASK),	/* GXorInverted */
MINTERM(~P9000_FOREGROUND_COLOR_MASK | ~P9000_DESTINATION_MASK),/* GXnand */
MINTERM(~0)														/* GXset */
};

/*
 * Minterms with background register as source
 */

STATIC const unsigned short 
p9000_state_background_color_rop_to_minterms_table[16] =
{
MINTERM(0),														/* GXclear */
MINTERM(P9000_BACKGROUND_COLOR_MASK & P9000_DESTINATION_MASK),	/* GXand */
MINTERM(P9000_BACKGROUND_COLOR_MASK & ~P9000_DESTINATION_MASK),	/* GXandReverse */
MINTERM(P9000_BACKGROUND_COLOR_MASK), 							/*GXcopy*/
MINTERM(~P9000_BACKGROUND_COLOR_MASK & P9000_DESTINATION_MASK ),/*GXAndInverted*/ 
MINTERM(P9000_DESTINATION_MASK), 								/* GXnoop */
MINTERM(P9000_BACKGROUND_COLOR_MASK ^ P9000_DESTINATION_MASK), 	/*GXxor*/
MINTERM(P9000_BACKGROUND_COLOR_MASK | P9000_DESTINATION_MASK),	/*GXor*/
MINTERM(~P9000_BACKGROUND_COLOR_MASK & ~P9000_DESTINATION_MASK),/*GXnor*/
MINTERM(P9000_BACKGROUND_COLOR_MASK  ^ ~P9000_DESTINATION_MASK),/*GXequiv*/
MINTERM(~P9000_DESTINATION_MASK), 								/*GXinvert*/
MINTERM(P9000_BACKGROUND_COLOR_MASK | ~P9000_DESTINATION_MASK),	/*GXorReverse*/
MINTERM(~P9000_BACKGROUND_COLOR_MASK),						/*GXcopyInverted */
MINTERM(~P9000_BACKGROUND_COLOR_MASK | P9000_DESTINATION_MASK),	/* GXorInverted */
MINTERM(~P9000_BACKGROUND_COLOR_MASK | ~P9000_DESTINATION_MASK),	/*GXnand*/
MINTERM(~0)														/* GXset */
};

#undef MINTERM
/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

/***
 *** Variables
 ***/

/***
 *** Functions
 ***/


/*
 * @doc:p9000_register_put_state:
 * Function for putting the p9000 chipset in the requested mode. This
 * function is called during vt_restore time
 * @endoc
 */

STATIC int
p9000_register_put_state(struct generic_screen_state *generic_state_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_state,INTERNAL))
	{
		(void) fprintf(debug_stream_p,
"(p9000_register_put_state)\n"
"{\n"
"\tregister_state_p = %p\n"
"}\n",
					   (void *) register_state_p);
	}
#endif

	P9000_WRITE_SYSTEM_REGISTER(P9000_REGISTER_INTERRUPT_EN,
		register_state_p->interrupt_en);

	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_PREVRTC,0);

	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_PREHRZC,0);


	P9000_WRITE_VRAM_CONTROL_REGISTER(P9000_VRAM_CONTROL_MEM_CONFIG,
		register_state_p->mem_config);
	
	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_SRTCTL,
		register_state_p->srtctl);
	
	P9000_WRITE_VRAM_CONTROL_REGISTER(P9000_VRAM_CONTROL_RFPERIOD,
	  register_state_p->rfperiod);

	P9000_WRITE_VRAM_CONTROL_REGISTER(P9000_VRAM_CONTROL_RLMAX,
	  register_state_p->rlmax);
	
	/*
	 *Program sysconf
	 */

	P9000_WRITE_SYSTEM_REGISTER(P9000_REGISTER_SYSCONFIG,
		register_state_p->sysconfig);
	
	/*
	 *Write timings
	 */

	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_HRZT,
	   register_state_p->hrzt);
	
	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_HRZSR,
	   register_state_p->hrzsr);
	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_HRZBR,
	   register_state_p->hrzbr);
	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_HRZBF,
	   register_state_p->hrzbf);
	
	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_VRTT,
	   register_state_p->vrtt);
	
	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_VRTSR,
	   register_state_p->vrtsr);

	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_VRTBR,
	   register_state_p->vrtbr);
	P9000_WRITE_VIDEO_CONTROL_REGISTER(P9000_VIDEO_CONTROL_VRTBF,
	   register_state_p->vrtbf);
	

	/*
	 * Program the drawing engine registers
	 */

	 P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_DRAW_MODE,
		register_state_p->draw_mode);


	P9000_REGISTER_SET_FOREGROUND_COLOR(register_state_p->fground);
	P9000_REGISTER_SET_BACKGROUND_COLOR(register_state_p->bground);
	
	P9000_REGISTER_SET_RASTER(register_state_p->raster);

	P9000_REGISTER_SET_PLANE_MASK(register_state_p->pmask);

	P9000_REGISTER_SET_CLIPPING_RECTANGLE(0,0,
		(screen_state_p->generic_state.screen_virtual_width -1),
		(screen_state_p->generic_state.screen_virtual_height -1));
	
	return 1;
}



/*
 * @doc:p9000_state_set_clip:
 *
 * Update the display libraries clipping state to that specified.
 *
 * @enddoc
 */

STATIC void
p9000_state_set_clip(
	SIint32 x_left, 
	SIint32 y_top, 
	SIint32 x_right, 
	SIint32 y_bottom)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_state,INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_state_set_clip)\n"
					   "{\n"
					   "\tx_left = %ld\n"
					   "\ty_top = %ld\n"
					   "\tx_right = %ld\n"
					   "\ty_bottom = %ld\n",
					   x_left, y_top, x_right, y_bottom);
	}
#endif

	/*
	 * check if any of the clip coordinates need changing.  If so mark
	 * the clip state to be invalid.
	 */

	if ((screen_state_p->generic_state.screen_clip_left != x_left) ||
		(screen_state_p->generic_state.screen_clip_right != x_right) ||
		(screen_state_p->generic_state.screen_clip_top != y_top) ||
		(screen_state_p->generic_state.screen_clip_bottom != y_bottom))
	{
		
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_state, INTERNAL))
		{
			(void) fprintf(debug_stream_p, "\t# changed.\n");
		}
#endif 

		/*
		 * Update the screen copy of the clip values.
		 */

		screen_state_p->generic_state.screen_clip_left = x_left;
		screen_state_p->generic_state.screen_clip_right = x_right;
		screen_state_p->generic_state.screen_clip_top = y_top;
		screen_state_p->generic_state.screen_clip_bottom = y_bottom;

		/*
		 * Mark that the clip registers don't correspond to the SI
		 * specified clip state.
		 */

		screen_state_p->clip_state = P9000_STATE_CLIP_NONE;

	}

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_state, INTERNAL))
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

}


/*
 * @doc:p9000_screen_state__gs_change__:
 *
 * This function is called whenever a new graphics state
 * is selected.
 *
 * @enddoc
 */

function void
p9000_screen_state__gs_change__()
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	SIGStateP si_gs_p = NULL;

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, graphics_state_p));

#if (defined(__DEBUG__)) 
	if (DEBUG_LEVEL_MATCH(p9000_state, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p, 
					   "(p9000_screen_state__gs_change__)\n"
					   "{\n");
	}
#endif 

	si_gs_p = &(graphics_state_p->generic_state.si_graphics_state);

	screen_state_p->changed_state.flags = 0;
	
	/*
	 * Handle the foreground color.
	 */

	screen_state_p->changed_state.foreground_color =
		(si_gs_p->SGfg & 0xFF);

	if (register_state_p->fground != (si_gs_p->SGfg & 0xFF))
	{

#if (defined(__DEBUG__)) 
		if (DEBUG_LEVEL_MATCH(p9000_state, INTERNAL))
		{
			(void) fprintf(debug_stream_p, "\t# FG\n");
		}
#endif 

		screen_state_p->changed_state.flags |=
			P9000_STATE_CHANGE_FOREGROUND_COLOR;

	}

	/*
	 * Handle the background color.
	 */

	screen_state_p->changed_state.background_color =
		(si_gs_p->SGbg & 0xFF);
		
	if (register_state_p->bground != (si_gs_p->SGbg & 0xFF))
	{

#if (defined(__DEBUG__)) 
		if (DEBUG_LEVEL_MATCH(p9000_state, INTERNAL))
		{
			(void) fprintf(debug_stream_p, "\t# BG\n");
		}
#endif 

		screen_state_p->changed_state.flags |=
			P9000_STATE_CHANGE_BACKGROUND_COLOR;

	}


	/*
	 * Handle the planemask.
	 */

	screen_state_p->changed_state.planemask =
		si_gs_p->SGpmask & 0xFF;
		
	if (register_state_p->pmask != (si_gs_p->SGpmask & 0xFF))
	{

#if (defined(__DEBUG__)) 
		if (DEBUG_LEVEL_MATCH(p9000_state, INTERNAL))
		{
			(void) fprintf(debug_stream_p, "\t# PMASK\n");
		}
#endif 

		screen_state_p->changed_state.flags |=
			P9000_STATE_CHANGE_PLANEMASK;
		
	}
	

#if (defined(__DEBUG__)) 
	if (DEBUG_LEVEL_MATCH(p9000_state, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif 

}

/*
 * @doc:p9000_screen_state__initialize__:
 *
 * Initializing the state module.
 *
 * @enddoc
 */

function void
p9000_screen_state__initialize__(
	SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
{

	struct p9000_screen_state *screen_state_p = 
		(struct p9000_screen_state *) si_screen_p->vendorPriv;

	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 &(screen_state_p->generic_state)));

	/*
	 * The clip routines get filled here.
	 */

	functions_p->si_poly_clip = functions_p->si_line_clip =
	functions_p->si_fillarc_clip = functions_p->si_font_clip = 
	functions_p->si_drawarc_clip = functions_p->si_fillarc_clip = 
		p9000_state_set_clip;

	
	/*
	 * Initialize minterm table pointers
	 */

	screen_state_p->fg_color_minterm_table_p =
			p9000_state_foreground_color_rop_to_minterms_table;

	screen_state_p->bg_color_minterm_table_p =
			p9000_state_background_color_rop_to_minterms_table;

	screen_state_p->blit_minterm_table_p =
			p9000_state_blit_rop_to_minterms_table;

	/*
	 * Set the pointer to the register_put_state function if the
	 * board level had not filled it in.
	 */

	if (screen_state_p->register_state_p->generic_state.register_put_state ==
		NULL)
	{
		screen_state_p->register_state_p->generic_state.register_put_state = 
			p9000_register_put_state;
	}

	/*
	 * Determine depth dependent parameters.
	 */

	switch(screen_state_p->generic_state.screen_depth)
	{
	case 4:
		screen_state_p->pixels_per_word = 8;
		screen_state_p->pixels_per_word_shift = 3;
		break;
	case 8:
		screen_state_p->pixels_per_word = 4;
		screen_state_p->pixels_per_word_shift = 2;
		break;
	case 16:
		screen_state_p->pixels_per_word = 2;
		screen_state_p->pixels_per_word_shift = 1;
		break;
	default:
		/*CONSTANTCONDITION*/
		ASSERT(0); 
		break;
	}
	
	/*
	 * Force reprogramming of the chipset state.
	 */
	
	screen_state_p->changed_state.flags = ~0U;
	screen_state_p->changed_state.foreground_color =
		register_state_p->fground;
	screen_state_p->changed_state.background_color =
		register_state_p->bground;
	screen_state_p->changed_state.planemask =
		register_state_p->pmask;
	
	STAMP_OBJECT(P9000_SCREEN_STATE, screen_state_p);

}

