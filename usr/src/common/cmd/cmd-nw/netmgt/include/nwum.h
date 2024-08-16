/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/include/nwum.h	1.1"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/include/nwum.h,v 1.1 1994/02/03 00:10:25 rbell Exp $
 */

/* filename: nwum.h  */
#ifndef _NWUM_H_
#define _NWUM_H_

#ifndef OK
#define OK 0
#endif

#ifndef NOTOK
#define NOTOK -1
#endif

#define MAX_NM_STRING  50  /* Maximum length of the string used in*/

/* Defines the specific trap numbers */

/* Watchdog alerts */
#define NWTRAP_CONN_TERMINATED_BY_WATCHDOG                  32

#define NWTRAP_LOGIN_DISABLED                               40
#define NWTRAP_LOGIN_ENABLED                                41

/* NCP alerts */
#define NWTRAP_OUT_OF_SERVER_CONN                           137
#define NWTRAP_USR_ACCT_DISABLED_BY_A_USR                   139

#define NWTRAP_USE_OF_UN_ENCRYPTED_PWD                      140

#define NWTRAP_LOGIN_DISABLED_BY_CONSOLE                    185
#define NWTRAP_LOGIN_ENABLED_BY_CONSOLE                     186

/* Bindery alerts */
#define NWTRAP_BINDERY_OPN_REQTD_BY_SRVR                    202
#define NWTRAP_BINDERY_OPN_REQTD_BY_USR                     203
#define NWTRAP_BINDERY_CLOSED_BY_SRVR                       204
#define NWTRAP_BINDERY_CLOSED_BY_USR                        205

#define NWTRAP_INTRUDER_LOCK_OUT                            206

/* Directory Services */
#define NWTRAP_DS_OPEN_FAILED                               207
#define NWTRAP_DS_OPEN_FAILED_INCONSISTENT_DATABASE         208
#define NWTRAP_DS_OPEN_SUCCEEDED                            209
#define NWTRAP_DS_CLOSE_SUCCEEDED                           210
#define NWTRAP_SKULKING_ERROR                               211
#define NWTRAP_DS_INTRUDER_NO_ADDR                          212

#define NWTRAP_SERVER_DOWN_AT_SERVER                        222

/* login/logout */
#define NWTRAP_LOGIN_CONNECTION                             230
#define NWTRAP_LOGOUT_CONNECTION                            232

#define NWTRAP_INVALID_SIGNATURE_STRING                     246
#define NWTRAP_INVALID_PBURST_SIGNATURE                     248

#define NWTRAP_DS_AUDIT_FILE_WRITE_ERROR                    266
#define NWTRAP_DS_AUDIT_FILE_FULL                           267
#define NWTRAP_DS_AUDIT_THRESHOLD_OVERFLOW                  268
#define NWTRAP_DS_AUDIT_MEMORY_ALLOCATION                   269

#define NWTRAP_SERVER_UP                                    270

#endif


