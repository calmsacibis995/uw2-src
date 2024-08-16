/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/token/nflxt.cf/Space.c	1.6"
#ident	"$Header: $"
#ifndef	TOKEN
#define TOKEN
#endif
#ifndef	UW1_1
#define	ESMP
#endif

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/cet.h>
#include <sys/dlpi_token.h>
#include <config.h>


#define	NSAPS		8
#define MAXMULTI	8
#define INETSTATS	1
#define STREAMS_LOG	0
#define IFNAME		"nflxt"

int nflxtstrlog = STREAMS_LOG;
char *nflxt_ifname = IFNAME;

#ifdef	ESMP
/*
 * The system can support upto 4 network connections.
 * Allocate maximum since since DLPI level externs nflxtconfig as an array,
 * therefore precluding autoconfiguration from dynamically allocating
 * nflxtconfig.
 */

DL_bdconfig_t nflxtconfig[CET_MAX_UNITS]; 
SLOT_UNIT nflxt_slots[CET_MAX_UNITS];

major_t nflxt_majors[NFLXT_CMAJORS] = {
    {
	NFLXT_CMAJOR_0
     },
    {
	NFLXT_CMAJOR_1
    },
    {
	NFLXT_CMAJOR_2
    },
    {
	NFLXT_CMAJOR_3
    },
};

/*
 * Used to map Board ID to Board name.
 */
struct cetboard_idtoname nflxt_cetboard_idtoname[] = {
	{
	"CPQ6100",			/* Uncompressed Board ID */
	"NetFlex TokenRing",		/* Board name for given Board ID*/
	"NetFlex Ethernet",		/* Other Board name for given Board ID*/
	1				/* Number of physical connections */
	},
	{
	"CPQ6101",
	"NetFlex-II TokenRing",
	"NetFlex-II Ethernet",
	1
	},
	{
	"CPQ6300",
	"NetFlex Dual-TokenRing",
	(char *)0,
	2
	},
	{
	"CPQ6002",
	"NetFlex-II TokenRing",
	(char *)0,
	1
	},
	{
	(char *)0,			/* End of Table */
	(char *)0,
	(char *)0,
	0
	}
};

/*
 * Use to obtain the board's network type, speed, and media.
 */
struct cetnet_type_media nflxt_cetnet_type_media[] = {
	{
	"16MBPS;STP",		/* String used to match board's NVM entry */
	NET_TPR_16Mbps,		/* Board's network type */
	DB_CONNECTOR		/* Board's network media */
	},
	{
	"16MBPS;UTP",
	NET_TPR_16Mbps,
	TP_CONNECTOR
	},
	{
	"4MBPS;STP",
	NET_TPR_4Mbps,
	DB_CONNECTOR|TOKEN_RING_4Mbps,
	},
	{
	"4MBPS;UTP",
	NET_TPR_4Mbps,
	TP_CONNECTOR|TOKEN_RING_4Mbps,
	},
	/*
	 * Reversing the String Entries was necessary since the CPQ6300
	 * has the String reversed in the NVM
	 */
	{
	"STP;16MBPS",		/* String used to match board's NVM entry */
	NET_TPR_16Mbps,		/* Board's network type */
	DB_CONNECTOR		/* Board's network media */
	},
	{
	"UTP;16MBPS",
	NET_TPR_16Mbps,
	TP_CONNECTOR
	},
	{
	"STP;4MBPS",
	NET_TPR_4Mbps,
	DB_CONNECTOR|TOKEN_RING_4Mbps,
	},
	{
	"UTP;4MBPS",
	NET_TPR_4Mbps,
	TP_CONNECTOR|TOKEN_RING_4Mbps,
	},
	{
	(char *)0,		/* End of Table */
	0,
	0
	}
};
#else

#define	BOARD_COUNT	1

int nflxt_board_count = BOARD_COUNT;
ushort_t nflxt_units = NFLXT_CNTLS;
int nflxtboards = NFLXT_CNTLS;

DL_bdconfig_t nflxtconfig[ NFLXT_CNTLS ] = {
#ifdef	NFLXT_0
    {
	NFLXT_CMAJOR_0,
	NFLXT_0_SIOA,
	NFLXT_0_EIOA,
	NFLXT_0_SCMA,
	NFLXT_0_ECMA,
	NFLXT_0_VECT,
	NSAPS,
	0
     },
#endif
#ifdef	NFLXT_1
    {
	NFLXT_CMAJOR_1,
	NFLXT_1_SIOA,
	NFLXT_1_EIOA,
	NFLXT_1_SCMA,
	NFLXT_1_ECMA,
	NFLXT_1_VECT,
	NSAPS,
	0
    },
#endif
#ifdef	NFLXT_2
    {
	NFLXT_CMAJOR_2,
	NFLXT_2_SIOA,
	NFLXT_2_EIOA,
	NFLXT_2_SCMA,
	NFLXT_2_ECMA,
	NFLXT_2_VECT,
	NSAPS,
	0
    },
#endif
#ifdef	NFLXT_3
    {
	NFLXT_CMAJOR_3,
	NFLXT_3_SIOA,
	NFLXT_3_EIOA,
	NFLXT_3_SCMA,
	NFLXT_3_ECMA,
	NFLXT_3_VECT,
	NSAPS,
	0
    },
#endif
};

struct slot_unit {
	short	slot;
	short	interface;
	short	unit;
	ushort	net_type;	
	ushort	net_media;	
	char	id[4];
	uchar_t	prod_id[18];
	char	id_string[32];
} nflxt_slots[NFLXT_CNTLS];
#endif	/* ESMP */
