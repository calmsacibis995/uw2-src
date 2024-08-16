/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/nucam_nwc_proto.h	1.2"
#ifndef	NUCAM_NWC_PROTO_H
#define NUCAM_NWC_PROTO_H
/*
 * file: nucam_nwc_proto.h
 *
 *	Purpose: provide the necessary prototypes to keep lint happy.
 *	There needs to be a single all-purpose file that collects
 *	together all the prototypes that nuc code needs. For now,
 *	this is a special file within fs/nucam only.
 */
extern uint_t 	NVLTleave();
extern void 	NWtlGetCredUserID();
extern void 	NWtlGetCredGroupID();
extern void 	NWtlSetCredUserID();
extern void 	NWtlSetCredGroupID();
extern void 	NWtlSetCredPid();
extern ccode_t	NWtlPSemaphore();
extern ccode_t	NWtlVSemaphore_l();
extern ccode_t	NWtlCreateAndSetSemaphore();
extern ccode_t	NWtlDestroySemaphore();

#endif	/* NUCAM_NWC_PROTO_H */
