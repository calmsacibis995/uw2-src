/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/trcvud.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: trcvud.c,v 1.2 1994/02/18 15:07:12 vtag Exp $"

/****************************************************************************
*	Program Name:		TLI/OS2 API
*	File Name:			trcvud.c
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
#include <errno.h>
#include <stdlib.h>
#include <tiuser.h>
#include <ts.h>
#ifdef OS2
#define RCVUDATA spx_rcvudata
#include <tli_spx.h>
#else
#define RCVUDATA t_rcvudata
#define API
#define REG register
#endif

#include <string.h>


/****  t_rcvudata  **********************************************************
*
*		Receive an IPX packet.
*
*	Parameters:
*		int						fd				(I)	file descriptor
*		struct t_unitdata		*unitdata	(O)	packet info
*		int						*flags		(O)	T_MORE flag info
*
*	Return:  0,  success
*		    -1,  failure
*
*	Comments:	See UNIX programmer's guide t_rcvudata(3N)
*				DOS IPX is organized around pre-posting listen ECBs
*					as asynchronous events (TBIND.C).
*				OS2 IPX is synchronous with time outs.  The LSL level
*					handles buffering of received packets with no
*					posted ECBs.
*				The difference in paradigms is reflected in the following code.
*
*****/
int API RCVUDATA (int fd, REG	struct t_unitdata *unitdata, int *flags)
{
int					i1;
IPX_HEADER	far * 	ipx;
TS_ECBP				te; /*additional variable added for the patch fix */
TSP					ts;
unsigned			result;

#ifdef DOS
	int					iflags;
#endif	/* DOS */

/*-----------------------------------------------------------------------
 *   VALID FD ?
 *----------------------------------------------------------------------*/

VideoPut(65);

	ts = FD_TO_TS(fd);

	if (!ts)
		return TLI_ERR(TBADF);
/*-----------------------------------------------------------------------
 *   IPX ?
 *----------------------------------------------------------------------*/
	if (ts->ts_type != TS_IPX)
		return TLI_ERR(TNOTSUPPORT);

#ifdef XTI
	if (ts->ts_state != T_IDLE)
		return TLI_ERR(TOUTSTATE); /* XTI provided error-04/23/91 */
#endif

/*-----------------------------------------------------------------------
 *   PENDING EVENT ?
 *----------------------------------------------------------------------*/
	if (ts->ts_event)
		return TLI_ERR(TLOOK);
/*-----------------------------------------------------------------------
 *   INCORRECT unitdata BUFFER LENGTH
 *----------------------------------------------------------------------*/
	if (unitdata->addr.maxlen > 0  &&  unitdata->addr.maxlen < 12)
		return TLI_ERR(TBUFOVFLW);

  

/*-----------------------------------------------------------------------
 *   BLOCKING MODE:  WAIT FOR A PACKET TO BE RECEIVED
 *   NON-BLOCKING MODE:  IF A PACKET HAS NOT BEEN RECEIVED, RETURN IMMEDIATELY
 *----------------------------------------------------------------------*/
#ifdef OS2
	/*----------------------------------------------------------------------
	 *   OS2: GRAB RIGHT TO BE THE CURRENT IPX RECEIVER ON THIS FD
	 *			(NON-BLOCKING == NOT WILLING TO WAIT FOR RIGHT => TNODATA)
	 *---------------------------------------------------------------------*/
	if (ts->ts_flags & F_TS_NONBLOCK) {
		if (DosSemRequest(&ts->ts_ipx_rcv_priv,SEM_IMMEDIATE_RETURN) == ERROR_SEM_TIMEOUT)
			return TLI_ERR(TNODATA);
	}
	else
		DosSemRequest(&ts->ts_ipx_rcv_priv,SEM_INDEFINITE_WAIT);
	/*----------------------------------------------------------------------
	 *   OS2: RETRIEVE ALL BUFFERS CURRENTLY WAITING TO BE RECEIVED
	 *---------------------------------------------------------------------*/
	result = SUCCESSFUL;
	for (te = ts->ts_rcv_posted.head;
			result == SUCCESSFUL && te != NULL;
			te = te->te_next)
	{
		
		result = IpxReceive(TS_SOCKET(ts),(ULONG) IPX_RETURN_IMMEDIATELY,(
                            IPX_ECB far*)&te->te_ecb);
		if (result == SUCCESSFUL){
			ts_rcv_esr(&te->te_ecb);
			}
	}
	/*----------------------------------------------------------------------
	 *   OS2: TIMED OUT ON A RECEIVE WHILE GETING BUFFERS
	 *      OS2 TIMEOUT CASE 1: IF TIMED OUT + NON-BLOCKING + DATA HAS COME IN
	 *         TNODATA
	 *---------------------------------------------------------------------*/
	if (result == IPX_TIMEOUT)
	{
		if (ts->ts_rcved.head == NULL)
		{
			if (ts->ts_flags & F_TS_NONBLOCK)
			{
				DosSemClear(&ts->ts_ipx_rcv_priv);  /* RELIQUISH IPX RCV RIGHT */
				return TLI_ERR(TNODATA);
			}
		/*----------------------------------------------------------------------
		 *   OS2 TIMEOUT CASE 2: IF TIMED OUT + BLOCKING + DATA HAS COME IN
		 *      RETRIEVE THE NEXT PACKET THAT COMES IN
		 *---------------------------------------------------------------------*/
			else
			{
				te = (TS_ECBP)ts->ts_rcv_posted.head;
				result = IpxReceive(TS_SOCKET(ts),IPX_INDEFINITE_WAIT,
                                (IPX_ECB far *)&te->te_ecb);
				if (result == SUCCESSFUL)
					ts_rcv_esr(&te->te_ecb);
			}
		}
		/*----------------------------------------------------------------------
		 *   OS2 TIMEOUT CASE 3: IF TIMED OUT + DATA HAS COME IN
		 *      SUCCESSFUL
		 *---------------------------------------------------------------------*/
		else
			result = SUCCESSFUL;
		/*----------------------------------------------------------------------
		 *   OS2 TIMEOUT CASE 4: IF TIMED OUT + NOT SUCCESS OR TIME OUT
		 *      PASS THE RESULT ON
		 *---------------------------------------------------------------------*/
	}
	te = (TS_ECBP)ts->ts_rcved.head;
	DosSemClear(&ts->ts_ipx_rcv_priv);  /* RELIQUISH IPX RCV RIGHT */
#else
	/*--------------------------------------------------------------------------
 	 *   DOS: WAIT FOR IPX EVENT
 	 *-------------------------------------------------------------------------*/

VideoPut(66);


	while (Q_empty(&ts->ts_rcved)) {
		if (ts->ts_flags & F_TS_NONBLOCK){
			return TLI_ERR(TNODATA);
		}
		IPXRelinquishControl();
	}

	te = (TS_ECBP)ts->ts_rcved.q_next;
	result = te->te_ecb.completionCode;

VideoPut(67);

#endif /* OS2 */
/*-----------------------------------------------------------------------
 *   CHECK FOR SUCCESSFUL COMPLETION CODE
 *----------------------------------------------------------------------*/
	switch (result) {
	case SUCCESSFUL:
		break;

	case IPX_PACKET_OVERFLW:
		ts->ts_state = T_ERRORLOCK;
		i1 = TLI_SYSERR(EPROTO);
		goto post_buffer;
	default:
#ifdef OS2
	case IPX_SOCKET_NOT_FOUND:
		i1 = TLI_ERR(TOUTSTATE);
		goto post_buffer;
#endif
		break;
	}
/*-----------------------------------------------------------------------
 *   ADJUST DATA INFO FOR IPX PACKET HEADER LENGTH
 *----------------------------------------------------------------------*/
	ipx = (IPX_HEADER far *) SPX_HDR_ADDR(te->te_ecb);
	if (te->te_rptr == SPX_HDR_ADDR(te->te_ecb)) {
		te->te_rptr += IPX_HDR_LEN;
		te->te_len = PACKET_LENGTH(ipx);
		te->te_len -= IPX_HDR_LEN;
	}
/*-----------------------------------------------------------------------
 *   ASSIGN udata INFO
 *----------------------------------------------------------------------*/
	unitdata->udata.len = MIN(unitdata->udata.maxlen, te->te_len);
	/* the (void *) is to force a small pointer in small data model */
	/* te_rptr is a far * and was allocated out of DGROUP in small model */
	/* so this will work */
	memcpy(unitdata->udata.buf, (void *)te->te_rptr, unitdata->udata.len);
	te->te_rptr += unitdata->udata.len;
	te->te_len -= unitdata->udata.len;
	if (unitdata->addr.maxlen > 0) {
		TBASSIGN(unitdata->addr.buf, &SOURCEP(ipx));
		unitdata->addr.len = 12;

	}

VideoPut(68);

/*-----------------------------------------------------------------------
 *   ASSIGN opt INFO
 *----------------------------------------------------------------------*/
	if (unitdata->opt.maxlen >= sizeof(IPX_OPTS)) {
		((IPX_OPTS *)unitdata->opt.buf)->ipx_type = ipx->packetType;
		unitdata->opt.len = sizeof(IPX_OPTS);
	} else
		unitdata->opt.len = 0;
/*-----------------------------------------------------------------------
 *   ASSIGN T_MORE FLAG
 *			IF MORE DATA LEAVE PACKET OUT THERE UNTIL ALL READ
 *----------------------------------------------------------------------*/
	if (te->te_len != 0) {
		*flags = T_MORE;
		return 0;
	}
	i1 = 0;
/*--------------------------------------------------------------------------
 *   REPOST THE TS_ECB
 *-------------------------------------------------------------------------*/
post_buffer:
#ifdef OS2
	te->te_rptr = SPX_HDR_ADDR(te->te_ecb);
	move_element_to_new_list(ts,&ts->ts_rcved,&ts->ts_rcv_posted,te);
#else

VideoPut(69);

	(void)ts_post_buffer(ts,te);
#endif	/* OS2 */
/*-----------------------------------------------------------------------
 *   RETURN
 *----------------------------------------------------------------------*/
	*flags = 0;
VideoPut(70);

	return i1;
}

/***************************************************************************/
/***************************************************************************/
