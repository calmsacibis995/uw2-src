/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/wdc3x/wdc3x.h	1.4"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 * Copyright (c) 1992,1993 Pittsburgh Powercomputing Corporation (PPc).
 * All Rights Reserved.
 */

enum wdtype {
    WD90TYPE_PVGA1=1,
    WD90TYPE_C00=0,
    WD90TYPE_C10=10,
    WD90TYPE_C11=11,
    WD90TYPE_C30=30,
    WD90TYPE_C31=31,
    WD90TYPE_C33=33,
};


extern const unsigned char reverse_nybble[16];
extern const unsigned char reverse_byte[256];

#define BLTREG_MAX 16
extern int bltregs[BLTREG_MAX];

extern int wd90_disp_x;
extern int wd90_disp_y;
extern int wd90_rop;

extern void repoutsw(unsigned short port, unsigned short *from, int numshorts);

enum {
    WDC31_640x480,
    WDC31_800x600,
    WDC31_1024x768,
};


/*  MISC. MACROS */

/* Calculating v only once, check it against a range of values  */
/* Ex:  OUTSIDE(x,1,3) is true when x<1 or x>3			*/
/* Ex:  OUTSIDE(x,3,1) is true when x==2 [range is 3...1, wrapping around 0] */
#define OUTSIDE(v,min,max)	((unsigned) (v) - (min) > (max) - (min))



/* REGISTER ACCESS MACROS */

/* define things that are actually macros as uppercase */
#define IN_REG in_reg
#define OUT_REG	out_reg

#define W_SEQ(x)	OUT_REG(&regtab[I_SEQ], 0, (x));

/* WD90Cxx registers */

/* Address Offset A */
#define R_PR0_A()	read_reg(&regtab[I_GRAPH], 0x09)
#define W_PR0_A(x)	OUT_REG(&regtab[I_GRAPH], 0x09, (x))

/* Alternate Address Offset B */
#define R_PR0_B()	read_reg(&regtab[I_GRAPH], 0x0A)
#define W_PR0_B(x)	OUT_REG(&regtab[I_GRAPH], 0x0A, (x))

/* Memory Size */
#define R_PR1()		read_reg(&regtab[I_GRAPH], 0x0B)
#define W_PR1(x)	OUT_REG(&regtab[I_GRAPH], 0x0B, (x))
#define PR1_1MEG	0xC0
#define PR1_512K	0x80
#define PR1_256K_WD	0x40
#define PR1_256K_IBM	0x00
#define PR1_ENPR0B	0x08
#define PR1_16BIT	0x04
#define PR1_BIOS16	0x02
#define PR1_BIOSMAPOUT	0x01

/* Video Select */
#define R_PR2()		read_reg(&regtab[I_GRAPH], 0x0C)
#define W_PR2(x)	OUT_REG(&regtab[I_GRAPH], 0x0C, (x))
#define PR2_USE_VCLK2	0x02		/* use external third clock */

/* CRT Control */
#define R_PR3()		read_reg(&regtab[I_GRAPH], 0x0D)
#define W_PR3(x)	OUT_REG(&regtab[I_GRAPH], 0x0D, (x))

/* Video Control */
#define R_PR4()		read_reg(&regtab[I_GRAPH], 0x0E)
#define W_PR4(x)	OUT_REG(&regtab[I_GRAPH], 0x0E, (x))
#define PR4_256COLOR	0x01

/* Unlock (PR0-PR4) / Status */
#define R_PR5(x)	read_reg(&regtab[I_GRAPH], 0x0F)
#define W_PR5(x)	OUT_REG(&regtab[I_GRAPH], 0x0F, (x))
#define PR5_LOCK_PR0_PR4	0x02
#define PR5_UNLOCK_PR0_PR4	0x05

/* Unlock (PR11-PR17) */
#define R_PR10(x) 	read_reg(wdc_ptr, 0x29)
#define W_PR10(x) 	OUT_REG(wdc_ptr, 0x29, (x))
#define PR10_LOCK_PR11_PR17	0x02
#define PR10_UNLOCK_PR11_PR17	0x85

/* EGA Switches */
#define R_PR11(x) 	read_reg(wdc_ptr, 0x2A)
#define W_PR11(x) 	OUT_REG(wdc_ptr, 0x2A, (x))

/* Scratch Pad */
#define R_PR12(x) 	read_reg(wdc_ptr, 0x2B)
#define W_PR12(x) 	OUT_REG(wdc_ptr, 0x2B, (x))

/* Interlace H/2 Start */
#define R_PR13(x) 	read_reg(wdc_ptr, 0x2C)
#define W_PR13(x) 	OUT_REG(wdc_ptr, 0x2C, (x))

/* Interlace H/2 End */
#define R_PR14(x) 	read_reg(wdc_ptr, 0x2D)
#define W_PR14(x) 	OUT_REG(wdc_ptr, 0x2D, (x))

/* Miscellaneous Control 1 */
#define R_PR15(x) 	read_reg(wdc_ptr, 0x2E)
#define W_PR15(x) 	OUT_REG(wdc_ptr, 0x2E, (x))
#define PR15_HIGH_VCLK	0x40

/* Miscellaneous Control 2 */
#define R_PR16()  	read_reg(wdc_ptr, 0x2F);
#define W_PR16(x)	OUT_REG(wdc_ptr, 0x2F, (x))

/* Miscellaneous Control 3 */
#define R_PR17()  	read_reg(wdc_ptr, 0x30);
#define W_PR17(x)	OUT_REG(wdc_ptr, 0x30, (x))

/* Unlock Sequencer Extended Registers (WONLY) */
#define R_PR20(x)	read_reg(&regtab[I_SEQ], 0x06)
#define W_PR20(x)	OUT_REG(&regtab[I_SEQ], 0x06, (x))
#define PR20_UNLOCK	0x48
#define PR20_LOCK	0x10

/* Display Configuration and Scratch-Pad */
#define R_PR21(x)	read_reg(&regtab[I_SEQ], 0x07)
#define W_PR21(x)	OUT_REG(&regtab[I_SEQ], 0x07, (x))

/* Memory Interface and FIFO Control */
#define R_PR30(x)	read_reg(&regtab[I_SEQ], 0x10)
#define W_PR30(x)	OUT_REG(&regtab[I_SEQ], 0x10, (x))
#define PR30_WB1		0x00	/* Write buffer 1 level deep */
#define PR30_WB2		0x40	/* Write buffer 2 levels deep */
#define PR30_WB3		0x80	/* Write buffer 3 levels deep */
#define PR30_WB4		0xC0	/* Write buffer 4 levels deep */
#define PR30_FIFO1		0x00	/* FIFO refills when 1 level empty */
#define PR30_FIFO2		0x01	/* FIFO refills when 2 levels empty */
#define PR30_FIFO3		0x02	/* FIFO refills when 3 levels empty */
#define PR30_FIFO4		0x03	/* FIFO refills when 4 levels empty */

/* System Interface Control */
#define R_PR31(x)	read_reg(&regtab[I_SEQ], 0x11)
#define W_PR31(x)	OUT_REG(&regtab[I_SEQ], 0x11, (x))
#define PR31_TURBO_BLANK	0x40	/* skips extra screen refresh cycles */
#define PR31_TURBO_TEXT		0x20	/* "improved text performance" */
#define PR31_WBUFF		0x04	/* enable write buffer */
#define PR31_16BIT		0x01	/* enable 16-bit IO to CRTC, etc. */

/* Miscellaneous Control 4 */
#define R_PR32(x)	read_reg(&regtab[I_SEQ], 0x12)
#define W_PR32(x)	OUT_REG(&regtab[I_SEQ], 0x12, (x))
#define PR32_USR0_CTRL		4	/* controls USR0 output (???) */

/*==========================================================================*/
/* WD90C3x registers */

/* CRTC Vertical Timing Overflow */
#define R_PR18()  	read_reg(wdc_ptr, 0x3E);
#define W_PR18(x)	OUT_REG(wdc_ptr, 0x3E, (x))

/* Signature Analyzer Control */
#define R_PR19()  	read_reg(wdc_ptr, 0x3F);
#define W_PR19(x)	OUT_REG(wdc_ptr, 0x3F, (x))

/* CRTC Shadow Register Control */
#define R_PR1A()  	read_reg(wdc_ptr, 0x3D);
#define W_PR1A(x)	OUT_REG(wdc_ptr, 0x3D, (x))

/* Scratch-Pad */
#define R_PR22(x)	read_reg(&regtab[I_SEQ], 0x08)
#define W_PR22(x)	OUT_REG(&regtab[I_SEQ], 0x08, (x))

/* Scratch-Pad */
#define R_PR23(x)	read_reg(&regtab[I_SEQ], 0x09)
#define W_PR23(x)	OUT_REG(&regtab[I_SEQ], 0x09, (x))

/* DRAM Timing and Zero-Wait-State Control Registers */
#define R_PR33(x)	read_reg(&regtab[I_SEQ], 0x13)
#define W_PR33(x)	OUT_REG(&regtab[I_SEQ], 0x13, (x))

/* Video Memory Mapping Register */
#define R_PR34(x)	read_reg(&regtab[I_SEQ], 0x14)
#define W_PR34(x)	OUT_REG(&regtab[I_SEQ], 0x14, (x))

/* Reserved */
#define R_PR35(x)	read_reg(&regtab[I_SEQ], 0x15)
#define W_PR35(x)	OUT_REG(&regtab[I_SEQ], 0x15, (x))

/*==========================================================================*/
/* other index registers */

#define R_INDEX_CTRL()	inw(0x23C0)
#define W_INDEX_CTRL(x) outw(0x23C0, (x))
#define INDEX_SYSTEM	0x0000
#define INDEX_BITBLT	0x0001
#define INDEX_CURSOR	0x0002
#define INDEX_RADDR(x)	(((x)<<8)&0x0F00) /* sub-index register to read */
#define INDEX_NO_AUTOINC 0x1000 /* disable auto-increment on register reads */

#define R_BLOCK_PORT()	inw(0x23C2)
#define W_BLOCK_PORT(x) outw(0x23C2, (x))

#define C31_BLT_PORT	0x23C4
#define R_BLT_PORT()	inw(C31_BLT_PORT)
#define W_BLT_PORT(x)	outw(C31_BLT_PORT, (x))

/*==========================================================================*/
/* SYSTEM REGISTERS */

#define SET_SYS_INDEX()	W_INDEX_CTRL(INDEX_SYS)
#define W_SYS_CTRL(x)	W_BLOCK_PORT((0x0 << 12) | ((x) & 0x0FFF))

/*==========================================================================*/
/* HARDWARE CURSOR */

#define SET_CSR_INDEX()	W_INDEX_CTRL(INDEX_CURSOR)
#define W_CSRREG(r, v)	W_BLOCK_PORT(((r)<<12) | ((v) & 0x0FFF))
#define U_CSRREG(r, x) \
    if (csrregs[r] != (x)) { csrregs[r] = (x);				  \
			     W_BLOCK_PORT( ((r) << 12) | ((x)&0x0FFF)); }

#define W_CSR_CTRL(x)	W_CSRREG(0x0, x)
#define U_CSR_ADDRLO(x)	U_CSRREG(0x1, x)
#define U_CSR_ADDRHI(x)	U_CSRREG(0x2, x)
#define U_CSR_FG(x)	U_CSRREG(0x3, x)
#define U_CSR_BG(x)	U_CSRREG(0x4, x)
#define U_CSR_ORG(x)	U_CSRREG(0x5, x)
#define U_CSR_X(x)	U_CSRREG(0x6, x)
#define U_CSR_Y(x)	U_CSRREG(0x7, x)
#define U_CSR_AUX(x)	U_CSRREG(0x8, x)

/* CSR_CTRL fields: */
#define CTRL_EN		0x800	/* enable harware cursor */
#define CTRL_32X32	0x000	/* 32x32 2-bit cursor */
#define CTRL_64X64	0x200	/* 64x64 2-bit cursor */
#define CTRL_PPROT	0x100	/* plane protection, whatever that means */
#define CTRL_2CLR	0x020	/* 2-color + transparent + inversion */
#define CTRL_2CLR_SP	0x020	/* 2-color + transparent + special inversion */
#define CTRL_3CLR	0x020	/* 3-color + transparent */

/*==========================================================================*/
/* HARDWARE BITBLT */

#define SET_BLT_INDEX()	W_INDEX_CTRL(INDEX_NO_AUTOINC | \
				     INDEX_RADDR(0) | \
				     INDEX_BITBLT)
#define W_BLTREG(r, v)	W_BLOCK_PORT(((r)<<12) | ((v) & 0x0FFF))

/* Update a register -- write only if the value has changed */
#define U_BLTREG(r, x) \
    if (bltregs[r] != (x)) { bltregs[r] = (x);				  \
			     W_BLOCK_PORT( ((r) << 12) | ((x)&0x0FFF)); }

/*
 *  Note:  W_BLT_xxx() should be changed to update bltreg[] contents if any
 *  registers are ever to have both W_BLT_xx() and U_BLT_xxx() used for them.
 *  For now, W_BLT... shouldn't be used on any register U_BLT... is used on.
 */

#define U_BLT_CTRL1(x)	U_BLTREG(0x0, x)
#define U_BLT_CTRL2(x)	U_BLTREG(0x1, x)
#define U_BLT_SRCLO(x)	U_BLTREG(0x2, x)
#define W_BLT_SRCLO(x)	(W_BLTREG(0x2, x), bltregs[2]=(x))
#define U_BLT_SRCHI(x)	U_BLTREG(0x3, x)
#define W_BLT_DSTLO(x)	W_BLTREG(0x4, x)	/* written, not updated */
#define U_BLT_DSTHI(x)	U_BLTREG(0x5, x)
#define U_BLT_WIDTH(x)	U_BLTREG(0x6, x)
#define U_BLT_HEIGHT(x)	U_BLTREG(0x7, x)
#define U_BLT_PITCH(x)	U_BLTREG(0x8, x)
#define U_BLT_ROP(x)	U_BLTREG(0x9, x)
#define U_BLT_FG(x)	U_BLTREG(0xA, x)
#define U_BLT_BG(x)	U_BLTREG(0xB, x)
#define W_BLT_TR_CLR(x)	W_BLTREG(0xC, x)
#define W_BLT_TR_MSK(x)	W_BLTREG(0xD, x)
#define U_BLT_MASK(x)	U_BLTREG(0xE, x)

#define R_BLT_CTRL1()	R_BLOCK_PORT()

/*
 * BLT_CTRL1 fields:
 */
#define CTRL1_GO	0x800 /* 0=done 1=busy,go */
#define CTRL1_BUSY	0x800 /* 0=done 1=busy,go */
#define CTRL1_REVERSE	0x400 /* 0=TOP-TO-BOT, 1=BOT-TO-TOP */
#define CTRL1_PACKED	0x100 /* addressing=packed-pixel (non-planar) */
#define CTRL1_SRC_LINEAR 0x040 /* linearity=linear (non-rectangular) */
#define CTRL1_DST_LINEAR 0x080 /* linearity=linear (non-rectangular) */
#define CTRL1_DST_IO	0x020 /* dst=I/O port (non-memory) */
/* source format */
#define CTRL1_SRC_CLR	0x000 /* source is color */
#define CTRL1_SRC_MCMP	0x004 /* source is monochrome from memory */
#define CTRL1_SRC_FG	0x008 /* source is fixed fg color */
#define CTRL1_SRC_MIO	0x00C /* source is mono from I/O port */
/* source select */
#define CTRL1_SRC_MEM	0x000 /* source is from memory */
#define CTRL1_SRC_IO	0x002 /* source is from I/O port (32-bit) */

/*
 * BLT_CTRL2 fields:
 */
#define CTRL2_INT	0x400 /* generate interrupt on BITBLT completion */
#define CTRL2_QSTART	0x080 /* quick start mode */
#define CTRL2_UPDST	0x040 /* update destination after BITBLT */
#define CTRL2_8x8PAT	0x010 /* enable 8x8 pattern fills */
#define CTRL2_MTR_EN	0x008 /* mono tranparency enable */
#define CTRL2_DTR_POL	0x004 /* 0=matching transp, 1=matching opaque */
#define CTRL2_DTR_EN	0x001 /* destination transparency enable */

/* BLT_ROP fields: */
#define ROP_COPY	0x300


#if 0
#define CLOBBER(flags,flag,field,fn) \
{ \
  if (pflags->flags & (flag)) oldfns.field = pfuncs->field; \
  else oldfns.field = NULL; \
  pfuncs->field = (fn); \
  pflags->flags |= (flag); \
}
#endif
#define CLOBBER(flags,flag,field,fn) \
{ \
  oldfns.field = pfuncs->field; \
  pfuncs->field = (fn); \
  pflags->flags |= (flag); \
}

#define GRAB(field, fn) \
{ \
  oldfns.field = pfuncs->field; \
  pfuncs->field = (fn); \
}

#define FALLBACK(field,args) \
  if (oldfns.field != NULL) \
    return( (*oldfns.field) args ); \
  else return(SI_FAIL);

