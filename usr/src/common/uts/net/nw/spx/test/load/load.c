/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/load/load.c	1.4"
#ident	"$Id: load.c,v 1.4 1994/09/06 21:28:52 meb Exp $"

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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <poll.h>
#include <tiuser.h>
#include <stdio.h>
#include <errno.h>

#include <sys/nwtdr.h>
#include <sys/nwportable.h>
#include <sys/ipx_app.h>
#include <sys/spx_app.h>


#define MAX_PACKET_DATA 16384
#define MAX_WAIT_TIME 5000          /* max wait time */

extern int errno;
extern int t_errno;

char *spxDev = "/dev/nspx2";
char *ipxDev = "/dev/ipx";

unsigned int packetSize=MAX_PACKET_DATA;
unsigned int minutes=0;
unsigned int waitTime=0;
uint16 socketToBind = 0;
uint32 destNet = 0x01010393;
uint8  destNode[IPX_NODE_SIZE] = {00,00,00,00,00,01};
/*filler for scanf, must be after node structure*/
uint8  extraNode[IPX_NODE_SIZE] = {0xff,0xff,0xff,0xff,0xff,0xff};
uint16 destSocket = 0xDEAD;
int useSPX = TRUE;
int verbose = FALSE;
int	compareData = TRUE;
int ipxChecksums = FALSE;
int incrementData = FALSE;
int testing;
int sendEND;
char titleStr[100];

void
ShowOptionsExit()
{
	fprintf(stderr,"%s: usage: %s",titleStr,titleStr);
	fprintf(stderr,"  [-I] [-v] [-w wait] [-m minutes] [-a<dest Net Addr>] [-n<dest Node Addr>] [-s<dest Socket>] [-p<packet size>] \n");
	fprintf(stderr,"%s       -I Use IPX transoprt\n");
	fprintf(stderr,"%s       -v verbose mode\n");
	fprintf(stderr,"%s       -w wait number of milliseconds to wait between writes\n");
	fprintf(stderr,"%s       -m minutes number of minutes to run \n");
	fprintf(stderr,"%s       -a dest Net Addr: server IPX Net Address override\n");
	fprintf(stderr,"%s       -n dest Node Addr: server IPX Net Address override\n");
	fprintf(stderr,"%s       -s dest Socket: server socket number override\n");
	fprintf(stderr,"%s       -p packet size: number of data bytes sent per tli call\n");
	fprintf(stderr,"%s       -C use IPX checksum on IPX and SPX data\n");
	fprintf(stderr,"%s       -U use incrementing pattern for data \n");

	exit(1);
}


int
RecieveDisconnect(fd)
	int fd;
{
	
	struct t_discon disconInfo;
	int retval;

	if ((retval=t_look(fd))<0) {
		fprintf(stderr,"%s: t_look failed \n",titleStr);
		t_error("");
		return(-1);
	}

	if (retval != T_DISCONNECT) {
		fprintf(stderr,"%s: no disconnect indication on the stream head \n",
			titleStr);
		return(-1);
	}

	/*
		Set up the receive disconnect structure 
	*/
	disconInfo.udata.len = 0;
	disconInfo.udata.maxlen = 0;
	disconInfo.udata.buf = (char *)NULL;
	disconInfo.reason = 0;
	disconInfo.sequence = 0;

	if (t_rcvdis(fd, &disconInfo)<0) {
		fprintf(stderr,"%s: t_rcvdis failed \n",titleStr);
		t_error("");
		return(-1);
	}

	if (verbose)
	switch (disconInfo.reason) {
		case TLI_SPX_CONNECTION_FAILED :
			fprintf(stderr,"%s: connection failed \n",titleStr);
			break;
		case TLI_SPX_CONNECTION_TERMINATED :
			fprintf(stderr,"%s: connection terminated by remote endpoint \n",
				titleStr);
			break;
		default:
			fprintf(stderr,"%s: default disconnect reason 0x%04X \n",
				titleStr, disconInfo.reason);
			break;
	}

	return (0);
}

int 
loadError(fd)
	int fd;
{
	int retval;

	fprintf(stderr,"loadError: errno= %d t_errno= %d\n",errno, t_errno);

	if (t_errno != TLOOK) return;

	if ((retval=t_look(fd))<0) {
		fprintf(stderr,"%s: t_look failed \n",titleStr);
		t_error(""); 
		return -1;
	}

	if (verbose)
	switch (retval) {
		case T_LISTEN :
			fprintf(stderr,"%s: listen indication arrived \n",titleStr);
			break;
		case T_CONNECT :
			fprintf(stderr,"%s: connect indication arrived \n",titleStr);
			break;
		case T_DATA :
			fprintf(stderr,"%s: data indication arrived \n",titleStr);
			break;
		case T_EXDATA :
			fprintf(stderr,"%s: expedited data indication arrived \n",titleStr);
			break;
		case T_DISCONNECT :
			fprintf(stderr,"%s: disconnect indication arrived \n",titleStr);
			RecieveDisconnect(fd);
			break;
		case T_ERROR :
			fprintf(stderr,"%s: fatal error arrived \n",titleStr);
			break;
		default :
			fprintf(stderr,"%s: TLook default 0x%X \n",titleStr,retval);
			break;
	}

	return retval;
}

void 
ScanArgs( argc, argv, optstr )
	int argc;
	char **argv;
	char *optstr;
{
	extern char *optarg;
	extern int optind, opterr;
	int socketNumber;
	char c;

	opterr =0;
	while ((c = getopt(argc, argv, optstr )) != -1 ) {
		switch (c) {
			case 'I' :
				useSPX = FALSE;
				break;
			case 'a' :
				sscanf(optarg,"%Lx",&destNet);
				break;
			case 'n' :
				 if (strlen(optarg) != 12) {
					fprintf(stderr,"Bad length for IPX node\n");
					ShowOptionsExit();
					break;
				}
				sscanf(optarg,"%2hx %2hx %2hx %2hx %2hx %2hx",&destNode[0],
						&destNode[1], &destNode[2], &destNode[3], 
						&destNode[4], &destNode[5]);
				break;
			case 'U' :
				incrementData = TRUE;
				break;
			case 'C' :
				ipxChecksums = TRUE;
				break;
			case 'm' :
				sscanf(optarg,"%d",&minutes);
				break;
			case 's' :
				sscanf(optarg,"%hx",&destSocket);
				break;
			case 'p' :
				sscanf(optarg,"%d",&packetSize);
				if (packetSize>MAX_PACKET_DATA) 
					packetSize = MAX_PACKET_DATA;
				break;
			case 'v' :
				verbose = TRUE;
				break;
            case 'w' :
                sscanf(optarg,"%d",&waitTime);
                if (waitTime>MAX_WAIT_TIME)
                    waitTime = MAX_WAIT_TIME;
                break;
			default:
				fprintf(stderr,"%s: invalid option %c\n",titleStr,c);

			case '?' :
				ShowOptionsExit();
				break;
			}
	}
}
void
milsleep(int waitTime)
{
    struct pollfd   fds[1];

    if (waitTime == 0)
        return;

    fds[0].fd = 1;
    fds[0].events = POLLPRI;
    poll(fds, 1, waitTime);
    return;
}

void
SendEnd(int signo)
{
	if (verbose) 
		fprintf(stderr,"SIGALRM: Start sending Terminate packets!!\n");
	sendEND = 1;
}
void
QuitThis(int signo)
{
	if (verbose) 
		fprintf(stderr,"Quit with signal %d\n",signo);
	sendEND = 1;
}

main(argc, argv)
	int argc;
	char **argv;

{ 
	char 				Device[50];
	int 				fd, i, j;
	int 				flags;
	struct t_info		*info_req;
	struct t_bind		*bind_req;
	struct t_call		*call = NULL;
	struct t_unitdata	*sud = NULL;
	struct t_optmgmt	*opts;
	SPX2_OPTIONS		*SpxIIOpts;
	ipxAddr_t			serversAddress;
	ipxAddr_t			*ipxAddr;
	unsigned char 		lastData;
    unsigned char		sndbuf[MAX_PACKET_DATA];
	int					sendEndCount;

	strcpy(titleStr, argv[0]);

	ScanArgs(argc, argv, "ICUvcm:a:n:s:p:w:");
	/*
	 * convert Net Node Socket to NetOrder
	 */
	destNet = GETINT32(destNet);
	destSocket = GETINT16(destSocket);

	if (useSPX) {
		strcpy(Device, spxDev);
		if (verbose)
			fprintf(stderr,"%s: Using SPX transport\n", titleStr);
	} else {
		strcpy(Device, ipxDev);
		if (verbose)
			fprintf(stderr,"%s: Using IPX transport\n", titleStr);
	}

	if ((fd=t_open(Device,O_RDWR, NULL)) <0) {
		fprintf(stderr,"%s: open of %s failed \n",
			titleStr,Device);
		loadError(fd);
		exit(-1);
	}

	if ((info_req = (struct t_info *)t_alloc(fd, T_INFO, T_ALL)) == NULL) {
		fprintf(stderr,"t_alloc of %s failed t_errno= %d errno= %d\n",
			Device, t_errno, errno);
		t_error("t_alloc of T_INFO failed");
		exit(-1);
	}

	if (t_getinfo(fd, info_req)<0) {
		fprintf(stderr,"%s: t_getinfo failed \n", titleStr);
		loadError(fd);
		goto quit;
	}
	if (packetSize > info_req->tsdu)
		packetSize = info_req->tsdu;

	t_free((char *)info_req, T_INFO);

	if (verbose) 
        fprintf(stderr,"%s: Endpoint using data size %d for transmitting\n",
			titleStr,packetSize);
	
	if ((bind_req = (struct t_bind *)t_alloc(fd, T_BIND, T_ALL)) == NULL) {
		fprintf(stderr,"t_alloc of %s failed t_errno= %d errno= %d\n",
			Device, t_errno, errno);
		t_error("t_alloc of T_BIND failed");
		exit(-1);
	}

	bind_req->qlen = 0;  
	bind_req->addr.len = sizeof(ipxAddr_t);
	ipxAddr = (ipxAddr_t *)bind_req->addr.buf;
	/* give us a dynamic socket number */
	ipxAddr->sock[0] = 0;
	ipxAddr->sock[1] = 0;

	if (t_bind(fd, bind_req, bind_req)<0) {
		fprintf(stderr,"%s: t_bind failed \n", titleStr);
		loadError(fd);
		goto quit;
	}

	if (verbose) {
		ipxAddr = (ipxAddr_t *)bind_req->addr.buf;
		fprintf(stderr,"\tBound to address:\n\t net     0x%02X%02X%02X%02X\n",
			ipxAddr->net[0],ipxAddr->net[1],ipxAddr->net[2],ipxAddr->net[3]);
		fprintf(stderr,"\t node    0x%02X%02X%02X%02X%02X%02X\n",
			ipxAddr->node[0],ipxAddr->node[1],ipxAddr->node[2],
			ipxAddr->node[3],ipxAddr->node[4],ipxAddr->node[5]);
		fprintf(stderr,"\t socket  0x%02X%02X\n",
			ipxAddr->sock[0],ipxAddr->sock[1]);
	}
	t_free((char *)bind_req, T_BIND);

	memcpy(&serversAddress.net[0], &destNet, sizeof(destNet));
	memcpy(&serversAddress.node[0], &destNode, sizeof(destNode));
	memcpy(&serversAddress.sock[0], &destSocket, sizeof(destSocket));
	
	if (useSPX) {
		if((opts = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, T_ALL))==NULL) {
			fprintf(stderr,"t_alloc of %s failed t_errno= %d errno= %d\n",
				Device, t_errno, errno);
			t_error("t_alloc of T_OPTMGMT failed");
			exit(-1);
		}
		/*
		 * get Default options
		 */
		opts->flags = T_DEFAULT;
		opts->opt.len = opts->opt.maxlen;
		if ((t_optmgmt(fd, opts, opts))<0) {
			fprintf(stderr,"%s: t_optmgmt T_DEFAULT failed \n", titleStr);
			loadError(fd);
			goto quit;
		}
		SpxIIOpts = (SPX2_OPTIONS *)opts->opt.buf;
		if (verbose) {
			fprintf(stderr,"%s: T_DEFAULT options are:  \n",titleStr);
			fprintf(stderr,"\tversionNumber            %Xh\n",
				SpxIIOpts->versionNumber);
			fprintf(stderr,"\tspxIIOptionNegotiate     %Xh\n",
				SpxIIOpts->spxIIOptionNegotiate);
			fprintf(stderr,"\tspxIIRetryCount          %d\n",
				SpxIIOpts->spxIIRetryCount);
			fprintf(stderr,"\tspxIIMinimumRetryDelay   %d\n",
				SpxIIOpts->spxIIMinimumRetryDelay);
			fprintf(stderr,"\tspxIIMaximumRetryDelta   %d\n",
				SpxIIOpts->spxIIMaximumRetryDelta);
			fprintf(stderr,"\tspxIIWatchdogTimeout     %d\n",
				SpxIIOpts->spxIIWatchdogTimeout);
			fprintf(stderr,"\tspxIIConnectionTimeout   %d\n",
				SpxIIOpts->spxIIConnectionTimeout);
			fprintf(stderr,"\tspxIILocalWindowSize     %d\n",
				SpxIIOpts->spxIILocalWindowSize);
			fprintf(stderr,"\tspxIISessionFlags        %Xh\n",
				SpxIIOpts->spxIISessionFlags);
		}
		SpxIIOpts->spxIIRetryCount = 12;
		opts->flags = T_CHECK;
		if ((t_optmgmt(fd, opts, opts))<0) {
			fprintf(stderr,"%s: t_optmgmt T_CHECK failed \n", titleStr);
			loadError(fd);
			goto quit;
		}
		if (opts->flags != T_SUCCESS) {
			fprintf(stderr,"%s: t_optmgmt T_CHECK returned T_FAILURE\n",
				titleStr);
			if (verbose) {
				fprintf(stderr,"%s: BAD options from T_CHECK are:\n",titleStr);
				fprintf(stderr,"\tversionNumber            %Xh\n",
					SpxIIOpts->versionNumber);
				fprintf(stderr,"\tspxIIOptionNegotiate     %Xh\n",
					SpxIIOpts->spxIIOptionNegotiate);
				fprintf(stderr,"\tspxIIRetryCount          %d\n",
					SpxIIOpts->spxIIRetryCount);
				fprintf(stderr,"\tspxIIMinimumRetryDelay   %d\n",
					SpxIIOpts->spxIIMinimumRetryDelay);
				fprintf(stderr,"\tspxIIMaximumRetryDelta   %d\n",
					SpxIIOpts->spxIIMaximumRetryDelta);
				fprintf(stderr,"\tspxIIWatchdogTimeout     %d\n",
					SpxIIOpts->spxIIWatchdogTimeout);
				fprintf(stderr,"\tspxIIConnectionTimeout   %d\n",
					SpxIIOpts->spxIIConnectionTimeout);
				fprintf(stderr,"\tspxIILocalWindowSize     %d\n",
					SpxIIOpts->spxIILocalWindowSize);
				fprintf(stderr,"\tspxIISessionFlags        %Xh\n",
					SpxIIOpts->spxIISessionFlags);
			}
		}
		t_free((char *)opts, T_OPTMGMT);


		if((call = (struct t_call *)t_alloc(fd, T_CALL, T_ALL))==NULL) {
			fprintf(stderr,"t_alloc of %s failed t_errno= %d errno= %d\n",
				Device, t_errno, errno);
			t_error("t_alloc of T_CALL failed");
			exit(-1);
		}

		if (verbose) {
			fprintf(stderr,"%s: connecting to Server at:  ",titleStr);
			for (i=0; i<IPX_NET_SIZE; i++) {
				fprintf(stderr,"%02X",serversAddress.net[i]);
			}
			fprintf(stderr,"h ");
			for (i=0; i<IPX_NODE_SIZE; i++)  {
				fprintf(stderr,"%02X",serversAddress.node[i]);
			}
			fprintf(stderr,"h ");
			fprintf(stderr," %02X" ,serversAddress.sock[0]);
			fprintf(stderr,"%02Xh\n" ,serversAddress.sock[1]);
		}

		call->addr.buf = (char *)&serversAddress;
		call->addr.len = call->addr.maxlen;

        if (ipxChecksums) {
			call->opt.len = call->opt.maxlen;
            SpxIIOpts = (SPX2_OPTIONS *)call->opt.buf;
            SpxIIOpts->versionNumber = OPTIONS_VERSION;
            SpxIIOpts->spxIIRetryCount = 12;
            SpxIIOpts->spxIISessionFlags = SPX_SF_IPX_CHECKSUM;
        }


		if ((t_connect(fd, call, call))<0) {
			fprintf(stderr,"%s: t_connect failed \n", titleStr);
			loadError(fd);
			goto quit;
		}
		t_free((char *)call, T_CALL);

	} else { 	/* end of if useSPX */
		if((sud = (struct t_unitdata *)t_alloc(fd, T_UNITDATA, T_ALL))==NULL) {
			fprintf(stderr,"t_alloc of %s failed t_errno= %d errno= %d\n",
				Device, t_errno, errno);
			t_error("t_alloc of T_UNITDATA failed");
			exit(-1);
		}
		sud->addr.len = sud->addr.maxlen;
		sud->addr.buf = (char *)&serversAddress;
		sud->udata.len = sud->udata.maxlen = packetSize;
		sud->udata.buf = (char *)&sndbuf;
		if (ipxChecksums) {
			sud->opt.len = 3;
			sud->opt.buf[0] = 0xBD;	/* IPX packet type*/
			*((uint16 *)&sud->opt.buf[1]) = IPX_CHKSUM_TRIGGER;
		} else {
			sud->opt.len = 1;
			sud->opt.buf[0] = 0xF0;	/* IPX packet type*/
		}
	}

	signal( SIGINT, QuitThis );
	signal( SIGQUIT, QuitThis );
	signal( SIGALRM, SendEnd );
	testing = 1;
	sendEND = 0;
	sendEndCount = 0;
	lastData = 0x0;
	if (minutes) {
		alarm( (minutes * 60) );
	}
    if (verbose) {
		if (minutes) 
        	fprintf(stderr,"%s: Will send Data for %d minutes\n", 
				titleStr,minutes);
        fprintf(stderr,"%s: Start sending data ...\n", titleStr);
        system("date");
    }
	if (ipxChecksums) {
		fprintf(stderr,"Using IPX Checksums\n");
	}		
    while (testing) {
		if (incrementData) {
			for (i=0; i<packetSize; i++, lastData++)
				sndbuf[i] = lastData;
		} else {
			memset(&sndbuf[0], lastData, packetSize);
        	lastData++;
		}
		if (sendEND) {
			if (verbose)
				fprintf(stderr,"%s Send Terminate packet\n", titleStr);
			sndbuf[0] = (unsigned char)'e';
			sndbuf[1] = (unsigned char)'n';
			sndbuf[2] = (unsigned char)'d';
			packetSize = 10;
			sendEndCount++;
		}
		if (sendEndCount > 2)
			testing = 0;
        if (useSPX) {
    	    flags = T_MORE;
            if ((t_snd( fd, (char *)sndbuf, packetSize, flags))<0) {
				if (errno == EINTR)
					continue;
				fprintf(stderr,"%s t_snd failed \n", titleStr);
				loadError(fd);
				break;
            }
        } else {
			if (t_sndudata(fd, sud)<0) {
					fprintf(stderr,"%s t_sndudata failed \n", titleStr);
					loadError(fd);
					break;
			}
        }
        if (waitTime) {
            milsleep(waitTime);
        }

    } /* end of while loop */

	if (verbose) {
		system("date");
		fprintf(stderr,"\n%s All Done \n", titleStr);
	}
	if (sud) {
		t_free((char*)sud, T_UNITDATA);
	}

	quit:

	if (t_close(fd)<0) {
		fprintf(stderr,"%s: t_close failed \n", titleStr);
		loadError(fd);
	}
}
