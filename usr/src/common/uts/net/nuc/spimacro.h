/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spimacro.h	1.11"
#ifndef _NET_NUC_SPIL_SPIMACRO_H
#define _NET_NUC_SPIL_SPIMACRO_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spimacro.h,v 2.51.2.1 1994/12/12 01:34:01 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: spimacro.h
 *	ABSTRACT: macro routines called in the SPI layer to determine
 *	which service protocol to switch to.
 */

#define SPI_INIT( OPS )  (*(OPS)->spi_init)()

#define SPI_FREE( OPS ) (*(OPS)->spi_free)()

#define SPI_SCREATE( OPS, SADDRESS, CREDS, TPDOMAIN, SSPTR ) \
	(*(OPS)->spi_screate)( SADDRESS, CREDS, TPDOMAIN, SSPTR )

#define SPI_SOPEN( OPS, SSPTR, CID, MODE, TASK, STPTR ) \
	(*(OPS)->spi_sopen)( SSPTR, CID, MODE, TASK, STPTR )

#define SPI_SCLOSE( OPS, STPTR ) (*(OPS)->spi_sclose)( STPTR )

#define SPI_AUTH( OPS, STPTR, ATYPE, OBJID, AKEY, AKEYLEN ) \
	(*(OPS)->spi_auth)( STPTR, ATYPE, OBJID, AKEY, AKEYLEN )

#define SPI_LICENSE( OPS, STPTR, FLAGS ) \
	(*(OPS)->spi_license)( STPTR, FLAGS )

/*
 *	Management portal (nwmp) requests
 */
#define SPI_RAW( OPS, STPTR, TOKEN, HDR, SHLN, RHLN, HKRES, DAT, SDLN, RDLN, DKRES ) \
	(*(OPS)->spi_raw)(STPTR, TOKEN, HDR, SHLN, RHLN, HKRES, DAT, SDLN, RDLN, DKRES )

#define SPI_REGISTER_RAW( OPS, STPTR, TKN, REF ) \
	(*(OPS)->spi_register_raw)( STPTR, TKN, REF )

#define SPI_RELINQUISH_RAW( OPS, STPTR, TKN ) \
	(*(OPS)->spi_relinquish_raw)( STPTR, TKN )

#define SPI_SSBU_NAME( OPS, TASK, NAME ) \
	(*(OPS)->spi_get_service_task_name)( TASK, NAME )
	
#define SPI_GET_MESSAGE( OPS, MESSAGE ) \
	(*(OPS)->spi_get_service_message)( MESSAGE )

#define CHECK_CONN_REFERENCE( ccode, nwmpD_conn_ref, task_conn_ref ) \
	ccode = SUCCESS; \
	if( nwmpD_conn_ref != NULL ) \
		if( nwmpD_conn_ref != task_conn_ref ) \
			ccode = SPI_NO_SUCH_TASK 
	
#endif /* _NET_NUC_SPIL_SPIMACRO_H */
