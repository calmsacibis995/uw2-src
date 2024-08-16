/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/smtpState.h	1.3"
#if	!defined(SMTPSTATE_H)
#define	SMTPSTATE_H

#if	!defined(MESSAGE_OBJ)
typedef	void * message_t;
#endif

#if	!defined(CONNECTION_OBJ)
typedef void connData_t;
#endif

typedef enum smtpServerState_e
    {
    sss_init,
    sss_ready,
    sss_mail,
    sss_rcpt,
    sss_data,
    sss_dataError,
    sss_quit,
    sss_childWait,
    sss_end
    }	smtpServerState_t;

#endif
