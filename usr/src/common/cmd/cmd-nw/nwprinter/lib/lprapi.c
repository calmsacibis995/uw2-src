/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/lib/lprapi.c	1.1"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/lib/lprapi.c,v 1.1 1994/02/11 18:18:40 nick Exp $";
#endif

#include <stdio.h>
#include <limits.h>
#if !defined(PATH_MAX)
#define PATH_MAX 255
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>

#include "lprapi.h"

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

int debugFd = -1;

static int
lprWriteNoBlock(int fd, char *packet, int count, int *retCount)
{
	int pCount;
	int status;

	status = 0;

	pCount = write(fd, packet, count);
	if (pCount == -1) {
		pCount = 0;
		if (errno == EWOULDBLOCK)
			status = 1;
		else
			status = -1;
	}
	if (retCount)
		*retCount = pCount;
#ifdef DEBUG
	if (debugFd != -1)
		write(debugFd, packet, count);
#endif
	return status;

}

static int
lprWrite(int fd, char *packet, int count, int *retCount)
{
	/* Make sure the data is written (BLOCKing mode)*/
	int blockMode;
	int status;
	int pCount;

	blockMode = LprSetBlock(fd);
	do {
		pCount = write(fd, packet, count);
	} while (pCount == -1 && errno == EAGAIN);
	if (blockMode == 0)	/* Back to no block mode */
		LprSetNoBlock(fd);
	
	if (pCount == -1) {
		status = -1;
		pCount = 0;
	} else {
		status = 0;
	}

	if (retCount)
		*retCount = pCount;

#ifdef DEBUG
	if (debugFd != -1)
		write(debugFd, packet, count);
#endif
	return status;
}


static int
lprRead(int fd, char *packet, int count, int *retCount)
{
	int pCount;
	int status;

	status = 0;
	pCount = read(fd, packet, count);
	if (pCount == -1) {
		pCount = 0;
		if (errno == EWOULDBLOCK)
			status = 1;
		else
			status = -1;
	}
	if (retCount)
		*retCount = pCount;

	return status;

}

/*
** Return the string of the last LprOpen
*/
static char tcpAddrStr[28];

char *
LprGetOpenAddrStr(void)
{
	return tcpAddrStr;
}

/*
** Return -1 on error else fd for socket
*/
int
LprOpen(char *hostName)
{
	struct servent		*sp;
	struct hostent		*hp;
	struct sockaddr_in	srvAddr;
	unsigned long		inAddr;
	int					fd, resvport;

	sp = getservbyname("printer", "tcp");
	if (sp == NULL)
		return -1;

	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = sp->s_port;

	/*
	** Get host addr --
	** Try convert dotted notation then gethostbyname
	*/
	if ((inAddr = inet_addr(hostName)) != INADDR_NONE) {
		memcpy(&srvAddr.sin_addr, &inAddr, sizeof(inAddr));
	} else {
		if ((hp = gethostbyname(hostName)) == NULL) {
			return -1;
		}
		memcpy(&srvAddr.sin_addr, hp->h_addr, hp->h_length);
	}

	/*
	** Build string incase others need it via LprGetOpenAddrStr()
	*/
	memcpy(&inAddr, &srvAddr.sin_addr, sizeof(inAddr));
	sprintf(tcpAddrStr, "%08x %02x", inAddr, sp->s_port);
	
	/*
	** Open socket on reserved port
	*/
	resvport = IPPORT_RESERVED - 1;
	if ((fd = rresvport(&resvport)) < 0) {
		return -1;
	}
	
	/*
	** Connect to the server
	*/
	if (connect(fd, (struct sockaddr *)&srvAddr,
					sizeof(srvAddr)) < 0 ) {
		close(fd);
		return -1;
	}
	/*
	** Return the file descriptor
	*/
	return fd;
}

/*
** Close the lpr socket
** Return -1 on error else OK 
*/
int
LprClose(int fd)
{
	if (fd != -1)
		(void)close(fd);
#ifdef DEBUG
	if (debugFd != -1)
		close(debugFd);
	debugFd = -1;
#endif
	return 0;
}


/*
** Set socket to NO BLOCK
** -1 Error
**  0 Was NOT set to NO Block
**  1 Already Set to Block
*/
int
LprSetNoBlock(int fd)
{
	int status;
	int fileFlags;

	if ((fileFlags = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;

	if (fileFlags & FNDELAY)
		return 1;

	status = fcntl(fd, F_SETFL, fileFlags | FNDELAY);
	if (status == -1)
		return -1;

	return 0;
}

/*
** Set socket to BLOCK
** -1 Error
**  0 Was NOT set to  Block
**  1 Already Set to Block
*/
int
LprSetBlock(int fd)
{
	int status;
	int fileFlags;
	
	if ((fileFlags = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;

	if ((fileFlags & FNDELAY) == 0)
		return 1;

	status = fcntl(fd, F_SETFL, fileFlags & (~FNDELAY));
	if (status == -1)
		return -1;
	return 0;
}

/*
** GetAck 
**  -2 Neg ack (socket good command error)
**  -1 Disconnect or socket error
**	 0 SUCCESS
**   1 Would block
**
*/
int
LprGetAck(int fd)
{
	char packet[4];
	int status;

	packet[0] = (char)0xff;
	status = lprRead(fd, packet, 4, NULL);
	if (status == -1 || status == 1) {
		return status;
	}
	
	if (packet[0] != 0) {
		return -2;
	}
	return 0;
}

int
LprStartPrinter(int fd, char *printerName)
{
	char	packet[PATH_MAX+3];
	
#ifdef DEBUG
	debugFd = open(printerName, O_CREAT | O_APPEND | O_RDWR, 0666); 
#endif
	/* 
	** Start printer message
	*/
	sprintf(packet,"%c%s\n", '\002', printerName);
	if (lprWrite(fd, packet, strlen(packet), NULL) < 0 ) {
		close(fd);
		return -1;
	}
	return 0;
}

/*
** Request queue state command
*/
int
LprRequestQueueState(int fd, char *fileName, char *userName)
{
	char packet[PATH_MAX+13];

	if (userName == NULL) {
		sprintf(packet,"%c%s\n", '\003', fileName);
	} else {
		sprintf(packet,"%c%s %s\n", '\003', fileName, userName);
	}
	if (lprWrite(fd, packet, strlen(packet), NULL) < 0 ) {
		return -1;
	}

	return 0;
}

/*
** Remove Queue entry
*/
int
LprRemoveQueueEntry(int fd, char *fileName, char *userName)
{
	char packet[PATH_MAX+13];

	sprintf(packet,"%c%s root %s\n", '\005', fileName, userName);
	if (lprWrite(fd, packet, strlen(packet), NULL) < 0 ) {
		return -1;
	}

	return 0;
}

/*
** Read queue state command
**	-1 = Done/Error, 0 = Success, 1 = Would block
*/
int
LprReadQueueState(int fd, int maxSize, char *buf)
{
	int retSize;
	int status;

	status = lprRead(fd, buf, maxSize, &retSize);
	if (status == -1 || retSize == 0 ) {
		return -1;
	}
	if (status == 1) {
		return 1;
	}
	buf[retSize] = '\0';
	return 0;
}

/*
** Send file Data command (After Start Printer)
*/
int
LprSendFileDataCommand(int fd, int size, char *fileName)
{
	char packet[PATH_MAX+13];
	sprintf(packet,"%c%ld %s\n", '\003', size, fileName);
	if (lprWrite(fd, packet, strlen(packet), NULL) < 0 ) {
		return -1;
	}

	return 0;
}

/*
** Send Control Data command (After Start Printer)
*/
int
LprSendControlDataCommand(int fd, int size, char *fileName)
{
	char packet[PATH_MAX+13];
	sprintf(packet,"%c%ld %s\n", '\002', size, fileName);
	if (lprWrite(fd, packet, strlen(packet), NULL) < 0 ) {
		return -1;
	}

	return 0;
}



/*
** Send file Data
*/
int
LprSendData(int fd, int size, char *data, int *retCount )
{
	return lprWrite(fd, data, size, retCount);
}

/*
** Send file Data NoBLOCK
*/
int
LprSendDataNoBlock(int fd, int size, char *data, int *retCount )
{
	int status;

	status = lprWriteNoBlock(fd, data, size, retCount);
	return status;
}

/*
** Send file Data End
*/
int
LprSendDataEnd(int fd )
{
	return lprWrite(fd, "", 1, NULL);
}

char *
LprGetHostName(void)
{
	static int done = 0;
	static char hostName[PATH_MAX];

	if (done  == 0) {
		(void)gethostname(hostName, sizeof (hostName));
		done = 1;
	}
	return hostName;
}


