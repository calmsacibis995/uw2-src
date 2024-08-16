/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:dsreq.c	1.2"
#include <nw/nwclient.h>
#include <nw/ntypes.h>
#include "nwaccess.h"
#include "nwdserr.h"
#include "nwdsdefs.h"


#define NDS_MAX_FRAGS         5
#define INITIAL_REQ_SIZE      25
#define NEXT_REQ_SIZE         5
#define INITIAL_REPLY_SIZE    12
#define NEXT_REPLY_SIZE       8


/*	Internal Functions */
N_INTERN_FUNC_C (NWRCODE) _GetMaxPacketSize( pNWAccess, pnuint );
N_INTERN_FUNC_C (nuint) _BuildFragmentList( nuint, pNWCFrag, nuint, pNWCFrag,
   								pnuint, pnuint, pNWCFrag );


/* -------------------------- Main Fragger Entry Point -------------*/
/*
   NWCFragmentRequest:
      D.S. request and reply packets are larger than 512 bytes
      which is the standard maximum packet size for traversing
      bridges. This routine performs any necessary data splitting
      and re-assembly.  The data is sent to the specified
      server and the number of bytes received is returned.

   pAccess           the shell connection id to send data to
   uFunction         the sub function of the 104 NCP
   luVerb            the Verb number of the 104 NCP
   uNumReqFratgs     number of fragments specified in sendFrags
   pReqFrags         addresses and length of the send buffers
   uNumReplyFrags    number of reply fragments that can be filled
   pReplyFrags       addresses and length of the reply buffers
   puActualReplySize number of data bytes received from the server
*/

N_GLOBAL_LIBRARY( NWRCODE )
NWCFragmentRequest
(
   pNWAccess pAccess,
   nuint    uFunction,
   nuint32  luVerb,
   nuint    uNumReqFrags,
   pNWCFrag pReqFrags,
   nuint    uNumReplyFrags,
   pNWCFrag pReplyFrags,
   pnuint   puActualReplyLen
)
{
   NWRCODE     err;

   nuint8      baReqBuffer[INITIAL_REQ_SIZE], /* req header buffer */
               baReplyBuffer[INITIAL_REPLY_SIZE];/* reply header buffer */
   nuint       uStaticReqOffset,        /* static send byte offset */
               uStaticReqFrag,          /* static send frag index */
               uStaticReplyOffset,      /* static reply byte offset */
               uStaticReplyFrag;        /* static reply frag index */

   NWCFrag     pIntReqFrag[NDS_MAX_FRAGS],    /* actual send fragments*/
               pIntReplyFrag[NDS_MAX_FRAGS];  /* actual recv fragments*/
   pNWCFrag    pNewReqFrag = pIntReqFrag,     /* req header frag */
               pNewReplyFrag = pIntReplyFrag; /* reply header frag */
   nuint       uIntReqFragCount,              /* actual send count */
               uIntReplyFragCount;            /* actual Recv count */

   nuint       uTotalReq,              /* total bytes to send */
               uTotalReply,            /* total bytes to recv */
               uPacketReply,           /* reply packet size */
               uMaxPktSize;            /* max packet size */

   nuint32     luTemp;
   nuint     	uTemp;

/*
**	find the maximum size we can transmit/receive
*/
   if((err = _GetMaxPacketSize(pAccess, &uMaxPktSize)) != 0)
      return err;

/*
**	Calculate the request data size
**	Total of all fragments to send plus 12 bytes for the header.
*/
   uTotalReq = 12;
   for (uStaticReqFrag = 0;
        uStaticReqFrag < uNumReqFrags;
        uStaticReqFrag ++)
   {
      uTotalReq += pReqFrags[uStaticReqFrag].uLen;
   }

/*
**	Calculate the reply data size
**	Total of all reply fragments. Headers do not count on replies.
*/
   for (uStaticReplyFrag = 0, uTotalReply = 0;
        uStaticReplyFrag < uNumReplyFrags;
        uStaticReplyFrag ++)
   {
      uTotalReply += pReplyFrags[uStaticReplyFrag].uLen;
   }

/*
**      Build our request header:
**      (only the first 2 fields are used after the first send).
**        offset  size    description
**           0   1 byte   subfunction
**           1   4 bytes  iteration handle (starts at -1L)
**           5   4 bytes  maximum fragment size
**           9   4 bytes  length of the request past this field
**          13   4 bytes  NCP flags (always 0L)
**          17   4 bytes  DS NCP verb
**          21   4 bytes  total reply size
**
**      Reply Header is:
**        offset  size    description
**           0   4 byte   fragment length
**           4   4 byte   iteration handle
*/
   baReqBuffer[0] = (nuint8)uFunction;
   NWCMemSet  (&baReqBuffer[ 1], 0xFF, 4);
   luTemp = (nuint32)uMaxPktSize - 8;     /* reply size less len and handle */
   NCopyLoHi32(&baReqBuffer[ 5], &luTemp);
   luTemp = (nuint32)uTotalReq;
   NCopyLoHi32(&baReqBuffer[ 9], &luTemp);
   NWCMemSet  (&baReqBuffer[13], 0x00, 4);
   NCopyLoHi32(&baReqBuffer[17], &luVerb);
   luTemp = (nuint32)uTotalReply;
   NCopyLoHi32(&baReqBuffer[21], &luTemp);

   pNewReqFrag->pAddr = (nptr)baReqBuffer;
   pNewReqFrag->uLen = (nuint)INITIAL_REQ_SIZE;

   pNewReplyFrag->pAddr = (nptr)baReplyBuffer;
   pNewReplyFrag->uLen = (nuint)INITIAL_REPLY_SIZE;

/*
**	Start sending data:
*/

/*
**	Initialize the reply and reply fragments
*/
   uStaticReqOffset   = uStaticReqFrag   = 0; /* static send offsets */
   uStaticReplyOffset = uStaticReplyFrag = 0; /* static recv offsets */
   uTotalReply = 0;

   for ( ; ; )
   {
      uIntReplyFragCount = _BuildFragmentList(uMaxPktSize,
                               pNewReplyFrag,
                               uNumReplyFrags, pReplyFrags,
                               &uStaticReplyFrag, &uStaticReplyOffset,
                               pIntReplyFrag);

      uIntReqFragCount = _BuildFragmentList(uMaxPktSize,
                               pNewReqFrag,
                               uNumReqFrags, pReqFrags,
                               &uStaticReqFrag, &uStaticReqOffset,
                               pIntReqFrag);

/*
**	Note: there is no way to know the reply size on some
**		platforms so get the real ncpReplyCount from
**		the data buffer.
*/
      if((err = NWCRequest(pAccess, DS_NCP_VERB,
            uIntReqFragCount, pIntReqFrag,
            uIntReplyFragCount, pIntReplyFrag,
            &uTemp)) != 0)
         return err;


/*
**	When the last byte is received, the server will
**	start sending data back.  Before that time, only
**	acknowledgements are exchanged.
*/
      NCopyLoHi32(&luTemp, &baReplyBuffer[0]);
      uPacketReply = (nuint) luTemp;

      uTotalReply += (uPacketReply - 4);

/*
**	if we did not get at least 4 bytes, the data is bogus
*/
      if (uPacketReply < 4)
         return (NWRCODE) ERR_REMOTE_FAILURE;

/*
**	no more data when the iteration handle is reset to 0xFF
*/
      if ( *((pnuint8)pIntReplyFrag->pAddr + 4) == 0xFF &&
           *((pnuint8)pIntReplyFrag->pAddr + 5) == 0xFF &&
           *((pnuint8)pIntReplyFrag->pAddr + 6) == 0xFF &&
           *((pnuint8)pIntReplyFrag->pAddr + 7) == 0xFF)
         break;

/*
**	Re-Build the request packet
*/

/*
**	Reset the Reply Buffer if we are not done sending data
*/
      if (uPacketReply == 4)
         uStaticReplyOffset = uStaticReplyFrag = 0; /* static recv offsets */
      if (uPacketReply > 7)
         pIntReplyFrag->uLen = NEXT_REPLY_SIZE;

/*
**	add the latest fragment handle to the request
*/
      NWCMemMove(&baReqBuffer[1], &baReplyBuffer[4], 4);
      pIntReqFrag->uLen = NEXT_REQ_SIZE;

   }

/*
**	set the optional reply size only if the caller wants it
*/
   if (puActualReplyLen)
      *puActualReplyLen = uTotalReply;


/*
**	return the DS completion code
*/
   NCopyLoHi32(&err, &baReplyBuffer[8]);

   return (err);
}


/*-----------------------------------------------------------------*/
/*
**   _GetMaxPacketSize:
**      Find the maximum data size we can send to the server.
**      Note: There is no call available on all platforms,
**      so we stubbed it with the standard maximum and hope
**      to do this later.
**
**   maxNCPSize - returned maximum packet size.
*/

N_INTERN_FUNC_C (NWRCODE) _GetMaxPacketSize
(
   pNWAccess pAccess,
   pnuint   pMaxNCPSize
)
{
	NWRCODE		rCode;

	rCode =  NWCGetConnInfo( pAccess, NWC_CONN_INFO_MAX_PACKET_SIZE,
								sizeof(nuint), (nptr)pMaxNCPSize);

	return(rCode);
}

/*-----------------------------------------------------------------*/
/*
**   _BuildFragmentList:
**      Build the outbound fragment list.
**
**   Returns: the number of fragments to send.
**
**   uPktSize       number of bytes to send
**   pFirstFrag     header fragment we are adding to the source list.
**   uSourceCount   number of fragments requested by the caller.
**   pSourceFrag    fragments requested by the caller.
**   puLastFrag     last source fragment sent.
**   puLastOffset   last source fragment byte sent.
**   pDestFrag     storage for fragment list to send out.
*/
N_INTERN_FUNC_C (nuint)
_BuildFragmentList (
   nuint       uPktSize,
   pNWCFrag    pFirstFrag,
   nuint       uSourceCount,
   pNWCFrag    pSourceFrag,
   pnuint      puLastFrag,
   pnuint      puLastOffset,
   pNWCFrag    pDestFrag
)
{
   nuint       uFillFragCount;


/*
**	ALWAYS build the first fragment to send.
**	(Assume that it is small enough to fit in a single packet.)
*/
   pDestFrag->pAddr = pFirstFrag->pAddr;
   pDestFrag->uLen  = pFirstFrag->uLen;

   uPktSize -= pDestFrag->uLen;
   pDestFrag++;

/*
**	Loop until we
**      - run out of send fragments / space
**       - have copied enough fragments to send the maximum size
**       - run out of input data space
*/
   for (uFillFragCount = 1;
        *puLastFrag < uSourceCount &&
            uFillFragCount < NDS_MAX_FRAGS;
        uFillFragCount++)
   {
      pDestFrag->pAddr =
         (nptr)((pnuint8)pSourceFrag[*puLastFrag].pAddr + *puLastOffset);
      pDestFrag->uLen  = pSourceFrag[*puLastFrag].uLen - *puLastOffset;

/*
**	check for valid fragment and make sure there is room
*/
      if ((*puLastFrag < uSourceCount) && (pDestFrag->uLen > uPktSize))
      {
/*
**	fragment was too large so split it and return
*/
         pDestFrag->uLen = uPktSize;
         *puLastOffset += uPktSize;
         uFillFragCount++;  /* Increment the count before exiting */
         goto Exit;
      }

/*
**	see if it is the last one
*/
      if (++*puLastFrag >= uSourceCount)
      {
         /* no more source fragments */
         uFillFragCount++;  /* Increment the count before exiting */
         goto Exit;
      }

/*
**	setup for the next fragment
*/
      uPktSize -= pDestFrag->uLen;
      pDestFrag++;
   }

Exit:
   return(uFillFragCount);
}
