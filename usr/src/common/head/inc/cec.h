/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/cec.h	1.1"
#include <npackon.h>

/* Data structure for CEC (Compressed Encoding) computation */

struct CECCTX	/* context buffer for CEC computation */
{
	BYTE 	buf[16];   /* buffer for forming cec into */
	BYTE 	mac[8];	   /* Mac on CEC data supplied */
	int		len;       /* # OF CHARS FED TO CEC ROUTINES, mod 8 */
};


/* Data structure for MD (Message Digest) computation */

struct MDCTX	/* context buffer for MD computation */
{
	BYTE	buf[48];   /* buffer for forming md into */
				   /* actual digest is buf[0]...buf[15] */
	BYTE	mac[16];   /* mac register */
	int		i;	   /* number of bytes handled, modulo 16 */
	BYTE   lmac;	   /* last mac value saved */
};


/* Union data structure */

union CECMDCTX /* context buffer for either computation */
{
	struct CECCTX cec;
	struct MDCTX  md;
};

#include <npackoff.h>
