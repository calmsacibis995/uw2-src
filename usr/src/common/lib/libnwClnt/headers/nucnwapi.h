/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:headers/nucnwapi.h	1.3"
#ifndef _NUCNWAPI_H_
#define _NUCNWAPI_H_

/*
**	NUC function prototypes
*/
N_EXTERN_FUNC_C (nint) initreq( INIT_REQ_T *, DS_INIT_REQ_T * );
N_EXTERN_FUNC_C (nint) sys_close_conn( nuint32 );
N_EXTERN_FUNC_C (nint) get_max_conns( pnuint32, pnuint32 );
N_EXTERN_FUNC_C (nint) get_server_context( nuint32, pnuint, pnuint );
N_EXTERN_FUNC_C (nint) license_conn( nuint32 );
N_EXTERN_FUNC_C (nint) unlicense_conn( nuint32 );
N_EXTERN_FUNC_C (nint) handle_info( nuint32, pnint );
N_EXTERN_FUNC_C (nint) ncp_request( nuint32, nuint8, pnuint8, nuint, pnuint8, pnuint );
N_EXTERN_FUNC_C (nint) make_conn_permanent( nuint32 );
N_EXTERN_FUNC_C (nint) get_conn_info( nuint32, nuint, nuint, pnuint8 );
N_EXTERN_FUNC_C (nint) close_conn( nuint32 );
N_EXTERN_FUNC_C (nint) set_conn_info( nuint32, nuint, nuint, pnuint8 );
N_EXTERN_FUNC_C (nint) scan_conn_info( pnuint32, nuint, pnstr, nuint, nuint, nuint, nuint, pnuint32, pnstr );
N_EXTERN_FUNC_C (nint) open_conn_by_reference( nuint32, pnuint32, nuint32 );
N_EXTERN_FUNC_C (nint) open_conn_by_addr( pNWCTranAddr, pnuint32, nuint32 );
N_EXTERN_FUNC_C (nint) open_conn_by_name( char *, nuint, pnuint32, nuint32 );
N_EXTERN_FUNC_C (nint) get_server_name_from_handle( nuint32, char ** );
N_EXTERN_FUNC_C (nint) get_server_address_from_handle( nuint32, struct netbuf ** );
N_EXTERN_FUNC_C (nint) get_conn_handle_list( char *, nint );

#endif /* _NUCNWAPI_H_ */
