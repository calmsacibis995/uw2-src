/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_state.h	1.2"
#if (! defined(__P9K_STATE_INCLUDED__))

#define __P9K_STATE_INCLUDED__



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
extern enum debug_level p9000_state_debug ;
#endif

/*
 *	Current module state.
 */

extern void
p9000_screen_state__gs_change__()
;

extern void
p9000_screen_state__initialize__(
	SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
;


#endif
