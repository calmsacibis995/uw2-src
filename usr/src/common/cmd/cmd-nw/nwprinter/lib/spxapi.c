/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/lib/spxapi.c	1.2"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/lib/spxapi.c,v 1.3 1994/08/18 16:26:54 vtag Exp $";
#endif

#include <fcntl.h>
#include <tiuser.h>
#include <stropts.h>
#ifdef OS_AIX
#include <sys/poll.h>
#else
#include <poll.h>
#endif /* OS_AIX */
#include <errno.h>
#include "spxapi.h"
#include "spxtdr.h"
#include "spx.h"

#ifndef NULL
#define NULL 0
#endif

extern int t_errno;

#define MAX_SPX_ADDRESS_DISPLAY_LENGTH	30
#define MAX_ERRNO_DISPLAY_LENGTH		28


int
SPXOpenTransport(SPXHandle_t *spxHandle,int16 socket )
{
	struct t_bind *reqtbind;
	struct t_bind *rettbind;
	int ccode;

	spxHandle->fd = t_open( SPX_DRIVER_NAME, O_RDWR, (struct t_info *) 0 );
	if (spxHandle->fd == -1) {
		spxHandle->errno = t_errno;
		spxHandle->errnoType = SPXET_T_ERRNO;
		return FAILURE;
	}

	/* Set up buffer to get address and socket of bound connection*/
	rettbind = (struct t_bind *)t_alloc(spxHandle->fd, T_BIND, T_ALL);
	if (rettbind == NULL) {
		spxHandle->errno = t_errno;
		spxHandle->errnoType = SPXET_T_ERRNO;
		t_close(spxHandle->fd);
		return FAILURE;
	}
	rettbind->addr.len = SPX_ADDRESS_LENGTH;
	rettbind->addr.maxlen = SPX_ADDRESS_LENGTH;
	rettbind->addr.buf = (char *) spxHandle->spxAddress;
	rettbind->qlen = 0;

	if (socket) {
		/* Bind to a specific socket if not zero 
		also allow upto 5 listen connects*/
		reqtbind = (struct t_bind *)t_alloc(spxHandle->fd, T_BIND, T_ALL);
		if (reqtbind == NULL) {
			spxHandle->errno = t_errno;
			spxHandle->errnoType = SPXET_T_ERRNO;
			t_free((char *)rettbind, T_BIND);
			t_close(spxHandle->fd);
			return FAILURE;
		} 
		TDRSetSocketInSPXAddress( spxHandle->spxAddress, socket );
#ifdef HARD_DEBUG_1
		printf("SPXOpenTransport: t_bind to socket %x\n", socket);
#endif
		reqtbind->addr.len = SPX_ADDRESS_LENGTH;
		reqtbind->addr.maxlen = SPX_ADDRESS_LENGTH;
		reqtbind->addr.buf = (char *) spxHandle->spxAddress;
		reqtbind->qlen = 5;
	} else {
		/* set to bind to any socket	*/
		reqtbind = (struct t_bind *)NULL;
	}

	ccode = t_bind( spxHandle->fd, reqtbind, rettbind );
	if (reqtbind)
		(void)t_free((char *)reqtbind, T_BIND);
	if (rettbind)
		(void)t_free((char *)rettbind, T_BIND);
	if (ccode == -1) {
		spxHandle->errno = t_errno;
		spxHandle->errnoType = SPXET_T_ERRNO;
		(void)t_close( spxHandle->fd );
		return FAILURE;
	}

	spxHandle->errno = 0;
	spxHandle->rprinterMode = FALSE;
	spxHandle->dataStreamType = 0;
	spxHandle->packetCount = 0;
	spxHandle->headerSize = SPX_HEADER_LENGTH;
	spxHandle->isSpxII = FALSE;
	return SUCCESS;
}


int
SPXCloseTransport(SPXHandle_t *spxHandle )
{
	if (t_close( spxHandle->fd ) == -1) {
		spxHandle->errno = t_errno;
		spxHandle->errnoType = SPXET_T_ERRNO;
		return FAILURE;
	}

	return 0;
}

int
SPXSaveHeader(SPXHandle_t *spxHandle)
{
	struct strioctl ioc;

	/* Only if rprinterMode */
	if (spxHandle->rprinterMode) {
		ioc.ic_cmd = SPX_SAVE_SEND_HEADER;
		ioc.ic_timout = 0;
		ioc.ic_len = 0;
		ioc.ic_dp = (char *) 0;

		if (ioctl( spxHandle->fd, I_STR, &ioc ) == -1) {
			spxHandle->errno = errno;
			spxHandle->errnoType = SPXET_ERRNO;
#ifdef HARD_DEBUG_1
			printf("SPXSaveHeader SPX_SAVE_SEND_HEADER is bad\n");
#endif
			return FAILURE;
		}
#ifdef HARD_DEBUG_1
		printf("SPXSaveHeader SPX_SAVE_SEND_HEADER is done\n");
#endif
	}
	return SUCCESS;
}


int
SPXConnect(
	SPXHandle_t		 *spxHandle,
	SPXAddress_ta	  spxAddress,
	SPXOptionsInfo_t *spxOptions)
{
	struct t_call *tcallsnd;
	struct t_call *tcallrcv;
	struct t_discon *disconInfo;
	SPX2_OPTIONS *spx2Options;

	if (SPXSaveHeader(spxHandle) != SUCCESS)
		return FAILURE;

    if((tcallsnd=
		(struct t_call *)t_alloc(spxHandle->fd, T_CALL, T_ALL))==NULL)
    {
        spxHandle->errno = t_errno;
        spxHandle->errnoType = SPXET_T_ERRNO;
        return FAILURE;
    }
    if((tcallrcv=(struct t_call *)t_alloc(spxHandle->fd, T_CALL, T_ALL))==NULL)
    {
        spxHandle->errno = t_errno;
        spxHandle->errnoType = SPXET_T_ERRNO;
        return FAILURE;
	} 
	tcallsnd->addr.buf = (char *) spxAddress;
	tcallsnd->addr.len = tcallsnd->addr.maxlen;

#ifdef HARD_DEBUG_1
	printf("t_connect to = %x %x %x\n", 
				((long *)spxAddress)[0],
				((long *)spxAddress)[1],
				((long *)spxAddress)[2]);
#endif
	if (t_connect( spxHandle->fd, tcallsnd, tcallrcv ) == -1) {
		if (t_errno == TLOOK) {
			spxHandle->errno = t_look( spxHandle->fd );
			spxHandle->errnoType = SPXET_TLOOK;
			/* Set up for t_rcvdis */
			if ((disconInfo =  (struct t_discon *)t_alloc(spxHandle->fd, T_DIS, T_ALL))
													== NULL) {
				spxHandle->errno = disconInfo->reason;
				spxHandle->errnoType = SPXET_T_ERRNO;
				return FAILURE;
			}
			if (spxHandle->errno == T_DISCONNECT &&
			t_rcvdis( spxHandle->fd, disconInfo ) != -1) {
				spxHandle->errno = disconInfo->reason;
				spxHandle->errnoType = SPXET_T_DISCONNECT;
			}
			t_free((char *)disconInfo, T_DIS);
		} else {
			spxHandle->errno = t_errno;
			spxHandle->errnoType = SPXET_T_ERRNO;
		}
		return FAILURE;
	}

	if (spxOptions) {
#ifdef HARD_DEBUG_1
		printf("SPXConnect: Options not supported\n");
#endif
		return FAILURE;
	}
	
	spxHandle->packetCount = 0;
	spx2Options = (SPX2_OPTIONS *)tcallrcv->opt.buf;
	if (spx2Options->spxIISessionFlags & SPX_SF_SPX2_SESSION) {
#ifdef HARD_DEBUG_1
		printf("SPXConnect: SPXII Session is active\n");
#endif
		spxHandle->isSpxII = TRUE;
		spxHandle->headerSize = SPXII_HEADER_LENGTH;
	} else {
		spxHandle->isSpxII = FALSE;
		spxHandle->headerSize = SPX_HEADER_LENGTH;
	}
	return 0;
}


int
SPXDisconnect( SPXHandle_t	*spxHandle)
{
	if (t_snddis( spxHandle->fd, (struct t_call *) 0 ) == -1) {
		if (t_errno == TLOOK) {
			spxHandle->errno = t_look( spxHandle->fd );
			spxHandle->errnoType = SPXET_TLOOK;
		} else {
			spxHandle->errno = t_errno;
			spxHandle->errnoType = SPXET_T_ERRNO;
		}
		return FAILURE;
	}

	return 0;
}

int
SPXSend(
	SPXHandle_t	*spxHandle,
	uint8 packet[],
	int  packetLength,
	int  *rSentLength,
	int  moreFlag)
{
	int flags;
	int sndLength;
	int sentLength;
	uint8 *packetPtr;
	struct t_discon *disconInfo;
	int lookval;
	int retval;

	if (rSentLength)		/* Assume 0 bytes sent */
		*rSentLength = 0;

	if (spxHandle->rprinterMode) {
		packetPtr = (uint8 *)malloc( packetLength +
			SPX_RPRINTER_MODE_SEND_OVERHEAD );
		memcpy( packetPtr + SPX_RPRINTER_MODE_SEND_OVERHEAD, packet,
			packetLength );
		sndLength = packetLength + SPX_RPRINTER_MODE_SEND_OVERHEAD;
		packetPtr[SPX_DATA_STREAM_TYPE_SEND_OFFSET] =
			spxHandle->dataStreamType;
	} else {
		sndLength = packetLength;
		packetPtr = packet;
	}

	flags = moreFlag ? T_MORE : 0;

	retval = 0;
	if((lookval = t_look( spxHandle->fd )) < 0) {	 /* error on t_look */
		spxHandle->errno = t_errno;
		spxHandle->errnoType = SPXET_TLOOK;
		retval = FAILURE;
	}
	if(lookval > 0) {								 /* handle event */
		if(lookval == T_DISCONNECT) {
			retval = FAILURE;
			/* Set up for t_rcvdis */
			if ((disconInfo =  (struct t_discon *)t_alloc(spxHandle->fd,
					T_DIS, T_ALL)) == NULL) {
				spxHandle->errno = t_errno;
				spxHandle->errnoType = SPXET_T_ERRNO;
			}
			if(t_rcvdis( spxHandle->fd, disconInfo ) != -1) {
				spxHandle->errno = disconInfo->reason;
				spxHandle->errnoType = SPXET_T_DISCONNECT;
			} else {
				spxHandle->errno = t_errno;
				spxHandle->errnoType = SPXET_T_ERRNO;
			}
			t_free((char *)disconInfo, T_DIS);
		}
	}
	if(retval != FAILURE) {			/* no event so snd */
#ifdef HARD_DEBUG_1
		{
			int i;
			printf("Send SPX %d bytes. data first 8 bytes =\n", sndLength);
			for (i = 0; i < 8; i++)
				printf("%02x ", (unsigned char)packetPtr[i]);
			printf("\n");
		}
#endif
		while((sentLength = t_snd( spxHandle->fd, (char *)packetPtr, sndLength, flags )) == -1) {
			if( (t_errno == TSYSERR) && (errno == EINTR)) {
				continue;
			}
			else {
				spxHandle->errno = t_errno;
				spxHandle->errnoType = SPXET_T_ERRNO;
				retval = FAILURE;
			}
		}
	}
		
	if (spxHandle->rprinterMode)
		free( packetPtr );
	if (rSentLength) {		/* Return number sent */
		if (spxHandle->rprinterMode) /* Adjust for function size */
			sentLength -= SPX_RPRINTER_MODE_SEND_OVERHEAD;
		*rSentLength = sentLength;
	}
	return(retval);
}

int
SPXReceive(SPXHandle_t *spxHandle, uint8 packet[], int *packetLength)
{
	int flags;
	int rcvLength;
	int rcvMaxLength;
	uint8 spxHeaderPacket[SPXII_HEADER_LENGTH];
	SPXIIHeader_t spxHeader;

	struct t_discon *disconInfo;

	if (spxHandle->rprinterMode && (spxHandle->packetCount == 0)) {
		/* Get next header */
		rcvMaxLength = spxHandle->headerSize;
#ifdef HARD_DEBUG_1
		printf("SPXRec read header size = %d\n", rcvMaxLength);
#endif
		while ((rcvLength = t_rcv( spxHandle->fd, (char *)spxHeaderPacket,
					rcvMaxLength, &flags )) == -1) {

			if( (t_errno == TSYSERR) && (errno == EINTR) )
				continue;
			else {
				if(t_errno == TLOOK) {
					spxHandle->errno = t_look( spxHandle->fd );
					spxHandle->errnoType = SPXET_TLOOK;
					if (spxHandle->errno != T_DISCONNECT)
						return FAILURE;
					else {
						if ((disconInfo = (struct t_discon *)t_alloc(
							spxHandle->fd, T_DIS, T_ALL)) != NULL) {
							if(t_rcvdis( spxHandle->fd, disconInfo )
								!= -1) {
								spxHandle->errno = disconInfo->reason;
								spxHandle->errnoType = SPXET_T_DISCONNECT;
								t_free((char *)disconInfo, T_DIS);
								return FAILURE;
							}
							t_free((char *)disconInfo, T_DIS);
						}
					}
				}
			}
			spxHandle->errno = t_errno;
			spxHandle->errnoType = SPXET_T_ERRNO;
			return FAILURE;
		}
		if (rcvLength < spxHandle->headerSize) {
			spxHandle->errno = SPX_BAD_HEADER;
			spxHandle->errnoType = SPXET_USER;
			return FAILURE;
		}
		TDRGetSPXHeader((uint8 *)spxHeaderPacket, &spxHeader);
		
		spxHandle->dataStreamType = spxHeader.dataStreamType;
		spxHandle->packetCount =
			rcvMaxLength = spxHeader.length - spxHandle->headerSize;
		spxHandle->destinationConnID = spxHeader.destinationConnID;

		if (rcvMaxLength <= 0) {
			spxHandle->packetCount = 0;
			*packetLength = 0;
			if (rcvMaxLength == 0)
				return 0;
#ifdef DEBUG
			printf("DEBUG-- SPXRecieve BAD Header length\n");
#endif __NOTE__
			return FAILURE;
		}

	}

	rcvMaxLength = *packetLength;

	while ((rcvLength = t_rcv( spxHandle->fd, (char *)packet,
		rcvMaxLength, &flags )) == -1)
	{
		if( (t_errno == TSYSERR) && (errno == EINTR) )
			continue;
		else {
			if(t_errno == TLOOK) {
				spxHandle->errno = t_look( spxHandle->fd );
				spxHandle->errnoType = SPXET_TLOOK;
				if (spxHandle->errno != T_DISCONNECT)
					return FAILURE;
				else {
					if ((disconInfo = (struct t_discon *)t_alloc(
						spxHandle->fd, T_DIS, T_ALL)) != NULL) {
						if(t_rcvdis( spxHandle->fd, disconInfo )
							!= -1) {
							spxHandle->errno = disconInfo->reason;
							spxHandle->errnoType = SPXET_T_DISCONNECT;
							t_free((char *)disconInfo, T_DIS);
							return FAILURE;
						}
						t_free((char *)disconInfo, T_DIS);
					}
				}
			}
		}
		spxHandle->errno = t_errno;
		spxHandle->errnoType = SPXET_T_ERRNO;
		return FAILURE;
	}
#ifdef HARD_DEBUG_1
	printf("SPXRecv == read %d bytes\n", rcvLength);
#endif
	*packetLength = rcvLength;
	if (spxHandle->rprinterMode) {
		spxHandle->packetCount -= rcvLength;
		if (spxHandle->packetCount < 0) {
#ifdef DEBUG
			spxHandle->packetCount = 0;
			printf("SPXRecv == Bad length in header\n");
#endif
		}
	}
	return 0;
}

void
SPXGetSocketFromAddress(
	SPXAddress_ta *spxAddress,
	uint16		  *socket)
{
	TDRGetSocketFromSPXAddress((uint8 *)spxAddress, socket );
}


void
SPXSetSocketInAddress(
	SPXAddress_ta *spxAddress,
	uint16		   socket)
{
	TDRSetSocketInSPXAddress((uint8 *) spxAddress, socket );
}


char *
SPXDisplayAddress( SPXAddress_ta spxAddress)
{
	int    i;
	uint8 *sp;
	char  *tp;
	static char dispString[MAX_SPX_ADDRESS_DISPLAY_LENGTH];

	sp = spxAddress;
	tp = dispString;

	for (i = 0; i < SPX_ADDRESS_NETWORK_LENGTH; i++, sp++ )
		tp += sprintf( tp, "%02X", *sp & 0xFF );

	*tp++ = ':';

	for (i = 0; i < SPX_ADDRESS_NODE_LENGTH; i++, sp++ )
		tp += sprintf( tp, "%02X", *sp & 0xFF );

	*tp++ = ':';

	for (i = 0; i < SPX_ADDRESS_SOCKET_LENGTH; i++, sp++ )
		tp += sprintf( tp, "%02X", *sp & 0xFF );

	return dispString;
}


char *
SPXDisplayErrno( SPXHandle_t	*spxHandle)
{
	char *dispTemplate;
	static char dispString[MAX_ERRNO_DISPLAY_LENGTH];

	if (!spxHandle) {
		sprintf( dispString, "errno = %d", errno );
		return dispString;
	}

	if (!spxHandle->errno) {
		*dispString = '\0';
		return dispString;
	}

	switch (spxHandle->errnoType) {
		case SPXET_T_ERRNO:
			dispTemplate = "t_errno = %d";
			break;
		case SPXET_TLOOK:
			dispTemplate = "TLOOK = 0x%04X";
			break;
		case SPXET_T_DISCONNECT:
			dispTemplate = "T_DISCONNECT = 0x%02X";
			break;
		case SPXET_ERRNO:
			dispTemplate = "errno = %d";
			break;
		default:
			dispTemplate = "error = %d";
			break;
	}

	sprintf( dispString, dispTemplate, spxHandle->errno );

	return dispString;
}

