/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_regs.h	1.7"
#if (! defined(__M64_REGS_INCLUDED__))

#define __M64_REGS_INCLUDED__



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
#include "m64_opt.h"

/***
 ***	Constants.
 ***/

/* 
 * NON-GUI Registers in io space 
 */

#define M64_IO_REGISTER_CRTC_H_TOTAL_DISP     0x02EC
#define M64_IO_REGISTER_CRTC_H_SYNC_STRT_WID  0x06EC
#define M64_IO_REGISTER_CRTC_V_TOTAL_DISP     0x0AEC
#define M64_IO_REGISTER_CRTC_V_SYNC_STRT_WID  0x0EEC
#define M64_IO_REGISTER_CRTC_VLINE_CRNT_VLINE 0x12EC
#define M64_IO_REGISTER_CRTC_OFF_PITCH        0x16EC
#define M64_IO_REGISTER_CRTC_INT_CNTL         0x1AEC
#define M64_IO_REGISTER_CRTC_GEN_CNTL         0x1EEC
#define M64_IO_REGISTER_CRTC_EXT_DISP \
			(M64_IO_REGISTER_CRTC_GEN_CNTL + 3)
#define M64_IO_REGISTER_OVR_CLR               0x22EC
#define M64_IO_REGISTER_OVR_WID_LEFT_RIGHT    0x26EC
#define M64_IO_REGISTER_OVR_WID_TOP_BOTTOM    0x2AEC
#define M64_IO_REGISTER_CUR_CLR0              0x2EEC
#define M64_IO_REGISTER_CUR_CLR1              0x32EC
#define M64_IO_REGISTER_CUR_OFFSET            0x36EC
#define M64_IO_REGISTER_CUR_HORZ_VERT_POSN    0x3AEC
#define M64_IO_REGISTER_CUR_HORZ_VERT_OFF     0x3EEC
#define M64_IO_REGISTER_SCRATCH_REG0          0x42EC
#define M64_IO_REGISTER_SCRATCH_REG1          0x46EC
#define M64_IO_REGISTER_CLOCK_CNTL        	  0x4AEC
#define M64_IO_REGISTER_BUS_CNTL              0x4EEC
#define M64_IO_REGISTER_MEM_CNTL              0x52EC
#define M64_IO_REGISTER_MEM_VGA_WP_SEL        0x56EC
#define M64_IO_REGISTER_MEM_VGA_RP_SEL        0x5AEC
#define M64_IO_REGISTER_DAC_REGS              0x5EEC
#define M64_IO_REGISTER_DAC_W_INDEX           0x5EEC
#define M64_IO_REGISTER_DAC_CNTL              0x62EC
#define M64_IO_REGISTER_GEN_TEST_CNTL         0x66EC
#define M64_IO_REGISTER_CONFIG_CNTL           0x6AEC
#define M64_IO_REGISTER_CONFIG_CHIP_ID        0x6EEC
#define M64_IO_REGISTER_CONFIG_STAT0          0x72EC
#define M64_IO_REGISTER_CONFIG_STAT1          0x76EC


/* 
 * NON-GUI Registers that can be memory mapped. Expressed in long offsets
 * Character offsets are given within comments.
 */

#define M64_REGISTER_CRTC_H_TOTAL_DISP_OFFSET       0x00 /* 0x0000 */
#define M64_REGISTER_CRTC_H_SYNC_STRT_WID_OFFSET    0x01 /* 0x0004 */
#define M64_REGISTER_CRTC_V_TOTAL_DISP_OFFSET       0x02 /* 0x0008 */
#define M64_REGISTER_CRTC_V_SYNC_STRT_WID_OFFSET    0x03 /* 0x000C */
#define M64_REGISTER_CRTC_VLINE_CRNT_VLINE_OFFSET   0x04 /* 0x0010 */
#define M64_REGISTER_CRTC_OFF_PITCH_OFFSET          0x05 /* 0x0014 */
#define M64_REGISTER_CRTC_INT_CNTL_OFFSET           0x06 /* 0x0018 */
#define M64_REGISTER_CRTC_GEN_CNTL_OFFSET           0x07 /* 0x001C */
#define M64_REGISTER_OVR_CLR_OFFSET                 0x10 /* 0x0040 */
#define M64_REGISTER_OVR_WID_LEFT_RIGHT_OFFSET      0x11 /* 0x0044 */
#define M64_REGISTER_OVR_WID_TOP_BOTTOM_OFFSET      0x12 /* 0x0048 */
#define M64_REGISTER_CUR_CLR0_OFFSET                0x18 /* 0x0060 */
#define M64_REGISTER_CUR_CLR1_OFFSET                0x19 /* 0x0064 */
#define M64_REGISTER_CUR_OFFSET_OFFSET              0x1A /* 0x0068 */
#define M64_REGISTER_CUR_HORZ_VERT_POSN_OFFSET      0x1B /* 0x006C */
#define M64_REGISTER_CUR_HORZ_VERT_OFF_OFFSET       0x1C /* 0x0070 */
#define M64_REGISTER_SCRATCH_REG0_OFFSET            0x20 /* 0x0080 */
#define M64_REGISTER_SCRATCH_REG1_OFFSET            0x21 /* 0x0084 */
#define M64_REGISTER_CLOCK_CNTL_OFFSET          	0x24 /* 0x0090 */
#define M64_REGISTER_BUS_CNTL_OFFSET                0x28 /* 0x00A0 */
#define M64_REGISTER_MEM_CNTL_OFFSET                0x2C /* 0x00B0 */
#define M64_REGISTER_MEM_VGA_WP_SEL_OFFSET          0x2D /* 0x00B4 */
#define M64_REGISTER_MEM_VGA_RP_SEL_OFFSET          0x2E /* 0x00B8 */
#define M64_REGISTER_DAC_REGS_OFFSET                0x30 /* 0x00C0 */
#define M64_REGISTER_DAC_CNTL_OFFSET                0x31 /* 0x00C4 */
#define M64_REGISTER_GEN_TEST_CNTL_OFFSET           0x34 /* 0x00D0 */
#define M64_REGISTER_CONFIG_CHIP_ID_OFFSET          0x38 /* 0x00E0 */
#define M64_REGISTER_CONFIG_STAT0_OFFSET            0x39 /* 0x00E4 */
#define M64_REGISTER_CONFIG_STAT1_OFFSET            0x3A /* 0x00E8 */


/* 
 * GUI MEMORY MAPPED Registers. In byte offsets.
 * Offsets within comments are Dword (32 bits in dos) offsets.
 */
#define M64_REGISTER_DST_OFF_PITCH_OFFSET           0x40 /* 0x0100 */
#define M64_REGISTER_DST_X_OFFSET                   0x41 /* 0x0104 */
#define M64_REGISTER_DST_Y_OFFSET                   0x42 /* 0x0108 */
#define M64_REGISTER_DST_Y_X_OFFSET                 0x43 /* 0x010C */
#define M64_REGISTER_DST_WID_OFFSET               	0x44 /* 0x0110 */
#define M64_REGISTER_DST_HEIGHT_OFFSET              0x45 /* 0x0114 */
#define M64_REGISTER_DST_HEIGHT_WID_OFFSET        	0x46 /* 0x0118 */
#define M64_REGISTER_DST_X_WID_OFFSET             	0x47 /* 0x011C */
#define M64_REGISTER_DST_BRES_LNTH_OFFSET           0x48 /* 0x0120 */
#define M64_REGISTER_DST_BRES_ERR_OFFSET            0x49 /* 0x0124 */
#define M64_REGISTER_DST_BRES_INC_OFFSET            0x4A /* 0x0128 */
#define M64_REGISTER_DST_BRES_DEC_OFFSET            0x4B /* 0x012C */
#define M64_REGISTER_DST_CNTL_OFFSET                0x4C /* 0x0130 */
#define M64_REGISTER_SRC_OFF_PITCH_OFFSET           0x60 /* 0x0180 */
#define M64_REGISTER_SRC_X_OFFSET                   0x61 /* 0x0184 */
#define M64_REGISTER_SRC_Y_OFFSET                   0x62 /* 0x0188 */
#define M64_REGISTER_SRC_Y_X_OFFSET                 0x63 /* 0x018C */
#define M64_REGISTER_SRC_WID1_OFFSET              	0x64 /* 0x0190 */
#define M64_REGISTER_SRC_HEIGHT1_OFFSET             0x65 /* 0x0194 */
#define M64_REGISTER_SRC_HEIGHT1_WID1_OFFSET      	0x66 /* 0x0198 */
#define M64_REGISTER_SRC_X_START_OFFSET             0x67 /* 0x019C */
#define M64_REGISTER_SRC_Y_START_OFFSET             0x68 /* 0x01A0 */
#define M64_REGISTER_SRC_Y_X_START_OFFSET           0x69 /* 0x01A4 */
#define M64_REGISTER_SRC_WID2_OFFSET              	0x6A /* 0x01A8 */
#define M64_REGISTER_SRC_HEIGHT2_OFFSET             0x6B /* 0x01AC */
#define M64_REGISTER_SRC_HEIGHT2_WID2_OFFSET      	0x6C /* 0x01B0 */
#define M64_REGISTER_SRC_CNTL_OFFSET                0x6D /* 0x01B4 */
#define M64_REGISTER_HOST_DATA0_OFFSET              0x80 /* 0x0200 */
#define M64_REGISTER_HOST_DATA1_OFFSET              0x81 /* 0x0204 */
#define M64_REGISTER_HOST_DATA2_OFFSET              0x82 /* 0x0208 */
#define M64_REGISTER_HOST_DATA3_OFFSET              0x83 /* 0x020C */
#define M64_REGISTER_HOST_DATA4_OFFSET              0x84 /* 0x0210 */
#define M64_REGISTER_HOST_DATA5_OFFSET              0x85 /* 0x0214 */
#define M64_REGISTER_HOST_DATA6_OFFSET              0x86 /* 0x0218 */
#define M64_REGISTER_HOST_DATA7_OFFSET              0x87 /* 0x021C */
#define M64_REGISTER_HOST_DATA8_OFFSET              0x88 /* 0x0220 */
#define M64_REGISTER_HOST_DATA9_OFFSET              0x89 /* 0x0224 */
#define M64_REGISTER_HOST_DATAA_OFFSET              0x8A /* 0x0228 */
#define M64_REGISTER_HOST_DATAB_OFFSET              0x8B /* 0x022C */
#define M64_REGISTER_HOST_DATAC_OFFSET              0x8C /* 0x0230 */
#define M64_REGISTER_HOST_DATAD_OFFSET              0x8D /* 0x0234 */
#define M64_REGISTER_HOST_DATAE_OFFSET              0x8E /* 0x0238 */
#define M64_REGISTER_HOST_DATAF_OFFSET              0x8F /* 0x023C */
#define M64_REGISTER_HOST_CNTL_OFFSET               0x90 /* 0x0240 */
#define M64_REGISTER_PAT_REG0_OFFSET                0xA0 /* 0x0280 */
#define M64_REGISTER_PAT_REG1_OFFSET                0xA1 /* 0x0284 */
#define M64_REGISTER_PAT_CNTL_OFFSET                0xA2 /* 0x0288 */
#define M64_REGISTER_SC_LEFT_OFFSET                 0xA8 /* 0x02A0 */
#define M64_REGISTER_SC_RIGHT_OFFSET                0xA9 /* 0x02A4 */
#define M64_REGISTER_SC_LEFT_RIGHT_OFFSET           0xAA /* 0x02A8 */
#define M64_REGISTER_SC_TOP_OFFSET                  0xAB /* 0x02AC */
#define M64_REGISTER_SC_BOTTOM_OFFSET               0xAC /* 0x02B0 */
#define M64_REGISTER_SC_TOP_BOTTOM_OFFSET           0xAD /* 0x02B4 */
#define M64_REGISTER_DP_BKGD_CLR_OFFSET             0xB0 /* 0x02C0 */
#define M64_REGISTER_DP_FRGD_CLR_OFFSET             0xB1 /* 0x02C4 */
#define M64_REGISTER_DP_WRITE_MASK_OFFSET           0xB2 /* 0x02C8 */
#define M64_REGISTER_DP_CHAIN_MASK_OFFSET           0xB3 /* 0x02CC */
#define M64_REGISTER_DP_PIX_WID_OFFSET	            0xB4 /* 0x02D0 */
#define M64_REGISTER_DP_MIX_OFFSET                  0xB5 /* 0x02D4 */
#define M64_REGISTER_DP_SRC_OFFSET                  0xB6 /* 0x02D8 */
#define M64_REGISTER_CLR_CMP_CLR_OFFSET             0xC0 /* 0x0300 */
#define M64_REGISTER_CLR_CMP_MASK_OFFSET            0xC1 /* 0x0304 */
#define M64_REGISTER_CLR_CMP_CNTL_OFFSET            0xC2 /* 0x0308 */
#define M64_REGISTER_FIFO_STAT_OFFSET               0xC4 /* 0x0310 */
#define M64_REGISTER_CONTEXT_MASK_OFFSET            0xC8 /* 0x0320 */
#define M64_REGISTER_CONTEXT_LOAD_CNTL_OFFSET       0xCB /* 0x032C */
#define M64_REGISTER_GUI_TRAJ_CNTL_OFFSET           0xCC /* 0x0330 */
#define M64_REGISTER_GUI_STAT_OFFSET                0xCE /* 0x0338 */

/*
 * bus_cntl register values.
 */
#define BUS_CNTL_BITS								0xFFFF7FFF
#define BUS_CNTL_BUS_WS								0x0000000F
#define BUS_CNTL_BUS_IO_16_EN						0x00000200
#define BUS_CNTL_BUS_DAC_SNOOP_EN					0x00000400
#define BUS_CNTL_BUS_FIFO_WS						0x000F0000
#define BUS_CNTL_BUS_FIFO_ERR_INT_EN				0x00100000
#define BUS_CNTL_BUS_FIFO_ERR_INT					0x00200000
#define BUS_CNTL_BUS_FIFO_HOST_ERR_INT_EN			0x00400000
#define BUS_CNTL_BUS_FIFO_HOST_ERR_INT				0x00800000
#define BUS_CNTL_BUS_PCI_DAC_WS						0x07000000
#define BUS_CNTL_BUS_PCI_DAC_DLY					0x08000000
#define BUS_CNTL_BUS_PCI_MEMW_WS					0x10000000
#define BUS_CNTL_BUS_PCI_BURST_DEC					0x20000000
#define BUS_CNTL_BUS_PCI_RDY_READ_DLY				0xC0000000

/*
 * clock_cntl register values.
 */
#define CLOCK_CNTL_BITS								0x000000FF
#define CLOCK_CNTL_CLOCK_SEL						0x0000000F
#define CLOCK_CNTL_CLOCK_DIV						0x00000030
#define CLOCK_CNTL_CLOCK_STROBE						0x00000040
#define CLOCK_CNTL_CLOCK_SERIAL_DATA				0x00000080

#define CLOCK_CNTL_CLOCK_DIV_BY_1					0x00000000
#define CLOCK_CNTL_CLOCK_DIV_BY_2					0x00000010
#define CLOCK_CNTL_CLOCK_DIV_BY_4					0x00000020

/*
 * Color compare funtions.
 */
#define CLR_CMP_CNTL_CLR_CMP_FN_FALSE 				0x00000000
#define CLR_CMP_CNTL_CLR_CMP_FN_TRUE				0x00000001
#define CLR_CMP_CNTL_CLR_CMP_FN_COMPARE_NOT_EQUAL	0x00000004
#define CLR_CMP_CNTL_CLR_CMP_FN_COMPARE_EQUAL		0x00000005
#define CLR_CMP_CNTL_CLR_CMP_SRC_COMPARE_DEST		0x00000000
#define CLR_CMP_CNTL_CLR_CMP_SRC_COMPARE_SOURCE		0x01000000


/*
 * config_chip_id register.
 */
#define CONFIG_CHIP_ID_CFG_CHIP_TYPE				0x0000FFFF
#define CONFIG_CHIP_ID_CFG_CHIP_CLASS				0x00FF0000
#define CONFIG_CHIP_ID_CFG_CHIP_REV					0xFF000000
#define CONFIG_CHIP_ID_CFG_CHIP_CLASS_SHIFT			16U
#define CONFIG_CHIP_ID_CFG_CHIP_REV_SHIFT			24U

/*
 * config_cntl register values.
 */
#define CONFIG_CNTL_CFG_MEM_AP_SIZE					0x00000003
#define CONFIG_CNTL_CFG_MEM_VGA_AP_EN				0x00000004
#define CONFIG_CNTL_CFG_MEM_AP_LOC					0x00003FF0
#define CONFIG_CNTL_CFG_CARD_ID						0x00070000
#define CONFIG_CNTL_CFG_VGA_DIS						0x00080000

#define CONFIG_CNTL_CFG_MEM_AP_SIZE_DISABLE			0x00000000
#define CONFIG_CNTL_CFG_MEM_AP_SIZE_4MB				0x00000001
#define CONFIG_CNTL_CFG_MEM_AP_SIZE_8MB				0x00000002
#define CONFIG_CNTL_CFG_MEM_AP_LOC_SHIFT			4U

/*
 * config_stat0 register fields.
 */
#define CONFIG_STAT0_CFG_BUS_TYPE					0x00000007
#define CONFIG_STAT0_CFG_MEM_TYPE					0x00000038
#define CONFIG_STAT0_CFG_DUAL_CAS_EN				0x00000040
#define CONFIG_STAT0_CFG_LOCAL_BUS_OPTION			0x00000180
#define CONFIG_STAT0_CFG_INIT_DAC_TYPE				0x00000E00
#define CONFIG_STAT0_CFG_INIT_CARD_ID				0x00007000
#define CONFIG_STAT0_CFG_AP_4GBYTE_DIS				0x80000000

#define CONFIG_STAT0_CFG_MEM_TYPE_DRAM_256Kx4		0x00000000
#define CONFIG_STAT0_CFG_MEM_TYPE_VRAM_256Kx4		0x00000008
#define CONFIG_STAT0_CFG_MEM_TYPE_VRAM_256Kx16S		0x00000010
#define CONFIG_STAT0_CFG_MEM_TYPE_DRAM_256Kx16		0x00000018
#define CONFIG_STAT0_CFG_MEM_TYPE_DRAM_256Kx16G		0x00000020
#define CONFIG_STAT0_CFG_MEM_TYPE_VRAM_256Kx4E		0x00000028
#define CONFIG_STAT0_CFG_MEM_TYPE_VRAM_256Kx16SE	0x00000030

#define CONFIG_STAT0_CFG_BUS_TYPE_ISA				0x00000000
#define CONFIG_STAT0_CFG_BUS_TYPE_EISA				0x00000001
#define CONFIG_STAT0_CFG_BUS_TYPE_VLB				0x00000006
#define CONFIG_STAT0_CFG_BUS_TYPE_PCI				0x00000007
#define CONFIG_STAT0_CFG_INIT_DAC_TYPE_ATI68875		2
#define CONFIG_STAT0_CFG_INIT_DAC_TYPE_BT476		3
#define CONFIG_STAT0_CFG_INIT_DAC_TYPE_BT481		4
#define CONFIG_STAT0_CFG_INIT_DAC_TYPE_ATI68860		5
#define CONFIG_STAT0_CFG_INIT_DAC_TYPE_STG1700		6
#define CONFIG_STAT0_CFG_INIT_DAC_TYPE_SC15021		7
#define CONFIG_STAT0_CFG_INIT_DAC_TYPE_SHIFT		9U
#define CONFIG_STAT0_CFG_INIT_CARD_ID_SHIFT			12U

/*
 * Crtc related registers.
 */
#define CRTC_GEN_CNTL_BITS							0x03FFFFFF
#define CRTC_GEN_CNTL_CRTC_PIX_BY_2_EN				0x00000020
#define CRTC_GEN_CNTL_CRTC_INTERLACE_EN				0x00000002
#define CRTC_GEN_CNTL_CRTC_PIX_WID					0x00000700
#define CRTC_GEN_CNTL_CRTC_PIX_WID_4				(1 << 8U)
#define CRTC_GEN_CNTL_CRTC_PIX_WID_8				(2 << 8U)
#define CRTC_GEN_CNTL_CRTC_PIX_WID_15				(3 << 8U)
#define CRTC_GEN_CNTL_CRTC_PIX_WID_16				(4 << 8U)
#define CRTC_GEN_CNTL_CRTC_PIX_WID_32				(6 << 8U)
#define CRTC_GEN_CNTL_CRTC_BYTE_PIX_ORDER			0x00000800
#define CRTC_GEN_CNTL_CRTC_EXT_DISP_EN				0x01000000
#define CRTC_GEN_CNTL_CRTC_EN						0x02000000
#define CRTC_GEN_CNTL_CRTC_PIX_WID_SHIFT			8U
#define CRTC_GEN_CNTL_CRTC_FIFO_LWM					0x000F0000
#define CRTC_GEN_CNTL_CRTC_FIFO_LWM_SHIFT			16U

#define CRTC_EXT_DISP_CRTC_EXT_DISP_EN				0x01

#define CRTC_H_SYNC_STRT_WID_BITS					0x003F07FF
#define CRTC_H_SYNC_STRT_WID_CRTC_H_SYNC_DLY		0x00000700				
#define CRTC_H_SYNC_STRT_WID_CRTC_H_SYNC_WID		0x001F0000
#define CRTC_H_SYNC_STRT_WID_CRTC_H_SYNC_POL		0x00200000
#define CRTC_H_SYNC_STRT_WID_CRTC_H_SYNC_WID_SHIFT	16U
#define CRTC_H_SYNC_STRT_WID_CRTC_H_SYNC_DLY_SHIFT	8U				

#define CRTC_H_TOTAL_DISP_BITS						0x00FF00FF
#define CRTC_H_TOTAL_DISP_CRTC_H_DISP				0x00FF0000
#define CRTC_H_TOTAL_DISP_CRTC_H_DISP_SHIFT			16U

#define CRTC_INT_CNTL_BITS							0x0000007F
#define CRTC_INT_CNTL_CRTC_VBLANK_INT_EN			0x00000002
#define CRTC_INT_CNTL_CRTC_VLINE_INT_EN				0x00000008

#define CRTC_OFF_PITCH_BITS							0xFFCFFFFF
#define CRTC_OFF_PITCH_CRTC_PITCH					0xFFC00000
#define CRTC_OFF_PITCH_CRTC_PITCH_SHIFT				22U

#define CRTC_V_SYNC_STRT_WID_BITS					0x003F07FF
#define CRTC_V_SYNC_STRT_WID_CRTC_V_SYNC_WID		0x001F0000
#define CRTC_V_SYNC_STRT_WID_CRTC_V_SYNC_POL		0x00200000
#define CRTC_V_SYNC_STRT_WID_CRTC_V_SYNC_WID_SHIFT	16U

#define CRTC_V_TOTAL_DISP_BITS						0x07FF07FF
#define CRTC_V_TOTAL_DISP_CRTC_V_TOTAL				0x000007FF
#define CRTC_V_TOTAL_DISP_CRTC_V_DISP				0x07FF0000
#define CRTC_V_TOTAL_DISP_CRTC_V_DISP_SHIFT			16U

/*
 * Cursor related.
 */
#define CUR_OFFSET_BITS								0x000FFFFF
#define CUR_HORZ_VERT_OFF_BITS						0x003F003F
#define CUR_HORZ_VERT_POSN_BITS						0x07FF07FF
#define CUR_HORZ_VERT_OFF_CUR_VERT_OFFSET_SHIFT		16U
#define CUR_MAX_HORZ_OFFSET_VALUE					63U
#define CUR_MAX_VERT_OFFSET_VALUE					63U

/*
 * dac_cntl register bit definitions.
 */
#define DAC_CNTL_BITS								0x00073F03
#define DAC_CNTL_DAC_EXT_SEL						0x00000003
#define DAC_CNTL_DAC_8BIT_EN						0x00000100
#define DAC_CNTL_DAC_PIX_DLY						0x00000600
#define DAC_CNTL_DAC_PIX_DLY_NO_DLY					0x00000000
#define DAC_CNTL_DAC_PIX_DLY_2TO4_NS				0x00000200
#define DAC_CNTL_DAC_PIX_DLY_4TO8_NS				0x00000400
#define DAC_CNTL_DAC_BLANK_ADJ						0x00001800
#define BLANK_ADJ_0_CLK								0
#define BLANK_ADJ_1_CLK								1
#define BLANK_ADJ_2_CLK								2
#define DAC_CNTL_DAC_BLANK_ADJ_SHIFT				11U
#define DAC_CNTL_DAC_VGA_ADR_EN						0x00002000
#define DAC_CNTL_DAC_TYPE							0x00070000
#define DAC_CNTL_DAC_TYPE_SHIFT						16U

/*
 *  Destination trajectory control register bit definitions.
 */
#define DP_MIX_BITS									0x001F001F
#define DP_MIX_DP_BKGD_MIX							0x0000001F
#define DP_MIX_DP_FRGD_MIX							0x001F0000
#define DP_MIX_DP_FRGD_MIX_SHIFT					16U

#define DP_MIX_GXclear								0x1U
#define DP_MIX_GXand								0xCU
#define DP_MIX_GXandReverse							0xDU
#define DP_MIX_GXcopy								0x7U
#define DP_MIX_GXandInverted						0xEU
#define DP_MIX_GXnoop								0x3U
#define DP_MIX_GXxor								0x5U
#define DP_MIX_GXor									0xBU
#define DP_MIX_GXnor								0xFU
#define DP_MIX_GXequiv								0x6U
#define DP_MIX_GXinvert								0x0U
#define DP_MIX_GXorReverse							0xAU
#define DP_MIX_GXcopyInverted						0x4U
#define DP_MIX_GXorInverted							0x9U
#define DP_MIX_GXnand								0x8U
#define DP_MIX_GXset								0x2U

#define DP_PIX_WID_BITS								0x01070707
#define DP_PIX_WID_DP_SRC_PIX_WID					0x00000700
#define DP_PIX_WID_DP_HOST_PIX_WID					0x00070000
#define DP_PIX_WID_DP_BYTE_PIX_ORDER				0x01000000
#define DP_PIX_WID_DP_BYTE_PIX_ORDER				0x01000000
#define DP_PIX_WID_DP_BYTE_PIX_ORDER_LSB_FIRST		0x01000000
#define DP_PIX_WID_DP_BYTE_PIX_ORDER_MSB_FIRST		0x00000000
#define DP_PIX_WID_DP_SRC_PIX_WID_SHIFT				8U
#define DP_PIX_WID_DP_HOST_PIX_WID_SHIFT			18U

/* 
 * Data path register DP_SRC.
 */
#define DP_SRC_BITS									0x00030707
#define DP_SRC_BKGD_COLOR							0U
#define DP_SRC_FRGD_COLOR							1U
#define DP_SRC_HOST_DATA							2U
#define DP_SRC_BLIT_SRC								3U
#define DP_SRC_PATTERN_REGISTERS					4U
#define DP_SRC_DP_MONO_SRC_ALWAYS_1					0U
#define DP_SRC_DP_MONO_SRC_PATTERN_REGISTERS		1U
#define DP_SRC_DP_MONO_SRC_HOST_DATA				2U
#define DP_SRC_DP_MONO_SRC_BLIT_SRC					3U
#define DP_SRC_DP_FRGD_SRC_SHIFT					8U
#define DP_SRC_DP_MONO_SRC_SHIFT					16U

/*
 * Engine trajectory registers.
 */
#define DST_HEIGHT_BITS								0x00007FFF
#define DST_HEIGHT_WID_BITS							0x1FFF7FFF
#define DST_HEIGHT_WID_DST_WID_SHIFT				16U
#define DST_WID_BITS								0x80001FFF
#define DST_WID_DST_WID_FILL_DIS					0x80000000
#define DST_X_BITS									0x00001FFF
#define DST_X_WID_BITS								0x1FFF1FFF
#define DST_X_WID_DST_WID_SHIFT						16U
#define DST_Y_BITS									0x00007FFF
#define DST_Y_X_BITS								0x1FFF7FFF
#define DST_Y_X_DST_X_SHIFT							16U
#define SRC_HEIGHT1_BITS							0x00007FFF
#define SRC_HEIGHT1_WID1_BITS						0x1FFF7FFF
#define SRC_HEIGHT1_WID1_SRC_WID1_SHIFT				16U
#define SRC_HEIGHT2_BITS							0x00007FFF
#define SRC_HEIGHT2_WID2_BITS						0x1FFF7FFF
#define SRC_HEIGHT2_WID2_SRC_WID2_SHIFT				16U
#define SRC_WID1_BITS								0x00001FFF
#define SRC_WID2_BITS								0x00001FFF
#define SRC_X_BITS									0x00001FFF
#define SRC_X_START_BITS							0x00001FFF
#define SRC_Y_BITS									0x00001FFF
#define SRC_Y_START_BITS							0x00007FFF
#define SRC_Y_X_BITS								0x1FFF7FFF
#define SRC_Y_X_SRC_X_SHIFT							16U
#define SRC_Y_X_START_BITS							0x1FFF7FFF
#define SRC_Y_X_START_SRC_X_START_SHIFT				16U


/*
 * Bit values for the dst_off_pitch register.
 */
#define DST_OFF_PITCH_BITS							0xFFCFFFFF
#define DST_OFF_PITCH_DST_OFFSET					0x000FFFFF
#define DST_OFF_PITCH_DST_PITCH						0xFFCFFFFF
#define DST_OFF_PITCH_DST_PITCH_SHIFT				22U

/*
 * Fifo status bits.
 */

#define FIFO_STAT_FIFO_STAT						0x0000FFFF
#define FIFO_STAT_FIFO_ERR						0x80000000

/*
 * Bit values in the gen_test_cntl register.
 */
#define GEN_TEST_CNTL_BITS						0xFF7F03FF
#define GEN_TEST_CNTL_GEN_CUR_EN				0x00000080
#define GEN_TEST_CNTL_GEN_GUI_EN				0x00000100
#define GEN_TEST_CNTL_GEN_BLOCK_WR_EN			0x00000200

/*
 * Bit values in the gui_stat register.
 */
#define GUI_STAT_BITS							0x00000FFF
#define GUI_STAT_GUI_ACTIVE						0x00000001

/*
 * GUI trajectory control register.
 */
#define GUI_TRAJ_CNTL_BITS						0x1F1F7FFF
#define GUI_TRAJ_CNTL_DST_X_DIR_RIGHT_TO_LEFT	0x00000000
#define GUI_TRAJ_CNTL_DST_X_DIR_LEFT_TO_RIGHT	0x00000001
#define GUI_TRAJ_CNTL_DST_Y_DIR_BOTTOM_TO_TOP	0x00000000
#define GUI_TRAJ_CNTL_DST_Y_DIR_TOP_TO_BOTTOM	0x00000002
#define GUI_TRAJ_CNTL_DST_Y_MAJOR_X_MAJOR_LINE	0x00000000
#define GUI_TRAJ_CNTL_DST_Y_MAJOR_Y_MAJOR_LINE	0x00000004
#define GUI_TRAJ_CNTL_DST_X_TILE				0x00000008
#define GUI_TRAJ_CNTL_DST_Y_TILE				0x00000010
#define GUI_TRAJ_CNTL_DST_LAST_PEL				0x00000020
#define GUI_TRAJ_CNTL_DST_POLYGON_EN			0x00000040
#define GUI_TRAJ_CNTL_DST_24_ROT_EN				0x00000080
#define GUI_TRAJ_CNTL_DST_BRES_SIGN_POSITIVE	0x00000000
#define GUI_TRAJ_CNTL_DST_BRES_SIGN_NEGATIVE	0x00000800
#define GUI_TRAJ_CNTL_SRC_PATT_EN				0x00010000
#define GUI_TRAJ_CNTL_SRC_PATT_ROT_EN			0x00020000
#define GUI_TRAJ_CNTL_SRC_LINEAR_EN				0x00040000
#define GUI_TRAJ_CNTL_SRC_BYTE_ALIGN			0x00080000
#define GUI_TRAJ_CNTL_SRC_LINE_X_DIR			0x00100000
#define GUI_TRAJ_CNTL_PAT_MONO_EN				0x01000000
#define GUI_TRAJ_CNTL_PAT_CLR_4X2_EN			0x02000000
#define GUI_TRAJ_CNTL_PAT_CLR_8X1_EN			0x04000000
#define GUI_TRAJ_CNTL_HOST_BYTE_ALIGN			0x10000000

/*
 * Host data register.
 */
#define HOST_DATA_REGISTER_WIDTH				32U

/*
 * Bit values in the mem_cntl register.
 */
#define MEM_CNTL_BITS							0x000707F7
#define MEM_CNTL_MEM_SIZE						0x00000007
#define MEM_CNTL_MEM_RD_LATCH_EN				0x00000010
#define MEM_CNTL_MEM_RD_LATCH_DLY				0x00000020
#define MEM_CNTL_MEM_SD_LATCH_EN				0x00000040
#define MEM_CNTL_MEM_SD_LATCH_DLY				0x00000080
#define MEM_CNTL_MEM_FULL_PLS					0x00000100
#define MEM_CNTL_MEM_CYC_LNTH					0x00000600
#define MEM_CNTL_MEM_BNDRY						0x00030000
#define MEM_CNTL_MEM_BNDRY_EN					0x00040000

#define MEM_CNTL_MEM_SIZE_512KB					0x00000000
#define MEM_CNTL_MEM_SIZE_1MB					0x00000001
#define MEM_CNTL_MEM_SIZE_2MB					0x00000002
#define MEM_CNTL_MEM_SIZE_4MB					0x00000003
#define MEM_CNTL_MEM_SIZE_6MB					0x00000004
#define MEM_CNTL_MEM_SIZE_8MB					0x00000005
#define MEM_CNTL_MEM_CYC_LNTH_5CL				0x00000000
#define MEM_CNTL_MEM_CYC_LNTH_6CL				0x00000200
#define MEM_CNTL_MEM_CYC_LNTH_7CL				0x00000400

#define SRC_OFF_PITCH_BITS						0xFFCFFFFF
#define SRC_OFF_PITCH_SRC_OFFSET				0x000FFFFF
#define SRC_OFF_PITCH_SRC_PITCH					0xFFC00000
#define SRC_OFF_PITCH_SRC_PITCH_SHIFT			22U

/*
 * definitions regarding scissor registers.
 */
#define SC_BOTTOM_BITS										0x00007FFF
#define SC_LEFT_BITS										0x00001FFF
#define SC_TOP_BITS											0x00007FFF
#define SC_RIGHT_BITS										0x00001FFF
#define SC_LEFT_RIGHT_BITS									0x1FFF1FFF
#define SC_TOP_BOTTOM_BITS									0x7FFF7FFF
#define SC_LEFT_RIGHT_SC_RIGHT_SHIFT						16U
#define SC_TOP_BOTTOM_SC_BOTTOM_SHIFT						16U

#define M64_REGISTER_SET_SIZE_IN_BYTES						1024
#define M64_REGISTER_SET_OFFSET_IN_BYTES_FOR_4MB_APERTURE	0x3FFC00
#define M64_REGISTER_SET_OFFSET_IN_BYTES_FOR_8MB_APERTURE	0x7FFC00

#define M64_SMALL_DUAL_PAGED_APERTURE_BASE_ADDRESS			0xA0000
#define M64_SMALL_DUAL_PAGED_APERTURE_SIZE					0x0FFFF
#define M64_SMALL_DUAL_PAGED_APERTURE_ONE_OFFSET			0
#define M64_SMALL_DUAL_PAGED_APERTURE_TWO_OFFSET			0x08000
#define M64_SMALL_DUAL_PAGED_APERTURE_REGISTERS_OFFSET		0xBFC00

/***
 ***	Types.
 ***/

/*
 * Register state for the m64 chipsets
 * After the generic state we will have two parts to this struct. 
 * One is what the X server knows as the chipsets register state and
 * Second what it should restore before giving up its Virtual Terminal.
 */
struct m64_register_state
{
    struct generic_register_state generic_state;

	/* 
	 * This one register cannot be memory mapped.
	 */
	unsigned int	saved_config_cntl;	
	unsigned int	config_cntl;	

	/*
	 * Both config_cntl and mem_cntl need to be initialized before
	 * accesses to the other mmaped registers can be enabled.
	 */
	unsigned int	saved_mem_cntl;
	unsigned int	mem_cntl;

	/*
	 * Mach64 Registers.
	 */
	unsigned long *saved_registers;
	unsigned long *registers;
};

/***
 *** Macros
 ***/

/*
 * Fifo overflow condition.
 */
#define M64_IS_FIFO_OVERFLOW()\
(register_base_address_p[M64_REGISTER_FIFO_STAT_OFFSET] & FIFO_STAT_FIFO_ERR)


/*
 * Reset and restarting the GUI engine.
 */
#define M64_RESET_GUI_ENGINE() 												\
{																			\
	unsigned long gen_test_cntl = 											\
		register_base_address_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET];			\
	gen_test_cntl &= GEN_TEST_CNTL_BITS;									\
	register_base_address_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] = 			\
		gen_test_cntl & ~ GEN_TEST_CNTL_GEN_GUI_EN;							\
	register_base_address_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] = 			\
		gen_test_cntl |  GEN_TEST_CNTL_GEN_GUI_EN;							\
}

/*
 *	Code to execute inside spin loops.
 */
#define M64_MICRO_DELAY()													\
{																			\
	volatile int _count = m64_graphics_engine_micro_delay_count ;			\
	while (_count--)														\
	{																		\
		;																	\
	}																		\
}


/*
 * Wait N_ENTRIES number of fifo slots to be free. 
 * (page 2-30 of the Prog manual).
 */
#define M64_WAIT_FOR_FIFO(N_ENTRIES)										\
{																			\
	volatile int _count = m64_graphics_engine_loop_timeout_count;			\
	while(((register_base_address_p[M64_REGISTER_FIFO_STAT_OFFSET] &		\
		FIFO_STAT_FIFO_STAT) > ((unsigned int)(0x8000U >> N_ENTRIES)))		\
		&& (--_count > 0))													\
	{																		\
		M64_MICRO_DELAY();													\
	}																		\
	if (_count <= 0)														\
	{																		\
		ASSERT(0);															\
		(void) fprintf(stderr, M64_GUI_ENGINE_RESET_MESSAGE);				\
		M64_RESET_GUI_ENGINE() 												\
	}																		\
}

/*
 * Wait till the GUI engine is idle. (page 2-30 of the Prog manual).
 */
#define M64_WAIT_FOR_GUI_ENGINE_IDLE()										\
{																			\
	volatile int _count = m64_graphics_engine_loop_timeout_count;			\
	M64_WAIT_FOR_FIFO(16);													\
	while((register_base_address_p[M64_REGISTER_GUI_STAT_OFFSET] & 			\
		GUI_STAT_GUI_ACTIVE) && (--_count > 0))								\
	{																		\
		M64_MICRO_DELAY();													\
	}																		\
	if (_count <= 0)														\
	{																		\
		ASSERT(0);															\
		(void) fprintf(stderr,M64_GUI_ENGINE_RESET_MESSAGE);				\
		M64_RESET_GUI_ENGINE() 												\
	}																		\
}

/***
 ***	Variables.
 ***/


/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean m64_registers_debug ;
#endif

/*
 *	Current module state.
 */

extern void
m64_register__initialize__(SIScreenRec *si_screen_p,
							struct m64_options_structure *options_p)
;


#endif
