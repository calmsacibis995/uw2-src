/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/etw32/etw32.h	1.5"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

extern SIFunctions etw32_oldfns;
extern int etw32_rop;
extern int etw32_bg_rop;
extern int etw32_mode;

extern unchar *etw32_mmio_base;	/* base address of accelerator registers */
extern unchar *etw32_reg_base;	/* base address of accelerator registers */
extern int etw32_disp_x;	/* virtual X resolution of the display */
extern int etw32_disp_y;	/* virtual Y resolution of the display */
extern int etw32_memsize;	/* # of bytes of memory on the video board */
extern int etw32_src_addr;	/* byte offset in vid mem of solid fg pat */
extern int etw32_base_port;	/* (v256_is_color ? 0x3D0 : 0x3B0) */
extern int etw32_src_stat;	/* src pixmap status */
extern int etw32_pat_stat;	/* pat pixmap status */

extern unchar etw32_get_reg(int address, int index);
extern void etw32_set_reg(int address, int index, int data);
int etw32_select_readpage(int offset);
int etw32_select_writepage(int offset);
int etw32_src_solid(int color);
int etw32_pat_solid(int color);



extern int mode_nosrc[16];

/*
 * misc. constants
 */
#define MASK_ALL 0xFF


/*
 * Macros for overriding SDD functions & for fallbacks.
 */

#if 0
#define CLOBBER(flags,flag,field,fn) \
{ \
  if (pflags->flags & (flag)) etw32_oldfns.field = pfuncs->field; \
  else etw32_oldfns.field = NULL; \
  pfuncs->field = (fn); \
  pflags->flags |= (flag); \
}
#endif
#define CLOBBER(flags,flag,field,fn) \
{ \
  etw32_oldfns.field = pfuncs->field; \
  pfuncs->field = (fn); \
  pflags->flags |= (flag); \
}

#define GRAB(field, fn) \
{ \
  etw32_oldfns.field = pfuncs->field; \
  pfuncs->field = (fn); \
}

#define FALLBACK(field,args) \
  if (etw32_oldfns.field != NULL) \
    return( (*etw32_oldfns.field) args ); \
  else return(SI_FAIL);

/*
 *  Misc. macros
 */
/* Calculating v only once, check it against a range of values  */
/* Ex:  OUTSIDE(x,1,3) is true when x<1 or x>3			*/
/* Ex:  OUTSIDE(x,3,1) is true when x==2 [range is 3...1, wrapping around 0] */
#define OUTSIDE(v,min,max)	((unsigned) (v) - (min) > (max) - (min))

/*
 *  ET4000/W32 hardware access macros
 */
#define CRTC_PORT	(etw32_base_port + 4)
#define KEY_PORT	(etw32_base_port + 8)
#define ISR1_PORT	(etw32_base_port + 0x0A)

#define TINY_PAUSE	asm("	jmp	.+2")


/*  Turn ON/OFF the "KEY" which allows access to certain registers */
#define SET_KEY()	{outb(0x3bf, 3); outb(KEY_PORT, 0xA0);}
#define UNSET_KEY()	outb(KEY_PORT, 0x29)

/* Set ATC flip-flop to INDEX (next write will be an index value, not DATA) */
#define	ATC_INDEX()	(inb(ISR1_PORT))

/*
 * VGA & extended register reads/writes:
 */
#define W_MISC(x)	outb(0x3C2, (x))
#define R_MISC()	inb(0x3CC)

#define W_CRTC(r,x)	etw32_set_reg(CRTC_PORT, (r), (x))
#define R_CRTC(r)	etw32_get_reg(CRTC_PORT, (r))

#define SEQ_PORT	0x3C4
#define W_SEQ(r,x)	etw32_set_reg(SEQ_PORT, (r), (x))
#define R_SEQ(r)	etw32_get_reg(SEQ_PORT, (r))

#define GDC_PORT	0x3CE
#define W_GDC(r, x)	etw32_set_reg(GDC_PORT, (r), (x))
#define R_GDC(r)	etw32_get_reg(GDC_PORT, (r))

/* Note: for ATC writes, index and data are written to the same port */
#define ATC_PORT	0x3C0
#define W_ATC(r, x)	{ATC_INDEX(); \
  			out_reg(&regtab[I_ATTR], (r), (x));}
#define R_ATC(r)	(ATC_INDEX(), etw32_get_reg(ATC_PORT, (r)))

#define W_SEG1(x)	outb(0x3CD, (x))
#define R_SEG1()	(inb(0x3CD))
#define W_SEG2(x)	outb(0x3CB, (x))
#define R_SEG2()	(inb(0x3CB))


/* Bit fields used in ATC register 10h : Mode Control */
#define ATC10_GRMODE	0x01	/* sets graphics mode (vs. text) */
#define ATC10_MONO	0x02	/* selects monochrome display attribute */

#define ATC16_BYPASS	0x80	/* bypass internal palette registers */

#define CRTC36_ENREG	0x20	/* enable memory-mapped registers */
#define CRTC36_ENMMU	0x08	/* enables three MMU "aperture" buffers */
#define CRTC36_ENACL	(CRTC36_ENREG | CRTC36_ENMMU)	/* enables ACL */

/*
 *  Area of memory to map for memory-mapped registers
 */
#define MMIO_SIZE	0x08000		/* 4K */
#define MMIO_BASE	0xB8000		/* phs addr of area to map */
#define MMIO_REGOFFSET	0x07F00		/* ACL/MMU reg base = 0xBFF00 (phys) */
#define MMIO_AP0OFFSET	0x00000		/* MMU aperture 0 base = 0xB8000  */
#define MMIO_AP1OFFSET	0x02000		/* MMU aperture 1 base = 0xBA000  */
#define MMIO_AP2OFFSET	0x04000		/* MMU aperture 2 base = 0xBC000  */

#define MMU_AP0		(etw32_mmio_base + MMIO_AP0OFFSET)
#define MMU_AP1		(etw32_mmio_base + MMIO_AP1OFFSET)
#define MMU_AP2		(etw32_mmio_base + MMIO_AP2OFFSET)

/*
 *  MMU register access macros:
 */
#define	MMU_BASE0	(*(SIint32 *) (etw32_reg_base + 0x00))
#define	MMU_BASE1	(*(SIint32 *) (etw32_reg_base + 0x04))
#define	MMU_BASE2	(*(SIint32 *) (etw32_reg_base + 0x08)) 
#define	MMU_CTRL	(*(SIint32 *) (etw32_reg_base + 0x13))

#define CTRL_ACL0	0x01	/* routes aperture 0 through the ACL */
#define CTRL_ACL1	0x02
#define CTRL_ACL2	0x04
#define CTRL_LIN0	0x10	/* sets aperture 0 addresing to linear */
#define CTRL_LIN1	0x20
#define CTRL_LIN2	0x40

/*
 *  Accelerator register access macros:
 */
#define SIint8	unsigned char

/* Non-queued registers */
#define ACL_SUSP	(*((SIint8 *) (etw32_reg_base + 0x30)))
#define ACL_STATE	(*((SIint8 *) (etw32_reg_base + 0x31)))
#define ACL_SYNC	(*((SIint8 *) (etw32_reg_base + 0x32)))
#define ACL_IMASK	(*((SIint8 *) (etw32_reg_base + 0x34)))
#define ACL_ISTATUS	(*((SIint8 *) (etw32_reg_base + 0x35)))
#define ACL_STATUS	(*((SIint8 *) (etw32_reg_base + 0x36)))

/* Queued registers */
#define ACL_PADDR	(*((SIint32 *) (etw32_reg_base + 0x80)))
#define ACL_SADDR	(*((SIint32 *) (etw32_reg_base + 0x84)))
#define ACL_PYOFF	(*((SIint16 *) (etw32_reg_base + 0x88)))
#define ACL_SYOFF	(*((SIint16 *) (etw32_reg_base + 0x8A)))
#define ACL_DYOFF	(*((SIint16 *) (etw32_reg_base + 0x8C)))
#define ACL_VBS		(*((SIint8 *) (etw32_reg_base + 0x8E)))
#define ACL_DIR		(*((SIint8 *) (etw32_reg_base + 0x8F)))
#define ACL_PWRAP	(*((SIint8 *) (etw32_reg_base + 0x90)))
#define ACL_SWRAP	(*((SIint8 *) (etw32_reg_base + 0x92)))
#define ACL_XPOS	(*((SIint16 *) (etw32_reg_base + 0x94)))
#define ACL_YPOS	(*((SIint16 *) (etw32_reg_base + 0x96)))
#define ACL_XCNT	(*((SIint16 *) (etw32_reg_base + 0x98)))
#define ACL_YCNT	(*((SIint16 *) (etw32_reg_base + 0x9A)))
#define ACL_RO		(*((SIint8 *) (etw32_reg_base + 0x9C)))
#define ACL_RELD	(*((SIint8 *) (etw32_reg_base + 0x9D)))
#define ACL_BGROP	(*((SIint8 *) (etw32_reg_base + 0x9E)))
#define ACL_FGROP	(*((SIint8 *) (etw32_reg_base + 0x9F)))
#define ACL_DADDR	(*((SIint32 *) (etw32_reg_base + 0xA0)))

/* Fields of ACL_SUSP */
#define SUSP_TERM	0x10	/* TERMINATE Operation! */
#define SUSP_SUSP	0x01	/* suspend operation */

/* Fields of ACL_SYNC */
#define SYNC_EN		0x01		/* Enable queue synchronization */

/* Fields of ACL_RO */
#define	RO_ADAUTO	0x00		/* CPU address not used */
#define	RO_ADCPU	0x10		/* CPU address is dest. addr */
#define RO_DAAUTO	0x00		/* CPU data is not used */
#define RO_DASRC	0x01		/* CPU data is source pixmap */
#define RO_DAMIX	0x02		/* CPU data is mix data */
#define RO_DAX		0x04		/* CPU data is X count */
#define RO_DAY		0x05		/* CPU data is Y count */

/* Fields of ACL_STATUS */
#define STATUS_BUSY	0x02	/* RDST flag : 1 if accelerator is busy */
#define STATUS_XY	0x04	/* XYST flag : 1 if processing an X/Y block */

/* Fields of ACL_STATE */
#define STATE_RESUME	0x08	/* Resume accelerator operation */
#define STATE_RESTORE	0x01	/* Transfer queue into internal registers */


/* Fields of ACL_SWRAP and ACL_PWRAP */
#define WRAP_X4		0x02
#define WRAP_X8		0x03
#define WRAP_X16	0x04
#define WRAP_X32	0x05
#define WRAP_X64	0x06
#define WRAP_XNONE	0x07
#define WRAP_Y1		0x00
#define WRAP_Y2		0x10
#define WRAP_Y4		0x20
#define WRAP_Y8		0x30
#define WRAP_YNONE	0x70
#define WRAP_NONE	(WRAP_YNONE | WRAP_XNONE)

/* Fields of ACL_DIR */
#define	DIR_XINC	0x00		/* X increasing */
#define	DIR_XDEC	0x01		/* X dereasing */
#define	DIR_YINC	0x00		/* Y increasing */
#define	DIR_YDEC	0x02		/* Y dereasing */
#define DIR_NORMAL	(DIR_XINC | DIR_YINC)
#define DIR_REVERSE	(DIR_XDEC | DIR_YDEC)

#define ROP_COPY	0xCC		/* dest = source */
#define ROP_NOOP	0xAA		/* dest = dest */
#define ROP_PCOPY	0xF0		/* dest = pattern */

#define VBS_1		0x00		/* 1-byte Virtual Bus Size */
#define	VBS_2		0x01		/* 2-byte VBS */
#define	VBS_4		0x02		/* 4-byte VBS */


/*
 *  Misc. accelerator-related macros
 */

/*
 * Use these to set source or pattern pixmaps to a solid color.
 */
#define SET_SRC_SOLID(color)		\
	{if (etw32_src_stat != (color)) etw32_src_solid(color);}
#define SET_PAT_SOLID(color)		\
	{if (etw32_pat_stat != (color)) etw32_pat_solid(color);}

/*
 * Use these when source or pattern pixmaps have been altered since the
 *    last SET_xxx_SOLID() call.
 */
#define SRC_NOT_SOLID()		(etw32_src_stat = -1)
#define PAT_NOT_SOLID()		(etw32_pat_stat = -1)



#if 0
#define	WAIT_ON_ACL()	while (ACL_STATUS & STATUS_BUSY) ;
#else
static int waitvar;
#define	WAIT_ON_ACL()	\
  if (ACL_STATUS & STATUS_BUSY) { \
  for (waitvar = 0; ACL_STATUS & STATUS_BUSY; ++waitvar) {\
    if (waitvar > 500000) {fprintf(stderr, "WAIT_ON_ACL: timeout\n"); break;} \
  } /* fprintf(stderr, "%d ", waitvar); /**/\
  }

#endif

#define START_BLIT()	(ACL_STATE = STATE_RESTORE | STATE_RESUME)

#define START_CPUBLIT()	(ACL_STATE = STATE_RESTORE)


#define CHECK_ACL() \
  if ( (R_CRTC(0x36) & CRTC36_ENACL) != CRTC36_ENACL) {	\
      /*fprintf(stderr, "Re-enabling ACL.\n");*/	\
      SET_KEY();					\
      W_CRTC(0x36, R_CRTC(0x36) | CRTC36_ENACL);	\
  }



#define CHECK_ACL_DONE() \
  while (ACL_STATUS & STATUS_XY) {				\
      /*fprintf(stderr, "ACL still busy!\n"); */			\
      if (ACL_STATUS & STATUS_XY) {				\
          MMU_AP0[0] = 0;					\
      } else {							\
	  /*fprintf(stderr, "...no.  Not busy anymore.\n");*/	\
      }								\
  }



/*
 *  CRTCB/SPRITE register access macros:
 */
#define CSR_PORT 0x217A


#define W_CSR_XLO(x)		etw32_set_reg(CSR_PORT, 0xE0, (x))
#define R_CSR_XLO()		etw32_get_reg(CSR_PORT, 0xE0)

#define W_CSR_XHI(x)		etw32_set_reg(CSR_PORT, 0xE1, (x))
#define R_CSR_XHI()		etw32_get_reg(CSR_PORT, 0xE1)

#define W_CSR_WLO(x)		etw32_set_reg(CSR_PORT, 0xE2, (x))
#define R_CSR_WLO()		etw32_get_reg(CSR_PORT, 0xE2)

#define W_CSR_WHI(x)		etw32_set_reg(CSR_PORT, 0xE3, (x))
#define R_CSR_WHI()		etw32_get_reg(CSR_PORT, 0xE3)

#define W_CSR_YLO(x)		etw32_set_reg(CSR_PORT, 0xE4, (x))
#define R_CSR_YLO()		etw32_get_reg(CSR_PORT, 0xE4)

#define W_CSR_YHI(x)		etw32_set_reg(CSR_PORT, 0xE5, (x))
#define R_CSR_YHI()		etw32_get_reg(CSR_PORT, 0xE5)

#define W_CSR_HLO(x)		etw32_set_reg(CSR_PORT, 0xE6, (x))
#define R_CSR_HLO()		etw32_get_reg(CSR_PORT, 0xE6)

#define W_CSR_HHI(x)		etw32_set_reg(CSR_PORT, 0xE7, (x))
#define R_CSR_HHI()		etw32_get_reg(CSR_PORT, 0xE7)

#define W_CSR_ADDRLO(x)		etw32_set_reg(CSR_PORT, 0xE8, (x))
#define R_CSR_ADDRLO()		etw32_get_reg(CSR_PORT, 0xE8)

#define W_CSR_ADDRMI(x)		etw32_set_reg(CSR_PORT, 0xE9, (x))
#define R_CSR_ADDRMI()		etw32_get_reg(CSR_PORT, 0xE9)

#define W_CSR_ADDRHI(x)		etw32_set_reg(CSR_PORT, 0xEA, (x))
#define R_CSR_ADDRHI()		etw32_get_reg(CSR_PORT, 0xEA)

#define W_CSR_ROWLO(x)		etw32_set_reg(CSR_PORT, 0xEB, (x))
#define R_CSR_ROWLO()		etw32_get_reg(CSR_PORT, 0xEB)

#define W_CSR_ROWHI(x)		etw32_set_reg(CSR_PORT, 0xEC, (x))
#define R_CSR_ROWHI()		etw32_get_reg(CSR_PORT, 0xEC)

#define W_CSR_PAN(x)		etw32_set_reg(CSR_PORT, 0xED, (x))
#define R_CSR_PAN()		etw32_get_reg(CSR_PORT, 0xED)

#define W_CSR_DEPTH(x)		etw32_set_reg(CSR_PORT, 0xEE, (x))
#define R_CSR_DEPTH()		etw32_get_reg(CSR_PORT, 0xEE)

#define W_CSR_CTRL(x)		etw32_set_reg(CSR_PORT, 0xEF, (x))
#define R_CSR_CTRL()		etw32_get_reg(CSR_PORT, 0xEF)

#define W_IMA_CTRL(x)		etw32_set_reg(CSR_PORT, 0xF7, (x))
#define R_IMA_CTRL()		etw32_get_reg(CSR_PORT, 0xF7)

#define CSRCTRL_OVERLAY		0x02	/* set sprite to overlay the screen */

#define IMACTRL_CSREN		0x80	/* enable cursor (sprite) */
