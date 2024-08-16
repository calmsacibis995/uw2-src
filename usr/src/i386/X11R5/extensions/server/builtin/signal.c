/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)builtinext:server/builtin/signal.c	1.15"

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include "X.h"
#include "misc.h"
#include "dixstruct.h"
#include "os.h"

#include "BIprivate.h"

typedef void (*SigDisp)(int);
struct SigDispRec {
    SigDisp		disp;
    unsigned char	flags;
#define RESET		( 1 << 0 )
};

#define SERVER_ID	-1	/* consistent with NORMAL_SERVER_MODE
				 * (ie, cur_c_s_fd == -1) */
#define NO_ID		-2
#define ID2INDX(I)	(I) - SERVER_ID
#define MAX_INDX	ID2INDX(MAX_ICLIENTS)

static struct SigTblRec {
    SigDisp		def_disp;
    SigDisp		cur_disp;
    struct SigDispRec	disps[MAX_INDX];
} *sig_tbl;

#define DEF_SIG_DISP(SIG)	sig_tbl[SIG].def_disp
#define CUR_SIG_DISP(SIG)	sig_tbl[SIG].cur_disp
#define SIG_DISP(SIG, I)	sig_tbl[SIG].disps[I].disp
#define SIG_FLAGS(SIG, I)	sig_tbl[SIG].disps[I].flags
#define SIG_DISP_RESET(SIG, I)	( SIG_FLAGS(SIG, I) & RESET )
#define CALL_SIG_FUNC(SIG, I)	(*SIG_DISP(SIG, I))(SIG)
#define NO_DISP			( (SigDisp)-1 )
#define DISP_IS_FUNC(D)		( ((D) != NO_DISP) && \
				  ((D) != SIG_IGN) && ((D) != SIG_DFL) )
static int		NumSigs;


/* CALL_SIG_HANLDER()- call i-c's or server signal handler.
 * 
 * Saving and restoring cur_client is really only necessary when making the
 * transition server -> client mode or vice versa but it's simplier to just
 * always do it.
 */
#define _CALL_IT(ID, HANDLER, SIG) \
    TRANSITION_CALL(ID, HANDLER(SIG))	/* Could be SERVER_ID */

#ifdef BI_DEBUG
#define CALL_SIG_HANDLER(ID, HANDLER, SIG) do \
    { \
	if (ID == SERVER_ID) \
	{ \
	    if (IN_CLIENT_MODE()) \
		BI_DPRINT1(stderr, \
			   "Call server's sig handler (%d) while %s running\n",\
			   SIG, ARGV0(CurClient)); \
	    else \
		BI_DPRINT3(stderr, "Calling server's sigfunc(%d)\n", SIG); \
	} else \
	{ \
	    if (IN_UPPER_SERVER_MODE() || \
		(IN_CLIENT_MODE() && (ID != CurClient))) \
		BI_DPRINT1(stderr, \
			   "Call %s's sig handler (%d) while %s running\n", \
			   ARGV0(ID), SIG, IN_UPPER_SERVER_MODE() ? \
			   ARGV0(CLIENT_INFO(MAP(BIGlobal.cur_c_s_fd))) : \
			   ARGV0(CurClient)); \
	    BI_DPRINT3(stderr, "Calling %s's sigfunc(%d)\n", ARGV0(ID), SIG); \
	} \
	_CALL_IT(ID, HANDLER, SIG); \
    } while(0)

#else
#define CALL_SIG_HANDLER(ID, HANDLER, SIG)	_CALL_IT(ID, HANDLER, SIG)
#endif

/***************************************************************************
 *
 *		PRIVATE ROUTINES
 */

/***************************************************************************
 * CallRealSigFunc-
 */
static void
(*CallRealSigFunc(char * name, void (*(**real_func)(int, void (*)(int)))(int),
	   int sig, SigDisp disp))(int)
{
    if (*real_func == NULL)
    {
	*real_func = (void (*(*)(int, void (*)(int)))(int))libc_dlsym(name);
	if (*real_func == NULL)
	    return(SIG_IGN);
    }
    return((*real_func)(sig, disp));
}

/***************************************************************************
 * sighandler-
 */
static void
sighandler(int sig)
{
    int id;

    for (id = SERVER_ID; id < NumClients; id++)
	if (DISP_IS_FUNC(SIG_DISP(sig, ID2INDX(id))))
	{
	    SigDisp handler = SIG_DISP(sig, ID2INDX(id));

	    /* If disposition is to be reset (ie, signal() behavior), stuff
	     * SIG_DFL into sig-func-rec before calling signal handler.
	     */
	    if (SIG_DISP_RESET(sig, ID2INDX(id)) &&
		(sig != SIGTRAP) && (sig != SIGILL) && (sig != SIGPWR))
	    {
		CUR_SIG_DISP(sig) = SIG_DISP(sig, ID2INDX(id)) = SIG_DFL;
	    }
	    CALL_SIG_HANDLER(id, handler, sig);	/* and continue! */
	}
}

/***************************************************************************
 *
 *		PUBLIC ROUTINES
 */

/***********************************************************************
 * sigignore-
 */
int
sigignore(int sig)
{
    (void)signal(sig, SIG_IGN);
    return(0);
}

/***************************************************************************
 * sig_func- common code to override signal and sigset (below).
 */
static void
(*sig_func(char * name, void (*(**real_func)(int, void (*)(int)))(int),
	   int reset, int sig, SigDisp disp))(int)
{
    SigDisp	prev_disp, new_disp;
    int		id;
    int		i;

    BI_DPRINT4(stderr, "%s:%s(%2d, %s)\n",
	       IN_CLIENT_MODE() ? ARGV0(CurClient) : "server",
	       name,
	       sig,
	       (disp == SIG_DFL) ? "SIG_DFL" :
	       (disp == SIG_IGN) ? "SIG_IGN" :
	       (disp == SIG_HOLD) ? "SIG_HOLD" : "FUNC");

    if ((disp == SIG_HOLD) ||
	(sig == SIGTRAP) || (sig == SIGUSR1) || (sig == SIGUSR2))
    {
	return(CallRealSigFunc(name, real_func, sig, disp));
    }

    /* Treat "kill" signals as "system generated".  When server gets these
     * signals, they are destined for server NOT builtin client(s).  Ignore
     * calls from clients attempting to set disp for these signals: we won't
     * dispatch these signals to them and we want server to control disp for
     * these signals.
     */
    if (IN_CLIENT_MODE() && ((sig == SIGHUP) || (sig == SIGINT) ||
			     (sig == SIGQUIT) || (sig == SIGTERM)))
    {
	return(SIG_IGN);
    }

    id = IN_CLIENT_MODE() ? CurClient : SERVER_ID;
    if (sig < NumSigs)
    {
	prev_disp = (SIG_DISP(sig, ID2INDX(id)) != NO_DISP) ?
	    SIG_DISP(sig, ID2INDX(id)) : DEF_SIG_DISP(sig);

    } else
    {
	int s;

	sig_tbl = (struct SigTblRec *)
	    Xrealloc(sig_tbl, (sig + 1) * sizeof(struct SigTblRec));

	/* "Uninitialize" *all* new entries */
	for (s = NumSigs; s <= sig; s++)
	{
	    DEF_SIG_DISP(s) = CUR_SIG_DISP(s) = NO_DISP;
	    for(i = 0; i < MAX_INDX; i++)
	    {
		SIG_DISP(s, i) = NO_DISP;
		SIG_FLAGS(sig, i) = 0;
	    }
	}
	prev_disp	= NO_DISP;
	NumSigs		= sig + 1;
    }

    SIG_DISP(sig, ID2INDX(id)) = disp;
    if (reset)
	SIG_FLAGS(sig, ID2INDX(id)) |= RESET;
    else
	SIG_FLAGS(sig, ID2INDX(id)) &= ~RESET;

    /* The real signal function (signal() or sigset()) must be called to set
     * the signal disposition and/or to return the existing signal
     * disposition.  Based on "disp" passed in, determine what new disp
     * should be.  If this is different from current disp or prev disp is
     * not known, call real signal function.  (If prev disp is not known,
     * then default disp is not known and this is set, too.)
     */
    new_disp = NO_DISP;
    for (i = 0; i < MAX_INDX; i++)
	if (DISP_IS_FUNC(SIG_DISP(sig, i)))
	{
	    if (new_disp != NO_DISP)
	    {
		new_disp = sighandler;
		break;
	    }
	    new_disp = SIG_DISP(sig, i);
	}

    if ((new_disp == NO_DISP) || (CUR_SIG_DISP(sig) == NO_DISP) ||
	(new_disp != CUR_SIG_DISP(sig)) || (prev_disp == NO_DISP))
    {
	SigDisp prev;

	CUR_SIG_DISP(sig) = (new_disp != NO_DISP) ? new_disp : disp;
	prev = CallRealSigFunc(name, real_func, sig, CUR_SIG_DISP(sig));
	if (prev_disp == NO_DISP)
	    prev_disp = DEF_SIG_DISP(sig) = prev;
    }
    return(prev_disp);
}

/***************************************************************************
 * signal- override signal().
 */
void (*signal(int sig, void (*disp)(int)))(int)
{
    static void (*(*real_func)(int, void (*)(int)))(int) = NULL;

    return(sig_func("signal", &real_func, 1, sig, disp));
}

/***************************************************************************
 * sigset- override sigset().
 */
void (*sigset(int sig, void (*disp)(int)))(int)
{
    static void (*(*real_func)(int, void (*)(int)))(int) = NULL;

    return(sig_func("sigset", &real_func, 0, sig, disp));
}

/***************************************************************************
 * BISigICExiting- i-c is exiting so clean up its entries in sigtbl.
 *	NOTE: should signal() be called to change disp?
 */
void
BISigICExiting(int ic)
{
    int sig;

    for (sig = 0; sig < NumSigs; sig++)
	if (SIG_DISP(sig, ID2INDX(ic)) != NO_DISP)
	    SIG_DISP(sig, ID2INDX(ic)) = NO_DISP;
}

/****************************************************************************
 * BICallTerminatingSigHandler- if client has SIGTERM handler, call it to
 *	force client to exit.
 */
int
BICallTerminatingSigHandler(int ic)
{
    if (DISP_IS_FUNC(SIG_DISP(SIGTERM, ID2INDX(ic))))
    {
	CALL_SIG_HANDLER(ic, SIG_DISP(SIGTERM, ID2INDX(ic)), SIGTERM);
	return(1);
    }
    return(0);
}
