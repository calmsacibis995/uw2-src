/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/ether/nflxe.cf/Space.c	1.8"
#ident	"$Header: $"
#ifndef	ETHER
#define ETHER
#endif
#ifndef	UW1_1
#define	ESMP
#endif

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/cet.h>
#include <sys/dlpi_ether.h>
#include <config.h>


#define	NSAPS		8
#define MAXMULTI	8
#define INETSTATS	1
#define STREAMS_LOG	0
#define IFNAME		"nflxe"

int nflxestrlog = STREAMS_LOG;
char *nflxe_ifname = IFNAME;

#ifdef	ESMP
/*
 * The system can support upto 4 network connections.
 * Allocate maximum since since DLPI level externs nflxeconfig as an array,
 * therefore precluding autoconfiguration from dynamically allocating
 * nflxeconfig.
 */
DL_bdconfig_t nflxeconfig[CET_MAX_UNITS]; 
SLOT_UNIT nflxe_slots[CET_MAX_UNITS];

major_t nflxe_majors[NFLXE_CMAJORS] = {
    {
	NFLXE_CMAJOR_0
     },
    {
	NFLXE_CMAJOR_1
    },
    {
	NFLXE_CMAJOR_2
    },
    {
	NFLXE_CMAJOR_3
    },
};

/*
 * Used to map Board ID to Board name.
 */
struct cetboard_idtoname nflxe_cetboard_idtoname[] = {
	{
	"CPQ6100",			/* Uncompressed Board ID */
	"NetFlex Ethernet",		/* Board name for given Board ID*/
	"NetFlex TokenRing",		/* Other Board name for given Board ID*/
	1				/* Number of physical connections */
	},
	{
	"CPQ6101",
	"NetFlex-II Ethernet",
	"NetFlex-II TokenRing",
	1
	},
	{
	"CPQ6200",
	"NetFlex Dual-Ethernet",
	(char *)0,
	2
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
struct cetnet_type_media nflxe_cetnet_type_media[] = {
	{
	"10MBPS;AUI",		/* String used to match board's NVM entry */
	NET_IEEE_8023,		/* Board's network type */
	DB_CONNECTOR		/* Board's network media */
	},
	{
	"10MBPS;UTP",
	NET_IEEE_8023,
	TP_CONNECTOR
	},
	{
	(char *)0,		/* End of Table */
	0,
	0
	}
};
#else

#define	BOARD_COUNT	1

int nflxe_board_count = BOARD_COUNT;
ushort_t nflxe_units = NFLXE_CNTLS;
int nflxeboards = NFLXE_CNTLS;

DL_bdconfig_t nflxeconfig[ NFLXE_CNTLS ] = {
#ifdef	NFLXE_0
    {
	NFLXE_CMAJOR_0,
	NFLXE_0_SIOA,
	NFLXE_0_EIOA,
	NFLXE_0_SCMA,
	NFLXE_0_ECMA,
	NFLXE_0_VECT,
	NSAPS,
	0
     },
#endif
#ifdef	NFLXE_1
    {
	NFLXE_CMAJOR_1,
	NFLXE_1_SIOA,
	NFLXE_1_EIOA,
	NFLXE_1_SCMA,
	NFLXE_1_ECMA,
	NFLXE_1_VECT,
	NSAPS,
	0
    },
#endif
#ifdef	NFLXE_2
    {
	NFLXE_CMAJOR_2,
	NFLXE_2_SIOA,
	NFLXE_2_EIOA,
	NFLXE_2_SCMA,
	NFLXE_2_ECMA,
	NFLXE_2_VECT,
	NSAPS,
	0
    },
#endif
#ifdef	NFLXE_3
    {
	NFLXE_CMAJOR_3,
	NFLXE_3_SIOA,
	NFLXE_3_EIOA,
	NFLXE_3_SCMA,
	NFLXE_3_ECMA,
	NFLXE_3_VECT,
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
} nflxe_slots[NFLXE_CNTLS];
#endif	/* ESMP */
