/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ppp/ppp_log.c	1.4"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */

#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <net/inet/if.h>
#include <net/inet/in_var.h>
#include <net/inet/ppp/ppp_kern.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <io/log/log.h>
#include <io/strlog.h>

#include <io/ddi.h>	/* must be last */

ppp_log_t	*ppp_log;

/*
 * int
 * ppplog_open(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Driver routines for the Point to Point Logging stream (PPLOG).
 *
 * Calling/Exit State:
 *	No locks held.
 */
/* ARGSUSED */
int
ppplog_open(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	pl_t	pl;

	pl = LOCK(ppp_log->log_lck, plstr);

	if (ppp_log->log_rdq != NULL) {
		UNLOCK(ppp_log->log_lck, pl);
		return EACCES;
	}

	if (drv_priv(credp)) {
		UNLOCK(ppp_log->log_lck, pl);
		return EPERM;
	}

	rdq->q_ptr = (caddr_t)ppp_log;
	WR(rdq)->q_ptr = (caddr_t)ppp_log;
	ppp_log->log_rdq = rdq;

	UNLOCK(ppp_log->log_lck, pl);
	qprocson(rdq);
	return 0;
}

/*
 * int
 * ppplog_close(queue_t *rdq, dev_t *devp, int oflag, int sflag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
/* ARGSUSED */
int
ppplog_close(queue_t *rdq, dev_t *devp, int oflag, int sflag, cred_t *credp)
{
	pl_t	pl;

	ASSERT(rdq->q_ptr == (caddr_t)ppp_log);

	qprocsoff(rdq);

	pl = LOCK(ppp_log->log_lck, plstr);
	rdq->q_ptr = NULL;
	WR(rdq)->q_ptr = NULL;
	ppp_log->log_rdq = NULL;
	UNLOCK(ppp_log->log_lck, pl);

	return 0;
}

/*
 * STATIC int
 * ppplog_ursrv(queue_t *rdq)
 *
 * Calling/Exit State:
 *	No locks held.
 */
int
ppplog_ursrv(queue_t *rdq)
{
	mblk_t	*mp;

	STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"ppplog_ursrv: begin rdq 0x%x", rdq);

	while ((mp = getq(rdq)) != NULL) {
		switch (mp->b_datap->db_type) {
		case M_PROTO: 
			if (canputnext(rdq))
				break;
			putbq(rdq,mp);

			STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
				"ppplog_ursrv: end rdq 0x%x (next queue full)",
				rdq);

			return 0;
			/* NOTREACHED */

		default:
			STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
				"ppplog_ursrv: got impossible message type %d",
				mp->b_datap->db_type);

			freemsg(mp);
			break;
		}
		putnext(rdq, mp);
	}
	return 0;
}

/*
 * void
 * ppplog(int index, char *fmt, unsigned int arg)
 *      Log a PPP message 
 *
 * Calling/Exit State:
 *      ppp_log->log_lck is held
 *
 * ppp logger ineterface fuction. Attemps to construct a log message and send
 * it up the PPP logger stream.  fmt is a printf style format string, except
 * that %s %e %E %g and %G are not supported.  fmt can be up to LOGSIZE bytes
 * long.  Up to four nemeric or character arguments can be provided.  Delivery
 * won't be done if message block can't be allocated or the PPP logger stream
 * is not opend
 */
void
#ifdef __STDC__
ppplog(int index, char *fmt, ...)
#else
ppplog(index, fmt, va_alist)
int	index;
char	*fmt;
va_dcl
#endif	/* __STDC__ */
{
        struct ppp_log_ctl_s	*logctl;
        struct ppp_log_dt_s	*logdt;
        mblk_t	*mp;
	int	*argp;
	char	*chp;
	va_list	ap;
	
	/* ASSERT(LOCK_OWNED(ppp_log->log_lck)); */

	if (ppp_log->log_rdq == NULL)
		return;

        if ((mp = allocb(sizeof(struct ppp_log_ctl_s), BPRI_HI)) == NULL)
                return;

        mp->b_wptr += sizeof(struct ppp_log_ctl_s);
        mp->b_datap->db_type = M_PROTO;

	/* LINTED pointer alignment */
        logctl = (struct ppp_log_ctl_s *)mp->b_rptr;
        logctl->function = PPP_LOG;
        logctl->l_index = index;

        mp->b_cont = allocb(sizeof(struct ppp_log_dt_s), BPRI_MED);
        if (mp->b_cont == NULL) {
                freeb(mp);
                return;
        }
	/* LINTED pointer alignment */
        logdt = (struct ppp_log_dt_s *)mp->b_cont->b_rptr;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif	/* __STDC__ */

	argp = &logdt->arg1;
	for (chp = fmt; *chp; chp++) {
		if (*chp != '%')
			continue;
		chp++;
		switch (*chp) {
		case 'd':
		case 'x':
			*argp = (int)va_arg(ap, int);
			argp++;
			break;

		default:

#if defined(DEBUG)
			cmn_err(CE_WARN,
				"ppplog: unknown format specification");
#endif	/* defined(DEBUG) */

			break;
		}
	}
	va_end(ap);

	strncpy(logdt->fmt, fmt, LOGSIZE);
        mp->b_cont->b_wptr += sizeof(struct ppp_log_dt_s);

	/* let our service routine send the message up stream */
	putq(ppp_log->log_rdq, mp);
	return;
}

/*
 * char *
 * ppp_hexdata(char *rt, mblk_t *mp) 
 *      Dump a packet in hex format 
 *
 * Calling/Exit State:
 *      ppp_log->log_lck is held
 *
 */
char *
ppp_hexdata(char *prefix_string,  mblk_t *mp)
{
        unchar	nibble;
	unchar	byte;
        char	*bfp = ppp_log->log_buf;
        unchar	*src;
        unchar	*srce;
        char	*bfpe = &ppp_log->log_buf[LOGSIZE - 20];

	/* copy prefix string into log buffer */
        for (; *prefix_string && bfp < bfpe; *bfp++ = *prefix_string++)
		;
        *bfp++ = ':';
        for (; mp; mp = mp->b_cont) {
                src = mp->b_rptr;
                srce = mp->b_wptr;
                while (src < srce && bfp < bfpe) {
			byte = *src++;
			nibble = (byte >> 4) & 0x0F;
			if (nibble < 10)
				*bfp++ = (char)(nibble + '0');
			else
				*bfp++ = (char)(nibble + ('A' - 10));
			nibble = byte & 0x0F;
			if (nibble < 10)
				*bfp++ = (char)(nibble + '0');
			else
				*bfp++ = (char)(nibble + ('A' - 10));
                }
        }
        *bfp++ = '\0';

        return (ppp_log->log_buf);
}
