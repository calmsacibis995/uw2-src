/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smsdrerr.h	1.1"

#if !defined(_SMSDRERR_H_INCLUDED)
#define _SMSDRERR_H_INCLUDED
#define NWSMDR_ERROR_CODE(err)		   (0xFFFE0000L | err)

#define NWSMDR_BEGIN_ERROR_CODES		NWSMDR_ERROR_CODE(0xFFFF)

#define NWSMDR_INVALID_CONNECTION		NWSMDR_ERROR_CODE(0xFFFF) /* An invalid connection handle was passed to the SMDR */
#define NWSMDR_INVALID_PARAMETER		NWSMDR_ERROR_CODE(0xFFFE) /* One or more of the paremeters is NULL or invalid */
#define NWSMDR_OUT_OF_MEMORY			NWSMDR_ERROR_CODE(0xFFFD) /* SMDR memory allocation failed */
#define NWSMDR_TRANSPORT_FAILURE		NWSMDR_ERROR_CODE(0xFFFC) /* The transport mechanism has failed */
#define NWSMDR_UNSUPPORTED_FUNCTION		NWSMDR_ERROR_CODE(0xFFFB) /* The requested function is not supported by the SMDR */	
#define NWSMDR_MODULE_ALREADY_EXPORTED	NWSMDR_ERROR_CODE(0xFFFA)
#define NWSMDR_DECRYPTION_FAILURE		NWSMDR_ERROR_CODE(0xFFF9)
#define NWSMDR_ENCRYPTION_FAILURE		NWSMDR_ERROR_CODE(0xFFF8)
#define NWSMDR_TSA_NOT_LOADED			NWSMDR_ERROR_CODE(0xFFF7)
#define NWSMDR_NO_SUCH_SMDR				NWSMDR_ERROR_CODE(0xFFF6)
#define NWSMDR_SMDR_CONNECT_FAILURE		NWSMDR_ERROR_CODE(0xFFF5)
#define NWSMDR_NO_MORE_DATA				NWSMDR_ERROR_CODE(0xFFF4)
#define NWSMDR_NO_SOCKETS				NWSMDR_ERROR_CODE(0xFFF3)
#define NWSMDR_INVALID_PROTOCOL			NWSMDR_ERROR_CODE(0xFFF2)
#define NWSMDR_NO_MORE_CONNECTIONS		NWSMDR_ERROR_CODE(0xFFF1)
#define NWSMDR_NO_SUCH_TSA				NWSMDR_ERROR_CODE(0xFFF0)
#define NWSMDR_INVALID_MESSAGE_NUMBER	NWSMDR_ERROR_CODE(0xFFEF)

#define NWSMDR_END_ERROR_CODES			NWSMDR_ERROR_CODE(0xFFEE)
#endif

