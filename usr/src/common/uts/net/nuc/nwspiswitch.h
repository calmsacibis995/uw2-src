/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwspiswitch.h	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwspiswitch.h,v 2.51.2.1 1994/12/12 01:28:39 stevbam Exp $"

#ifndef _NET_NUC_SPIL_NWSPISWITCH_H
#define _NET_NUC_SPIL_NWSPISWITCH_H

/*
 *  Netware Unix Client
 *
 *	  MODULE: nwspiswitch.h
 *	ABSTRACT: Switch table structure.
 *
 *	NOTE: This module should be included in each service protocol
 *        engine package in order to be installed correctly.
 */

/*
 *	Define the operations that are performed by spi in 
 *	normal mode.
 */
typedef struct {
			/*
			 *	Maintenance/Configuration requests
			 */
	ccode_t	(*spi_init)(),	
			(*spi_free)(),
			(*spi_screate)(),
			(*spi_sopen)(),
			(*spi_sclose)(),
			(*spi_auth)(),
			(*spi_license)(),

			/*
			 *	Raw service protocol requests
			 */
			(*spi_raw)(),
			(*spi_register_raw)(),
			(*spi_relinquish_raw)(),
			(*spi_get_service_task_name)(),
#ifndef FS_ONLY
			(*spi_get_service_message)();
#else
			(*spi_get_service_message)(),
#endif /* FS_ONLY */
#ifdef FS_ONLY
			(*spi_sdelete)(),

			/*
			 *	Generic file service requests
			 */
			(*spi_nopen)(),
			(*spi_nclose)(),
			(*spi_ncreate)(),
			(*spi_ndelete)(),
			(*spi_nrename)(),
			(*spi_nget_nsinfo)(),
			(*spi_nset_nsinfo)(),
			(*spi_ncheck_access)(),

			/*
			 *	File service requests
			 */
			(*spi_fread)(),
			(*spi_fwrite)(),
			(*spi_flink)(),
			(*spi_ftrunc)(),
			(*spi_flock)(),
			(*spi_funlock)(),
			(*spi_fsize)(),

			/*
			 *	Directory service requests
			 */
			(*spi_dget_ents)(),
			(*spi_dget_path)(),

			/*
			 *	Volume information requests
			 */
			(*spi_vopen)(),
			(*spi_vclose)(),
			(*spi_vget_info)();
#endif /* FS_ONLY */
} SPI_OPS_T;

#endif /* _NET_NUC_SPIL_NWSPISWITCH_H */
