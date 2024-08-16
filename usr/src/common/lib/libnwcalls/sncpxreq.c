/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:sncpxreq.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwncpext.h"
#include "nwserver.h"

/*manpage*NWNCPExtensionRequest**********************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWNCPExtensionRequest
         (
            nuint16 conn,
            nuint32 NCPExtensionID,
            void NWPTR requestData,
            nuint16 reqDataLen,
            void NWPTR replyData,
            pnuint16 replyDataLen
         )

REMARKS: Passes an NCP extension request to the server

ARGS:  > NCPExtensionID
         The ID to use for the NCP extension request

       > requestData
       > reqDataLen
      <  replyData
      <  replyDataLen

INCLUDE: nwncpext.h

RETURN:  n/a

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2

SEE:

NCP:     37 --  NCP Extension Request

CHANGES: 14 Dec 1992 - written - jwoodbur
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWNCPExtensionRequest
(
   NWCONN_HANDLE  conn,
   nuint32        NCPExtensionID,
   void NWPTR     requestData,
   nuint16        reqDataLen,
   void NWPTR     replyData,
   pnuint16       replyDataLen
)
{
   NWCCODE ccode;
   nuint16 serverVer, maxReplySize;
   NW_FRAGMENT reqFrag[2], replyFrag[2];
   nuint8 bitBucket[1];
   nuint16 rdl = 0;

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if(serverVer < 3110)
      return(0x89fd);

   if(!requestData)
   {
      requestData = (nptr)bitBucket;
      reqDataLen = 0;
   }

   if(!replyDataLen)
      replyDataLen = &rdl;

   if(!replyData)
   {
      replyData = (nptr)bitBucket;
      *replyDataLen = 0;
   }

   maxReplySize = (serverVer == 3110) ? 100 : 530;

   if(reqDataLen <= 523 && *(pnuint16)replyDataLen <= maxReplySize)
   {
      struct
      {
         nuint16  len;
         nuint32 extID;
         nuint16  maxReplyLen;
      } reqBuf;

      reqBuf.len = (nuint16)NSwap16((nuint16)((nuint16) 6 + reqDataLen));
      reqBuf.extID       = NCPExtensionID;
      reqBuf.maxReplyLen = *replyDataLen;

      reqFrag[0].fragAddress   = &reqBuf;
      reqFrag[0].fragSize      = sizeof(reqBuf);

      reqFrag[1].fragAddress   = requestData;
      reqFrag[1].fragSize      = reqDataLen;

      replyFrag[0].fragAddress = replyDataLen;
      replyFrag[0].fragSize    = 2;

      replyFrag[1].fragAddress = replyData;
      replyFrag[1].fragSize    = *replyDataLen;

      ccode = NWRequest(conn, 37, 2, reqFrag, 2, replyFrag);
   }
   else
   {
      reqFrag[0].fragAddress   = requestData;
      reqFrag[0].fragSize      = reqDataLen;

      replyFrag[0].fragAddress = replyData;
      replyFrag[0].fragSize    = *replyDataLen;

      ccode = NWFragNCPExtensionRequest(conn, NCPExtensionID, 1, reqFrag, 1, replyFrag);
   }

   return ccode;
}

/*manpage*NWFragNCPExtensionRequest******************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWFragNCPExtensionRequest(
           NWCONN_HANDLE conn,
           nuint32 NCPExtensionID,
           nuint16 reqFragCount,
           NW_FRAGMENT NWPTR reqFragList,
           nuint16 replyFragCount,
           NW_FRAGMENT NWPTR replyFragList);

REMARKS: Passes an fragmented NCP extension request to the server

ARGS:  > NCPExtensionID
         The ID to use for the NCP extension request

       > reqFragCount
       > reqFragList
       > replyFragCount
       > replyFragList
         Entries in this list may be modified by NWFragNCPExtensionRequest

INCLUDE: nwncpext.h

RETURN:  n/a

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2

SEE:

NCP:     37 --  NCP Extension Request

CHANGES: 14 Dec 1992 - written - jwoodbur
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWFragNCPExtensionRequest(
  NWCONN_HANDLE conn,
  nuint32 NCPExtensionID,
  nuint16 reqFragCount,
  NW_FRAGMENT NWPTR reqFragList,
  nuint16 replyFragCount,
  NW_FRAGMENT NWPTR replyFragList)
{
NWCCODE ccode;
nuint8 dataBuf[524], NWPTR pDataBuf;
nuint32 reqLen, replyLen;
nuint16 serverVer, curFrag, numReqFrags, maxReqLen, curReqLen;
NW_FRAGMENT reqFrag[2], replyFrag[3];
union
{
  struct
  {
    nuint16  len;
    nuint32 extID;
    nuint32 version;
    nuint16  fragLen;      /* first.fraglen is same offset as next.fragLen */
    nuint32 totalLen;
    nuint32 maxReplyLen;
  } first;
  struct
  {
    nuint16  len;
    nuint32 extID;
    nuint32 sessionID;
    nuint16  fragLen;      /* first.fraglen is same offset as next.fragLen */
  } next;
} reqBuf;
struct
{
  nuint16 fragLen;
  nuint32 totalLen;
} replyBuf;

  if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
    return ccode;

  if(serverVer < 3110)
    return(0x89fd);

  reqLen = 0L;
  if(reqFragList)
    for(curFrag = 0; curFrag < reqFragCount; curFrag++)
      reqLen += (nuint32) reqFragList[curFrag].fragSize;

  replyLen = 0L;
  if(replyFragList)
    for(curFrag = 0; curFrag < replyFragCount; curFrag++)
      replyLen += (nuint32) replyFragList[curFrag].fragSize;

  reqBuf.first.len         = 0xffff;
  reqBuf.first.extID       = NCPExtensionID;
  reqBuf.first.version     = 0;
  reqBuf.first.totalLen    = reqLen;
  reqBuf.first.maxReplyLen = replyLen;

  reqFrag[0].fragAddress   = &reqBuf;
  reqFrag[0].fragSize      = sizeof(reqBuf.first);

  replyFrag[0].fragAddress = &reqBuf.next.sessionID;
  replyFrag[0].fragSize    = 4;

  replyFrag[1].fragAddress = &replyBuf;
  replyFrag[1].fragSize    = 6;

  replyFrag[2].fragAddress = dataBuf;
  replyFrag[2].fragSize    = 524;

  curFrag = 0;
  numReqFrags = 2;
  maxReqLen = 511;
  do
  {
    if(reqLen)
    {
      pDataBuf = reqFragList[curFrag].fragAddress;
      if(reqFragList[curFrag].fragSize > maxReqLen)
      {
        reqBuf.next.fragLen = maxReqLen;
        reqFragList[curFrag].fragSize -= maxReqLen;
        reqFragList[curFrag].fragAddress =
          (pnuint8)reqFragList[curFrag].fragAddress + maxReqLen;
      }
      else if(reqFragList[curFrag].fragSize == maxReqLen || reqFragCount == 1)
      {
        reqBuf.next.fragLen = (nuint16) reqFragList[curFrag++].fragSize;
      }
      else /* if multiple frags and current is less than max frag size */
      {
        curReqLen = maxReqLen;
        pDataBuf = dataBuf;
        reqBuf.next.fragLen = 0;
        while(curReqLen && curFrag < reqFragCount)
        {
          if(reqFragList[curFrag].fragSize > curReqLen)
          {
            NWCMemMove(pDataBuf, reqFragList[curFrag].fragAddress, curReqLen);
            reqFragList[curFrag].fragAddress =
              (pnuint8)reqFragList[curFrag].fragAddress + curReqLen;
            reqFragList[curFrag].fragSize -= curReqLen;
            reqBuf.next.fragLen += curReqLen;
            curReqLen = 0;
          }
          else
          {
            NWCMemMove(pDataBuf, reqFragList[curFrag].fragAddress,
               reqFragList[curFrag].fragSize);
            pDataBuf += reqFragList[curFrag].fragSize;
            curReqLen -= (nuint16) reqFragList[curFrag].fragSize;
            reqBuf.next.fragLen += (nuint16) reqFragList[curFrag].fragSize;
            curFrag++;
          }
        }
        pDataBuf = dataBuf;
      }

      reqFrag[1].fragSize    = reqBuf.next.fragLen;
      reqFrag[1].fragAddress = pDataBuf;
      reqLen -= reqBuf.next.fragLen;
    }
    else
    {
      reqBuf.next.fragLen = 0;
      numReqFrags = 1;
    }

    if((ccode = NWRequest(conn, 37, numReqFrags, reqFrag, 3, replyFrag)) != 0)
      return ccode;

    if(maxReqLen == 511)
    {
      reqFrag[0].fragSize = sizeof(reqBuf.next);
      maxReqLen = 519;
    }

  } while(reqLen);

  /* at this point all writes are done and reply packet contains read stuff, if any */

  replyFrag[1].fragSize = 2;
  curFrag = 0;

  if(replyLen > replyBuf.totalLen)
    replyLen = replyBuf.totalLen;

  while(replyLen)
  {
    pDataBuf = dataBuf;
    if(replyLen >= replyBuf.fragLen)
      replyLen -= replyBuf.fragLen;
    else
    {
      replyBuf.fragLen = (nuint16) replyLen;
      replyLen = 0;
    }

    while(curFrag < replyFragCount && replyBuf.fragLen)
    {
      if(replyBuf.fragLen >= replyFragList[curFrag].fragSize)
      {
        NWCMemMove(replyFragList[curFrag].fragAddress, pDataBuf,
            replyFragList[curFrag].fragSize);
        if(replyBuf.fragLen == replyFragList[curFrag].fragSize)
          replyBuf.fragLen = 0;
        else
        {
          pDataBuf += replyFragList[curFrag].fragSize;
          replyBuf.fragLen -= (nuint16) replyFragList[curFrag].fragSize;
        }

        curFrag++;
      }
      else
      {
        NWCMemMove(replyFragList[curFrag].fragAddress, pDataBuf,
            replyBuf.fragLen);
        replyFragList[curFrag].fragAddress =
          (pnuint8)replyFragList[curFrag].fragAddress + replyBuf.fragLen;
        replyFragList[curFrag].fragSize -= replyBuf.fragLen;
        replyBuf.fragLen = 0;
      }
    }
    if(replyLen)
    {
      if((ccode = NWRequest(conn, 37, numReqFrags, reqFrag, 3,
         replyFrag)) != 0)
      {
        return ccode;
      }
    }
  }
  return(0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/sncpxreq.c,v 1.6 1994/09/26 17:50:06 rebekah Exp $
*/
