/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_regs.h	1.6"
#if (! defined(__M_REGS_INCLUDED__))

#define __M_REGS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/
#include <sidep.h>
#include "stdenv.h"

#include <sys/types.h>
#include <sys/inline.h>

#include "g_regs.h"
#include "m_opt.h"

/***
 ***	Constants.
 ***/
#define MACH_REGS8514_INC         0x0002     /*ATI 8514 names active         */
#define MACH_VESA8514_INC         0x0001 

#if defined(ATBUS)

#define MACH_REGISTER_SETUP_ID1            0x0100 /* (R) Setup ID 1 */
#define MACH_REGISTER_SETUP_ID2            0x0101 /* (R) Setup ID 2 */
#define MACH_REGISTER_SETUP_OPT            0x0102 /* (RW) setup options */

#endif /* AT Bus */

#if defined(MICROCHANNEL)
/*
 * Setup registers : Microchannel
 */
#define MACH_REGISTER_SETUP_ID1_MC         0x0100 /* (R) address of id byte 1 */
#define MACH_REGISTER_SETUP_ID1_MC_VALUE   0x0089 /* value of id byte 1 */
#define MACH_REGISTER_SETUP_ID2_MC         0x0101 /* (R) address of id byte 2 */
#define MACH_REGISTER_SETUP_ID2_MC_VALUE   0x0080 /* value of id byte 2 */
#define MACH_REGISTER_SETUP_OPT_MC         0x0102 /* (RW) setup options */
#define MACH_REGISTER_ROM_SETUP_MC         0x0103 /* (W) rom setup */
#define MACH_REGISTER_SETUP_1_MC           0x0104 /* (W) setup byte 1 */
#define MACH_REGISTER_SETUP_2_MC           0x0105 /* (W) setup byte 2 */

#endif /* Microchannel */

#if defined(EISA)

/*
 * Setup registers  : EISA
 */
#define MACH_REGISTER_SETUP_ID1_EISA         0x0C80	/* (R) address of id byte 1 */
#define MACH_REGISTER_SETUP_ID1_EISA_VALUE   0x0006	/* value of id byte 1 */
#define MACH_REGISTER_SETUP_ID2_EISA         0x0C81	/* (R) address of id byte 2 */
#define MACH_REGISTER_SETUP_ID2_EISA_VALUE   0x0089	/* value of id byte 2 */
#define MACH_REGISTER_SETUP_ID3_EISA         0x0C82	/* (R) address of id byte 3 */
#define MACH_REGISTER_SETUP_ID3_EISA_VALUE   0x0044	/* value of id byte 3 */
#define MACH_REGISTER_SETUP_ID4_EISA         0x0C83	/* (R) address of id byte 4 */
#define MACH_REGISTER_SETUP_ID4_EISA_VALUE   0x0000	/* value of id byte 4 */
#define MACH_REGISTER_SETUP_OPTION_EISA      0x0C84	/* (RW) address of setup option */
#define MACH_REGISTER_ROM_SETUP_EISA         0x0C85	/* (W) rom base address setup */
#define MACH_REGISTER_SETUP_1_EISA           0x0C86	/* (W) setup byte 1 */
#define MACH_REGISTER_SETUP_2_EISA           0x0C87	/* (W) setup byte 2 */

#endif /* EISA */


#define MACH_REGISTER_H_TOTAL              0x02E8 /* (W) Horizontal Total */
#define MACH_REGISTER_DISP_STATUS          0x02E8 /* (R) Display status */
#define MACH_REGISTER_DAC_MASK             0x02EA /* (RW) DAC Mask */
#define MACH_REGISTER_ATT_MODE_CNTL        MACH_REGISTER_DAC_MASK /* -alias- */
#define MACH_REGISTER_DAC_R_INDEX          0x02EB /* (RW) DAC Read Index */
#define MACH_REGISTER_DAC_W_INDEX          0x02EC /* (RW) DAC Write Index */
#define MACH_REGISTER_DAC_DATA             0x02ED /* (RW) RGB Color Value */
#define MACH_REGISTER_OVERSCAN_COLOR_8     0x02EE /* (W) Overscan color 8 */
#define MACH_REGISTER_OVERSCAN_BLUE_24     0x02EF /* (W) Overscan blue 24 */

#define MACH_REGISTER_H_DISP               0x06E8 /* (W) Horizontal Displayed */
#define MACH_REGISTER_OVERSCAN_GREEN_24    0x06EE /* (W) Overscan green 24 */
#define MACH_REGISTER_OVERSCAN_RED_24      0x06EF /* (W) Overscan red 24 */

#define MACH_REGISTER_H_SYNC_STRT          0x0AE8 /* (W) Horizontal Sync Position */
#define MACH_REGISTER_CURSOR_OFFSET_LO     0x0AEE /* (W) Cursor offset lo */
#define MACH_REGISTER_H_SYNC_WID           0x0EE8 /* (W) Horizontal Sync Width */
#define MACH_REGISTER_CURSOR_OFFSET_HI     0x0EEE /* (W) Cursor offset hi */

#define MACH_REGISTER_V_TOTAL              0x12E8 /* (W) Vertical Total */
#define MACH_REGISTER_CONFIG_STATUS_1      0x12EE /* (R) Config status 1 */
#define MACH_REGISTER_HORZ_CURSOR_POSN     0x12EE /* (W) Horiz cursor position */

#define MACH_REGISTER_V_DISP               0x16E8 /* (W) Vertical Displayed */
#define MACH_REGISTER_CONFIG_STATUS_2      0x16EE /* (R) Config status 2 */
#define MACH_REGISTER_VERT_CURSOR_POSN     0x16EE /* (W) Vertical Cursor position */

#define MACH_REGISTER_V_SYNC_STRT          0x1AE8 /* (W) Vertical Sync Position */
#define MACH_REGISTER_CURSOR_COLOR_0       0x1AEE /* (W) Cursor color 0 */
#define MACH_REGISTER_FIFO_TEST_DATA       0x1AEE /* (R) Fifo test data */
#define MACH_REGISTER_CURSOR_COLOR_1       0x1AEF /* (W) Cursor color 1 */
#define MACH_REGISTER_V_SYNC_WID           0x1EE8 /* (W) Vertical Sync Width */
#define MACH_REGISTER_HORZ_CURSOR_OFFSET   0x1EEE /* (W) Horiz cursor offset */
#define MACH_REGISTER_VERT_CURSOR_OFFSET   0x1EEF /* (W) Vertical cursor offet */

#define MACH_REGISTER_DISP_CNTL            0x22E8 /* (W) CRT Configuration */
#define MACH_REGISTER_CRT_PITCH            0x26EE /* (W) CRT pitch */
#define MACH_REGISTER_CRT_OFFSET_LO        0x2AEE /* (W) CRT offset lo */
#define MACH_REGISTER_CRT_OFFSET_HI        0x2EEE /* (W) CRT Offset hi */

#define MACH_REGISTER_LOCAL_CONTROL        0x32EE /* (W) Local control */
#define MACH_REGISTER_FIFO_OPT             0x36EE /* (W) Fifo options */
#define MACH_REGISTER_MISC_OPTIONS         0x36EE /* (RW) Misc options (MACH32) */
#define MACH_REGISTER_EXT_CURSOR_COLOR_0   0x3AEE /* (W) Cursor color 0 */
#define MACH_REGISTER_FIFO_TEST_TAG        0x3AEE /* (R) Fifo test tag */
#define MACH_REGISTER_EXT_CURSOR_COLOR_1   0x3EEE /* (W) Cursor color 1 */

#define MACH_REGISTER_SUBSYS_STATUS        0x42E8 /* (R) Interrupt Status */
#define MACH_REGISTER_SUBSYS_CNTL          0x42E8 /* (W) Interrupt Control */
#define MACH_REGISTER_MEM_BNDRY            0x42EE /* (RW) Mem boundary */
#define MACH_REGISTER_ROM_PAGE             0x46E8 /* ROM Page Select */
#define MACH_REGISTER_SHADOW_CTL           0x46EE /* (W) Shadow control */
#define MACH_REGISTER_GRAPH_MODE           0x4AE8 /* (W) Graphics Mode Control */
#define MACH_REGISTER_ADVFUNC_CNTL         MACH_REGISTER_GRAPH_MODE	/* -alias- */
#define MACH_REGISTER_CLOCK_SEL            0x4AEE /* (RW?) Clock select */

#define MACH_REGISTER_SCRATCH_PAD_0        0x52EE /* (RW) Scratch Pad
													 Register */
#define MACH_REGISTER_ROM_ADDR_1           0x52EE /* -alias- */

#define MACH_REGISTER_SCRATCH_PAD_1        0x56EE /* (RW) Scratch Pad Register */
#define MACH_REGISTER_SHADOW_SET           0x5AEE /* (W) Shadow Set */
#define MACH_REGISTER_MEM_CFG              0x5EEE /* (RW) Mem config */

#define MACH_REGISTER_EXT_GE_STATUS        0x62EE /* (R) Ext GE status */
#define MACH_REGISTER_HORZ_OVERSCAN        0x62EE /* (W) Horiz overscan */
#define MACH_REGISTER_VERT_OVERSCAN        0x66EE /* (W) Vertical overscan */
#define MACH_REGISTER_MAX_WAITSTATES       0x6AEE /* (RW) max waitstates */
#define MACH_REGISTER_GE_OFFSET_LO         0x6EEE /* (W) GE offset low */

#define MACH_REGISTER_BOUNDS_LEFT          0x72EE /* (R) bounds left */
#define MACH_REGISTER_GE_OFFSET_HI         0x72EE /* (W) GE offset hi */
#define MACH_REGISTER_BOUNDS_TOP           0x76EE /* (R) bounds top */
#define MACH_REGISTER_GE_PITCH             0x76EE /* (W) GE pitch */
#define MACH_REGISTER_BOUNDS_RIGHT         0x7AEE /* (R) bounds right */
#define MACH_REGISTER_EXT_GE_CONFIG        0x7AEE /* (W) Ext GE config */
#define MACH_REGISTER_BOUNDS_BOTTOM        0x7EEE /* (R) bounds bottom */
#define MACH_REGISTER_MISC_CNTL            0x7EEE /* (W) Misc control */

#define MACH_REGISTER_CYP                  0x82E8 /* (RW) Current Y Position */
#define MACH_REGISTER_CUR_Y                MACH_REGISTER_CYP /* -alias- */
#define MACH_REGISTER_PATT_DATA_INDEX      0x82EE /* (RW) Pattern data index */
#define MACH_REGISTER_CXP                  0x86E8 /* (RW) Current X Position */
#define MACH_REGISTER_CUR_X                MACH_REGISTER_CXP /* -alias- */
#define MACH_REGISTER_DEST_Y               0x8AE8 /* (W) Copy Y Destination */
#define MACH_REGISTER_SRC_Y                0x8AE8 /* (W) Source Y */
#define MACH_REGISTER_BRES_INCR1           0x8AE8 /* (W) Bresenham Line Incr 1 */
#define MACH_REGISTER_AXSTP                MACH_REGISTER_BRES_INCR1 /* -alias- */
#define MACH_REGISTER_DEST_X               0x8EE8 /* (W) Copy X Destination */
#define MACH_REGISTER_SRC_X                0x8EE8 /* (W) Src X (8514) */
#define MACH_REGISTER_BRES_INCR2           0x8EE8 /* Bresenham Line Incr 2 */
#define MACH_REGISTER_DIASTP               0x8EE8 /* Diagonal step */
#define MACH_REGISTER_PATT_DATA            0x8EEE /* (W) Pattern data */
#define MACH_REGISTER_R_EXT_GE_CONFIG      0x8EEE /* (R) Ext GE config */

#define MACH_REGISTER_BRES_D               0x92E8 /* (RW) Bresenham D */
#define MACH_REGISTER_ERR_TERM             MACH_REGISTER_BRES_D /* -alias- */
#define MACH_REGISTER_R_MISC_CNTL          0x92EE /* (R) Misc Cntl */
#define MACH_REGISTER_SRC_CMP_FN		   0x92EE /* (W) Src cmp fn */
#define MACH_REGISTER_RECT_WIDTH           0x96E8 /* Rectangle Width */
#define MACH_REGISTER_BRES_LENGTH          0x96E8 /* Bresenham Line Length (8514) */
#define MACH_REGISTER_MAJ_AXIS_PCNT        0x96E8 /* (W) Major axis pixel count */
#define MACH_REGISTER_BRES_COUNT           0x96EE /* (RW) Bresenham count */
#define MACH_REGISTER_GE_STAT              0x9AE8 /* (R) Graphics Engine Status */
#define MACH_REGISTER_DRAW_COMMAND         0x9AE8 /* (W) Draw Command */
#define MACH_REGISTER_CMD                  MACH_REGISTER_DRAW_COMMAND /* -alias- */
#define MACH_REGISTER_EXT_FIFO_STATUS      0x9AEE /* (R) Ext fifo status */
#define MACH_REGISTER_LINEDRAW_INDEX       0x9AEE /* (W) Linedraw index */
#define MACH_REGISTER_SHORT_STROKE         0x9EE8 /* (W) Short Stroke Vector */

#define MACH_REGISTER_BKGD_COLOR           0xA2E8 /* (W) Background Color */
#define MACH_REGISTER_LINEDRAW_OPT         0xA2EE /* (RW) Linedraw options */
#define MACH_REGISTER_FRGD_COLOR           0xA6E8 /* (W) Foreground Color */
#define MACH_REGISTER_DEST_X_START         0xA6EE /* (W) Destination X start */
#define MACH_REGISTER_WRT_MASK             0xAAE8 /* (W) Write Mask */
#define MACH_REGISTER_DEST_X_END           0xAAEE /* (W) Destination X end */
#define MACH_REGISTER_RD_MASK              0xAEE8 /* (W) Read Mask */
#define MACH_REGISTER_DEST_Y_END           0xAEEE /* (W) Destination Y end */

#define MACH_REGISTER_CMP_COLOR            0xB2E8 /* (W) Dest Compare Color */
#define MACH_REGISTER_R_H_TOTAL_AND_DISP   0xB2EE /* (R) Horiz total and disp */
#define MACH_REGISTER_SRC_X_START          0xB2EE /* (W) Src X start */
#define MACH_REGISTER_BKGD_MIX             0xB6E8 /* (W) Background Mix (8514) */
#define MACH_REGISTER_ALU_BG_FN            0xB6EE /* (W) Background alu */
#define MACH_REGISTER_R_H_SYNC_STRT        0xB6EE /* (R) Horiz Sync Start */
#define MACH_REGISTER_FRGD_MIX             0xBAE8 /* (W) Foreground Mix (8514) */
#define MACH_REGISTER_ALU_FG_FN            0xBAEE /* (W) Foreground Mix */
#define MACH_REGISTER_R_H_SYNC_WID         0xBAEE /* (R) Horiz Sync Width */
#define MACH_REGISTER_MULTI_FN             0xBEE8 /* (W) Multifunction Control */
#define MACH_REGISTER_SRC_X_END            0xBEEE /* (W) Src X End */

#define MACH_REGISTER_R_V_TOTAL            0xC2EE /* (R) Vertical Total */
#define MACH_REGISTER_SRC_Y_DIR            0xC2EE /* (W) Src Y Dir */
#define MACH_REGISTER_EXT_SHORT_STROKE     0xC6EE /* (W) Short stroke vectors */
#define MACH_REGISTER_R_V_DISP             0xC6EE /* (R) Vertical Display */
#define MACH_REGISTER_R_V_SYNC_STRT        0xCAEE /* (R) Vertical Sync Start */
#define MACH_REGISTER_SCAN_X               0xCAEE /* (W) Scan to X register */
#define MACH_REGISTER_DP_CONFIG            0xCEEE /* (W) Datapath config */
#define MACH_REGISTER_VERT_LINE_CNTR       0xCEEE /* (R) Vertical Line Counter */

#define MACH_REGISTER_PATT_LENGTH          0xD2EE /* (W) Pattern length */
#define MACH_REGISTER_R_V_SYNC_WID         0xD2EE /* (R) Vertical Sync Width */
#define MACH_REGISTER_PATT_INDEX           0xD6EE /* (RW) Pattern index */
#define MACH_REGISTER_EXT_SCISSOR_L        0xDAEE /* (W) Scissor left */
#define MACH_REGISTER_R_SRC_X              0xDAEE /* (R) Src X */
#define MACH_REGISTER_EXT_SCISSOR_T        0xDEEE /* (W) Scissor top */
#define MACH_REGISTER_R_SRC_Y              0xDEEE /* (R) Src Y */

#define MACH_REGISTER_PIX_TRANS            0xE2E8 /* (RW) CPU Data Transfer */
#define MACH_REGISTER_EXT_SCISSOR_R        0xE2EE /* (W) Scissor right */
#define MACH_REGISTER_EXT_SCISSOR_B        0xE6EE /* (W) Scissor bottom */
#define MACH_REGISTER_DEST_CMP_FN          0xEEEE /* (W) Destination compare fn */

#define MACH_REGISTER_LINEDRAW             0xFEEE /* (W) Linedraw */

/*
 * DISP CNTL
 */
#define MACH_DISP_CNTL_Y_CONTROL           0x0006U

/*
 * Graphics Mode Control Register (GRAPH_MODE)
 */
#define MACH_RESOLUTION           0x0004 
#define MACH_PASS_THROUGH         0x0001 

/*
 * Horizontal Sync Width (H_SYNC_WIDTH)
 */
#define MACH_H_POLARITY_SHIFT	  5U	
#define MACH_H_POLARITY           0x0020 
#define MACH_H_WIDTH              0x001F 

/*
 * Vertical Sync Width (V_SYNC_WIDTH)
 */
#define MACH_V_POLARITY_SHIFT	  5U	
#define MACH_V_POLARITY           0x0020 
#define MACH_V_WIDTH              0x001F 

/*
 * CRT Configuration Register (CRT_CONFIG)
 */
#define MACH_ENABLE_DISPLAY       0x0060 
#define MACH_ENABLE_DISPLAY_SHIFT 0x0005 
#define MACH_ENABLE_DISPLAY_NO_CHANGE 	    0x0000 
#define MACH_ENABLE_DISPLAY_ENABLE 0x0020 
#define MACH_ENABLE_DISPLAY_RESET           0x0040 
#define MACH_ENABLE_DISPLAY_RESET2 0x0060 
#define MACH_INTERLACE            0x0010 
#define MACH_INTERLACE_SHIFT	  4U
#define MACH_DOUBLE_SCAN          0x0008 
#define MACH_Y_CONTROL            0x0006 
#define MACH_Y_CONTROL_SHIFT      0x0001 
#define MACH_Y_CONTROL_SKIP_1_2   0x0000 
#define MACH_Y_CONTROL_SKIP_2     0x0002 

/*
 * Interrupt Control Register (INT_CTL)
 */
#define MACH_GE_RESET             0xC000 
#define MACH_GE_RESET_SHIFT       0x000E 
#define MACH_GE_RESET_NO_CHANGE   0x0000 
#define MACH_GE_RESET_NORMAL      0x4000 
#define MACH_GE_RESET_RESET       0x8000 
#define MACH_HW_TEST              0x3000 
#define MACH_HW_TEST_SHIFT        0x000C 
#define MACH_HW_TEST_NO_CHANGE    0x0000 
#define MACH_HW_TEST_OFF          0x1000 
#define MACH_HW_TEST_ON           0x2000 
#define MACH_FIFO_EMPTY_ENA       0x0800 
#define MACH_INVALID_IO_ENA       0x0400 
#define MACH_INSIDE_SCISSOR_ENA   0x0200 
#define MACH_VBLANK_ENA           0x0100 
#define MACH_FIFO_EMPTY_ACK       0x0008 
#define MACH_INVALID_IO_ACK       0x0004 
#define MACH_INSIDE_SCISSOR_ACK   0x0002 
#define MACH_VBLANK_ACK           0x0001 

/*
 * MISC_OPTIONS
 */

#define MACH_MISC_OPTIONS_W_STATE_ENA		0x0001U
#define MACH_MISC_OPTIONS_HOST_8_ENA		0x0002U
#define MACH_MISC_OPTIONS_MEM_SIZE_ALIAS	0x000CU
#define MACH_MISC_OPTIONS_MEM_SIZE_ALIAS_SHIFT	0x0002U
#define MACH_MISC_OPTIONS_512_KB			0x0000U
#define MACH_MISC_OPTIONS_1_MB				0x0004U
#define MACH_MISC_OPTIONS_2_MB				0x0008U
#define MACH_MISC_OPTIONS_4_MB				0x000CU
#define MACH_MISC_OPTIONS_DISABLE_ENA		0x0010U
#define MACH_MISC_OPTIONS_16_BIT_IO			0x0020U
#define MACH_MISC_OPTIONS_DISABLE_DAC		0x0040U

/*
 * ROM Page Selection Register (ROM_PAGE)
 */
#define MACH_ROM_PAGE_SEL_MASK    0x0007 

/*
 * SUBSYS_STATUS
 */

#define MACH_MEM_SIZE             0x0080           
#define MACH_MONITOR_ID           0x0070U
#define MACH_FIFO_EMPTY_INT       0x0008 
#define MACH_INVALID_IO_INT       0x0004 
#define MACH_INSIDE_SCISSOR_INT   0x0002 
#define MACH_VBLANK_INT           0x0001 
#define MACH_MONITOR_ID_SHIFT     0x0004U
#define MACH_MONITOR_ID_8514A     0x0020 
#define MACH_MONITOR_ID_VGA8503   0x0050 
#define MACH_MONITOR_ID_VGA8513   0x0060 
#define MACH_MONITOR_ID_VGA8512   0x0060 
#define MACH_MONITOR_ID_NOMON     0x0070 
#define MACH_MONITOR_ID_HIRES     0x0040     /* hires bit */
#define MACH_MONITOR_ID_COLOR     0x0020     /* color bit */

/*                                                                     
 * Graphics Engine Status (GE_STATUS)                    
 */
#define MACH_HW_BUSY              0x0200 
#define MACH_DATA_READY           0x0100 
#define MACH_FIFO_OCCUPIED        0x00FF 

/*
 * Video Status (VIDEO_STATUS)
 */
#define MACH_LINE_COUNT           0x0004 
#define MACH_VERT_SYNC            0x0002 
#define MACH_RGB_TEST             0x0001 

/*
 * Draw Command (DRAW_COMMAND)
 */
/* opcode field */
#define MACH_OP_CODE              0xE000 
#define MACH_OP_CODE_SHIFT        0x000D 
#define MACH_DRAW_SETUP           0x0000 
#define MACH_DRAW_LINE            0x2000 
#define MACH_FILL_RECT_H1H4       0x4000 
#define MACH_FILL_RECT_V1V2       0x6000 
#define MACH_FILL_RECT_V1H4       0x8000 
#define MACH_DRAW_POLY_LINE       0xA000 
#define MACH_BITBLT               0xC000 
#define MACH_DRAW_FOREVER         0xE000 
/* swap field */
#define MACH_LSB_FIRST            0x1000 
/* data width field */
#define MACH_DATA_WIDTH           0x0200 
#define MACH_BIT16                0x0200 
#define MACH_BIT8                 0x0000 
/* CPU wait field */
#define MACH_CPU_WAIT             0x0100 
/* octant field */
#define MACH_OCTANT               0x00E0 
#define MACH_OCTANT_SHIFT         0x0005 
#define MACH_YPOSITIVE            0x0080 
#define MACH_YMAJOR               0x0040 
#define MACH_XPOSITIVE            0x0020 
/* draw field */
#define MACH_DRAW                 0x0010 
/* direction field */
#define MACH_DIR_TYPE             0x0008 
#define MACH_DEGREE               0x0008 
#define MACH_XY                   0x0000 
#define MACH_RECT_RIGHT_AND_DOWN  0x00E0     /* quadrant 3 */
#define MACH_RECT_LEFT_AND_UP     0x0000     /* quadrant 1 */
/* last pel off field */
#define MACH_LAST_PEL_OFF_SHIFT   0x0002 
#define MACH_LAST_PEL_OFF         0x0004 
#define MACH_LAST_PEL_ON          0x0000 
/* pixel mode */
#define MACH_PIXEL_MODE           0x0002 
#define MACH_MULTI                0x0002 
#define MACH_SINGLE               0x0000 
/* read/write */
#define MACH_RW                   0x0001 
#define MACH_WRITE                0x0001 
#define MACH_READ                 0x0000 

/*
 * Foreground Mix (FG_MIX)
 * Background Mix (BG_MIX)
 */
#define MACH_IBM_COLOR_SRC        0x0060 
#define MACH_IBM_COLOR_SRC_SHIFT  0x0005 
#define MACH_IBM_COLOR_SRC_BG     0x0000 
#define MACH_IBM_COLOR_SRC_FG     0x0020 
#define MACH_IBM_COLOR_SRC_HOST   0x0040 
#define MACH_IBM_COLOR_SRC_BLIT   0x0060
#define MACH_IBM_COLOR			  0x00FF
#define MACH_MIX_FN               0x001F 
#define MACH_MIX_FN_SHIFT         0x0000 
#define MACH_MIX_FN_NOT_D         0x0000 
#define MACH_MIX_FN_ZERO          0x0001 
#define MACH_MIX_FN_ONE           0x0002 
#define MACH_MIX_FN_LEAVE_ALONE   0x0003 
#define MACH_MIX_FN_NOT_S         0x0004 
#define MACH_MIX_FN_XOR           0x0005 
#define MACH_MIX_FN_XNOR          0x0006 
#define MACH_MIX_FN_PAINT         0x0007 
#define MACH_MIX_FN_NAND          0x0008 
#define MACH_MIX_FN_D_OR_NOT_S    0x0009 
#define MACH_MIX_FN_NOT_D_OR_S    0x000A 
#define MACH_MIX_FN_OR            0x000B 
#define MACH_MIX_FN_AND           0x000C 
#define MACH_MIX_FN_NOT_D_AND_S   0x000D 
#define MACH_MIX_FN_D_AND_NOT_S   0x000E 
#define MACH_MIX_FN_NOR           0x000F 
#define MACH_MIX_FN_MIN           0x0010 
#define MACH_MIX_FN_SUBS          0x0011 
#define MACH_MIX_FN_SUBD          0x0012 
#define MACH_MIX_FN_ADD           0x0013 
#define MACH_MIX_FN_MAX           0x0014 
#define MACH_MIX_FN_AVG           0x0017 
#define MACH_MIX_FN_SUBSZ         0x0018     /* subtract source, saturate */
#define MACH_MIX_FN_SUBDZ         0x001A     /* subtract dest, saturate */
#define MACH_MIX_FN_ADDS          0x001B     /* add with saturation */

/*
 * Multifunction Register (MULTI_FN)
 */
#define MACH_MF_SELECT            0xF000 
#define MACH_MF_SELECT_SHIFT      0x000C 
#define MACH_MF_RECT_HEIGHT       0x0000 
#define MACH_MF_SCISSOR_TOP       0x1000 
#define MACH_MF_SCISSOR_LEFT      0x2000 
#define MACH_MF_SCISSOR_BOTTOM    0x3000 
#define MACH_MF_SCISSOR_RIGHT     0x4000 
#define MACH_MF_GE_CONFIG         0x5000 
#define MACH_MF_PATTERN_LOW       0x8000 
#define MACH_MF_PATTERN_HIGH      0x9000 
#define MACH_MF_ALU_CONFIG        0xA000 
#define MACH_MF_VALUE             0x0FFF 
#define MACH_PLANE_SELECT         0x0010 
#define MACH_GE_Y_CONTROL         0x000C 
#define MACH_GE_Y_CONTROL_SHIFT   0x0002 
#define MACH_GE_Y_CONTROL_SKIP_1_2 0x0000    /* 640x480x4 */
#define MACH_GE_Y_CONTROL_LINEAR  0x0004     /* linear */
#define MACH_GE_X_CONTROL         0x0003 
#define MACH_GE_X_CONTROL_SHIFT   0x0000 
#define MACH_GE_X_CONTROL_NORMAL  0x0002     /* 640x480x4 */
#define MACH_MF_ALU_CONFIG_MONO_SRC_BLIT	0x00C0

/*
 * Write mask in IBM mode is at the most 8 bits wide.
 */
#define MACH_IBM_RD_WRT_MASK				0x00FF

/*
 * DRAW_CMD register.
 */
#define MACH_CMD_NOOP_CMD					0x0000
#define MACH_CMD_DRAW_LINE_CMD				0x2000
#define MACH_CMD_FILL_RECT_HOR_CMD			0x4000
#define MACH_CMD_FILL_RECT_VPIX_CMD			0x6000
#define MACH_CMD_FILL_RECT_VNIB_CMD			0x8000
#define MACH_CMD_FILL_DRAW_POLY_LINE_CMD	0xA000
#define MACH_CMD_BLIT_CMD					0xC000
#define MACH_CMD_RESERVED_CMD				0xE000
#define MACH_CMD_LSB_FIRST					0x1000
#define MACH_CMD_DATA_WIDTH_USE_16			0x0200
#define MACH_CMD_CPU_WAIT					0x0100
#define MACH_CMD_YPOS						0x0080
#define MACH_CMD_YMAJOR						0x0040
#define MACH_CMD_XPOS						0x0020
#define MACH_CMD_DRAW						0x0010
#define MACH_CMD_DIR_TYPE_DEGREE			0x0008
#define MACH_CMD_LAST_PEL_OFF				0x0004
#define MACH_CMD_PIXEL_MODE_NIBBLE			0x0002
#define MACH_CMD_WRITE						0x0001
#define MACH_CMD_0_DEGREE_RADIAL_LINE		0x0000
#define MACH_CMD_45_DEGREE_RADIAL_LINE		0x0020
#define MACH_CMD_90_DEGREE_RADIAL_LINE		0x0040
#define MACH_CMD_135_DEGREE_RADIAL_LINE		0x0060
#define MACH_CMD_180_DEGREE_RADIAL_LINE		0x0080
#define MACH_CMD_225_DEGREE_RADIAL_LINE		0x00a0
#define MACH_CMD_270_DEGREE_RADIAL_LINE		0x00c0
#define MACH_CMD_315_DEGREE_RADIAL_LINE		0x00e0

/*
 * IBM 8514 mode : selection of the foreground and background sources.
 */
#define MACH_IBM_SELECT_FRGD_COLOR	0x0020
#define MACH_IBM_SELECT_BKGD_COLOR  0x0000
#define MACH_IBM_SELECT_BLIT		0x0060

/*
 * ALU Configuration : (DP CONFIG)
 */
#define MACH_DP_CONFIG_FG_COLOR_SRC    				0xE000
#define MACH_DP_CONFIG_FG_COLOR_SRC_SHIFT			0xD
#define MACH_DP_CONFIG_FG_COLOR_SRC_BKGD_COLOR    	0x0000
#define MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR		0x2000
#define MACH_DP_CONFIG_FG_COLOR_SRC_HOST			0x4000
#define MACH_DP_CONFIG_FG_COLOR_SRC_BLIT			0x6000
#define MACH_DP_CONFIG_FG_COLOR_SRC_PATT			0xA000
#define MACH_DP_CONFIG_LSB_FIRST					0x1000
#define MACH_DP_CONFIG_RESERVED						0x0C00
#define MACH_DP_CONFIG_DATA_WIDTH_USE_16			0x0200
#define MACH_DP_CONFIG_BG_COLOR_SRC					0x0180
#define MACH_DP_CONFIG_BG_COLOR_SRC_SHIFT			0x7
#define MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR    	0x0000
#define MACH_DP_CONFIG_BG_COLOR_SRC_FRGD_COLOR		0x0080
#define MACH_DP_CONFIG_BG_COLOR_SRC_HOST			0x0100
#define MACH_DP_CONFIG_BG_COLOR_SRC_BLIT			0x0110
#define MACH_DP_CONFIG_MONO_SRC         			0x0060 
#define MACH_DP_CONFIG_MONO_SRC_SHIFT   			0x5
#define MACH_DP_CONFIG_MONO_SRC_ONE     			0x0000
#define MACH_DP_CONFIG_MONO_SRC_PATT    			0x0020
#define MACH_DP_CONFIG_MONO_SRC_HOST    			0x0040
#define MACH_DP_CONFIG_MONO_SRC_BLIT    			0x0060
#define MACH_DP_CONFIG_ENABLE_DRAW					0x0010
#define MACH_DP_CONFIG_UNUSED						0x0008
#define MACH_DP_CONFIG_READ_MODE_MONO_DATA			0x0004
#define MACH_DP_CONFIG_POLY_FILL_MODE_ENABLE		0x0002
#define MACH_DP_CONFIG_WRITE						0x0001

#define MACH_DP_CONFIG_RESET_EXTENDED_DATA_PATHS	0x2211

#define MACH_COLOR_CMP_FN         0x0038 
#define MACH_COLOR_CMP_FN_SHIFT   0x0003 
#define MACH_COLOR_CMP_FN_FALSE   0x0000 
#define MACH_COLOR_CMP_FN_TRUE    0x0008 
#define MACH_COLOR_CMP_FN_GE      0x0010 
#define MACH_COLOR_CMP_FN_LT      0x0018 
#define MACH_COLOR_CMP_FN_NE      0x0020 
#define MACH_COLOR_CMP_FN_EQ      0x0028 
#define MACH_COLOR_CMP_FN_LE      0x0030 
#define MACH_COLOR_CMP_FN_GT      0x0038 

#define MACH_MONO_READ            0x0004 
#define MACH_POLY_FILL_ENABLE     0x0004 
#define MACH_POLY_FILL_TYPE       0x0002 

/*
 * Raw Bresenham Linedraw and Miscellaneous Registers
 *    the suffix "MASK" is used when there is only one (eponymous) field
 *    in a register
 */
#define MACH_CXP_MASK             0x07FF 
#define MACH_CYP_MASK             0x07FF 
#define MACH_BRES_INCR1_MASK      0x1FFF 
#define MACH_BRES_INCR2_MASK      0x1FFF 
#define MACH_BRES_D_MASK          0x1FFF 
#define MACH_BRES_LENGTH_MASK     0x07FF 
#define MACH_POS_SETUP_MASK       0x0001 
#define MACH_RECT_WIDTH_MASK      0x07FF 
#define MACH_DESTX_MASK           0x07FF 
#define MACH_DESTY_MASK           0x07FF 

#define MACH_WAIT_IO              0x4000 

#define MACH_SELECT_PASSTHROUGH_8514     0x0001

/*
 * CONFIG_STATUS_1
 */

#define MACH8_CLK_MODE				0x0001
#define MACH8_BUS_16				0x0002
#define MACH8_BUS_16_SHIFT			0x0001U
#define MACH8_MC_BUS				0x0004
#define MACH8_MC_BUS_SHIFT			0x0002U
#define MACH8_EEPROM_ENA			0x0008
#define MACH8_EEPROM_ENA_SHIFT		0x0003U
#define MACH8_DRAM_ENA				0x0010
#define MACH8_DRAM_ENA_SHIFT		0x0004U
#define MACH8_MEM_INSTALLED			0x0060
#define MACH8_MEM_INSTALLED_SHIFT	0x0005U
#define MACH8_ROM_ENA				0x0080
#define MACH8_ROM_ENA_SHIFT			0x0007U
#define MACH8_ROM_PAGE_ENA			0x0100
#define MACH8_ROM_PAGE_ENA_SHIFT	0x0008U
#define MACH8_ROM_LOCATION			0xFE00
#define MACH8_ROM_LOCATION_SHIFT	0x0009U

#define MACH32_8514_ONLY            0x0001U
#define MACH32_BUS_TYPE             0x000EU
#define MACH32_BUS_TYPE_SHIFT       1U
#define MACH32_MEM_TYPE             0x0070U
#define MACH32_MEM_TYPE_SHIFT       4U
#define MACH32_CHIP_DIS             0x0080U
#define MACH32_CHIP_DIS_SHIFT       7U
#define MACH32_L_BUS_102_DECODE_ENA 0x0100U
#define MACH32_DAC_TYPE             0x0E00U
#define MACH32_DAC_TYPE_SHIFT       9U
#define MACH32_MC_ADR_DECODE        0x1000U
#define MACH32_CARD_ID              0xE000U

/*
 * CONFIG STATUS 2
 */
#define MACH8_SHARE_CLOCK                   0x0001
#define MACH8_HI_RES_BOOT                   0x0002
#define MACH8_EPROM_16_ENA                  0x0004
#define MACH8_WRITE_PER_BIT                 0x0008
#define MACH8_FLASH_ENA                     0x0010

#define MACH32_SLOW_SEQ_EN                  0x0001
#define MACH32_MEM_ADDR_DIS                 0x0002
#define MACH32_MEM_ADDR_DIS_SHIFT           1U
#define MACH32_ISA_16_ENA                   0x0004
#define MACH32_ISA_16_ENA_SHIFT             2U
#define MACH32_KOR_TXT_MODE_ENA             0x0008
#define MACH32_KOR_TXT_MODE_ENA_SHIFT       3U
#define MACH32_LOCAL_BUS_SUPPORT            0x0030
#define MACH32_LOCAL_BUS_SUPPORT_SHIFT      4U
#define MACH32_LOCAL_BUS_CONFIG             0x0040
#define MACH32_LOCAL_BUS_CONFIG_SHIFT       6U
#define MACH32_LOCAL_BUS_RD_DLY_ENA         0x0080
#define MACH32_LOCAL_BUS_RD_DLY_ENA_SHIFT   7U
#define MACH32_LOCAL_DAC_EN                 0x0100
#define MACH32_LOCAL_DAC_EN_SHIFT           8U
#define MACH32_LOCAL_RDY_EN                 0x0200
#define MACH32_LOCAL_RDY_EN_SHIFT           9U
#define MACH32_EEPROM_ADR_SEL               0x0400
#define MACH32_EEPROM_ADR_SEL_SHIFT         10U
#define MACH32_GE_STRAP_SEL                 0x0800
#define MACH32_GE_STRAP_SEL_SHIFT           11U

/*
 * Register sets.
 */

#define MACH_PRIMARY_REGISTER_SET   0
#define MACH_SHADOW_REGISTER_SET_1  1
#define MACH_SHADOW_REGISTER_SET_2  2

/*
 * Locking (SHADOW CTL)
 */

#define MACH_LOCK_CRT_PARAMS        (0x0001U << 0U)
#define MACH_LOCK_Y_CONTROL         (0x0001U << 1U)
#define MACH_LOCK_H_PARAMS          (0x0001U << 2U)
#define MACH_LOCK_H_DISP            (0x0001U << 3U)
#define MACH_LOCK_V_PARAMS          (0x0001U << 4U)
#define MACH_LOCK_V_DISP            (0x0001U << 5U)
#define MACH_LOCK_OVERSCAN          (0x0001U << 6U)	/* MACH32 */

/*
 * CLOCK SEL
 */
#define MACH_CLOCK_SEL_COMPOSITE_SYNC 		0x1000U
#define MACH_CLOCK_SEL_VFIFO_DEPTH  		0x0F00U
#define MACH_CLOCK_SEL_VFIFO_DEPTH_SHIFT 	0x8U
#define MACH_CLOCK_SEL_CLK_DIV      		0x00C0U
#define MACH_CLOCK_SEL_CLK_DIV_1            0x0000U
#define MACH_CLOCK_SEL_CLK_DIV_2            0x0040U
#define MACH_CLOCK_SEL_CLK_DIV_SHIFT   		0x6U
#define MACH_CLOCK_SEL_CLK_SEL      		0x003CU
#define MACH_CLOCK_SEL_CLK_SEL_SHIFT		0x2U

/*
 * EXT GE CONFIG register.
 */
#define MACH_EXT_GE_CONFIG_DAC_8_BIT_ENABLE		0x4000U
#define	MACH_EXT_GE_CONFIG_DAC_EXT_ADDR			0x3000U
#define	MACH_EXT_GE_CONFIG_DAC_EXT_ADDR_SHIFT	12U
#define MACH_EXT_GE_CONFIG_24_BIT_COLOR_ORDER	0x0400U 
#define MACH_EXT_GE_CONFIG_24_BIT_COLOR_CONFIG	0x0200U 
#define MACH_EXT_GE_CONFIG_MULTIPLEX_PIXELS		0x0100U 
#define MACH_EXT_GE_CONFIG_16_BIT_COLOR_MODE	0x00C0U 
#define MACH_EXT_GE_CONFIG_PIXEL_WIDTH			0x0030U 
#define MACH_EXT_GE_CONFIG_Z1280                0x0010U
#define MACH_EXT_GE_CONFIG_ALIAS_ENA            0x0008U
#define MACH_EXT_GE_CONFIG_MONITOR_ALIAS        0x0007U

#define MACH_EXT_GE_CONFIG_PIXEL_WIDTH_4 		0x0000U
#define MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8 		0x0010U
#define MACH_EXT_GE_CONFIG_PIXEL_WIDTH_16 		0x0020U
#define MACH_EXT_GE_CONFIG_PIXEL_WIDTH_24 		0x0030U
#define MACH_EXT_GE_CONFIG_16_BIT_555_MODE		0x0000U
#define MACH_EXT_GE_CONFIG_16_BIT_565_MODE		0x0040U
#define MACH_EXT_GE_CONFIG_16_BIT_655_MODE		0x0080U
#define MACH_EXT_GE_CONFIG_16_BIT_664_MODE		0x00C0U
#define MACH_EXT_GE_CONFIG_24_BIT_3_BPP			0x0000U
#define MACH_EXT_GE_CONFIG_24_BIT_4_BPP			0x0200U
#define MACH_EXT_GE_CONFIG_24_BIT_RGBA_MODE		0x0000U
#define MACH_EXT_GE_CONFIG_24_BIT_ABGR_MODE		0x0400U
#define MACH_EXT_GE_CONFIG_RAMDAC_PAGE_0		0x0000U
#define MACH_EXT_GE_CONFIG_RAMDAC_PAGE_1		0x1000U
#define MACH_EXT_GE_CONFIG_RAMDAC_PAGE_2		0x2000U
#define MACH_EXT_GE_CONFIG_RAMDAC_PAGE_3		0x3000U
#define MACH_EXT_GE_CONFIG_6_BIT_DAC_OPERATION	0x0000U
#define MACH_EXT_GE_CONFIG_8_BIT_DAC_OPERATION	0x4000U

/*
 * MISC CNTL
 */
#define MACH_R_MISC_CNTL_PIXEL_DELAY 			0x0C00U
#define MACH_R_MISC_CNTL_PIXEL_DELAY_SHIFT    	10
#define MACH_R_MISC_CNTL_BLANK_ADJUST 			0x0300U
#define MACH_R_MISC_CNTL_BLANK_ADJUST_SHIFT		8
#define MACH_R_MISC_CNTL_ROM_PAGE_SEL			0x00F0U

/*
 * MEM_BNDRY
 */

#define MACH_MEM_BNDRY_MEM_PAGE_BNDRY		0x000F
#define MACH_MEM_BNDRY_MEM_BNDRY_ENA		0x0010

/*
 * MEM_CFG
 */

#define MACH_MEM_CFG_MEM_APERTURE_SEL_DISABLED	0x0000U
#define MACH_MEM_CFG_MEM_APERTURE_SEL_1_MB		0x0001U
#define MACH_MEM_CFG_MEM_APERTURE_SEL_4_MB		0x0002U
#define MACH_MEM_CFG_MEM_APERTURE_SEL_RESERVED	0x0003U
#define MACH_MEM_CFG_MEM_APERTURE_PAGE			0x000CU
#define MACH_MEM_CFG_MEM_APERTURE_PAGE_0		0x0000U
#define MACH_MEM_CFG_MEM_APERTURE_PAGE_SHIFT	0x2U
#define MACH_MEM_CFG_RESERVED					0x00F0U
#define MACH_MEM_CFG_MEM_APERTURE_LOC			0xFF00U
#define MACH_MEM_CFG_MEM_APERTURE_LOC_SHIFT		0x8U

/*
 * CURSOR OFFSET HI
 */
#define MACH_CURSOR_OFFSET_HI_CURSOR_ENA        0x8000U

/*
 * LOCAL CONTROL 
 */
#define MACH_LOCAL_CONTROL_DAC_BLANK_ADJ        0x0008U

/*
 * LINEDRAW_OPT
 */
#define MACH_LINEDRAW_OPT_POLY_MODE             0x0002U
#define MACH_LINEDRAW_OPT_LAST_PEL_OFF          0x0004U
#define MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE       0x0008U
#define MACH_LINEDRAW_OPT_OCTANT_DEGREE         0x00E0U
#define MACH_LINEDRAW_OPT_OCTANT_DEGREE_SHIFT   5U
#define MACH_LINEDRAW_OPT_0_DEGREE_RADIAL_LINE		0x0000U
#define MACH_LINEDRAW_OPT_45_DEGREE_RADIAL_LINE		0x0020U
#define MACH_LINEDRAW_OPT_90_DEGREE_RADIAL_LINE		0x0040U
#define MACH_LINEDRAW_OPT_135_DEGREE_RADIAL_LINE	0x0060U
#define MACH_LINEDRAW_OPT_180_DEGREE_RADIAL_LINE	0x0080U
#define MACH_LINEDRAW_OPT_225_DEGREE_RADIAL_LINE	0x00A0U
#define MACH_LINEDRAW_OPT_270_DEGREE_RADIAL_LINE	0x00C0U
#define MACH_LINEDRAW_OPT_315_DEGREE_RADIAL_LINE	0x00E0U
#define MACH_LINEDRAW_OPT_XPOS					0x0020U
#define MACH_LINEDRAW_OPT_YMAJOR				0x0040U
#define MACH_LINEDRAW_OPT_YPOS					0x0080U
#define MACH_LINEDRAW_OPT_BOUND_RESET           0x0100U
#define MACH_LINEDRAW_OPT_CLIP_MODE             0x6000U
#define MACH_LINEDRAW_OPT_CLIP_MODE_SHIFT       9U
#define MACH_LINEDRAW_OPT_CLIP_MODE_DISABLED    0x0000U
#define MACH_LINEDRAW_OPT_CLIP_MODE_STROKED     0x0200U
#define MACH_LINEDRAW_OPT_CLIP_MODE_POLYGON     0x0400U
#define MACH_LINEDRAW_OPT_CLIP_MODE_PATTERN     0x0600U

/*
 * EXT_SHORT_STROKE 
 */
#define MACH_EXT_SHORT_STROKE_DRAW				0x0010U
#define MACH_EXT_SHORT_STROKE_LENGTH_MASK		0x000FU
#define MACH_EXT_SHORT_STROKE_DRAW_0			0x0000U
#define MACH_EXT_SHORT_STROKE_DRAW_45			0x0020U
#define MACH_EXT_SHORT_STROKE_DRAW_90			0x0040U
#define MACH_EXT_SHORT_STROKE_DRAW_135			0x0060U
#define MACH_EXT_SHORT_STROKE_DRAW_180			0x0080U
#define MACH_EXT_SHORT_STROKE_DRAW_225			0x00A0U
#define MACH_EXT_SHORT_STROKE_DRAW_270			0x00C0U
#define MACH_EXT_SHORT_STROKE_DRAW_315			0x00E0U

/*
 * MAX_WAITSTATES
 */
#define MACH_USE_HORIZONTAL_LINE_DRAW_OPTIMIZATIONS 	0x0100

/*
 * HORZ_OVERSCAN
 */
#define MACH_HORZ_OVERSCAN_H_SYNC_DELAY			0x0F00
#define MACH_HORZ_OVERSCAN_H_SYNC_DELAY_SHIFT	0x8
#define MACH_HORZ_OVERSCAN_OVERSCAN_RIGHT		0x00F00
#define MACH_HORZ_OVERSCAN_OVERSCAN_LEFT		0x000F

/***
 ***	Macros.
 ***/

/*
 * Unlocking the shadow sets.
 */
#define MACH_UNLOCK_SHADOW_SET(set)\
	/* CONSTANTCONDITION */\
	if (((set) > 0) && ((set) <= 2))\
	{\
		outw(MACH_REGISTER_SHADOW_SET, (set));\
		outw(MACH_REGISTER_SHADOW_CTL, 0);\
	}

/*
 * Lock the shadow set (note the bit 7 is undefined for MACH8...)
 */
#define MACH_LOCK_SHADOW_SET(set)\
	/* CONSTANTCONDITION */\
	if (((set > 0) && ((set) <= 2)))\
	{\
		outw(MACH_REGISTER_SHADOW_SET, (set));\
		outw(MACH_REGISTER_SHADOW_CTL, (MACH_LOCK_CRT_PARAMS |\
										MACH_LOCK_Y_CONTROL |\
										MACH_LOCK_H_PARAMS |\
										MACH_LOCK_H_DISP |\
										MACH_LOCK_V_PARAMS |\
										MACH_LOCK_V_DISP |\
										MACH_LOCK_OVERSCAN));\
	}

#define MACH_SELECT_SHADOW_SET(set)\
	/* CONSTANTCONDITION */\
	if (((set) >= 0) && ((set) <=2))\
	{\
		outw(MACH_REGISTER_SHADOW_SET, (set));\
	}\

/*
 * Resetting the graphics engine.
 */
#define MACH_RESET_GRAPHICS_ENGINE()\
{\
	outw(MACH_REGISTER_SUBSYS_CNTL,\
		 (MACH_GE_RESET_RESET |\
		  (MACH_VBLANK_ACK |\
		   MACH_INSIDE_SCISSOR_ACK | MACH_INVALID_IO_ACK |\
		   MACH_FIFO_EMPTY_ACK)));\
	outw(MACH_REGISTER_SUBSYS_CNTL,\
		 (MACH_GE_RESET_NORMAL |\
		  (MACH_VBLANK_ACK |\
		   MACH_INSIDE_SCISSOR_ACK | MACH_INVALID_IO_ACK |\
		   MACH_FIFO_EMPTY_ACK)));\
}

/*
 * Delays.
 */
#define MACH_DELAY()\
{\
	volatile int __count = mach_graphics_engine_loop_timeout_count;\
    do\
	{\
		;\
	} while(--__count>=0);\
}

#define MACH_MICRO_DELAY()\
{\
	volatile int __tmp_count = mach_graphics_engine_micro_delay_count;\
    do\
	{\
		;\
	} while(--__tmp_count>=0);\
}

#define MACH_WAIT_FOR_VERTICAL_BLANK()\
{\
	 volatile int __count = mach_graphics_engine_loop_timeout_count;\
	 for(;(__count > 0) && \
		 !(inw(MACH_REGISTER_SUBSYS_STATUS) & MACH_VBLANK_ACK);__count--)\
	 {\
		  ;\
	 }\
	if (__count <= 0)\
	{\
		(void) fprintf(stderr, "\nMACH: VBLANK TIMEOUT "\
					   "\"%s\",%d\n", __FILE__, __LINE__);\
	}\
}

#define MACH_WAIT_FOR_FIFO(N_ENTRIES)\
{\
	 if ((mach_graphics_engine_number_of_fifo_entries_free -=\
		  (N_ENTRIES)) < 0)\
	 {\
		 mach_register_wait_for_fifo(N_ENTRIES);\
	 }\
}

/*
 * The following macro allows one to pump data at the optimal rate
 * to the graphics engine.
 */
#define MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(PORT, COUNT, POINTER,\
										  PORT_SIZE_IN_BYTES_SHIFT, FUNCTION)\
{\
	register unsigned char  *__pointer = (unsigned char *) (POINTER);\
	const int __count = (COUNT);\
	const int __port_shift = (PORT_SIZE_IN_BYTES_SHIFT);\
	const unsigned char  *__last_fence = \
		__pointer + (__count << __port_shift);\
    const unsigned char  *__first_fence = \
		__pointer + \
		(((mach_graphics_engine_fifo_blocking_factor & \
		   (mach_graphics_engine_fifo_blocking_factor - 1)) ? \
		  (__count / mach_graphics_engine_fifo_blocking_factor) :\
		  (__count & ~(mach_graphics_engine_fifo_blocking_factor - 1))) <<\
		 __port_shift);\
    if (__first_fence > __pointer)\
	{\
		register int __increment =\
			mach_graphics_engine_fifo_blocking_factor <<\
				(__port_shift);\
		do\
		{\
			mach_register_wait_for_fifo(\
				mach_graphics_engine_fifo_blocking_factor);\
			FUNCTION (PORT, mach_graphics_engine_fifo_blocking_factor,\
					  __pointer);\
			__pointer += __increment;\
		} while (__pointer < __first_fence);\
	}\
	if (__last_fence > __first_fence)\
	{\
		register int __excess_count = (__last_fence - __first_fence)\
			>> (__port_shift);\
		MACH_WAIT_FOR_FIFO(__excess_count);\
		FUNCTION (PORT, __excess_count, __pointer);\
	}\
}



/*
 * Waiting for the engine to become idle.
 */
#define MACH_WAIT_FOR_ENGINE_IDLE()\
{\
	 volatile int __count = mach_graphics_engine_loop_timeout_count;\
	 for(;(__count > 0) && \
		 (inw(MACH_REGISTER_GE_STAT) & MACH_HW_BUSY);__count--)\
	 {\
		 MACH_MICRO_DELAY();\
	 }\
	 if (__count <= 0)\
	 {\
		  (void) fprintf(stderr, "\nMACH: ENGINE IDLE RESET "\
						 "\"%s\",%d\n", __FILE__,  __LINE__);\
		  ASSERT(mach_no_operation_fail() == SI_SUCCEED);\
		  MACH_RESET_GRAPHICS_ENGINE();\
	 }\
}

/*
 * Waiting for the pixtrans register to get data.
 */
#define MACH_WAIT_FOR_DATA_READY()\
{\
	 volatile int __count = mach_graphics_engine_loop_timeout_count;\
	 for(; (__count > 0) &&\
		 (!(inw(MACH_REGISTER_GE_STAT) &\
					  MACH_DATA_READY));__count--)\
	 {\
		 MACH_MICRO_DELAY();\
	 }\
	 if (__count <= 0)\
	 {\
		  (void) fprintf(stderr, "\nMACH: DATA READY RESET "\
						 "\"%s\",%d\n", __FILE__,  __LINE__);\
		  ASSERT(mach_no_operation_fail() == SI_SUCCEED);\
		  MACH_RESET_GRAPHICS_ENGINE();\
	 }\
}

/*
 * Set the DAC input to be the 8514 / MACH chip :
 * 
 */
#define MACH_PASSTHROUGH_8514(clock_sel)\
{\
	 unsigned short __t =\
		 ((clock_sel) |= MACH_SELECT_PASSTHROUGH_8514);\
	 outw(MACH_REGISTER_CLOCK_SEL, __t);\
	 MACH_DELAY();\
}

#define MACH_PASSTHROUGH_8514_ADVFUNC_CNTL(advfunc_cntl)\
{\
	 unsigned short __t =\
		 ((advfunc_cntl) |= MACH_SELECT_PASSTHROUGH_8514);\
	 outw(MACH_REGISTER_ADVFUNC_CNTL, __t);\
	 MACH_DELAY();\
}

/*
 * Set the DAC input to be from the VGA.
 */
#define MACH_PASSTHROUGH_VGA(clock_sel)\
{\
	 unsigned short __t =\
		 ((clock_sel) &= ~ MACH_SELECT_PASSTHROUGH_8514);\
	 outw(MACH_REGISTER_CLOCK_SEL, __t);\
	 MACH_DELAY();\
}
 
#define MACH_PASSTHROUGH_VGA_ADVFUNC_CNTL(advfunc_cntl)\
{\
	 unsigned short __t =\
		 ((advfunc_cntl) &= ~ MACH_SELECT_PASSTHROUGH_8514);\
	 outw(MACH_REGISTER_ADVFUNC_CNTL, __t);\
	 MACH_DELAY();\
}
 
/*
 * Disable the crt controller.
 */
#define MACH_DISABLE_CRT_CONTROLLER(disp_cntl)\
{\
	(disp_cntl) = (((disp_cntl) & (~MACH_ENABLE_DISPLAY)) |\
				   MACH_ENABLE_DISPLAY_RESET);\
	outb(MACH_REGISTER_DISP_CNTL, (disp_cntl));\
	MACH_DELAY();\
}

 
/*
 * Enable the crt controller.
 */
#define MACH_ENABLE_CRT_CONTROLLER(disp_cntl)\
{\
	(disp_cntl) = (((disp_cntl) & (~MACH_ENABLE_DISPLAY)) |\
				   MACH_ENABLE_DISPLAY_ENABLE);\
	outb(MACH_REGISTER_DISP_CNTL, (disp_cntl));\
	MACH_DELAY();\
}

#define MACH_IS_IO_ERROR()\
	(inw(MACH_REGISTER_SUBSYS_STATUS) & MACH_INVALID_IO_ACK)

#define MACH_IS_DATA_READY()\
	(inw(MACH_REGISTER_GE_STAT) & MACH_DATA_READY)

#define MACH_IS_X_OUT_OF_BOUNDS(X)\
	((X) < -512 || (X) > 1535)

#define MACH_IS_Y_OUT_OF_BOUNDS(Y)\
	((Y) < -512 || (Y) > 1535)


/***
 ***	Types.
 ***/
#if (!defined(__ASSEMBLER__))

/*
 * Register state for the mach chipsets
 */
struct mach_register_state
{
    struct generic_register_state generic_state;

    /*
     * Drawing control registers.
     */

	unsigned short bounds_bottom; /* (W) */
	unsigned short bounds_left;	/* (W) */
	unsigned short bounds_right; /* (W) */
	unsigned short bounds_top;	/* (W) */
	unsigned short bres_count;	/* (W) */
	unsigned short cmd;			/* (W) */
	unsigned short cmp_color;	/* (W) */
	unsigned short cur_x;		/* (RW) */
	unsigned short cur_y;		/* (RW) */
	unsigned short dest_x_end;	/* (RW) */
	unsigned short dest_x_start; /* (RW) */
	unsigned short dest_y_end;	/* (RW) */
	unsigned short dp_config;	/* (W) */
	unsigned short err_term;	/* (RW) */
	unsigned short ext_short_stroke; /* (W) */
	unsigned short ge_stat;		/* (R) */
	unsigned short linedraw_index; /* (W) */
	unsigned short linedraw_opt; /* (RW) */
	unsigned short maj_axis_pcnt; /* (W) */
	unsigned short min_axis_pcnt; /* (W) 0xBEE8 */
	unsigned short pattern_h;	/* (W) 0xBEE8 */
	unsigned short pattern_l;	/* (W) 0xBEE8 */
	unsigned short patt_data[
		 DEFAULT_MACH_NUMBER_OF_COLOR_PATTERN_DATA_REGISTERS +
		 DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS]; /* (W) */
	unsigned short patt_data_index;	/* (W) */
	unsigned short pix_cntl;	/* (W) 0xBEE8 */
	unsigned short pix_trans;	/* (W) */
	unsigned short scan_to_x;	/* (W) */
	unsigned short short_stroke; /* (W) */
	unsigned short src_x;		/* (W) */
	unsigned short src_x_destx_diastp; /* (W) */
	unsigned short src_x_end;	/* (W) */
	unsigned short src_x_start;	/* (W) */
	unsigned short src_y;		/* (W) */
	unsigned short src_y_dir;	/* (W) */
	unsigned short src_y_desty_diastp; /* (W) */
	
	/*
	 * CRT control registers.
	 */
	unsigned short advfunc_cntl; /* (W) */
	unsigned short clock_sel;	/* (W) */
	unsigned short crt_offset_hi; /* (W) */
	unsigned short crt_offset_lo; /* (W) */
	unsigned short crt_pitch;	/* (W) */
	unsigned short disp_cntl;	/* (W) */
	unsigned short h_total;		/* (W) */
	unsigned short h_disp;		/* (W) */
	unsigned short h_sync_strt;	/* (W) */
	unsigned short h_sync_wid;	/* (W) */
	unsigned short shadow_ctl;	/* (W) */
	unsigned short shadow_set;	/* (W) */
	unsigned short v_total;		/* (W) */
	unsigned short v_disp;		/* (W) */
	unsigned short v_sync_strt;	/* (W) */
	unsigned short v_sync_wid;	/* (W) */
	
	
	/*
	 * Engine control registers.
	 */
	unsigned short ext_ge_config; /* (W) */
	unsigned short fifo_opt;	/* (W) */
	unsigned short ge_offset_hi; /* (W) */
	unsigned short ge_offset_lo; /* (W) */
	unsigned short ge_pitch;	/* (W) */
	unsigned short max_waitstates;	/* (RW) */
	unsigned short subsys_cntl;	/* (W) */
	unsigned short rom_page_sel; /* (W) */
	
	/*
	 * Pixel transfer registers.
	 */
	unsigned short alu_bg_fn;	/* (W) */
	unsigned short alu_fg_fn;	/* (W) */
	unsigned short bkgd_color;	/* (W) */
#if (defined(IBM8514))
	unsigned short bkgd_mix;	/* (W) */
#endif
	unsigned short dest_cmp_fn;	/* (W) */
	unsigned short dest_cmp_color; /* (W) */
	unsigned short ext_scissor_b; /* (W) */
	unsigned short ext_scissor_l; /* (W) */
	unsigned short ext_scissor_r; /* (W) */
	unsigned short ext_scissor_t; /* (W) */
	unsigned short frgd_color;	/* (W) */
#if (defined(IBM8514))
	unsigned short frgd_mix;	/* (W) */
#endif
	unsigned short linedraw;	/* (W) */
	unsigned short patt_index;	/* (W) */
	unsigned short patt_length;	/* (W) */
	unsigned short rd_mask;		/* (W) */
	unsigned short src_cmp_fn;	/* (W) */
	unsigned short src_cmp_color; /* (W) */
	unsigned short wrt_mask;	/* (W) */
	
	/*
	 * Status registers.
	 */
	unsigned short config_status_1;	/* (R) */
	unsigned short config_status_2;	/* (R) */
	unsigned short ext_fifo_status;	/* (R) */
	unsigned short ext_ge_status; /* (R) */
	unsigned short subsys_stat;	/* (R) */
	unsigned short vert_line_cntr; /* (R) MACH32 */
	
	/*
	 * Overscan registers.
	 */
	unsigned short horz_overscan; /* (W) MACH32 */
	unsigned short overscan_color_8; /* (W) MACH32 */
	unsigned short overscan_blue_24; /* (W) MACH32 */
	unsigned short overscan_green_24; /* (W) MACH32 */
	unsigned short overscan_red_24;	/* (W) MACH32 */
	unsigned short vert_overscan; /* (W) MACH32 */
	/*
	 * Memory boundary and interface registers.
	 */
	unsigned short mem_bndry;	/* (RW) MACH32 */
	unsigned short mem_cfg;		/* (RW) MACH32 */
	
	/*
	 * Hardware Cursor registers.
	 */
	unsigned short cursor_offset_lo; /* (W) MACH32 */
	unsigned short cursor_offset_hi; /* (W) MACH32 */
	unsigned short horz_cursor_offset; /* (W) MACH32 */
	unsigned short horz_cursor_posn; /* (W) MACH32 */
	unsigned short vert_cursor_offset; /* (W) MACH32 */
	unsigned short vert_cursor_posn; /* (W) MACH32 */
	unsigned short cursor_color_0; /* (W) MACH32 */
	unsigned short cursor_color_1; /* (W) MACH32 */
	unsigned short ext_cursor_color_0; /* (W) MACH32 */
	unsigned short ext_cursor_color_1; /* (W) MACH32 */
	
	/*
	 * Miscellaneous registers.
	 */
	unsigned short local_control; /* (W) MACH32 */
	unsigned short misc_cntl;	/* (W) MACH32 */
	unsigned short misc_options; /* (RW) MACH32 */
	unsigned short rom_addr_1;	/* (RW) */
	unsigned short rom_addr_2;	/* (RW) */

#if (defined(MICROCHANNEL))
	unsigned short setup_id1;	/* (R) */
	unsigned short setup_id2;	/* (R) */
#endif	/* MICROCHANNEL */

	unsigned short setup_opt;	/* (R) */

	/*
	 * save values.
	 */
	unsigned short saved_clock_sel;
	
	unsigned short saved_mem_bndry;	/* (RW) MACH32 */
	
};


/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 * Keeping the following frequently accessed variables in the screen
 * state structure, would incur a performance penalty, and hence these
 * are globals.
 */
extern int mach_graphics_engine_loop_timeout_count ;
extern int mach_graphics_engine_number_of_fifo_entries_free ;
extern int mach_graphics_engine_fifo_blocking_factor ;
extern int mach_graphics_engine_micro_delay_count ;

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean mach_register_debug ;
#endif

#endif	/* __ASSEMBLER__ */

/*
 *	Current module state.
 */

extern void
mach_register_wait_for_fifo(const int n_entries)
;


#endif
