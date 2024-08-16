/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/auth_des.c	1.5.9.3"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * auth_des.c, client-side implementation of DES authentication
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include "trace.h"
#include <rpc/des_crypt.h>
#include <sys/time.h>
#include <sys/syslog.h>

#define	MILLION			1000000L
#define	RTIME_TIMEOUT		5	/* seconds to wait for sync */

#define	AUTH_PRIVATE(auth)	(struct ad_private *) auth->ah_private
#define	ALLOC(object_type)	(object_type *) mem_alloc(sizeof (object_type))
#define	FREE(ptr, size)		mem_free((char *)(ptr), (int) size)
#define	ATTEMPT(xdr_op)		if (!(xdr_op)) return (FALSE)

static struct auth_ops *authdes_ops();

/*
 * This struct is pointed to by the ah_private field of an "AUTH *"
 */
struct ad_private {
	char *ad_fullname;		/* client's full name */
	u_int ad_fullnamelen;		/* length of name, rounded up */
	char *ad_servername;		/* server's full name */
	u_int ad_servernamelen;		/* length of name, rounded up */
	u_int ad_window;		/* client specified window */
	bool_t ad_dosync;		/* synchronize? */
	char *ad_timehost;		/* remote host to sync with */
	struct timeval ad_timediff;	/* server's time - client's time */
	u_long ad_nickname;		/* server's nickname for client */
	struct authdes_cred ad_cred;	/* storage for credential */
	struct authdes_verf ad_verf;	/* storage for verifier */
	struct timeval ad_timestamp;	/* timestamp sent */
	des_block ad_xkey;		/* encrypted conversation key */
	u_char ad_pkey[1024];		/* Servers actual public key */
};

AUTH *authdes_pk_seccreate();

/*
 * documented version of authdes_seccreate
 */
AUTH *
authdes_seccreate(servername, win, timehost, ckey)
	char *servername;		/* network name of server */
	u_int win;			/* time to live */
	char *timehost;			/* optional hostname to sync with */
	des_block *ckey;		/* optional conversation key to use */
{
	u_char	pkey_data[1024];
	netobj	pkey;
	AUTH	*dummy;

	trace1(TR_authdes_seccreate, 0);
	if (! getpublickey(servername, pkey_data)) {
		trace1(TR_authdes_seccreate, 1);
		return (NULL);
	}
	pkey.n_bytes = (char *) pkey_data;
	pkey.n_len = strlen(pkey_data) + 1;
	dummy = authdes_pk_seccreate(servername, &pkey, win, timehost, ckey);
	return (dummy);
}

/*
 * Slightly modified version of authdes_seccreate which takes the public key
 * of the server principal as an argument. This spares us a call to
 * getpublickey() which in the nameserver context can cause a deadlock.
 */
AUTH *
authdes_pk_seccreate(servername, pkey, window, timehost, ckey)
	char *servername;		/* network name of server */
	netobj *pkey;			/* public key of server */
	u_int window;			/* time to live */
	char *timehost;			/* optional hostname to sync with */
	des_block *ckey;		/* optional conversation key to use */
{
	AUTH *auth;
	struct ad_private *ad;
	char namebuf[MAXNETNAMELEN+1];

	/*
	 * Allocate everything now
	 */
	trace2(TR_authdes_pk_seccreate, 0, window);
	auth = ALLOC(AUTH);
	if (auth == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32",
			"%s: out of memory"),
			"authdes_pk_seccreate");
		trace1(TR_authdes_pk_seccreate, 1);
		return (NULL);
	}
	ad = ALLOC(struct ad_private);
	if (ad == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32",
			"%s: out of memory"),
			"authdes_pk_seccreate");
		goto failed;
	}
	ad->ad_fullname = ad->ad_servername = NULL; /* Sanity reasons */
	ad->ad_timehost = NULL;
	ad->ad_timediff.tv_sec = 0;
	ad->ad_timediff.tv_usec = 0;
	memcpy(ad->ad_pkey, pkey->n_bytes, pkey->n_len);
	(void) getnetname(namebuf);
	ad->ad_fullnamelen = RNDUP(strlen(namebuf));
	ad->ad_fullname = (char *)mem_alloc(ad->ad_fullnamelen + 1);
	ad->ad_servernamelen = strlen(servername);
	ad->ad_servername = (char *)mem_alloc(ad->ad_servernamelen + 1);

	if (ad->ad_fullname == NULL || ad->ad_servername == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32",
			"%s: out of memory"),
			"authdes_pk_seccreate");
		goto failed;
	}
	if (timehost != NULL) {
		ad->ad_timehost = (char *)mem_alloc(strlen(timehost) + 1);
		if (ad->ad_timehost == NULL) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:32",
				"%s: out of memory"),
				"authdes_pk_seccreate");
			goto failed;
		}
		memcpy(ad->ad_timehost, timehost, strlen(timehost) + 1);
		ad->ad_dosync = TRUE;
	} else {
		ad->ad_dosync = FALSE;
	}
	memcpy(ad->ad_fullname, namebuf, ad->ad_fullnamelen + 1);
	memcpy(ad->ad_servername, servername, ad->ad_servernamelen + 1);
	ad->ad_window = window;
	if (ckey == NULL) {
		if (key_gendes(&auth->ah_key) < 0) {
			(void) syslog(LOG_ERR,
			  gettxt("uxnsl:33",
			      "authdes_pk_seccreate: unable to gen conversation key"));
			goto failed;
		}
	} else {
		auth->ah_key = *ckey;
	}

	/*
	 * Set up auth handle
	 */
	auth->ah_cred.oa_flavor = AUTH_DES;
	auth->ah_verf.oa_flavor = AUTH_DES;
	auth->ah_ops = authdes_ops();
	auth->ah_private = (caddr_t)ad;

	if (!authdes_refresh(auth)) {
		goto failed;
	}
	trace1(TR_authdes_pk_seccreate, 1);
	return (auth);

failed:
	if (auth)
		FREE(auth, sizeof (AUTH));
	if (ad) {
		if (ad->ad_fullname)
			FREE(ad->ad_fullname, ad->ad_fullnamelen + 1);
		if (ad->ad_servername)
			FREE(ad->ad_servername, ad->ad_servernamelen + 1);
		if (ad->ad_timehost)
			FREE(ad->ad_timehost, strlen(ad->ad_timehost) + 1);
		FREE(ad, sizeof (struct ad_private));
	}
	trace1(TR_authdes_pk_seccreate, 1);
	return (NULL);
}

/*
 * Implement the five authentication operations
 */


/*
 * 1. Next Verifier
 */
/*ARGSUSED*/
static void
authdes_nextverf(auth)
	AUTH *auth;
{
	trace1(TR_authdes_nextverf, 0);
	trace1(TR_authdes_nextverf, 1);
	/* what the heck am I supposed to do??? */
}


/*
 * 2. Marshal
 */
static bool_t
authdes_marshal(auth, xdrs)
	AUTH *auth;
	XDR *xdrs;
{
	struct ad_private *ad = AUTH_PRIVATE(auth);
	struct authdes_cred *cred = &ad->ad_cred;
	struct authdes_verf *verf = &ad->ad_verf;
	des_block cryptbuf[2];
	des_block ivec;
	int status;
	long len;
	register long *ixdr;

	/*
	 * Figure out the "time", accounting for any time difference
	 * with the server if necessary.
	 */
	trace1(TR_authdes_marshal, 0);
	(void) gettimeofday(&ad->ad_timestamp, (struct timezone *) 0);
	ad->ad_timestamp.tv_sec += ad->ad_timediff.tv_sec;
	ad->ad_timestamp.tv_usec += ad->ad_timediff.tv_usec;
	if (ad->ad_timestamp.tv_usec >= MILLION) {
		ad->ad_timestamp.tv_usec -= MILLION;
		ad->ad_timestamp.tv_sec += 1;
	}

	/*
	 * XDR the timestamp and possibly some other things, then
	 * encrypt them.
	 */
	ixdr = (long *)cryptbuf;
	IXDR_PUT_LONG(ixdr, ad->ad_timestamp.tv_sec);
	IXDR_PUT_LONG(ixdr, ad->ad_timestamp.tv_usec);
	if (ad->ad_cred.adc_namekind == ADN_FULLNAME) {
		IXDR_PUT_U_LONG(ixdr, ad->ad_window);
		IXDR_PUT_U_LONG(ixdr, ad->ad_window - 1);
		ivec.key.high = ivec.key.low = 0;
		status = cbc_crypt((char *)&auth->ah_key, (char *)cryptbuf,
			2 * sizeof (des_block),
			DES_ENCRYPT | DES_HW, (char *)&ivec);
	} else {
		status = ecb_crypt((char *)&auth->ah_key, (char *)cryptbuf,
			sizeof (des_block), DES_ENCRYPT | DES_HW);
	}
	if (DES_FAILED(status)) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:34",
			"authdes_marshal: DES encryption failure"));
		trace1(TR_authdes_marshal, 1);
		return (FALSE);
	}
	ad->ad_verf.adv_xtimestamp = cryptbuf[0];
	if (ad->ad_cred.adc_namekind == ADN_FULLNAME) {
		ad->ad_cred.adc_fullname.window = cryptbuf[1].key.high;
		ad->ad_verf.adv_winverf = cryptbuf[1].key.low;
	} else {
		ad->ad_cred.adc_nickname = ad->ad_nickname;
		ad->ad_verf.adv_winverf = 0;
	}

	/*
	 * Serialize the credential and verifier into opaque
	 * authentication data.
	 */
	if (ad->ad_cred.adc_namekind == ADN_FULLNAME) {
		len = ((1 + 1 + 2 + 1)*BYTES_PER_XDR_UNIT + ad->ad_fullnamelen);
	} else {
		len = (1 + 1)*BYTES_PER_XDR_UNIT;
	}

	if (ixdr = xdr_inline(xdrs, 2*BYTES_PER_XDR_UNIT)) {
		IXDR_PUT_LONG(ixdr, AUTH_DES);
		IXDR_PUT_LONG(ixdr, len);
	} else {
		ATTEMPT(xdr_putlong(xdrs, (long *)&auth->ah_cred.oa_flavor));
		ATTEMPT(xdr_putlong(xdrs, &len));
	}
	ATTEMPT(xdr_authdes_cred(xdrs, cred));

	len = (2 + 1)*BYTES_PER_XDR_UNIT;
	if (ixdr = xdr_inline(xdrs, 2*BYTES_PER_XDR_UNIT)) {
		IXDR_PUT_LONG(ixdr, AUTH_DES);
		IXDR_PUT_LONG(ixdr, len);
	} else {
		ATTEMPT(xdr_putlong(xdrs, (long *)&auth->ah_verf.oa_flavor));
		ATTEMPT(xdr_putlong(xdrs, &len));
	}
	ATTEMPT(xdr_authdes_verf(xdrs, verf));
	trace1(TR_authdes_marshal, 1);
	return (TRUE);
}


/*
 * 3. Validate
 */
static bool_t
authdes_validate(auth, rverf)
	AUTH *auth;
	struct opaque_auth *rverf;
{
	struct ad_private *ad = AUTH_PRIVATE(auth);
	struct authdes_verf verf;
	int status;
	register u_long *ixdr;

	trace1(TR_authdes_validate, 0);
	if (rverf->oa_length != (2 + 1) * BYTES_PER_XDR_UNIT) {
		trace1(TR_authdes_validate, 1);
		return (FALSE);
	}
	ixdr = (u_long *)rverf->oa_base;
	verf.adv_xtimestamp.key.high = (u_long)*ixdr++;
	verf.adv_xtimestamp.key.low = (u_long)*ixdr++;
	verf.adv_int_u = IXDR_GET_U_LONG(ixdr);

	/*
	 * Decrypt the timestamp
	 */
	status = ecb_crypt((char *)&auth->ah_key, (char *)&verf.adv_xtimestamp,
		sizeof (des_block), DES_DECRYPT | DES_HW);

	if (DES_FAILED(status)) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:35",
			"authdes_validate: DES decryption failure"));
		trace1(TR_authdes_validate, 1);
		return (FALSE);
	}

	/*
	 * xdr the decrypted timestamp
	 */
	ixdr = (u_long *)verf.adv_xtimestamp.c;
	verf.adv_timestamp.tv_sec = IXDR_GET_LONG(ixdr) + 1;
	verf.adv_timestamp.tv_usec = IXDR_GET_LONG(ixdr);

	/*
	 * validate
	 */
	if (memcmp((char *)&ad->ad_timestamp, (char *)&verf.adv_timestamp,
		sizeof (struct timeval)) != 0) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:36",
			"authdes_validate: verifier mismatch"));
		trace1(TR_authdes_validate, 1);
		return (FALSE);
	}

	/*
	 * We have a nickname now, let's use it
	 */
	ad->ad_nickname = verf.adv_nickname;
	ad->ad_cred.adc_namekind = ADN_NICKNAME;
	trace1(TR_authdes_validate, 1);
	return (TRUE);
}

/*
 * 4. Refresh
 */
static bool_t
authdes_refresh(auth)
	AUTH *auth;
{
	struct ad_private *ad = AUTH_PRIVATE(auth);
	struct authdes_cred *cred = &ad->ad_cred;
	netobj		pkey;

	trace1(TR_authdes_refresh, 0);
	if (ad->ad_dosync &&
		!synchronize(ad->ad_timehost, &ad->ad_timediff)) {
		/*
		 * Hope the clocks are synced!
		 */
		ad->ad_timediff.tv_sec = ad->ad_timediff.tv_usec = 0;
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:37",
			"authdes_refresh: unable to synchronize with server"));
	}
	ad->ad_xkey = auth->ah_key;
	pkey.n_bytes = (char *)(ad->ad_pkey);
	pkey.n_len = strlen(ad->ad_pkey) + 1;
	if (key_encryptsession_pk(ad->ad_servername, &pkey, &ad->ad_xkey) < 0) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:38",
			"authdes_refresh: unable to encrypt conversation key"));
		trace1(TR_authdes_refresh, 1);
		return (FALSE);
	}
	cred->adc_fullname.key = ad->ad_xkey;
	cred->adc_namekind = ADN_FULLNAME;
	cred->adc_fullname.name = ad->ad_fullname;
	trace1(TR_authdes_refresh, 1);
	return (TRUE);
}


/*
 * 5. Destroy
 */
static void
authdes_destroy(auth)
	AUTH *auth;
{
	struct ad_private *ad = AUTH_PRIVATE(auth);

	trace1(TR_authdes_destroy, 0);
	FREE(ad->ad_fullname, ad->ad_fullnamelen + 1);
	FREE(ad->ad_servername, ad->ad_servernamelen + 1);
	if (ad->ad_timehost)
		FREE(ad->ad_timehost, strlen(ad->ad_timehost) + 1);
	FREE(ad, sizeof (struct ad_private));
	FREE(auth, sizeof (AUTH));
	trace1(TR_authdes_destroy, 1);
}

/*
 * Synchronize with the server at the given address, that is,
 * adjust timep to reflect the delta between our clocks
 */
static bool_t
synchronize(timehost, timep)
	char *timehost;
	struct timeval *timep;
{
	struct timeval mytime;
	time_t ptime;

	trace1(TR_synchronize, 0);
	if (! rpcb_gettime(timehost, &ptime)) {
		/* try to contact INET time server */
		struct timeval timeout;

		timep->tv_sec = ptime;
		timep->tv_usec = 0;

		timeout.tv_sec = RTIME_TIMEOUT;
		timeout.tv_usec = 0;
		if (rtime_tli(timehost, timep, &timeout) < 0) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:39",
				"synchronize time failed"));
			trace1(TR_synchronize, 1);
			return (FALSE);
		}
	}
	(void) gettimeofday(&mytime, (struct timezone *) 0);
	timep->tv_sec -= mytime.tv_sec;
	if (mytime.tv_usec > timep->tv_usec) {
		timep->tv_sec -= 1;
		timep->tv_usec += MILLION;
	}
	timep->tv_usec -= mytime.tv_usec;
	trace1(TR_synchronize, 1);
	return (TRUE);
}

static struct auth_ops *
authdes_ops()
{
	static struct auth_ops ops;

	trace1(TR_authdes_ops, 0);
	if (ops.ah_destroy == NULL) {
		ops.ah_nextverf = authdes_nextverf;
		ops.ah_marshal = authdes_marshal;
		ops.ah_validate = authdes_validate;
		ops.ah_refresh = authdes_refresh;
		ops.ah_destroy = authdes_destroy;
	}
	trace1(TR_authdes_ops, 1);
	return (&ops);
}
