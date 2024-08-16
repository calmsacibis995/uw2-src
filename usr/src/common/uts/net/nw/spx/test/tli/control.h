/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/control.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW4U_SPX_TEST_TLI_CONTROL_H  /* wrapper symbol for kernel use */
#define _NET_NW4U_SPX_TEST_TLI_CONTROL_H  /* subject to change without notice */

#ident	"$Id: control.h,v 1.2 1994/02/18 15:06:06 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		control.h
*	Date Created:	04/17/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Support for control.c
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/
#ifdef MAIN
#define EXT
#else
#define EXT extern
#endif

#include <sys/types.h>
#include <net/nw/nwtdr.h>

/*--------------------------------------------------------------------
*	Constants
*-------------------------------------------------------------------*/
#define ADDRACK		GETINT16(0x11)
#define ADDRDONE	GETINT16(0x22)
#define ADDRREQ		GETINT16(0x33)
#define ADDRTIMEOUT	30000
#define DELAYTIME	250
#define DYN		0x80
/*#define CLK_TCK		18.1			/* TEB */

#define CLNT		'C'
#define CLNTCTRLSKT	"\x4A\x4A"
#define SRVR		'S'
#define SRVRCTRLSKT	"\x4B\x4B"
#define SPXCTRLSKT	"\x8E\x8E"			/* used for spx sync */

#define NET1		"\x01\x01\x03\x8B"
#define NET2		"\x01\x01\x03\x93"
#define NODE		"\xFF\xFF\xFF\xFF\xFF\xFF"

#define RESET		0x44
#define SYNCACK	 	GETINT16(0x55)
#define SYNCDONE	GETINT16(0x66)
#define SYNCNACK	GETINT16(0x77)
#define SYNCREQ		GETINT16(0x88)
#define SYNCTIMEOUT	30000

#define MAXOPT      4
#define MAXBUF      80
EXT char    opt[MAXOPT];
EXT char    buf[MAXBUF];
EXT char    tmpbuf[MAXBUF];

/*--------------------------------------------------------------------
*	Type Definitions
*-------------------------------------------------------------------*/
typedef struct syncdata
{
	unsigned short msg;
	unsigned short major;
	unsigned short minor;
} sydata;


#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------
*	Function Prototypes
*-------------------------------------------------------------------*/
void	broadcast (char *network, char *socket, void *data, int size);
void	ClntSync (uint8 *srvraddr, short int major, short int minor);
int	Delay (void);
void	Delaytime (unsigned msecs);
void	EndCtrl (void);
void	FindSrvr (uint8 *srvraddr);
void	InitCtrl (char mode);
void	ScreamAndDie (char *msg);
void	SrvrAdd (int clients);
void	SrvrSync (int clients, short int major, short int minor);
int	rcvudata (tudataptr ud, int *flags);
void	sndudata (tudataptr ud);
void	TScreamAndDie (char *msg);

#ifdef __cplusplus
}
#endif

#endif /* _NET_NW4U_SPX_TEST_TLI_CONTROL_H */
