/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _MIP_H
#define _MIP_H

#ident	"@(#)stand:i386at/standalone/boot/boothdr/mip.h	1.5"
#ident  "$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

struct machconfig {
	char		*sigaddr;	/* Machine signature location 	*/
	unsigned char	siglen;		/* Signature length 		*/
	unsigned char	sigid[10];	/* Signature to match 		*/
	unsigned char	old_mt;		/* OLD Machine type 		*/
	unsigned char	machine;	/* Machine type 		*/
	unsigned long	m_flag;		/* status flag			*/
	int		(*m_entry)();	/* machine entry point		*/
	int		(*m_ident)();	/* machine identifier entry 	*/
	};

struct	sys_desc {
	short	len;		/* Table length in bytes */
	unchar	sd_model;
	unchar	sd_submodel;
	unchar	sd_BIOSrev;
	unchar	sd_feature1;
	unchar	sd_feature2;
	unchar	sd_feature3;
	unchar	sd_feature4;
	unchar	sd_feature5;
};


/*
 *      Machine definitions for machine field
 */

#define MPC_UNKNOWN	0
#define MPC_AT386	1	/* Generic AT 386 */
#define MPC_COMPAQ	2	/* Compaq */
#define MPC_ATT		4	/* AT&T 6386 WGS */
#define MPC_M380	6	/* Olivetti M380 Series */
#define MPC_DELL	7	/* Dell 386 machines except model 325 */
#define MPC_D325	8	/* Dell 386 model 325 */
#define MPC_ALR		9	/* Advanced Logic Research */
#define MPC_ZDS		10	/* Zenith Data Systems */
#define MPC_TOSHIBA	11	/* Toshoba Personal Computer */
#define MPC_NECpwrmate	12	/* NEC Powermate */
#define MPC_INTEL30X	13	/* Intel 300 series(recent); 301,302,303 */
#define MPC_I386PSA	14	/* Older(at least BIOS wise) 300 series */
#define MPC_COROLLARY	15	/* Corollary */
#define MPC_LSX		16	/* Olivetti LSX50xx mp systems  */
#define MPC_MC386	0x81	/* Generic Micro Channel Machine */
#define MPC_PS2		0x82	/* IBM PS/2 */
#define MPC_APRICOT	0x83	/* Apricot Personal Computer */
#define MPC_OL_Px00	0x84	/* Olivetti Micro-channel P500 & P800 */

/*
 *	Machine model definitions. Us with machine type definitions above.
 */

/* OLIVETTI */

#define OL_M380		0x45	/* M380 including XP1 and XP3 */
#define OL_XP5		0xC5	/* M380 - XP5, tower with display */
#define OL_XP479	0x50	/* M380 - XP4/7/9 */
#define OL_P500		0x61	/* Micro-channel P500 */
#define OL_P800		0x62	/* Micro-channel P800 */

#define M_FLG_SRGE	1	/* sig scattered in a range of memory	*/

#define M_ID_AT386		0
#define M_ID_MC386		1
#define M_ID_COROLLARY  	4
#define M_ID_OLIVETTILSX  	5

#define SYS_MODEL() 	*(char *)0xFFFFE
#define MODEL_AT	(unsigned char)0xFC
#define MODEL_MC	(unsigned char)0xF8

#define SD_MICROCHAN	0x0002

#define MIP_init	(lpcbp->lp_pdata[0])/* machine init entry point	*/
#define MIP_end		(lpcbp->lp_pdata[1])/* machine final startup ent*/

#define EISA_MFG_NAME1		0xc80
#define MFG_NAME1_MASK		0x7f
#define EISA_MFG_NAME2		0xc81
#define EISA_PROD_TYPE		0xc82
#define XM_TYPE			0x2
#define EISA_PROD_REV		0xc83
#define REV(x)			((x)>>3)
#define NO_ROM_REV		0x2

#endif	/* _BOOT_MIP_H */
