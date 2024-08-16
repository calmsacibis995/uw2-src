/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/load/load_d.c	1.4"
#ident	"$Id: load_d.c,v 1.5 1994/09/06 21:29:39 meb Exp $"

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
#define MAX_WAIT_TIME 5000			/* max wait time */

extern int errno;
extern int t_errno;

char *spxDev = "/dev/nspx2";
char *ipxDev = "/dev/ipx";

unsigned int packetSize=MAX_PACKET_DATA;
unsigned int waitTime=0;
uint16 socketToBind = 0xDEAD;
int verbose = FALSE;
int compareData = TRUE;
int uniqueData = FALSE;
int ipxChecksums = FALSE;
int incrementData = FALSE;
int useSPX = TRUE;
int testing;
char titleStr[100];

void
ShowOptionsExit()
{
	fprintf(stderr,"%s: usage: %s",titleStr,titleStr);
	fprintf(stderr,"  [-I] [-v] [-c] [-s<Socket>] [-p<packet size>] \n");
	fprintf(stderr,"%s       -I Use IPX transoprt\n");
	fprintf(stderr,"%s       -v verbose mode\n");
	fprintf(stderr,"%s       -u assume DATA is incremented on each packet\n");
	fprintf(stderr,"%s       -c DO NOT compare data received\n");
	fprintf(stderr,"%s       -s Socket: server socket number override\n");
	fprintf(stderr,"%s       -p packet size: buffer size for receiving DATA\n");
	fprintf(stderr,"%s		 -C use IPX checksum on IPX and SPX data\n");
	fprintf(stderr,"%s		 -U assume incrementing pattern for data\n");
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
	t_error("loadError");

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
	char c;

	opterr =0;
	while ((c = getopt(argc, argv, optstr )) != -1 ) {
		switch (c) {
			case 'I' :
				useSPX = FALSE;
				break;
			case 'c' :
				compareData = FALSE;
				break;
			case 'u' :
				uniqueData = TRUE;
				break;
			case 'U' :
				incrementData = TRUE;
				break;
			case 'C' :
				ipxChecksums = TRUE;
				break;
			case 's' :
				sscanf(optarg,"%hx",&socketToBind);
				break;
			case 'p' :
				sscanf(optarg,"%d",&packetSize);
				if (packetSize>MAX_PACKET_DATA) 
					packetSize = MAX_PACKET_DATA;
				break;
			case 'w' :
				sscanf(optarg,"%d",&waitTime);
				if (waitTime>MAX_WAIT_TIME) 
					waitTime = MAX_WAIT_TIME;
				break;
			case 'v' :
				verbose = TRUE;
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
QuitThis(int signo)
{
	if (verbose)
		fprintf(stderr,"Quit This with signal %d\n",signo);
	testing = 0;
}

main(argc, argv)
	int argc;
	char **argv;

{ 
	char				Device[50];
	int 				fd, i;
	int					bytesRcvd, rflags;
	struct t_info		*info_req;
	struct t_bind		*bind_req;
	struct t_call		*call = NULL;
	struct t_unitdata   *rud = NULL;
	struct t_optmgmt	*opts;
    SPX2_OPTIONS        *SpxIIOpts;
	ipxAddr_t			serversAddress;
	ipxAddr_t			*ipxAddr;
	unsigned char 		lastData;
	unsigned char		rcvbuf[MAX_PACKET_DATA];
	long				packets=0;
	long				tmpBytes=0;
	long				megaBytes=0;


	strcpy(titleStr, argv[0]);

	ScanArgs(argc, argv, "ICUvcus:p:w:");
	/*
	 * convert Socket to NetOrder
	 */
	socketToBind = GETINT16(socketToBind);

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
        fprintf(stderr,"%s: Endpoint using buffer size %d for receiving\n",
			titleStr,packetSize);

	if ((bind_req = (struct t_bind *)t_alloc(fd, T_BIND, T_ALL)) == NULL) {
		fprintf(stderr,"t_alloc of %s failed t_errno= %d errno= %d\n",
			Device, t_errno, errno);
		t_error("t_alloc of T_BIND failed");
		exit(-1);
	}

	bind_req->qlen = 1;  
	bind_req->addr.len = sizeof(ipxAddr_t);
	ipxAddr = (ipxAddr_t *)bind_req->addr.buf;
	memcpy(&ipxAddr->sock[0], &socketToBind, sizeof(socketToBind));

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
        if (ipxChecksums) {
            SpxIIOpts->spxIISessionFlags = SPX_SF_IPX_CHECKSUM;
        }
        opts->flags = T_NEGOTIATE;
        if ((t_optmgmt(fd, opts, opts))<0) {
            fprintf(stderr,"%s: t_optmgmt T_NEGOTIATE failed \n", titleStr);
            loadError(fd);
            goto quit;
        }
        if (verbose) {
            fprintf(stderr,"%s: After T_NEGOTIATE the options are:\n",titleStr);
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
        t_free((char *)opts, T_OPTMGMT);

		if((call = (struct t_call *)t_alloc(fd, T_CALL, T_ALL))==NULL) {
			fprintf(stderr,"t_alloc of %s failed t_errno= %d errno= %d\n",
				Device, t_errno, errno);
			t_error("t_alloc of T_CALL failed");
			exit(-1);
		}
		if (verbose) {
			fprintf(stderr,"\n%s: SPX listening for clients Connect Requests ...\n",
				titleStr);
		}

		while (t_listen(fd, call)<0) {
			if (errno == EINTR)
				continue;
			fprintf(stderr,"%s: t_listen failed \n", titleStr);
			loadError(fd);
			goto quit;
		}

		if (verbose) {
   		 	ipxAddr = (ipxAddr_t *)call->addr.buf;
			fprintf(stderr,"\tConnect Request from address:\n\t net     0x%02X%02X%02X%02X\n",
				ipxAddr->net[0],ipxAddr->net[1],
				ipxAddr->net[2],ipxAddr->net[3]);
			fprintf(stderr,"\t node    0x%02X%02X%02X%02X%02X%02X\n",
				ipxAddr->node[0],ipxAddr->node[1],ipxAddr->node[2],
				ipxAddr->node[3],ipxAddr->node[4],ipxAddr->node[5]);
			fprintf(stderr,"\t socket  0x%02X%02X\n",
				ipxAddr->sock[0],ipxAddr->sock[1]);
		}

		if (t_accept(fd, fd, call)<0) {
			fprintf(stderr,"%s: t_accept failed \n", titleStr);
			loadError(fd);
			goto quit;
		}
		t_free((char *)call, T_CALL);
	} else {
		if((rud = (struct t_unitdata *)t_alloc(fd, T_UNITDATA, T_ALL))==NULL) {
			fprintf(stderr,"t_alloc of %s failed t_errno= %d errno= %d\n",
				Device, t_errno, errno);
			t_error("t_alloc of T_UNITDATA failed");
			exit(-1);
		}
		rud->addr.buf = (char *)&serversAddress;
		rud->udata.len = rud->udata.maxlen = packetSize;
		rud->udata.buf = (char *)&rcvbuf;
	}

	signal( SIGINT, QuitThis );
	signal( SIGQUIT, QuitThis );
	lastData = 0xff;
	tmpBytes = 0;
	megaBytes = 0;
	testing = 1;

	if (verbose) {
		fprintf(stderr, "%s:  Endpoint in RECEIVE loop ...  \n",titleStr);
		if (compareData) {
			fprintf(stderr, "\tEndpoint will compare received data\n");
		} else {
			fprintf(stderr, "\tEndpoint will NOT compare received data\n");
		}
		system("date");
	}
    if (ipxChecksums) {
        fprintf(stderr,"Using IPX Checksums\n");
    }

	while (testing) {
		if (useSPX) {
			if ((bytesRcvd = t_rcv(fd,(char *)rcvbuf,packetSize,&rflags))<0) {
					fprintf(stderr,"%s t_rcv failed \n", titleStr);
					loadError(fd);
					break;
			}
		}else {
			if (t_rcvudata(fd, rud, &rflags)<0) {
					fprintf(stderr,"%s t_rcvudata failed \n", titleStr);
					loadError(fd);
					break;
			}
			bytesRcvd = rud->udata.len;
		}
		packets++;
		if ((rcvbuf[0] == (unsigned char)'e') && 
				(rcvbuf[1] == (unsigned char)'n') && 
				(rcvbuf[2] == (unsigned char)'d')){
			if (verbose) {
				fprintf(stderr,"\tReceived Terminate Packet from Endpoint\n");
			}
			testing = 0;
			break;
		}
		if (useSPX) {   /* only compare with last data if using SPX */
			if (!(rflags & T_MORE)) {
				fprintf(stderr,"%s t_rcv return T_MORE cleared \n", titleStr);
				break;
			}
			if (uniqueData) {
				if (rcvbuf[0] != (unsigned char)(lastData + 1)) {
					fprintf(stderr,"%s t_rcv Bad 1st DATA: exp 0x%X  rcv 0x%x\n",
						titleStr, lastData, rcvbuf[0]);
					fprintf(stderr,"\tCOMPARE assumed unique DATA in each packet\n");
					break;
				}
			} else {
				if ((rcvbuf[0] != lastData) &&
							(rcvbuf[0] != (unsigned char)(lastData + 1))) {
					fprintf(stderr,"%s t_rcv Bad 1st DATA: exp 0x%X  rcv 0x%x \n",
						titleStr, lastData, rcvbuf[0]);
					break;
				}
			}
		}
		lastData = rcvbuf[0];
		if (compareData) {
			if (incrementData) {
				for(i=0; i<bytesRcvd; i++) {
					if (rcvbuf[i] != (unsigned char)(lastData + i)) {
						fprintf(stderr,
							"%s BadDATA: exp 0x%X rcv 0x%x offset %x\n",
							titleStr,(unsigned char)(lastData) + i, rcvbuf[i], i);
						break;
					}
				}
			} else { 
				for(i=0; i<bytesRcvd; i++) {
					if (rcvbuf[i] != lastData) {
						fprintf(stderr,
							"%s BadDATA: exp 0x%X rcv 0x%x offset %x\n",
							titleStr, lastData, rcvbuf[i], i);
						break;
					}
				}
			}
			lastData = rcvbuf[i-1];
			if ( i != bytesRcvd ) {
				fprintf(stderr,"%s Wrong amount of good DATA act=%d, exp=%d\n",
					titleStr, i, bytesRcvd);
				break;
			}
		}
		tmpBytes += bytesRcvd;
		if (tmpBytes > 1000000) {
			megaBytes++;
			tmpBytes -= 1000000;
		}
		if (verbose) {
			if ((packets & (long)0xffff) == 0xffff) {
				system("date");
				fprintf(stderr,"\t0x%08X Packets received\n\t%010d MegaBytes received\n",
					packets, megaBytes);
			}
		}
	} /* end of while loop */

	if (verbose ) {
		fprintf(stderr,"\n%s All Done \n", titleStr);
		system("date");
		fprintf(stderr,"%s 0x%08X Packets received\n\t%08d MegaBytes received\n",
			titleStr, packets, megaBytes);
	}

	quit:

	if (t_close(fd)<0) {
		fprintf(stderr,"%s: t_close failed \n", titleStr);
		loadError(fd);
	}
}
