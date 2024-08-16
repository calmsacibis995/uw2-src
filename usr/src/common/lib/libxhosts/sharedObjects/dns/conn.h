/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:sharedObjects/dns/conn.h	1.1"
typedef void conn_t;

conn_t
    *connectionGetHost(char *hostName, void *localData, void (*callback)());

void
    connectionInit(int debugLevel),
    connectionFree(conn_t *conn_p),
    connectionSend(conn_t *conn_p, char *data, int nbytes, void (*readCallback)());
