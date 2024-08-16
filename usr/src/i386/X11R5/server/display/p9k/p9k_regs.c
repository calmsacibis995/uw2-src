/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_regs.c	1.2"
/***
 ***	NAME
 ***
 ***		p9k_regs.c : register definitions for the p9000 driver.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9k_regs.h"
 ***
 ***	DESCRIPTION
 ***
 ***	@doc:p9k_regs.c:
 ***
 ***		This file contains register offset definitions for the
 ***		Weitek P9000 driver.  These offsets are from the base of
 ***		the 1 MB register area.  Using offset based addressing
 ***		while accessing the p9000 registers is preferred over
 ***		keeping pre-calculated addresses in the screen state on
 ***		the grounds of speed and efficiency.
 ***
 ***	@enddoc
 ***
 ***	@doc:swapcontrol:
 ***
 ***		The bit, byte, and word swap control bits in the address
 ***		of the registers are separate #defines.  Register names
 ***		give the base address of the register in the chipset
 ***		register area.  We have a separate set of symbols for
 ***		every register used in little-endian mode.
 ***
 ***    @enddoc
 ***
 ***	@doc:addressing:
 ***
 ***		The Power 9000 chips supports the following
 ***		addressing scheme.
 ***		
 ***   	---------------
 ***    | 			  |\
 ***	|   		  |	|
 ***	|-------------|	| Frame Buffer Access
 ***	|   		  |	|
 ***	|   		  |/
 ***	|-------------|2MB --\
 ***	|   		  |	     | Power 9000 registers.
 ***    |   		  |	  	 |  
 ***	|-------------|1MB --/\
 ***	|			  |	      | Non Power 9000 accesses.
 ***	|			  |	      /
 ***	---------------0MB ----------------
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
#include "g_regs.h"

/***
 ***	Constants.
 ***/

/*
 * VGA Specific registers and field values
 */

/*
 * VGA MISC OUTPUT register read and write indices
 */

#define P9000_VGA_MISCOUT_REGISTER_READ_ADDRESS 	0x3CC
#define P9000_VGA_MISCOUT_REGISTER_WRITE_ADDRESS 	0x3C2

/*
 * VGA SEQUENCER index and data registers
 */

#define	P9000_VGA_SEQUENCER_INDEX_REGISTER		0x3C4	
#define	P9000_VGA_SEQUENCER_DATA_REGISTER		0x3C5	

#define	P9000_VGA_SEQUENCER_OUTCNTL_INDEX		0x12
#define P9000_VGA_SEQUENCER_MISC_INDEX			0x11



#define P9000_VGA_SEQUENCER_MISC_CRLOCK			0x20

/*
 * Masks used to program the ICD2061A  Frequency Synthesizer
 */

#define P9000_IC_REG0		0x0L		/*Mask Selects ICD Video clock reg 1 */
#define P9000_IC_REG1		0x200000L	/*Mask Selects ICD Video clock reg 2 */
#define P9000_IC_REG2		0x400000L 	/*Mask Selects ICD Video clock reg 3 */
#define	P9000_IC_MREG		0x600000L 	/*Mask Selects ICD Mem timing clock */
#define P9000_IC_CNTL		0xC18000L	/*Mask for TCD Control Register */
#define P9000_IC_DIV4		0xA40000L	


/*
 * Data swapping control.
 */

#define P9000_ADDRESS_SWAP_HALF_WORDS	0x40000		/* enable halfword swap */
#define P9000_ADDRESS_SWAP_BYTES		0x20000		/* enable byte swap */
#define P9000_ADDRESS_SWAP_BITS			0x10000		/* enable bit swap */

#if (defined(i386))

/*
 * Swap definition for the I386
 */

#define P9000_ADDRESS_SWAP_DEFAULT		 0

#else

#error This code will work only for the i386 family of CPUs.

#endif /* i386 */

/*
 * Video and system control register addresses.
 */

#define P9000_ADDRESS_SYSTEM_CONTROL 			0x100000 /* 20 */
#define P9000_ADDRESS_VIDEO_CONTROL				0x100100 /* 20,8 */
#define P9000_ADDRESS_VRAM_CONTROL				0x100180 /* 20,8,7 */
#define P9000_ADDRESS_RAMDAC_CONTROL			0x100200 /* 20,9 */

/*
 * User registers
 */

#define P9000_ADDRESS_PARAMETER_CONTROL 		0x180180U /* 20,19,8,7 */
#define P9000_ADDRESS_PARAMETER_COORDINATE 		0x181000U /* 20,19,12 */
#define P9000_ADDRESS_DRAWING_ENGINE			0x180200U /* 20,19,9 */
#define P9000_ADDRESS_META_COORDINATE			0x181200U /* 20,19,12,9 */
#define P9000_ADDRESS_QUAD_COMMAND				0x180008U /* 20,19,3 */
#define P9000_ADDRESS_BLIT_COMMAND				0x180004U /* 20,19,2 */
#define P9000_ADDRESS_PIXEL8_COMMAND			0x18000CU /* 20,19,3,2 */
#define P9000_ADDRESS_PIXEL1_COMMAND			0x180080U /* 20,19,7-2 */
#define P9000_ADDRESS_NEXT_PIXELS_COMMAND		0x180014U /* 20,19,4,2 */
#define P9000_ADDRESS_STATUS					0x180000U /* 20,19 */

/*
 * Direct framebuffer access.
 */

#define P9000_FRAMEBUFFER_OFFSET				0x200000U /* 21 */

/*
 * System control registers.
 */

#define P9000_REGISTER_SYSCONFIG				0x00000004U /* RW */
#define P9000_REGISTER_INTERRUPT				0x00000008U /* RW */
#define P9000_REGISTER_INTERRUPT_EN				0x0000000CU /* RW */

/**
 ** Register fields
 **/

/*
 * sysconfig register
 */

#define P9000_SYSCONFIG_VERSION				0x00000007U	/* version field */
#define P9000_SYSCONFIG_PIXEL_BUF_WRITE		0x00000200U
											/* bank for FB writes */
#define P9000_SYSCONFIG_PIXEL_BUF_READ		0x00000400U
											/* bank for FB reads */
#define P9000_SYSCONFIG_PIXEL_SWAP_BITS		0x00000800U
											/* swap bits on FB access */
#define P9000_SYSCONFIG_PIXEL_SWAP_BYTES	0x00001000U
											/* swap bytes on FB access */
#define P9000_SYSCONFIG_PIXEL_SWAP_HALF		0x00002000U
											/* swap half words on FB access */
#define P9000_SYSCONFIG_HORIZONTAL_RESOLUTION 0x007FC000U
											/* 3, 3 bit fields for h. res */
#define P9000_SYSCONFIG_HRES_ADD_32			0x01U /* field 1 only */
#define P9000_SYSCONFIG_HRES_ADD_64			0x02U /* fields 1,2 */
#define P9000_SYSCONFIG_HRES_ADD_128		0x04U /* fields 1,2,3 */
#define P9000_SYSCONFIG_HRES_ADD_256		0x08U /* fields 1,2,3 */
#define P9000_SYSCONFIG_HRES_ADD_512		0x10U /* fields 1,2,3 */
#define P9000_SYSCONFIG_HRES_ADD_1024		0x20U /* fields 2,3 */
#define P9000_SYSCONFIG_HRES_ADD_2048		0x40U /* fields 3 only */
#define P9000_SYSCONFIG_HRES_FIELD_1_SHIFT	13U	/* shifts for the h res */
#define P9000_SYSCONFIG_HRES_FIELD_2_SHIFT	16U	/* fields */
#define P9000_SYSCONFIG_HRES_FIELD_3_SHIFT	19U	/*  */
#define P9000_SYSCONFIG_HRES_FIELD_1_ALLOWED	(0+32+64+128+256+512)
#define P9000_SYSCONFIG_HRES_FIELD_2_ALLOWED 	(0+64+128+256+512+1024)
#define P9000_SYSCONFIG_HRES_FIELD_3_ALLOWED	(0+128+256+512+1024+2048)

#define P9000_SYSCONFIG_DRIVELOAD2			0x10000000U
											/* double drive load */

/*
 * interrupt status register
 */

#define P9000_INTERRUPT_C_DE_IDLE			0x00000001U	/* drawing engine */
#define P9000_INTERRUPT_DE_IDLE				0x00000002U	/* idle */
#define P9000_INTERRUPT_C_PICK				0x00000004U	/* pick done */
#define P9000_INTERRUPT_PICK				0x00000008U	/*  */
#define P9000_INTERRUPT_C_VBLANKED			0x00000010U	/* vertical */
#define P9000_INTERRUPT_VBLANKED			0x00000020U	/* blank */

/*
 * interrupt enable register.
 */

#define P9000_INTERRUPT_EN_C_DE_IDLE_EN		0x00000001U	/* drawing engine */
#define P9000_INTERRUPT_EN_DE_IDLE_EN		0x00000002U	/* idle */
#define P9000_INTERRUPT_EN_C_PICKED_EN		0x00000004U	/*  */
#define P9000_INTERRUPT_EN_PICKED_EN		0x00000008U	/*  */
#define P9000_INTERRUPT_C_VBLANKED_EN		0x00000010U	/*  */
#define P9000_INTERRUPT_VBLANKED_EN			0x00000020U /*  */
#define P9000_INTERRUPT_C_MEN				0x00000040U	/* master interrupt */
#define P9000_INTERRUPT_MEN					0x00000080U	/* enable */

/*
 * The parameter engine coordinate register fields.
 */

#define P9000_PARAMETER_COORDINATE_X_32		0x00000008U	/* 32 bit X */
#define P9000_PARAMETER_COORDINATE_Y_32		0x00000010U	/* 32 bit Y */
#define P9000_PARAMETER_COORDINATE_XY_16	0x00000018U	/* packed XY */
#define P9000_PARAMETER_COORDINATE_REL		0x00000020U	/* is relative */
#define P9000_PARAMETER_COORDINATE_ABS		0x00000000U	/* is absolute */
#define P9000_PARAMETER_COORDINATE_REG_0	0x00000000U	/* X[0],Y[0] */
#define P9000_PARAMETER_COORDINATE_REG_1	0x00000040U	/* X[1],Y[1] */
#define P9000_PARAMETER_COORDINATE_REG_2	0x00000080U	/* X[2],Y[2] */
#define P9000_PARAMETER_COORDINATE_REG_3	0x000000C0U	/* X[3],Y[3] */

/*
 * status register.
 */

#define P9000_STATUS_QUAD_INTERSECTS		0x00000001U	/* bit 0 */
#define P9000_STATUS_QUAD_VISIBLE			0x00000002U	/* bit 1 */
#define P9000_STATUS_QUAD_HIDDEN			0x00000004U	/* bit 2 */
#define P9000_STATUS_QUAD_CONCAVE			0x00000008U	/* bit 3 */
#define P9000_STATUS_QUAD_SOFTWARE			0x00000010U	/* bit 4 */
#define P9000_STATUS_BLIT_SOFTWARE			0x00000020U	/* bit 5 */
#define P9000_STATUS_PIXEL_SOFTWARE			0x00000040U	/* bit 6 */
#define P9000_STATUS_PICKED					0x00000080U	/* bit 7 */
#define P9000_STATUS_BUSY					0x40000000U	/* bit 30 */
#define P9000_STATUS_DONT_ISSUE_QUAD_BLIT	0x80000000U	/* bit 31 */
											/* don't issue quad/blit */

/*
 * The parameter control and condition registers.
 */

#define P9000_PARAMETER_CONTROL_OOR			0x00000004U	/* 1<<2 (R) */
#define P9000_PARAMETER_CONTROL_CINDEX		0x0000000CU	/* 3<<2 (RW) */
#define P9000_PARAMETER_CONTROL_W_OFF_XY	0x00000010U	/* 4<<2 (RW) */
#define P9000_PARAMETER_CONTROL_PE_W_MIN	0x00000014U	/* 5<<2 (R) */
#define P9000_PARAMETER_CONTROL_PE_W_MAX	0x00000018U	/* 6<<2 (R) */
#define P9000_PARAMETER_CONTROL_YCLIP		0x00000020U	/* 8<<2 (R) */
#define P9000_PARAMETER_CONTROL_XCLIP		0x00000024U	/* 9<<2 (R) */
#define P9000_PARAMETER_CONTROL_XEDGE_LT	0x00000028U	/* 10<<2 (R) */
#define P9000_PARAMETER_CONTROL_XEDGE_GT	0x0000002CU /* 11<<2 (R) */
#define P9000_PARAMETER_CONTROL_YEDGE_LT	0x00000030U	/* 12<<2 (R) */
#define P9000_PARAMETER_CONTROL_YEDGE_GT	0x00000034U	/* 13<<2 (R) */

/*
 * The OOR (out of range) register.
 */

#define P9000_OOR_Y_0						0x00000001U
#define P9000_OOR_Y_1						0x00000002U
#define P9000_OOR_Y_2						0x00000004U
#define P9000_OOR_Y_3						0x00000008U
#define P9000_OOR_X_0						0x00000010U
#define P9000_OOR_X_1						0x00000020U
#define P9000_OOR_X_2						0x00000040U
#define P9000_OOR_X_3						0x00000080U

/*
 * The Xclip register.
 */

#define P9000_XCLIP_X_GT_MAX_X_0			0x00000001U
#define P9000_XCLIP_X_GT_MAX_X_1			0x00000002U
#define P9000_XCLIP_X_GT_MAX_X_2			0x00000004U
#define P9000_XCLIP_X_GT_MAX_X_3			0x00000008U
#define P9000_XCLIP_X_LT_MIN_X_0			0x00000010U
#define P9000_XCLIP_X_LT_MIN_X_1			0x00000020U
#define P9000_XCLIP_X_LT_MIN_X_2			0x00000040U
#define P9000_XCLIP_X_LT_MIN_X_3			0x00000080U

/*
 * The Yclip register.
 */

#define P9000_YCLIP_X_GT_MAX_X_0			0x00000001U
#define P9000_YCLIP_X_GT_MAX_X_1			0x00000002U
#define P9000_YCLIP_X_GT_MAX_X_2			0x00000004U
#define P9000_YCLIP_X_GT_MAX_X_3			0x00000008U
#define P9000_YCLIP_X_LT_MIN_X_0			0x00000010U
#define P9000_YCLIP_X_LT_MIN_X_1			0x00000020U
#define P9000_YCLIP_X_LT_MIN_X_2			0x00000040U
#define P9000_YCLIP_X_LT_MIN_X_3			0x00000080U

/*
 * The Xedge lt register.
 */

#define P9000_XEDGE_LT_X0_GT_X1				0x00000001U
#define P9000_XEDGE_LT_X1_GT_X2				0x00000002U
#define P9000_XEDGE_LT_X2_GT_X3				0x00000004U
#define P9000_XEDGE_LT_X3_GT_X0				0x00000008U
#define P9000_XEDGE_LT_X1_GT_X3				0x00000010U
#define P9000_XEDGE_LT_X0_GT_X2				0x00000020U

/*
 * The Xedge gt register.
 */

#define P9000_XEDGE_GT_X0_LT_X1				0x00000001U
#define P9000_XEDGE_GT_X1_LT_X2				0x00000002U
#define P9000_XEDGE_GT_X2_LT_X3				0x00000004U
#define P9000_XEDGE_GT_X3_LT_X0				0x00000008U
#define P9000_XEDGE_GT_X1_LT_X3				0x00000010U
#define P9000_XEDGE_GT_X0_LT_X2				0x00000020U

/*
 * The Yedge lt register.
 */

#define P9000_YEDGE_LT_X0_GT_X1				0x00000001U
#define P9000_YEDGE_LT_X1_GT_X2				0x00000002U
#define P9000_YEDGE_LT_X2_GT_X3				0x00000004U
#define P9000_YEDGE_LT_X3_GT_X0				0x00000008U
#define P9000_YEDGE_LT_X1_GT_X3				0x00000010U
#define P9000_YEDGE_LT_X0_GT_X2				0x00000020U

/*
 * The Yedge gt register.
 */

#define P9000_YEDGE_GT_X0_LT_X1				0x00000001U
#define P9000_YEDGE_GT_X1_LT_X2				0x00000002U
#define P9000_YEDGE_GT_X2_LT_X3				0x00000004U
#define P9000_YEDGE_GT_X3_LT_X0				0x00000008U
#define P9000_YEDGE_GT_X1_LT_X3				0x00000010U
#define P9000_YEDGE_GT_X0_LT_X2				0x00000020U

/*
 * Drawing engine registers.
 */

#define P9000_DRAWING_ENGINE_FGROUND		0x00000000U	/* foreground color */
#define P9000_DRAWING_ENGINE_BGROUND		0x00000004U	/* background color */
#define P9000_DRAWING_ENGINE_PMASK			0x00000008U	/* planemask */
#define P9000_DRAWING_ENGINE_DRAW_MODE		0x0000000CU	/* drawing mode */
#define P9000_DRAWING_ENGINE_PAT_ORIGINX	0x00000010U	/* pattern origin X */
#define P9000_DRAWING_ENGINE_PAT_ORIGINY	0x00000014U	/* pattern origin Y */
#define P9000_DRAWING_ENGINE_RASTER			0x00000018U	/* raster operation */
#define P9000_DRAWING_ENGINE_W_MIN			0x00000020U	/* window min. */
#define P9000_DRAWING_ENGINE_W_MAX			0x00000024U	/* window max. */
#define P9000_DRAWING_ENGINE_PATTERN		0x00000080U	/* pattern registers */
#define P9000_DRAWING_ENGINE_PATTERN_END	0x0000009CU	/* last patt reg */

/*
 * The draw mode register.
 */

#define P9000_DRAW_MODE_C_DEST_BUFFER		0x00000001U
#define P9000_DRAW_MODE_DEST_BUFFER			0x00000002U
#define P9000_DRAW_MODE_C_PICK				0x00000004U
#define P9000_DRAW_MODE_PICK				0x00000008U

/*
 * The raster register.
 */

#define P9000_RASTER_MINTERMS				0x0000FFFFU	/* bits 0-15 */
#define P9000_RASTER_QUAD_OVERSIZED_MODE	0x00010000U	/* bit 16 */
#define P9000_RASTER_USE_PATTERN			0x00020000U	/* bit 17 */

/*
 * Minimum and maximum registers.
 */

#define P9000_W_MIN_MAX_Y_VALUE				0x00001FFFU	/* bits 0-12 */
#define P9000_W_MIN_MAX_X_VALUE				0x1FFF0000U	/* bits 16-28 */

/**
 ** Video control registers.
 **/
 
/*
 * Horizontal sync registers.
 */

#define	P9000_VIDEO_CONTROL_HRZC			0x00000004U	/* (R) */
#define P9000_VIDEO_CONTROL_HRZT			0x00000008U	/* (RW) */
#define P9000_VIDEO_CONTROL_HRZSR			0x0000000CU	/* (RW) */
#define P9000_VIDEO_CONTROL_HRZBR			0x00000010U	/* (RW) */
#define P9000_VIDEO_CONTROL_HRZBF			0x00000014U	/* (RW) */
#define P9000_VIDEO_CONTROL_PREHRZC			0x00000018U	/* (RW) */

/*
 * Vertical sync registers.
 */

#define	P9000_VIDEO_CONTROL_VRTC			0x0000001CU	/* (R) */
#define P9000_VIDEO_CONTROL_VRTT			0x00000020U	/* (RW) */
#define P9000_VIDEO_CONTROL_VRTSR			0x00000024U	/* (RW) */
#define P9000_VIDEO_CONTROL_VRTBR			0x00000028U	/* (RW) */
#define P9000_VIDEO_CONTROL_VRTBF			0x0000002CU	/* (RW) */
#define P9000_VIDEO_CONTROL_PREVRTC			0x00000030U	/* (RW) */

/*
 * Video control registers: screen repaint.
 */

#define P9000_VIDEO_CONTROL_SRADDR			0x00000034U	/* (R) */
#define P9000_VIDEO_CONTROL_SRTCTL			0x00000038U	/* (RW) */
#define P9000_VIDEO_CONTROL_SRADDR_INC		0x0000003CU	/*  */

#define P9000_VIDEO_CONTROL_QSFCOUNTER		NOT KNOWN!	/* (R) */

/*
 * The strctl register.
 */

#define P9000_SRTCTL_QSF_SELECT				0x00000007U	/* bits 0-2 */
#define P9000_SRCTL_DISPLAY_BUFFER			0x00000008U	/* bit 3 */
#define P9000_SRTCTL_HBLNK_RELOAD			0x00000010U	/* bit 4 */
#define P9000_SRTCTL_ENABLE_VIDEO			0x00000020U	/* bit 5 */
#define P9000_SRTCTL_COMPOSITE				0x00000040U	/* bit 6 */
#define P9000_SRTCTL_INTERNAL_HSYNC			0x00000080U	/* bit 7 */
#define P9000_SRTCTL_INTERNAL_VSYNC			0x00000100U	/* bit 8 */

/** 
 ** The VRAM control registers.
 **/

#define P9000_VRAM_CONTROL_MEM_CONFIG		0x00000004U	/*  */
#define P9000_VRAM_CONTROL_RFPERIOD			0x00000008U
#define P9000_VRAM_CONTROL_RFCOUNT			0x0000000CU
#define P9000_VRAM_CONTROL_RLMAX			0x00000010U
#define P9000_VRAM_CONTROL_RLCUR			0x00000014U


/*
 * Meta-coordinate accesses.
 */

#define P9000_META_COORDINATE_VTYPE_POINT			0x000000000U
#define P9000_META_COORDINATE_VTYPE_LINE			0x000000040U
#define P9000_META_COORDINATE_VTYPE_TRIANGLE		0x000000080U
#define P9000_META_COORDINATE_VTYPE_QUADRILATERAL	0x0000000C0U

/*
 * @doc:P9000_META_COORDINATE_REL:
 *
 * `Or'-ed in if a meta-coordinate point is relative to the previous
 * vertex.  If left zeroed, coordinates are wrt the window origin.
 *
 * @enddoc
 */

#define P9000_META_COORDINATE_REL			0x000000020U
#define P9000_META_COORDINATE_ABS			0x000000000U

/*
 * @doc:P9000_META_COORDINATE_YX:
 *
 * This field determines whether the meta-coordinate loaded is a 32 bit X
 * value, a 32 bit Y value, or a packed pair of 16bit X and Y values
 *
 * @enddoc
 */

#define P9000_META_COORDINATE_YX_X			0x000000008U /* 32 bit X value */
#define P9000_META_COORDINATE_YX_Y			0x000000010U /* 32 bit Y value */
#define P9000_META_COORDINATE_YX_XY			0x000000018U /* 32 bit XY value */

/***
 *** Macros.
 ***/

/*
 * VGA related macros
 */

/*
 *Unlocking the VGA registers
 */

#define	P9000_UNLOCK_VGA_REGISTERS()\
{\
	unsigned short tmp;\
	outb(P9000_VGA_SEQUENCER_INDEX_REGISTER,\
		 P9000_VGA_SEQUENCER_MISC_INDEX);\
	tmp = \
		inb(P9000_VGA_SEQUENCER_DATA_REGISTER);\
	outb(P9000_VGA_SEQUENCER_DATA_REGISTER,tmp);\
	outb(P9000_VGA_SEQUENCER_DATA_REGISTER,tmp);\
	tmp = inb(P9000_VGA_SEQUENCER_DATA_REGISTER);\
	outb(P9000_VGA_SEQUENCER_DATA_REGISTER,\
		(tmp) & ~P9000_VGA_SEQUENCER_MISC_CRLOCK);\
}

#define P9000_LOCK_VGA_REGISTERS() /**/

	
/*
 * Accessing the registers.
 * The following macros *assume* that screen_state_p is declared
 */


#define P9000_READ_REGISTER(register)\
	*((volatile unsigned int *)\
	  ((p9000_base_p) + ((register | P9000_ADDRESS_SWAP_DEFAULT)/4)))

#define P9000_WRITE_REGISTER(register, data)\
	*((volatile unsigned int *)\
	 ((p9000_base_p) + ((register | P9000_ADDRESS_SWAP_DEFAULT)/4))) =\
	 (data)


/*
 * Macros to do bounds check on x and y coordinates
 */

#define	P9000_IS_X_COORDINATE_OUT_OF_BOUNDS(x)	\
	(((x) < 0) || ((x) > 2048))

#define	P9000_IS_Y_COORDINATE_OUT_OF_BOUNDS(y)  \
	(((y) < 0) || ((y) > 2048))

/*
 * Accessing the P9000 system control registers
 */

#define P9000_WRITE_SYSTEM_REGISTER(register, data)\
	P9000_WRITE_REGISTER((register | P9000_ADDRESS_SYSTEM_CONTROL),\
						 data)

#define P9000_READ_SYSTEM_CONTROL(register)\
	P9000_READ_REGISTER((register | P9000_ADDRESS_SYSTEM_CONTROL))


/*
 * Accessing the parameter coordinate registers.
 */

#define P9000_WRITE_PARAMETER_COORDINATE_REGISTER(name, type,\
												  relative,data)\
	P9000_WRITE_REGISTER((P9000_ADDRESS_PARAMETER_COORDINATE |\
						  (name) | (type) | (relative)), data)

#define P9000_READ_PARAMETER_COORDINATE_REGISTER(name, type, relative)\
	P9000_READ_REGISTER((P9000_ADDRESS_PARAMETER_COORDINATE | (name) |\
						 (type) | (relative)))

/*
 * Accessing the status register.
 */

#define P9000_READ_STATUS_REGISTER()\
	P9000_READ_REGISTER(P9000_ADDRESS_STATUS)

/*
 *  Accessing the parameter control and condition registers.
 */
	
#define P9000_WRITE_PARAMETER_CONTROL_REGISTER(register, data)\
	P9000_WRITE_REGISTER((P9000_ADDRESS_PARAMETER_CONTROL |\
	(register)),data)

#define P9000_READ_PARAMETER_CONTROL_REGISTER(register)\
	P9000_READ_REGISTER((P9000_ADDRESS_PARAMETER_CONTROL |\
	(register)))

/*
 * Accessing the Video and System control registers.
 */

#define P9000_WRITE_VIDEO_CONTROL_REGISTER(register, data)\
	P9000_WRITE_REGISTER((P9000_ADDRESS_VIDEO_CONTROL |\
	(register)), data)

#define P9000_READ_VIDEO_CONTROL_REGISTER(register)\
	P9000_READ_REGISTER((P9000_ADDRESS_VIDEO_CONTROL |\
	(register)) 

/*
 * Accessing the Video and System control registers.
 */

#define P9000_WRITE_VRAM_CONTROL_REGISTER(register, data)\
	P9000_WRITE_REGISTER((P9000_ADDRESS_VRAM_CONTROL |\
	(register)), data)

#define P9000_READ_VRAM_CONTROL_REGISTER(register)\
	P9000_READ_REGISTER((P9000_ADDRESS_VRAM_CONTROL |\
	(register)) 

/*
 * Accessing the drawing engine registers.
 * @doc:accessing drawing engine registers:
 * <<<These registers must be accessed only when the engine is idle>>>
 * @enddoc
 */

#define P9000_WRITE_DRAWING_ENGINE_REGISTER(register, data)	\
	P9000_WRITE_REGISTER((P9000_ADDRESS_DRAWING_ENGINE |\
	(register)), data)

#define P9000_READ_DRAWING_ENGINE_REGISTER(register)\
	P9000_READ_REGISTER((P9000_ADDRESS_DRAWING_ENGINE |\
	(register)))

/*
 * @doc:P9000_WRITE_META_COORDINATE_REGISTER:
 *
 * Write to a meta-coordinate register.  `type' is the type of
 * operation "point", "line", "triangle" etc.  `relative' if set,
 * performs addressing wrt to the previous vertex <<<not window
 * relative addressing>>>.
 *
 * @enddoc
 */

#define P9000_WRITE_META_COORDINATE_REGISTER(type, relative, packing, value)\
	P9000_WRITE_REGISTER((P9000_ADDRESS_META_COORDINATE | (type) |\
	(relative) | (packing)),\
	value)

/*
 * Pattern registers
 */
						
#define P9000_WRITE_PATTERN_REGISTER(index, data, flags)			\
	P9000_WRITE_DRAWING_ENGINE_REGISTER(							\
		(P9000_DRAWING_ENGINE_PATTERN + ((index) << 2U) | (flags)),	\
		data)

#define P9000_READ_PATTERN_REGISTER(index, flags)					\
	P9000_READ_DRAWING_ENGINE_REGISTER(								\
		(P9000_DRAWING_ENGINE_PATTERN + ((index) << 2U) | (flags)))

/*
 * @doc:P9000_PACK_XY:
 *
 * Coordinate packing.  The X value is moved into the upper 16 bits
 * and the Y value is kept in the lower 16 bits.
 *
 * @enddoc */

#define P9000_PACK_XY(X, Y)						\
	(((X) << 16U) | ((Y) & 0xFFFFU))


/*
 * @doc:P9000_INITIATE_QUAD_COMMAND:
 *
 * Initiate a QUAD command.  The four vertices x,y[0]..x,y[3]
 * have to be filled beforehand.  Returns the contents of the
 * status register.
 *
 * Usage: status = P9000_INITIATE_QUAD_COMMAND();
 *
 * @enddoc
 */

#define P9000_INITIATE_QUAD_COMMAND()				\
	P9000_READ_REGISTER(P9000_ADDRESS_QUAD_COMMAND)


/*
 * @doc:P9000_INITIATE_BLIT_COMMAND:
 *
 * Initiate a BLIT command.  The four vertices x,y[0]..x,y[3]
 * have to be filled beforehand.  Returns the contents of the
 * status register.
 *
 * Usage: status = P9000_INITIATE_BLIT_COMMAND();
 *
 * @enddoc
 */

#define P9000_INITIATE_BLIT_COMMAND()\
	P9000_READ_REGISTER(P9000_ADDRESS_BLIT_COMMAND)

/*
 * @doc:P9000_PIXEL8_COMMAND:
 * 
 * Write data out to the pixel8 port.
 * 
 * @enddoc
 */

#define P9000_PIXEL8_COMMAND(data,flags)\
	P9000_WRITE_REGISTER(P9000_ADDRESS_PIXEL8_COMMAND | (flags),\
	data)
						
/*
 * @doc:P9000_PIXEL1_COMMAND:
 * 
 * Write stipple data out to the pixel1 port.
 *
 * @enddoc
 */

#define P9000_PIXEL1_COMMAND(data, length,flags)\
	P9000_WRITE_REGISTER((P9000_ADDRESS_PIXEL1_COMMAND |\
	(((length - 1) & 31) << 2U)) | (flags), data)

/*
 * @doc:P9000_NEXT_PIXELS_COMMAND:
 *
 * Advances the frame buffer to the next block for consequtive
 * `pixel8' or `pixel1' operations.
 *
 * @enddoc
 */

#define P9000_NEXT_PIXELS_COMMAND(width)\
	P9000_WRITE_REGISTER(P9000_ADDRESS_NEXT_PIXELS_COMMAND, width)

/*
 * @doc:P9000_REGISTER_WAIT_FOR_ENGINE_IDLE:
 * 
 * Wait for drawing engine to become idle.  This macro will time out
 * after a set period and shut the server down in an orderly manner if
 * the graphics engine does not become idle in this period.
 *
 * @enddoc
 */

#define	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE()							\
	{																	\
		volatile unsigned long  __count =								\
			screen_state_p->graphics_engine_loop_timeout_count;			\
		while((P9000_READ_STATUS_REGISTER() & P9000_STATUS_BUSY) &&		\
			  (--__count > 0U));										\
		if (__count == 0U)												\
		{																\
			(void) 														\
				p9000_global_graphics_engine_timeout_handler(__FILE__,	\
															 __LINE__);	\
		}																\
	}

/*
 * @doc:P9000_REGISTER_IS_GRAPHICS_ENGINE_IDLE:
 *
 * Return TRUE if the graphics engine is idle.
 *
 * @enddoc
 */

#define P9000_REGISTER_IS_GRAPHICS_ENGINE_IDLE()			\
	((P9000_READ_STATUS_REGISTER() & P9000_STATUS_BUSY) == 0)

/*
 * @doc:P9000_REGISTER_SET_CLIPPING_RECTANGLE:
 *
 * Macro for programming the clipping registers.  The clipping
 * rectangle is specified in inclusive coordinates.
 *
 * @enddoc
 */

#define	P9000_REGISTER_SET_CLIPPING_RECTANGLE(x1, y1, x2, y2)			\
	{																	\
		ASSERT(P9000_REGISTER_IS_GRAPHICS_ENGINE_IDLE());               \
		P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_W_MIN,	\
											P9000_PACK_XY((x1), (y1)));	\
		P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_W_MAX,	\
											P9000_PACK_XY((x2), (y2)));	\
	}


/*
 * @doc:P9000_REGISTER_SET_RASTER:
 *
 * Set the raster register.
 * 
 * @enddoc
 */

#define P9000_REGISTER_SET_RASTER(raster)									\
	{																		\
		ASSERT(P9000_REGISTER_IS_GRAPHICS_ENGINE_IDLE());					\
		P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_RASTER,	\
											(raster));						\
	}

/*
 * @doc:P9000_REGISTER_SET_FOREGROUND_COLOR:
 *
 * Program the foreground color.
 *
 * @enddoc
 */

#define	P9000_REGISTER_SET_FOREGROUND_COLOR(color)							\
	{																		\
		ASSERT(P9000_REGISTER_IS_GRAPHICS_ENGINE_IDLE());					\
		P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_FGROUND,	\
											(color));						\
	}

/*
 * @doc:P9000_REGISTER_SET_BACKGROUND_COLOR:
 *
 * @enddoc
 */

#define	P9000_REGISTER_SET_BACKGROUND_COLOR(color)							\
	{																		\
		ASSERT(P9000_REGISTER_IS_GRAPHICS_ENGINE_IDLE());					\
		P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_BGROUND,	\
											(color));						\
	}

/*
 * @doc:P9000_REGISTER_SET_PLANE_MASK:
 *
 * Set the planemask register.
 
 * @enddoc
 */

#define	P9000_REGISTER_SET_PLANE_MASK(plane_mask)                       \
	{																	\
		ASSERT(P9000_REGISTER_IS_GRAPHICS_ENGINE_IDLE());				\
		P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_PMASK,	\
											(plane_mask));				\
	}


/*
 * Writes to the w_off_xy register with non zero 3rd or 7th nibble
 * don't seem to go thru'. This macro checks for such values
 */ 

#define P9000_REGISTER_IS_VALID_ORIGIN(x_origin,y_origin)\
((!((x_origin) & 0xF000)) && (!((y_origin) & 0xF000)))

/***
 ***	Types.
 ***/

struct p9000_register_state
{
	
	struct generic_register_state generic_state;
	
	/*
	 *Chipset registers
 	 */

	/*
	 * System configuration registers
	 */
	unsigned int sysconfig;
	unsigned int interrupt_en;

	/*
	 * Video control registers
	 */
	unsigned int hrzt;
	unsigned int hrzsr;
	unsigned int hrzbr;
	unsigned int hrzbf;
	unsigned int prehrzc;

	unsigned int vrtc; 
	unsigned int vrtt;
	unsigned int vrtsr;
	unsigned int vrtbr;
	unsigned int vrtbf;	
	unsigned int prevrtc;
	
	unsigned int sraddr;
	unsigned int srtctl;
	

	/*
	 * VRAM  control registers
	 */
	unsigned int mem_config;
	unsigned int rfperiod;
	unsigned int rfcount;
	unsigned int rlmax;
	unsigned int rlcur;


	/*
	 *Drawing engine registers
	 */

	unsigned char fground;
	unsigned char bground;
	
	unsigned char pmask;

	unsigned int raster;

	unsigned char draw_mode;
	
	/*
	 *4 bits are valid
	 */

	unsigned char pat_originx;
	unsigned char pat_originy;

	/*
	 *Clipping registers
	 */
	unsigned int w_min;
	unsigned int w_max;
	
	
	/*
	 * Saved register values: Mainly VGA registers 
	 */
	
	unsigned int saved_outcntl_register;
	unsigned int saved_miscout_register;

	/*
	 * pointer to raw data for the pattern registers.
	 */
	
	unsigned long *pattern_registers_p;
	
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
export enum debug_level p9000_register_debug = DEBUG_LEVEL_NONE;
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

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/
