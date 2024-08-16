/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/tliprcol.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: tliprcol.c,v 1.2 1994/02/18 15:07:00 vtag Exp $"
#include "tlitest.h"
#include "control.h"
#include "tliprcol.h"
#include <string.h>


/*====================================================================
*
*	LoadAddr
*
*		This function loads the address into a TLI structure.
*
*	Return:	void
*
*	Parameters:
*		*buf	(O)	Address buffer to load
*		mode	(I)	CLNT or SRVR
*
*===================================================================*/
void LoadAddr (char *buf, unsigned char mode)
{
	Addr	*addr;
	int i;

		addr = (Addr *) buf;
		switch (ProtoStk)
		{
			case 1:
			case 2:
				switch (mode)
				{
					case SRVR:
						memcpy((char *)&addr->ipx.net,Srvraddr,10);
					case CLNT:
						memcpy(addr->ipx.socket,SOCKET,2);
						break;
					default:
						memset((char *)&addr->ipx.net,0,12);
						break;
				}
	/*
				addr->ipx.socket = SOCKET;
	*/
				break;
			case 3:
			case 4:
				mode &= 0x7f;
				if (mode == CLNT)
					strcpy((char*)addr->nb.name,"Client1");
				else 
					strcpy((char*)addr->nb.name,"Server1");
				addr->nb.name_type = 0;
				addr->nb.name_num = 0;
				break;
			case 5:
			case 6:
				mode &= 0x7f;
				addr->tcp.family = 2;
				addr->tcp.port = 0x0104;		/* 0x0401 = 1025 */
				for (i=0;i<8;i++)
					addr->tcp.reserved[i] = 0;
				if (mode == CLNT)
				{
					addr->tcp.ipaddr[0] = 12;		/* class A node */
					addr->tcp.ipaddr[1] = 1;
					addr->tcp.ipaddr[2] = 0;
					addr->tcp.ipaddr[3] = 9;		/* class A network */
				}
				else
				{
					addr->tcp.ipaddr[0] = 11;
					addr->tcp.ipaddr[1] = 1;
					addr->tcp.ipaddr[2] = 0;
					addr->tcp.ipaddr[3] = 9;
				}
				break;
			default:
				break;
		}
}
