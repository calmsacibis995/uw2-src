/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwlogin:nwbinderyprops.h	1.1"
#ident	"$Header: $"

#ifndef _NWBINDERYPROPS_H
#define _NWBINDERYPROPS_H



#include "nct.h"
/*
 *LOGIN_CONTROL
 *	Set/Item:			Item
 *	Dynamic/Static:		Static
 *	Read access:		Object
 *	Write access:		Supervisor
 *This property can be owned by users.  The account restriction information is
 *recorded here.  The data field uses 86 bytes in a structure as outlined below:
 *
 *	Exp_Date:  The date that the account expires.  The date is recorded in
 *	the order of Year, Month, then Day.
 *
 *	Acct_Expired:  This is a flag to denote if the account is disabled.  The
 *	value of 0xFF means the account is disabled, and the value of 0x00 means
 *	the account is not disabled.
 *
 *	Pass_Date:  The date that the password expires.  The date is recorded in
 *	the order of Year, Month, then Day.
 *
 *	Pass_Grace:  The number of remaining grace logins for the user.  A value
 *	of 0xFF means that a limit has not been placed on the number of grace
 *	logins.
 *
 *	Exp_Interval:  The number of days between forced password changes.
 *
 *	Grace_Reset:  The number of grace logins allowed. A value of 0xFF means
 *	that a limit has not been placed on the number of grace logins.
 *
 *	Min_Pass_Length:  The minimum password length for the user password.
 *
 *	Max_Connections:  The maximum number of concurrent connections for the
 *	user.
 *
 *	Time_BitMap:  This is a bit map for the allowed login times.  The
 *	allowed times are kept in one half hour blocks, making 48 blocks in a
 *	day.  48 blocks * 7 days = 336 blocks.  42 bytes * 8 bits/byte = 336
 *	bits. The bits in the 42 bytes correspond directly to the allowed login
 *	times.  The first six bytes map to Sunday, the second six to Monday,
 *	etc.
 *
 *	Last_Log_Time:  The last login date and time is kept in six bytes:
 *	Year, Month, Day, Hour, Minute, Second.
 *
 *	RestrictionFlags:  These are restrictions for passwords.  A value of
 *	0x01 means that only the supervisor can change passwords.  A value of
 *	0x02 means that unique passwords are required.
 *
 *	UnknownInfo:  Unused byte to realign the data on an even boundary.
 *
 *	MaxDiskBlocks:  The maximum number of 4K blocks that the user is allowed
 *	to use on the disk.  A value of 0x7FFFFFFF is the default.
 *
 *	BadLogCount:  The number of bad login attempts for this user.  A value
 *	of 0xFF means that the account is locked.
 *
 *	NextResetTime:  This value is the number of minutes from January 1, 1985
 *	until the next reset time.
 *
 *	BadStnAddress:  A twelve byte station address contains the socket
 *	number. Therefore, the first four bytes are the network address, the
 *	next six bytes are the station address, and the last two bytes are the
 *	socket number.
 */

typedef struct {
	uint8		Exp_Date[3];
	uint8		Acct_Expired;
	uint8		Pass_Date[3];
	uint8		Pass_Grace;
	uint16		Exp_Interval;
	uint8		Grace_Reset;
	uint8		Min_Pass_Length;
	uint16		Max_Connections;
	uint8		Time_BitMap[42];
	uint8		Last_Log_Time[6];
	uint8		RestrictionFlags;
	uint8		UnknownInfo;
	uint32		MaxDiskBlocks;
	uint16		BadLogCount;
	uint32		NextResetTime;
	uint8		BadStnAddress[12];
} LOGIN_CONTROL_T;

#define ONLY_SUPERVISOR_CHANGE			0x01

/*NET_ADDRESS
 *	Set/Item:		Item
 *	Dynamic/Static:		Dynamic
 *	Read access:		Any
 *	Write access:		NetWare
 *This property can be owned by file servers.  The data contains the twelve-byte
 *address of the server.  The first four bytes are the network address, the next
 *six are the node address, and the last two bytes are the socket address.
 */
typedef struct {
	uint8		network[4];
	uint8		node[6];
	uint8		socket[2];
} NET_ADDRESS_T;

#endif /* _NWBINDERYPROPS_H */
