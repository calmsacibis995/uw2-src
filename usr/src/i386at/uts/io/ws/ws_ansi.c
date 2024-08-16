/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/ws/ws_ansi.c	1.6"
#ident	"$Header: $"


#include <io/ascii.h>
#include <io/ansi/at_ansi.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/tty.h>
#include <io/ws/tcl.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>


extern wstation_t Kdws;


/*
 * int
 * wsansi_parse(kdcnops_t *, channel_t *, unchar *, int)
 *
 * Calling/Exit State:
 *	None.
 */
int
wsansi_parse(kdcnops_t *cnops, channel_t *chp, unchar *addr, int cnt)
{
	termstate_t	*tsp = &chp->ch_tstate;
	ushort		ch; 


	while (cnt--) {
		ch = *addr++ & 0xFF; 
		if (ch == A_ESC || ch == A_CSI || 
		    (ch < ' ' && tsp->t_font == ANSI_FONT0)) {
#ifndef NO_MULTI_BYTE
			if (chp->ch_dmode == KD_GRTEXT)
				cnops->cn_gs_ansi_cntl(cnops, chp, tsp, (ushort)ch);
			else
				wsansi_cntl(cnops, chp, tsp, ch);
#else
			wsansi_cntl(cnops, chp, tsp, ch);
#endif /* NO_MULTI_BYTE */

		} else {

#ifndef NO_MULTI_BYTE
			if (chp->ch_dmode == KD_GRTEXT)
				cnops->cn_gcl_norm(cnops, chp, tsp, (ushort)ch);
			else
				tcl_norm(cnops, chp, tsp, ch);
#else
			tcl_norm(cnops, chp, tsp, ch);
#endif /* NO_MULTI_BYTE */
		}
	}  /* while (cnt--) */

	return (0);
}


/*
 * int
 * wsansi_cntl(kdcnops_t *, channel_t *, termstate_t *, ushort)
 *
 * Calling/Exit State:
 *	None.
 *
 * Note:
 *	The control characters like the BELL are not processed because
 *	the console switch (conssw) entry points cannot hold any locks.
 *	The problem was caused by the sysmsg driver which now holds the
 *	cmn_err lock before calling the kd console entry points. Since,
 *	the cmn_err lock has the highest hierarchy and any locks acquired
 *	would cause the hierarchy violation. Since kdtone acquires the
 *	mutex lock to process the BELL character, a lock hierarchy
 *	violation occurs. This is the reason the BELL character and
 *	other control characters are ignored in the console entry points.
 */
int
wsansi_cntl(kdcnops_t *cnops, channel_t *chp, termstate_t *tsp, ushort ch)
{
	switch (ch) {
/*
	case A_BEL:
		(*cnops->cn_bell)(&Kdws, chp);
		break;
*/

	case A_BS:
		tcl_bs(cnops, chp, tsp);
		break;

	case A_HT:
		tcl_ht(cnops, chp, tsp);
		break;

	case A_NL:
	case A_VT:
		if (tsp->t_row == tsp->t_rows - 1) {
			tsp->t_ppar[0] = 1;
			tcl_scrlup(cnops, chp, tsp);
		} else {
			tsp->t_row++;
			tsp->t_cursor += tsp->t_cols;
		}
		(*cnops->cn_setcursor)(chp, tsp);
		break;

	case A_FF:
		tcl_reset(cnops, chp, tsp);
		break;

	case A_CR:
		tsp->t_cursor -= tsp->t_col;
		tsp->t_col = 0;
		(*cnops->cn_setcursor)(chp, tsp);
		break;

	case A_GS:
		tcl_bht(cnops, chp, tsp);
		break;

	default:
		break;
	}

	return (0);
}
