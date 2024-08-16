/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/tricord/triccs.h	1.1"
#ident	"$Header: $"


/*
** ident @(#) triccs.h 1.1 1 2/11/94 11:49:06
**
** sccs_id[] = {"@(#) 1.1 triccs.h "}
*/

/*
***************************************************************************
**
**      MODULE NAME:  triccs.h
**
**      PURPOSE: Tricord CCS include file 
**
**      DEPENDENCIES:
**
**          o Tricord Powerframe Model 30/40 & ESxxxx hardware.
**
**      REVISION HISTORY:
**      FPR/CRN     Date    Author      Description
**
**		2/4/93      M. Conner   Initial dev. TLP5/x27 rel.
**     
****************************************************************************
*/
/*
***************************************************************************
**
**      INCLUDE FILE NAME: triccs.h 
**
**      PURPOSE:  Tricord K2 CCS equate file 
**
**      DEPENDENCIES:
**
**          o None.
**
**
**      REVISION HISTORY:
**      FPR/CRN    Date    Author     Description
**
**      Rev 1.6    5/13/92 EDA		Initial Version
** 
**        ----     5/14/92 MWC		Converted for SCO use
**
**      COPYRIGHT:
**
**          (c) Tricord systems, Inc. 1990, 1991, 1992
**               All rights reserved.
**
**
***************************************************************************
*/

/*
 *   --- CCS I/O register map definitions
 */
#define	CCS_ADDR_LOCAL	 	0xcf0	/* Local I/O address */
#define	CCS_DATA_LOCAL	 	0xcf2	/* Local I/O data */
#define	CCS_SLOT		0xcf4	/* Slot Id */
#define	CCS_EISA_ID		0xc80	/* CCS EISA Identifier */
#define	CCS_FCR	 		0xc84	/* feature control register */	
#define	CCS_CFG_REG	 	0xc86	/* Configuration Register */
#define	CCS_HEX_DSP 		0xc87	/* Diagnostic display */
#define	CCS_ATTR_PAGE	 	0xc8a	/* Attribute RAM paging address */	
#define	CCS_ATTR_DATA	 	0xc8d	/* Attribute RAM data register */
#define	CCS_EN_PC_REG	 	0xc8e	/* Sys Bus Parity Chk Enable Reg */
#define	CCS_SENSE		0xc8f	/* Sys Bus Parity Chk Sense Reg */
#define	CCS_SR	 		0xc90	/* Module Status Register */
#define	CCS_PE_SOURCE	 	0xc92	/* Sys Bus Parity Err Source Reg */
#define	CCS_PE_STATUS	 	0xc93	/* Sys Bus Parity Err Byte Src.	*/

/*
 *	--- LOCAL IO EQUATES
 */
#define	CCS_MR1_UART		0x00	/* Mode Register */
#define	CCS_MR2_UART		0x00	/* Mode Register */
#define	CCS_SR_UART		0x04	/* Status Register */
#define	CCS_CLKSR_UART		0x04	/* Clock Select Register */
#define	CCS_CSR_UART		0x08	/* Command Register  */
#define	CCS_RHR_UART		0x0c	/* Receive Holding Register */
#define	CCS_THR_UART		0x0c	/* Transmit Holding Register */
#define	CCS_ACR_UART		0x10	/* Auxiliary Control Register */
#define	CCS_IMR_UART		0x14	/* Interrupt Mask Register */
#define	CCS_CTU_UART		0x18	/* Counter/Timer Upper */
#define	CCS_CTUR_UART		0x18	/* Counter/Timer Upper Register */
#define	CCS_CTL_UART		0x1c	/* Counter/Timer Lower */
#define	CCS_CTLR_UART		0x1c	/* Counter/Timer Lower Register */

/**************************************************************************
 * --- CCS MASKS AND MISC EQUATES
 ************************************************************************** 
*/

/*
 *   --- Feature Control register I/O 0c84h (CCS_FCR)
 */

/*
* D[15:8]  - Reserved
* D[7]     - CPU Reset
* D[6]     - Serial port DTR
* D[5]     - Assert SYS_ATTN
* D[4]     - Reserved
* D[3]     - Assert SYS_IMS_ATTN
* D[2]     - Over-ride Config. RAM
* D[1]     - Over-ride Config. RAM
* D[0]     - Enable Cache and Config. RAM
*/
#define	CCS_CR_RESET_MSK	0x0080	/* CPU Reset */
#define	CCS_CR_DTR_MSK          0x0040	/* Serial port DTR */
#define	CCS_CR_SYS_ATN_MSK	0x0020	/* Assert SYS_ATTN */
#define	CCS_CR_SYS_IMS_ATN_MSK  0x0008	/* Assert SYS_IMS_ATTN */
#define	CCS_CR_CRAM_ORIDE1_MSK  0x0004	/* Over-ride Config. RAM */
#define	CCS_CR_CRAM_ORIDE2_MSK  0x0002	/* Over-ride Config. RAM */
#define	CCS_CR_CACHE_EN_MSK     0x0001	/* Enable Cache and Config. RAM */

#define	CCS_CR_RESET		1	/* CPU Reset */
#define	CCS_CR_SYS_ATN		1	/* Assert SYS_ATTN */
#define	CCS_CR_SYS_IMS_ATN	1  	/* Assert SYS_IMS_ATTN */
#define	CCS_CR_OR_NORMAL	00	/* Normal operation  */
					/* cache not over-ridden, cache */
					/* enabled, write back */
#define	CCS_CR_OR_CDWT		01	/* Cache disabled, Write Through */
#define	CCS_CR_OR_CEWT		10	/* Cache enabled, Write Through */
#define	CCS_CR_OR_CETM		11	/* Cache enabled test mode */
#define	CCS_CR_ENC_ENCRAM	1	/* Enable Cache and Config. RAM */
#define CCS_SLAVE_START_FCR	CCS_CR_OR_NORMAL | CCS_CR_ENC_ENCRAM

/**************************************************************************
 *   --- Configuration register
 **************************************************************************
 */

/*
*	D[7] - cpu-cache module reset one-shot
*	D[6] - master/slave
*	D[5] - Self test enable
*	D[4] - System bus enable
*	D[3] - BIOS H/DIAG L ROM decoding
*	D[2] - ROM enable,
*	D[1] - strong/weak write orders
*	D[0] - cache line size (32 or 64 bytes)
*/

#define	CCS_CF_ONE_SHOT_MSK	0x80	/* cpu-cache module reset one-shot*/  
#define	CCS_CF_MSTR_SLAVE_MSK	0x40	/* master/slave */
#define	CCS_CF_SELF_TEST_MSK	0x20	/* Self test enable */  
#define	CCS_CF_SYS_BUS_MSK     	0x10	/* System bus enable */  
#define	CCS_CF_LROM_DECODE_MSK	0x08	/* BIOS H/DIAG L ROM decoding*/
#define	CCS_CF_LROM_ENABLE_MSK	0x04	/* ROM enable,*/
#define	CCS_CF_STRNG_WEAK_MSK	0x02	/* strong/weak write orders */ 
#define	CCS_CF_LNSIZE_MSK	0x01	/* cache line size (32 or 64 bytes) */
#define	CCS_CF_ONE_SHOT		1	/* cpu-cache module reset one-shot */
#define	CCS_CF_MASTER		1	/* master/slave */
#define	CCS_CF_SLAVE		0	/* master/slave */	
#define	CCS_CF_SELF_TEST_ENABLE	1	/* Self test enable*/
#define	CCS_CF_SYS_BUS_ENABLE	1	/* System bus enable */
#define	CCS_CF_LROM_DECODE_BIOS	1	/* BIOS ROM decoding */
#define	CCS_CF_LROM_DECODE_DIAG	0	/* DIAG ROM decoding */
#define	CCS_CF_LROM_ENABLE	1	/* ROM enable, */
#define	CCS_CF_STRONG_ORDER	1	/* strong write orders*/
#define	CCS_CF_WEAK_ORDER 	0	/* weak write orders*/
#define	CCS_CF_LNSIZE_64	0	/* cache line size 64 bytes*/
#define	CCS_CF_LNSIZE_32	1	/* cache line size 32 bytes */

/*
*	--- HEX DISPLAY
*
*	D[7]   - AMBER LED
*	D[6:0] - GREEN LEDs
*/
#define	CCS_AMBER_LED_MSK 	0x80 		/* CCS AMBER LED Mask */
#define	CCS_GREEN_LED6_MSK	0x40		/* CCS Green LED #6 mask */
#define	CCS_GREEN_LED5_MSK	0x20		/* CCS Green LED #5 mask */
#define	CCS_GREEN_LED4_MSK	0x10		/* CCS Green LED #4 mask */
#define	CCS_GREEN_LED3_MSK	0x08		/* CCS Green LED #3 mask */
#define	CCS_GREEN_LED2_MSK	0x04		/* CCS Green LED #2 mask */
#define	CCS_GREEN_LED1_MSK	0x02		/* CCS Green LED #1 mask */
#define	CCS_GREEN_LED0_MSK	0x01		/* CCS Green LED #0 mask */


/*
*	--- Attribute RAM data register (rw, byte  )
*
*	D[7:5] - reserved
*	D[4]   - 64/32 Bit Slave
*	D[3]	 - Main memory H/EISA L
*	D[2]	 - Write Back H/Write Though L
*	D[1]	 - Memory Read only L
*	D[0]	 - Memory cachability L
*
*/

#define	CCS_BUS_SIZE_MSK	0x10	/* Bus Size 64Bits H/32Bits L */
#define	CCS_MEM_EISA_MSK	0x08	/* Main memory H/EISA L */
#define	CCS_WB_WT_MSK		0x04	/* Write Back H/Write Though L */
#define	CCS_MEM_RO_MSK		0x02	/* Memory Read only L*/
#define	CCS_SELECT_64BIT	1	/* Select 64 Bit Wide Bus */
#define	CCS_SELECT_32BIT	0	/* Select 32 Bit Wide Bus*/
#define	CCS_SELECT_MEMORY	1	/* Select Main memory */
#define	CCS_SELECT_EISA		0	/* Select EISA memory*/
#define	CCS_SELECT_WB		1	/* Write Back */
#define	CCS_SELECT_WT		0	/* Write Though */
#define	CCS_SELECT_MEM_RO	0	/* Memory Read only */
#define	CCS_SELECT_MEM_RW 	1	/* Memory Read/Write */
#define	CCS_SELECT_MEM_CACHE	0	/* Memory cachable */
#define	CCS_SELECT_MEM_NOT_CACH	1	/* Memory NOT cachable */
#define	CCS_ATTR_RAM_SZ		0xffff	/* size of CCS attribute RAM */
 
/*
*	--- Sys Bus Parity Checker Enable Reg. (rw, byte  )
*
*	
*	D[7] - Enable Parity Check for byte 7
*	D[6] - Enable Parity Check for byte 6
*	D[5] - Enable Parity Check for byte 5
*	D[4] - Enable Parity Check for byte 4
*	D[3] - Enable Parity Check for byte 3
*	D[2] - Enable Parity Check for byte 2
*	D[1] - Enable Parity Check for byte 1
*	D[0] - Enable Parity Check for byte 0
*/
#define	CCS_BEN7_MSK		0x80	/* Enable Parity Check for byte 7*/
#define	CCS_BEN6_MSK		0x40	/* Enable Parity Check for byte 6*/
#define	CCS_BEN5_MSK		0x20	/* Enable Parity Check for byte 5*/
#define	CCS_BEN4_MSK		0x10	/* Enable Parity Check for byte 4*/
#define	CCS_BEN3_MSK		0x08	/* Enable Parity Check for byte 3*/
#define	CCS_BEN2_MSK		0x04	/* Enable Parity Check for byte 2*/
#define	CCS_BEN1_MSK		0x02	/* Enable Parity Check for byte 1*/
#define	CCS_BEN0_MSK		0x01	/* Enable Parity Check for byte 0*/
#define	CCS_BEN_ALL_MSK		0xff	/* Enable Parity Check for all bytes*/
#define	CCS_EN_PC		0	/* Enable Parity Checker */

/*
*	--- Sys Bus Parity Checker Sense Reg. (rw, byte  )
*
*	D[7] - Parity Sense for byte 7
*	D[6] - Parity Sense for byte 6
*	D[5] - Parity Sense for byte 5
*	D[4] - Parity Sense for byte 4
*	D[3] - Parity Sense for byte 3
*	D[2] - Parity Sense for byte 2
*	D[1] - Parity Sense for byte 1
*	D[0] - Parity Sense for byte 0
*/

#define	CCS_SEN7_MSK		0x80	/* Parity Sense for byte 7 */
#define	CCS_SEN6_MSK		0x40	/* Parity Sense for byte 6 */
#define	CCS_SEN5_MSK		0x20	/* Parity Sense for byte 5 */
#define	CCS_SEN4_MSK		0x10	/* Parity Sense for byte 4 */
#define	CCS_SEN3_MSK		0x08	/* Parity Sense for byte 3 */
#define	CCS_SEN2_MSK		0x04	/* Parity Sense for byte 2 */
#define	CCS_SEN1_MSK		0x02	/* Parity Sense for byte 1*/
#define	CCS_SEN0_MSK		0x01	/* Parity Sense for byte 0 */
#define	CCS_SEN_ALL_MSK		0x0ff	/* Parity Sense for all bytes */
#define	CCS_SENSE_EVEN		0	/* Generate even parity (default) */
#define	CCS_SENSE_ODD		1	/* Generate odd parity */

/*
*	--- Module Status Register	   (ro, word  )
*
*
*	D[15] 	- Cache self test pass/fail
*	D[14] 	- System bus address width (40 or 32 bits)
*	D[13:12] - cacheline size (256, 128, 64 or 32 bytes)
*	D[11:8]  - module diagnostic switches
*	D[7] 		- Master/Slave
*	D[6] 		- Is it me?
*	D[5] 		- IMS present
*	D[4] 		- Flush/Sync/Init status
*	D[3] 		- Parity Error
*	D[2] 		- Module fault LED (formerly reserved)
*	D[1] 		- cpu shutdown cycle occurred (formerly reserved)
*	D[0] 		- module detected error Interrupt pending to IMS 
*/
#define	CCS_SR_CACHE_ST_MSK	0x08000	/* Cache self test pass/fail */
#define	CCS_SR_BUS_WIDTH_MSK	0x04000	/* System bus address width */
					/* (40 or 32 bits) */
#define	CCS_SR_LNSIZE_MSK	0x03000	/* cacheline size */
					/* (256, 128, 64 or 32 bytes) */
#define	CCS_SR_SWITCH_MSK	0x00f00	/* module diagnostic switches */
#define	CCS_SR_MSTR_SLV_MSK	0x00080	/* Master/Slave */
#define	CCS_SR_IS_IT_ME_MSK	0x00040	/* Is it me? */	
#define	CCS_SR_IMS_PRESENT_MSK	0x00020	/* IMS present */
#define	CCS_SR_FSIS_MSK		0x00010	/* Flush/Sync/Init status */
#define	CCS_SR_PE_MSK		0x00008	/* Parity Error */
#define	CCS_SR_RED_LED_MSK 	0x00004	/* Module fault LED */
#define	CCS_SR_SHUTDOWN_MSK	0x00002	/* cpu shutdown cycle occurred */
#define	CCS_SR_IMSATTN_MSK  	0x00001	/* error Interrupt pending to IMS */
#define	CCS_SR_CACHE_ST_PASS    1	/* Cache self test pass */
#define	CCS_SR_CACHE_ST_FAIL    0	/* Cache self test pass */
#define	CCS_SR_BUS_WIDTH_40	1	/* System bus address width 40 bits */
#define	CCS_SR_BUS_WIDTH_32	0	/* System bus address width 32 bits */
#define	CCS_SR_LNSIZE_32      	0	/* cacheline size 32  bytes */
#define	CCS_SR_LNSIZE_64      	1	/* cacheline size 64  bytes */
#define	CCS_SR_LNSIZE_128     	2	/* cacheline size 128 bytes */
#define	CCS_SR_LNSIZE_256     	3	/* cacheline size 256 bytes */
#define	CCS_SR_MASTER		1	/* master/slave */
#define	CCS_SR_SLAVE		0	/* master/slave	*/
#define	CCS_SR_IT_IS_ME		1	/* Yes its is! */
#define	CCS_SR_IMS_PRESENT	1	/* IMS is installed	*/
#define	CCS_SR_SYNC_FLUSH	1	/* Flush or Sync in progress */
#define	CCS_SR_RED_LED_ON     	1	/* Module fault LED */
#define	CCS_SR_SHUTDOWN		1	/* cpu shutdown cycle occurred */
#define	CCS_SR_IMSATTN		1	/* error Interrupt pending to IMS */

/*
*	--- Sys Bus Parity Error Source Reg.(ro, byte  )
*
*
*	D[7:4]	- 	Slave id on parity error
*
*	D[3]   	- 	parity error on slave write (formerly reserved) 
*		   	This bit when a one indicates that the module has 
*			detected a parity error on data written to the module
*
*	D[2]   	- 	parity error on cpu read (formerly reserved) This 
*		   	bit when a one indicates that the module has detected
*			a parity error on a cpu read.
*
*	D[1]   	- 	local cache read parity error  (formerly bit 2= 
*		   	local parity error) This bit when a one indicates 
*			that a local cache parity error on a cache read has 
*   			been detected
*
*	D[0]   	- 	local cache copy back parity error   This bit when 
*		   	a one indicates that a local cache parity error on 
*			a copy back has been detected
*/

#define	CCS_PE_SLV_ID_MSK	0xf0	/* Slave id on parity error */
#define	CCS_PE_SLV_WRT_MSK	0x08	/* parity error on slave write */
#define	CCS_PE_CPU_READ_MSK	0x04	/* parity error on cpu read */
#define	CCS_PE_LOC_CREAD_MSK	0x02	/* local cache read parity error */
#define	CCS_PE_LOC_CPY_BACK_MSK	0x01	/* local cache copy back parity error */
#define	CCS_PE_SLV_WRT		1	/* parity error on slave write */
#define	CCS_PE_CPU_READ		1	/* parity error on cpu read */
#define	CCS_PE_LOC_CREAD	1	/* local cache read parity error */
#define	CCS_PE_LOC_CPY_BACK	1	/* local cache copy back parity error */

/*
*	--- Parity Error Byte source Reg. (ro, byte 0zc93)
*
*
*	Bits [7] - 	Parity byte 7 error.
*	Bits [6] - 	Parity byte 6 error. 	
*	Bits [5] - 	Parity byte 5 error.
*	Bits [4] - 	Parity byte 4 error.
*	Bits [3] - 	Parity byte 3 error.
*	Bits [2] - 	Parity byte 2 error.
*	Bits [1] - 	Parity byte 1 error.
*	Bits [0] - 	Parity byte 0 error.
*/

#define	CCS_PE_BYTE7_MSK	0x80	/* Parity byte 7 error. */
#define	CCS_PE_BYTE6_MSK	0x40	/* Parity byte 6 error. */
#define	CCS_PE_BYTE5_MSK	0x20	/* Parity byte 5 error. */
#define	CCS_PE_BYTE4_MSK	0x10	/* Parity byte 4 error. */
#define	CCS_PE_BYTE3_MSK	0x08	/* Parity byte 3 error. */
#define	CCS_PE_BYTE2_MSK	0x04	/* Parity byte 2 error. */
#define	CCS_PE_BYTE1_MSK	0x02	/* Parity byte 1 error. */
#define	CCS_PE_BYTE0_MSK	0x01	/* Parity byte 0 error. */

/*
*	---	CCS HEX Display POST Codes
*/

#define	CCS_PC_NULL		0x00	/*Null (contents of display at reset) */
#define	CCS_PC_READY		0x70	/* Module "Ready" (used by hs proto */
#define	CCS_PC_LAMP_TEST	0x7f	/* Lamp Test Post code */
#define	CCS_PC_HEART_BEAT	0x40	/* CCS Module Heart Beat Code Mask */

/*
*	---	CCS HEX Display ERROR Codes
*/

