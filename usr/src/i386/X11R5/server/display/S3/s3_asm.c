/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_asm.c	1.4"
/***
 ***	NAME
 ***
 ***		s3_asm.c : prototypes for the functions written in
 ***	assembler.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s3_asm.h"
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

/***
 ***	Constants.
 ***/

#define S3_ASM_TRANSFER_TO_VIDEO_MEMORY     (1U << 1)
#define S3_ASM_TRANSFER_FROM_VIDEO_MEMORY	(1U << 2)
#define S3_ASM_TRANSFER_THRO_PLANE			(1U << 3)
#define S3_ASM_TRANSFER_ACROSS_PLANE		(1U << 4)

/***
 ***	Macros.
 ***/


/***
 ***	Types.
 ***/

/*
 *	Forward declarations.
 */
export void s3_asm_repz_inl(const int port, int count, 
							  void *pointer_p); 
export void s3_asm_repz_inw(const int port, int count, 
							  void *pointer_p);
export void s3_asm_repz_inb(const int port, int count,
							  void *pointer_p);
export void s3_asm_repz_outl(const int port, int count, 
							   void *pointer_p);
export void s3_asm_repz_outw(const int port, int count, 
							   void *pointer_p);
export void s3_asm_repz_outb(const int port, int count, 
							   void *pointer_p);
export void s3_asm_repz_inl_4(const int port, int count, 
								void *pointer_p);
export void s3_asm_repz_inw_4(const int port, int count, 
								void *pointer_p);
export void s3_asm_repz_inb_4(const int port, int count, 
								void *pointer_p);
export void s3_asm_repz_outl_4(const int port, int count, 
								 void *pointer_p);
export void s3_asm_repz_outw_4(const int port, int count,
								 void *pointer_p);
export void s3_asm_repz_outb_4(const int port, int count, 
								 void *pointer_p);


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean s3_asm_debug = FALSE;
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

#include <sys/types.h>
#include <memory.h>
#include "s3_globals.h"
#include "s3_regs.h"
#include <stdarg.h>

#include "s3_state.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 *** 	Functions.
 ***/
function void
s3_asm_transfer_helper(unsigned char *memory_bits_p, 
	const int memory_step, const int transfers_per_line, 
	const int memory_height, 
	const int screen_upper_left_x, const int screen_upper_left_y, 
	const int screen_width, const int screen_height,
	void (*transfer_function_p)(const int port, int transfer_count,
		void *bits_p),
	const int transfer_port, int flags, ...)
{
	unsigned short command = 0;
	const unsigned char *memory_bits_fence_p = memory_bits_p +
		(memory_height * memory_step); 

	S3_CURRENT_SCREEN_STATE_DECLARE();

	va_list args_list;
	
	va_start(args_list, flags);
	
	/*
	 * Only one of TO or FROM directions should be set.
	 */
	ASSERT(!((flags & S3_ASM_TRANSFER_TO_VIDEO_MEMORY) && 
			 (flags & S3_ASM_TRANSFER_FROM_VIDEO_MEMORY)));
	
	ASSERT((flags & S3_ASM_TRANSFER_TO_VIDEO_MEMORY) || 
		   (flags & S3_ASM_TRANSFER_FROM_VIDEO_MEMORY));
	
	if ((screen_width <= 0) || (screen_height <= 0))
	{
		return;
	}

	command = screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_USE_PIXTRANS | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

	if (flags & S3_ASM_TRANSFER_THRO_PLANE)
	{
		command |= S3_CMD_PX_MD_THRO_PLANE;
	}
	else
	{
		command |= S3_CMD_PX_MD_ACROSS_PLANE;
	}

#if defined(__DEBUG__)
	if (s3_asm_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_asm_transfer_helper) {\n"
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
			"\tcommand       = 0x%x\n"
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
			flags,
			command);
	}
#endif

	/*
	 * Blit a rectangle of height from display memory to the cpu memory.
	 */
	S3_WAIT_FOR_FIFO(4);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
		screen_upper_left_x );
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
		screen_upper_left_y);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
		screen_width - 1);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
		((screen_height - 1) & S3_MULTIFUNC_VALUE_BITS));


	S3_WAIT_FOR_FIFO(1);
	if (flags & S3_ASM_TRANSFER_TO_VIDEO_MEMORY)
	{
		command |= S3_CMD_WRITE;
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD, command);
		S3_WAIT_FOR_FIFO(8);
	}
	else if (flags & S3_ASM_TRANSFER_FROM_VIDEO_MEMORY)
	{
		command |= S3_CMD_READ;
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD, command);
		S3_WAIT_FOR_READ_DATA_IN_PIXTRANS_REGISTER();
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
	return;
}

/*
 * s3_asm_move_screen_bits
 * 
 * Move screen bits from source_x, source_y, to destination_x,
 * destination_y.  This will be ultimately written as an inline macro.
 * The caller sets the ROP, planemask, dpconfig etc registers to
 * achieve bitblts or stplblts as needed.
 */

function void
s3_asm_move_screen_bits(int source_x, int source_y,
	int destination_x, int destination_y, int width, int height, int flags)
{
	unsigned int	x_dir;
	unsigned int	y_dir;
	S3_CURRENT_SCREEN_STATE_DECLARE();

	if ((width <= 0) || (height <= 0))
	{
		return;
	}

	S3_WAIT_FOR_FIFO(7);
	if (source_x >= destination_x)
	{
		/*
		 * Copy left to right 
		 */
		x_dir = S3_CMD_AXIAL_X_LEFT_TO_RIGHT; 
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, source_x);
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, destination_x);
	}
	else
	{
		/*
		 * Copy right to left
		 */
		x_dir = S3_CMD_AXIAL_X_RIGHT_TO_LEFT;
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, source_x + width - 1);
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
			destination_x + width - 1);
	}

	if (source_y >= destination_y)
	{
		/*
		 * Copy top to bottom
		 */
		y_dir = S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_Y, source_y);
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, destination_y);
	}
	else
	{
		/*
		 * Copy bottom to top
		 */
		y_dir = S3_CMD_AXIAL_Y_BOTTOM_TO_TOP;
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_Y, source_y + height - 1);
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
			destination_y + height - 1);
	}

	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, width - 1);
	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS));

	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		screen_state_p->cmd_flags | 
		S3_CMD_TYPE_BITBLT |
		S3_CMD_WRITE | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		x_dir|
		S3_CMD_AXIAL_X_MAJOR |
		(flags & S3_ASM_TRANSFER_ACROSS_PLANE ? 
			S3_CMD_PX_MD_ACROSS_PLANE : 
			S3_CMD_PX_MD_THRO_PLANE ) | 
		y_dir);
}

/*
 * 16 bit MMIO writes of 4bpp case.
 */
function void
s3_mmio_write_4_bit_pixels_16(const int port, const int count, void *pointer_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	register int	k = count;
	register unsigned short	*src_p = (unsigned short *)pointer_p;
	register unsigned short *dst_p = 
		(unsigned short *)screen_state_p->mmio_base_address;

	if (count <= 0) 
	{
		return;
	}

	/*
	 * We cannot write more than 32 K at a time.
	 */
	ASSERT(count <= 32*512);
	ASSERT(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(screen_state_p));

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		ASSERT(screen_state_p->use_mmio == 0);
		S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
	}

	ASSERT(screen_state_p->use_mmio == 1);
	/*
	 * Pump the data into the port.
	 */
	do
	{
		*dst_p++ = ((*src_p & 0xF0F0U) >> 4U) | ((*src_p & 0x0F0FU) << 4U);
		src_p++;
	} while(--k);

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
	}
	return;
}

/*
 * 16 bit MMIO reads of 4bpp cases.
 */
function void
s3_mmio_read_4_bit_pixels_16(const int port, const int count, void *pointer_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	register int	k = count;
	register unsigned short	*dst_p = (unsigned short *)pointer_p;
	register unsigned short *src_p = 
		(unsigned short *)screen_state_p->mmio_base_address;

	if (count <= 0) 
	{
		return;
	}

	/*
	 * We cannot read more than 32 K at a time.
	 */
	ASSERT(count <= 32*512);

	ASSERT(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(screen_state_p));

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		ASSERT(screen_state_p->use_mmio == 0);
		S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
	}

	ASSERT(screen_state_p->use_mmio == 1); 

	/* 
	 * Read data from the port.  
	 */
	do
	{
		*dst_p = *src_p++;
		*dst_p++ = ((*dst_p & 0xF0F0U) >> 4U) | ((*dst_p & 0x0F0FU) << 4U);
	} while(--k);

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
	}
	return;
}

/*
 * 16 bit MMIO writes of 8bpp case.
 */
function void
s3_mmio_write_8_bit_pixels_16(const int port, const int count, void *pointer_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	register int	k = count;
	register unsigned short	*src_p = (unsigned short *)pointer_p;
	register unsigned short *dst_p = 
		(unsigned short *)screen_state_p->mmio_base_address;

	if (count <= 0) 
	{
		return;
	}

	/*
	 * We cannot write more than 32 K at a time.
	 */
	ASSERT(count <= 32*512);
	ASSERT(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(screen_state_p));

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		ASSERT(screen_state_p->use_mmio == 0);
		S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
	}

	ASSERT(screen_state_p->use_mmio == 1);
	/*
	 * Pump the data into the port.
	 */
	do
	{
		*dst_p++ = *src_p++;
	} while(--k);
	
	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
	}
	return;
}

/*
 * 16 bit MMIO reads of 8bpp cases.
 */
function void
s3_mmio_read_8_bit_pixels_16(const int port, const int count, void *pointer_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	register int	k = count;
	register unsigned short	*dst_p = (unsigned short *)pointer_p;
	register unsigned short *src_p = 
		(unsigned short *)screen_state_p->mmio_base_address;

	if (count <= 0) 
	{
		return;
	}

	/*
	 * We cannot read more than 32 K at a time.
	 */
	ASSERT(count <= 32*512);

	ASSERT(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(screen_state_p));

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		ASSERT(screen_state_p->use_mmio == 0);
		S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
	}

	ASSERT(screen_state_p->use_mmio == 1); 

	/* 
	 * Read data from the port.  
	 */
	do
	{
		*dst_p++ = *src_p++;
	} while(--k);

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
	}
	return;
}

/*
 * 8 bit MMIO writes of 4bpp case.
 */
function void
s3_mmio_write_4_bit_pixels_8(const int port, const int count, void *pointer_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	register int	k = count;
	register unsigned char	*src_p = (unsigned char *)pointer_p;
	register unsigned char *dst_p = 
		(unsigned char *)screen_state_p->mmio_base_address;

	if (count <= 0) 
	{
		return;
	}

	/*
	 * We cannot write more than 32 K at a time.
	 */
	ASSERT(count <= 32*1024);
	ASSERT(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(screen_state_p));

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		ASSERT(screen_state_p->use_mmio == 0);
		S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
	}

	ASSERT(screen_state_p->use_mmio == 1);
	/*
	 * Pump the data into the port.
	 */
	do
	{
		*dst_p++ = ((*src_p & 0xF0U) >> 4U) | ((*src_p & 0x0FU) << 4U);
		src_p++;
	} while(--k);

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
	}
	return;
}

/*
 * 8 bit MMIO reads of 4bpp cases.
 */
function void
s3_mmio_read_4_bit_pixels_8(const int port, const int count, void *pointer_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	register int	k = count;
	register unsigned char	*dst_p = (unsigned char *)pointer_p;
	register unsigned char *src_p = 
		(unsigned char *)screen_state_p->mmio_base_address;

	if (count <= 0) 
	{
		return;
	}

	/*
	 * We cannot read more than 32 K at a time.
	 */
	ASSERT(count <= 32*1024);

	ASSERT(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(screen_state_p));

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		ASSERT(screen_state_p->use_mmio == 0);
		S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
	}

	ASSERT(screen_state_p->use_mmio == 1); 

	/* 
	 * Read data from the port.  
	 */
	do
	{
		*dst_p = *src_p++;
		*dst_p++ = ((*dst_p & 0xF0U) >> 4U) | ((*dst_p & 0x0FU) << 4U);
	} while(--k);

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
	}
	return;
}

/*
 * 8 bit MMIO writes of 8bpp case.
 */
function void
s3_mmio_write_8_bit_pixels_8(const int port, const int count, void *pointer_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	register int	k = count;
	register unsigned char	*src_p = (unsigned char *)pointer_p;
	register unsigned char *dst_p = 
		(unsigned char *)screen_state_p->mmio_base_address;

	if (count <= 0) 
	{
		return;
	}

	/*
	 * We cannot write more than 32 K at a time.
	 */
	ASSERT(count <= 32*1024);
	ASSERT(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(screen_state_p));

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		ASSERT(screen_state_p->use_mmio == 0);
		S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
	}

	ASSERT(screen_state_p->use_mmio == 1);
	/*
	 * Pump the data into the port.
	 */
	do
	{
		*dst_p++ = *src_p++;
	} while(--k);
	
	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
	}
	return;
}

/*
 * 8 bit MMIO reads of 8bpp cases.
 */
function void
s3_mmio_read_8_bit_pixels_8(const int port, const int count, void *pointer_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	register int	k = count;
	register unsigned char	*dst_p = (unsigned char *)pointer_p;
	register unsigned char *src_p = 
		(unsigned char *)screen_state_p->mmio_base_address;

	if (count <= 0) 
	{
		return;
	}

	/*
	 * We cannot read more than 32 K at a time.
	 */
	ASSERT(count <= 32*1024);

	ASSERT(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(screen_state_p));

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		ASSERT(screen_state_p->use_mmio == 0);
		S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
	}

	ASSERT(screen_state_p->use_mmio == 1); 

	/* 
	 * Read data from the port.  
	 */
	do
	{
		*dst_p++ = *src_p++;
	} while(--k);

	if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
		screen_state_p))
	{
		S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
	}
	return;
}

#if (defined(lint))
void s3_asm_repz_outl(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_outw(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_outb(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_inl(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_inw(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_inb(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_inb_4(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_inw_4(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_inl_4(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_outb_4(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_outw_4(const int port, int count, void *pointer_p)
{
	return;
}

void s3_asm_repz_outl_4(const int port, int count, void *pointer_p)
{
	return;
}

#endif
