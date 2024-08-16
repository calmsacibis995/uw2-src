/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_TSM_TSMDEF_H	/* wrapper symbol for kernel use */
#define _IO_TSM_TSMDEF_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/odi/tsm/tsmdef.h	1.1"
#ident	"$Header:$"

/*
 *	tsmdef.h, definitions exported by the TSMs for use by other
 *	modules. definitions local to the TSM should be put in the
 *	header file local to the TSM's directory.
 */

#define TSM_T_ETHER		0
#define TSM_T_TOKEN		1
#define TSM_T_FDDI		2
#define TSM_T_RXNET		3
#define TSM_T_PCN2		4
#define TSM_T_ISDN		5
#define TSM_T_ATM		6
#define TSM_T_MAX		7	/* number of different TSM types */

/*
 * these are used for indexing into a specific Frame-Type
 */
#define E8022_INTERNALID	0
#define E8023_INTERNALID	1
#define EII_INTERNALID		2
#define ESNAP_INTERNALID	3

#define TOKEN_INTERNALID	0
#define TOKENSNAP_INTERNALID	1

/*
 * these are the NOVELL assigned Frame IDs
 */
#define	E8023_ID		5
#define	EII_ID			2
#define	E8022_ID		3
#define	ESNAP_ID		10

#define	TOKEN_ID		4
#define	TOKENSNAP_ID		11

#define STATISTICSMASK		0x0D300003
#define ETHERNETSTATSCOUNT	8
#define LONGCOUNTER		0x00
#define LARGECOUNTER		0x01

#endif	/* _IO_TSM_TSMDEF_H */
