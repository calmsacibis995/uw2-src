/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ncpdvops.h	1.7"
#ifndef _NET_NUC_NCP_NCPDVOPS_H
#define _NET_NUC_NCP_NCPDVOPS_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ncpdvops.h,v 2.51.2.1 1994/12/12 01:25:21 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncpdvops.h
 *	ABSTRACT: NCP DOS name space version operations structure 
 */

/* #include <nwctypes.h> */

typedef struct ncpDVOps {

	ccode_t (*ncp_fopen)(),		/* File NCP's */
			(*ncp_fclose)(),
			(*ncp_fread)(),
			(*ncp_fwrite)(),
			(*ncp_fcreate)(),
			(*ncp_fdelete)(),
			(*ncp_frename)(),
			(*ncp_flock)(),
			(*ncp_funlock)(),
			(*ncp_ftrunc)(),
	
			/*
			 *	Directory NCP's 
			 */
			(*ncp_dopen)(),	
			(*ncp_dclose)(),
			(*ncp_dcreate)(),
			(*ncp_ddelete)(),
			(*ncp_drename)(),
			(*ncp_dset_nsinfo)(),
			(*ncp_dget_nsinfo)(),
			(*ncp_dget_erights)(),
			(*ncp_dget_path)(),
			(*ncp_dget_entries)(),
	
			(*ncp_vget_num)(),	/* Volume NCP's */
			(*ncp_vget_info)();
} ncp_dvOps_t;

#endif /* _NET_NUC_NCP_NCPDVOPS_H */
