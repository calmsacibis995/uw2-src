/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mergeipx:mipxadm.c	1.8"
/*
 *        Copyright  Univel Inc. 1991
 *        (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *        No part of this file may be duplicated, revised, translated, localized
 *        or modified in any manner or compiled, linked or uploaded or
 *        downloaded to or from any computer system without the prior written
 *        consent of Univel, Inc.
 *
 *
 *   Merge IPX driver admin module
 *
 *  MODULE:	mipxadm.c
 *
 *  ABSTRACT:	Admin process for STREAMS mux for merge IPX  support.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <stropts.h>
#include <stdlib.h>
#include <sys/stream.h>
#include <sys/nwctypes.h>
#include <sys/ipx_app.h>
#include "merge.h"
#include "mpip_usr.h"

#define	FAIL	-1
#define IPX_GETMORESTREAMS 0x01
#define IPX_DIE	       	0x03
#define MIPX_SET_SOCKET 0xff
#define MIPX_SET_NET 0xfe
#define MIPX_REL_DOS 0xfd
#define	swapint32( x) {uint32 i; i = x; x = (((i) & 0xff) << 24 ) | (((i) & 0xff00) << 8) | (((i) & 0xff0000) >> 8) | (((i) & 0xff000000) >> 24); }

static int muxid[21]; /*each process will have a maximum of 20 streams*/
static int next;

void	exit();
struct setsock {
	int muxid;
	unsigned char sock[IPX_SOCK_SIZE];
};

extern int errno;

int
ProcessforIPXHeavy(cntl_fd, lnstring)
int cntl_fd;
char *lnstring;
{
	lanInfo_t   *laninfo;
	struct strioctl ioc;
	IpxConfiguredLans_t numlans;
	int Lannum;
	int fd;
	int ret;

	fd = open ("/dev/ipx", O_RDWR);
	if (fd < 0) 
		exit (-1);
	Lannum = atoi(lnstring); /*This is passed on from the dos side
				based on the user's input*/
	ioc.ic_cmd = IPX_GET_CONFIGURED_LANS;
	ioc.ic_timout = INFTIM;
	ioc.ic_len = sizeof(IpxConfiguredLans_t );
	ioc.ic_dp = (char *) &numlans;
	if (ioctl(fd, I_STR, &ioc) < 0) {
		return (-1);
	}
	if (Lannum > numlans.lans) 
		return -1;
	laninfo = (lanInfo_t *) malloc (numlans.lans * sizeof(lanInfo_t));
	laninfo = (lanInfo_t *) malloc (2 * sizeof(lanInfo_t));
	ioc.ic_cmd = IPX_GET_LAN_INFO;
	ioc.ic_timout = INFTIM;
	ioc.ic_len = sizeof(lanInfo_t);
	ioc.ic_dp = (char *) laninfo;
	if (ioctl(fd, I_STR, &ioc) < 0) {
		return (-1);
	}
	close(fd);
	/*potential problems, does the ipx driver require it to be swapped or*/
	swapint32(laninfo[Lannum].network);
	ioc.ic_cmd = MIPX_SET_NET;
	ioc.ic_timout = INFTIM;
	ioc.ic_len = sizeof(laninfo[Lannum].network);
	ioc.ic_dp = (char *) &laninfo[Lannum].network;
	if (ioctl(cntl_fd, I_STR, &ioc) < 0) {
		return (-1);
	}
}

int
LinkMoreStreams(cntl_fd, number)
int cntl_fd;
int number;
{
	unsigned char socket[2];
	struct strioctl ioc;
	struct setsock setsocket;
	int fd;
	int count;
	int mid;
	int tmp;

	for (count=0; count < number; count++) {
		if ((fd  = open("/dev/ipx", O_RDWR)) < 0) {
			break;
		}

		socket[0] = 0;
		socket[1] = 0;
		ioc.ic_cmd = IPX_SET_SOCKET;
		ioc.ic_timout = 0;
		ioc.ic_len = IPX_SOCK_SIZE;
		ioc.ic_dp = (char *)socket;
		if (ioctl(fd, I_STR, &ioc) < 0)
			break;
		mid = ioctl(cntl_fd, I_LINK, fd);
		if (mid < 0) {
			break;
		}
		else muxid[next++] = mid;
		close(fd);
	}
	return (count);
}

void
UnlinkAllStreams(fd)
int fd;
{
	int i;

/*
  Have to unlink the mpip driver last because of the way the other streams are
  kept track of thorugh the mpip driver stream.

*/
	for(i=next - 1; i >= 0; i-- )
		ioctl(fd, I_UNLINK, muxid[i]);
}

static int
InitVpi(argc, argv, fd)
int argc;
char *argv[];
int fd;
{
	struct strioctl ioc;
	mpipInitT mergeinit;
	int vpifd;

	vpifd = open("/dev/mpip", O_RDWR|O_SYNC, 0);
	if ( vpifd == FAIL) {
		return FAIL;
	}
	mergeinit.vm86pid = atoi(argv[1]);
	mergeinit.irqNum = atoi(argv[2]);
	mergeinit.ioBasePort  = atoi(argv[3]);

	ioc.ic_cmd = MPIP_INIT;
	ioc.ic_timout = INFTIM;
	ioc.ic_len = sizeof(mergeinit);
	ioc.ic_dp = (char *) &mergeinit;

	if (ioctl( vpifd, I_STR, &ioc) == FAIL)
	{
		return FAIL;
	}
	muxid[0] = ioctl(fd, I_LINK, vpifd);
	if (muxid[0] < 0) {
		return FAIL;
	}
	next = 1; /*The first muxid is the stream to mpip*/
	return 0;
}

void
quit_mipxadm(retval, cntl_fd)
int retval;
int cntl_fd;
{

	struct strioctl ioc;

	ioc.ic_cmd = MIPX_REL_DOS;
	ioc.ic_timout = 5;
	ioc.ic_len = 0;
	ioc.ic_dp = (char *) NULL;
	ioctl(cntl_fd, I_STR, &ioc);
	UnlinkAllStreams(cntl_fd);
	close(cntl_fd);
	exit (retval);
}

int
main (argc, argv)
int argc;
char *argv[];
{
	int cntl_fd;
	unsigned char  cmd;
	struct pollfd pf;

#if	1
	switch(fork()) {
	case 0:
		break;
	case -1:
		exit (-1);
	default:
		exit(0);
	}
#endif
	setsid();
	cntl_fd = open ("/dev/mipx", O_RDWR);
	if (cntl_fd < 0)
		exit(-1);
	if (InitVpi(argc, argv, cntl_fd) < 0)
		quit_mipxadm (-1, cntl_fd);

	if (ProcessforIPXHeavy(cntl_fd, argv[4]) < 0)
		quit_mipxadm (-1, cntl_fd);

	if (LinkMoreStreams(cntl_fd, 15) < 15)
		quit_mipxadm(-1);
	while (1) {
		pf.fd = cntl_fd;
		pf.events = POLLIN;
		poll (&pf, 1, INFTIM);
		if (pf.revents & (POLLERR|POLLHUP)) {
			UnlinkAllStreams(cntl_fd);
			close(cntl_fd);
			exit(-1);
		}
		else {
			if (read(cntl_fd, &cmd, 1) < 0) {
				close (cntl_fd);
				exit(-1); /*something wrong with the stream*/
			}
			switch (cmd) {
			case IPX_GETMORESTREAMS:
				LinkMoreStreams(cntl_fd, 5);
				break;
			case IPX_DIE:
				UnlinkAllStreams(cntl_fd);
				close(cntl_fd);
				exit (0);
				break;
			default:
				break;
			}
		}
	}
}
