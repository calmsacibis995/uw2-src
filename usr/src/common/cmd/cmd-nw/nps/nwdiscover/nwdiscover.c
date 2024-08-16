/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nwdiscover/nwdiscover.c	1.20"
#ident	"$Id: nwdiscover.c,v 1.49.2.1.2.1 1995/02/03 15:38:33 meb Exp $"

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

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stropts.h>
#include <poll.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/ipx_app.h>
#include <nwmsg.h>
#include <nwconfig.h>
#include <npsmsgtable.h>
#include <sys/sap_app.h>
#include <time.h>
#include <limits.h>
#ifdef _KERNEL
#undef _KERNEL
#endif

extern int NWCMConfigFileLineNo;

/*
 * int strcmpi(s1, s2)
 *
 *	Like strcmp(), but case insensitive.
 */

static int
strcmpi(char * s1, char * s2)
{
	int	r;

	while (*s1 && *s2)
		if ((r = (int) (toupper(*s1++) - toupper(*s2++))) != 0)
			return r;
	return (int) (toupper(*s1) - toupper(*s2));
}


/*
 * General defines and initializations
 */
char titleStr[] = "nwdiscover";
char program[] = "nwdiscover";
#define FALSE 0
#define TRUE 1
#define FAILURE 0xFF
#define SUCCESS 0x00
#define SERVER_NAME_LENGTH 48

/*
 * 	              Command Line Options 
 *		     .......................
 * a	check all frame types and device types, even if there is a response 
 *	from a server (default is to stop searching after find first server).
 * 
 * f	query the network to see if there is a server responding to 
 *	messages of this frame type. (default is to try all frame types)
 *
 * r	if there is no response from the network, try again, -r times
 *	(default is 2)
 * 
 * t	timeout after -t seconds (default is 3)
 *
 * p	output packet debug printout
 *
 * P	input packet debug printout
 *
 * D	Debug printout
 *
 * d	use only this path name as the network device driver (default is 
 *	what ever is returned by netinfo command).
 *
 * v	turn on verbose mode. This will print debugging info to stderr
 *	(default is verbose mode off)
 *
 * u	update the config file. If this option is on, the config file
 *	will be updated with the frame type and network name and address.
 *	The config file updated will be *******
 *	(default is to not update the config file).
 *
 * e    exclude searching for this frame type.
 *
 * no options
 * 	all frame types and network devices will be searched until the first
 *	response from a GNS.
 *..............................................................................
 *
 *  DPRINTF prints only if -D option is set and -DDEBUG is defined
 */
#ifdef DEBUG_PACKET
#ifndef DEBUG
#define DEBUG
#endif
#endif

/*
 *
 *	Set up the default values for the command line options
 * 
 */
#ifdef DEBUG
int debugOpt = FALSE;
int debugInPkt = FALSE;
int debugOutPkt = FALSE;
int debugInDl = FALSE;
int debugOutDl = FALSE;
static char *optstr = "e:f:r:t:d:DBbPpvuac";
#else
static char *optstr = "e:f:r:t:d:vuac";
#endif

#ifdef DEBUG
#define DPRINTF(arg) if(debugOpt) fprintf arg
#define DPACKET(test, strbuf, string) if(test == TRUE) \
	PrintIpxPacket((ServerPacketStruct_t *)(strbuf)->buf, string, (strbuf)->len, 1)
#define DIPXPACKET(test, strbuf, string) if(test == TRUE) \
	PrintIpxPacket((ServerPacketStruct_t *)(strbuf)->buf, string, (strbuf)->len, 0)
#define PERROR(arg) perror(arg)
#else
#define DPRINTF(arg)
#define DPACKET(test, strbuf, string)
#define DIPXPACKET(test, strbuf, string)
#define PERROR(arg)
#endif

extern char *optarg;
char tmpNetworkDevice[PATH_MAX];	/* hold the netowrk device name */
char tmpName[NWCM_MAX_STRING_SIZE];
int tmpBool;
char networkDevice[PATH_MAX];		/* default network device */
int gnsAlarmTime=3 * 1000;/* default timeout in milliseconds */
int gnsRetries=2;		/* default retries */
int lanInstance=0;		/* total number of responses from gns */
int tokenRing=0;		/* use token ring  */
uint32 ourNetwork=0;
uint32 checkAllFrames = FALSE;
uint32 checkAllDevs = FALSE;
uint32 doFindServer = FALSE;
uint32 triedCurrentConfig = FALSE;
uint32 gotDev= FALSE;
uint32 verbose = FALSE;
uint32 concise = FALSE;
uint32 updateConfigFile = FALSE;


/*
 * Definition of Function Prototypes
 */
void inventNetwork (void);
int FindAllServers (void);
int FindOneServer (uint16  sap);
int dlpiInfo (int fd);
void alarmSignal (int);
void ExitAll (int val);
int bindSap (int fd, uint16 sap);
int SendGNS (int fd, uint16 sap, char *name);
int SendRIP (int fd);
char *sap2text (uint16 sap);
void UpdateConfigFile(int lanInstance, uint16 sap);
void sapUnbind (int fd);
int fscanf(FILE *strm, const char *format, ...);
int access(const char *path, int amode);
int ConfigFileReset(int Lan);

uint16	socket = 0x5030;
/*
 * Definition Get Nearest Server Request structures and data
 */
typedef struct {
        ipxHdr_t        header;
        uint16          type;
        uint16          serverType;
        uint8           pad[16];
} getNearestServer_t;

getNearestServer_t getNearestServerData = {
	{	IPX_CHKSUM, 
	 	GETINT16(sizeof(getNearestServer_t) - 16 ),
	 	IPX_TRANSPORT_CONTROL,
	 	IPX_NULL_PACKET_TYPE,
	 	{
			{0x00, 0x00, 0x00, 0x00},
			{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
			{0x04, 0x52}
		},
		{	{0x00, 0x00, 0x00, 0x00},
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
			{0xFF, 0xFF}				/* To be filled in */
		},
	},
	GETINT16( 3),
	GETINT16( 4),
	{	0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	}
};

struct strbuf gnsData = {
	sizeof( getNearestServerData),
	sizeof( getNearestServerData) - 16,
	(char *) &getNearestServerData
};

typedef struct {
	dl_unitdata_req_t req;
	uint8             ether_addr[6];
	uint16            lsap;
	uint8             pad[36];
} getNearestServerRequest_t;

/*	strbufs for Get Nearest Server Query message.  */
getNearestServerRequest_t getNearestServerRequest = {
	{	DL_UNITDATA_REQ,			/* dl_primitive */
	 	8,					/* dl_dest_addr_length */
	 	sizeof(dl_unitdata_req_t),		/* dl_dest_addr_offset */
	 	{ 25, 5 }				/* dl_priority */
	},
	{	0xff, 0xff, 0xff, 0xff, 0xff, 0xff},	/* ether_addr[6] */
	0,					/* lsap */
	{	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	/* padding[36] */
	 	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}
};

/*	strbufs for Get Nearest Server Query message.  */
struct strbuf gnsControl = {
	sizeof( getNearestServerRequest),  /* max length */
	sizeof( getNearestServerRequest),  /* length */
	(char *) &getNearestServerRequest  /* address of message */
};
typedef struct {
	uint16	replyType;
	uint16	serverType;
	char	name[SERVER_NAME_LENGTH];
} gnsReply_t;


/*
 * Definition of sap and frame types
 * This is the array to contain all possible frame types to try.
 * The array is terminated with the last enry equal to 0.
 * The first array entry can be overrided with the -f command line
 * option.
*/
static struct snap_sap snapsap = { 0, 0x8137};

int ethernet_II = FALSE;
int ethernet_8023 = FALSE;
int ethernet_8022 = FALSE;
int ethernet_SNAP = FALSE;
int token_ring = FALSE;
int token_ring_SNAP = FALSE;

#define BIND_ETHERNET2  0x8137
#define BIND_8023       0x8023
#define BIND_8022       0x8022
#define BIND_SNAP       0x8138
#define BIND_8025       0x8025
#define BIND_8025_SNAP	0x8238
#define SAP_EXCLUDE	0xefff

typedef struct {
	uint16  sap;
	char    name[SERVER_NAME_LENGTH];
} sapProbe_t;
/*
 * DO NOT CHANGE THE ORDER OF THIS ARRAY WITHOUT MODIFYING THE CODE BEBLOW
 * THAT REFERENCE IT
 * 
 * Two arrays are need to fix the problem if a frame type is excluded on
 * one device due to frame type conflicts, that fame type should not be 
 * excluded on all devices. All frame types should be tried on any 
 * addtional devices. The array "allSapsToTry" will be copied to 
 * "sapsToTry" before changing to a different device.
 *
 * Be sure to change both of the following array if the size of one of
 * them need to change.
 */ 
sapProbe_t	allSapsToTry[] = {
	{ BIND_8022,      "" },
	{ BIND_ETHERNET2, "" },
	{ BIND_SNAP,      "" },
	{ BIND_8023,      "" },
	{ BIND_8025,      "" },
	{ BIND_8025_SNAP, "" },
	{ 0,              "" }  /* mark the end */
};
sapProbe_t	sapsToTry[] = {
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" }  /* mark the end */
};

#define NETWORKS 20
uint16 sapToBind;
uint32	networks[NETWORKS];

/*..........................................................................*/
#ifdef DEBUG

typedef struct ServPacketInfo
{
	uint16					TargetType;
	uint8					TargetServer[NWMAX_SERVER_NAME_LENGTH];
	uint8					TargetAddress[12];
	uint16					ServerHops;
} ServPacketInfo_t;

typedef struct ServerPacketStruct
{
	ipxHdr_t				ipxHdr;	
	uint16					Operation;
	/* can have multiple ServPacketInfo_t structures, DONOT use sizeof */
	ServPacketInfo_t		ServerTable[1];	
} ServerPacketStruct_t;

/*..........................................................................
 *
 *	These functions are used, for debugging
 */
void 
PrintIpxAddress(
	ipxAddr_t *ipxAddr)
{
	int i;

	fprintf(stderr," 0x");
	for (i=0; i<IPX_NET_SIZE; i++)
		fprintf(stderr,"%02X",ipxAddr->net[i]);

	fprintf(stderr," ");
	for (i=0; i<IPX_NODE_SIZE; i++)
		fprintf(stderr,"%02X",ipxAddr->node[i]);

	fprintf(stderr," ");
	for (i=0; i<IPX_SOCK_SIZE; i++)
		fprintf(stderr,"%02X",ipxAddr->sock[i]);
	return;
}

/*..........................................................................
 *
 *	Print Server Information, all parameters are in net order
 */
void
PrintSrvPacket(
	uint16		 type,
	uint8		*name,
	ipxAddr_t	*addr,
	uint16		 hops)
{
	fprintf(stderr, "type 0x%04X @ ", GETINT16(type));
	PrintIpxAddress(addr);
	fprintf(stderr, ", hops %2d \"%s\"\n", GETINT16(hops), name);
	return;
}

/*..........................................................................*/
void
PrintIpxPacket(
	ServerPacketStruct_t *ipxPacketPtr,
	char *str,
	int packetLength,
	int rawflag)
{
	int i,co,sp,ix,hdrsize;
	ServPacketInfo_t *srvpkt;

	if( rawflag || (ipxPacketPtr->ipxHdr.chksum != 0xFFFF)) {
		hdrsize = 0;
		fprintf(stderr, "\n%sLength 0x%X\n", str, packetLength);
		rawflag++;
	} else {
		hdrsize = IPX_HDR_SIZE;
		/*
		**	Print IPX header
		*/
		fprintf(stderr,"\n%sipxPacket dump from 0x%X\n", str, ipxPacketPtr);
		fprintf(stderr,"%scheckSum: 0x%04X\n", 
			str, ipxPacketPtr->ipxHdr.chksum);
		fprintf(stderr,"%slength  : 0x%04X\n",
			str, GETINT16(ipxPacketPtr->ipxHdr.len));
		fprintf(stderr,"%spt      : 0x%02X\n", str, ipxPacketPtr->ipxHdr.pt);
		fprintf(stderr,"%stc      : 0x%02X\n", str, ipxPacketPtr->ipxHdr.tc);
		fprintf(stderr,"%sdest    : ", str);
		PrintIpxAddress(&ipxPacketPtr->ipxHdr.dest);
		fprintf(stderr,"\n");
		fprintf(stderr,"%ssrc     : ", str);
		PrintIpxAddress(&ipxPacketPtr->ipxHdr.src);
		fprintf(stderr,"\n");
		
		packetLength -= (IPX_HDR_SIZE + sizeof(ipxPacketPtr->Operation));

		if( (packetLength <= 0) || (packetLength % sizeof(ServPacketInfo_t))) {
			rawflag++;
		}

		/*
		*	Print Operation
		*/
		if( (rawflag == 0) || (packetLength == 2) || (packetLength == 0)) {
			fprintf(stderr,"%sfunction: ", str);
			switch( GETINT16(ipxPacketPtr->Operation)) {
			case SAP_GSQ:
				fprintf(stderr,"GENERAL SERVICE REQUEST");
				break;
			case SAP_GSR:
				fprintf(stderr,"GENERAL SERVICE RESPONSE");
				break;
			case SAP_NSQ:
				fprintf(stderr,"NEAREST SERVICE REQUEST");
				break;
			case SAP_NSR:
				fprintf(stderr,"NEAREST SERVICE RESPONSE");
				break;
			default:
				fprintf( stderr, "0x%04x", GETINT16(ipxPacketPtr->Operation));
				rawflag++;
				break;
			}
		}

		/*
		**	Print Service Type
		*/
		if( packetLength == 2) {
			fprintf(stderr,", type 0x%04X\n", PGETINT16(&ipxPacketPtr->ServerTable));
			return;
		} else {
			fprintf(stderr, "\n");
		}

		if( rawflag == 0) {
			/*
			**	Print Server Entries
			*/
			co = packetLength / sizeof(ServPacketInfo_t);
			srvpkt = ipxPacketPtr->ServerTable;
			for( i = 0; i < co; i++) {
				fprintf(stderr,"%sserver %d: ", str, i+1);
				PrintSrvPacket( srvpkt->TargetType, srvpkt->TargetServer,
					(ipxAddr_t *)srvpkt->TargetAddress, srvpkt->ServerHops);
				srvpkt++;
			}
			return;
		}
	}

	if( rawflag) {
		/*
		**	This wasn't an ordinary SAP packet, dump data in hex
		*/
		co = sp = 0;
		ix = hdrsize;
		fprintf(stderr, "%s%02X:", str, ix);
		for( i=((uint32)ipxPacketPtr + hdrsize); 
				i < packetLength + (uint32)ipxPacketPtr; i++) {
			if( co == 16) {
				co = 0;
				sp = 0;
				fprintf(stderr,"\n");
				fprintf(stderr, "%s%02X:", str, ix);
			}
			if( sp == 4) {
				fprintf(stderr, " ");
				sp = 0;
			}
			fprintf(stderr," %02X",*(uint8 *)i); 
			co++;
			sp++;
			ix++;
		}
		fprintf(stderr,"\n");
	}
	return;

}
#endif /* DEBUG */

/*..........................................................................*/
/*ARGSUSED*/
main ( int argc, char *argv[], char *envp[] )

{
	FILE *fp;
	int n, index, cfd;
	char c;
	int ccode;
	char *cp;

	ccode = MsgBindDomain(MSG_DOMAIN_DSCVR, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);

	if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
		if(ccode == NWCM_SYNTAX_ERROR)
			fprintf(stderr,"%s: NWCM-2.0-6: A syntax error was detected in the NetWare Configuration file.\nError on line number %d.\n",
					titleStr, NWCMConfigFileLineNo);
		else
			fprintf(stderr,"%s: Error NWCM-2.0-%d.\n", titleStr, ccode);
		ExitAll(1);
	}

	DPRINTF(( stderr, "%s: (main) Entry!\n", titleStr));

	/*
	 *	Clear list of networks found
	 */
	for( n=0; n < NETWORKS; n++) {
		networks[n] = 0;
	}

	while ((c = getopt(argc, argv, optstr )) != (char)-1) {
		switch (c) {
			case 'a' :
				checkAllFrames = TRUE;
				checkAllDevs = TRUE;
				break;
			case 'c' :
				concise = TRUE;
				break;
			case 'v' :
				verbose = TRUE;
				break;
			case 'r' :
				if( (gnsRetries = atoi( optarg)) < 1) {
					fprintf( stderr, MsgGetStr(NWD_RETRY));
					ExitAll(1);
				}
				break;
			case 't' :
				if( (gnsAlarmTime = atoi( optarg)) < 0) {
					fprintf(stderr, MsgGetStr(NWD_TIMEOUT));
					ExitAll(1);
				}
				if( gnsAlarmTime == 0) {
					gnsAlarmTime = 500; /* 0 = 1/2 second */
				} else {
					gnsAlarmTime *= 1000;
				}
				break;
			case 'u':
				if( NWCMGetParam("ipx_auto_discovery", NWCP_BOOLEAN, &tmpBool)
						!= SUCCESS) {
					tmpBool = 0;
				}
				if (tmpBool == 0) {
					fprintf(stderr, MsgGetStr(NWD_AUTOD));
					ExitAll(1);
				}
				updateConfigFile = TRUE;
				break;
#ifdef DEBUG
			case 'b':
				debugInDl = TRUE;
				break;
			case 'B':
				debugOutDl = TRUE;
				break;
			case 'D':
				debugOpt = TRUE;
				break;
			case 'p':
				debugInPkt = TRUE;
				break;
			case 'P':
				debugOutPkt = TRUE;
				break;
#endif
			case 'd': {
				char *s=optarg;
					if (isupper( *s))
						*s=tolower(*s);
					strcpy(networkDevice, s);
				/*
				 * make sure this is a valid device/path name
				 */
				if(access(networkDevice, R_OK|W_OK) < 0) {
					perror(networkDevice);
					ExitAll(1);
				}
				gotDev = TRUE;
				break;
				}
			case 'f': {			/* force frame type */
				char	*s=optarg;
				uint16  frame;

				do {
					if( isupper( *s))
						*s=tolower(*s);
				} while( *(++s) != '\0' );
				if( strcmpi( optarg, "ETHERNET_II") == 0) {
					frame = BIND_ETHERNET2;
					ethernet_II = TRUE;
				}
				else if (strcmpi( optarg, "ETHERNET_802.2") == 0) {
					ethernet_8022 = TRUE;
					frame = BIND_8022;
				}
				else if (strcmpi( optarg, "ETHERNET_802.3") == 0) {
					ethernet_8023 = TRUE;
					frame = BIND_8023;
				}
				else if (strcmpi( optarg, "ETHERNET_SNAP") == 0) {
					ethernet_SNAP = TRUE;
					frame = BIND_SNAP;
				}
				else if (strcmpi( optarg, "TOKEN-RING") == 0) {
					token_ring = TRUE;
					frame = BIND_8025;
				}
				else if (strcmpi( optarg, "TOKEN-RING_SNAP") == 0) {
					token_ring_SNAP = TRUE;
					frame = BIND_8025_SNAP;
				}
				else {
					fprintf(stderr,MsgGetStr(NWD_FRAME));
					fprintf(stderr,MsgGetStr(NWD_FRAME_CHOOSE));
					ExitAll(1);
				}
				allSapsToTry[0].sap = frame; /* do this frame type	*/
				allSapsToTry[1].sap = 0;	 /* only */

				break;
			}
			case 'e': {			/* exclude frame type */
				char	*s=optarg;
				uint16  frame;

				do {
					if( isupper( *s))
						*s=tolower(*s);
				} while( *(++s) != '\0' );
				if( strcmpi( optarg, "ETHERNET_II") == 0) {
					frame = BIND_ETHERNET2;
					ethernet_II = TRUE;
				}
				else if (strcmpi( optarg, "ETHERNET_802.2") == 0) {
					ethernet_8022 = TRUE;
					frame = BIND_8022;
				}
				else if (strcmpi( optarg, "ETHERNET_802.3") == 0) {
					ethernet_8023 = TRUE;
					frame = BIND_8023;
				}
				else if (strcmpi( optarg, "ETHERNET_SNAP") == 0) {
					ethernet_SNAP = TRUE;
					frame = BIND_SNAP;
				}
				else if (strcmpi( optarg, "TOKEN-RING") == 0) {
					token_ring = TRUE;
					frame = BIND_8025;
				}
				else if (strcmpi( optarg, "TOKEN-RING_SNAP") == 0) {
					token_ring_SNAP = TRUE;
					frame = BIND_8025_SNAP;
				}
				else {
					fprintf(stderr,MsgGetStr(NWD_FRAME));
					fprintf(stderr,MsgGetStr(NWD_FRAME_CHOOSE));
					ExitAll(1);
				}
				for(index=0; allSapsToTry[index].sap != 0; index++) {
					if (allSapsToTry[index].sap == frame)
						allSapsToTry[index].sap = SAP_EXCLUDE;
				}

				break;
			}

			case '?':
			default :
				fprintf(stderr, MsgGetStr(NWD_USAGE));
				fprintf(stderr, MsgGetStr(NWD_USAGEa));
				fprintf(stderr, MsgGetStr(NWD_USAGEc));
				fprintf(stderr, MsgGetStr(NWD_USAGEd));
#ifdef DEBUG
				fprintf(stderr, MsgGetStr(NWD_USAGEb));
				fprintf(stderr, MsgGetStr(NWD_USAGEB));
				fprintf(stderr, MsgGetStr(NWD_USAGED));
				fprintf(stderr, MsgGetStr(NWD_USAGEp));
				fprintf(stderr, MsgGetStr(NWD_USAGEP));
#endif
				fprintf(stderr, MsgGetStr(NWD_USAGEe));
				fprintf(stderr, MsgGetStr(NWD_USAGEf));
				fprintf(stderr, MsgGetStr(NWD_USAGEr));
				fprintf(stderr, MsgGetStr(NWD_USAGEt));
				fprintf(stderr, MsgGetStr(NWD_USAGEu));
				fprintf(stderr, MsgGetStr(NWD_USAGEv));
				ExitAll(1);
		}
	}

	if( concise) {
		verbose = FALSE;
	}

	/*
	**	Only allow root users to go on from here.  We'll do that by
	**	checking ability to open nwconfig file in RDWR mode.
	*/
	if((cp = (char *)NWCMGetConfigFilePath()) == NULL) {
		fprintf(stderr, MsgGetStr(NWD_PRIV));
		ExitAll(1);
	}

	if ((cfd = open(NWCMConfigFilePath, O_RDWR)) < 0) {
		fprintf(stderr, MsgGetStr(NWD_PRIV));
		ExitAll(1);
	} else {
		close(cfd);
	}
	
	if( NWCMGetParam("ipx_internal_network", NWCP_INTEGER, &ourNetwork)
			== NWCM_SUCCESS) {
		/*
		**	If internal network is configured, must have -au options
		*/
		if( (ourNetwork != 0) &&
				(updateConfigFile == TRUE) && (checkAllDevs == FALSE)) {
			fprintf(stderr, MsgGetStr(NWD_INT_NET));
			ExitAll(1);
		}
	}

	for(index=0; allSapsToTry[index].sap != 0; index++) {
		sapsToTry[index] = allSapsToTry[index];
	}

	/*
	 * If a device is passed in, do NOT do the netinfo command
	 */
	if (gotDev == TRUE)
		checkAllDevs = FALSE;

	else {

	/*
	 * execute the netinfo command to get the network device(s)
	 * that are configured on the system. For each network device, 
	 * find the frame type(s) associted with it.
	 */
	/*
	 * no need for error checking on system(), it is done automatically 
	 */
		if (system("netinfo -l dev > /tmp/frame.dev") < 0)
			ExitAll(1);

		if ((fp = fopen( "/tmp/frame.dev", "r")) == NULL) {
			fprintf(stderr, MsgGetStr(NWD_TMP_FILE));
			perror("");
			ExitAll(1);
		}
	}
	if (checkAllDevs) {
		ConfigFileReset( 1 ); /* reinitialize all configured networks */
		while ((fscanf(fp, "%s", tmpNetworkDevice)) != -1)
		{
			sprintf(networkDevice, "/dev/%s", tmpNetworkDevice);
			FindAllServers();
		}
	} else {
		if (gotDev == FALSE) {
			if((fscanf(fp, "%s", tmpNetworkDevice)) == -1) {
				fprintf(stderr, MsgGetStr(NWD_NO_DEV));
				perror("");
				ExitAll(1);
			}
			sprintf(networkDevice, "/dev/%s", tmpNetworkDevice);

			/*
			 * if not searching all devices and frame types, check to see if
			 * lan_1_network is defined already. If so, search that one
			 * and possibly exit if info is found.
			 * Otherwise, search all types until the first one is found.
			 */
			if ((NWCMGetParam("lan_1_network", NWCP_INTEGER, &ourNetwork) == NWCM_SUCCESS) 
					&& ourNetwork != 0) {
				NWCMGetParam("lan_1_adapter", NWCP_STRING, tmpName);
				if(strlen(tmpName) != 0)
					strcpy(networkDevice, tmpName);

				NWCMGetParam("lan_1_frame_type", NWCP_STRING, tmpName);
				{
					char	*s=tmpName;
					uint16  frame;
					do {
						if( isupper( *s))
							*s=tolower(*s);
					} while( *(++s) != '\0' );
					if( strcmpi( tmpName, "ETHERNET_II") == 0) {
						frame = BIND_ETHERNET2;
						ethernet_II = TRUE;
					}
					else if (strcmpi( tmpName, "ETHERNET_802.2") == 0) {
						ethernet_8022 = TRUE;
						frame = BIND_8022;
					}
					else if (strcmpi( tmpName, "ETHERNET_802.3") == 0) {
						ethernet_8023 = TRUE;
						frame = BIND_8023;
					}
					else if (strcmpi( tmpName, "ETHERNET_SNAP") == 0) {
						ethernet_SNAP = TRUE;
						frame = BIND_SNAP;
					}
					else if (strcmpi( tmpName, "TOKEN-RING") == 0) {
						token_ring = TRUE;
						frame = BIND_8025;
					}
					else if (strcmpi( tmpName, "TOKEN-RING_SNAP") == 0) {
						token_ring_SNAP = TRUE;
						frame = BIND_8025_SNAP;
					}
					else {
						fprintf(stderr, MsgGetStr(NWD_FRAME1));
						ExitAll(1);
					}
					/*
					 * Call FindOneServer to find the information for lan 1
					 * and possibly update the configuration file. If
					 * FindOneServer returns, something went wrong.
					 * Rewind the device file so we can search
					 * all servers. If the desired server is found, the
					 * program exits.
					 */
					FindOneServer(frame);
					triedCurrentConfig = TRUE;
					rewind(fp);
					if((fscanf(fp, "%s", tmpNetworkDevice)) == -1) {
						fprintf(stderr, MsgGetStr(NWD_NO_DEV));
						perror("");
						ExitAll(1);
					}
					sprintf(networkDevice, "/dev/%s", tmpNetworkDevice);
				}
			} else {
				/*
				**	Case of -u option and NWCM parameters wrong.  Go through
				**	all of the devices in netdrivers before giving up and
				**	inventing a network.
				*/
				if(FindAllServers() == FAILURE)
				{
					while ((fscanf(fp, "%s", tmpNetworkDevice)) != -1)
					{
						sprintf(networkDevice, "/dev/%s", tmpNetworkDevice);
						if(FindAllServers() != FAILURE)
							break;
					}
				}
				doFindServer = TRUE;
			}
		}
		/*
		 * when you reach this point, either the -d command line option
		 * was set or the there was no response for the frame type already 
		 * listed in the config file. In either event, search through the
		 * entire sap table for a response from the server
		 */
		if(doFindServer == FALSE)
		{
			if(FindAllServers() == FAILURE)
			{
				if(gotDev == FALSE) {
					while ((fscanf(fp, "%s", tmpNetworkDevice)) != -1)
					{
						sprintf(networkDevice, "/dev/%s", tmpNetworkDevice);
						if(FindAllServers() != FAILURE)
							break;
					}
				}
			}
		}
	}

	if(updateConfigFile) {
		if( lanInstance == 0) {
			if(triedCurrentConfig == FALSE)
			{
				inventNetwork();
				if ( tokenRing ) {
					UpdateConfigFile(1, BIND_8025 );
					if( !concise) {
						fprintf(stdout, MsgGetStr(NWD_INVENT),
							ourNetwork, sap2text(BIND_8025), networkDevice);
					}
				} else {
					UpdateConfigFile(1, BIND_8022 );
					if( !concise) {
						fprintf(stdout, MsgGetStr(NWD_INVENT),
							ourNetwork, sap2text(BIND_8022), networkDevice);
					}
				}
			}
		}
		/*
		 *	Turn off autodiscovery if -au options set.  (Ignore errors
		 *	so we don't break machines that don't support autodiscovery
		 */
		if(checkAllDevs)
		{
			tmpBool = 0;
			NWCMSetParam("ipx_auto_discovery", NWCP_BOOLEAN, &tmpBool);
		}
	}
	ExitAll(0);
	/*NOTREACHED*/
	return(0);
}
/*
 * When you get to this point, you were unable to find any type of server on
 * the network. As such, invent one and update the config file accordingly.
 */
void
inventNetwork()
{
	FILE *fp;
	int n;

	if ((n = NWCMGetParam("server_name", NWCP_STRING, tmpName)) != NWCM_SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(n, MsgGetStr(NWD_CFG_X), "server_name");
		ExitAll(1);
	}
	ourNetwork = (uint32) (tmpName[0] + tmpName[1] +  tmpName[2] +  tmpName[3] + time(0));

	if(gotDev == FALSE)
	{
		if ((fp = fopen( "/tmp/frame.dev", "r")) == NULL) {
			fprintf(stderr, MsgGetStr(NWD_TMP_FILE));
			perror("");
			ExitAll(1);
		}
		if ((fscanf(fp, "%s", tmpNetworkDevice)) == -1) {
			fprintf(stderr, MsgGetStr(NWD_NO_DEV)) ;
			perror("");
			ExitAll(1);
		}
		sprintf(networkDevice, "/dev/%s", tmpNetworkDevice);
	
	}

}

/*
 * Try a specified frame type. If found, possibly update config file and exit.
 */
int
FindOneServer(uint16 frame_type)
{
	int index, j, fd;

	DPRINTF(( stderr, "%s: (FindOneServers) Entry\n", titleStr));
	DPRINTF(( stderr, "%s: (FindOneServers) \n\n\n\n", titleStr));

	for(index=0; allSapsToTry[index].sap != 0; index++) {
		sapsToTry[index] = allSapsToTry[index];
	}

	/* 
	 * find the index in sapsToTry
	 */
	for(index=j=0; sapsToTry[j].sap; j++) 
		if (sapsToTry[j].sap == frame_type) {
			index = j;
			break;
		}
	if ((sapsToTry[index].sap == 0) || (sapsToTry[index].sap == SAP_EXCLUDE)) {

		DPRINTF((stderr, "%s: sapsToTry[%d].sap == 0 || sapsToTry[%d].sap == SAP_EXCLUDE\n", titleStr, index));
		return(FAILURE);
	}

	if( ( fd=open(networkDevice, O_RDWR)) <0) {
		fprintf(stderr, MsgGetStr(NWD_OPEN), networkDevice);
		perror("");
		return( FAILURE);
	}

	if ( dlpiInfo ( fd ) == FAILURE )
		return ( FAILURE );

	if( bindSap( fd, sapsToTry[index].sap) != SUCCESS) {
		close(fd);
		return(FAILURE);
	}

	for( j=0; j < gnsRetries; j++) {
		/*
		 * send SAP and/or RIP request 
		 */
		if( (SendGNS( fd, sapsToTry[index].sap, (caddr_t)sapsToTry[index].name)
				== SUCCESS) || (SendRIP( fd) == SUCCESS)) {
			/*
			 * UPDATE CONFIG FILE with the following info... (always use lan 1
			 * for this)
			 */
			if( concise) {
				/*	Do not internationalize, output designed for app input */
				fprintf(stdout,"%s %s 0x%.8X %d\n",
					sap2text(sapsToTry[index].sap), networkDevice, ourNetwork, 1);
			} else {
				fprintf(stdout, MsgGetStr(NWD_GOT_NET), 1,
					sap2text(sapsToTry[index].sap), networkDevice, ourNetwork);
			}
			UpdateConfigFile(1, sapsToTry[index].sap);
			sapUnbind (fd);
			close(fd);
			ExitAll(0);
		}
	}
	sapUnbind (fd);
	close(fd);
	return(FAILURE);

}

/*
 * For each frame type in the sapsToTry array, call bindSap to see if a 
 * server will respond to the get nearest server request. For each time 
 * there is a response, increment the lanInstance variable to denote 
 * another instance of a Lan.
 */
int
FindAllServers ( void )

{
	int sap, n, i, j, fd;
	int	dupflag;

	DPRINTF(( stderr, "%s: (FindAllServers) Entry\n", titleStr));
	DPRINTF(( stderr, "%s: (FindAllServers) \n\n\n\n", titleStr));

	for(i=0; allSapsToTry[i].sap != 0; i++) {
		sapsToTry[i] = allSapsToTry[i];
	}

	i=0;
	do {
		/*
		 *	We have seen cases where we get a packet on the next
		 *	frame type from the previous frame type even though
		 *	it is unbound and closed.  We use the unique socket
		 *	number to identify if this packet was sent on this frame type
		 */
		socket++;	/* Get a unique socket number for this frame type */
	    for( j=0; j < gnsRetries; j++) {
		if ((sap = sapsToTry[i].sap) == SAP_EXCLUDE)
			break;  /* from retry loop */

		if( ( fd=open(networkDevice, O_RDWR)) <0) {
			fprintf(stderr, MsgGetStr(NWD_OPEN), networkDevice);
			perror("");
			return( FAILURE);
		}

		if ( dlpiInfo ( fd ) == FAILURE )
			return ( FAILURE );

		if ( tokenRing ) {
			if ( sap == BIND_8022 || sap == BIND_ETHERNET2 || 
					sap == BIND_SNAP || sap == BIND_8023 ) {
				close(fd);
				break;
			}	
		} else {
			if ( sap == BIND_8025 || sap == BIND_8025_SNAP ) {
				close(fd);
				break;
			}
		}

		if( bindSap( fd, sap) != SUCCESS) {
			close(fd);
			break; /* from the retry loop */
		}

		/*
		 * send SAP and/or RIP request 
		 */
		if((SendGNS( fd, sap, (caddr_t)sapsToTry[i].name) == SUCCESS) ||
		   (SendRIP( fd) == SUCCESS)) {
			/*
			 * UPDATE CONFIG FILE with the following info...
			 */
			lanInstance++;
			if( concise) {
				/*	Do not internationalize, output designed for app input */
				fprintf(stdout,"%s %s 0x%.8X %d\n", sap2text(sap),
					networkDevice, ourNetwork, lanInstance);
			} else {
				fprintf(stdout, MsgGetStr(NWD_GOT_NET), lanInstance,
					sap2text(sap), networkDevice, ourNetwork);
			}
			n = 0;
			dupflag = FALSE;
			while( networks[n] != 0 ) {
				if( networks[n] == ourNetwork) {
					fprintf(stderr, MsgGetStr(NWD_DUP_NET),
						lanInstance, ourNetwork, networkDevice, sap2text(sap));
					dupflag = TRUE;
					lanInstance--;
				}
				n++;
			}
			/*
			 * Since some frame types can not coexist with others, if frame
			 * type 802.2 * or 802 SNAP are discovered, don't discover 802.3
			 * (and vice versa).
			 *
			 * Exclude frame types only if updating the configuration file,
			 * otherwise show everthing that is out there.
			 */
			if(updateConfigFile) {
				if ((sap == BIND_8022 || sap == BIND_SNAP) && 
								(sapsToTry[3].sap != 0)) {
					sapsToTry[3].sap = SAP_EXCLUDE;
				}
				if ((sap == BIND_8023)  && (sapsToTry[0].sap != 0)) {
					sapsToTry[0].sap = SAP_EXCLUDE;
				}
				if ((sap == BIND_8023)  && (sapsToTry[2].sap != 0)) {
					sapsToTry[2].sap = SAP_EXCLUDE;
				}
			}

			if( dupflag == FALSE) {
				UpdateConfigFile(lanInstance, sapsToTry[i].sap);
				networks[n] = ourNetwork;
			}
			sapUnbind (fd);
			close(fd);
			break; /* from retry loop */
		}
		sapUnbind (fd);
		close(fd);
	    }
	} while( sapsToTry[++i].sap && (checkAllFrames || !lanInstance));

/*
**	If lanInstance is still zero, we failed
*/
	if(!lanInstance)
		return(FAILURE);
	else
		return(SUCCESS);

}

int
SendRIP ( int fd)
{
	int		flags;
	int		alarmTime;
	struct strbuf	cbuf, dbuf;
	char		recCtlBuf[256], recDataBuf[256];
	struct pollfd pfd;

	union 		DL_primitives *dl_p;

	typedef struct {
		uint8   targetNet[4];
		uint8   targetHops[2];
		uint8	targetTime[2];
	} routeEntry_t;

	typedef struct {
		ipxHdr_t	header;
		uint8		operation[2];
		routeEntry_t routeTable[1];
	} routePacket_t;
	routePacket_t	*route_p;

	routePacket_t routePacketData = {
		{	IPX_CHKSUM,					/* checksum FFFF if not done */
			0x00,						/* length of data and ipx header */
			IPX_TRANSPORT_CONTROL,		/* transport control */
			IPX_RIP_PACKET_TYPE,		/* packet type */
					{					/* destination address */
			   {0x00, 0x00, 0x00, 0x00},
			   {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
			   {0x04, 0x53}
			},
					{					/* source address */
			   {0x00, 0x00, 0x00, 0x00},
			   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
			   {0xFF, 0xFF}				/* To be filled in */
			},
		},
		0x00, 0x01,
		{
			0xff, 0xff, 0xff, 0xff,	/* Rip get all nets */
			0x00, 0x00,
			0x00, 0x00
		}
	};
	
	struct strbuf ripPacket; 


	PPUTINT16( sizeof(routePacketData), &routePacketData.header.len);

	memcpy( routePacketData.header.src.node,
	&((getNearestServer_t *)gnsData.buf)->header.src.node, IPX_NODE_SIZE);

	if ( (ethernet_SNAP) || (token_ring_SNAP) ) {
		getNearestServerRequest.lsap = 0xAA;
		getNearestServerRequest.req.dl_dest_addr_length = 8;
	} else {
		if ( (ethernet_8022) || (token_ring) ) {
			getNearestServerRequest.lsap = 0xE0;
			getNearestServerRequest.req.dl_dest_addr_length = 7;

		} else {
			getNearestServerRequest.lsap = 0;
			getNearestServerRequest.req.dl_dest_addr_length = 8;
		}
	}
	
	alarmTime = gnsAlarmTime;
	if ( (token_ring) || (token_ring_SNAP) ) {
		if (alarmTime < 3000) {
			alarmTime = 3000;
		}
	}

	flags = 0;
	ripPacket.maxlen = sizeof(routePacketData);
	ripPacket.len = sizeof(routePacketData);
	ripPacket.buf = (char *) &routePacketData;

	DPRINTF(( stderr, "%s: (SendRIP) Entry\n", titleStr));
	DPRINTF((stderr, "%s: RIP size %d, maxlen=%d, len=%d\n",
		titleStr, sizeof (routePacketData), ripPacket.maxlen, ripPacket.len));

	PPUTINT16( socket, routePacketData.header.src.sock);
	DPACKET( debugOutPkt, &gnsControl, "RIP DLPI Header: ");
	DIPXPACKET( debugOutPkt, &ripPacket, "RIP Data: ");
	if (putmsg(fd, &gnsControl, &ripPacket, flags)<0) {
		fprintf(stderr,MsgGetStr(NWD_RIP_FAIL));
		perror("");
		return( FAILURE);
	}

	dbuf.len    = 
	dbuf.maxlen = sizeof(recDataBuf);
	dbuf.buf    = recDataBuf;

	cbuf.len    = 
	cbuf.maxlen = sizeof(recCtlBuf);
	cbuf.buf    = recCtlBuf;

	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	flags = 0;
	for (;;) {
		if( poll( &pfd, 1, alarmTime) < 0) {
			DPRINTF(( stderr, "%s: <poll failure>: ", titleStr));
			PERROR("");
			return( FAILURE);
		}

		if( (pfd.revents & POLLIN) == 0) {
			if( verbose) {
				DPRINTF(( stderr, "%s: <timeout>\n", titleStr));
			}
			return( FAILURE);
		}

		if( getmsg( fd, &cbuf, &dbuf, &flags) <0) {
			DPRINTF(( stderr, "%s: <getmsg failure>: ", titleStr));
			PERROR("");
			return( FAILURE);
		}

		dl_p = (union DL_primitives *) cbuf.buf;
		if( dl_p->dl_primitive == DL_UNITDATA_IND) {


			DPRINTF((stderr, "%s: (SendRIP) got DL_UNITDATA_IND\n", titleStr));
			route_p = (routePacket_t *) dbuf.buf;
			if((GETINT16(route_p->header.chksum) == IPX_CHKSUM) && 
			(!IPXCMPNET(route_p->header.src.net,routePacketData.header.src.net))){
				if( PGETINT16( route_p->header.dest.sock) == socket) {
					DPACKET( debugInPkt, &cbuf, "Input DL Header: ");
					DIPXPACKET( debugInPkt, &dbuf, "Input Data: ");
					alarm( 0);
					ourNetwork = PGETINT32( route_p->header.src.net);
					return( SUCCESS);
				} else {
					DPRINTF((stderr,"Detected bogus net 0x%x\n", PGETINT32( route_p->header.src.net)));
				}
			}
		}
	}
}

void
UpdateConfigFile(int lanInstance, uint16 sap)
{
	int rval;
	int ppaVal = 0;
	char NWCMTokenName[50];

	if (updateConfigFile == FALSE)
		return;
	if (verbose) {
		fprintf(stdout, MsgGetStr(NWD_UPD_CFG),lanInstance, networkDevice,
			ourNetwork,sap2text(sap));
	}

	sprintf(NWCMTokenName, "lan_%d_adapter", lanInstance);
	if ((rval = NWCMSetParam(NWCMTokenName, NWCP_STRING, networkDevice)) != NWCM_SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(rval, MsgGetStr(NWD_CFG_X), NWCMTokenName);
	}

	sprintf(NWCMTokenName, "lan_%d_if_name", lanInstance);
	if ((rval = NWCMSetToDefault(NWCMTokenName)) != NWCM_SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(rval, MsgGetStr(NWD_CFG_X), NWCMTokenName);
	}

	sprintf(NWCMTokenName, "lan_%d_frame_type", lanInstance);
	if ((rval = NWCMSetParam(NWCMTokenName, NWCP_STRING, sap2text(sap))) != NWCM_SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(rval, MsgGetStr(NWD_CFG_X), NWCMTokenName);
	}

	sprintf(NWCMTokenName, "lan_%d_network", lanInstance);
	if ((rval = NWCMSetParam(NWCMTokenName, NWCP_INTEGER, &ourNetwork)) != NWCM_SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(rval, MsgGetStr(NWD_CFG_X), NWCMTokenName);
	}

	sprintf(NWCMTokenName, "lan_%d_ppa", lanInstance);
	if ((rval = NWCMSetParam(NWCMTokenName, NWCP_INTEGER, &ppaVal)) != NWCM_SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(rval, MsgGetStr(NWD_CFG_X), NWCMTokenName);
	}

	sprintf(NWCMTokenName, "lan_%d_adapter_type", lanInstance);
    if (tokenRing) {
        rval = NWCMSetParam(NWCMTokenName, NWCP_STRING, "TOKEN-RING_DLPI");
    }else {
        rval = NWCMSetParam(NWCMTokenName, NWCP_STRING, "ETHERNET_DLPI");
    }
	if (rval != NWCM_SUCCESS) {
		fprintf(stderr, "\n");
		NWCMPerror(rval, MsgGetStr(NWD_CFG_X), NWCMTokenName);
	}

	return;
}
int
ConfigFileReset(int Lan)
{
	char NWCMTokenName[50];

	if (!updateConfigFile)
		return(0);

	for (;;)
	{
		sprintf(NWCMTokenName, "lan_%d_frame_type", Lan);
		if (NWCMSetToDefault(NWCMTokenName) != NWCM_SUCCESS)
			return(0);

		sprintf(NWCMTokenName, "lan_%d_network", Lan);
		if (NWCMSetToDefault(NWCMTokenName) != NWCM_SUCCESS)
			return(0);

		sprintf(NWCMTokenName, "lan_%d_ppa", Lan);
		if (NWCMSetToDefault(NWCMTokenName) != NWCM_SUCCESS)
			return(0);

		sprintf(NWCMTokenName, "lan_%d_adapter_type", Lan);
		if (NWCMSetToDefault(NWCMTokenName) != NWCM_SUCCESS)
			return(0);

		sprintf(NWCMTokenName, "lan_%d_adapter", Lan);
		if (NWCMSetToDefault(NWCMTokenName) != NWCM_SUCCESS)
			return(0);
		Lan++;
	}
}

int
bindSap ( int fd, uint16 sap)

{
	int		flags;
	struct strbuf	cbuf;
	int		rval;
	dl_bind_req_t	bind;
	char		recCtlBuf[256];

	dl_bind_ack_t	*bindAck;
#ifdef DEBUG
	dl_error_ack_t	*errorAck;
#endif

	int		csmacdmode;
	struct strioctl	ioc;

	DPRINTF(( stderr, "%s: (bindSap) Entry\n",titleStr));
	DPRINTF(( stderr, "%s: (bindSap) sap=0x%x \n",titleStr, sap));
	if( verbose) {
		fprintf(stdout, MsgGetStr(NWD_TRY), sap2text( sap), networkDevice);
	}

	switch ((unsigned long) sap) {
		case BIND_ETHERNET2: {
			bind.dl_sap = (unsigned long)sap;
			ethernet_8023 = FALSE;
			ethernet_8022 = FALSE;
			ethernet_SNAP = FALSE;
			token_ring = FALSE;
			token_ring_SNAP = FALSE;

			ethernet_II = TRUE;
			break;
		}
		case BIND_8023: {
			bind.dl_sap = 0xE0;
			ethernet_II = FALSE;
			ethernet_8022 = FALSE;
			ethernet_SNAP = FALSE;
			token_ring = FALSE;
			token_ring_SNAP = FALSE;

			ethernet_8023 = TRUE;
			break;
		}
		case BIND_8022: {
			bind.dl_sap = 0xE0;
			ethernet_II = FALSE;
			ethernet_8023 = FALSE;
			ethernet_SNAP = FALSE;
			token_ring = FALSE;
			token_ring_SNAP = FALSE;

			ethernet_8022 = TRUE;
			break;
		}
		case BIND_SNAP: {
			bind.dl_sap = 0xAA;
			ethernet_II = FALSE;
			ethernet_8023 = FALSE;
			ethernet_8022 = FALSE;
			token_ring = FALSE;
			token_ring_SNAP = FALSE;

			ethernet_SNAP = TRUE;
			break;
		}
		case BIND_8025: {
			bind.dl_sap = 0xE0;
			ethernet_II = FALSE;
			ethernet_8023 = FALSE;
			ethernet_8022 = FALSE;
			ethernet_SNAP = FALSE;
			token_ring_SNAP = FALSE;

			token_ring = TRUE;
			break;
		}
		case BIND_8025_SNAP: {
#ifdef NW_UP
			bind.dl_sap = 0xAA;
#else
			bind.dl_sap = BIND_ETHERNET2;
#endif
			ethernet_II = FALSE;
			ethernet_8023 = FALSE;
			ethernet_8022 = FALSE;
			ethernet_SNAP = FALSE;
			token_ring = FALSE;

			token_ring_SNAP = TRUE;
			break;
		}
		default:
			break;
	}

	/*
	 *	Do the BIND
	 */

	bind.dl_primitive    = DL_BIND_REQ;
	bind.dl_max_conind   = 0;
	bind.dl_service_mode = DL_CLDLS;
	bind.dl_conn_mgmt    = 0;

	cbuf.len    = cbuf.maxlen = sizeof(dl_bind_req_t);
	cbuf.buf    = (char *)&bind;

	flags=0;
	DPACKET( debugOutDl, &cbuf, "Bind request: ");
	if (putmsg(fd, &cbuf, NULL, flags) <0) {
		fprintf(stderr,MsgGetStr(NWD_BIND_REQ));
		perror("");
		close( fd);
		return( FAILURE);
	}

	/*
	 *	Make sure our bind request is ACKed.
	 */

	cbuf.len    = cbuf.maxlen = sizeof(recCtlBuf);
	cbuf.buf    = recCtlBuf;

	flags = 0;

	rval = getmsg(fd, &cbuf, NULL, &flags);
	if (rval < 0) {
		fprintf(stderr, MsgGetStr(NWD_BIND_ACK));
		perror("");
		close( fd);
		return( FAILURE);
	}
	DPACKET( debugInDl, &cbuf, "Bind ACK: ");

	bindAck = (dl_bind_ack_t *)recCtlBuf;

	if (bindAck->dl_primitive != DL_BIND_ACK) {
		fprintf(stderr, MsgGetStr(NWD_BIND_RESP));
#ifdef DEBUG
		errorAck = (dl_error_ack_t *) recCtlBuf;
#endif
		DPRINTF((stderr,"%s: dl_errno %d dl_unix_errno %d\n",
			titleStr, errorAck->dl_errno, errorAck->dl_unix_errno));
		close( fd);
		return( FAILURE);
	}

	/*
	 *	Put our node address in the GNS request
	 */
	memcpy(&((getNearestServer_t *)gnsData.buf)->header.src.node,
		&recCtlBuf[bindAck->dl_addr_offset], IPX_NODE_SIZE);

	DPRINTF((stderr,"%s: dl_primitive %d dl_sap 0x%X\n", 
		titleStr, bindAck->dl_primitive, bindAck->dl_sap));

	/*	Flush the remaining data or control info if it was not
	 *	retreived by the earlier getmsg call  
	 *
	 */
	while ((rval == MORECTL) | (rval == MOREDATA)) {
		rval = getmsg(fd, &cbuf, &cbuf, &flags);
	}


	switch ((unsigned long) sap) {
		case BIND_8023: {
			ioc.ic_cmd = DLIOCCSMACDMODE;
			ioc.ic_timout = 3;
			ioc.ic_len = sizeof (int);
			ioc.ic_dp = (char *) &csmacdmode;

			if (ioctl (fd, I_STR, &ioc) < 0) {
				perror ("ioctl: I_STR: DLIOCCSMACDMODE");
				sapUnbind ( fd );
				close ( fd );
				return (FAILURE);
			}
			break;
		}

#ifdef NW_UP
		case BIND_8025_SNAP:
			/* FALLTHRU */
#endif
		case BIND_SNAP: {
			dl_subs_bind_req_t      *subsBind;
			dl_subs_bind_ack_t      *subsBindAck;
			/*
			 *	Do the subsequent BIND
			 */

			DPRINTF(( stderr, "%s: (bindSap) Attempting subsequent BIND\n", titleStr));
			subsBind = (dl_subs_bind_req_t *)& recCtlBuf;

			subsBind->dl_primitive = DL_SUBS_BIND_REQ;
			subsBind->dl_subs_sap_offset= sizeof (dl_subs_bind_req_t);
			subsBind->dl_subs_sap_length= sizeof (struct snap_sap);
			memcpy(&recCtlBuf[sizeof(dl_subs_bind_req_t)],
				&snapsap, sizeof (struct snap_sap));

			cbuf.len    = 
			cbuf.maxlen = (sizeof(dl_subs_bind_req_t) + sizeof (struct snap_sap));
			cbuf.buf    = (char *)&recCtlBuf;

			flags=0;
			DPACKET( debugOutDl, &cbuf, "Subsequent Bind request: ");
			if (putmsg(fd, &cbuf, NULL, flags)<0) {
				fprintf(stderr, MsgGetStr(NWD_SUBBIND_REQ));
				perror("");
				sapUnbind ( fd );		/* unbind us! */
				close( fd);
				return( FAILURE);
			}

			/*
			 *	Make sure our bind request is ACKed.
			 */

			cbuf.len    = cbuf.maxlen	= sizeof(recCtlBuf);
			cbuf.buf    = recCtlBuf;

			flags = 0;

			rval = getmsg(fd, &cbuf, NULL, &flags);
			if (rval < 0) {
				fprintf(stderr, MsgGetStr(NWD_SUBBIND_ACK));
				perror("");
				sapUnbind ( fd );		/* unbind us! */
				close( fd);
				return( FAILURE);
			}
			DPACKET( debugInDl, &cbuf, "Subsequent Bind Ack: ");

			subsBindAck = (dl_subs_bind_ack_t *)&recCtlBuf;

			if (subsBindAck->dl_primitive != DL_SUBS_BIND_ACK) {
				fprintf(stderr, MsgGetStr(NWD_SUBBIND_RESP));
#ifdef DEBUG
				errorAck = (dl_error_ack_t *) recCtlBuf;
#endif
				DPRINTF((stderr,"%s: dl_errno %d dl_unix_errno %d\n",
					titleStr, errorAck->dl_errno, errorAck->dl_unix_errno ));
				sapUnbind ( fd );		/* unbind us! */
				close( fd);
				return( FAILURE);
			}
			DPRINTF((stderr,"%s: (bindSap) Success on subsbindack\n",titleStr));

			/*	Flush the remaining data or control info if it was not
			 *	retreived by the earlier getmsg call  -- Ram M
			 *
			 */
			while ((rval == MORECTL) | (rval == MOREDATA)) {
				rval = getmsg(fd, &cbuf, &cbuf, &flags);
			}
			break;
		}
		default:
			break;
	}
	return(SUCCESS);
}

/* ARGSUSED */
int
SendGNS ( int fd, uint16 sap, char *name)
{
	int		flags;
	int		alarmTime;
	struct strbuf	cbuf, dbuf;
	char		recCtlBuf[256], recDataBuf[256];
	struct pollfd pfd;

	union 		DL_primitives *dl_p;
	ipxHdr_t	*ipx_p;

	/*
	 *	Send the Get Nearest Server message
	 */

	DPRINTF(( stderr, "%s: (SendGNS) Entry\n",titleStr));
	DPRINTF(( stderr, "%s: (SendGNS)  sap=0x%x, \n",titleStr, sap));
	DPRINTF((stderr,"%s: GNS size %d, maxlen=%d, len=%d\n", titleStr,
		sizeof (getNearestServerRequest), gnsControl.maxlen, gnsControl.len));

	if ( (ethernet_SNAP) || (token_ring_SNAP) ) {
		getNearestServerRequest.lsap = 0xAA;
		getNearestServerRequest.req.dl_dest_addr_length = 8;
	}
	else {
		if ( (ethernet_8022) || (token_ring) ) {
			getNearestServerRequest.lsap = 0xE0;
			getNearestServerRequest.req.dl_dest_addr_length = 7;

		}
		else {
			getNearestServerRequest.lsap = 0;
			getNearestServerRequest.req.dl_dest_addr_length = 8;
		}
	}

	alarmTime = gnsAlarmTime;
	if ( (token_ring) || (token_ring_SNAP) ) {
		if (alarmTime < 3000) {
			alarmTime = 3000;
		}
	}

	flags = 0;
	DPRINTF((stderr,"%s: sending GNS request to file servers\n",titleStr));
	getNearestServerData.serverType = GETINT16(FILE_SERVER_TYPE); 
	PPUTINT16( socket, getNearestServerData.header.src.sock);
	DPACKET( debugOutPkt, &gnsControl, "GNS Request DL Header: ");
	DIPXPACKET( debugOutPkt, &gnsData, "GNS Request: ");
	if (putmsg(fd, &gnsControl, &gnsData, flags)<0) {
		fprintf(stderr, MsgGetStr(NWD_GNS_FAIL));
		perror("");
		return( FAILURE);
	}
	DPRINTF((stderr,"%s: sending GNS request to UnixWare servers\n", titleStr));
	getNearestServerData.serverType=GETINT16(UNIXWARE_TYPE);
	DPACKET( debugOutPkt, &gnsControl, "GNS Request DL Header: ");
	DIPXPACKET( debugOutPkt, &gnsData, "GNS Request: ");
	if (putmsg(fd, &gnsControl, &gnsData, flags)<0) {
		fprintf(stderr, MsgGetStr(NWD_GNS_FAIL));
		perror("");
		return( FAILURE);
	}

	dbuf.len    = 
	dbuf.maxlen = sizeof(recDataBuf);
	dbuf.buf    = recDataBuf;

	cbuf.len    = 
	cbuf.maxlen = sizeof(recCtlBuf);
	cbuf.buf    = recCtlBuf;

	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	flags = 0;
	for( ;;) {
		if( poll( &pfd, 1, alarmTime) < 0) {
			DPRINTF(( stderr, "%s: <poll failure>: ", titleStr));
			PERROR("");
			return( FAILURE);
		}

		if( (pfd.revents & POLLIN) == 0) {
			if( verbose) {
				DPRINTF(( stderr, "%s: <timeout>\n", titleStr));
			}
			return( FAILURE);
		}

		if( getmsg( fd, &cbuf, &dbuf, &flags) <0) {
			DPRINTF(( stderr, "%s: <getmsg failure>", titleStr));
			PERROR("");
			return( FAILURE);
		}

		dl_p = (union DL_primitives *) cbuf.buf;
		if( dl_p->dl_primitive == DL_UNITDATA_IND) {

			DPRINTF((stderr, "%s: (SendGNS) got DL_UNITDATA_IND\n", titleStr));
			ipx_p = (ipxHdr_t *) dbuf.buf;
			DPRINTF((stderr, "%s: <ipxChecksum=0x%X ipxLen=%d >\n", 
					titleStr, GETINT16(ipx_p->chksum), GETINT16(ipx_p->len)));
			if(GETINT16(ipx_p->chksum) == IPX_CHKSUM && 
					GETINT16(ipx_p->len) >= sizeof(ipxHdr_t)+4){
				gnsReply_t *reply = (gnsReply_t *)((char *)ipx_p+sizeof( ipxHdr_t));

				DPRINTF((stderr, "%s: <reply type=0x%X serverType=0x%X >\n", 
						titleStr, GETINT16(reply->replyType), 
						GETINT16(reply->serverType)));

				if(
				  (GETINT16(reply->replyType) == SAP_NSR && 
					GETINT16(reply->serverType) == FILE_SERVER_TYPE) 
					||
					(GETINT16(reply->replyType) == SAP_NSR && 
					GETINT16(reply->serverType) == UNIXWARE_TYPE)) {

					DPRINTF((stderr,"%s: <rcv sock=0x%X exp sock=0x%X >\n", 
							titleStr, PGETINT16(ipx_p->dest.sock), socket));
					if( PGETINT16( ipx_p->dest.sock) == socket) {
						alarm( 0);
						ourNetwork = PGETINT32( ipx_p->src.net);
						strcpy( name, reply->name);
						DPRINTF((stderr, "%s: <got reply from %s>\n", titleStr, reply->name));
						DPACKET( debugInPkt, &cbuf, "Input DL Header: ");
						DIPXPACKET( debugInPkt, &dbuf, "Input Data: ");
						return( SUCCESS);
					} else {
						DPRINTF((stderr,"Detected bogus net 0x%x\n", PGETINT32( ipx_p->src.net)));
					}
				}
			}
		}
	}
}
int
dlpiInfo ( int fd )

{
	int					flags;
	int					rval;
	dl_info_req_t		infreq;
	dl_info_ack_t		*info;
#ifdef DEBUG
	dl_error_ack_t		*errorAck;
#endif
	struct strbuf		cbuf;
	char				recCtlBuf[256];

	DPRINTF(( stderr, "%s: (dlpiInfo) Entry\n", titleStr));

	infreq.dl_primitive = DL_INFO_REQ;

	cbuf.len    = cbuf.maxlen = sizeof(dl_info_req_t);
	cbuf.buf    = (char *)&infreq;

	flags=0;
	DPACKET( debugOutDl, &cbuf, "DLPI Info Request: ");
	if (putmsg(fd, &cbuf, NULL, flags)<0) {
		fprintf(stderr,MsgGetStr(NWD_DLPI_REQ));
		perror("");
		return( FAILURE );
	}

	/*
	 *	wait for the information to come back
	 */

	cbuf.len    = cbuf.maxlen = sizeof(recCtlBuf);
	cbuf.buf    = recCtlBuf;

	flags = 0;

	rval = getmsg(fd, &cbuf, NULL, &flags);
	if (rval < 0) {
		fprintf(stderr,MsgGetStr(NWD_DLPI_ACK));
		perror("");
		return( FAILURE );
	}
	DPACKET( debugInDl, &cbuf, "DLPI Info Ack: ");

	info = (dl_info_ack_t *)recCtlBuf;

	if (info->dl_primitive != DL_INFO_ACK) {
		perror("");
		fprintf(stderr,MsgGetStr(NWD_DLPI_RESP));
#ifdef DEBUG
		errorAck = (dl_error_ack_t *) recCtlBuf;
#endif
		DPRINTF((stderr,"%s: dl_errno %d dl_unix_errno %d\n",
			titleStr, errorAck->dl_errno, errorAck->dl_unix_errno ));
		return( FAILURE);
	}

	/*
	 *	ok, we have the info structure
	 */

	switch ( info -> dl_mac_type ) {
	case DL_TPB:
	case DL_TPR:
		DPRINTF(( stderr, "%s: TOKEN RING DETECTED\n", titleStr));
		tokenRing = 1;		/* use token ring  */
		break;
	default : /* assume other == ethernet */
		DPRINTF(( stderr, "%s: (dlpiInfo) Assuming Ethernet\n", titleStr));
		tokenRing = 0;
		break;
	}
	return ( SUCCESS );
}

/*ARGSUSED*/
void
alarmSignal ( int val )

{
	return;
}


void
ExitAll ( int val )

{

	DPRINTF(( stderr, "%s: (ExitAll) Exiting with %d\n", titleStr ,val));

	unlink ("/tmp/frame.dev");
	exit(val);
}

char *
sap2text ( uint16 sap )

{
	switch ( sap ) {
		case BIND_ETHERNET2:
			return( "ETHERNET_II");

		case BIND_SNAP:
			return( "ETHERNET_SNAP");

		case BIND_8022:
			return( "ETHERNET_802.2");

		case BIND_8023:
			return( "ETHERNET_802.3");

		case BIND_8025:
			return( "TOKEN-RING");

		case BIND_8025_SNAP:
			return( "TOKEN-RING_SNAP");

		default:
			return( "(?sap?)" );
	}
}



void
sapUnbind ( int fd )

{
	int					flags;
	int					rval;
	dl_unbind_req_t		unbind;
	dl_ok_ack_t			*unbindAck;
#ifdef DEBUG
	dl_error_ack_t		*errorAck;
#endif
	struct strbuf		cbuf;
	char				recCtlBuf[256];

	DPRINTF(( stderr, "%s: (sapUnbind) Entry\n", titleStr));

	unbind.dl_primitive = DL_UNBIND_REQ;

	cbuf.len    = cbuf.maxlen = sizeof(dl_unbind_req_t);
	cbuf.buf    = (char *)&unbind;

	flags=0;
	DPACKET( debugOutDl, &cbuf, "Unbind Request: ");
	if (putmsg(fd, &cbuf, NULL, flags)<0) {
		fprintf(stderr,MsgGetStr(NWD_UNB_REQ));
		perror("");
	}

	/*
	 *	Make sure our unbind request is ACKed.
	 */

	cbuf.len    = cbuf.maxlen = sizeof(recCtlBuf);
	cbuf.buf    = recCtlBuf;

	flags = 0;

	rval = getmsg(fd, &cbuf, NULL, &flags);
	if (rval < 0) {
		fprintf(stderr,MsgGetStr(NWD_UNB_ACK));
		perror("");
	}
	DPACKET( debugInDl, &cbuf, "Unbind Ack: ");

	unbindAck = (dl_ok_ack_t *)recCtlBuf;

	if (unbindAck->dl_primitive != DL_OK_ACK) {
		fprintf(stderr,MsgGetStr(NWD_UNB_RESP));
#ifdef DEBUG
		errorAck = (dl_error_ack_t *) recCtlBuf;
#endif
		DPRINTF((stderr,"%s: dl_errno %d dl_unix_errno %d\n",
			titleStr, errorAck->dl_errno, errorAck->dl_unix_errno ));
	}
}

