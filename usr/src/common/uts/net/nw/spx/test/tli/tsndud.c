/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/tsndud.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: tsndud.c,v 1.2 1994/02/18 15:07:15 vtag Exp $"
/****************************************************************************
*	Program Name:		TLI/OS2 API
*	File Name:			tsndud.c
*	Date Created:		01/04/89
*	Version:				1.0 (SPX)
*	Programmer(s):		Mentat Inc., J. Richey, S. McBride, P.Krishnaswami
*	Files Used:			
*	Date Modified:
*	Purpose:
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1989 by Mentat Inc.
*	COPYRIGHT (c) 1989-1990 by Novell Inc.  All Rights Reserved.
***************************************************************************/
/****************************************************************************
*                                                                           *
* (C) Unpublished Copyright of Novell, Inc. All Rights Reserved             *
*                                                                           *
*  No part of this file may be duplicayed, revised, translated, localized   *
*  or modified in any manner or compiled, linked or uploaded or downloaded  *
*  to or from any computer system without the prior written consent of      *
*  Novell, Inc.                                                             *
*                                                                           *
*****************************************************************************/

#include <common.h>
#include <ts.h>
#include <tispxipx.h>
#include <errno.h>
#include <stdlib.h>
#include <tiuser.h>
#include <time.h>

#ifdef OS2
#define SNDUDATA spx_sndudata
#include <memory.h>
#include <tli_spx.h>
#else
#define SNDUDATA t_sndudata
#define API
#define REG register
#endif	/* OS2 */

#ifndef CLOCKTICK

#define CLOCKTICK    	 	18
#define RIPREQUEST			60
#define FREQUENCY_OF_RIP CLOCKTICK * RIPREQUEST

#endif



/* dest -- just pass four of the twelve byte network address */


static TS_ECB		te;
/****  t_sndudata  **********************************************************
*
*		Send an IPX packet
*
*	Parameters:
*		int	fd					(I)	file descriptor
*		struct t_unitdata		(I)	data to be sent and info
*
:  0,  success
*			  -1,  failure
*
*	Comments:	See UNIX programmer's guide t_sndudata(3N)
*
*****/
int API
SNDUDATA(fd, unitdata)
	int	fd;
REG	struct t_unitdata * unitdata;
{
	int			result;
	IPX_HEADER	ipx;
REG	TSP	ts;


/*-----------------------------------------------------------------------
 *   VALID FD ?
 *----------------------------------------------------------------------*/
VideoPut(90);

	ts = FD_TO_TS(fd);
	if (!ts){
		return TLI_ERR(TBADF);
	}
/*-----------------------------------------------------------------------
 *   IPX ?
 *----------------------------------------------------------------------*/
	if (ts->ts_type != TS_IPX)
		return TLI_ERR(TNOTSUPPORT);
/*-----------------------------------------------------------------------
 *   CORRECT NETWORK ADDRESS SIZE ?
 *----------------------------------------------------------------------*/
/*	if (unitdata->addr.len != 12)
		return TLI_ERR(TBADADDR);   There is no provision to return this
			                           error code -04/23/91 
*/

/*-----------------------------------------------------------------------
 *   PACKET HAS ZERO BYTES ?
 *----------------------------------------------------------------------*/
#ifdef XTI
	if (unitdata->udata.len == 0)
		return TLI_ERR(TBADDATA);  /* added on 04/23/91 */

	if(ts->ts_state != T_IDLE)
		return TLI_ERR(TOUTSTATE);	  /* added on 04/23/91 */
#else
	if(unitdata->udata.len = 0)
		return 0;
#endif

VideoPut(89);


 if(unitdata->udata.len > IPX_MAX_PACKET_SIZE)
    		return TLI_SYSERR(EPROTO);

/*-----------------------------------------------------------------------
 *   SET UP NETWORK ADDRESS
 *----------------------------------------------------------------------*/
#ifdef DOS
	te.te_ecb.socketNumber = TS_SOCKET(ts);
#endif
	TBASSIGN(&DEST(ipx), unitdata->addr.buf);
/*-----------------------------------------------------------------------
 *   ASSIGN THE PACKET TYPE (IF THAT OPTION BEING USED)
 *----------------------------------------------------------------------*/
	if (unitdata->opt.len >= sizeof(IPX_OPTS))
		ipx.packetType = ((IPX_OPTS *)unitdata->opt.buf)->ipx_type;
	else
		ipx.packetType = 0;
/*-----------------------------------------------------------------------
 *   DETERMINE THE TARGET OR BRIDGE NETWORK ADDRESS
 *   SEND THE PACKET
 *----------------------------------------------------------------------*/
VideoPut(90);


	FRAG_COUNT(te) = 2;
	SPX_HDR_ADDR(te.te_ecb) = (void far *) &ipx;
	SPX_HDR_SIZE(te.te_ecb) = IPX_HDR_LEN;
	SPX_DATA_ADDR(te.te_ecb) = (void far *) unitdata->udata.buf;
	SPX_DATA_SIZE(te.te_ecb) = unitdata->udata.len;
#ifdef OS2
/*	result = IpxGetLocalTarget(unitdata->addr.buf, (IPX_ECB far*)&te.te_ecb, &l1); */


	result = GetImmediateaddress(&ts->time,unitdata->addr.buf,ts->networkAddress, ts->nodeAddress); 
	if (result == 1) {

		result = IpxSend(TS_SOCKET(ts),(IPX_ECB far *)&te.te_ecb);
#else
/*	te.te_ecb.ESRAddress = ts_esr_wfz_done;*/
	te.te_ecb.ESRAddress = NULL;

VideoPut(89);


	result = GetImmediateaddress(&ts->time,unitdata->addr.buf,ts->networkAddress,
		                                            ts->nodeAddress);
VideoPut(88);



/*	result = IPXGetLocalTarget(unitdata->addr.buf, te.te_ecb.immediateAddress, &i1); */

#if 0
    printf("NetworkAddress: %02x%02x%02x%02x/%02x%02x%02x%02x%02x%02x\n",
		unitdata->addr.buf[0] & 0xFF, unitdata->addr.buf[1] & 0xFF,
		unitdata->addr.buf[2] & 0xFF,unitdata->addr.buf[3] & 0xFF, unitdata->addr.buf[4] & 0xFF,
		unitdata->addr.buf[5] & 0xFF, unitdata->addr.buf[6] & 0xFF,
		unitdata->addr.buf[7] & 0xFF, unitdata->addr.buf[8] & 0xFF,
		unitdata->addr.buf[9] & 0xFF);

	printf("immediateAddress :%02x%02x%02x%02x%02x%02x\n",
		     te.te_ecb.immediateAddress[0],
		     te.te_ecb.immediateAddress[1],
		     te.te_ecb.immediateAddress[2],
		     te.te_ecb.immediateAddress[3],
		     te.te_ecb.immediateAddress[4],
		     te.te_ecb.immediateAddress[5]);
#endif


	ts_clear_direction();
	if (result == 1) {
VideoPut(87);


		IPXSendPacket(&te.te_ecb);
		ts_clear_direction();
		while(te.te_ecb.inUseFlag)
			IPXRelinquishControl();
VideoPut(86);


		result = RESULT(te.te_ecb);

#endif 	/* OS2 */
/*--------------------------------------------------------------------
 *   RETURN ON SUCCESSFUL COMPLETION
 *-------------------------------------------------------------------*/
		if (result == 0)
			return 0;
/*-----------------------------------------------------------------------
 *   ERROR => SET UNITDATA ERROR
 *----------------------------------------------------------------------*/
	} else
		result = IPX_NO_RESOURCE;	/* packet undeliverable */
	ts->ts_event = T_UDERR;
	TBASSIGN(ts->ts_addr, &DEST(ipx));
	ts->ts_packet_type = ipx.packetType;
	ts->ts_completion_code = result;
	return 0;
}


/***************************************************************************/
/***************************************************************************/


int GetImmediateaddress(unsigned long *time,unsigned char dest[12],
                        unsigned char network[4],unsigned char node[6])
{

#ifdef DOS
#define IPXGETLOCALTARGET IPXGetLocalTarget
#define ch
	int			transporttime;
#endif

#ifdef OS2
#define IPXGETLOCALTARGET  IpxGetLocalTarget
#define ch   (IPX_ECB far*)&
	unsigned long		transporttime;
#endif

	int res;
	unsigned long start = clock();


	if(!*time || start > *time){ /* this is the first for the fd or
		                                 old time has lapsed */
#ifdef DOS
	  	if((res = IPXGETLOCALTARGET(dest,ch te.te_ecb.immediateAddress,&transporttime)) == 0){
#else	   
		if((res = IPXGETLOCALTARGET(dest,ch te.te_ecb,&transporttime)) == 0){
#endif
			*time = start+ FREQUENCY_OF_RIP;
			/* update fd */
			memcpy((unsigned char*)&network[0],&dest[0],4);/* save the network address */
			memcpy((unsigned char*)&node[0],&dest[4],6); /* save the node address */
			return(1);
		}
		else {
		  	return(0);
   	}
	}
	else{ /* timer not changed but check if remote address has changed */
		if(memcmp(&dest[0],&network[0],10) != 0){ /* remote address has changed */

#ifdef DOS
			if((res = IPXGETLOCALTARGET(dest,ch te.te_ecb.immediateAddress,&transporttime))
				                                                         == 0){
#else
			if((res = IPXGETLOCALTARGET(dest,ch te.te_ecb,&transporttime)) == 0){
#endif
				*time = start+ FREQUENCY_OF_RIP;
				/* update fd */
				memcpy((unsigned char*)&network[0],&dest[0],4);/* save the network address */
				memcpy((unsigned char*)node,&dest[4],6); /* save the node address */
				return(1);
			}
			else 
				return (0);
		}

		else {

					return(1);/* remote address not changed and timer not elapsed */
		}
	}

}
