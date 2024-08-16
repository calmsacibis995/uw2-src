/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:mail/server.h	1.1"
#if	!defined(SERVER_H)
#define	SERVER_H

#include	<netconfig.h>
#include	<netdir.h>

#if	!defined(SERVER_OBJ)
typedef void connection_t;
#endif

connection_t
    *connNew
	(
	int fd,
	struct netconfig *nconf,
	void (*readFunc)(),
	void (*freeDataFunc)(),
	void *data
	);

void
    connNewClient
	(
	struct nd_hostserv *hostserv_p,
	void (*callback)(),
	void *localData,
	void (*freeDataFunc)(),
	void (*readFunc)(),
	int reservedPort
	),
    connTerminate(connection_t *conn_p),
    connSendBinary(connection_t *conn_p, char *data, int length),
    connSend(connection_t *conn_p, char *string),
    connMainLoop();
    /*
    connSetApplicationContext(XtAppContext appContext);
    */

int
    connNewListener
	(
	char *service,
	int length,
	void *(*acceptFunc)(),
	void (*readFunc)(),
	void (*freeFunc)(),
	void *data
	),
    connInit(int debugLevel);

#endif
