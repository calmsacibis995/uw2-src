/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/smtpType.h	1.1"
extern int
    DebugLevel;

#if	!defined(SMTP_TYPE_H)
#define	SMTP_TYPE_H

#if	!defined(MESSAGE_OBJ)
typedef void message_t;
#endif

#if	!defined(CONNECTION_OBJ)
typedef void conn_t;
#endif

#if	!defined(MSGCONN_OBJ)
typedef void msgConn_t;
#endif

#endif
