/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:sharedObjects/dns/conn.c	1.1"
#include	<stdio.h>
#include	<netdir.h>
#include	<malloc.h>
#include	<mail/server.h>
#include	<mail/table.h>

typedef struct conn_s
    {
    void
	*conn_localData,
	(*conn_readCallback)(),
	(*conn_freeLocal)(),
	(*conn_callback)();

    char
	*conn_incommingMessage,
	conn_byteCountBuffer[2];

    unsigned
	conn_curMsgPos,
	conn_byteCount,
	conn_openCount,
	conn_splitByteCount:1,
	conn_freeLock:1;

    connection_t
	*conn_conn;
    }	conn_t;

static table_t
    *HostTable;

static int
    DebugLevel = 0;

void
    connectionFree(conn_t *conn_p)
	{
	connection_t
	    *tmpConn_p;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"connectionFree(0x%x) Entered.\n",
		(int) conn_p
		);
	    }

	if(conn_p == NULL)
	    {
	    }
	else if(--conn_p->conn_openCount > 0)
	    {
	    }
	else if(conn_p->conn_freeLock)
	    {
	    }
	else if(tableDeleteEntryByValue(HostTable, conn_p))
	    {
	    }
	else
	    {
	    conn_p->conn_freeLock = 1;
	    if(conn_p->conn_freeLocal != NULL) conn_p->conn_freeLocal(conn_p->conn_localData);
	    if(conn_p->conn_conn != NULL)
		{
		tmpConn_p = conn_p->conn_conn;
		connTerminate(tmpConn_p);
		}

	    free(conn_p);
	    }

	if(DebugLevel > 4) (void) fprintf(stderr, "connectionFree() Exited.\n");
	}

static void
    readFunc(connection_t *connection_p, conn_t *conn_p, char *data, int nbytes)
	{
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"readFunc(0x%x, 0x%x, 0x%x, %d) Entered.\n",
		(int) connection_p,
		(int) conn_p,
		(int) data,
		nbytes
		);

	    (void) fprintf
		(
		stderr,
		"\t&localData = 0x%x.\n",
		(int) &conn_p->conn_localData
		);
	    }

	while(nbytes > 0)
	    {
	    if(conn_p->conn_incommingMessage == 0)
		{
		if(conn_p->conn_splitByteCount)
		    {
		    conn_p->conn_splitByteCount = 0;
		    conn_p->conn_byteCountBuffer[1] = *data++;
		    nbytes--;
		    conn_p->conn_byteCount = _getshort(conn_p->conn_byteCountBuffer);
		    if(DebugLevel > 4)
			{
			(void) fprintf
			    (
			    stderr,
			    "\tnbytes = %d.  message is %d bytes long.\n",
			    nbytes,
			    conn_p->conn_byteCount
			    );
			}
		    }
		else if(nbytes >= 2)
		    {
		    conn_p->conn_byteCount = _getshort(data);
		    data += 2;
		    nbytes -= 2;
		    if(DebugLevel > 4)
			{
			(void) fprintf
			    (
			    stderr,
			    "\tnbytes = %d.  message is %d bytes long.\n",
			    nbytes,
			    conn_p->conn_byteCount
			    );
			}
		    }
		else
		    {
		    if(DebugLevel > 4)
			{
			(void) fprintf
			    (
			    stderr,
			    "\tnbytes = %d.  Split byte count.\n",
			    nbytes
			    );
			}

		    conn_p->conn_splitByteCount = 1;
		    conn_p->conn_byteCountBuffer[0] = *data++;
		    nbytes--;
		    continue;
		    }

		if(nbytes < conn_p->conn_byteCount)
		    {
		    conn_p->conn_incommingMessage = malloc(conn_p->conn_byteCount);
		    conn_p->conn_curMsgPos = 0;
		    }
		else
		    {
		    if(DebugLevel > 4)
			{
			(void) fprintf
			    (
			    stderr,
			    "\tsending whole message %d bytes left.\n",
			    nbytes - conn_p->conn_byteCount
			    );
			}

		    conn_p->conn_readCallback
			(
			conn_p,
			conn_p->conn_localData,
			data,
			conn_p->conn_byteCount
			);

		    if(DebugLevel > 8)
			{
			(void) fprintf
			    (
			    stderr,
			    "\tlocalData = 0x%x.\n",
			    (int) conn_p->conn_localData
			    );
			}

		    data += conn_p->conn_byteCount;
		    nbytes -= conn_p->conn_byteCount;
		    continue;
		    }
		}

	    if(conn_p->conn_curMsgPos + nbytes >= conn_p->conn_byteCount)
		{
		if(DebugLevel > 4)
		    {
		    (void) fprintf
			(
			stderr,
			"\tsending accumulated message %d bytes left.\n",
			conn_p->conn_curMsgPos + nbytes - conn_p->conn_byteCount
			);
		    }

		memcpy
		    (
		    conn_p->conn_incommingMessage + conn_p->conn_curMsgPos,
		    data,
		    conn_p->conn_byteCount - conn_p->conn_curMsgPos
		    );
		
		conn_p->conn_readCallback
		    (
		    conn_p,
		    conn_p->conn_localData,
		    conn_p->conn_incommingMessage,
		    conn_p->conn_byteCount
		    );

		data += conn_p->conn_byteCount - conn_p->conn_curMsgPos;
		nbytes -= conn_p->conn_byteCount - conn_p->conn_curMsgPos;
		free(conn_p->conn_incommingMessage);
		conn_p->conn_incommingMessage = NULL;
		}
	    else
		{
		if(DebugLevel > 4)
		    {
		    (void) fprintf(stderr, "\taccumulating message.\n");
		    }

		memcpy
		    (
		    conn_p->conn_incommingMessage + conn_p->conn_curMsgPos,
		    data,
		    nbytes
		    );
		
		conn_p->conn_curMsgPos += nbytes;
		nbytes = 0;
		}
	    }

	if(DebugLevel > 4) (void) fprintf(stderr, "readFunc() Exited.\n");
	}

static void
    gotNew(conn_t *conn_p, connection_t *connection_p)
	{
	if(conn_p->conn_callback == NULL)
	    {
	    /* no callback */
	    connectionFree(conn_p);
	    }
	else if(connection_p == NULL)
	    {
	    /* Could not connect */
	    conn_p->conn_freeLock = 1;
	    conn_p->conn_callback(conn_p->conn_localData, NULL);
	    conn_p->conn_freeLock = 0;
	    /* Free will be done in server library. */
	    }
	else
	    {
	    conn_p->conn_conn = connection_p;
	    conn_p->conn_callback(conn_p->conn_localData, conn_p);
	    }
	}

static conn_t
    *connectionNew(char *hostName, void *localData, void (*callback)())
	{
	conn_t
	    *result;
	
	struct nd_hostserv
	    hostserv;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"connectionNew(%s, 0x%x, 0x%x) Entered.\n",
		hostName,
		(int) localData,
		(int) callback
		);
	    }

	if((result = (conn_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else
	    {
	    hostserv.h_host = hostName;
	    hostserv.h_serv = "domain";
	    result->conn_localData = localData;
	    result->conn_callback = callback;
	    result->conn_openCount = 1;

	    tableAddEntry(HostTable, hostName, (void *)result, connectionFree);

	    connNewClient
		(
		&hostserv,
		gotNew,
		result,
		connectionFree,
		readFunc,
		0
		);
	    }

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"connectionNew() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return(result);
	}

conn_t
    *connectionGetHost(char *hostName, void *localData, void (*callback)())
	{
	conn_t
	    *result;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"connectionGetHost(%s, 0x%x, 0x%x) Entered.\n",
		hostName,
		(int) localData,
		(int) callback
		);
	    }
	if((result = (conn_t *)tableGetValueByNoCaseString(HostTable, hostName)) != NULL)
	    {
	    result->conn_openCount++;
	    callback(localData, result);
	    }
	else if((result = connectionNew(hostName, localData, callback)) == NULL)
	    {
	    }

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"connectionGetHost() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return(result);
	}

void
    connectionSend(conn_t *conn_p, char *data, int nbytes, void (*readCallback)())
	{
	conn_p->conn_readCallback = readCallback;
	if(conn_p->conn_conn != NULL) connSendBinary(conn_p->conn_conn, data, nbytes);
	}

void
    connectionInit(int debugLevel)
	{
	DebugLevel = debugLevel;
	connInit(DebugLevel);
	if(HostTable == NULL) HostTable = tableNew();
	}
