/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/authdesubr.c	1.12"
#ident 	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	authdesubr.c, miscellaneous support routines for kernel
 *	implentation of DES style suthentication.
 */

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <svc/time.h>
#include <net/socketvar.h>
#include <svc/errno.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <net/socket.h>
#include <util/sysmacros.h>
#include <net/inet/in.h>
#include <net/rpc/rpc.h>
#include <net/rpc/clnt.h>
#include <net/ktli/t_kuser.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <proc/cred.h>
#include <svc/utsname.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <io/uio.h>
#include <svc/systeminfo.h>
#include <net/rpc/rpcb_prot.h>

extern	timestruc_t	hrestime;

#define TOFFSET		((u_long)86400*(365*70 + (70/4)))
#define WRITTEN		((u_long)86400*(365*86 + (86/4)))
#define NC_INET		"inet"		/* XXX */

/*
 * rtime()
 *	Get time from a remote machine.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0 on success, -1 on failure.
 *
 * Description:
 * 	Sets the time, obtaining it from specified host. Since
 *	the timeserver returns with the time of day in seconds
 *	since Jan 1, 1900, we subtract 86400(365*70 + 17) to
 *	get time since Jan 1, 1970, which is what get/settimeofday()
 *	use.
 *
 * Parameters:
 *
 *	synctp			# device of transport to sync with
 *	addrp			# addr of host to get time from
 *	calltype		# use rpc of straight call for sync
 *	timep			# return time in
 *	wait			# time to wait to hear from host
 *
 */
int
rtime(dev_t synctp, struct netbuf *addrp, int calltype,
	struct timeval *timep, struct timeval *wait)
{
	int			error;
	int			timo;
	time_t			thetime;
	int			dummy;
	struct t_kunitdata	*unitdata;
	TIUSER			*tiptr;
	int			type;
	int			uderr;
	int			retries;

	retries = 5;

	if (calltype == 0) {
		/*
		 * Use old method.
		 */
again:
		RPCLOG(0x10, "rtime: using old method\n", 0);

		if ((error = t_kopen(NULL, synctp, FREAD|FWRITE|FNDELAY,
				&tiptr, u.u_lwpp->l_cred)) != 0) {

			RPCLOG(0x10, "rtime: t_kopen %d\n", error);

			return -1;
		}
	
		if ((error = t_kbind(tiptr, NULL, NULL)) != 0) {
			(void)t_kclose(tiptr, 1);

			RPCLOG(0x10, "rtime: t_kbind %d\n", error);

			return -1;
		}
	
		if ((error = t_kalloc(tiptr, T_UNITDATA, T_UDATA|T_ADDR,
					 	(char **)&unitdata)) != 0) {

			RPCLOG(0x10, "rtime: t_kalloc %d\n", error);

			(void)t_kclose(tiptr, 1);
			return -1;
		}
	
		unitdata->addr.len = addrp->len;
		bcopy(addrp->buf, unitdata->addr.buf, unitdata->addr.len);
	
		unitdata->udata.buf = (caddr_t)&dummy;
		unitdata->udata.len = sizeof(dummy);
	
		if ((error = t_ksndudata(tiptr, unitdata, NULL)) != 0) {

			RPCLOG(0x10, "rtime: t_ksndudata %d\n", error);

			(void)t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void)t_kclose(tiptr, 1);
			return -1;
		}
	
		timo = (int)(wait->tv_sec * HZ +
				(wait->tv_usec * HZ) / MILLION);

		RPCLOG(0x10, "rtime: timo %x\n", timo);
		if ((error = t_kspoll(tiptr, timo, POLL_SIG_CATCH,
							&type)) != 0) {
			RPCLOG(0x10, "rtime: t_kspoll %d\n", error);

			(void)t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void)t_kclose(tiptr, 1);
			return -1;
		}
	
		if (type == 0) {

			RPCLOG(0x10, "rtime: t_kspoll timed out\n", 0);

			(void)t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void)t_kclose(tiptr, 1);
			return -1;
		}
	
		if ((error =t_krcvudata(tiptr, unitdata, &type, &uderr)) != 0) {

			RPCLOG(0x10, "rtime: t_krcvudata %d\n", error);

			(void)t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void)t_kclose(tiptr, 1);
			return -1;
		}
	
		if (type != T_DATA) {

			RPCLOG(0x10, "rtime: t_krcvudata rtnd type %d\n", type);

			(void)t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void)t_kclose(tiptr, 1);
			if (retries-- == 0)
				return -1;
			else	goto again;
		}
	
		if (unitdata->udata.len < sizeof(u_long)) {

			RPCLOG(0x10, "rtime: bad rcvd length %d\n",
						unitdata->udata.len);

			(void)t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			(void)t_kclose(tiptr, 1);
			if (retries-- == 0)
				return -1;
			else	goto again;
		}
	
		/* LINTED pointer alignment */
		thetime = (time_t) ntohl(*(u_long *)unitdata->udata.buf);

		(void)t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
		(void)t_kclose(tiptr, 1);
	} else {
		CLIENT			*client;
		struct knetconfig	config;
		struct timeval		timeout;

		RPCLOG(0x10, "rtime: using new method\n", 0);

		/*
		 * We talk to rpcbind.
		 */
		config.knc_rdev = synctp;
		config.knc_protofmly = NC_INET;		/* XXX */
		error = clnt_tli_kcreate(&config, addrp, (u_long)RPCBPROG,
			(u_long)RPCBVERS, 0, retries, u.u_lwpp->l_cred,
			&client);
		if (error != 0) {

			RPCLOG(0x10, 
		"key_call: clnt_tli_kcreate rtned error %d", error);

			return -1;
		}

		timeout.tv_sec = 60;
		timeout.tv_usec = 0;

		/*
		 * allocate the xid.
		 */
		clnt_clts_setxid(client, alloc_xid());

		error = CLNT_CALL(client, RPCBPROC_GETTIME, xdr_void, NULL, 
				xdr_u_long, (caddr_t)&thetime, timeout,
				(struct netbuf *)NULL, 0, POLL_SIG_CATCH);
		auth_destroy(client->cl_auth);
		clnt_destroy(client);
		if (error != RPC_SUCCESS) {

			RPCLOG(0x10, 
			"rtime: time sync CLNT_CALL failed: error %x", error);
			RPCLOG(0x10, clnt_sperrno(error), 0);

			error = EIO;
			return -1;
		}
	}

	if (calltype != 0)
		thetime += TOFFSET;

	RPCLOG(0x10, "rtime: thetime = %x\n", thetime);

	if (thetime < WRITTEN) {

		RPCLOG(0x10, "rtime: time returned is too far in past %x",
								thetime);
		RPCLOG(0x10, "rtime: WRITTEN %x", WRITTEN);

		return -1;
	}

	thetime -= TOFFSET;

	timep->tv_sec = thetime;

	RPCLOG(0x10, "rtime: timep->tv_sec = %x\n", timep->tv_sec);
	RPCLOG(0x10, "rtime: machine time = %x\n", hrestime.tv_sec);

	timep->tv_usec = 0;

	RPCLOG(0x10, "rtime: returning success\n", 0);

	return (0);
}

/*
 * sitoa(s, i)
 *	Convert short to ascii, and add on the a string.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns pointer to the converted ascii.
 *
 * Description:
 *	Convert short to ascii.
 *
 * Parameters:
 *
 *	s			# string to add on the ascii to
 *	i			# short to convert to ascii
 *
 */
static char *
sitoa(char *s, short i)
{
	char	*p;
	char	*end;
	char	c;

	if (i < 0) {
		*s++ = '-';		
		i = -i;
	} else if (i == 0) {
		*s++ = '0';
	}

	/*
	 * format in reverse order
	 */
	for (p = s; i > 0; i /= 10) {	
		*p++ = (i % 10) + '0';
	}
	*(end = p) = 0; 

	/*
	 * reverse
	 */
	while (p > s) {
		c = *--p;
		*p = *s;
		*s++ = c;
	}

	return(end);
}

/*
 * atoa(dst, src)
 *	Copy string to string.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns pointer to end of string.
 *
 * Description:
 *	Copy string to string.
 *
 * Parameters:
 *
 *	dst			# destination string
 *	src			# source string
 *
 */
static char *
atoa(char *dst, char *src)
{
	while (*dst++ = *src++)
		;
	return(dst-1);
}

/*
 * kgetnetname(netname)
 *	Get my network name.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns pointer to end of string.
 *
 * Description:
 *	Get my network name.
 *
 *	WARNING: this gets the network name in sun unix format. 
 *	Other operating systems (non-unix) are free to put something
 *	else here.
 *
 * Parameters:
 *
 *	netname			# return pointer to network name
 *
 */
void
kgetnetname(char *netname)
{
	char	myutsname[SYS_NMLN];
	char	*p;

	/*
	 * locking handled ny getutsname().
	 */
	getutsname(utsname.nodename, myutsname);

	p = atoa(netname, "unix.");
	if (!pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
		p = atoa(p, myutsname);
	} else {
		p = sitoa(p, (short)u.u_lwpp->l_cred->cr_uid);
	}
	*p++ = '@';
	p = atoa(p, srpc_domain);
}
