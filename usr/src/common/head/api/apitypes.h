/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/apitypes.h	1.1"
#ident	"$Header: $"

/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    include/api/apitypes.h 1.4 (Novell) 10/16/91
 */

/*
**	NetWare/SRC
**
**	Description:
**		NetWare types.
*/
#ifndef NWTYPES
#define NWTYPES
#ifndef lint
	typedef void		void_t;			/*	void */
#else
	typedef int			void_t;
#endif

#ifdef	NOTDEF	
	typedef	int16		flag_t;			/*	misc. flag type */
#endif	/* NOTDEF */
	typedef	uint8		clienttype_t;	/*	client type */
	typedef	uint8		dhandletype_t;	/*	directory handle type */
	typedef	uint32		length_t;		/*	mainly packetLength */
	typedef	uint32		shmoffset_t;	/*	shared mem offset */	
#ifdef cw
	typedef	int8		boolean_t;		/*	boolean flag */
	typedef int32		hostid_t;		/*	host id for a user */
#endif
#ifdef	UNDEF
	typedef uint32		spl_t;			/*	used in the test & set code */
#endif

#ifdef AIX
#include <sys/types.h>
#define	fhandle_t	nwfhandle_t
#endif
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
	typedef unsigned int (*PFU)();		/* Ptr to unsigned function */
#endif
