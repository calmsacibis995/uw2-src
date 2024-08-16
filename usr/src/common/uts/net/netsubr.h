/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NETSUBR_H	/* wrapper symbol for kernel use */
#define _NET_NETSUBR_H	/* subject to change without notice */

#ident	"@(#)kern:net/netsubr.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <net/tihdr.h>
#include <net/xti.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/tihdr.h>
#include <sys/xti.h>

#endif /* _KERNEL_HEADERS */

extern char ti_statetbl[TE_NOEVENTS][TS_NOSTATES];

#define	TI_BADSTATE	127		/* unreachable state */

/*
 * Macro to change state
 * NEXTSTATE(event, current state)
 */
#define	NEXTSTATE(X,Y)		ti_statetbl[X][Y]

extern int tlitosyserr(int);


/*
 * Option management macro definitions,
 * Option management inforamation data structure definition.
 * XTI Option management function declarations,
 */

/*
 * optmgmtfunc structure is used to build tables of option level specific
 * processing functions which are used by optmgmt_doopt() and
 * optmgmt_negotiateopt().
 *
 * Fields:
 *	level	the option level the option management function handles
 *	func	pointer to the level specific option management function
 */
struct optmgmtfunc {
	unsigned long	level;
	int		(*func)();
};

/*
 * Options Information (table entry) Structure
 *	This structure defines information related to Options such
 *	as default values, ranges of valid values (if applicable),
 *	and various attributes associated with the option.
 *
 * Description of Fields:
 *
 * optinf_name:		Option name.
 * optinf_dfltval:	Option's default value.	 A value of NULL
 *			indicates there is no default value to be
 *			initialized for the option.  Also dependng on
 *			imiplementation, no default value may be
 *			interpreted as the option is off or not active.
 * optinf_len:		Length of the option's value. A length of
 *			T_UNSPEC indicates a variablelength.
 * otinf_minvalp:	For options whose values are not asolute
 *			requibrements, this indicates the minimum value.
 * optinf_maxval:	For options whose values are not absolute
 *			requirements, this indicates the maximum value.
 * optinf_flag:		Various option attributes (defined below).
 *
 *			OPTINF_ABSOLUTEREQ indicates option requests to
 *				negotiate its value are absolute requirements.
 *			OPTINF_ASSOCRELATED indicates option is
 *				association related.
 *			OPTINF_NOTSUPPORTED indicates option is not supported.
 *			OPTINF_READONLY indicates option is read-only.
 *			OPTINF_PRIVILEGED indicates P_DRIVER privilege is
 *				required to negotiate or retrieve option.
 *			OPTINF_NODEFAULT indicates option has no default.
 */
struct optinfo {
	unsigned long	optinf_name;
	void	       *optinf_dfltval;
	unsigned long	optinf_len;
	void	       *optinf_minval;
	void	       *optinf_maxval;
	unsigned long	optinf_flag;
};
#define OPTINF_ABSOLUTEREQ	0x01
#define OPTINF_ASSOCRELATED	0x02
#define OPTINF_NOTSUPPORTED	0x04
#define OPTINF_READONLY		0x08
#define OPTINF_PRIVILEGED	0x10
#define OPTINF_NODEFAULT	0x20

/*
 * GOODOPTHDR(opthdr, endopt, optaction, prim)
 *
 * Description:
 *	Determine whether or not the option header is valid
 *	given its:
 *
 *	opthdr: pointer to the option header
 *	endopt: pointer to end of options
 *	optaction: the action to be performed on the options behalf.
 *	prim: the TI primitive that contains the option(s).
 *
 *	-The length must be  >= the size of the header.
 *	-Cases where the length of the option value can be zero:
 *		-the special option name T_ALLOPT or
 *	-A T_ALLOPT
 *		-can only be specified via a T_OPTMGMT_REQ primitive and
 *		-its option must not have a value.
 *		-option action can not be T_CHECK.
 *
 * Return:
 *	0		Failure Indication
 *	non-zero	Success Indication
 */
#define	GOODOPTHDR(opthdr, endopt, optaction, prim) \
(\
 (((struct t_opthdr *)opthdr)->len >= sizeof(struct t_opthdr))\
 &&\
 (((void *)((char *)opthdr + ((struct t_opthdr *)opthdr)->len)) <= (void *)endopt)\
 && \
 (\
  (\
   (((struct t_opthdr *)opthdr->len - sizeof(struct t_opthdr)) == 0)\
   &&\
   (\
    (\
     ((struct t_opthdr *)opthdr->name == T_ALLOPT)\
     &&\
     (prim == T_OPTMGMT_REQ)\
     &&\
     (optaction != T_CHECK)\
    )\
    || ((struct t_opthdr *)opthdr->name != T_ALLOPT)\
   )\
  )\
  ||\
  (\
   (((struct t_opthdr *)opthdr->len - sizeof(struct t_opthdr)) != 0)\
   &&\
   ((struct t_opthdr *)opthdr->name != T_ALLOPT)\
  )\
 )\
)
/*
 * GOODOPTREQ(req, mp)
 *
 * Description:
 *	Determine whether or not the TI request in regards to options is valid
 *	given the:
 *
 *	req: pointer to the TI primitive request
 *	mp: mblk_t pointer to the message the request came in.
 *
 *	-The length of the options must be >= sizeof(struct t_opthdr).
 *	-The offset into the message where the options are located must be
 *	 >= the size of the TI primitive request.
 *	-The offset + length must not be > than the size of the message the
 *	 request came in.
 *
 * Return:
 *	0		Failure Indication
 *	non-zero	Success Indication
 */
#define	GOODOPTREQ(req, mp) \
(\
 ((req)->OPT_length >= sizeof(struct t_opthdr))\
 &&\
 ((req)->OPT_offset >= sizeof(*req))\
 &&\
 (((req)->OPT_offset + (req)->OPT_length) <= (mp->b_wptr - mp->b_rptr))\
)

extern void optmgmt_doopt(queue_t *q, mblk_t *bp, struct optmgmtfunc *functbl);
extern int optmgmt_negotiateopt(queue_t *q, mblk_t *bp, struct optmgmtfunc *functbl, mblk_t **retmpp);
extern int optmgmt_mkopt(mblk_t **mpp, unsigned long mkoptlen, unsigned long mkoptlevel, unsigned long mkoptname, unsigned long mkoptstatus,  void *mkoptval);
extern void optmgmt_errorack(queue_t *q, mblk_t *bp, int tierr, int syserr);

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_NETSUBR_H */
