/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/lipmx/rripx.c	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: rripx.c,v 1.7 1994/09/19 14:59:39 vtag Exp $"
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
 */

/* rripx.c
 * This module implements IPX's portion of the Replaceable
 * Router NWU interface spec.
 */
#ifdef _KERNEL_HEADERS
#include <net/nw/ipx/lipmx/rripx.h>
#else
#include "rripx.h"
#endif /* _KERNEL_HEADERS */

FSTATIC void	*RegisterRRouter(RROUTERMethods_t *rrouterMethods);
FSTATIC int		 DeregisterRRouter(void *rrouterToken);

RRIPXMethods_t	RRIPX = {
	RegisterRRouter,			/* Register Router */
	DeregisterRRouter,			/* De-register Router */
	LIPMXsendData,				/* SendData */
	LIPMXmapSapLanToNetwork,	/* MapSapLanToNetwork */
	LIPMXuseMethodWithLanKey,	/* UseMethodWithLanKey */
	0							/* Count of calls into router */
};

extern RROUTERMethods_t		RROUTER = { 0 };
static RROUTERMethods_t		DefaultRouter = { 0 };
static RROUTERMethods_t		*defaultRRouterToken = NULL;
static RROUTERMethods_t		*currentRRouterToken = NULL;


/*
 * void * RegisterRRouter(RROUTERMethods_t *rrouterMethods)
 *	Insure all rrouterMethods are defined then replace entries in a 
 *	global table of RRouterEntryPoints with func pointers passed in arg.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 *	
 */
void *
RegisterRRouter(RROUTERMethods_t *rrouterMethods)
{
	NTR_ENTER(1, rrouterMethods, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
            "Enter RegisterRRouter"));

	/* Assume this always happens from a default state.
	** (except when called from IPXDeregisterRouter(),
	** when it just looks like we are).
	** Have init routine called from IPX's init
	** that calls this with NoRouterEntryPoints *.
	*/
	/* Assume init runs before anything useful, and that
	** the default router is the first one to register.
	** If current Token is NULL, the default (NULL) router
	** is registering via the init routine
	*/
	/* Do all the func pointers point somewhere ?
	** Test for valid Func addresses for all needed services
	*/
	if (!RRIPX.SendData || !RRIPX.UseMethodWithLanKey) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"RegisterRRouter: Not all RRIPX methods defined !"));
#ifdef DEBUG
		cmn_err(CE_WARN,
				"RegisterRRouter: Not all RRIPX methods defined !\n");
#endif
		return((void *)NTR_LEAVE(NULL));
	}

	if (!rrouterMethods->GrantLanKey
			|| !rrouterMethods->UpdateLanKey
			|| !rrouterMethods->InvalidateLanKey
			|| !rrouterMethods->DigestRouterPacket
#ifdef OLD_SAP_CHECKING
			|| !rrouterMethods->CheckSapPacket
#endif
			|| !rrouterMethods->MapNetToIpxLanKey
			|| !rrouterMethods->GetRouteInfo) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"RegisterRRouter: Not all RROUTER methods defined !"));
#ifdef DEBUG
		cmn_err(CE_WARN,
				"RegisterRRouter: Not all RROUTER methods defined !");
#endif
		return((void *)NTR_LEAVE(NULL));
	}

/* explain ??? */
	if(currentRRouterToken == NULL) {
		if(defaultRRouterToken != NULL) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"RegisterRRouter: Attempt to register non-NULL Router before NULL Router !"));
#ifdef DEBUG
			cmn_err(CE_WARN,
				"RegisterRRouter: Attempt to register non-NULL Router before NULL Router");
#endif
			return((void *)NTR_LEAVE(NULL));
		}
		DefaultRouter = *rrouterMethods;
		defaultRRouterToken = &DefaultRouter;
		currentRRouterToken = &DefaultRouter;
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"RegisterRRouter: Registering NULL Router 0x%X",
				defaultRRouterToken));
#ifdef DEBUG
		cmn_err(CE_CONT, "RegisterRRouter: Registering NULL Router 0x%X\n",
				defaultRRouterToken);
#endif
	} else {
		/* Test if another router (non-default token)
	 	* is currently running.
	 	*/
		if (currentRRouterToken != defaultRRouterToken) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"RegisterRRouter: Attempt to register concurrent non-NULL Router"));
#ifdef DEBUG
			cmn_err(CE_WARN,
				"RegisterRRouter: Attempt to register concurrent non-NULL Router");
#endif
				return((void *)NTR_LEAVE(NULL));
		}
		currentRRouterToken = rrouterMethods;
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"RegisterRRouter: Registering Router 0x%X",
				currentRRouterToken));
#ifdef RR_DEBUG
		/*
		 *+ Inform that we are registering router
		 */
		cmn_err(CE_NOTE, "RegisterRRouter: Registering Router 0x%X",
				currentRRouterToken);
#endif
	}

	/* Replace entries in a global table of RRouterEntryPoints
	** with func pointers passed in arg.
	*/
	RROUTER = *rrouterMethods;
	RegisterLansWithRRouter();

	/* Return Token for id/deregestering.
	** Assume new router starts up on successful return.
	*/
	return((void *)NTR_LEAVE(currentRRouterToken));
}

/*
 * int DeregisterRRouter(void *rrouterToken)
 *	Test for valid rrouterToken, Invalidate all lanKeys (DivestRRouterLanKeys)
 *	then change the current router to the default router (NoRouter).
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
int
DeregisterRRouter(void *rrouterToken)
{
	NTR_ENTER(1, rrouterToken, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
            "Enter DeregisterRRouter"));
	/* Assume router has downed itself, freed its resources,
	** divested itself of rrLanKeys, and needs only to free
	** ??? NO - we release our own lanKeys
	** its entry points and relenquish its registry token.
	*/
	/* Test for valid token.
	*/
	if(!((RROUTERMethods_t *)rrouterToken == currentRRouterToken)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"DeregisterRouter: Bogus token!!!"));
		/*
		 *+ Deregister router call passed an invalid token
		 */
		cmn_err(CE_WARN, "DeregisterRouter: Invalid token!!!");
		return(NTR_LEAVE(4));	/* Invalid Resource Tag ? */
	}

	DivestRRouterLanKeys();

	/* Restore default entry points.
	** Use a little trickery here and let initNoRouter
	** and IPXRegister do the work for us.
	*/
	currentRRouterToken = defaultRRouterToken;
	RROUTER = DefaultRouter;
	/* Invalidate token.
	*/
	NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"DeregisterRRouter: Re-Registered default RRouter 0x%X\n",
		defaultRRouterToken));
	return(NTR_LEAVE(0));
}
