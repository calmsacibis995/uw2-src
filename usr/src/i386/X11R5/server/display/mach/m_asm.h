/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_asm.h	1.1"

#if (! defined(__M_ASM_INCLUDED__))

#define __M_ASM_INCLUDED__



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

extern void mach_asm_repz_inl(const int port, int count, 
							  void *pointer_p); 
extern void mach_asm_repz_inw(const int port, int count, 
							  void *pointer_p);
extern void mach_asm_repz_inb(const int port, int count,
							  void *pointer_p);
extern void mach_asm_repz_outl(const int port, int count, 
							   void *pointer_p);
extern void mach_asm_repz_outw(const int port, int count, 
							   void *pointer_p);
extern void mach_asm_repz_outb(const int port, int count, 
							   void *pointer_p);
extern void mach_asm_repz_inl_4(const int port, int count, 
								void *pointer_p);
extern void mach_asm_repz_inw_4(const int port, int count, 
								void *pointer_p);
extern void mach_asm_repz_inb_4(const int port, int count, 
								void *pointer_p);
extern void mach_asm_repz_outl_4(const int port, int count, 
								 void *pointer_p);
extern void mach_asm_repz_outw_4(const int port, int count,
								 void *pointer_p);
extern void mach_asm_repz_outb_4(const int port, int count, 
								 void *pointer_p);

extern void mach_asm_plot_points(int count, void *points_p);

extern void mach_asm_ibm_blit_text(void *font_glyphs_p,
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
extern int mach_asm_esi ;


/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean mach_asm_debug ;
#endif

/*
 *	Current module state.
 */

extern void
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
;

extern void
mach_asm_move_screen_bits(int source_x, int source_y,
						  int destination_x, int destination_y,
						  int width, int height)
;


#endif
