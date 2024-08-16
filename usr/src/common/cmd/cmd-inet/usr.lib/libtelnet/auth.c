/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libtelnet/auth.c	1.1.1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*      SCCS IDENTIFICATION        */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)auth.c	5.1 (Berkeley) 2/28/91";
#endif /* not lint */

/*
 * Copyright (C) 1990 by the Massachusetts Institute of Technology
 *
 * Export of this software from the United States of America is assumed
 * to require a specific license from the United States Government.
 * It is the responsibility of any person or organization contemplating
 * export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */


#if	defined(AUTHENTICATE)
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <pfmt.h>
#define	AUTH_NAMES
#include <arpa/telnet.h>
#ifdef	__STDC__
#include <stdlib.h>
#endif
#ifdef	NO_STRING_H
#include <strings.h>
#else
#include <string.h>
#endif

#include "encrypt.h"
#include "auth.h"
#include "misc-proto.h"
#include "auth-proto.h"

#define	typemask(x)		(1<<((x)-1))

int auth_debug_mode = 0;
static 	char	*Name = "Noname";
static	int	Server = 0;
static	Authenticator	*authenticated = 0;
static	int	authenticating = 0;
static	int	validuser = 0;
static	unsigned char	_auth_send_data[256];
static	unsigned char	*auth_send_data;
static	int	auth_send_cnt = 0;

/*
 * Authentication types supported.  Plese note that these are stored
 * in priority order, i.e. try the first one first.
 */
Authenticator authenticators[] = {
#ifdef	KRB5
	{ AUTHTYPE_KERBEROS_V5, AUTH_WHO_CLIENT|AUTH_HOW_MUTUAL,
				kerberos5_init,
				kerberos5_send,
				kerberos5_is,
				kerberos5_reply,
				kerberos5_status,
				kerberos5_printsub },
	{ AUTHTYPE_KERBEROS_V5, AUTH_WHO_CLIENT|AUTH_HOW_ONE_WAY,
				kerberos5_init,
				kerberos5_send,
				kerberos5_is,
				kerberos5_reply,
				kerberos5_status,
				kerberos5_printsub },
#endif
#ifdef	KRB4
	{ AUTHTYPE_KERBEROS_V4, AUTH_WHO_CLIENT|AUTH_HOW_MUTUAL,
				kerberos4_init,
				kerberos4_send,
				kerberos4_is,
				kerberos4_reply,
				kerberos4_status,
				kerberos4_printsub },
	{ AUTHTYPE_KERBEROS_V4, AUTH_WHO_CLIENT|AUTH_HOW_ONE_WAY,
				kerberos4_init,
				kerberos4_send,
				kerberos4_is,
				kerberos4_reply,
				kerberos4_status,
				kerberos4_printsub },
#endif
	{ 0, },
};

static Authenticator NoAuth = { 0 };

static int	i_support = 0;
static int	i_wont_support = 0;

	Authenticator *
findauthenticator(type, way)
	int type;
	int way;
{
	Authenticator *ap = authenticators;

	while (ap->type && (ap->type != type || ap->way != way))
		++ap;
	return(ap->type ? ap : 0);
}

	void
auth_init(name, server)
	char *name;
	int server;
{
	Authenticator *ap = authenticators;

	Server = server;
	Name = name;

	i_support = 0;
	authenticated = 0;
	authenticating = 0;
	while (ap->type) {
		if (!ap->init || (*ap->init)(ap, server)) {
			i_support |= typemask(ap->type);
			if (auth_debug_mode)
				pfmt(stdout, MM_NOSTD, ":401:>>>%s: I support auth type %d %d\r\n",
					Name,
					ap->type, ap->way);
		}
		++ap;
	}
}

	void
auth_disable_name(name)
	char *name;
{
	int x;
	for (x = 0; x < AUTHTYPE_CNT; ++x) {
		if (!strcasecmp(name, AUTHTYPE_NAME(x))) {
			i_wont_support |= typemask(x);
			break;
		}
	}
}

	int
getauthmask(type, maskp)
	char *type;
	int *maskp;
{
	register int x;

	if (strcasecmp(type, AUTHTYPE_NAME(0))) {
		*maskp = -1;
		return(1);
	}

	for (x = 1; x < AUTHTYPE_CNT; ++x) {
		if (!strcasecmp(type, AUTHTYPE_NAME(x))) {
			*maskp = typemask(x);
			return(1);
		}
	}
	return(0);
}

	int
auth_enable(type)
	int type;
{
	return(auth_onoff(type, 1));
}

	int
auth_disable(type)
	int type;
{
	return(auth_onoff(type, 0));
}

	int
auth_onoff(type, on)
	char *type;
	int on;
{
	int mask = -1;
	Authenticator *ap;

	if (!strcasecmp(type, "?") || !strcasecmp(type, "help")) {
		if (on)
			pfmt(stdout, MM_NOSTD, ":402:auth enable 'type'\n");
		else
			pfmt(stdout, MM_NOSTD, ":403:auth disable 'type'\n");
		pfmt(stdout, MM_NOSTD, ":404:Where 'type' is one of:\n");
		pfmt(stdout, MM_NOSTD, ":405:\t%s\n", AUTHTYPE_NAME(0));
		for (ap = authenticators; ap->type; ap++)
			pfmt(stdout, MM_NOSTD, ":405:\t%s\n", AUTHTYPE_NAME(ap->type));
		return(0);
	}

	if (!getauthmask(type, &mask)) {
		pfmt(stdout, MM_NOSTD, ":406:%s: invalid authentication type\n", type);
		return(0);
	}
	mask = getauthmask(type, &mask);
	if (on)
		i_wont_support &= ~mask;
	else
		i_wont_support |= mask;
	return(1);
}

	int
auth_togdebug(on)
	int on;
{
	if (on < 0)
		auth_debug_mode ^= 1;
	else
		auth_debug_mode = on;
	if (auth_debug_mode)
		pfmt(stdout, MM_NOSTD, ":407:auth debugging enabled\n");
	else
		pfmt(stdout, MM_NOSTD, ":408:auth debugging disabled\n");
	return(1);
}

	int
auth_status()
{
	Authenticator *ap;

	if (i_wont_support == -1)
		pfmt(stdout, MM_NOSTD, ":409:Authentication disabled\n");
	else
		pfmt(stdout, MM_NOSTD, ":410:Authentication enabled\n");

	for (ap = authenticators; ap->type; ap++) {
		if (i_wont_support & typemask(ap->type))
			pfmt(stdout, MM_NOSTD, ":412:%s: disabled\n", AUTHTYPE_NAME(ap->type));
		else
			pfmt(stdout, MM_NOSTD, ":411:%s: enabled\n", AUTHTYPE_NAME(ap->type));
	}
	return(1);
}

/*
 * This routine is called by the server to start authentication
 * negotiation.
 */
	void
auth_request()
{
	static unsigned char str_request[64] = { IAC, SB,
						 TELOPT_AUTHENTICATION,
						 TELQUAL_SEND, };
	Authenticator *ap = authenticators;
	unsigned char *e = str_request + 4;

	if (!authenticating) {
		authenticating = 1;
		while (ap->type) {
			if (i_support & ~i_wont_support & typemask(ap->type)) {
				if (auth_debug_mode) {
					pfmt(stdout, MM_NOSTD, ":413:>>>%s: Sending type %d %d\r\n",
						Name, ap->type, ap->way);
				}
				*e++ = ap->type;
				*e++ = ap->way;
			}
			++ap;
		}
		*e++ = IAC;
		*e++ = SE;
		net_write(str_request, e - str_request);
		printsub('>', &str_request[2], e - str_request - 2);
	}
}

/*
 * This is called when an AUTH SEND is received.
 * It should never arrive on the server side (as only the server can
 * send an AUTH SEND).
 * You should probably respond to it if you can...
 *
 * If you want to respond to the types out of order (i.e. even
 * if he sends  LOGIN KERBEROS and you support both, you respond
 * with KERBEROS instead of LOGIN (which is against what the
 * protocol says)) you will have to hack this code...
 */
	void
auth_send(data, cnt)
	unsigned char *data;
	int cnt;
{
	Authenticator *ap;
	static unsigned char str_none[] = { IAC, SB, TELOPT_AUTHENTICATION,
					    TELQUAL_IS, AUTHTYPE_NULL, 0,
					    IAC, SE };
	if (Server) {
		if (auth_debug_mode) {
			pfmt(stdout, MM_NOSTD, ":414:>>>%s: auth_send called!\r\n", Name);
		}
		return;
	}

	if (auth_debug_mode) {
		pfmt(stdout, MM_NOSTD, ":415:>>>%s: auth_send got:", Name);
		printd(data, cnt); pfmt(stdout, MM_NOSTD, ":416:\r\n");
	}

	/*
	 * Save the data, if it is new, so that we can continue looking
	 * at it if the authorization we try doesn't work
	 */
	if (data < _auth_send_data ||
	    data > _auth_send_data + sizeof(_auth_send_data)) {
		auth_send_cnt = cnt > sizeof(_auth_send_data)
					? sizeof(_auth_send_data)
					: cnt;
		bcopy((void *)data, (void *)_auth_send_data, auth_send_cnt);
		auth_send_data = _auth_send_data;
	} else {
		/*
		 * This is probably a no-op, but we just make sure
		 */
		auth_send_data = data;
		auth_send_cnt = cnt;
	}
	while ((auth_send_cnt -= 2) >= 0) {
		if (auth_debug_mode)
			pfmt(stdout, MM_NOSTD, ":417:>>>%s: He supports %d\r\n",
				Name, *auth_send_data);
		if ((i_support & ~i_wont_support) & typemask(*auth_send_data)) {
			ap = findauthenticator(auth_send_data[0],
					       auth_send_data[1]);
			if (!ap) {
				pfmt(stdout, MM_NOSTD, ":418:Internal state error: cannot find authentication type %d a second time\r\n", *auth_send_data);
			} else if (ap->send) {
				if (auth_debug_mode)
					pfmt(stdout, MM_NOSTD, ":419:>>>%s: Trying %d %d\r\n",
						Name, auth_send_data[0],
							auth_send_data[1]);
				if ((*ap->send)(ap)) {
					/*
					 * Okay, we found one we like
					 * and did it.
					 * we can go home now.
					 */
					if (auth_debug_mode)
						pfmt(stdout, MM_NOSTD, ":420:>>>%s: Using type %d\r\n",
							Name, *auth_send_data);
					auth_send_data += 2;
					return;
				}
			}
			/* else
			 *	just continue on and look for the
			 *	next one if we didn't do anything.
			 */
		}
		auth_send_data += 2;
	}
	net_write(str_none, sizeof(str_none));
	printsub('>', &str_none[2], sizeof(str_none) - 2);
	if (auth_debug_mode)
		pfmt(stdout, MM_NOSTD, ":421:>>>%s: Sent failure message\r\n", Name);
	auth_finished(0, AUTH_REJECT);
}

	void
auth_send_retry()
{
	/*
	 * if auth_send_cnt <= 0 then auth_send will end up rejecting
	 * the authentication and informing the other side of this.
	 */
	auth_send(auth_send_data, auth_send_cnt);
}

	void
auth_is(data, cnt)
	unsigned char *data;
	int cnt;
{
	Authenticator *ap;

	if (cnt < 2)
		return;

	if (data[0] == AUTHTYPE_NULL) {
		auth_finished(0, AUTH_REJECT);
		return;
	}

	if (ap = findauthenticator(data[0], data[1])) {
		if (ap->is)
			(*ap->is)(ap, data+2, cnt-2);
	} else if (auth_debug_mode)
		pfmt(stdout, MM_NOSTD, ":422:>>>%s: Invalid authentication in IS: %d\r\n",
			Name, *data);
}

	void
auth_reply(data, cnt)
	unsigned char *data;
	int cnt;
{
	Authenticator *ap;

	if (cnt < 2)
		return;

	if (ap = findauthenticator(data[0], data[1])) {
		if (ap->reply)
			(*ap->reply)(ap, data+2, cnt-2);
	} else if (auth_debug_mode)
		pfmt(stdout, MM_NOSTD, ":423:>>>%s: Invalid authentication in SEND: %d\r\n",
			Name, *data);
}

	void
auth_name(data, cnt)
	unsigned char *data;
	int cnt;
{
	Authenticator *ap;
	unsigned char savename[256];

	if (cnt < 1) {
		if (auth_debug_mode)
			pfmt(stdout, MM_NOSTD, ":424:>>>%s: Empty name in NAME\r\n", Name);
		return;
	}
	if (cnt > sizeof(savename) - 1) {
		if (auth_debug_mode)
			pfmt(stdout, MM_NOSTD, ":425:>>>%s: Name in NAME (%d) exceeds %d length\r\n",
					Name, cnt, sizeof(savename)-1);
		return;
	}
	bcopy((void *)data, (void *)savename, cnt);
	savename[cnt] = '\0';	/* Null terminate */
	if (auth_debug_mode)
		pfmt(stdout, MM_NOSTD, ":426:>>>%s: Got NAME [%s]\r\n", Name, savename);
	auth_encrypt_user(savename);
}

	int
auth_sendname(cp, len)
	unsigned char *cp;
	int len;
{
	static unsigned char str_request[256+6]
			= { IAC, SB, TELOPT_AUTHENTICATION, TELQUAL_NAME, };
	register unsigned char *e = str_request + 4;
	register unsigned char *ee = &str_request[sizeof(str_request)-2];

	while (--len >= 0) {
		if ((*e++ = *cp++) == IAC)
			*e++ = IAC;
		if (e >= ee)
			return(0);
	}
	*e++ = IAC;
	*e++ = SE;
	net_write(str_request, e - str_request);
	printsub('>', &str_request[2], e - &str_request[2]);
	return(1);
}

	void
auth_finished(ap, result)
	Authenticator *ap;
	int result;
{
	if (!(authenticated = ap))
		authenticated = &NoAuth;
	validuser = result;
}

	/* ARGSUSED */
	static void
auth_intr(sig)
	int sig;
{
	auth_finished(0, AUTH_REJECT);
}

	int
auth_wait(name)
	char *name;
{
	if (auth_debug_mode)
		pfmt(stdout, MM_NOSTD, ":427:>>>%s: in auth_wait.\r\n", Name);

	if (Server && !authenticating)
		return(0);

	(void) signal(SIGALRM, auth_intr);
	alarm(30);
	while (!authenticated)
		if (telnet_spin())
			break;
	alarm(0);
	(void) signal(SIGALRM, SIG_DFL);

	/*
	 * Now check to see if the user is valid or not
	 */
	if (!authenticated || authenticated == &NoAuth)
		return(AUTH_REJECT);

	if (validuser == AUTH_VALID)
		validuser = AUTH_USER;

	if (authenticated->status)
		validuser = (*authenticated->status)(authenticated,
						     name, validuser);
	return(validuser);
}

	void
auth_debug(mode)
	int mode;
{
	auth_debug_mode = mode;
}

	void
auth_printsub(data, cnt, buf, buflen)
	unsigned char *data, *buf;
	int cnt, buflen;
{
	Authenticator *ap;

	if ((ap = findauthenticator(data[1], data[2])) && ap->printsub)
		(*ap->printsub)(data, cnt, buf, buflen);
	else
		auth_gen_printsub(data, cnt, buf, buflen);
}

	void
auth_gen_printsub(data, cnt, buf, buflen)
	unsigned char *data, *buf;
	int cnt, buflen;
{
	register unsigned char *cp;
	unsigned char tbuf[16];

	cnt -= 3;
	data += 3;
	buf[buflen-1] = '\0';
	buf[buflen-2] = '*';
	buflen -= 2;
	for (; cnt > 0; cnt--, data++) {
		sprintf((char *)tbuf, " %d", *data);
		for (cp = tbuf; *cp && buflen > 0; --buflen)
			*buf++ = *cp++;
		if (buflen <= 0)
			return;
	}
	*buf = '\0';
}
#endif
