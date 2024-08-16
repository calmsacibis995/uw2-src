/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/tliprcol.h	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW4U_SPX_TEST_TLI_TLIPRCOL_H  /* wrapper symbol for kernel use */
#define _NET_NW4U_SPX_TEST_TLI_TLIPRCOL_H  /* subject to change without notice */

#ident	"$Id: tliprcol.h,v 1.2 1994/02/18 15:07:02 vtag Exp $"
/*--------------------------------------------------------------------
*	Defines
*-------------------------------------------------------------------*/
#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifndef LONG
#define LONG unsigned long
#endif

#ifndef NB_NAME_SIZE
#define NB_NAME_SIZE 16
#endif

/*--------------------------------------------------------------------
*	Structure Types
*-------------------------------------------------------------------*/
typedef struct IpxAddress
{
	LONG net;
	BYTE node[6];
	BYTE socket[2];
} IpxAddr;


/* 0 0 0 0 0 64 1 2 15 03 0 0 0 0 0 1 0 0 0 0 2 0 0 0 */

typedef struct NBAddress
{
	unsigned char	name[NB_NAME_SIZE];	/* Actual NetBIOS name */
	unsigned int	name_type;				/* Name type: NB_UNIQUE/NB_GROUP */
	unsigned int	name_num;				/* internal name number */
} NBAddr;

typedef struct TCPAddress
{
	WORD family; 			/* intel format - always 2*/
	WORD port;				/* motorola format 1st 1k reserved */
	BYTE ipaddr[4];		/* motorola format */
	BYTE reserved[8]; 	/* zero'ed */
}TCPAddr;

typedef union Address
{
	IpxAddr	ipx;
	NBAddr	nb;
	TCPAddr	tcp;
}Addr;


/*--------------------------------------------------------------------
*	Function Prototypes
*-------------------------------------------------------------------*/
void LoadAddr (char *buf, unsigned char mode);

#endif /* _NET_NW4U_SPX_TEST_TLI_TLIPRCOL_H */
