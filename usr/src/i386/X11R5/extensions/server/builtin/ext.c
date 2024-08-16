/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)builtinext:server/builtin/ext.c	1.9"

#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <libgen.h>

#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"

#include "os.h"
#include "../os/osdep.h"
#include "dixstruct.h"

#include "extnsionst.h"
#define BUILTIN_SERVER_		/* don't want Xlib structures */
#include "builtinstr.h"
#include "BIprivate.h"

/****************************************************************************
 *	Forward Declarations
 */
static int	BuiltinOnce(char * name);
static int	ProcQueryVersion(ClientPtr client);
static int	ValidateClient(ClientPtr client, char * name);

/****************************************************************************
 *	Define global/static variables and #defines, and
 *	Declare externally referenced variables
 */
static int	BuiltinReqCode;
static int	BuiltinEventBase;

/****************************************************************************
 *
 *		PRIVATE FUNCTIONS
 */
/****************************************************************************
 * BuiltinOnce- keep a list of clients that have run builtin so that they
 *	cannot be restarted for a run of the server.  Store the basename
 *	(name has been "dup'ed" by caller).
 */
static int
BuiltinOnce(char * name)
{
    static char *	names[MAX_ICLIENTS];
    static int		cnt = 0;
    char *		bname;
    char **		p;

    if (name == NULL)
    {
	for (p = names; p < names + cnt; p++)
	    Xfree(*p);
	cnt = 0;
	return(1);
    }

    bname = basename(name);
    for (p = names; p < names + cnt; p++)
	if (strcmp(bname, *p) == 0)
	    return(0);

    names[cnt++] = Xstrdup(bname);
    return(1);
}

/****************************************************************************
 * MainProc- process request to "build in" client.
 */
static int
MainProc(ClientPtr client)
{
    REQUEST(xReq);

    switch(stuff->data)
    {
    case X_BuiltinQueryVersion:
	return (ProcQueryVersion(client));

    case X_BuiltinRunClient:
	return (BuiltinProcRunClient(client));

    default:
	return(BadRequest);
    }
}

/****************************************************************************
 * SMainProc- handle requests in byte-swapped format.  Should never
 * be called.
 */
static int
SMainProc(ClientPtr client)
{
    fprintf(stderr, "SBuiltinMainProc: should not get called\n");
    return(SProcSimpleReq(client));
}

/****************************************************************************
 * ProcQueryVersion
 */
static int
ProcQueryVersion(ClientPtr client)
{
    REQUEST(xBuiltinQueryVersionReq);
    xBuiltinQueryVersionReply reply;
    char * name = (char *)&stuff[1];	/* variable data begins after req */

    REQUEST_AT_LEAST_SIZE(xBuiltinQueryVersionReq);

    reply.type			= X_Reply;
    reply.length		= 0;
    reply.sequenceNumber	= client->sequence;
    reply.majorVersion		= BUILTIN_MAJOR_VERSION;
    reply.minorVersion		= BUILTIN_MINOR_VERSION;
    reply.valid			= ValidateClient(client, name);
#ifdef CANT_BE
    if (client->swapped) {
    	swaps(&reply.sequenceNumber, n);
	swaps(&reply.majorVersion, n);
	swaps(&reply.minorVersion, n);
    }
#endif
    WriteToClient(client, sizeof(xBuiltinQueryVersionReply), (char *)&reply);
    return(client->noClientException);
}

/****************************************************************************
 * ResetProc-
 */
static void
ResetProc(ExtensionEntry * extEntry)
{
    /* (void)BuiltinOnce(NULL); */
}

/****************************************************************************
 * ValidateClient-
 *   1	Must be local connection.  These are flagged in AllStreams mask.
 *   2	Don't exceed max number of internal clients.
 *   3	Can only build-in one copy of client.
 *   4	Can only run client builtin once (no restarts).
 */
static int
ValidateClient(ClientPtr client, char * name)
{
    extern fd_set	AllStreams;
    char		bname[PATH_MAX];
    char		argv_name[PATH_MAX];
    int			i;

    if ((NumClients >= MAX_ICLIENTS) ||
	!FD_ISSET(CLIENT_FD(client), &AllStreams))
	return(0);

    strcpy(argv_name, name);
    strcpy(bname, basename(argv_name));
    for(i = 0; i < NumClients; i++)
    {
	strcpy(argv_name, ARGV0(i));
	if (strcmp(basename(argv_name), bname) == 0)
	    return(0);
    }
    /* Only call BuiltinOnce in the end if client still valid */
    return(BuiltinOnce(bname));
}

/****************************************************************************
 *
 *		PUBLIC FUNCTIONS
 */

/****************************************************************************
 * BuiltinSendExecEvent- send exec "event" to surrogate client.
 */
void
BuiltinSendExecEvent(ClientPtr client)
{
    xBuiltinExecNotifyEvent	event;

    event.type			= BuiltinEventBase + BuiltinExecNotify;
    event.sequenceNumber	= client->sequence;

    (void)WriteEventsToClient(client, 1, (xEvent *)&event);
}

/***************************************************************************
 * BuiltinSendExitEvent- send exit "event" and exit code to surrogate client.
 */
void
BuiltinSendExitEvent(ClientPtr client, int exit_code)
{
    xBuiltinExitNotifyEvent	event;

    event.type			= BuiltinEventBase + BuiltinExitNotify;
    event.exit_code		= exit_code;
    event.sequenceNumber	= client->sequence;

    (void)WriteEventsToClient(client, 1, (xEvent *)&event);

    /* Flush to ensure client gets exit code immediately */
    (void)FlushClient(client, (OsCommPtr)client->osPrivate, NULL, 0);
}

/****************************************************************************
 * BuiltinExtentionInit- Called from InitExtensions.
 */
void
BuiltinExtensionInit()
{
    ExtensionEntry * extEntry =
	AddExtension(BUILTIN_EXT_NAME, BuiltinNumberEvents,
		     BuiltinNumberErrors, MainProc,
		     /*SProcSimpleReq*/SMainProc,
		     ResetProc, StandardMinorOpcode);
    if (extEntry)
    {
	BuiltinReqCode		= extEntry->base;
	BuiltinEventBase	= extEntry->eventBase;
    }
    else
	FatalError("BuiltinExtensionInit: AddExtensions failed\n");
}
