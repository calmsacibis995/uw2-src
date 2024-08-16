/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/tsad.h	1.8"
/***
 *
 *  name	tsad.h - definitions and structures for unix tsa daemon
 *
 ***/

#ifndef _TSAD_H_INCLUDED
#define _TSAD_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <smstypes.h>
#include <smsutapi.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
//#include <sysent.h>	/* This file may not be present in all platforms */
#include <unistd.h>	/* This file may not be present in all platforms */
#include <poll.h>
#include <tiuser.h>
#include <sys/param.h>
#include <string.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <tsalib.h>
#include <netdb.h>
#include <fcntl.h>

#ifdef TLI
#include <netconfig.h>
#include <netdir.h>
#include <tiuser.h>
#endif

#define SPX		1
#define TCPIP		2
#define ADSP		4
#define REGISTRATION_TIMEOUT	60 * 60 
#define MIN_TIMEOUT	60 
#define REPLY_PACKET_SIZE	32
#define CONNECT_TIMEOUT	30

#define MAX_ADDRESS		32
#define TCP_ADDRESS_SIZE        16              /* sizeof struct sockaddr_in */
#define IPX_ADDRESS_SIZE        12              /* sizeof IPX_ADDR */
#define ADSP_ADDRESS_SIZE       4
#define TSNAMELEN		64

#define DEFAULT_ENTRY		"default"
#define SMS_SERVER		"smshost"
#define TSA_NAME		"/usr/sbin/tsaunix"
#define CONFIG_FILE		"/etc/unixtsa.conf"
#define LOG_FILE		"/var/adm/unixtsa.log"
#define LOG_MODE		"a"
#define LOG_SIZE		64 * 1024
#define CONSLOG			"/dev/console"
#define CONFIG_MAP		"unixtsa.conf"
#define SERVICE_NAME		"unixtsa"
#define TCP_NAME		"tcp"
#define SPX_NAME		"spx"
#define WORKSTATION_TYPE	"UNIX"

typedef struct worknode
{
	struct worknode *next;
	
	char
		TSname[64],
		TStype[32],
		TSversion[32];

        UINT8
                TCPaddress[TCP_ADDRESS_SIZE],
                SPXaddress[IPX_ADDRESS_SIZE],
                ADSPaddress[ADSP_ADDRESS_SIZE];

        UINT32
                TCPaddressSize,
                SPXaddressSize,
                ADSPaddressSize;

        UINT32
                protocol,
                preferredProtocol,
                registrationTime,
                fileAddress;

} 
WORKSTATION_INFO;

struct requestPacket {
	UINT32	packetType ;
	WORKSTATION_INFO	targetInfo ;
} ;

/* Packet type definitions */
#define REGISTER		0
#define WITHDRAW		1
#define ADVERTISE_SERVICE 	2
#define REPLY_PACKET		3

struct replyPacket {
	UINT32 packetType ;
	INT32 returnCode ;
} ;

struct advertisePacket {
	UINT32 packetType ;
	struct sockaddr_in rcvaddr ;
} ;

/*
 * server connection structure
 * There is one instance of this structure for each entry in the
 * config file and/or NIS map.  Note however that there is only
 * one instance of requestData for each server.
 */
struct serverData {
	char			*serverName ;
	int			protocol ;
	int			success ;
	int			lastFailure ;
	int			myRequestData ; // requestData local(1) or shared(0)
	struct nd_addrlist	*serverAddress ;
	struct requestPacket	*requestData ;
	struct netconfig	*netData ;
	struct serverData	*next ;

	// i18n information
	char			*tsaCodeset,
				*engineCodeset,
				*engineLocale ;

} ;

#endif
