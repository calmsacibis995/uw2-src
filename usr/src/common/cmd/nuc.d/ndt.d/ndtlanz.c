/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:ndtlanz.c	1.1"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/ndtlanz.c,v 1.1 1994/01/31 21:52:07 duck Exp $"

/*
 *        Copyright Novell Inc. 1991
 *        (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *        No part of this file may be duplicated, revised, translated, localized
 *        or modified in any manner or compiled, linked or uploaded or
 *        downloaded to or from any computer system without the prior written
 *        consent of Novell, Inc.
 *
 *
 *  Netware Unix Client 
 *        Author: Duck
 *       Created: Thu Nov 12 10:29:58 MST 1992
 *
 *  MODULE:		ndtlanz
 *
 *  ABSTRACT:	Decode IPX packet fragments from the trace table.
 *
 */
#include	<sys/param.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/nwctrace.h>
#include	<sys/traceuser.h>
#include	<sys/stream.h>
#include	<syms.h>
#include	"ndt.h"

extern	double			etime;	/* elapsed time	in a routine or on the stopwatch or the wire*/


/*
 *	Case and printf all in one, for ncp decodes.
 *	Left justify 30 character function strings.
 */
/* #define cp(c,s)	case (c): printf("%-30s",(s)); break duck hack */
#define cp(c,s)	case (c): return((s))


/*
 *	Handle default case of an unknown ncp 
 *	function/subfunction codes.
 */
#define defaultFunction	default:										\
							printf( "UNKNOWN function %d/%d", 			\
								p->functionCode, p->subFunctionCodeA);	\
							return( "UNKNOWN NCP request")







/*
 *	Array to keep track of the time a reply came in from a server.
 *	When we see the corresponding request, we know how long it
 *	was outstanding.
 *	For the time being, we don't factor the actual server into this,
 *	so things may be confusing if we're using the same connection
 *	number on two servers at the same time.
 */
#define	MAX_CONN	4096
static double	serverResponse[MAX_CONN] = {0.0};






		char *ncp_request();
		char *ncp_reply();
		char *decode_lanz_trace( trace_t *tp);


trace_t *
print_lanz( tp)
	trace_t *tp;
{
	struct pkt 		*p;
	char			*txrx;
	int				connId;

	txrx= (tp->type == NVLTT_xmit) ? "TX" : "RX";

	p = (struct pkt *) &tp->v1;
	connId = (p->connHigh<<8) | p->connLow;

	switch( p->ipxPacketType ) {
		case 17:							/* NCP packet */
			printf("%s ncp ", txrx);
		
			switch( p->ncpType) {

				case 0x1111:
				case 0x2222:
				case 0x5555:
				case 0x7777:
					printf("request: %-30s", ncp_request(p) );
					printf("        seq:%d conn:%d task:%d\n",
						p->sequenceNumber, connId, p->taskNumber);

					break;

				case 0x3333:
				case 0x9999:
					printf("  reply:  %-30s", ncp_reply( p));
					printf("        seq:%d conn:%d task:%d cCode:0x%x cStat:0x%x\n", 
						p->sequenceNumber, connId, p->taskNumber, p->completionCode,
						p->connectionStatus);
						
					break;

				default: /* NCP type */
					printf( "UNKNOWN  ncpType=0x%x\n", p->ncpType);
					break;
			}
			break;

		default:							/* IPX Packet Type */
			printf( "%s Non-NCP packet\n", txrx);
			break;
	}
	return tp;
}




char *
ncp_request( p)
	struct pkt *p;
{

	switch( p->ncpType) {
		cp(  0x1111, "Create Service Connection");
		cp(  0x5555, "Destroy Service Connection");
		cp(  0x7777, "Packet Burst");
		case 0x2222:

			switch( p->functionCode) {

				cp( 18, "Get Volume Info with Number");
				cp( 20, "Get File Server Date and Time");

				case 21:							/* Message */
					switch( p->subFunctionCodeA) {
						cp(  0, "Send Broadcast Message (old)");
						cp(  1, "Get Broadcast Message (old)");
						cp(  2, "Disable Broadcasts");
						cp(  3, "Enable Broadcasts");
						cp(  4, "Send Personal Message");
						cp(  5, "Get Personal Message");
						cp(  9, "Broadcast to Console");
						cp( 10, "Send Broadcast Message");
						cp( 11, "Get Broadcast Message");
						defaultFunction;
					}
					break;


				case 22:							/* File/Directory */
					switch( p->subFunctionCodeA) {
						cp(  0, "Set Directory Handle");
						cp(  1, "Get Directory Path");
						cp(  2, "Scan Directory Information");
						cp(  3, "Get Effective Directory Rights");
						cp(  5, "Get Volume Number");
						cp(  6, "Get Volume Name");
						cp( 10, "Create Directory");
						cp( 11, "Delete Directory");
						cp( 12, "Scan Directory Trustees");
						cp( 13, "Add Directory Trustee");
						cp( 14, "Delete Directory Trustee");
						cp( 15, "Rename Directory");
						cp( 18, "Alloc Perm Dir Handle");
						cp( 19, "Alloc Temp Dir Handle");
						cp( 20, "Delete Dir Handle");
						cp( 25, "Set Directory Info");
						cp( 30, "Scan a Directory");
						cp( 42, "Get Effective Rights For Dir");
						cp( 45, "Get Directory Information");
						cp( 46, "Rename or Move");
						cp( 47, "Get Name Space Information");
						cp( 48, "Get Name Space Directory Entry");
						defaultFunction;
					}
					break;

				case 23:
					switch( p->subFunctionCodeA) {
						cp( 15, "Scan File Information");
						cp( 16, "Set File Information");
						cp( 17, "Get File Server Information");
						cp( 20, "Login Object");
						cp( 22, "Get Station's Logged Info");
						cp( 23, "Get Login Key");
						cp( 24, "Keyed Login");
						cp( 52, "Rename Object");
						cp( 53, "Get Bindery Object ID");
						cp( 54, "Get Bindery Object Name");
						cp( 55, "Scan Bindery Object");
						cp( 60, "Scan Property");
						cp( 61, "Read Property Value");
						cp( 62, "Write Property Value");
						cp( 68, "Close Bindery");
						cp( 69, "Open Bindery");
						cp( 70, "Get Bindery Access Level");

						cp(100, "Create Queue");					/* QMS */
						cp(101, "Destroy Queue");
						cp(102, "Read Queue Current Status (old)");
						cp(103, "Set Queue Current Status");
						cp(104, "Create Queue Job and File");
						cp(105, "Close File and Start Queue Job (old)");
						cp(106, "Remove Job from Queue");
						cp(107, "Get Queue Job List");

						cp(201, "Get FS Description Strings");
						cp(222, "Get Phys Record Locks by File");
						cp(233, "Get Volume Info");
						defaultFunction;
					}
					break;

				cp( 24, "End of Job");
				cp( 25, "Logout");
				cp( 33, "Negotiate Buffer Size");
				cp( 62, "File Search Initialize");
				cp( 63, "File Search Continue");
				cp( 64, "Search For a File");
				cp( 65, "Open File (old)");
				cp( 66, "Close File");
				cp( 67, "Create File");
				cp( 68, "Erase File");
				cp( 69, "Rename File");
				cp( 70, "Set File Attributes");
				cp( 71, "Get Current Size of File");
				cp( 72, "Read From a File");
				cp( 73, "Write to a File");
				cp( 76, "Open File");
				cp( 77, "Create New File");

				case 87:					/* 3.11 Enhanced */
					switch( p->subFunctionCodeB) {
						cp(  1, "(386) Open/Create File/Dir");
						cp(  2, "(386) Initialize Search");
						cp(  4, "(386) Rename/Move File/Dir");
						cp(  6, "(386) Obtain File/Dir Info");
						cp(  7, "(386) Modify File/Dir Info");
						cp(  8, "(386) Delete File/Dir");
						cp( 12, "(386) Alloc Short Dir Handle");
						cp( 19, "(386) Get NameSpace Info");
						cp( 20, "(386) Search for File/Dir Set");
						cp( 21, "(386) Get Path String from Handle");
						cp( 22, "(386) Generate Directory Base");
						cp( 24, "(386) Get NS Loaded list from Vol");
						cp( 25, "(386) Set NameSpace Info");
						cp( 27, "(386) Set Huge NameSpace Info");
						cp( 28, "(386) Get Full Path String");
						cp( 29, "(386) Get Effective Dir Rights");
						defaultFunction;
					}
					break;

				case 95:
					switch( p->subFunctionCodeB) {
						cp(  0, "NUC Capable?");
						cp(  1, "NUC OpenFile");
						cp(  2, "NUC CreateFileOrSubdirectory");
						cp(  3, "NUC RenameFileOrSubdirectory");
						cp(  4, "NUC DeleteFileOrSubdirectory");
						cp(  5, "NUC LinkFile");
						cp(  6, "NUC GetAttributes");
						cp(  7, "NUC SetAttributes");
						cp(  8, "NUC CheckAccess");
						cp(  9, "NUC GetDirectoryEntries");
						cp( 21, "NUC ReadFile");
						cp( 22, "NUC WriteFile");
						defaultFunction;
					}
					break;
					
				cp(101, "Packet Burst Connection");
			
				case 104:					/* NDS Directory Services */
					switch( p->subFunctionCodeB) {
						cp(  0, "Send NDS frag Request/Reply");
						cp(  1, "Ping for NDS NCP");
						cp(  2, "Manage NDS Connection Status");
						defaultFunction;
					}
					break;

				defaultFunction;		/* Handle all the rest */
			}
			break;
	}
}



char *
ncp_reply( p)
	struct pkt *p;
{

	switch( p->ncpType) {
		cp(  0x9999, " Server Busy");
		case 0x3333:
		
			switch( p->completionCode) {
				cp( 0x00, "Success");
				cp( 0x80, "Lock Fail");
				cp( 0x81, "No More File Handles");
				cp( 0x82, "No Open Privileges");
				cp( 0x84, "No Create Privileges");
				cp( 0x85, "No Create/Delete Privileges");
				cp( 0x88, "Bad File Handle");
				cp( 0x89, "No Search Privileges");
				cp( 0x8a, "No Delete Privileges");
				cp( 0x8b, "No Rename Privileges");
				cp( 0x8c, "No Modify Privileges");
				cp( 0x8d, "Some Files In Use");
				cp( 0x8e, "All Files In Use");
				cp( 0x91, "Some Names Exist");
				cp( 0x92, "All Names Exist");
				cp( 0x93, "No Read Privileges");
				cp( 0x94, "No Write Privileges");
				cp( 0x96, "Server Out of Memory");

				cp( 0x99, "Directory Full/Invalid Name");
				cp( 0x9b, "Bad Dir Handle");
				cp( 0x9c, "Invalid Path");
				cp( 0x9d, "No Dir Handles");
				cp( 0x9e, "Bad Filename");
				cp( 0xa0, "Directory Not Empty");
				cp( 0xa2, "Read Locked");
				cp( 0xd3, "No Queue Rights");
				cp( 0xfb, "No Such Property");
				cp( 0xfc, "No Such Object");
				cp( 0xfd, "Unknown Req/Lock Collision");
				cp( 0xfe, "Timeout");
				cp( 0xff, "Failure");
				default:
					printf("UNKNOWN completion code 0x%x", p->completionCode);
					return("UNKNOWN reply");
			}
			break;
	}
}




void
gather_xmit( tp, time)
	trace_t	*tp;
	double	time;
{
	struct pkt *p;
	int			connId;

	p = (struct pkt *) &tp->v1;

	if( p->ipxPacketType  == 17) {				/* NCP packet */
		if( p->ncpType == 0x7777) {				/* packet burst	*/
			etime = 0.0;
		} else {
			connId = (p->connHigh<<8) | p->connLow;
			if( (connId >= MAX_CONN) || (serverResponse[connId] == 0.0) )
				etime=0.0;						/* bad connId || request without reply */
			else
				etime = serverResponse[connId] - time;
		}
	}
}



void
gather_rec( tp, time)
	trace_t	*tp;
	double	time;
{
	struct pkt *p;
	int			connId;

	p = (struct pkt *) &tp->v1;

	if( (p->ipxPacketType  == 17) && (p->ncpType != 0x7777)) {		/* NCP packet, not packet burst */
		connId = (p->connHigh<<8) | p->connLow;
		if( connId < MAX_CONN)
			serverResponse[connId] = time;
	}
}




char *
decode_lanz_trace( tp)
	trace_t *tp;
{
	struct pkt 		*p;

	p = (struct pkt *) &tp->v1;

	switch( p->ipxPacketType ) {
		case 17:							/* NCP packet */
		
			switch( p->ncpType) {

				case 0x1111:
				case 0x2222:
				case 0x5555:
				case 0x7777:
					return( ncp_request( p));
					break;

				case 0x3333:
				case 0x9999:
					return( ncp_reply( p));
					break;

				default: /* NCP type */
					return( "UNKNOWN  ncpType");
			}
			break;

		default:							/* IPX Packet Type */
			return( "Non-NCP");
			break;
	}
}




