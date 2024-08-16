/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_asm.h	1.3"
#if (! defined(__S3_ASM_INCLUDED__))

#define __S3_ASM_INCLUDED__



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
extern void s3_asm_repz_inl(const int port, int count, 
							  void *pointer_p); 
extern void s3_asm_repz_inw(const int port, int count, 
							  void *pointer_p);
extern void s3_asm_repz_inb(const int port, int count,
							  void *pointer_p);
extern void s3_asm_repz_outl(const int port, int count, 
							   void *pointer_p);
extern void s3_asm_repz_outw(const int port, int count, 
							   void *pointer_p);
extern void s3_asm_repz_outb(const int port, int count, 
							   void *pointer_p);
extern void s3_asm_repz_inl_4(const int port, int count, 
								void *pointer_p);
extern void s3_asm_repz_inw_4(const int port, int count, 
								void *pointer_p);
extern void s3_asm_repz_inb_4(const int port, int count, 
								void *pointer_p);
extern void s3_asm_repz_outl_4(const int port, int count, 
								 void *pointer_p);
extern void s3_asm_repz_outw_4(const int port, int count,
								 void *pointer_p);
extern void s3_asm_repz_outb_4(const int port, int count, 
								 void *pointer_p);


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean s3_asm_debug ;
#endif

/*
 *	Current module state.
 */

extern void
s3_asm_transfer_helper(unsigned char *memory_bits_p, 
	const int memory_step, const int transfers_per_line, 
	const int memory_height, 
	const int screen_upper_left_x, const int screen_upper_left_y, 
	const int screen_width, const int screen_height,
	void (*transfer_function_p)(const int port, int transfer_count,
		void *bits_p),
	const int transfer_port, int flags, ...)
;

extern void
s3_asm_move_screen_bits(int source_x, int source_y,
	int destination_x, int destination_y, int width, int height, int flags)
;

extern void
s3_mmio_write_4_bit_pixels_16(const int port, const int count, void *pointer_p)
;

extern void
s3_mmio_read_4_bit_pixels_16(const int port, const int count, void *pointer_p)
;

extern void
s3_mmio_write_8_bit_pixels_16(const int port, const int count, void *pointer_p)
;

extern void
s3_mmio_read_8_bit_pixels_16(const int port, const int count, void *pointer_p)
;

extern void
s3_mmio_write_4_bit_pixels_8(const int port, const int count, void *pointer_p)
;

extern void
s3_mmio_read_4_bit_pixels_8(const int port, const int count, void *pointer_p)
;

extern void
s3_mmio_write_8_bit_pixels_8(const int port, const int count, void *pointer_p)
;

extern void
s3_mmio_read_8_bit_pixels_8(const int port, const int count, void *pointer_p)
;


#endif
