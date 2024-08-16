/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_regs.h	1.9"
#if (! defined(__S3_REGS_INCLUDED__))

#define __S3_REGS_INCLUDED__



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
#include "s3_options.h"
/***
 ***	Constants.
 ***/

/*
 *  Enumeration values that indicates the the current chipset state. 
 *  This is used by the register state set function.
 */
#define DEFINE_CHIPSET_REGISTER_STATE_KINDS() \
	DEFINE_CHIPSET_REGISTER_STATE(CHIPSET_REGISTERS_NOT_IN_XSERVER_MODE),\
	DEFINE_CHIPSET_REGISTER_STATE(CHIPSET_REGISTERS_IN_XSERVER_MODE)
	
enum chipset_register_state_kind 
{ 
#define DEFINE_CHIPSET_REGISTER_STATE(NAME) NAME
	DEFINE_CHIPSET_REGISTER_STATE_KINDS()
#undef DEFINE_CHIPSET_REGISTER_STATE
};

#if (defined(__DEBUG__))
extern const char *const chipset_register_state_kind_to_dump[] ;
#endif

/*
 * In case user has specified to use the alternate io port address to 
 * be used xor the port address with this value.
 */
#define S3_ALTERNATE_IO_PORT_XOR_VALUE 	0x3A0

/*
 * The register mnemonics have been derived from the S3 928/801
 * Chipset reference manuals ( appendix A : Register Reference).
 */
/*
 * General or External registers.
 */

#define VGA_GENERAL_REGISTER_MISC_WR		0x3C2 /*(R)Miscellaneous output */
#define VGA_GENERAL_REGISTER_MISC_RD		0x3CC /*(W)Miscellaneous output */
#define VGA_GENERAL_REGISTER_FCR_WT_COLOR	0x3DA /*(W)Feature control      */
#define VGA_GENERAL_REGISTER_FCR_WT_MONO	0x3BA /*(W)Feature control      */
#define VGA_GENERAL_REGISTER_FCR_RD			0x3CA /*(R)Feature control      */
#define VGA_GENERAL_REGISTER_STATUS_0		0x3C2 /*(R)Input Status 0       */
#define VGA_GENERAL_REGISTER_STATUS_1_COLOR	0x3DA /*(R)Input Status 1       */
#define VGA_GENERAL_REGISTER_STATUS_1_MONO	0x3BA /*(R)Input Status 1       */

/*
 * Sequencer Registers.
 */

#define VGA_NUMBER_OF_SEQUENCER_REGISTERS		0x05
#define VGA_SEQUENCER_REGISTER_SEQX				0x3C4 /*(R/W)Sequencer Index*/
#define VGA_SEQUENCER_REGISTER_SEQ_DATA			0x3C5 /*(R/W)Sequencer Data */
#define VGA_SEQUENCER_REGISTER_RST_SYNC_INDEX	0x00  /*(R/W)Reset          */
#define VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX	0x01  /*(R/W)Clocking Mode  */
#define VGA_SEQUENCER_REGISTER_EN_WT_PL_INDEX	0x02  /*(R/W)Enable WR Plane*/
#define VGA_SEQUENCER_REGISTER_CH_FONT_SL_INDEX	0x03  /*(R/W)Char Font Select*/
#define VGA_SEQUENCER_REGISTER_MEM_MODE_INDEX	0x04  /*(R/W)Mem Mode Control*/

/*
 * CRT Controller Registers.
 * All Registers are Read/Write other than indexes 24 and 26.
 */

#define VGA_NUMBER_OF_CRTC_REGISTERS		0x25
#define VGA_CRTC_REGISTER_CRTC_ADR_MONO		0x3B4 /*CRT Controller Index     */
#define VGA_CRTC_REGISTER_CRTC_ADR_COLOR	0x3D4 /*CRT Controller Index     */
#define VGA_CRTC_REGISTER_CRTC_DATA_MONO	0x3B5 /*CRT Controller Data      */
#define VGA_CRTC_REGISTER_CRTC_DATA_COLOR	0x3D5 /*CRT Controller Data      */
#define VGA_CRTC_REGISTER_H_TOTAL_INDEX		0x00 /*Horizontal Total          */
#define VGA_CRTC_REGISTER_H_D_END_INDEX		0x01 /*Horizontal Display End    */
#define VGA_CRTC_REGISTER_S_H_BLNK_INDEX	0x02 /*Start Horizontal Blank    */
#define VGA_CRTC_REGISTER_E_H_BLNK_INDEX	0x03 /*End Horizontal Blank      */
#define VGA_CRTC_REGISTER_S_H_SY_P_INDEX	0x04 /*Start Horizontal Sync Pos */
#define VGA_CRTC_REGISTER_E_H_SY_P_INDEX	0x05 /*End Horizontal Sync Pos   */
#define	VGA_CRTC_REGISTER_V_TOTAL_INDEX		0x06 /*Vertical Total            */
#define	VGA_CRTC_REGISTER_OVFL_REG_INDEX	0x07 /*CRTC Overflow             */
#define	VGA_CRTC_REGISTER_P_R_SCAN_INDEX	0x08 /*Preset Row Scan           */
#define	VGA_CRTC_REGISTER_MAX_S_LN_INDEX	0x09 /*Maximum Scan Line         */
#define	VGA_CRTC_REGISTER_CSSL_INDEX		0x0A /*Cursor Start Scan Line    */
#define	VGA_CRTC_REGISTER_CESL_INDEX		0x0B /*Cursor End Scan Line      */
#define	VGA_CRTC_REGISTER_STA_H_INDEX		0x0C /*Start Address High        */
#define	VGA_CRTC_REGISTER_STA_L_INDEX		0x0D /*Start Address Low         */
#define VGA_CRTC_REGISTER_CLA_H_INDEX		0x0E /*Cursor Location Addr High */
#define VGA_CRTC_REGISTER_CLA_L_INDEX		0x0F /*Cursor Location Addr Low  */ 
#define VGA_CRTC_REGISTER_VRS_INDEX			0x10 /*Vertical Retrace Start    */
#define VGA_CRTC_REGISTER_VRE_INDEX			0x11 /*Vertical Retrace End      */
#define	VGA_CRTC_REGISTER_VDE_INDEX			0x12 /*Vertical Display End      */
#define	VGA_CRTC_REGISTER_SCREEN_OFFSET_INDEX	0x13 /*Offset ?              */
#define	VGA_CRTC_REGISTER_ULL_INDEX			0x14 /*Underline Location        */
#define	VGA_CRTC_REGISTER_SVB_INDEX			0x15 /*Start Vertical Blank      */
#define	VGA_CRTC_REGISTER_EVB_INDEX			0x16 /*End Vertical Blank        */
#define	VGA_CRTC_REGISTER_CRT_MD_INDEX		0x17 /*CRTC Mode Control         */
#define	VGA_CRTC_REGISTER_LCM_INDEX			0x18 /*Line Control              */
#define	VGA_CRTC_REGISTER_GCCL_INDEX		0x22 /*CPU Latch Data            */
#define	VGA_CRTC_REGISTER_AT_CNTL_F_INDEX	0x24 /*Attribute Controller Flag */
#define	VGA_CRTC_REGISTER_AT_CNTL_I_INDEX	0x26 /*Attribute Controller Index*/

/*
 * Graphics Controller Registers.
 */
#define VGA_NUMBER_OF_GRAPHICS_CONTROLLER_REGISTERS 0x09
#define VGA_GRAPHICS_CONTROLLER_REGISTER_ADR		0x3CE /*Index            */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_DATA		0x3CF /*Data 			 */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_SET_RST_DT	0x00 /*Set/Reset         */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_EN_S_R_DT	0x01 /*Enable Set/Reset  */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_COLOR_CMP	0x02 /*Color Compare     */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_WT_ROP_RTC	0x03 /*RasterOp/Rotate   */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_RD_PL_SL	0x04 /*Read Plane Select */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_GRP_MODE	0x05 /*Mode              */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_MISC_GM	0x06 /*Mem Map Mode Cntl */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_CMP_DNTC	0x07 /*Color Dont Care   */
#define VGA_GRAPHICS_CONTROLLER_REGISTER_BIT_MASK	0x08 /*Bit Mask          */

/*
 * Attribute Controller Registers.
 * All Attribute Controller indexes are read/write.
 */

#define VGA_NUMBER_OF_ATTRIBUTE_CONTROLLER_REGISTERS 0x15
#define VGA_ATTRIBUTE_CNTL_REGISTER_ADR		0x3C0 /*Index                   */	
#define VGA_ATTRIBUTE_CNTL_REGISTER_DATA_R 	0x3C1 /*Data                    */
#define VGA_ATTRIBUTE_CNTL_REGISTER_DATA_W 	0x3C0 /*Data                    */
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR00	0x00 /*16 Palette Regs  */
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR01	0x01
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR02	0x02
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR03	0x03
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR04	0x04
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR05	0x05
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR06	0x06
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR07	0x07
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR08	0x08
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR09	0x09
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR0A	0x0A
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR0B	0x0B
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR0C	0x0C
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR0D	0x0D
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR0E	0x0E
#define VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR0F	0x0F
#define VGA_ATTRIBUTE_CNTL_REGISTER_ATR_MODE		0x10 /*Mode Control      */ 
#define VGA_ATTRIBUTE_CNTL_REGISTER_BDR_CLR			0x11 /*Border Color      */
#define VGA_ATTRIBUTE_CNTL_REGISTER_DISP_PLN		0x12 /*Color Plane Enable*/
#define VGA_ATTRIBUTE_CNTL_REGISTER_H_PX_PAN		0x13 /*Hori Pixel Panning*/
#define VGA_ATTRIBUTE_CNTL_REGISTER_PX_PAD			0x14 /*Pixel Padding     */

/*
 * Setup Registers.  
 */

#define S3_REGISTER_SETUP_OS		0x102 	/*(R/W) Setup Option Select      */
#define S3_REGISTER_SETUP_VSE		0x46e8  /*(W)   Video Subsystem Enable   */

/*
 * Video DAC registers.
 */

#define	VGA_DAC_REGISTER_DAC_AD_MK	0X3C6 /*(R/W) DAC Mask                   */
#define	VGA_DAC_REGISTER_DAC_RD_AD	0X3C7 /*(W)   DAC Read Index             */
#define	VGA_DAC_REGISTER_DAC_STS	0X3C7 /*(R)   DAC Status                 */
#define	VGA_DAC_REGISTER_DAC_WR_AD	0X3C8 /*(R/W) DAC Write Index            */
#define	VGA_DAC_REGISTER_DAC_DATA	0X3C9 /*(R/W) DAC Data                   */

/*
 * S3 VGA Registers.
 * Every register other than index 30 is read/write. 
 * Index 30 is read only.
 */

#define S3_VGA_REGISTER_CHIP_ID_INDEX		0x30 /*Chip ID/Rev Register      */
#define S3_VGA_REGISTER_MEM_CNFG_INDEX		0x31 /*Memory Configuration      */
#define S3_VGA_REGISTER_BKWD_1_INDEX		0x32 /*Backward Compatibility 1  */
#define S3_VGA_REGISTER_BKWD_2_INDEX		0x33 /*Backward Compatibility 2  */
#define S3_VGA_REGISTER_BKWD_3_INDEX		0x34 /*Backward Compatibility 3  */
#define S3_VGA_REGISTER_CRTR_LOCK_INDEX		0x35 /*CRT REgister Lock         */
#define S3_VGA_REGISTER_CNFG_REG1_INDEX		0x36 /*Configuration 1           */
#define S3_VGA_REGISTER_CNFG_REG2_INDEX		0x37 /*Configuration 2           */
#define S3_VGA_REGISTER_REG_LOCK1_INDEX 	0x38 /*Register Lock 1           */
#define S3_VGA_REGISTER_REG_LOCK2_INDEX 	0x39 /*Register Lock 2           */
#define S3_VGA_REGISTER_MISC_1_INDEX		0x3A /*Miscellaneous 1           */
#define S3_VGA_REGISTER_DT_EX_POS_INDEX 	0x3B /*Data Trans Execute Posn   */
#define S3_VGA_REGISTER_IL_RTSTART_INDEX	0x3C /*Interlace Retrace Start   */

/*
 * System Control Registers.
 * All Registers are read/write.
 */

#define S3_SYSTEM_CONTROL_REGISTER_SYS_CNFG_INDEX		0x40 /*System Config */
#define S3_SYSTEM_CONTROL_REGISTER_BIOS_FLAG_INDEX		0x41 /*BIOS Flag     */
#define S3_SYSTEM_CONTROL_REGISTER_MODE_CTL_INDEX		0x42 /*Mode control  */
#define S3_SYSTEM_CONTROL_REGISTER_EXT_MODE_INDEX 		0x43 /*Extended mode */
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX		0x45 /*HW cursor mode*/
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_INDEX		0x46 /*HW cursor X   */
															 /*origin		 */
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_HI_INDEX	0x46 /*HW cursor X   */
															 /*origin hi byte*/
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_LO_INDEX	0x47 /*HW cursor X   */
															 /*origin lo byte*/
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_INDEX		0x48 /*HW cursor Y   */
															 /*origin		 */
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_HI_INDEX	0x48 /*HW cursor Y   */
															 /*origin hi byte*/
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_LO_INDEX	0x49 /*HW cursor Y   */
															 /*origin lo byte*/
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_FGSTK_INDEX		0x4A /*HW cursor FG  */
															 /*stack		 */
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_BGSTK_INDEX		0x4B /*HW cursor BG  */
															 /*stack		 */
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_INDEX		0x4C /*HW cursor     */
														     /*address       */
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_HI_INDEX	0x4C /*HW cursor     */
														     /*address  high */
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_LO_INDEX	0x4D /*HW cursor     */
														     /*address   low */
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_DX_INDEX		0x4E /*HW cursor pat */
														     /*display startX*/
#define S3_SYSTEM_CONTROL_REGISTER_HWGC_DY_INDEX		0x4F /*HW cursor pat */
														     /*display startY*/

/* 
 * System Extension Registers.
 * All registers are read/write.
 */

#define	S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_1	0x50 /*Extended system Cont 1*/
#define	S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_2	0x51 /*Extended system Cont 2*/
#define S3_SYSTEM_EXTENSION_REGISTER_EXT_BFLG1	0x52 /*Extended BIOS Flag 1  */
#define S3_SYSTEM_EXTENSION_REGISTER_EX_MCTL_1	0x53 /*Extended Memory Cont 1*/
#define S3_SYSTEM_EXTENSION_REGISTER_EX_MCTL_2	0x54 /*Extended Memory Cont 2*/
#define S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT 	0x55 /*Extended DAC Control  */
#define S3_SYSTEM_EXTENSION_REGISTER_EX_SYNC_1	0x56 /*External Sync Cont 1  */
#define S3_SYSTEM_EXTENSION_REGISTER_EX_SYNC_2	0x57 /*External Sync Cont 2  */
#define S3_SYSTEM_EXTENSION_REGISTER_LAW_CTL	0x58 /*LinearAddr Window Cntl*/
#define S3_SYSTEM_EXTENSION_REGISTER_LAW_POS	0x59 /*LinearAddr Window Posn*/
#define S3_SYSTEM_EXTENSION_REGISTER_EXT_BFLG2	0x5B /*Extended BIOS Flag 2  */
#define	S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT	0x5C /*General Out Port      */
#define	S3_SYSTEM_EXTENSION_REGISTER_EXT_H_OVF	0x5D /*Extended Horz Overflow*/
#define	S3_SYSTEM_EXTENSION_REGISTER_EXT_V_OVF	0x5E /*Extended Vert Overflow*/
#define	S3_SYSTEM_EXTENSION_REGISTER_BGNT_TPOS	0x5F /*Bus Grant Termination *
													  *Position Register(only*
													  *in 928)               */

/*
 * Enhanced Command Registers.
 */

/* (R) Subsystem Status */
#define S3_ENHANCED_COMMAND_REGISTER_SUBSYS_STAT	0x42E8 
/* (W) Subsystem Control */
#define S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL	0x42E8
/* (W) Advanced Function Control */
#define S3_ENHANCED_COMMAND_REGISTER_ADVFUNC_CNTL	0x4AE8
/* (R/W) Current Y Position */
#define	S3_ENHANCED_COMMAND_REGISTER_CUR_Y			0x82E8
/* (R/W) Current X Position */
#define	S3_ENHANCED_COMMAND_REGISTER_CUR_X			0x86E8
/* (R/W) Destination Y Position/Axial Step Constant */
#define	S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP	0x8AE8
/* (R/W) Destination X Position/Diagonal Step Constant */
#define	S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP	0x8EE8
/* (R/W) Error Term */
#define	S3_ENHANCED_COMMAND_REGISTER_ERR_TERM		0x92E8
/* (R/W) Major Axis Pixel Count */
#define	S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT	0x96E8
/* (R) Graphics Processor Status */
#define	S3_ENHANCED_COMMAND_REGISTER_GP_STAT		0x9AE8
/* (W) Drawing Command */
#define	S3_ENHANCED_COMMAND_REGISTER_CMD			0x9AE8
/* (W) Short Stroke Vector Transfer */
#define	S3_ENHANCED_COMMAND_REGISTER_SHORT_STROKE	0x9EE8
/* (R/W) Background Color */
#define	S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR		0xA2E8
/* (R/W) Foreground Color */
#define	S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR		0xA6E8
/* (R/W) Write Mask */
#define	S3_ENHANCED_COMMAND_REGISTER_WRT_MASK		0xAAE8
/* (R/W) Read Mask */
#define	S3_ENHANCED_COMMAND_REGISTER_RD_MASK		0xAEE8
/* (R/W) Color Compare */
#define	S3_ENHANCED_COMMAND_REGISTER_COLOR_CMP		0xB2E8
/* (W) Background Mix */
#define S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX 		0xB6E8
/* (W) Foreground Mix */
#define S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX		0xBAE8
/* (R) Read Register Data */
#define	S3_ENHANCED_COMMAND_REGISTER_RD_REG_DT		0xBEE8
/* (R/W) Pixel Data Transfer */
#define	S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS		0xE2E8
/* (R/W) Pixel Data Transfer-Extension */
#define	S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS_EXT	0xE2EA

/*
 * The following indexes are present as 4 most significant bits in the 
 * S3 Multifunction control register when written. The remaining 12 bits
 * are treated as data for the indexed registers.
 */

#define	S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL	0xBEE8
/* Minor Axis pixel count */
#define S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX	0x0000
/* Top Scissors */
#define S3_ENHANCED_COMMAND_REGISTER_SCISSORS_T_INDEX		0x1000
/* Left Scissors */
#define S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX		0x2000
/* Bottom Scissors */
#define S3_ENHANCED_COMMAND_REGISTER_SCISSORS_B_INDEX		0x3000
/* Right Scissors */
#define S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX		0x4000
/* Pixel Control */
#define S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX			0xA000
/* Multifunction Control Miscellaneous */
#define S3_ENHANCED_COMMAND_REGISTER_MULT_MISC_INDEX		0xE000
/* Read Register Select */
#define	S3_ENHANCED_COMMAND_REGISTER_READ_SEL				0xF000

/*
 * Any chipset revision specific registers/undocumented registers.
 * like the S3 801's  CR61 etc.
 */
#define S3_NUMBER_OF_EXTRA_REGISTERS					50
#define S3_EXTRA_CRTC_REGISTER_REGISTER_CR60_INDEX		0x60
#define S3_EXTRA_CRTC_REGISTER_REGISTER_CR61_INDEX		0x61
#define S3_EXTRA_CRTC_REGISTER_REGISTER_CR62_INDEX		0x62
#define S3_REGISTER_CR60_INDEX							0
#define S3_REGISTER_CR61_INDEX							1
#define S3_REGISTER_CR62_INDEX							2

/*   
 * Values for the registers.
 */
/*
 * Dac register size.
 * This is the size of the vga dac since the register width is only
 * a byte long. Hence we can index only 256 entries.
 */
#define S3_STANDARD_VGA_PALETTE_SIZE					256

/*
 * Memory Configuration Register S3 VGA register CR31.
 */
#define MEM_CNFG_STRT_ADR								0x30

/*
 * Memory sizes in configuration 1 register. CR36.
 */
#define CONFIG_1_DISPLAY_MEMORY_SIZE_512	0xe0
#define CONFIG_1_DISPLAY_MEMORY_SIZE_1024	0xc0
#define CONFIG_1_DISPLAY_MEMORY_SIZE_2048	0x80
#define CONFIG_1_DISPLAY_MEMORY_SIZE_3072	0x40
#define CONFIG_1_DISPLAY_MEMORY_SIZE_4096	0x00
/*
 * MISC OUT register.
 */
#define MISC_OUT_COLOR_EMULATION			0x01
#define MISC_OUT_ENB_RAM					0x02
#define MISC_OUT_25175_CLOCK				0x00
#define MISC_OUT_28322_CLOCK				0x04
#define MISC_OUT_EXTERNAL_CLOCK				0x0c
#define MISC_OUT_PGSL						0x20
#define MISC_OUT_HSPBAR						0x40
#define MISC_OUT_VSPBAR						0x80


/* 
 * Sequencer Register.
 */
#define	VGA_SEQUENCER_RESET_VALUE			0x00
#define	VGA_SEQUENCER_START_VALUE			0x03
#define VGA_SEQUENCER_CLK_MODE_SCREEN_OFF	0x20

/*
 * Linear address window control register. LAW_CTL CR58
 */
#define LAW_CTL_LAW_SIZE_64K				0x00
#define LAW_CTL_LAW_SIZE_1M					0x01
#define LAW_CTL_LAW_SIZE_2M					0x02
#define LAW_CTL_LAW_SIZE_4M					0x03
#define LAW_CTL_ENB_RAC						0x04
#define LAW_CTL_ISA_LAD						0x08
#define LAW_CTL_ENABLE_LINEAR				0x10
#define LAW_CTL_LMT_WPE						0x20
#define LAW_CTL_SAM_256						0x40
#define LAW_CTL_RAS_6_MCLK					0x80

/*
 * SUBSYS_CNTL_REGISTER.
 */
#define	SUBSYS_CNTL_RESET_GRAPHICS_ENGINE	0x800FU
#define	SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE	0x4000U

/*
 * Multifunction control register 
 */
#define S3_MULTIFUNC_VALUE_BITS				0x0FFF

/*
 * FRGD and BKGD Mix registers.
 */
#define S3_MIX_REGISTER_MIX_TYPE_BITS		0x000F
#define S3_MIX_REGISTER_CLR_SRC_BITS		0x0060

/*
 * Mix values to go into the Background and Foreground Mix registers.
 * C and N represent Current and New. 
 * Refer s3 928 programmers guide pg 10-14.
 */
#define	S3_MIX_FN_NOT_C					0	/* GXinvert */
#define	S3_MIX_FN_LOGICAL_ZERO			1	/* GXclear */
#define	S3_MIX_FN_LOGICAL_ONE			2	/* GXset */
#define	S3_MIX_FN_LEAVE_C_AS_IS			3	/* GXnoop */
#define	S3_MIX_FN_NOT_N					4	/* GXcopyInverted */
#define	S3_MIX_FN_C_XOR_N				5	/* GXxor */
#define	S3_MIX_FN_NOT_C_XOR_N			6	/* GXequiv */
#define	S3_MIX_FN_N						7	/* GXcopy */
#define	S3_MIX_FN_NOT_C_OR_NOT_N		8	/* GXnand */
#define	S3_MIX_FN_C_OR_NOT_N			9	/* GXorInverted */
#define	S3_MIX_FN_NOT_C_OR_N			10	/* GXorReverse */
#define	S3_MIX_FN_C_OR_N				11	/* GXor */
#define	S3_MIX_FN_C_AND_N				12	/* GXand */
#define	S3_MIX_FN_NOT_C_AND_N			13	/* GXandReverse */
#define	S3_MIX_FN_C_AND_NOT_N			14	/* GXandInverted */
#define	S3_MIX_FN_NOT_C_AND_NOT_N		15	/* GXnor */

/*
 * CLR_SRC bits values. These bits are present in the mix registers.
 */
#define S3_CLR_SRC_BKGD_COLOR			0x0000
#define S3_CLR_SRC_FRGD_COLOR			0x0020
#define S3_CLR_SRC_CPU_DATA				0x0040
#define S3_CLR_SRC_VIDEO_DATA			0x0060

/*
 * Drawing Command Register - CMD
 */
#define S3_CMD_TYPE_NOOP				0x0000
#define S3_CMD_TYPE_LINEDRAW			0x2000
#define S3_CMD_TYPE_RECTFILL			0x4000
#define S3_CMD_TYPE_BITBLT				0xC000
#define S3_CMD_TYPE_PATTERNFILL			0xE000
#define S3_CMD_LSB_FIRST				0x1000
#define S3_CMD_BUS_WIDTH_16				0x0200
#define S3_CMD_USE_PIXTRANS				0x0100
#define S3_CMD_AXIAL_Y_BOTTOM_TO_TOP	0x0000
#define S3_CMD_AXIAL_Y_TOP_TO_BOTTOM	0x0080
#define S3_CMD_AXIAL_X_MAJOR			0x0000
#define S3_CMD_AXIAL_Y_MAJOR			0x0040
#define S3_CMD_AXIAL_X_RIGHT_TO_LEFT	0x0000
#define S3_CMD_AXIAL_X_LEFT_TO_RIGHT	0x0020
#define S3_CMD_DRAW						0x0010
#define S3_CMD_DIR_TYPE_RADIAL			0x0008
#define S3_CMD_DIR_TYPE_AXIAL			0x0000
#define S3_CMD_LAST_PXOF				0x0004
#define S3_CMD_PX_MD_THRO_PLANE			0x0000
#define S3_CMD_PX_MD_ACROSS_PLANE		0x0002
#define S3_CMD_READ						0x0000
#define S3_CMD_WRITE					0x0001
#define S3_CMD_0_DGR_RADIAL_LINE		0x0000
#define S3_CMD_45_DGR_RADIAL_LINE		0x0020
#define S3_CMD_90_DGR_RADIAL_LINE		0x0040
#define S3_CMD_135_DGR_RADIAL_LINE		0x0060
#define S3_CMD_180_DGR_RADIAL_LINE		0x0080
#define S3_CMD_225_DGR_RADIAL_LINE		0x00a0
#define S3_CMD_270_DGR_RADIAL_LINE		0x00c0
#define S3_CMD_315_DGR_RADIAL_LINE		0x00e0

/*
 * S3 system control register. System config register CR40.
 */
#define SYS_CNFG_ENA_8514				0x01
#define SYS_CNFG_CPC_SEL				0x02
#define SYS_CNFG_WR_WAIT				0x04
#define SYS_CNFG_EWRT_POST				0x08
#define SYS_CNFG_DEC_WAIT_0				0x00
#define SYS_CNFG_DEC_WAIT_1				0x10
#define SYS_CNFG_DEC_WAIT_2				0x30
#define SYS_CNFG_DEC_WAIT_3				0x20
#define SYS_CNFG_RD_WAIT_0				0x00
#define SYS_CNFG_RD_WAIT_1				0x40
#define SYS_CNFG_RD_WAIT_2				0xc0
#define SYS_CNFG_RD_WAIT_3				0x80
#define SYS_CNFG_ISA_DIS_NOWS			0x80

/*
 * S3 System control register. Mode control register CR42.
 */
#define MODE_CTL_CR42_INTERLACED		0x20

/*
 * S3 system control register. Extended mode register CR43.
 */
#define EXT_MODE_64K_CLR				0x08
#define EXT_MODE_XEN					0x10
#define EXT_MODE_HCTR_X2				0x80

/*
 * S3 system control register. Hardware Graphics Cursor register. CR45.
 */
#define HGC_MODE_HWGC_ENB				0x01
#define HGC_MODE_HWC_HSTR_X2W			0x04
#define HGC_MODE_HWC_HSTR_X3W			0x08
#define HGC_MODE_HWGC_1280				0x10
#define HGC_MODE_ENB_485				0x20

/*
 * Hardware graphics cursor origin-X registers. CR46,CR47.
 */
#define HWGC_ORGX_LO_BITS				0xFF
#define HWGC_ORGX_HI_BITS               0x07

/*
 * Hardware graphics cursor origin-Y registers. CR48,CR49.
 */
#define HWGC_ORGY_LO_BITS				0xFF
#define HWGC_ORGY_HI_BITS               0x07

/*
 * Hardware graphics cursor storage start address registers  CR4C,CR4D.
 */
#define HWGC_STA_LO_BITS                0xFF
#define HWGC_STA_HI_BITS                0x0F

/*
 * Hardware graphics cursor  pattern display start X CR4E.
 */
#define HWGC_DX_BITS                    0x3F

/*
 * Hardware graphics cursor  pattern display start Y CR4F.
 */
#define HWGC_DY_BITS                    0x3F

/*
 * Extended system control 1 register. CR50 S3 system extension registers.
 */
#define EX_SCTL_1_GE_SCR_W_1024_OR_2048	0x00
#define EX_SCTL_1_GE_SCR_W_640			0x40
#define EX_SCTL_1_GE_SCR_W_800			0x80
#define EX_SCTL_1_GE_SCR_W_1280			0xC0
#define EX_SCTL_1_GE_PXL_LNGH_16		0x10
#define EX_SCTL_1_GE_PXL_LNGH_32		0x30

/*
 * Extended system control 2 register. CR51 S3 system extension registers.
 */
#define EX_SCTL_2_DIS_SPXF				0x40
#define EX_SCTL_2_ENB_ERW				0x80
#define EX_SCTL_2_DISP_ST_AD			0x03

/*
 * Extended memory control register 1. CR53 S3 system extension registers.
 */
#define EX_MCTL_1_ENB_MMIO				0x10
#define EX_MCTL_1_PAR_VRAM				0x20
#define EX_MCTL_1_SWP_NBL				0x40
#define EX_MCTL_1_ENB_NBLW				0x80

/*
 * Extended memory control register 2. CR54
 */
#define EX_MCTL_2_RAC_EXT_PFTCH_MASK	0x07

/*
 * Extended dac control register. CR55.
 */
#define EX_DAC_CT_HWGC_EXOP				0x20
#define EX_DAC_CT_CURSOR_MODE_X11		0x10
#define EX_DAC_CT_ENB_SID				0x08
#define EX_DAC_CT_ENABLE_GIR			0x04
#define EX_DAC_CT_DAC_R_SEL_BITS		0x03

/*
 * GP_STAT Register. Graphics processor status.
 */
#define GP_STAT_READ_DATA_AVAILABLE 	0x0100
#define GP_STAT_GE_BUSY 				0x0200

/*
 * SUBSYS_STAT Register. Graphics engine interrupt status.
 */
#define SUBSYS_STAT_FIFO_OVERFLOW		0x0004

/*
 * ADVFUNC_CNTL values.
 */
#define ADVFUNC_CNTL_ENB_EHFC_VGA					0x0000
#define ADVFUNC_CNTL_ENB_EHFC_ENHANCED				0x0001
#define ADVFUNC_CNTL_SCRN_SIZE_800_OR_1024			0x0004

/*
 * PIX_CNTL values. Pixel control register.
 */
#define PIX_CNTL_DONT_PACK_DATA			0x0000
#define PIX_CNTL_PACK_DATA				0x0004
#define PIX_CNTL_DT_EX_SRC_FRGD_MIX		0x0000
#define PIX_CNTL_DT_EX_SRC_CPU_DATA		0x0080
#define PIX_CNTL_DT_EX_SRC_VID_MEM		0x00C0
#define PIX_CNTL_DT_EX_SRC_BITS			0x00C0


/*
 * MMIO parameters.
 */
#define S3_MMIO_WINDOW_BASE_PHYSICAL_ADDRESS	0xA0000
#define S3_MMIO_WINDOW_SIZE_IN_BYTES			0x0FFFF
#define I386_PAGE_SIZE							4096

/*
 * Graphics engine pattern blit parameters.
 */
#define S3_GE_ASSIST_PATBLT_PATTERN_WIDTH			8
#define S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT			8
#define S3_GE_ASSIST_PATBLT_PATTERN_WIDTH_SHIFT		3
#define S3_GE_ASSIST_PATBLT_PATTERN_WIDTH_HEIGHT	3

/***
 ***	Types.
 ***/
#if (!defined(__ASSEMBLER__))

struct vga_general_register_state 
{
	unsigned char	misc_out;
	unsigned char	feature_control;
	unsigned char	status_0;
	unsigned char	status_1;
};

struct vga_sequencer_register_state
{
	unsigned char 	reset_sync;
	unsigned char 	clk_mode;
	unsigned char 	enab_wr_plane;
	unsigned char 	ch_font_sel;
	unsigned char 	mem_mode;
};

struct vga_crt_controller_register_state
{
	unsigned char	h_total;
	unsigned char	h_d_end;
	unsigned char	s_h_blank;
	unsigned char	e_h_blank;
	unsigned char	s_h_sy_p;
	unsigned char	e_h_sy_p;
	unsigned char	v_total;
	unsigned char	ovfl_reg;
	unsigned char	preset_row_scan;
	unsigned char	max_scan_lines;
	unsigned char	cursor_start_scan_line;
	unsigned char	cursor_end_scan_line;
	unsigned char	start_addr_h;
	unsigned char	start_addr_l;
	unsigned char	cursor_loc_addr_h;
	unsigned char	cursor_loc_addr_l;
	unsigned char	vert_ret_start;
	unsigned char	vert_ret_end;
	unsigned char	vert_disp_end;
	unsigned char	screen_offset;
	unsigned char	under_line_loc;
	unsigned char	start_vert_blank;
	unsigned char	end_vert_blank;
	unsigned char	crtc_mode_control;
	unsigned char	line_cmp;
	unsigned char	cpu_latch_data;
	unsigned char	attribute_controller_flag;
	unsigned char	attribute_controller_index;
};

struct vga_graphics_controller_register_state 
{
	unsigned char	set_reset;
	unsigned char	enable_set_reset;
	unsigned char	color_cmp;
	unsigned char	raster_op_rotate_counter;
	unsigned char	read_plane_select;
	unsigned char	graphics_controller_mode;
	unsigned char	mem_map_mode_control;
	unsigned char	cmp_dont_care;
	unsigned char	bit_mask;
};

struct vga_attribute_controller_register_state
{
	unsigned char	palette_reg[16];
	unsigned char	attr_mode_control;
	unsigned char	border_color;
	unsigned char	color_plane_enable;
	unsigned char	horiz_pixel_panning;
	unsigned char	pixel_padding;
};

struct vga_dac_register_state
{
	unsigned char	dac_mask;
	unsigned char	dac_rd_index;
	unsigned char	dac_status;
	unsigned char	dac_wr_index;
	unsigned char	dac_data;
};

struct standard_vga_state
{
	struct vga_general_register_state 	
							standard_vga_general_registers;
	struct vga_sequencer_register_state	
							standard_vga_sequencer_registers;
	struct vga_crt_controller_register_state 
							standard_vga_crtc_registers;
	struct vga_graphics_controller_register_state  
							standard_vga_graphics_controller_registers;
	struct vga_attribute_controller_register_state 
							standard_vga_attribute_controller_registers;
	struct vga_dac_register_state 
							standard_vga_dac_registers;
	/*
	 * Setup registers
	 * We will never ever touch these registers. 
	 */
	unsigned char	setup_option_select; 	/* R/W */
	unsigned char	video_subsystem_enable;	/* -/W */
};

struct s3_vga_register_state
{
												/* 801  928 */
	unsigned char 	chip_id;					/* R/-	R/- */  
	unsigned char 	mem_cfg; 					/* R/W  R/W */
	unsigned char	bkwd_compat_1;				/* R/W  R/W */
	unsigned char	bkwd_compat_2;				/* R/W  R/W */
	unsigned char	bkwd_compat_3;				/* R/W  R/W */
	unsigned char 	crt_reg_lock;				/* R/W  R/W */
	unsigned char 	config_1;  					/* R/W  R/W */
	unsigned char 	config_2; 					/* R/W  R/W */
	unsigned char	register_lock1;				/* R/W  R/W */
	unsigned char 	register_lock2;				/* R/W  R/W */
	unsigned char 	misc_1;						/* R/W  R/W */
	unsigned char 	data_execute_position;		/* Resd R/W */
	unsigned char 	interlace_retrace_start;	/* R/W  R/W */
};

struct s3_system_control_register_state
{
	unsigned char	system_config;						/* R/W */
	unsigned char	bios_flag; 							/* R/W */
	unsigned char 	mode_control;						/* R/W */
	unsigned char 	extended_mode;						/* R/W */
	unsigned char 	hw_cursor_mode;						/* R/W */
	unsigned short 	hw_cursor_origin_x;					/* R/W */
	unsigned short 	hw_cursor_origin_y;					/* R/W */
	unsigned char 	hw_cursor_frgd_stack;				/* R/W */
	unsigned char 	hw_cursor_bkgd_stack;				/* R/W */
	unsigned short 	hw_cursor_start_address;			/* R/W */
	unsigned char 	hw_cursor_pattern_start_x_position; /* R/W */
	unsigned char 	hw_cursor_pattern_start_y_position; /* R/W */
};
struct s3_system_extension_register_state 
{
	unsigned char	extended_system_control_1;			/* R/W */
	unsigned char	extended_system_control_2;			/* R/W */
	unsigned char	extended_bios_flag_1;				/* R/W */
	unsigned char	extended_mem_control_1;				/* R/W */
	unsigned char	extended_mem_control_2;				/* R/W */
	unsigned char	extended_dac_control;				/* R/W */
	unsigned char	extended_sync_1;					/* R/W */
	unsigned char	extended_sync_2;					/* R/W */
	unsigned char	extended_linear_addr_window_control;/* R/W */
	unsigned short	extended_linear_addr_window_pos;	/* R/W */
	unsigned char	extended_bios_flag_2;				/* R/W */
	unsigned char	extended_general_out_port;			/* R/W */
	unsigned char	extended_horz_ovfl;					/* R/W */
	unsigned char	extended_vert_ovfl;					/* R/W */
	unsigned char	extended_bus_grant_termination_pos;	/* R/W;absent in 801 */
};

struct s3_enhanced_commands_register_state 
{
	unsigned short subsystem_status;		/* R/- */
	unsigned short subsytsem_control;		/* -/W */
	unsigned short advfunc_control;			/* -/W  R/W in 928 */
	unsigned short cur_x;					/* R/W */
	unsigned short cur_y;					/* R/W */
	unsigned short dest_x_diag_step;		/* R/W */
	unsigned short dest_y_axial_step;		/* R/W */
	unsigned short error_term;				/* R/W */
	unsigned short major_axis_pixel_count;	/* R/W */
	unsigned short gp_status;				/* R/- */
	unsigned short draw_command;			/* -/W */
	unsigned short short_stroke;			/* -/W */
	unsigned long bkgd_color;				/* R/W 16 bits only in 801 */
	unsigned long frgd_color;				/* R/W 16 bits only in 801 */
	unsigned long write_mask;				/* R/W 16 bits only in 801 */
	unsigned long read_mask;				/* R/W 16 bits only in 801 */
	unsigned long color_compare;			/* R/W 16 bits only in 801 */
	unsigned short bkgd_mix;				/* -/W */
	unsigned short frgd_mix;				/* -/W */
	unsigned short read_register_data;		/* R/- */
	unsigned short minor_axis_pixel_count;	/* -/W */
	unsigned short scissor_t;				/* -/W */
	unsigned short scissor_l;				/* -/W */
	unsigned short scissor_b;				/* -/W */
	unsigned short scissor_r;				/* -/W */
	unsigned short pixel_control;			/* -/W */
	unsigned short mult_misc;				/* -/W */
	unsigned short read_sel;				/* -/W */
	unsigned short pix_trans;				/* R/W */
	unsigned short pix_trans_ext;			/* R/W */
};

/*
 * 20 apr 1994.
 * node type for the linklist of extra crtc registers that keep 
 * cropping up every with every new stepping number.
 */
struct new_s3_register_node {
	int		index; /* crtc register index */
	int		mask;  /* bit positions to modify */
	int		value; /* value to write */
	int		rbits; /* reserved bits, a 0 will be written */
	int		saved_value; /* original value , used when restoring */
	struct new_s3_register_node *next_p;		
};

/*
 * Register state for the s3 chipsets
 * After the generic state we will have two parts to this struct. 
 * One is what the X server knows as the chipsets register state and
 * Second what it should restore before giving up its Virtual Terminal.
 */
struct s3_register_state
{
    struct generic_register_state generic_state;

	/*
	 * Generic VGA Registers. These are the standard IBM VGA registers.
	 */
	struct standard_vga_state	standard_vga_registers;

	/*
	 * S3 VGA Registers.
	 */
	struct s3_vga_register_state s3_vga_registers;

	/*
	 * System Control Registers.
	 */
	struct s3_system_control_register_state s3_system_control_registers;
	/* 
	 * System Extension Registers.
	 */
	struct s3_system_extension_register_state s3_system_extension_registers;
	/*
	 * Undocumented registers/chipset version specific registers etc 
	 * if any, like 801 has CR61-CR65.
	 */
	unsigned long s3_extra_registers[S3_NUMBER_OF_EXTRA_REGISTERS];

	/*
	 * Pointer to list of new registers, definitions of new bits
	 * that keeps cropping up with new revisions of the S3 chipsets.
	 */
	struct new_s3_register_node *new_s3_registers_list_head_p;

	/*
	 * Enhanced Commands Registers.
	 */
	struct s3_enhanced_commands_register_state s3_enhanced_commands_registers;

	/*
	 * save values.
	 * The vga state before switching into the X server state.
	 * This is the state that would be eventually restored when the
	 * X server exits.
	 */
	struct standard_vga_state	saved_standard_vga_registers;
	struct s3_vga_register_state saved_s3_vga_registers;
	struct s3_system_control_register_state 
						saved_s3_system_control_registers;
	struct s3_system_extension_register_state 
						saved_s3_system_extension_registers;
	struct s3_enhanced_commands_register_state 
						saved_s3_enhanced_commands_registers;
	unsigned long saved_s3_extra_registers[S3_NUMBER_OF_EXTRA_REGISTERS];
	unsigned char saved_standard_vga_palette[S3_STANDARD_VGA_PALETTE_SIZE*3];
	enum chipset_register_state_kind   current_chipset_state_kind;
};

/***
 *** Macros
 ***/
#define S3_READ_CRTC_REGISTER(CRTC_REGISTER_INDEX,STORE) \
	outb(screen_state_p->vga_crtc_address, (CRTC_REGISTER_INDEX));\
	STORE = inb(screen_state_p->vga_crtc_data);

#define S3_WRITE_CRTC_REGISTER(CRTC_REGISTER_INDEX,VALUE) \
	outb(screen_state_p->vga_crtc_address, (CRTC_REGISTER_INDEX));\
	outb(screen_state_p->vga_crtc_data, (VALUE));

#define S3_READ_SEQUENCER_REGISTER(SEQ_REGISTER_INDEX,STORE) \
	outb( VGA_SEQUENCER_REGISTER_SEQX,(SEQ_REGISTER_INDEX));\
	STORE = inb(VGA_SEQUENCER_REGISTER_SEQ_DATA);			

#define S3_WRITE_SEQUENCER_REGISTER(SEQ_REGISTER_INDEX,VALUE) \
	outb(VGA_SEQUENCER_REGISTER_SEQX,(SEQ_REGISTER_INDEX));\
	outb(VGA_SEQUENCER_REGISTER_SEQ_DATA,(VALUE));			

#define S3_READ_GRAPHICS_CONTROLLER_REGISTER(GRAPHICS_REGISTER_INDEX,STORE) \
	outb(VGA_GRAPHICS_CONTROLLER_REGISTER_ADR,(GRAPHICS_REGISTER_INDEX));\
	STORE = inb(VGA_GRAPHICS_CONTROLLER_REGISTER_DATA);

#define S3_WRITE_GRAPHICS_CONTROLLER_REGISTER(GRAPHICS_REGISTER_INDEX,VALUE) \
	outb(VGA_GRAPHICS_CONTROLLER_REGISTER_ADR,(GRAPHICS_REGISTER_INDEX));\
	outb(VGA_GRAPHICS_CONTROLLER_REGISTER_DATA,(VALUE));			

#define S3_CLEAR_ATTRIBUTE_CONTROLLER_FLIPFLOP()\
	(void)inb(screen_state_p->vga_input_status_address);

#define S3_READ_ATTRIBUTE_CONTROLLER_REGISTER(ATTR_REGISTER_INDEX,STORE);\
	S3_CLEAR_ATTRIBUTE_CONTROLLER_FLIPFLOP();\
	outb(VGA_ATTRIBUTE_CNTL_REGISTER_ADR,(ATTR_REGISTER_INDEX));\
	STORE = inb(VGA_ATTRIBUTE_CNTL_REGISTER_DATA_R);

#define S3_WRITE_ATTRIBUTE_CONTROLLER_REGISTER(ATTR_REGISTER_INDEX,VALUE)\
	S3_CLEAR_ATTRIBUTE_CONTROLLER_FLIPFLOP();\
	outb(VGA_ATTRIBUTE_CNTL_REGISTER_ADR,(ATTR_REGISTER_INDEX));\
	outb(VGA_ATTRIBUTE_CNTL_REGISTER_DATA_W,(VALUE));

#define S3_UNLOCK_CLOCK_REGISTERS()\
	{\
		unsigned char	_bkwd3_reg;\
		S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_3_INDEX,_bkwd3_reg);\
		_bkwd3_reg &= ~0xE0;\
		S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_3_INDEX,_bkwd3_reg);\
	}

#define S3_LOCK_CLOCK_REGISTERS()\
	{\
		unsigned char	_bkwd3_reg;\
		S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_3_INDEX,_bkwd3_reg);\
		_bkwd3_reg &= ~0xE0;\
		_bkwd3_reg |= 0xA0;\
		S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_3_INDEX,_bkwd3_reg);\
	}

#define S3_UNLOCK_CRT_TIMING_REGISTERS()\
	{\
		unsigned char	_lock_reg;\
		S3_READ_CRTC_REGISTER(VGA_CRTC_REGISTER_VRE_INDEX,_lock_reg);\
		_lock_reg &= ~0x80;\
		S3_WRITE_CRTC_REGISTER(VGA_CRTC_REGISTER_VRE_INDEX,_lock_reg);\
		S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_REG_LOCK1_INDEX,0x48);\
		S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CRTR_LOCK_INDEX,_lock_reg);\
		_lock_reg &= ~0x30;\
		S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_CRTR_LOCK_INDEX,_lock_reg);\
	}

#define S3_LOCK_CRT_TIMING_REGISTERS()\
	{\
		unsigned char	_lock_reg;\
		S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CRTR_LOCK_INDEX,_lock_reg);\
		_lock_reg |= 0x30;\
		S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_CRTR_LOCK_INDEX,_lock_reg);\
		S3_READ_CRTC_REGISTER(VGA_CRTC_REGISTER_VRE_INDEX,_lock_reg);\
		_lock_reg |= 0x80;\
		S3_WRITE_CRTC_REGISTER(VGA_CRTC_REGISTER_VRE_INDEX,_lock_reg);\
	}

#define S3_UNLOCK_S3_VGA_REGISTERS() \
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_REG_LOCK1_INDEX,0x48)

#define S3_LOCK_S3_VGA_REGISTERS() \
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_REG_LOCK1_INDEX,0x00)

#define S3_UNLOCK_SYSTEM_REGISTERS() \
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_REG_LOCK2_INDEX,0xa0)

#define S3_LOCK_SYSTEM_REGISTERS() \
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_REG_LOCK2_INDEX,0x00)

#define S3_UNLOCK_ENHANCED_REGISTERS()\
	{\
		unsigned char cr_40;\
		S3_UNLOCK_S3_VGA_REGISTERS();\
		S3_UNLOCK_SYSTEM_REGISTERS();\
		S3_READ_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_SYS_CNFG_INDEX,cr_40);\
		cr_40 |= 0x1;\
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_SYS_CNFG_INDEX,cr_40);\
	}

#define S3_LOCK_ENHANCED_REGISTERS()\
	{\
		unsigned char cr_40;\
		S3_UNLOCK_S3_VGA_REGISTERS();\
		S3_UNLOCK_SYSTEM_REGISTERS();\
		S3_READ_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_SYS_CNFG_INDEX,cr_40);\
		cr_40 &= 0xfe;\
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_SYS_CNFG_INDEX,cr_40);\
	}


#define S3_RESET_AND_HOLD_SEQUENCER()\
	S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_RST_SYNC_INDEX,\
		VGA_SEQUENCER_RESET_VALUE);

#define S3_START_SEQUENCER()\
	S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_RST_SYNC_INDEX,\
		VGA_SEQUENCER_START_VALUE);

#define S3_TURN_SCREEN_OFF()\
	{\
		unsigned char 	seq_1;\
		S3_READ_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,\
			seq_1);\
		seq_1 |= VGA_SEQUENCER_CLK_MODE_SCREEN_OFF;\
		S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,\
			seq_1);\
	}

#define S3_TURN_SCREEN_ON()\
	{\
		unsigned char 	seq_1;\
		S3_READ_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,\
			seq_1);\
		seq_1 &= ~VGA_SEQUENCER_CLK_MODE_SCREEN_OFF;\
		S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,\
			seq_1);\
	}

/* 
 * Wait for display period to start.
 * Wait for sync period to start.
 * Repeat this COUNT times.
 */
#define HORIZONTAL_SYNC	0
#define VERTICAL_SYNC	1
#define S3_WAIT_FOR_SYNC(COUNT,SYNC_TYPE)\
	{\
		int	i = (COUNT);\
		unsigned char val;\
		while (i--)\
		{\
			int	j = s3_crtc_sync_loop_timeout_count;\
			do\
			{\
				val = inb(screen_state_p->vga_input_status_address)&0x1;\
			} while(--j && val);\
			if (j <= 0)\
			{\
				(void) fprintf(stderr,WAIT_FOR_SYNC_FAILED_MESSAGE);\
				ASSERT(0);\
			}\
			j = s3_crtc_sync_loop_timeout_count;\
			do\
			{\
				if(SYNC_TYPE == HORIZONTAL_SYNC)\
				{\
					val = inb(screen_state_p->vga_input_status_address)&0x01;\
				}\
				else\
				{\
					val = inb(screen_state_p->vga_input_status_address)&0x08;\
				}\
			} while(--j && (!val));\
			if (j <= 0)\
			{\
				(void) fprintf(stderr,WAIT_FOR_SYNC_FAILED_MESSAGE);\
				ASSERT(0);\
			}\
		}\
	}

#define S3_WAIT_FOR_VBLANK_INTERVAL(SCREEN_STATE_P)\
{\
	 volatile int __count = s3_crtc_sync_loop_timeout_count;\
	 for(;(__count > 0) && \
		!(inb(SCREEN_STATE_P->vga_input_status_address)&0x8);__count--)\
	 {\
		  ;\
	 }\
	if (__count <= 0)\
	{\
		(void) fprintf(stderr, "\nS3: VBLANK TIMEOUT "\
					   "\"%s\",%d\n", __FILE__, __LINE__);\
	}\
}

#define S3_MICRO_DELAY()\
	{\
		volatile int _count = s3_graphics_engine_micro_delay_count ;\
		while (_count--)\
		{\
			;\
		}\
	}

#define	S3_IO_SET_ENHANCED_REGISTER(PORT,VALUE)\
	{\
		outw((PORT) ^ \
			screen_state_p->enhanced_register_io_address_xor_value,(VALUE));\
	}

#if (defined(lint))
extern int s3_first_mmioable_enhanced_register ;
#define S3_FIRST_ENHANCED_REGISTER_THAT_CAN_USE_MMIO \
	s3_first_mmioable_enhanced_register
#else
#define S3_FIRST_ENHANCED_REGISTER_THAT_CAN_USE_MMIO 0x8000
#endif

/*
 * In case linear frame buffer is enabled then the enhanced registers are
 * to be accessed only through the io address space. mmio is out.
 */
#define	S3_MMIO_SET_ENHANCED_REGISTER(PORT,VALUE)\
	{\
		ASSERT(screen_state_p->use_mmio);\
		ASSERT(PORT > S3_FIRST_ENHANCED_REGISTER_THAT_CAN_USE_MMIO);\
		*((volatile short *)\
		  ((unsigned)(screen_state_p->mmio_base_address)+(PORT))) = \
			(VALUE);\
	}

#define S3_SET_ENHANCED_REGISTER(PORT,VALUE)\
	if(screen_state_p->use_mmio && \
		(PORT > S3_FIRST_ENHANCED_REGISTER_THAT_CAN_USE_MMIO))\
	{\
		S3_MMIO_SET_ENHANCED_REGISTER((PORT),(VALUE))\
	}\
	else\
	{\
		S3_IO_SET_ENHANCED_REGISTER((PORT),(VALUE))\
	}

#define S3_INLINE_WAIT_FOR_FIFO(N_ENTRIES)\
	{\
		if((s3_graphics_engine_number_of_fifo_entries_free -= (N_ENTRIES))<0)\
		{\
			volatile int _count = s3_graphics_engine_loop_timeout_count;\
			while((inb(S3_ENHANCED_COMMAND_REGISTER_GP_STAT))&&(--_count > 0))\
			{\
				S3_MICRO_DELAY();\
			}\
			s3_graphics_engine_number_of_fifo_entries_free = 8 - (N_ENTRIES);\
			if (_count <= 0)\
			{\
				(void) fprintf(stderr,"S3: GE RESET %s:%d\n",__FILE__,__LINE__);\
				S3_SET_ENHANCED_REGISTER(\
					S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,\
					SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);\
				S3_SET_ENHANCED_REGISTER(\
					S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,\
					SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);\
					s3_graphics_engine_number_of_fifo_entries_free = 0;\
			}\
		}\
	}

#ifdef DELETE
#define S3_INLINE_WAIT_FOR_FIFO(N_ENTRIES)\
		{\
			volatile int _count = s3_graphics_engine_loop_timeout_count;\
			s3_graphics_engine_number_of_fifo_entries_free = 0;\
			while((inb(S3_ENHANCED_COMMAND_REGISTER_GP_STAT) & \
				(0x100 >> N_ENTRIES))&&(--_count > 0))\
			{\
				S3_MICRO_DELAY();\
			}\
			if (_count <= 0)\
			{\
				(void) fprintf(stderr,"S3: GE RESET %s:%d\n",__FILE__,__LINE__);\
				S3_SET_ENHANCED_REGISTER(\
					S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,\
					SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);\
				S3_SET_ENHANCED_REGISTER(\
					S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,\
					SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);\
			}\
		}
#endif

#define S3_WAIT_FOR_FIFO(N_ENTRIES)\
{\
	 if ((s3_graphics_engine_number_of_fifo_entries_free -= (N_ENTRIES))<0)\
	 {\
		 s3_register_wait_for_fifo(N_ENTRIES);\
	 }\
}

#ifdef DELETE
#define S3_IS_FIFO_OVERFLOW()\
	(inw(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_STAT) & \
		SUBSYS_STAT_FIFO_OVERFLOW)
#endif

#define S3_IS_FIFO_OVERFLOW() s3_is_fifo_overflow()

#define S3_WAIT_FOR_GE_IDLE()\
	{\
		volatile int _count = s3_graphics_engine_loop_timeout_count;\
		while ((inw(S3_ENHANCED_COMMAND_REGISTER_GP_STAT) & GP_STAT_GE_BUSY)\
			&& (--_count > 0))\
		{\
			S3_MICRO_DELAY();\
		}\
		if (_count <= 0)\
		{\
			(void) fprintf(stderr,"S3: GE RESET %s:%d\n",__FILE__,__LINE__);\
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,\
				SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);\
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,\
				SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);\
		}\
		else\
		{\
	 		s3_graphics_engine_number_of_fifo_entries_free = 8;\
		}\
	}

#define S3_WAIT_FOR_READ_DATA_IN_PIXTRANS_REGISTER()\
	{\
		volatile int _count = s3_graphics_engine_loop_timeout_count;\
		while ((!(inw(S3_ENHANCED_COMMAND_REGISTER_GP_STAT) & \
			GP_STAT_READ_DATA_AVAILABLE)) && (--_count > 0))\
		{\
			S3_MICRO_DELAY();\
		}\
		if (_count <= 0)\
		{\
			(void) fprintf(stderr,"S3: GE RESET %s:%d\n",__FILE__,__LINE__);\
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,\
				SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);\
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,\
				SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);\
		}\
	}

#define S3_SET_FG_COLOR_SOURCE(COLOR_SOURCE)\
	{\
		if((screen_state_p->register_state.s3_enhanced_commands_registers.\
			frgd_mix & S3_MIX_REGISTER_CLR_SRC_BITS) != (COLOR_SOURCE))\
		{\
			screen_state_p->register_state.s3_enhanced_commands_registers.\
				frgd_mix &= ~S3_MIX_REGISTER_CLR_SRC_BITS;\
			screen_state_p->register_state.s3_enhanced_commands_registers.\
				frgd_mix |= COLOR_SOURCE;\
			S3_WAIT_FOR_FIFO(1);\
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,\
			screen_state_p->register_state.s3_enhanced_commands_registers.\
				frgd_mix);\
		}\
	}

#define S3_SET_BG_COLOR_SOURCE(COLOR_SOURCE)\
	{\
		if((screen_state_p->register_state.s3_enhanced_commands_registers.\
			bkgd_mix & S3_MIX_REGISTER_CLR_SRC_BITS) != (COLOR_SOURCE))\
		{\
			screen_state_p->register_state.s3_enhanced_commands_registers.\
				bkgd_mix &= ~S3_MIX_REGISTER_CLR_SRC_BITS;\
			screen_state_p->register_state.s3_enhanced_commands_registers.\
				bkgd_mix |= COLOR_SOURCE;\
			S3_WAIT_FOR_FIFO(1);\
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,\
			screen_state_p->register_state.s3_enhanced_commands_registers.\
				bkgd_mix);\
		}\
	}

#define S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(MIX_REG_TYPE)\
	{\
		if((screen_state_p->register_state.s3_enhanced_commands_registers.\
			pixel_control & PIX_CNTL_DT_EX_SRC_BITS) != (MIX_REG_TYPE))\
		{\
			screen_state_p->register_state.s3_enhanced_commands_registers.\
				pixel_control &= ~PIX_CNTL_DT_EX_SRC_BITS;\
			screen_state_p->register_state.s3_enhanced_commands_registers.\
				pixel_control |= (MIX_REG_TYPE);\
			S3_WAIT_FOR_FIFO(1);\
			S3_SET_ENHANCED_REGISTER(\
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,\
				S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |\
				(screen_state_p->register_state.s3_enhanced_commands_registers.\
				pixel_control & S3_MULTIFUNC_VALUE_BITS));\
		}\
	}


#define S3_SWITCH_INTO_MMIO_MODE(SCREEN_STATE_P)\
	{\
		(SCREEN_STATE_P)->use_mmio = 1;\
		S3_UNLOCK_S3_VGA_REGISTERS();\
		S3_UNLOCK_SYSTEM_REGISTERS();\
		(SCREEN_STATE_P)->register_state.s3_system_extension_registers.\
		extended_mem_control_1 |= 	EX_MCTL_1_ENB_MMIO;\
		S3_WRITE_CRTC_REGISTER( S3_SYSTEM_EXTENSION_REGISTER_EX_MCTL_1,\
		(SCREEN_STATE_P)->register_state.s3_system_extension_registers.\
		extended_mem_control_1);\
		S3_LOCK_SYSTEM_REGISTERS();\
		S3_LOCK_S3_VGA_REGISTERS();\
	}

#define S3_SWITCH_OUTOF_MMIO_MODE(SCREEN_STATE_P)\
	{\
		(SCREEN_STATE_P)->use_mmio = 0;\
		S3_UNLOCK_S3_VGA_REGISTERS();\
		S3_UNLOCK_SYSTEM_REGISTERS();\
		(SCREEN_STATE_P)->register_state.s3_system_extension_registers.\
		extended_mem_control_1 &= 	~EX_MCTL_1_ENB_MMIO;\
		S3_WRITE_CRTC_REGISTER( S3_SYSTEM_EXTENSION_REGISTER_EX_MCTL_1,\
		(SCREEN_STATE_P)->register_state.s3_system_extension_registers.\
		extended_mem_control_1);\
		S3_LOCK_SYSTEM_REGISTERS();\
		S3_LOCK_S3_VGA_REGISTERS();\
	}

#define	S3_IS_X_OUT_OF_BOUNDS(x)\
	(((x) < 0)  || ((x) > (screen_state_p->generic_state.screen_physical_width - 1)))

#define	S3_IS_Y_OUT_OF_BOUNDS(y)\
	(((y) < 0)  || ((y) > (screen_state_p->generic_state.screen_physical_height - 1)))

/*
 * Hardware cursor manipulation macros
 */

#define	S3_ENABLE_HARDWARE_CURSOR(SCREEN_STATE_P)						\
{																		\
	unsigned char __hgc_mode__;											\
																		\
	S3_UNLOCK_S3_VGA_REGISTERS();										\
	S3_UNLOCK_SYSTEM_REGISTERS();										\
																		\
	S3_READ_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,	\
	  __hgc_mode__);													\
	__hgc_mode__ |= HGC_MODE_HWGC_ENB;									\
																		\
	(SCREEN_STATE_P)->													\
		register_state.s3_system_control_registers.hw_cursor_mode =		\
		__hgc_mode__;													\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,	\
	   __hgc_mode__);													\
	S3_LOCK_SYSTEM_REGISTERS();											\
	S3_LOCK_S3_VGA_REGISTERS();											\
}



#define	S3_DISABLE_HARDWARE_CURSOR(SCREEN_STATE_P)						\
{																		\
	unsigned char __hgc_mode__;											\
																		\
	S3_UNLOCK_S3_VGA_REGISTERS();										\
	S3_UNLOCK_SYSTEM_REGISTERS();										\
																		\
	S3_READ_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,	\
	  __hgc_mode__);													\
	__hgc_mode__ &= ~HGC_MODE_HWGC_ENB;									\
																		\
	(SCREEN_STATE_P)->													\
		register_state.s3_system_control_registers.hw_cursor_mode =		\
		__hgc_mode__;													\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,	\
	   __hgc_mode__);													\
	S3_LOCK_SYSTEM_REGISTERS();											\
	S3_LOCK_S3_VGA_REGISTERS();											\
}

#define S3_SET_HARDWARE_CURSOR_POSITION(SCREENSTATEP,X,Y)					\
{																			\
	unsigned char _cr46;													\
	unsigned char _cr47;													\
	unsigned char _cr48;													\
	unsigned char _cr49;													\
	S3_UNLOCK_S3_VGA_REGISTERS();											\
	S3_UNLOCK_SYSTEM_REGISTERS();											\
	_cr46 =  ((unsigned)(X) >> 8U) & HWGC_ORGX_HI_BITS;						\
	_cr47 =  (X) & HWGC_ORGX_LO_BITS;										\
	_cr48 =  ((unsigned)(Y) >> 8U) & HWGC_ORGY_HI_BITS;						\
	_cr49 =  (Y) & HWGC_ORGY_LO_BITS;										\
	(SCREENSTATEP)->register_state.s3_system_control_registers.				\
		hw_cursor_origin_x = (X);											\
	(SCREENSTATEP)->register_state.s3_system_control_registers.				\
		hw_cursor_origin_y = (Y);											\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_LO_INDEX,	\
		_cr47);																\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_HI_INDEX,	\
		_cr46);																\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_LO_INDEX,	\
		_cr49);																\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_HI_INDEX,	\
		_cr48);																\
	S3_LOCK_SYSTEM_REGISTERS();												\
	S3_LOCK_S3_VGA_REGISTERS();												\
}



#define	S3_SET_HARDWARE_CURSOR_STORAGE_START_ADDRESS(SCREENSTATEP,START_ADDR)\
{																			 \
	unsigned char _cr4c;													 \
	unsigned char _cr4d;													 \
	S3_UNLOCK_S3_VGA_REGISTERS();											 \
	S3_UNLOCK_SYSTEM_REGISTERS();											 \
	(SCREENSTATEP)->register_state.s3_system_control_registers.				 \
		hw_cursor_start_address = (START_ADDR);								 \
	_cr4c =  ((unsigned)(START_ADDR) >> 8U) & HWGC_STA_HI_BITS;				 \
	_cr4d =  (START_ADDR) & HWGC_STA_LO_BITS;								 \
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_HI_INDEX,	 \
		_cr4c);																 \
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_LO_INDEX,	 \
		_cr4d);																 \
	S3_LOCK_SYSTEM_REGISTERS();											 	 \
	S3_LOCK_S3_VGA_REGISTERS();												 \
}

#define	S3_SET_HARDWARE_CURSOR_HORZ_DISPLAY_OFFSET(SCREENSTATEP,HORZ_OFFSET)\
{																			\
	unsigned char _cr4e;													\
	S3_UNLOCK_S3_VGA_REGISTERS();											\
	S3_UNLOCK_SYSTEM_REGISTERS();											\
	(SCREENSTATEP)->register_state.s3_system_control_registers.				\
		hw_cursor_pattern_start_x_position =  (HORZ_OFFSET);				\
	_cr4e = (HORZ_OFFSET) & HWGC_DX_BITS;									\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_DX_INDEX,_cr4e); \
	S3_LOCK_SYSTEM_REGISTERS();												\
	S3_LOCK_S3_VGA_REGISTERS();												\
}

#define	S3_SET_HARDWARE_CURSOR_VERT_DISPLAY_OFFSET(SCREENSTATEP,VERT_OFFSET)\
{																			\
	unsigned char _cr4f;													\
	S3_UNLOCK_S3_VGA_REGISTERS();											\
	S3_UNLOCK_SYSTEM_REGISTERS();											\
	(SCREENSTATEP)->register_state.s3_system_control_registers.				\
		hw_cursor_pattern_start_y_position =  (VERT_OFFSET);				\
	_cr4f = (VERT_OFFSET) & HWGC_DY_BITS;									\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_DY_INDEX,_cr4f); \
	S3_LOCK_SYSTEM_REGISTERS();												\
	S3_LOCK_S3_VGA_REGISTERS();												\
}


/*
 * Cursor colors for 4/8 bit modes *alone*
 * Cursor colors for  16 bit modes are more involved and require programming
 * the cursor color stack. Does not look like the crtc regs should be unlocked
 * for this operation, but check this out.
 */
#define	S3_SET_HARDWARE_CURSOR_COLORS(SCREENSTATEP,FG_COLOR,BG_COLOR)		  \
{																			  \
																			  \
	switch((SCREENSTATEP)->generic_state.screen_depth) 						  \
	{																		  \
		case 4:																  \
		case 8:																  \
			screen_state_p->register_state.standard_vga_registers.			  \
				standard_vga_crtc_registers.cursor_loc_addr_l =  (BG_COLOR);  \
			screen_state_p->register_state.standard_vga_registers.			  \
				standard_vga_crtc_registers.cursor_loc_addr_h = (FG_COLOR);	  \
			S3_WRITE_CRTC_REGISTER(VGA_CRTC_REGISTER_CLA_H_INDEX,FG_COLOR);	  \
			S3_WRITE_CRTC_REGISTER(VGA_CRTC_REGISTER_CLA_L_INDEX,BG_COLOR);	  \
			break;															  \
		case 16:															  \
		{																	  \
			unsigned char _cr45;											  \
			S3_UNLOCK_S3_VGA_REGISTERS();									  \
			S3_UNLOCK_SYSTEM_REGISTERS();									  \
			/*																  \
			 *Reset the cursor color stack pointer							  \
			 */																  \
			S3_READ_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,					  \
				_cr45);														  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_FGSTK_INDEX,				  \
				(FG_COLOR) & 0xFF);											  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_FGSTK_INDEX,				  \
				((FG_COLOR) >> 8) & 0xFF);									  \
																			  \
			S3_READ_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,					  \
				_cr45);														  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_BGSTK_INDEX,				  \
				(BG_COLOR) & 0xFF);											  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_BGSTK_INDEX,				  \
				((BG_COLOR) >> 8) & 0xFF);									  \
			S3_LOCK_SYSTEM_REGISTERS();										  \
			S3_LOCK_S3_VGA_REGISTERS();										  \
		}																	  \
		break;																  \
		case 24:															  \
		{																	  \
			unsigned char _cr45;											  \
			S3_UNLOCK_S3_VGA_REGISTERS();									  \
			S3_UNLOCK_SYSTEM_REGISTERS();									  \
			/*																  \
			 *Reset the cursor color stack pointer							  \
			 */																  \
			S3_READ_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,					  \
				_cr45);														  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_FGSTK_INDEX,				  \
				(FG_COLOR) & 0xFF);											  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_FGSTK_INDEX,				  \
				((FG_COLOR) >> 8) & 0xFF);									  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_FGSTK_INDEX,				  \
				((FG_COLOR) >> 16) & 0xFF);									  \
																			  \
			S3_READ_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,					  \
				_cr45);														  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_BGSTK_INDEX,				  \
				(BG_COLOR) & 0xFF);											  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_BGSTK_INDEX,				  \
				((BG_COLOR) >> 8) & 0xFF);									  \
			S3_WRITE_CRTC_REGISTER(											  \
				S3_SYSTEM_CONTROL_REGISTER_HWGC_BGSTK_INDEX,				  \
				((BG_COLOR) >> 16) & 0xFF);									  \
			S3_LOCK_SYSTEM_REGISTERS();										  \
			S3_LOCK_S3_VGA_REGISTERS();										  \
		}																	  \
			break;															  \
		default:															  \
			/*CONSTANTCONDITION*/											  \
			ASSERT(0);														  \
	}																		  \
}

/*
 * Program the display start address value. This macro is used for 
 * hardware panning.
 */
#define	S3_SET_DISPLAY_START_ADDRESS(SCREENSTATEP,X,Y)					\
{																		\
	unsigned char	_crc = 0;											\
	unsigned char	_crd = 0;											\
	unsigned char	_cr31 = 0;											\
	unsigned char	_cr51 = 0;											\
	unsigned int	_start_address;										\
	  																	\
	_start_address =													\
		((X) /(32 / (SCREENSTATEP)->generic_state.screen_depth)) +		\
		(Y) * (SCREENSTATEP)->register_state.standard_vga_registers.	\
		standard_vga_crtc_registers.screen_offset * 2;					\
																		\
	_crd = _start_address & 0xFF;										\
	_start_address >>= 8U;												\
	_crc = _start_address  & 0xFF;										\
	_start_address >>= 4U;												\
																		\
	S3_UNLOCK_S3_VGA_REGISTERS();										\
	S3_UNLOCK_SYSTEM_REGISTERS();										\
																		\
	S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_MEM_CNFG_INDEX ,_cr31);		\
	S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_2 ,_cr51);\
																		\
	_cr31 = _cr31 & ~MEM_CNFG_STRT_ADR;									\
	_cr31 = _cr31 | (_start_address & MEM_CNFG_STRT_ADR);				\
	_start_address >>= 6U;												\
																		\
	_cr51 = _cr51 & ~EX_SCTL_2_DISP_ST_AD;								\
	_cr51 =  _cr51  | (_start_address & EX_SCTL_2_DISP_ST_AD);			\
																		\
	S3_WRITE_CRTC_REGISTER(VGA_CRTC_REGISTER_STA_H_INDEX , _crc);		\
	S3_WRITE_CRTC_REGISTER(VGA_CRTC_REGISTER_STA_L_INDEX , _crd);		\
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_MEM_CNFG_INDEX ,_cr31);		\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_2 ,_cr51);\
	S3_LOCK_SYSTEM_REGISTERS();											\
	S3_LOCK_S3_VGA_REGISTERS();											\
}

/*
 * BT485 registers and related macros.
 */
#define BT485_WRITE_ADDR_INDEX		0x00
#define BT485_RAMDAC_DATA_INDEX		0x01	
#define BT485_PIXEL_MASK_INDEX		0x02
#define BT485_READ_ADDR_INDEX		0x03
#define BT485_CURS_WR_ADDR_INDEX	0x04
#define BT485_CURS_DATA_INDEX		0x05
#define BT485_COMMAND_REG_0_INDEX	0x06
#define BT485_CURS_RD_ADDR_INDEX	0x07
#define BT485_COMMAND_REG_1_INDEX	0x08
#define BT485_COMMAND_REG_2_INDEX	0x09
#define BT485_STATUS_REG_INDEX		0x0A
#define BT485_CURS_RAM_DATA_INDEX	0x0B
#define BT485_CURS_X_LOW_INDEX		0x0C
#define BT485_CURS_X_HIGH_INDEX		0x0D
#define BT485_CURS_Y_LOW_INDEX		0x0E
#define BT485_CURS_Y_HIGH_INDEX		0x0F

#define BT485_COMMAND_REG_0			0x3C6
#define BT485_COMMAND_REG_1			0x3c8
#define BT485_COMMAND_REG_2			0x3c9
#define BT485_WRITE_ADDR			0x3C8
#define BT485_STATUS_REG			0x3C6

#define BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK		0x80
#define BT485_COMMAND_REG_0_RESOLUTION_6BIT				0x00
#define BT485_COMMAND_REG_0_RESOLUTION_8BIT				0x02
#define BT485_COMMAND_REG_0_SLEEP_MODE					0x01
#define BT485_COMMAND_REG_1_PIXEL_SELECT_4				0x60
#define BT485_COMMAND_REG_1_PIXEL_SELECT_8				0x40
#define BT485_COMMAND_REG_1_PIXEL_SELECT_16				0x20
#define BT485_COMMAND_REG_1_PIXEL_SELECT_24				0x00
#define BT485_COMMAND_REG_1_TRUE_COLOR_BYPASS			0x10
#define BT485_COMMAND_REG_1_16_BIT_FORMAT_555			0x00
#define BT485_COMMAND_REG_1_16_BIT_FORMAT_565			0x08
#define BT485_COMMAND_REG_1_16_BIT_MUX_2_IS_1			0x00
#define BT485_COMMAND_REG_1_16_BIT_MUX_1_IS_1			0x04
#define BT485_COMMAND_REG_2_PORTSEL_UNMASK				0x20
#define BT485_COMMAND_REG_2_CLKSEL_PCLK1				0x10
#define BT485_COMMAND_REG_2_DISPLAY_MODE_INTERLACED		0x80
#define BT485_COMMAND_REG_3_2X_CLOCK_MULTIPLY			0x08

#define BT485_SELECT_REGISTER_SET(REGISTER_SET_NUMBER)\
{\
	unsigned char __cr55__;\
	S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,__cr55__);\
	__cr55__ &= ~EX_DAC_CT_DAC_R_SEL_BITS;\
	__cr55__ |= ((REGISTER_SET_NUMBER) & EX_DAC_CT_DAC_R_SEL_BITS);\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,__cr55__);\
}

/*
 * The Command Register 3 in the Bt485 must be accessed through
 * a very odd sequence, as the older RAMDACs had already defined 16
 * registers. Apparently this overlays the Status register.
 * - from xfree86 ( also refer bt485 advance information page 4-216.)
 */
#define BT485_OUT_COMMAND_REGISTER_3(MASK,VALUE)\
{\
	unsigned char __tmp__;\
	BT485_SELECT_REGISTER_SET(1);\
	__tmp__  = (inb(BT485_COMMAND_REG_0) & 0x7F) | 0x80;\
	outb(BT485_COMMAND_REG_0, __tmp__);\
	BT485_SELECT_REGISTER_SET(0);\
	outb(BT485_WRITE_ADDR, 0x01);\
	BT485_SELECT_REGISTER_SET(2);\
	__tmp__  = ((MASK)? (inb(BT485_STATUS_REG) & (MASK)) : 0);\
	outb(BT485_STATUS_REG, __tmp__ | (VALUE));\
	BT485_SELECT_REGISTER_SET(0);\
}

/*
 * TI3025/3020 Dac registers and related macros. 
 *
 */

/* Direct standard VGA registers, special index and data registers */

#define TI_WRITE_ADDR       0x3C8   /* CR55 low bit == 0 */
#define TI_RAMDAC_DATA      0x3C9   /* CR55 low bit == 0 */
#define TI_PIXEL_MASK       0x3C6   /* CR55 low bit == 0 */
#define TI_READ_ADDR        0x3C7   /* CR55 low bit == 0 */
#define TI_INDEX_REG        0x3C6   /* CR55 low bit == 1 */
#define TI_DATA_REG     	0x3C7   /* CR55 low bit == 1 */

/* Indirect indexed registers */

#define TI_CURS_X_LOW       	0x00
#define TI_CURS_X_HIGH      	0x01    /* only lower 4 bits are used */
#define TI_CURS_Y_LOW       	0x02
#define TI_CURS_Y_HIGH      	0x03    /* only lower 4 bits are used */
#define TI_SPRITE_ORIGIN_X  	0x04
#define TI_SPRITE_ORIGIN_Y  	0x05
#define TI_CURS_CONTROL     	0x06
#define TI_CURS_RAM_ADDR_LOW    0x08		/* W */
#define TI_CURS_RAM_ADDR_HIGH   0x09		/* W */
#define TI_CURS_RAM_DATA    	0x0A

#define TI3025_TRUECOLOR_CONTROL	0x0E
#define TI3025_VGA_SWITCH_CONTROL	0x0F

#define TI_WINDOW_START_X_LOW   0x10
#define TI_WINDOW_START_X_HIGH  0x11
#define TI_WINDOW_STOP_X_LOW    0x12
#define TI_WINDOW_STOP_X_HIGH   0x13
#define TI_WINDOW_START_Y_LOW   0x14
#define TI_WINDOW_START_Y_HIGH  0x15
#define TI_WINDOW_STOP_Y_LOW    0x16
#define TI_WINDOW_STOP_Y_HIGH   0x17
#define TI_MUX_CONTROL_1    	0x18
#define TI_MUX_CONTROL_2    	0x19
#define TI_INPUT_CLOCK_SELECT   0x1A
#define TI_OUTPUT_CLOCK_SELECT  0x1B
#define TI_PALETTE_PAGE     	0x1C
#define TI_GENERAL_CONTROL  	0x1D

#define	TI3025_MISC_CONTROL			0x1E

#define TI_OVERSCAN_COLOR_RED   0x20
#define TI_OVERSCAN_COLOR_GREEN 0x21
#define TI_OVERSCAN_COLOR_BLUE  0x22
#define TI_CURSOR_COLOR_0_RED   0x23
#define TI_CURSOR_COLOR_0_GREEN 0x24
#define TI_CURSOR_COLOR_0_BLUE  0x25
#define TI_CURSOR_COLOR_1_RED   0x26
#define TI_CURSOR_COLOR_1_GREEN 0x27
#define TI_CURSOR_COLOR_1_BLUE  0x28
#define TI_AUXILIARY_CONTROL    0x29
#define TI_GENERAL_IO_CONTROL   0x2A
#define TI_GENERAL_IO_DATA  	0x2B

#define TI3025_PLL_CONTROL				0x2C
#define TI3025_PIXEL_CLOCK_PLL_DATA		0x2D
#define TI3025_MEMORY_CLOCK_PLL_DATA	0x2E
#define TI3025_LOOP_CLOCK_PLL_DATA		0x2F

#define TI_COLOR_KEY_OLVGA_LOW  0x30
#define TI_COLOR_KEY_OLVGA_HIGH 0x31
#define TI_COLOR_KEY_RED_LOW    0x32
#define TI_COLOR_KEY_RED_HIGH   0x33
#define TI_COLOR_KEY_GREEN_LOW  0x34
#define TI_COLOR_KEY_GREEN_HIGH 0x35
#define TI_COLOR_KEY_BLUE_LOW   0x36
#define TI_COLOR_KEY_BLUE_HIGH  0x37
#define TI_COLOR_KEY_CONTROL    0x38

#define TI3025_M_D_CLK_CONTROL			0x39

#define TI_SENSE_TEST       	0x3A
#define TI_TEST_DATA        	0x3B		/* R */
#define TI_CRC_LOW      		0x3C		/* R */
#define TI_CRC_HIGH     		0x3D		/* R */
#define TI_CRC_CONTROL      	0x3E		/* W */
#define TI_ID           		0x3F		/* R */

#define TI3025_MODE85_CONTROL		0xD5
#define TI3025_RESET_REGISTER		0xFF	/* W */


/*
 * Cursor Control Register.
 */
#define   TI_CURS_SPRITE_ENABLE 0x40
#define   TI_CURS_X_WINDOW_MODE 0x10
#define   TI_CURS_CTRL_MASK     (TI_CURS_SPRITE_ENABLE | TI_CURS_X_WINDOW_MODE)

/*
 * Mux control 1 register.
 */
#define   TI_MUX1_PSEUDO_COLOR  0x80

/*
 * Mux control 2 register.
 */
#define   TI_MUX2_BUS_VGA   	0x98
#define   TI_MUX2_BUS_PIX64 	0x1C

/*
 * Input Clock Selection Register.
 */
#define   TI_ICLK_CLK0      	0x00
#define   TI_ICLK_CLK0_DOUBLE   0x10
#define   TI_ICLK_CLK1      	0x01
#define   TI_ICLK_CLK1_DOUBLE   0x11
#define	  TI_ICLK_PCLK_PLL		0x05	

/*
 * Output Clock Selection Register.
 */
#define   TI_OCLK_VGA       	0x3E
#define   TI_OCLK_S_V4_R8   	0x53

/*
 * Auxiliary control register.
 */
#define   TI_AUX_SELF_CLOCK 	0x08
#define   TI_AUX_W_CMPL     	0x01

/*
 * General I/O control register.
 */
#define   TI_GIC_ALL_BITS   	0x1F

/*
 * General I/O data register.
 */
#define   TI_GID_S3_DAC_6BIT    0x1C
#define   TI_GID_S3_DAC_8BIT    0x1E
#define   TI_GID_TI_DAC_6BIT    0x1D
#define   TI_GID_TI_DAC_8BIT    0x1F

/*
 * Pixel Clock PLL Data Register.
 */
#define   PCLK_PLL_DATA_P_VALUE_PCLKOUT 0x04

/*
 * ID register.
 */
#define   TI3020_ID   	0x20
#define   TI3025_ID   	0x81

#define S3_WRITE_TI_DAC_REGISTER(INDEX, DATA)\
{\
	unsigned char __tmp__;\
	S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT, __tmp__);\
   	__tmp__ &= 0xFC;\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT, \
													__tmp__ | 0x01);\
   	outb(TI_INDEX_REG, INDEX);\
	outb(TI_DATA_REG, DATA);\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT, __tmp__);\
}\

#define	S3_READ_TI_DAC_REGISTER(INDEX, DATA)\
{\
	unsigned char __tmp__;\
	S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT, __tmp__);\
   	__tmp__ &= 0xFC;\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT, \
													__tmp__ | 0x01);\
   	outb(TI_INDEX_REG, INDEX);\
	DATA = inb(TI_DATA_REG);\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT, __tmp__);\
}\

/*
 *	Forward declarations.
 */

/***
 ***	Variables.
 ***/

/*
 * Globals, since tucking them into the state structures 
 * might affect speed.
 */
extern int s3_graphics_engine_micro_delay_count ;
extern int s3_graphics_engine_loop_timeout_count ;
extern int s3_crtc_sync_loop_timeout_count ;
extern int s3_dac_access_delay_count ;
extern int s3_graphics_engine_fifo_blocking_factor ;
extern int s3_graphics_engine_number_of_fifo_entries_free ;


/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean s3_register_debug ;
#endif

#endif	/* __ASSEMBLER__ */

/*
 *	Current module state.
 */

extern void
s3_register__initialize__(SIScreenRec *si_screen_p,
							struct s3_options_structure *options_p)
;


#endif
