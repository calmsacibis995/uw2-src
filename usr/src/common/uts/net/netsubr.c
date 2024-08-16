/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/netsubr.c	1.7"
/*      Copyright (c) 1990, 1991 UNIX System Laboratories, Inc. */
/*      Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T   */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*      UNIX System Laboratories, Inc.                          */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */

/*
 * State transition table for TI interfaces
 */

#include <net/tihdr.h>
#include <net/xti.h>
#include <net/netsubr.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <util/param.h>
#include <util/mod/moddefs.h>

#define DRVNAME "net - Network utilities module"

MOD_MISC_WRAPPER(net, NULL, NULL, DRVNAME);

/*
 * Required definitions
 */
#define	NR	TI_BADSTATE		/* not reachable */

char ti_statetbl[TE_NOEVENTS][TS_NOSTATES] = {
					/* STATES */
/* 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 */

/* 0 */ { 1, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/* 1 */ {NR, NR, NR,  2, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/* 2 */ {NR, NR, NR,  4, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/* 3 */ {NR,  3, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/* 4 */ {NR, NR, NR, NR,  3, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/* 5 */ {NR,  0,  3, NR,  3,  3, NR, NR,  7, NR, NR, NR,  6,  7,  9, 10, 11},
/* 6 */ {NR, NR,  0, NR, NR,  6, NR, NR, NR, NR, NR, NR,  3, NR,  3,  3,  3},
/* 7 */ {NR, NR, NR, NR, NR, NR, NR, NR,  9, NR, NR, NR, NR,  3, NR, NR, NR},
/* 8 */ {NR, NR, NR, NR, NR, NR, NR, NR,  3, NR, NR, NR, NR,  3, NR, NR, NR},
/* 9 */ {NR, NR, NR, NR, NR, NR, NR, NR,  7, NR, NR, NR, NR,  7, NR, NR, NR},
/*10 */ {NR, NR, NR,  5, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*11 */ {NR, NR, NR, NR, NR, NR, NR,  8, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*12 */ {NR, NR, NR, NR, NR, NR, 12, 13, NR, 14, 15, 16, NR, NR, NR, NR, NR},
/*13 */ {NR, NR, NR, NR, NR, NR, NR, NR, NR,  9, NR, 11, NR, NR, NR, NR, NR},
/*14 */ {NR, NR, NR, NR, NR, NR, NR, NR, NR,  9, NR, 11, NR, NR, NR, NR, NR},
/*15 */ {NR, NR, NR, NR, NR, NR, NR, NR, NR, 10, NR,  3, NR, NR, NR, NR, NR},
/*16 */ {NR, NR, NR,  7, NR, NR, NR,  7, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*17 */ {NR, NR, NR, NR, NR, NR,  9, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*18 */ {NR, NR, NR, NR, NR, NR, NR, NR, NR,  9, 10, NR, NR, NR, NR, NR, NR},
/*19 */ {NR, NR, NR, NR, NR, NR, NR, NR, NR,  9, 10, NR, NR, NR, NR, NR, NR},
/*20 */ {NR, NR, NR, NR, NR, NR, NR, NR, NR, 11,  3, NR, NR, NR, NR, NR, NR},
/*21 */ {NR, NR, NR, NR, NR, NR,  3, NR, NR,  3,  3,  3, NR, NR, NR, NR, NR},
/*22 */ {NR, NR, NR, NR, NR, NR, NR,  3, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*23 */ {NR, NR, NR, NR, NR, NR, NR,  7, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*24 */ {NR, NR, NR,  9, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*25 */ {NR, NR, NR,  3, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*26 */ {NR, NR, NR,  3, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
/*27 */ {NR, NR, NR,  3, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR, NR},
};

/*
 * TLI error translation
 */
static int tli_errs[] = {
	0,				/* <no error> */
	EADDRNOTAVAIL,			/* TBADADDR */
	ENOPROTOOPT,			/* TBADOPT */
	EACCES,				/* TACCES */
	EBADF,				/* TBADF */
	EADDRNOTAVAIL,			/* TNOADDR */
	EPROTO,				/* TOUTSTATE */
	EPROTO,				/* TBADSEQ */
	0,				/* TSYSERR: should never get this */
	EPROTO,				/* TLOOK: never sent by transport */
	EMSGSIZE,			/* TBADDATA */
	EMSGSIZE,			/* TBUFOVFLW */
	EPROTO,				/* TFLOW */
	EWOULDBLOCK,			/* TNODATA */
	EPROTO,				/* TNODIS */
	EPROTO,				/* TNOUDERR */
	EINVAL,				/* TBADFLAG */
	EPROTO,				/* TNOREL */
	EOPNOTSUPP,			/* TNOTSUPPORT */
	EPROTO,				/* TSTATECHNG */
	EPROTO,				/* TNOSTRUCTYPE */
	EPROTO,				/* TBADNAME */
	EPROTO,				/* TBADQLEN */
	EADDRINUSE,			/* TADDRBUSY */
	EPROTO,				/* TINDOUT */
	EPROTO,				/* TPROVMISMATCH */
	EPROTO,				/* TRESQLEN */
	EPROTO,				/* TRESADDR */
	EPROTO,				/* TQFULL */
	EPROTO,				/* TPROTO */
};

/*
 * tlitosyserr(int terr)
 *	Translate a TLI error into a system error as best we can.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns system error.
 *
 * Description:
 *	Translate a TLI error into a system error as best we can.
 *
 * Parameters:
 *
 *	terr			# TLI error to translate
 *	
 */
int
tlitosyserr(int terr)
{
	if (terr >= sizeof tli_errs / sizeof *tli_errs)
		return EPROTO;
	else
		return tli_errs[terr];
}


/*
 * XTI Option Management processing functions
 */

/*
 * void optmgmt_doopt(queue_t *q, mblk_t *bp, struct optmgmtfunc *functbl)
 *
 * Description:
 *	Do option management processing for a T_OPTMGMT_REQ and send response
 *	(T_OPTMGMT_ACK or T_ERROR_ACK) back.
 *
 * Calling/Exit State:
 *	Parameters:
 *		q	Is a pass through for the level specific options
 *		bp	Contains the T_OPTMGMT_REQ
 *		functbl	An array of pointers to level specific functions.
 *
 *	Locks:
 *		No locks are held on entry.  No locks are held on exit. No locks
 *		are acquired in this function.
 */
void
optmgmt_doopt(queue_t *q, mblk_t *bp, struct optmgmtfunc *functbl)
{
/*	struct T_optmgmt_req *req = BPTOT_OPTMGMT_REQ(bp); */
	struct T_optmgmt_req *req = (struct T_optmgmt_req *)bp->b_rptr;
	int error = 0;
	long optaction = req->MGMT_flags;
	mblk_t *retmp = NULL;

	ASSERT(((union T_primitives *)req)->type == T_OPTMGMT_REQ);

	/*
	 * Validate the format of the request in regards to the option length
	 * and offset. Validate the option level.
	 * Then for each option validate the option level
	 * independent characteristics of each option header.
	 * Other Validation Criteria:
	 * -Only one option level can be specified in a T_OPTMGMT_REQ.
         * -Only one T_ALLOPT can be specified in a T_OPTMGMT_REQ.
	 * -No other options can be specified when a T_ALLOPT is specified.
	 * For each option call its level specific option management function
	 * which is found in the functbl.  Finally if everything is successful,
	 * build a T_OPTMGMT_ACK with the option result, go through each
	 * specified option's result and set the MFMT_flags field to the
	 * "worst" status (according to XPG4 XTI).
	 * option action T_NEGOTIATE is handle by optmgmt_negotiateopt().  This
	 * is the function other TI primitives with options call to handle
	 * their options.
	 * NOTE: T_NEGOTIATE is the only valid option action for TI primitives
	 * with options other than T_OPTMGMT_REQ.
	 */
	switch (optaction) {
	case T_NEGOTIATE:
		error = optmgmt_negotiateopt(q, bp, functbl, &retmp);
		break;
	case T_CHECK:
	case T_CURRENT:
	case T_DEFAULT:
	{
		/* pointers to message buffer in bp for processing options */
		struct t_opthdr *opt;	/* beginning of options */
		struct t_opthdr *eopt;	/* end of options */
		struct t_opthdr *curopt;/* current option */

		unsigned long optlevel;
		struct optmgmtfunc *funcp;
		int alloptflag = 0; /* indicates a T_ALLOPT has been found */


		if ((bp->b_wptr - bp->b_rptr) < sizeof(struct T_optmgmt_req)) {
			error = -EINVAL;
			break;
		}
		if (!GOODOPTREQ(req, bp)) {
			error = TBADOPT;
			break;
		}
		/*
		 * Since only one option level is valid for a T_OPTMGMT_REQ,
		 * get level and level specific optmgmt function based on
		 * the first option.
		 * If level is unsupported or unrecognized for a T_OPTMGMT_REQ
		 * a TBADOPT is returned.
		 */
		optlevel = opt->level;
		for (funcp = functbl; funcp->func && funcp->level != optlevel;
		 funcp++);
		if (!funcp->func) {
			error = TBADOPT;
			break;
		}

		/*
		 * Save pointers to beginning and end of options. They will be
		 * used in validation checks
		 */
		opt = (struct t_opthdr *)
		 ((void *)((char *)bp->b_rptr + req->OPT_offset));
		eopt = (struct t_opthdr *)
		 ((void *)((char *)opt + req->OPT_length));

		for (curopt = opt; curopt;
		 curopt = OPT_NEXTHDR(opt, req->OPT_length, curopt)) {
			/*
			 * Consider the option BAD if
			 * the option header is invalid OR
			 * there are more than one option level OR
			 * option name is T_ALLOPT and it is not the first
			 *  option (implies there are options before it) OR
			 * option name T_ALLOPT is specified more than once OR
			 * an option is specified after an T_ALLOPT.
			 */
			if ((!GOODOPTHDR(curopt, eopt, optaction, T_OPTMGMT_REQ))
			 || (optlevel != curopt->level) ||
			 ((curopt->name == T_ALLOPT) && (curopt != opt)) ||
			 (curopt->name == T_ALLOPT ? alloptflag++ : 0) ||
			 alloptflag) {
				error = TBADOPT;
				break;
			}
			/*
			 * Call level specific optmgmt functions to handle
			 * option.  Options results will be returned in
			 * retmp.
			 */ 
			if (error = (*funcp->func)(q, (union T_primitives *)req,
			 curopt, &retmp)) {
				break;
			}
		}
		break;
	}
	default:
		error = TBADFLAG;
		break;
	}

	if (error && retmp) {
		freemsg(retmp);
	}
	if (error < 0) {
		optmgmt_errorack(q, bp, TSYSERR, -error);
	} else if (error > 0) {

		optmgmt_errorack(q, bp, error, 0);
	} else {
		long size = 0;
/*		struct T_optmgmt_ack *ack = BPTOT_OPTMGMT_ACK(bp); */
		struct T_optmgmt_ack *ack = (struct T_optmgmt_ack *)bp->b_rptr;
		mblk_t *tbp;
		mblk_t *mp1;
		struct t_opthdr *opt;
		struct t_opthdr *curopt;
		long status = curopt->status;

		/* flags used for determining the "worst" status */
		int readonlyflag = 0;
		int failureflag = 0;
		int partsuccessflag = 0;
		int successflag = 0;
		int loopcnt;

		/* caluclate size of option result from retrun message */
		for (mp1 = retmp; mp1; mp1 = mp1->b_cont) {
			size += mp1->b_wptr - mp1->b_rptr;
		}

		/* format T_OPTMGMT_ACK from orignal message bp */
		ack->PRIM_type = T_OPTMGMT_ACK;
		ack->OPT_offset = sizeof(struct T_optmgmt_ack);
		ack->OPT_length = size;
		bp->b_wptr = bp->b_rptr + sizeof(struct T_optmgmt_ack);
		if (bp->b_cont) {
			freemsg(bp->b_cont);
		}
		bp->b_cont = retmp;

		/* pullup message since timod only looks at the first block */
		if ((tbp = msgpullup(bp, -1)) == (mblk_t *)NULL) {
			freemsg(bp->b_cont);
			optmgmt_errorack(q, bp, TSYSERR, ENOSR);
			return;
		}
		freemsg(bp);
		bp = tbp;
		/*
		 * ACKs message type is M_PCPROTO.  Can now convert message type
		 * to M_PCPROTO after message has been pulled up.
		 */
		bp->b_datap->db_type = M_PCPROTO;

		/*
		 * Go through all of the return option's status field and set
		 * the MGMT_flags to the "worst" status as defined by XPG4
		 * XTI.
		 */
		opt = (struct t_opthdr *)
		 (void *)((char *)bp->b_rptr + sizeof(struct T_optmgmt_req));
		for (curopt = opt, loopcnt = 0; curopt;
		 curopt = OPT_NEXTHDR(opt, ack->OPT_length, curopt), loopcnt++) {
			if (status == T_NOTSUPPORT) {
				/* "worst" status no need to check further */
				break;
			} else if (status == T_READONLY) {
				readonlyflag++;
			} else if (status == T_FAILURE) {
				failureflag++;
			} else if (status == T_PARTSUCCESS) {
				partsuccessflag++;
			} else if (status == T_SUCCESS) {
				successflag++;
			} else {
				ASSERT(status != T_NOTSUPPORT ||
				 status != T_READONLY || status != T_FAILURE ||
				 status != T_PARTSUCCESS || status != T_SUCCESS);
			}
		}
		/*
		 * If only one option result or status is T_NOTSUPPORT,
		 * MGMT_flags is set to status.
		 */
		if (status == T_NOTSUPPORT || loopcnt == 1) {
			ack->MGMT_flags = status;
		} else if (readonlyflag) {
			ack->MGMT_flags = T_READONLY;
		} else if (failureflag) {
			ack->MGMT_flags = T_FAILURE;
		} else if (partsuccessflag) {
			ack->MGMT_flags = T_PARTSUCCESS;
		} else if (successflag) {
			ack->MGMT_flags = T_SUCCESS;
		}


		/* send up the ack */
		qreply(q, bp);
	}
	return;
}

/*
 * int optmgmt_negotiateopt(queue_t *q, mblk_t *bp, struct optmgmtfunc *functbl, *	mblk_t **retmpp)
 *
 * Description:
 *	Do option management processing for a T_OPTMGMT_REQ, T_CONN_REQ,
 *	T_CONN_RES, or T_UNITDATA_REQ for option action T_NEGOTIATE and
 *	build a option results in retmpp.
 *
 * Calling/Exit State:
 *	Parameters:
 *		q	Is a pass through for the level specific options
 *		bp	Contains the TI primivite REQuest.
 *		functbl	An array of pointers to level specific functions.
 *		retmp	Where the options results will be built.
 *
 *	Locks:
 *		No locks are held on entry.  No locks are held on exit. No locks
 *		are acquired in this functions.
 *
 * Return Values:
 *	0	Success
 *	>0	Failure; represents a TI error
 *	<0	Failure; system error in its negative form
 */
int
optmgmt_negotiateopt(queue_t *q, mblk_t *bp, struct optmgmtfunc *functbl,
 mblk_t **retmpp)
{
/*	union T_primitives *req = BPTOT_PRIMITIVES(bp); */
	union T_primitives *req = (union T_primitives *)bp->b_rptr;
	long prim = req->type;
	int error = 0;
	long optaction;
	unsigned long optlevel;
	long OPT_length;
	struct optmgmtfunc *funcp;
	int alloptflag = 0; /* indicates a T_ALLOPT has been found */
	mblk_t *tbp;

	/* pointers to message buffer in bp for processing options */
	struct t_opthdr *opt;	/* beginning of options */
	struct t_opthdr *eopt;	/* end of options */
	struct t_opthdr *curopt;/* current option */

	/*
	 * Validate that the request is either a T_OPTMGMT_REQ, T_CONN_REQ,
	 * T_CONN_RES, or a T_UNITDATA_REQ.  If request is a T_OPTMGMT_REQ
	 * its MGMT_flags must be T_NEGOTIATE.
	 * Validate the format of the request in regards to the option length
	 * and offset.  Save pointers to beginning and end of options, and
	 * the option length. They will be used in validation checks.
	 * Validate the option level.  For option levels that are
	 * not recognized, return TBADOPT for a T_OPTMGMT_REQ otherwise ignore
	 * the option.
	 * 
	 * Then for each option validate the option level
	 * independent characteristics of each option header.
	 * For each option call its level specific option management function
	 * which is found in the functbl.  the level specific function build
	 * the option results if everything is successful.
	 * NOTE: T_NEGOTIATE is the only valid option action for TI primitives
	 * with options other than T_OPTMGMT_REQ.
	 * OTHER CRITERIA:
	 * -Only one option level can be specified in a T_OPTMGMT_REQ.
         * -Only one T_ALLOPT can be specified in a T_OPTMGMT_REQ.
	 * -T_ALLOPT is only valid in a T_OPTMGMT_REQ.
	 * -No other options can be specified when a  T_ALLOPT is specified.
	 */
	switch (prim) {
	case T_OPTMGMT_REQ:
		ASSERT(((struct T_optmgmt_req *)req)->MGMT_flags == T_NEGOTIATE);
		if ((bp->b_wptr - bp->b_rptr) < sizeof(struct T_optmgmt_req)) {
			return -EINVAL;
		}
		if (!GOODOPTREQ((struct T_optmgmt_req *)req, bp)) {
			return TBADOPT;
		}
		opt = (struct t_opthdr *)
	 	((void *)((char *)bp->b_rptr + ((struct T_optmgmt_req *)req)->OPT_offset));
		eopt = (struct t_opthdr *)
	 	((void *)((char *)opt + ((struct T_optmgmt_req *)req)->OPT_length));
		OPT_length = ((struct T_optmgmt_req *)req)->OPT_length;
		break;
	case T_CONN_REQ:
		if ((bp->b_wptr - bp->b_rptr) < sizeof(struct T_conn_req)) {
			return -EINVAL;
		}
		if (!GOODOPTREQ((struct T_conn_req *)req, bp)) {
			return TBADOPT;
		}
		opt = (struct t_opthdr *)
	 	((void *)((char *)bp->b_rptr + ((struct T_conn_req *)req)->OPT_offset));
		eopt = (struct t_opthdr *)
	 	((void *)((char *)opt + ((struct T_conn_req *)req)->OPT_length));
		OPT_length = ((struct T_conn_req *)req)->OPT_length;
		break;
	case T_CONN_RES:
		if ((bp->b_wptr - bp->b_rptr) < sizeof(struct T_conn_res)) {
			return -EINVAL;
		}
		if (!GOODOPTREQ((struct T_conn_res *)req, bp)) {
			return TBADOPT;
		}
		opt = (struct t_opthdr *)
	 	((void *)((char *)bp->b_rptr + ((struct T_conn_res *)req)->OPT_offset));
		eopt = (struct t_opthdr *)
	 	((void *)((char *)opt + ((struct T_conn_res *)req)->OPT_length));
		OPT_length = ((struct T_conn_res *)req)->OPT_length;
		break;
	case T_UNITDATA_REQ:
		if ((bp->b_wptr - bp->b_rptr) < sizeof(struct T_unitdata_req)) {
			return -EINVAL;
		}
		if (!GOODOPTREQ((struct T_unitdata_req *)req, bp)) {
			return TBADOPT;
		}
		opt = (struct t_opthdr *)
	 	((void *)((char *)bp->b_rptr + ((struct T_unitdata_req *)req)->OPT_offset));
		eopt = (struct t_opthdr *)
	 	((void *)((char *)opt + ((struct T_unitdata_req *)req)->OPT_length));
		OPT_length = ((struct T_unitdata_req *)req)->OPT_length;
		break;
	default:
		/* not a valid TI primitive */
		return -EINVAL;
	}

	/*
	 * Save option level of first option for comparision to levels
	 * of other options (if any).  A T_OPTMGMT_REQ can only specify
	 * one level.
	 */
	optlevel = opt->level;


	for (curopt = opt; curopt;
	 curopt = OPT_NEXTHDR(opt, OPT_length, curopt)) {
		/*
		 * Consider the option BAD if
		 * the option header is invalid OR
		 * there are more than one option level AND
		 *  primitive is  T_OPTMGMT_REQ OR
		 * option name is T_ALLOPT AND
		 *  it is not the first option (implies there are options
		 *  before it) OR
		 *  OR primitive is not T_OPTMGMT_REQ OR
		 * option name T_ALLOPT is specified more than once OR
		 * an option is specified after an T_ALLOPT.
		 */
		if ((!GOODOPTHDR(curopt, eopt, optaction, prim)) ||
		 ((optlevel != curopt->level) && (prim == T_OPTMGMT_REQ)) ||
		 ((curopt->name == T_ALLOPT)
		  && ((curopt != opt) || (prim != T_OPTMGMT_REQ))) ||
		 (curopt->name == T_ALLOPT ? alloptflag++ : 0) ||
		 alloptflag) {
			return TBADOPT;
		}

		/*
		 * Get the level specific optmgmt function for the current
		 * option.  If no function is found (i.e. level is not supported
		 * or unrecognized) return TBADOPT if primitive is T_OPTMGMT_REQ
		 * otherwise ignore the option.
		 */
		for (funcp = functbl;
		 funcp->func && funcp->level != curopt->level;
		 funcp++);
		if (!funcp->func) {
			if (prim == T_OPTMGMT_REQ) {
				return TBADOPT;
			} else {
				continue;
			}
		}

		/*
		 * Call level specific optmgmt functions to handle
		 * option.  Options results will be returned in
		 * retmpp.
		 */ 
		if (error = (*funcp->func)(q, req, curopt, retmpp)) {
			return error;
		}
	}
	/* if there a option result message pull it up */
	if (*retmpp != (mblk_t *)NULL) {
		if ((tbp = msgpullup(*retmpp, -1)) == (mblk_t *)NULL) {
			freemsg(*retmpp);
			*retmpp = (mblk_t *)NULL;
			return -ENOSR;
		} else {
			freemsg(*retmpp);
			*retmpp = tbp;
		}
	}
	return 0;
}

/*
 * int optmgmt_mkopt(mblk_t **mpp, unsigned long mkoptlen,
 *  unsigned long mkoptlevel, unsigned long mkoptname,
 *  unsigned long mkoptstatus,  void *mkoptval)
 *
 * Description:
 *	Given t_opthdr information and an option value, format it into a
 *	t_opthdr and copy into the last mblk_t at b_wptr.
 *	If there is not enough space in the last mblk_t, a new mblk_t
 *	is allocated.
 *	If there is no mblk_t one is allocated and set into *mpp.
 *	If length of mkoptlen is zero it indicates that there is no option
 *	value to format.
 *
 * Calling/Exit State:
 *	Parameters:
 *		mmp		Where to copy formatted option
 *		mkoptlen	Length of option value
 *		mkoptlevel	Option level
 *		mkoptname	Option name
 *		mkoptstatus	Option status
 *		mkoptval	Pointer to option's value
 *
 *	Locking:
 *		No locks are held on entry.  No locks are held on exit.
 *		No locks are acquired in this function.
 *
 *	Return Values:
 *	1	Success
 *	0	Failure (Unable to allocate mblk_t).
 */
int
optmgmt_mkopt(mblk_t **mpp, unsigned long mkoptlen, unsigned long mkoptlevel,
 unsigned long mkoptname, unsigned long mkoptstatus,  void *mkoptval)
{
	struct t_opthdr *opt;
	unsigned long totlen;
	mblk_t *mp = *mpp;

	ASSERT(mpp != (mblk_t **)NULL);
	/*
	 * caluclate total length the formatted option will consume which
	 * includes size of t_opthdr, length of option value,  and alignment
	 * padding.
	 */
	totlen = T_ALIGN(sizeof(struct t_opthdr) + mkoptlen);

	if (!mp) {
		/*
		 * Why 64?  For most options, 64 octets will be enough to hold
		 * three formatted options.
		 */
		if (!(mp = allocb(max(totlen, 64), BPRI_MED))) {
			return 0;
		}
		*mpp = mp;
	} else {
		for (; mp->b_cont; mp = mp->b_cont);
		if ((mp->b_datap->db_lim - mp->b_wptr) < totlen) {
			if (!(mp->b_cont = allocb(max(totlen, 64), BPRI_MED))) {
				return 0;
			}
		}
		mp = mp->b_cont;
		mp->b_datap->db_type = M_PROTO;
	}
	/* LINTED pointer alignment */
	opt = (struct t_opthdr *)mp->b_wptr;
	bzero((caddr_t)opt, totlen);
	opt->len = sizeof(struct t_opthdr) + mkoptlen;
	opt->level = mkoptlevel;
	opt->name = mkoptname;
	opt->status = mkoptstatus;
	if (mkoptlen) {
		bcopy((caddr_t)mkoptval,
		 (caddr_t)opt + sizeof (struct t_opthdr), mkoptlen);
	}
	mp->b_wptr += totlen;

	return 1;
}

/*
 * void optmgmt_errorack(queue_t *q, mblk_t *bp, int tierr, int syserr)
 *
 * Description:
 *	return tranport errors found during the processing of T_OPTMGMT_REQ
 *	requests by converting original request into a T_ERROR_ACK.
 *
 * Calling/Exit State:
 *	Parameters:
 *		q	Read queue.
 *		bp	Message block containing the orignal TI primitive
 *			request.
 *		tierr	Transport Interface errors
 *		syserr	System errors
 *
 *	Locking:
 *		No locks held on entry.  No locks held on exit.  No locks are
 *		acquired in this function.
 */
void
optmgmt_errorack(queue_t *q, mblk_t *bp, int tierr, int syserr)
{
	struct T_error_ack *terrack;
	long prim;

/*	prim = BPTOT_PRIMITIVES(bp)->type; */
	prim = ((union T_primitives *)bp->b_rptr)->type;

	/*
	 * verify that the size of the T_ERROR_ACK can fit into the message.
	 * if not, allocate a new message.
	 */
	if ((bp->b_datap->db_lim - bp->b_datap->db_base)
	 < sizeof(struct T_error_ack)) {
		mblk_t *bp1;
		if (!(bp1 = allocb(sizeof(struct T_error_ack), BPRI_HI))) {
			/* nothing sane to do but error out stream */
			bp->b_rptr = bp->b_datap->db_base;
			bp->b_wptr = bp->b_rptr + sizeof(char);
			*bp->b_rptr = EPROTO;
			bp->b_datap->db_type = M_ERROR;
			putnext(RD(q), bp);
			return;
		} else {
			freemsg(bp);
			bp = bp1;
		}
	}

	if (bp->b_cont)
		freemsg(bp->b_cont);
	bp->b_cont = NULL;
	bp->b_rptr = bp->b_datap->db_base;
/*	terrack = BPTOT_ERROR_ACK(bp);*/
	terrack = (struct T_error_ack *)bp->b_rptr;
	bp->b_wptr = bp->b_rptr + sizeof (struct T_error_ack);
	bp->b_datap->db_type = M_PCPROTO;
	terrack->PRIM_type = T_ERROR_ACK;
	terrack->ERROR_prim = prim;
	terrack->TLI_error = tierr;
	terrack->UNIX_error = syserr;
	qreply(q, bp);
}
