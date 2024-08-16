/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/auth_des.c	1.18"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	auth_des.c, client-side implementation of des authentication
 */

#include <net/inet/in.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_des.h>
#include <net/rpc/clnt.h>
#include <net/rpc/des_crypt.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/socket.h>
#include <net/xti.h>
#include <svc/time.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/types.h>

extern	int			strlen(char *);
extern	void			bcopy(void *, void *, size_t);
extern	void			uniqtime(struct timeval *);
extern	bool_t			xdr_authdes_cred(XDR *, struct authdes_cred *);
extern	bool_t			xdr_authdes_verf(XDR *, struct authdes_verf *);
extern	int			bcmp(char *, char *, size_t);
extern	enum clnt_stat		key_gendes(des_block *);
extern	enum clnt_stat		key_encryptsession(char *, des_block *);
extern	int			rtime(dev_t, struct netbuf *, int,
					struct timeval *, struct timeval *);

extern	int			rtimetimeout;

#define AUTH_PRIVATE(auth)	(struct ad_private *) auth->ah_private
#define ATTEMPT(xdr_op)		if (!(xdr_op)) return (FALSE)

bool_t				authdes_refresh();
bool_t				synchronize();

/*
 * operations for auth_des, initialized by des_ops_init()
 */
static	struct			auth_ops desauthops;

/*
 * this struct is pointed to by the ah_private field of an "AUTH *"
 */
struct ad_private {
	char	*ad_fullname; 		/* client's full name */
	u_int	ad_fullnamelen;		/* length of name, rounded up */
	char	*ad_servername; 	/* server's full name */
	u_int	ad_servernamelen;	/* length of name, rounded up */
	u_int	ad_window;		/* client specified window */
	bool_t	ad_dosync;		/* synchronize? */		
	struct	netbuf ad_syncaddr;	/* remote host to synch with */
	dev_t	ad_synctp;		/* Maj/Min of host transport device */
	int	ad_calltype;		/* use rpc or straight call for sync */
	struct	timeval ad_timediff;	/* server's time - client's time */
	u_long	ad_nickname;		/* server's nickname for client */
	struct	authdes_cred ad_cred;	/* storage for credential */
	struct	authdes_verf ad_verf;	/* storage for verifier */
	struct	timeval ad_timestamp;	/* timestamp sent */
	des_block ad_xkey;		/* encrypted conversation key */
};


/*
 * authdes_create()
 *	Create a client des style authenticator.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns an auth handle.
 *
 * Description:
 *	Create a client des style authenticator.
 *
 * Parameters:
 *
 *	servername		# network name of server
 *	window			# time to live of credentials
 *	syncaddr		# optional addr of host to sync with 
 *	ckey			# optional conversation key to use
 *	synctp			# device of transport to sync with.
 *	calltype		# use rpc or straight call for sync
 *	retauth			# set to new auth handle if success
 *	
 */
/*ARGSUSED*/
int
authdes_create(char *servername, u_int window, struct netbuf *syncaddr,
		dev_t synctp, des_block *ckey, int calltype, AUTH **retauth)
{
	AUTH			*auth;
	struct	ad_private	*ad;
	char			namebuf[MAXNETNAMELEN+1];
	void			kgetnetname();
	int			error = 0;

	if (retauth == NULL)
		return (EINVAL);
	*retauth = NULL;

	/*
 	 * Allocate everything now
	 */
	auth = (AUTH *)kmem_alloc(sizeof(AUTH), KM_SLEEP);
	ad = (struct ad_private *)kmem_alloc(sizeof(struct ad_private),
						KM_SLEEP);
	kgetnetname(namebuf);

	ad->ad_fullnamelen = RNDUP(strlen(namebuf));
	ad->ad_fullname =
		(char *)kmem_alloc((u_int)(ad->ad_fullnamelen + 1), KM_SLEEP);

	ad->ad_servernamelen = strlen(servername);
	ad->ad_servername =
		(char *)kmem_alloc((u_int)(ad->ad_servernamelen + 1), KM_SLEEP);

	if (auth == NULL || ad == NULL || ad->ad_fullname == NULL ||
		ad->ad_servername == NULL) {
		cmn_err(CE_CONT, "authdes_create: out of memory");
		error = ENOMEM;
		goto failed;
	}

	/* 
	 * Set up private data
	 */
	bcopy(namebuf, ad->ad_fullname, (int)(ad->ad_fullnamelen + 1));
	bcopy(servername, ad->ad_servername, (int)(ad->ad_servernamelen + 1));
	if (syncaddr != NULL) {
		ad->ad_syncaddr = *syncaddr;
		ad->ad_synctp = synctp;
		ad->ad_dosync = TRUE;
		ad->ad_calltype = calltype;
	} else {
		ad->ad_timediff.tv_sec = 0;
		ad->ad_timediff.tv_usec = 0;
		ad->ad_dosync = FALSE;
	}

	ad->ad_window = window;
	if (ckey == NULL) {
		enum clnt_stat stat;
		if ((stat = key_gendes(&auth->ah_key)) != RPC_SUCCESS) {
			cmn_err(CE_CONT,
			"authdes_create: unable to gen conv. key");
			if (stat == RPC_INTR)
				error = EINTR;
			else if (stat == RPC_TIMEDOUT)
				error = ETIMEDOUT;
			else
				error = EINVAL;		/* XXX */
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
	auth->ah_ops = &desauthops;
	auth->ah_private = (caddr_t)ad;

	if (!authdes_refresh(auth)) {
		goto failed;
	}	
	*retauth = auth;
	return (0);

failed:
	if (ad != NULL && ad->ad_fullname != NULL) 
		kmem_free(ad->ad_fullname, ad->ad_fullnamelen + 1);
	if (ad != NULL && ad->ad_servername != NULL) 
		kmem_free(ad->ad_servername, ad->ad_servernamelen + 1);
	if (ad != NULL) 
		kmem_free(ad, sizeof(struct ad_private));
	if (auth != NULL) 
		kmem_free(auth, sizeof(AUTH)); 

	return ((error == 0) ? EINVAL : error);		/* XXX */
}

/*
 * authdes_nextverf(auth)
 *	Verify des style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	No return value.
 *
 * Description:
 *	Does nothing.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *
 */
/*ARGSUSED*/
STATIC void
authdes_nextverf(AUTH *auth)
{
}

/*
 * authdes_marshal(auth, xdrs, cr, addr, pre4dot0)
 *	Serialize des style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Called from client side.
 *
 *	Serializes the des style credentials.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *	xdrs			# stream to serialize into
 *	cr			# credentials
 *	addr			# address of remote host, not used
 *	pre4dot0			# pre 4.0 rpc/nfs compat, not used
 *	
 */
/* ARGSUSED */
STATIC bool_t
authdes_marshal(AUTH *auth, XDR *xdrs, struct cred *cr,
		struct netbuf *addr, u_int pre4dot0)
{
	/* LINTED pointer alignment */
	struct	ad_private	*ad = AUTH_PRIVATE(auth);
	struct	authdes_cred	*cred = &ad->ad_cred;
	struct	authdes_verf	*verf = &ad->ad_verf;
	des_block		cryptbuf[2];	
	des_block		ivec;
	int			status;
	int			len;
	long			*ixdr;

	/*
	 * Figure out the "time", accounting for any time difference
	 * with the server if necessary.
	 */
	uniqtime(&ad->ad_timestamp);
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
			2*sizeof(des_block), DES_ENCRYPT, (char *)&ivec);
	} else {
		status = ecb_crypt((char *)&auth->ah_key, (char *)cryptbuf, 
			sizeof(des_block), DES_ENCRYPT);
	}
	if (DES_FAILED(status)) {
		cmn_err(CE_CONT, "authdes_marshal: DES encryption failure");
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
		len = BASE_DESFCREDSZ*BYTES_PER_XDR_UNIT + ad->ad_fullnamelen;
	} else {
		len = BASE_DESNCREDSZ*BYTES_PER_XDR_UNIT;
	}

	if (ixdr = xdr_inline(xdrs, 2*BYTES_PER_XDR_UNIT)) {
		IXDR_PUT_LONG(ixdr, AUTH_DES);
		IXDR_PUT_LONG(ixdr, len);
	} else {
		ATTEMPT(xdr_putlong(xdrs, (long *)&auth->ah_cred.oa_flavor)); 
		ATTEMPT(xdr_putlong(xdrs, (long *)&len)); 
	}
	ATTEMPT(xdr_authdes_cred(xdrs, cred));

	len = (2 + 1)*BYTES_PER_XDR_UNIT; 
	if (ixdr = xdr_inline(xdrs, 2*BYTES_PER_XDR_UNIT)) {
		IXDR_PUT_LONG(ixdr, AUTH_DES);
		IXDR_PUT_LONG(ixdr, len);
	} else {
		ATTEMPT(xdr_putlong(xdrs, (long *)&auth->ah_verf.oa_flavor)); 
		ATTEMPT(xdr_putlong(xdrs, (long *)&len)); 
	}
	ATTEMPT(xdr_authdes_verf(xdrs, verf));
	return (TRUE);
}

/*
 * authdes_validate(auth, verf)
 *	Validate server's des style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Called from client side.
 *
 *	Validates the des style credentials based on the 
 *	verifier provided in verf.
 *
 * Parameter:
 *
 *	auth			# auth handle
 *	rverf			# response verifier
 *
 */
STATIC bool_t
authdes_validate(AUTH *auth, struct opaque_auth *rverf)
{
	/* LINTED pointer alignment */
	struct	ad_private	*ad = AUTH_PRIVATE(auth);
	struct	authdes_verf	verf;
	int			status;
	u_long			*ixdr;

	RPCLOG(0x10, "authdes_validate: rverf->oa_length = %d\n", 
	       rverf->oa_length);

	if (rverf->oa_length != (2 + 1) * BYTES_PER_XDR_UNIT) {
		return (FALSE);
	}

	/* LINTED pointer alignment */
	ixdr = (u_long *)rverf->oa_base;
	verf.adv_xtimestamp.key.high = (u_long)*ixdr++;
	verf.adv_xtimestamp.key.low = (u_long)*ixdr++;
	verf.adv_int_u = IXDR_GET_U_LONG(ixdr);
	RPCLOG(0x10, "authdes_validate: verf.adv_nickname = %d\n",
	       verf.adv_nickname);

	/*
	 * Decrypt the timestamp
	 */
	status = ecb_crypt((char *)&auth->ah_key, (char *)&verf.adv_xtimestamp,
			sizeof(des_block), DES_DECRYPT);

	if (DES_FAILED(status)) {
		cmn_err(CE_CONT, "authdes_validate: DES decryption failure");

		return (FALSE);
	}

	/*
	 * xdr the decrypted timestamp 
	 */
	/* LINTED pointer alignment */
	ixdr = (u_long *)verf.adv_xtimestamp.c;
	verf.adv_timestamp.tv_sec = IXDR_GET_LONG(ixdr) + 1;
	verf.adv_timestamp.tv_usec = IXDR_GET_LONG(ixdr);

	/*
	 * validate
	 */
	if (bcmp((char *)&ad->ad_timestamp, (char *)&verf.adv_timestamp,
		 sizeof(struct timeval)) != 0) {
		cmn_err(CE_CONT, "authdes_validate: verifier mismatch");
		return (FALSE);
	}

	/*
	 * We have a nickname now, let's use it
	 */
	ad->ad_nickname = verf.adv_nickname;	
	ad->ad_cred.adc_namekind = ADN_NICKNAME;

	return (TRUE);	
}

/*
 * authdes_refresh(auth)
 *	Refresh des style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success and FALSE on failure.
 *
 * Description:
 *	Refreshes the clients credentials. Essentially
 *	a resync. with the server.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *	
 */
bool_t
authdes_refresh(AUTH *auth)
{
	/* LINTED pointer alignment */
	struct	ad_private	*ad = AUTH_PRIVATE(auth);
	struct	authdes_cred	*cred = &ad->ad_cred;

	if (ad->ad_dosync && 
			!synchronize(ad->ad_synctp, &ad->ad_syncaddr,
					ad->ad_calltype, &ad->ad_timediff)) {
		/*
		 * Hope the clocks are synced!
		 */
		ad->ad_timediff.tv_sec = ad->ad_timediff.tv_usec = 0;

		cmn_err(CE_CONT,
			"authdes_refresh: unable to sync. with server");
	}

	ad->ad_xkey = auth->ah_key;
	if (key_encryptsession(ad->ad_servername, &ad->ad_xkey)
				!= RPC_SUCCESS) {
		cmn_err(CE_CONT,
			"authdes_refresh: unable to encrypt conv. key");
		return (FALSE);
	}

	cred->adc_fullname.key = ad->ad_xkey;
	cred->adc_namekind = ADN_FULLNAME;
	cred->adc_fullname.name = ad->ad_fullname;

	return (TRUE);
}

/*
 * authdes_destroy(auth)
 *	Destroy des style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	No return value.
 *
 * Description:
 *	Destroys the des style credentials by calling kmem_free.
 *
 * Parameters:
 *
 *	auth			# auth handle
 */
STATIC void
authdes_destroy(AUTH *auth)
{
	/* LINTED pointer alignment */
	struct	ad_private	*ad = AUTH_PRIVATE(auth);

	kmem_free(ad->ad_fullname, ad->ad_fullnamelen + 1);
	kmem_free(ad->ad_servername, ad->ad_servernamelen + 1);
	kmem_free(ad, sizeof(struct ad_private));
	kmem_free(auth, sizeof(AUTH));
}
	
/*
 * synchronize(synctp, syncaddr, calltype, timep)
 *	Sync. with server.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Synchronize with the server at the given address, that is,
 *	adjust timep to reflect the delta between our clocks
 *
 * Parameters:
 *
 *	synctp			# device of transport to sync with.
 *	syncaddr		# address of server to sunc. with
 *	calltype		# use rpc or straight call for sync
 *	timep			# time to sync.
 *
 */
bool_t
synchronize(dev_t synctp, struct netbuf *syncaddr,
		int calltype, struct timeval *timep)
{
	struct	timeval	mytime;
	struct	timeval	timeout;

	timeout.tv_sec = rtimetimeout;
	timeout.tv_usec = 0;
	if (rtime(synctp, syncaddr, calltype, timep, &timeout) < 0) {
		return (FALSE);
	}

	uniqtime(&mytime);
	timep->tv_sec -= mytime.tv_sec;
	if (mytime.tv_usec > timep->tv_usec) {
		timep->tv_sec -= 1;
		timep->tv_usec += MILLION;
	}
	timep->tv_usec -= mytime.tv_usec;

	return (TRUE);
}

/*
 * desauth_ops_init()
 *	Initialize authops vector for des.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns a void.
 *
 * Description:
 *	Initialize authops vector for des.
 *
 * Parameters:
 *
 */
void
desauth_ops_init()
{
	desauthops.ah_nextverf = authdes_nextverf;
	desauthops.ah_marshal = authdes_marshal;
	desauthops.ah_validate = authdes_validate;
	desauthops.ah_refresh = authdes_refresh;
	desauthops.ah_destroy = authdes_destroy;
}
