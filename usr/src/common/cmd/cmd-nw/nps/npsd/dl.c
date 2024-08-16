/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/npsd/dl.c	1.14"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: dl.c,v 1.14 1994/09/22 21:27:04 vtag Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include "npsd.h"
#include "dl.h"
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>

int EthernetDLPI(int, lanInfo_t *, int, char *);
void dl_perror(dl_error_ack_t *); 
int GetDlInfo(int, dl_info_ack_t *,lanInfo_t *);
int DLPIAttach( int, char *, long);
int DLPIBind(int, long, uint8 *);
int DLPISubBind(int, long, long);

char	xcinfo[100];
char	xdinfo[100];
struct strbuf	xcbuf = {sizeof(xcinfo), 0, xcinfo};
struct strbuf	xdbuf = {sizeof(xdinfo), 0, xdinfo};

/* Valid types of network adapters,
 * be sure and increment adapterTypeCount.
 */

a_functionMap_t adapterTypes[] = {
	{ EthernetDLPI,			"ETHERNET_DLPI",		},
	{ EthernetDLPI,			"TOKEN-RING_DLPI"		},
	{ NULL,					NULL					}
};

f_functionMap_t frameTypes[] = {
	{ FRAME_ETHERNET2,	"ETHERNET_II"		},
	{ FRAME_8022,		"ETHERNET_802.2",	},
	{ FRAME_8023,		"ETHERNET_802.3",	},
	{ FRAME_8022_SNAP,	"ETHERNET_SNAP"		},
	{ FRAME_8025,		"TOKEN-RING"		},
	{ FRAME_8025_SNAP,	"TOKEN-RING_SNAP"	},
	{ NULL,				NULL				}
};

/* This table is used to convert between DL messages and DLPI messages.
 */
 int DLPIMsgTable[] =
{
	NPS_DLPI_BADSAP,       NPS_DLPI_BADADDR,     NPS_DLPI_ACCESS,
	NPS_DLPI_OUTSTATE,     NPS_DLPI_SYSERR,      NPS_DLPI_BADCORR,
	NPS_DLPI_BADDATA,      NPS_DLPI_UNSUPPORTED, NPS_DLPI_BADPPA,
	NPS_DLPI_BADPRIM,      NPS_DLPI_BADQOSPARAM, NPS_DLPI_BADQOSTYPE,
	NPS_DLPI_BADTOKEN,     NPS_DLPI_BOUND,       NPS_DLPI_INITFAILED,
	NPS_DLPI_NOADDR,       NPS_DLPI_NOTINIT,     NPS_DLPI_UNDELIVERABLE,
	NPS_DLPI_NOTSUPPORTED, NPS_DLPI_TOOMANY,     NPS_DLPI_NOTENAB,
	NPS_DLPI_BUSY,         NPS_DLPI_NOAUTO,      NPS_DLPI_NOXIDAUTO,
	NPS_DLPI_NOTESTAUTO,   NPS_DLPI_XIDAUTO,     NPS_DLPI_TESTAUTO,
	NPS_DLPI_PENDING
};

/*ARGSUSED*/
int
EthernetDLPI(int fd, lanInfo_t *lanInfoPtr, int lanIndex, char *device)
{
	int		cCode;
	long	dlpiPPA;
	long	ifNameLen;
	char	ifName[NWCM_MAX_STRING_SIZE];
	char	tokenName[NWCM_MAX_STRING_SIZE];
	char	frameType[NWCM_MAX_STRING_SIZE];
	f_functionMap_t	*fp;
	dl_info_ack_t	ackBuf;
	struct strioctl     ioc;
	int                 csmacdmode;

	DPRINTF(("EthernetDLPI, process configuration parameters\n"));
	/* read optional dlpi ppa */
	(void) sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex,
		PPA_TOKEN);

	if ((Cret = NWCMGetParam(tokenName,NWCP_INTEGER,&dlpiPPA)) != NWCM_SUCCESS){
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X), 
			LAN_TOKEN_LEADER, lanIndex, PPA_TOKEN);
		return FAILURE;
	}

    /* read optional if name */
    sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, IF_NAME_TOKEN);

    if ((Cret = NWCMGetParam(tokenName,NWCP_STRING,ifName)) != NWCM_SUCCESS) {
	fprintf(stderr, "\n");
        NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X),
            LAN_TOKEN_LEADER, lanIndex, IF_NAME_TOKEN);
        return FAILURE;
    } else {
        ifNameLen = strlen(ifName);
    }

	if (Verbose) {
		if (ifNameLen) {
			printf( MsgGetStr(NPS_IFNAME_INFO), ifName);
		} else {
			if (dlpiPPA != -1) {
				printf( MsgGetStr(NPS_PPA_INFO), dlpiPPA);
			}
		}
	}

	/* read frame type */
	sprintf(tokenName, "%s%d%s", LAN_TOKEN_LEADER, lanIndex, FRAME_TYPE_TOKEN);

	if ((Cret = NWCMGetParam(tokenName,NWCP_STRING,frameType)) != NWCM_SUCCESS){
		fprintf(stderr, "\n");
		NWCMPerror( Cret, MsgGetStr(NPS_CFG_LAN_X_X), 
			LAN_TOKEN_LEADER, lanIndex, FRAME_TYPE_TOKEN);
		return FAILURE;
	}

	for (fp = frameTypes; fp->name; fp++)
		if (strcmp(frameType, fp->name) == 0)
			break;
	
	if (!fp->name) {
		fprintf( stderr, MsgGetStr(NPS_CFG_LAN_X_FRAME), 
			LAN_TOKEN_LEADER, lanIndex, FRAME_TYPE_TOKEN, frameType);
		return FAILURE;
	}

	/*
	**	Do not internationalize the following printf
	*/
	if (Verbose)
		printf("  %s", frameType);

	DPRINTF(("EthernetDLPI, do DL_ATTACH\n"));
	if( DLPIAttach( fd, fp->name, dlpiPPA) == FAILURE) {
		return( FAILURE);
	}

	DPRINTF(("EthernetDLPI, set frame type for 0x%X\n", fp->name));
	switch( fp->frame) {

	case FRAME_ETHERNET2:
		DPRINTF(("%s: EthernetDLPI(): set frame type Ethernet II\n"));
		cCode = DLPIBind(fd, NOVELL_SAP, lanInfoPtr->nodeAddress);
		break;

	case FRAME_8025:
		DPRINTF(("%s: EthernetDLPI(): set frame type TOKEN RING 802.5\n"));
		cCode = DLPIBind(fd, DSAP_8025, lanInfoPtr->nodeAddress);
		break;

	case FRAME_8022:
		DPRINTF(("%s: EthernetDLPI(): set frame type Ethernet 802.2\n"));
		cCode = DLPIBind(fd, DSAP_8022, lanInfoPtr->nodeAddress);
		break;

	case FRAME_8023:
		DPRINTF(("%s: EthernetDLPI(): set frame type Ethernet 802.3\n"));
		/*
		**	Put into 802.3 Raw mode
		*/
		if( DLPIBind(fd, RAW_8023, lanInfoPtr->nodeAddress) == FAILURE) {
			return( FAILURE);
		}

		ioc.ic_cmd                  = DLIOCCSMACDMODE;
		ioc.ic_timout               = 3;
		ioc.ic_len                  = sizeof (int);
		ioc.ic_dp                   = (char *) &csmacdmode;

		if( ioctl( fd, I_STR, &ioc) < 0) {
			perror ("ioctl: DLIOCCSMACDMODE:");
			return (FAILURE);
		}
		cCode = SUCCESS;
		break;

	case FRAME_8022_SNAP:
		DPRINTF(("%s: EthernetDLPI(): set frame type Ethernet SNAP\n"));
		if( DLPIBind(fd, DSAP_8022_SNAP, lanInfoPtr->nodeAddress) == FAILURE) {
			return(FAILURE);
		}
		cCode = DLPISubBind( fd, NOVELL_SNAP_ORG, NOVELL_SNAP_TYPE);
		break;

	case FRAME_8025_SNAP:
		DPRINTF(("%s: EthernetDLPI(): set frame type TOKEN RING SNAP\n"));
		if( DLPIBind(fd, DSAP_8025_SNAP, lanInfoPtr->nodeAddress) == FAILURE) {
			return(FAILURE);
		}
#ifdef NW_UP
		cCode = DLPISubBind( fd, NOVELL_SNAP_ORG, NOVELL_SNAP_TYPE);
#else
		cCode = SUCCESS;
#endif
		break;

	default:
		DPRINTF(("%s: EthernetDLPI(): unknown frame type 0x%X\n", fp->frame));
		cCode = FAILURE;
	}

	/* Binds may have happened - discover full info and
	 * set up for feed to lipmx
	 */
	GetDlInfo(fd, &ackBuf, lanInfoPtr);
	return( cCode);
}

int
GetDlInfo(int fd, dl_info_ack_t *ackBufp, lanInfo_t *lanInfoPtr)
{	
	int				flags;
	int				ret;
	struct strbuf	cbuf;
	dl_info_req_t	infoReq;

	/* read optional dlpi ppa */
	DPRINTF(("%s: GetDlInfo() : do DL_INFO_REQ\n", titleStr));
	infoReq.dl_primitive = DL_INFO_REQ;
	cbuf.len = sizeof(dl_info_req_t);
	cbuf.maxlen = sizeof(dl_info_req_t);
	cbuf.buf = (char *)&infoReq;
	flags = 0;

	if (putmsg(fd, &cbuf, NULL, flags)<0) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_PUTMSG));
		perror("");
		return FAILURE;
	}

	cbuf.len = sizeof(dl_info_ack_t);
	cbuf.maxlen = sizeof(dl_info_ack_t);
	cbuf.buf = (char *)ackBufp;

	if ((ret = getmsg(fd, &cbuf, NULL, &flags)) < 0) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_DI_GETMSG));
		perror("");
		return FAILURE;
	}
	xcbuf.len = 0;
	while ((ret == MORECTL) || (ret == MOREDATA)) {
		ret = getmsg(fd, &xcbuf, &xdbuf, &flags);
		DPRINTF (("npsd: (GetDlInfo) Discarding extra data, ctl %d, data %d\n",
			xcbuf.len, xdbuf.len));
	}
	DPRINTF(("%s: GetDlInfo() : getmsg() OK\n", titleStr));

	if (ackBufp->dl_primitive != DL_INFO_ACK) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_INFO));
		dl_perror((dl_error_ack_t *)ackBufp);
		return FAILURE;
	}

	if( lanInfoPtr != NULL) {
		lanInfoPtr->dlInfo.SDU_max = ackBufp->dl_max_sdu;
		lanInfoPtr->dlInfo.SDU_min = ackBufp->dl_min_sdu;
		lanInfoPtr->dlInfo.ADDR_length = ackBufp->dl_addr_length;
		lanInfoPtr->dlInfo.SAP_length = 0; /* UnixWare doesn't set it */
		if( xcbuf.len < sizeof( ackBufp->dl_addr_length)) {
			fprintf(stderr, MsgGetStr(NPS_DLPI_INFO));
			dl_perror((dl_error_ack_t *)ackBufp);
			return FAILURE;
		}
		memcpy(&lanInfoPtr->dlInfo.dlAddr[0], xcinfo,
			 ackBufp->dl_addr_length);

		/* Determine if physical address is offset within
		 * the DL SAP address.
		 */
		lanInfoPtr->dlInfo.physAddrOffset = (lanInfoPtr->dlInfo.SAP_length < 0)
				? 0
				: lanInfoPtr->dlInfo.SAP_length;

		DPRINTF(("%s: GetDlInfo() : done\n", titleStr));
		DPRINTF(("%s:    SDU_max = %d\n", titleStr, lanInfoPtr->dlInfo.SDU_max));
		DPRINTF(("%s:    SDU_min = %d\n", titleStr, lanInfoPtr->dlInfo.SDU_min));
		DPRINTF(("%s:    ADDR_length = 0x%X\n",
			titleStr, lanInfoPtr->dlInfo.ADDR_length));
		DPRINTF(("%s:    SAP_length = 0x%X\n",
			titleStr, lanInfoPtr->dlInfo.SAP_length));
		DPRINTF(("%s:    physAddrOffset = 0x%X\n",
			titleStr, lanInfoPtr->dlInfo.physAddrOffset));
#ifdef DEBUG
		{
			int z;
			DPRINTF(("GetDlInfo() Addr:\n\t"));
			for(z=0; z<lanInfoPtr->dlInfo.ADDR_length; z++)
				DPRINTF(("%2X ", lanInfoPtr->dlInfo.dlAddr[z]));
			DPRINTF(("\n"));
		}
#endif /* DEBUG */
	}
	return(SUCCESS);
}

/* DLPIBind()
 * fd : in:   file descriptor to operate on 
 * vsap : in:   VSAP to bind 
 * snapAddr : in:   optional subsequent bind for SNAP VSAP 
 * nodeAddr : out:  optional place to store node address 
 */
int
DLPIBind(int fd, long vsap, uint8 *nodeaddr)
{	
	struct strbuf		cbuf;
	char				recBuf[NPSD_BIND_ACK_BUF_SIZE];
	int					flags;
	dl_bind_req_t		bind;
	dl_bind_ack_t		*bindAck;
	int					 rval;

	/*
	 * Bind to VSAP
	 */
	bind.dl_primitive = DL_BIND_REQ;
	bind.dl_sap = vsap;
	bind.dl_max_conind = 0;
	bind.dl_service_mode = DL_CLDLS;
	bind.dl_conn_mgmt = 0;
	bind.dl_xidtest_flg = 0;

	cbuf.len = sizeof(dl_bind_req_t);
	cbuf.maxlen = sizeof(dl_bind_req_t);
	cbuf.buf = (char *)&bind;

	flags = 0;

	if (putmsg(fd, &cbuf, NULL, flags) < 0) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_BIND_PUTMSG));
		perror("");
		return FAILURE;
	}

	cbuf.len = sizeof(recBuf);
	cbuf.maxlen = sizeof(recBuf);
	cbuf.buf = recBuf;

	flags = 0;

	if ( (rval = getmsg(fd, &cbuf, NULL, &flags)) < 0) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_BIND_GETMSG));
		perror("");
		return FAILURE;
	}
	while ((rval == MORECTL) || (rval == MOREDATA)) {
		rval = getmsg(fd, &xcbuf, &xdbuf, &flags);
		DPRINTF (("npsd: (DLPIBind) Discarding extra data, ctl %d, data %d\n",
			xcbuf.len, xdbuf.len));
	}

	bindAck = (dl_bind_ack_t *)recBuf;

	if (bindAck->dl_primitive != DL_BIND_ACK) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_BIND));
		dl_perror((dl_error_ack_t *)recBuf);
		return(FAILURE);
	}
	DPRINTF(("%s: DLPI Bind complete\n", titleStr));

	/*
	 * Save Node Address If Requested
	 */
	if (nodeaddr) {
		memset(nodeaddr, 0, IPX_NODE_SIZE);
		memcpy(nodeaddr, &recBuf[bindAck->dl_addr_offset],
			MIN(bindAck->dl_addr_length, IPX_NODE_SIZE));
	}
	return SUCCESS;
}

int
DLPISubBind( int fd, long org, long type)
{
	struct strbuf		cbuf;
	int					flags;
	int					ret;
	struct {
		dl_subs_bind_req_t	SubsBind;
		snapHdr_t			SnapHdr;
	} recBuf;

	dl_subs_bind_ack_t		ackBuf;

	/*
	**	Do the subsequent BIND
	*/

	DPRINTF (("npsd: (DLPISubBind) Doing subsequent BIND, org 0x%X, type 0x%X\n",
		org, type));

	recBuf.SubsBind.dl_primitive = DL_SUBS_BIND_REQ;
	recBuf.SubsBind.dl_subs_sap_offset =
			(char *)&recBuf.SnapHdr - (char *)&recBuf.SubsBind;
	recBuf.SubsBind.dl_subs_sap_length = sizeof(snapHdr_t);
	recBuf.SnapHdr.snapOrg = org;
	recBuf.SnapHdr.snapType = type;

	cbuf.len    = (sizeof(recBuf));
	cbuf.maxlen = (sizeof(recBuf));
	cbuf.buf    = (char *)&recBuf;

	flags=0;
	if (putmsg(fd, &cbuf, NULL, flags) < 0) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_SUBBIND_PUTMSG));
		perror("");
		return( FAILURE);
	}


	DPRINTF (("npsd: (DLPISubBind) Sent subsequent BIND, getting response\n"));
	/*
	 *	Make sure our bind request is ACKed.
	 */

	cbuf.len = sizeof(ackBuf);
	cbuf.maxlen = sizeof(ackBuf);
	cbuf.buf = (char *)&ackBuf;
	flags = 0;

	
	if ( (ret = getmsg(fd, &cbuf, NULL, &flags)) < 0) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_SUBBIND_GETMSG));
		perror("");
		return(FAILURE);
	}
	while ((ret == MORECTL) || (ret == MOREDATA)) {
		ret = getmsg(fd, &xcbuf, &xdbuf, &flags);
		DPRINTF (("npsd: (DLPISubBind) Discarding extra data, ctl %d, data %d\n",
			xcbuf.len, xdbuf.len));
	}

	if ((cbuf.len < sizeof(dl_subs_bind_ack_t)) ||
			(ackBuf.dl_primitive != DL_SUBS_BIND_ACK)) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_SUBBIND));
		dl_perror((dl_error_ack_t *)&recBuf.SubsBind);
		return(FAILURE);
	}

	if (ackBuf.dl_primitive != DL_SUBS_BIND_ACK) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_SUBBIND));
		dl_perror((dl_error_ack_t *)&recBuf.SubsBind);
		return(FAILURE);
	}
	DPRINTF(("npsd: (DLPISubBind) Success on subs_bind_ack\n"));
	return( SUCCESS);
}

void
dl_perror( dl_error_ack_t *errorAck)
{
	int		err;
	char	message[MSG_MAX_LEN];

	if (errorAck->dl_primitive != DL_ERROR_ACK) {
		fprintf(stderr, MsgGetStr(NPS_DLPI_X), errorAck->dl_primitive);
		return;
	}
	DPRINTF(("\n%s: dl_perror: dl_primitive=0x%x, dl_error_primitive=0x%x,\n ", 
		titleStr, errorAck->dl_primitive, errorAck->dl_error_primitive));
	DPRINTF(("\t\t dl_errno=0x%x, dl_unix_errno=0x%x\n",
		errorAck->dl_errno, errorAck->dl_unix_errno));

	switch (err = errorAck->dl_errno) 
	{
		case DL_SYSERR:
			errno = errorAck->dl_unix_errno;
			perror(MsgGetStr(NPS_DLPI_SYSERR));
			break;
		default:
			if ( err > sizeof(DLPIMsgTable) )
				fprintf(stderr, MsgGetStr(NPS_DLPI_UNKNOWN), errorAck->dl_errno);
			else
			{
				strcpy(message, MsgGetStr(DLPIMsgTable[err]));
				fprintf(stderr, MsgGetStr(NPS_DLPI_BEGIN), message);
				break;
			}		
	}
	return;
}

int
DLPIAttach( 
	int	etherFd, 
	char *ifName,
	long ppa)
{
	int 			ret;
	struct strbuf 	cbuf;
	int 			flags;
	char			recCtl[256];
	dl_info_ack_t	ackBuf;
	dl_attach_req_t *attachReq;
	dl_ok_ack_t 	*okAck;
	int 			len;
	int 			ifNameLen;

	if( GetDlInfo( etherFd, &ackBuf, NULL) == FAILURE) {
		return(FAILURE);
	}

	/*
	 * If the device is provider style 2 then
	 */
	if( ackBuf.dl_provider_style == DL_STYLE2) {
		DPRINTF(("%s: EthernetDLPI() : DL_STYLE_2\n", titleStr));

		ifNameLen = strlen(ifName);
		len = sizeof(dl_attach_req_t);
		if( ifNameLen)
			len += ifNameLen + 1;
		if( (attachReq = malloc( len)) == NULL ) {
			perror("malloc");
			return(FAILURE);
		}
		/*
		 * Attach to ppa
		 */
		attachReq->dl_primitive = DL_ATTACH_REQ;
		attachReq->dl_ppa = ppa;
		cbuf.len = len;
		cbuf.maxlen = len;
		cbuf.buf = (char *)attachReq;
		if (ifNameLen)
			strcpy((((char *) attachReq) + sizeof(dl_attach_req_t)), ifName);

		flags = 0;
		if (putmsg(etherFd, &cbuf, NULL, flags) < 0) {
            fprintf(stderr, MsgGetStr(NPS_DLPI_ATTACH_PUTMSG));
            perror("");
			return(FAILURE);
		}
		free( attachReq);

		cbuf.len = 0;
		cbuf.maxlen = sizeof(recCtl);
		cbuf.buf = (char *)&recCtl;

		flags = 0;
		if ( (ret = getmsg(etherFd, &cbuf, NULL, &flags)) < 0) {
            fprintf(stderr, MsgGetStr(NPS_DLPI_ATTACH_GETMSG));
            perror("");
			return(FAILURE);
		}
		while ((ret == MORECTL) || (ret == MOREDATA)) {
			ret = getmsg(etherFd, &xcbuf, &xdbuf, &flags);
			DPRINTF (("npsd: (DLPIAttach) Discarding extra data, ctl %d, data %d\n",
				xcbuf.len, xdbuf.len));
		}
		okAck = (dl_ok_ack_t *)recCtl;
		if (cbuf.len < sizeof(okAck) 
			|| okAck->dl_primitive != DL_OK_ACK) {
            fprintf(stderr, MsgGetStr(NPS_DLPI_ATTACH));
			dl_perror((dl_error_ack_t *)cbuf.buf);
#ifdef DEBUG
		{
			unsigned char  *buf;
			int  i;

			DPRINTF(("\nDL_ATTACH_REQ: Response\n\t"));
			buf = (unsigned char *)cbuf.buf;
			for(i = 0; i < cbuf.len; i++) {
				DPRINTF(("%x ",*buf++));
			}
			DPRINTF(("\n"));
		}
#endif
			return(FAILURE);
		}
		DPRINTF(("%s: attach SUCCESS\n",titleStr));
	}
	DPRINTF(("%s: do DLPIBind() to etherFd=%x\n",titleStr, etherFd));
	return(SUCCESS);
}
