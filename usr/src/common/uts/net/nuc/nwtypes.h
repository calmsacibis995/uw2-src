/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwtypes.h	1.10"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwtypes.h,v 2.51.2.1 1994/12/12 01:29:03 stevbam Exp $"

#ifndef _NET_NUC_NWTYPES_H
#define _NET_NUC_NWTYPES_H

/*
 *  Netware Unix Client 
 *
 *	Description:
 *		NetWare types.
 */

typedef	int16		flag_t;			/*	misc. flag type */
typedef	uint8		clienttype_t;	/*	client type */
typedef	uint8		dhandletype_t;	/*	directory handle type */
typedef	uint32		length_t;		/*	mainly packetLength */
typedef	uint32		shmoffset_t;	/*	shared mem offset */	
typedef	uint32		nspace_t;		/*	name space identifier */

typedef uint8		fhandle_t;		/*	file handle */
typedef	uint16		connection_t;	/*	connection number */	
typedef	uint8		volume_t;		/*	volume number */
typedef	uint8		task_t;			/*	task number */	
typedef	uint8		dhandle_t;		/*	directory handle */
typedef	uint16		direntry_t;		/*	directory entry */
typedef	uint8		nwccode_t;		/*	NetWare ccode */	
typedef	uint16		negbuffsize_t;	/*	neg buffer size */
typedef	uint8		seqnum_t;		/*	sequence number */
typedef	uint16		socket_t;		/*	XNS socket type */
typedef uint32		accessrights_t;	/*	access rights */
typedef uint8		strlen_t;		/*	NCP string length */
typedef	int16		objecttype_t;	/*	Bindery types */
typedef	uint32		objectid_t;		/*	Bindery IDs */
typedef uint16		rights_t;		/* trustee rights */

typedef int (*PFI)();				/* Ptr to int function */
typedef char *(*PFC)();				/* Ptr to char * function */
typedef void_t (*PFV)();			/* Ptr to void function */
typedef unsigned int (*PFU)();		/* Ptr to unsigned function */

#endif /* _NET_NUC_NWTYPES_H */
