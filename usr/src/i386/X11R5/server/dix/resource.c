/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:dix/resource.c	1.2"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/* $XConsortium: resource.c,v 1.87 91/10/30 14:50:54 rws Exp $ */

/*	Routines to manage various kinds of resources:
 *
 *	CreateNewResourceType, CreateNewResourceClass, InitClientResources,
 *	FakeClientID, AddResource, FreeResource, FreeClientResources,
 *	FreeAllResources, LookupIDByType, LookupIDByClass
 */

/* 
 *      A resource ID is a 32 bit quantity, the upper 3 bits of which are
 *	off-limits for client-visible resources.  The next 7 bits are
 *      used as client ID, and the low 22 bits come from the client.
 *	A resource ID is "hashed" by extracting and xoring subfields
 *      (varying with the size of the hash table).
 *
 *      It is sometimes necessary for the server to create an ID that looks
 *      like it belongs to a client.  This ID, however,  must not be one
 *      the client actually can create, or we have the potential for conflict.
 *      The 30th bit of the ID is reserved for the server's use for this
 *      purpose.  By setting CLIENT_ID(id) to the client, the SERVER_BIT to
 *      1, and an otherwise arbitrary ID in the low 22 bits, we can create a
 *      resource "owned" by the client.
 */

#include "X.h"
#include "misc.h"
#include "os.h"
#include "resource.h"
#include "dixstruct.h" 
#include "opaque.h"
#include "windowstr.h"

extern void HandleSaveSet();
extern void FlushClientCaches();
static void RebuildTable();
extern WindowPtr *WindowTable;

#define SERVER_MINID 32

#define INITBUCKETS 64
#define INITHASHSIZE 6
#define MAXHASHSIZE 11

typedef struct _Resource {
    struct _Resource	*next;
    XID			id;
    RESTYPE		type;
    pointer		value;
} ResourceRec, *ResourcePtr;
#define NullResource ((ResourcePtr)NULL)

typedef struct _ClientResource {
    ResourcePtr *resources;
    int		elements;
    int		buckets;
    int		hashsize;	/* log(2)(buckets) */
    XID		fakeID;
    XID		endFakeID;
    XID		expectID;
} ClientResourceRec;

static RESTYPE lastResourceType;
static RESTYPE lastResourceClass;
static RESTYPE TypeMask;

typedef int (*DeleteType)();

static DeleteType *DeleteFuncs = (DeleteType *)NULL;

RESTYPE
CreateNewResourceType(deleteFunc)
    DeleteType deleteFunc;
{
    RESTYPE next = lastResourceType + 1;
    DeleteType *funcs;

    if (next & lastResourceClass)
	return 0;
    funcs = (DeleteType *)xrealloc(DeleteFuncs,
				   (next + 1) * sizeof(DeleteType));
    if (!funcs)
	return 0;
    lastResourceType = next;
    DeleteFuncs = funcs;
    DeleteFuncs[next] = deleteFunc;
    return next;
}

RESTYPE
CreateNewResourceClass()
{
    RESTYPE next = lastResourceClass >> 1;

    if (next & lastResourceType)
	return 0;
    lastResourceClass = next;
    TypeMask = next - 1;
    return next;
}

ClientResourceRec clientTable[MAXCLIENTS];

/*****************
 * InitClientResources
 *    When a new client is created, call this to allocate space
 *    in resource table
 *****************/

Bool
InitClientResources(client)
    ClientPtr client;
{
    register int i, j;
 
    if (client == serverClient)
    {
	extern int dixDestroyPixmap(), FreeGC();
	extern int CloseFont(), FreeCursor();
	extern int FreeColormap(), FreeClientPixels();
	extern int OtherClientGone(), DeletePassiveGrab();

	lastResourceType = RT_LASTPREDEF;
	lastResourceClass = RC_LASTPREDEF;
	TypeMask = RC_LASTPREDEF - 1;
	if (DeleteFuncs)
	    xfree(DeleteFuncs);
	DeleteFuncs = (DeleteType *)xalloc((lastResourceType + 1) *
					   sizeof(DeleteType));
	if (!DeleteFuncs)
	    return FALSE;
	DeleteFuncs[RT_NONE & TypeMask] = (int (*)()) NoopDDA;
	DeleteFuncs[RT_WINDOW & TypeMask] = (int (*)())DeleteWindow;
	DeleteFuncs[RT_PIXMAP & TypeMask] = dixDestroyPixmap;
	DeleteFuncs[RT_GC & TypeMask] = FreeGC;
	DeleteFuncs[RT_FONT & TypeMask] = CloseFont;
	DeleteFuncs[RT_CURSOR & TypeMask] = FreeCursor;
	DeleteFuncs[RT_COLORMAP & TypeMask] = FreeColormap;
	DeleteFuncs[RT_CMAPENTRY & TypeMask] = FreeClientPixels;
	DeleteFuncs[RT_OTHERCLIENT & TypeMask] = OtherClientGone;
	DeleteFuncs[RT_PASSIVEGRAB & TypeMask] = DeletePassiveGrab;
    }
    clientTable[i = client->index].resources =
	(ResourcePtr *)xalloc(INITBUCKETS*sizeof(ResourcePtr));
    if (!clientTable[i].resources)
	return FALSE;
    clientTable[i].buckets = INITBUCKETS;
    clientTable[i].elements = 0;
    clientTable[i].hashsize = INITHASHSIZE;
    /* Many IDs allocated from the server client are visible to clients,
     * so we don't use the SERVER_BIT for them, but we have to start
     * past the magic value constants used in the protocol.  For normal
     * clients, we can start from zero, with SERVER_BIT set.
     */
    clientTable[i].fakeID = client->clientAsMask |
			    (client->index ? SERVER_BIT : SERVER_MINID);
    clientTable[i].endFakeID = (clientTable[i].fakeID | RESOURCE_ID_MASK) + 1;
    clientTable[i].expectID = client->clientAsMask;
    for (j=0; j<INITBUCKETS; j++) 
    {
        clientTable[i].resources[j] = NullResource;
    }
    return TRUE;
}

static int
Hash(client, id)
    int client;
    register XID id;
{
    id &= RESOURCE_ID_MASK;
    switch (clientTable[client].hashsize)
    {
	case 6:
	    return ((int)(0x03F & (id ^ (id>>6) ^ (id>>12))));
	case 7:
	    return ((int)(0x07F & (id ^ (id>>7) ^ (id>>13))));
	case 8:
	    return ((int)(0x0FF & (id ^ (id>>8) ^ (id>>16))));
	case 9:
	    return ((int)(0x1FF & (id ^ (id>>9))));
	case 10:
	    return ((int)(0x3FF & (id ^ (id>>10))));
	case 11:
	    return ((int)(0x7FF & (id ^ (id>>11))));
    }
    return -1;
}

static XID
AvailableID(client, id, maxid, goodid)
    register int client;
    register XID id, maxid, goodid;
{
    register ResourcePtr res;

    if ((goodid >= id) && (goodid <= maxid))
	return goodid;
    for (; id <= maxid; id++)
    {
	res = clientTable[client].resources[Hash(client, id)];
	while (res && (res->id != id))
	    res = res->next;
	if (!res)
	    return id;
    }
    return 0;
}

/*
 * Return the next usable fake client ID.
 *
 * Normally this is just the next one in line, but if we've used the last
 * in the range, we need to find a new range of safe IDs to avoid
 * over-running another client.
 */

XID
FakeClientID(client)
    register int client;
{
    register XID id, maxid;
    register ResourcePtr *resp;
    register ResourcePtr res;
    register int i;
    XID goodid;

    id = clientTable[client].fakeID++;
    if (id != clientTable[client].endFakeID)
	return id;
    id = ((Mask)client << CLIENTOFFSET) | (client ? SERVER_BIT : SERVER_MINID);
    maxid = id | RESOURCE_ID_MASK;
    goodid = 0;
    for (resp = clientTable[client].resources, i = clientTable[client].buckets;
	 --i >= 0;)
    {
	for (res = *resp++; res; res = res->next)
	{
	    if ((res->id < id) || (res->id > maxid))
		continue;
	    if (((res->id - id) >= (maxid - res->id)) ?
		(goodid = AvailableID(client, id, res->id - 1, goodid)) :
		!(goodid = AvailableID(client, res->id + 1, maxid, goodid)))
		maxid = res->id - 1;
	    else
		id = res->id + 1;
	}
    }
    if (id > maxid) {
	if (!client)
	    FatalError("FakeClientID: server internal ids exhausted\n");
	MarkClientException(clients[client]);
	id = ((Mask)client << CLIENTOFFSET) | (SERVER_BIT * 3);
	maxid = id | RESOURCE_ID_MASK;
    }
    clientTable[client].fakeID = id + 1;
    clientTable[client].endFakeID = maxid + 1;
    return id;
}

Bool
AddResource(id, type, value)
    XID id;
    RESTYPE type;
    pointer value;
{
    int client;
    register ClientResourceRec *rrec;
    register ResourcePtr res, *head;
    	
    client = CLIENT_ID(id);
    rrec = &clientTable[client];
    if (!rrec->buckets)
    {
	ErrorF("AddResource(%x, %x, %x), client=%d \n",
		id, type, (unsigned long)value, client);
        FatalError("client not in use\n");
    }
    if ((rrec->elements >= 4*rrec->buckets) &&
	(rrec->hashsize < MAXHASHSIZE))
	RebuildTable(client);
    head = &rrec->resources[Hash(client, id)];
    res = (ResourcePtr)xalloc(sizeof(ResourceRec));
    if (!res)
    {
	(*DeleteFuncs[type & TypeMask])(value, id);
	return FALSE;
    }
    res->next = *head;
    res->id = id;
    res->type = type;
    res->value = value;
    *head = res;
    rrec->elements++;
    if (!(id & SERVER_BIT) && (id >= rrec->expectID))
	rrec->expectID = id + 1;
    return TRUE;
}

static void
RebuildTable(client)
    int client;
{
    register int j;
    register ResourcePtr res, next;
    ResourcePtr **tails, *resources;
    register ResourcePtr **tptr, *rptr;

    /*
     * For now, preserve insertion order, since some ddx layers depend
     * on resources being free in the opposite order they are added.
     */

    j = 2 * clientTable[client].buckets;
    tails = (ResourcePtr **)ALLOCATE_LOCAL(j * sizeof(ResourcePtr *));
    if (!tails)
	return;
    resources = (ResourcePtr *)xalloc(j * sizeof(ResourcePtr));
    if (!resources)
    {
	DEALLOCATE_LOCAL(tails);
	return;
    }
    for (rptr = resources, tptr = tails; --j >= 0; rptr++, tptr++)
    {
	*rptr = NullResource;
	*tptr = rptr;
    }
    clientTable[client].hashsize++;
    for (j = clientTable[client].buckets,
	 rptr = clientTable[client].resources;
	 --j >= 0;
	 rptr++)
    {
	for (res = *rptr; res; res = next)
	{
	    next = res->next;
	    res->next = NullResource;
	    tptr = &tails[Hash(client, res->id)];
	    **tptr = res;
	    *tptr = &res->next;
	}
    }
    DEALLOCATE_LOCAL(tails);
    clientTable[client].buckets *= 2;
    xfree(clientTable[client].resources);
    clientTable[client].resources = resources;
}

void
FreeResource(id, skipDeleteFuncType)
    XID id;
    RESTYPE skipDeleteFuncType;
{
    int		cid;
    register    ResourcePtr res;
    register	ResourcePtr *prev, *head;
    register	int *eltptr;
    int		elements;
    Bool	gotOne = FALSE;

    if (((cid = CLIENT_ID(id)) < MAXCLIENTS) && clientTable[cid].buckets)
    {
	head = &clientTable[cid].resources[Hash(cid, id)];
	eltptr = &clientTable[cid].elements;

	prev = head;
	while (res = *prev)
	{
	    if (res->id == id)
	    {
		RESTYPE rtype = res->type;
		*prev = res->next;
		elements = --*eltptr;
		if (rtype & RC_CACHED)
		    FlushClientCaches(res->id);
		if (rtype != skipDeleteFuncType)
		    (*DeleteFuncs[rtype & TypeMask])(res->value, res->id);
		xfree(res);
		if (*eltptr != elements)
		    prev = head; /* prev may no longer be valid */
		gotOne = TRUE;
	    }
	    else
		prev = &res->next;
        }
	if(clients[cid] && (id == clients[cid]->lastDrawableID))
	{
	    clients[cid]->lastDrawable = (DrawablePtr)WindowTable[0];
	    clients[cid]->lastDrawableID = WindowTable[0]->drawable.id;
	}
    }
    if (!gotOne)
	FatalError("Freeing resource id=%X which isn't there", id);
}
void
FreeResourceByType(id, type, skipFree)
    XID id;
    RESTYPE type;
    Bool    skipFree;
{
    int		cid;
    register    ResourcePtr res;
    register	ResourcePtr *prev, *head;

    if (((cid = CLIENT_ID(id)) < MAXCLIENTS) && clientTable[cid].buckets)
    {
	head = &clientTable[cid].resources[Hash(cid, id)];

	prev = head;
	while (res = *prev)
	{
	    if (res->id == id && res->type == type)
	    {
		*prev = res->next;
		if (type & RC_CACHED)
		    FlushClientCaches(res->id);
		if (!skipFree)
		    (*DeleteFuncs[type & TypeMask])(res->value, res->id);
		xfree(res);
		break;
	    }
	    else
		prev = &res->next;
        }
	if(clients[cid] && (id == clients[cid]->lastDrawableID))
	{
	    clients[cid]->lastDrawable = (DrawablePtr)WindowTable[0];
	    clients[cid]->lastDrawableID = WindowTable[0]->drawable.id;
	}
    }
}

/*
 * Change the value associated with a resource id.  Caller
 * is responsible for "doing the right thing" with the old
 * data
 */

Bool
ChangeResourceValue (id, rtype, value)
    XID	id;
    RESTYPE rtype;
    pointer value;
{
    int    cid;
    register    ResourcePtr res;

    if (((cid = CLIENT_ID(id)) < MAXCLIENTS) && clientTable[cid].buckets)
    {
	res = clientTable[cid].resources[Hash(cid, id)];

	for (; res; res = res->next)
	    if ((res->id == id) && (res->type == rtype))
	    {
		if (rtype & RC_CACHED)
		    FlushClientCaches(res->id);
		res->value = value;
		return TRUE;
	    }
    }
    return FALSE;
}

void
FreeClientResources(client)
    ClientPtr client;
{
    register ResourcePtr *resources;
    register ResourcePtr this;
    int j;

    /* This routine shouldn't be called with a null client, but just in
	case ... */

    if (!client)
	return;

    HandleSaveSet(client);

    resources = clientTable[client->index].resources;
    for (j=0; j < clientTable[client->index].buckets; j++) 
    {
        /* It may seem silly to update the head of this resource list as
	we delete the members, since the entire list will be deleted any way, 
	but there are some resource deletion functions "FreeClientPixels" for 
	one which do a LookupID on another resource id (a Colormap id in this
	case), so the resource list must be kept valid up to the point that
	it is deleted, so every time we delete a resource, we must update the
	head, just like in FreeResource. I hope that this doesn't slow down
	mass deletion appreciably. PRH */

	ResourcePtr *head;

	head = &resources[j];

        for (this = *head; this; this = *head)
	{
	    RESTYPE rtype = this->type;
	    *head = this->next;
	    if (rtype & RC_CACHED)
		FlushClientCaches(this->id);
	    (*DeleteFuncs[rtype & TypeMask])(this->value, this->id);
	    xfree(this);	    
	}
    }
    xfree(clientTable[client->index].resources);
    clientTable[client->index].buckets = 0;
}

void
FreeAllResources()
{
    int	i;

    for (i=0; i<currentMaxClients; i++) 
    {
        if (clientTable[i].buckets) 
	    FreeClientResources(clients[i]);
    }
}

Bool
LegalNewID(id, client)
    XID id;
    register ClientPtr client;
{
    return ((client->clientAsMask == (id & ~RESOURCE_ID_MASK)) &&
	    ((clientTable[client->index].expectID <= id) ||
	     !LookupIDByClass(id, RC_ANY)));
}

/*
 *  LookupIDByType returns the object with the given id and type, else NULL.
 */ 
pointer
LookupIDByType(id, rtype)
    XID id;
    RESTYPE rtype;
{
    int    cid;
    register    ResourcePtr res;

    if (((cid = CLIENT_ID(id)) < MAXCLIENTS) && clientTable[cid].buckets)
    {
	res = clientTable[cid].resources[Hash(cid, id)];

	for (; res; res = res->next)
	    if ((res->id == id) && (res->type == rtype))
		return res->value;
    }
    return (pointer)NULL;
}

/*
 *  LookupIDByClass returns the object with the given id and any one of the
 *  given classes, else NULL.
 */ 
pointer
LookupIDByClass(id, classes)
    XID id;
    RESTYPE classes;
{
    int    cid;
    register    ResourcePtr res;

    if (((cid = CLIENT_ID(id)) < MAXCLIENTS) && clientTable[cid].buckets)
    {
	res = clientTable[cid].resources[Hash(cid, id)];

	for (; res; res = res->next)
	    if ((res->id == id) && (res->type & classes))
		return res->value;
    }
    return (pointer)NULL;
}
