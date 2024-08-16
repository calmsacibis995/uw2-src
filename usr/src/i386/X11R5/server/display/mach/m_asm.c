/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/***
 ***	NAME
 ***
 ***		mach_asm.c : prototypes for the functions written in
 ***	assembler.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m_asm.h"
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

#ident	"@(#)mach:mach/m_asm.c	1.2"

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

#define MACH_ASM_USE_NON_CONFORMING_BLIT		(1U << 0)
#define MACH_ASM_TRANSFER_TO_VIDEO_MEMORY       (1U << 1)
#define MACH_ASM_TRANSFER_FROM_VIDEO_MEMORY		(1U << 2)

/***
 ***	Macros.
 ***/

asm void 
mach_asm_inline_repz_outw(int port, int count, void *ptr)
{
%con port; treg ptr, count;
		movl	%esi,	mach_asm_esi 	// save %esi
		movl	ptr,	%esi			// count cannot be in esi
		movl	count,	%ecx
		movl	port,	%edx
		repz
		outsw
		movl	mach_asm_esi,	%esi 	// restore %esi
%con port; reg ptr; mem count;
		movl 	%esi,	mach_asm_esi 	// save %esi
		pushl	count
		movl	ptr,	%esi			// ptr can't be on stack
		popl	%ecx
		movl	port,	%edx
		repz
		outsw
		movl	mach_asm_esi,	%esi 	// restore %esi
%con port; mem ptr; reg count;
		movl	%esi,	mach_asm_esi	// save %esi
		pushl	ptr
		movl	count, 	%ecx			// count can't be on stack
		popl	%esi
		movl	port,	%edx
		repz
		outsw
		movl	mach_asm_esi,	%esi	// restore %esi
%con port; mem ptr, count;
		movl 	%esi, 	mach_asm_esi	// save %esi
		movl	ptr,	%esi			// can't be in any register
		movl	count, 	%ecx			// can't be in any register
		movl	port,	%edx
		repz
		outsw
		movl	mach_asm_esi,	%esi	// restore %esi
%error
}

/***
 ***	Types.
 ***/

/*
 *	Forward declarations.
 */

export void mach_asm_repz_inl(const int port, int count, 
							  void *pointer_p); 
export void mach_asm_repz_inw(const int port, int count, 
							  void *pointer_p);
export void mach_asm_repz_inb(const int port, int count,
							  void *pointer_p);
export void mach_asm_repz_outl(const int port, int count, 
							   void *pointer_p);
export void mach_asm_repz_outw(const int port, int count, 
							   void *pointer_p);
export void mach_asm_repz_outb(const int port, int count, 
							   void *pointer_p);
export void mach_asm_repz_inl_4(const int port, int count, 
								void *pointer_p);
export void mach_asm_repz_inw_4(const int port, int count, 
								void *pointer_p);
export void mach_asm_repz_inb_4(const int port, int count, 
								void *pointer_p);
export void mach_asm_repz_outl_4(const int port, int count, 
								 void *pointer_p);
export void mach_asm_repz_outw_4(const int port, int count,
								 void *pointer_p);
export void mach_asm_repz_outb_4(const int port, int count, 
								 void *pointer_p);

export void mach_asm_plot_points(int count, void *points_p);

export void mach_asm_ibm_blit_text(void *font_glyphs_p,
				unsigned short *glyph_list_start_p,
				unsigned short *glyph_list_end_p,
				int current_x,
				int y_top,
				int width);

/***
 ***	Variables.
 ***/

/*
 * Saved registers used in the inline macros.
 */
export int mach_asm_esi = 0;


/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean mach_asm_debug = FALSE;
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

#include "m_globals.h"
#include "m_regs.h"
#include <stdarg.h>

#if (defined(__DEBUG__))
#include "m_state.h"
#endif

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 *** 	Functions.
 ***/

/*
 * mach_asm_ms_helper
 *
 * Transfer bits from system memory to video memory.
 */

function void
mach_asm_transfer_helper(unsigned char *memory_bits_p, 
						 const int memory_step, 
						 const int transfers_per_line, 
						 const int memory_height,
						 const int screen_upper_left_x,
						 const int screen_upper_left_y,
						 const int screen_width,
						 const int screen_height,
						 void (*transfer_function_p)(const int port, int
													 transfer_count, void
													 *bits_p),
						 const int transfer_port,
						 int flags,
						 ...)
{
	int skip = 0;
	const unsigned char *memory_bits_fence_p = memory_bits_p +
		(memory_height * memory_step); 

#if (defined(__DEBUG__))
	MACH_CURRENT_SCREEN_STATE_DECLARE();
#endif

	va_list args_list;
	
	ASSERT(!MACH_IS_IO_ERROR());
	ASSERT(!MACH_IS_DATA_READY());
	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);

	va_start(args_list, flags);
	
#if defined(__DEBUG__)
	if (mach_asm_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_asm_transfer_helper)\n"
"{\n"
"\tmemory_bits_p    = %p\n"
"\tmemory_step   = %d\n"
"\ttransfers_per_line = %d\n"
"\tmemory_height=   %d\n"
"\tscreen_upper_left_x     = %d\n"
"\tscreen_upper_left_y    = %d\n"
"\tscreen_width  =   %d\n"
"\tscreen_height   = %d\n"
"\ttransfer_function_p = %p\n"
"\ttransfer_port = 0x%x\n"
"\tflags         = %d\n"
"}\n",
					   (void *) memory_bits_p,
					   memory_step,
					   transfers_per_line,
					   memory_height,
					   screen_upper_left_x,
					   screen_upper_left_y,
					   screen_width,
					   screen_height,
					   (void *) transfer_function_p,
					   transfer_port,
					   flags);
	}
#endif

	if (flags & MACH_ASM_USE_NON_CONFORMING_BLIT)
	{
		/*
		 * get the skip for the first line.
		 */
		skip = va_arg(args_list, int);

#if (defined(__DEBUG__))
		if (mach_asm_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_asm_transfer_helper)\n"
"{\n"
"\t# Non confirming blit\n"
"\t\tskip = %d\n"
"}\n",
						   skip);
		}
#endif

	}
	
	MACH_WAIT_FOR_FIFO(6);
	outw(MACH_REGISTER_CUR_X,
		 screen_upper_left_x + skip);
	outw(MACH_REGISTER_CUR_Y, screen_upper_left_y);
	outw(MACH_REGISTER_DEST_X_START, screen_upper_left_x);
	outw(MACH_REGISTER_DEST_X_END, screen_upper_left_x +
		 screen_width);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);
	outw(MACH_REGISTER_DEST_Y_END, screen_upper_left_y +
		 screen_height); /* blit initiator */

	/*
	 * Only one of TO or FROM directions should be set.
	 */
	ASSERT(!((flags & MACH_ASM_TRANSFER_TO_VIDEO_MEMORY) && 
			 (flags & MACH_ASM_TRANSFER_FROM_VIDEO_MEMORY)));
	
	ASSERT((flags & MACH_ASM_TRANSFER_TO_VIDEO_MEMORY) || 
		   (flags & MACH_ASM_TRANSFER_FROM_VIDEO_MEMORY));
	

	if (flags & MACH_ASM_TRANSFER_TO_VIDEO_MEMORY)
	{
		MACH_WAIT_FOR_FIFO(16);
	}
	else if (flags & MACH_ASM_TRANSFER_FROM_VIDEO_MEMORY)
	{
		MACH_WAIT_FOR_DATA_READY();
	}

	ASSERT(memory_bits_p < memory_bits_fence_p);
	
	do
	{
		(*transfer_function_p)(transfer_port,
							   transfers_per_line,
							   memory_bits_p);
		memory_bits_p += memory_step;
	} while (memory_bits_p < memory_bits_fence_p);
	
		
	va_end(args_list);

	ASSERT(!MACH_IS_IO_ERROR());
	ASSERT(!MACH_IS_DATA_READY());
	
	return;
}
	
/*
 * mach_asm_move_screen_bits
 * 
 * Move screen bits from source_x, source_y, to destination_x,
 * destination_y.  This will be ultimately written as an inline macro.
 * The caller sets the ROP, planemask, dpconfig etc registers to
 * achieve bitblts or stplblts as needed.
 */

function void
mach_asm_move_screen_bits(int source_x, int source_y,
						  int destination_x, int destination_y,
						  int width, int height)
{

#if (defined(__DEBUG__))
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);
#endif

	MACH_WAIT_FOR_FIFO(10);

	if (source_x >= destination_x)
	{
		/*
		 * Copy from left to right.
		 */
		outw(MACH_REGISTER_SRC_X, source_x);
		outw(MACH_REGISTER_SRC_X_START, source_x);
		outw(MACH_REGISTER_SRC_X_END, source_x + width);
		outw(MACH_REGISTER_CUR_X, destination_x);
		outw(MACH_REGISTER_DEST_X_START, destination_x);
		outw(MACH_REGISTER_DEST_X_END, destination_x + width);
	}
	else
	{
		/*
		 * Copy from right to left.
		 */
		outw(MACH_REGISTER_SRC_X, source_x + width);
		outw(MACH_REGISTER_SRC_X_START, source_x + width);
		outw(MACH_REGISTER_SRC_X_END, source_x);
		outw(MACH_REGISTER_CUR_X, destination_x + width);
		outw(MACH_REGISTER_DEST_X_START, destination_x + width);
		outw(MACH_REGISTER_DEST_X_END, destination_x);
	}
	
	/*
	 * Determine the vertical directon of the copy
	 */
	if (source_y >= destination_y)
	{
		/*
		 * top to bottom.
		 */
		outw(MACH_REGISTER_SRC_Y_DIR, 1);
		outw(MACH_REGISTER_SRC_Y, source_y);
		outw(MACH_REGISTER_CUR_Y, destination_y);
		outw(MACH_REGISTER_DEST_Y_END, destination_y + height);
								/* BLIT STARTS */
	}
	else
	{
		/*
		 * bottom to top.
		 */
		outw(MACH_REGISTER_SRC_Y_DIR, 0);
		outw(MACH_REGISTER_SRC_Y, source_y + height - 1);
		outw(MACH_REGISTER_CUR_Y, destination_y + height - 1);
		outw(MACH_REGISTER_DEST_Y_END, destination_y - 1);
								/* BLIT STARTS */
	}
}

#if (defined(lint))


void mach_asm_repz_inl(const int port, int count, 
							  void *pointer_p)
{
	return;
}

void mach_asm_repz_inw(const int port, int count, 
							  void *pointer_p)
{
	return;
}

void mach_asm_repz_inb(const int port, int count,
							  void *pointer_p)
{
	return;
}

void mach_asm_repz_outl(const int port, int count, 
							   void *pointer_p)
{
	return;
}

void mach_asm_repz_outw(const int port, int count, 
							   void *pointer_p)
{
	return;
}

void mach_asm_repz_outb(const int port, int count, 
							   void *pointer_p)
{
	return;
}

void mach_asm_repz_inl_4(const int port, int count, 
								void *pointer_p)
{
	return;
}

void mach_asm_repz_inw_4(const int port, int count, 
								void *pointer_p)
{
	return;
}

void mach_asm_repz_inb_4(const int port, int count, 
								void *pointer_p)
{
	return;
}

void mach_asm_repz_outl_4(const int port, int count, 
								 void *pointer_p)
{
	return;
}

void mach_asm_repz_outw_4(const int port, int count,
								 void *pointer_p)
{
	return;
}

void mach_asm_repz_outb_4(const int port, int count, 
								 void *pointer_p)
{
	return;
}

void mach_asm_plot_points(int count, void *points_p)
{
	return;
}

#if (defined(UNUSED_FUNCTIONALITY))

void outl(unsigned port, ulong val)
{
	return;
}

#endif /* UNUSED_FUNCTIONALITY */

void outw(unsigned port, ulong val)
{
	return;
}

void outb(unsigned port, ulong val)
{
	return;
}

ulong inb(unsigned port)
{
	return 0L;
}

ulong inw(unsigned port)
{
	return 0L;
}

#if (defined(UNUSED_FUNCTIONALITY))

ulong inl(unsigned port)
{
	return 0L;
}
#endif /* UNUSED_FUNCTIONALITY */

#endif /* lint */
