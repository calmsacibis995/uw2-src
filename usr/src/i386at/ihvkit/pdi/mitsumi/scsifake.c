/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:pdi/mitsumi/scsifake.c	1.1"
#ident	"@(#)ihvkit:pdi/mitsumi/scsifake.c	1.1"
#ident	"$Header: $"

/*******************************************************************************
 *******************************************************************************
 *
 *	SCSIFAKE.C
 *
 *	SDI to NON-SCSI Interface
 *
 *	Notes :
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	INCLUDES
 *
 ******************************************************************************/

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/privilege.h>
#include <sys/mkdev.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/bootinfo.h>
#include <sys/debug.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/dynstructs.h>
#include <sys/moddefs.h>
#include "crmc5s.h"
#include <sys/hba.h>

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif


/*******************************************************************************
 *
 *	EXTERNALS
 *
 ******************************************************************************/

extern int   crmc5s_gtol[MAX_HAS];	/* global to local */
extern int   crmc5s_ltog[MAX_HAS];	/* local to global */
extern ctrl_info_t *ctrl;	/* local controller structs */

/*******************************************************************************
 *
 *	PROTOTYPES
 *
 ******************************************************************************/

/*
 * Routines to handle translation
 */

void            crmc5s_func(sblk_t * sp);
void            crmc5s_cmd(sblk_t * sp);

extern int      crmc5s_test(struct sb * sb);
extern void     crmc5s_rw(sblk_t * sp);
extern void     crmc5s_inquir(struct sb * sb);
extern void     crmc5s_sense(struct sb * sb);
extern void     crmc5s_format(struct sb * sb);
extern void     crmc5s_rdcap(struct sb * sb);
extern void     crmc5s_mselect(struct sb * sb);
extern void     crmc5s_msense(struct sb * sb);

/******************************************************************************
 ******************************************************************************
 *
 *	SCSI TRANSLATION UTILITIES
 *
 ******************************************************************************
 *****************************************************************************/

/*******************************************************************************
 *
 *	crmc5s_func ( sblk_t *sp )
 *
 *	Process a FUNCTION request
 *
 *	Entry :
 *		*sp		ptr to scsi job
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *
 ******************************************************************************/

void
crmc5s_func(sblk_t * sp)
{
	struct scsi_ad *sa;
	int             c;
	int             t;

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = crmc5s_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	/*
	 * Build a SCSI_TRESET job and send it on its way
	 */

	cmn_err(CE_WARN, "crmc5s: (_func) HA %d TC %d wants to be reset\n", c, t);
	sdi_callback((struct sb *)sp->sbp);

}

/*******************************************************************************
 *
 *	crmc5s_cmd( sblk_t *sp )
 *
 *	Build and send an SCB associated command to the hardware
 *
 *	Entry :
 *		*sp		ptr to scsi job ptr
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *		- this routine ASSUMES that *sp* is SCB or ISCB -- no SFB
 *
 ******************************************************************************/

void 
crmc5s_cmd(sblk_t * sp)
{
	struct scsi_ad *sa;
	int             c;
	int             t;
	int		l;
	struct sb      *sb = (struct sb *) & sp->sbp->sb;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = crmc5s_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

	/*
	 * get and fillout a command block
	 */

	if (sb->SCB.sc_cmdsz == SCS_SZ) {
		/*
		 * 6 byte SCSI command
		 */
		struct scs     *scsp;	/* gross but true, folks */

		scsp = (struct scs *)(void *) sb->SCB.sc_cmdpt;

		switch (scsp->ss_op) {

		case SS_REWIND:
		case SS_ERASE:
		case SS_FLMRK:
		case SS_SPACE:
		case SS_LOAD:
		case SS_LOCK:
			cmn_err(CE_NOTE, "cmd:%d ctl:%d,%d,%d.", scsp->ss_op, 
			    c, t, l);
			sb->SCB.sc_comp_code = (unsigned long) SDI_ERROR;
			sdi_callback(sb);
			break;

		case SS_TEST:
			if(crmc5s_test(sb))
				sb->SCB.sc_comp_code = SDI_ASW;
			else sb->SCB.sc_comp_code = (unsigned long) SDI_ERROR;
			sdi_callback(sb);
			break;
		case SS_REQSEN:
			/*
			 * request sense
			 */
			crmc5s_sense(sb);
			sdi_callback(sb);
			break;

		case SS_READ:
		case SS_WRITE:
			/*
			 * issue read/write
			 */
			crmc5s_rw(sp);
			break;

		case SS_INQUIR:
			/*
			 * issue a device inquiry
			 */
			crmc5s_inquir(sb);
			sdi_callback(sb);
			break;

		case SS_RESERV:
		case SS_RELES:
			/*
			 * reserve/release unit
			 */
			sb->SCB.sc_comp_code = (unsigned long) SDI_ASW;
			sdi_callback(sb);
			break;

		case SS_MSENSE:
			/*
			 * mode sense
			 */
			crmc5s_msense(sb);
			sdi_callback(sb);
			break;

		case SS_SDDGN:
			/*
			 * send diagnostic
			 */
			sb->SCB.sc_comp_code = (unsigned long) SDI_ASW;
			sdi_callback(sb);
			break;

		case SS_REASGN:
			/*
			 * reassign blocks
			 */
			sb->SCB.sc_comp_code = (unsigned long) SDI_ASW;
			sdi_callback(sb);
			break;

		case 0x04:
			/*
			 * format unit
			 */
			crmc5s_format(sb);
			sdi_callback(sb);
			break;

		default:
			cmn_err(CE_WARN, "crmc5s: (_cmd) Unknown SCSI 6 BYTE command : %x\n", scsp->ss_op);
			sb->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
			sdi_callback(sb);
			break;
		}
	}
	/* 6 byte command */
	else if (sb->SCB.sc_cmdsz == SCM_SZ) {

		/*
		 * 10 byte SCSI command
		 */
		struct scm     *scm;
		scm = (struct scm *)(void *) (SCM_RAD(sb->SCB.sc_cmdpt));

		switch (scm->sm_op) {

		case SM_RDCAP:
			/*
			 * read drive capacity
			 */
			crmc5s_rdcap(sb);
			sdi_callback(sb);
			break;

		case SM_READ:
		case SM_WRITE:
			/*
			 * read/write extended
			 */
			crmc5s_rw(sp);
			break;

		case SM_SEEK:
			/*
			 * seek extended
			 */
			sb->SCB.sc_comp_code = (unsigned long) SDI_ASW;
			sdi_callback(sb);
			break;

		case 0x2fL:
			/*
			 * verify command
			 */
			sb->SCB.sc_comp_code = SDI_ASW;
			sdi_callback(sb);
			break;

		case SM_RDDL:
		case SM_RDDB:
		case SM_WRDB:
		case SD_ERRHEAD:
		case SD_ERRHEAD1:
			/*
			 * defect list, data buffer, error code junk
			 */
			sb->SCB.sc_comp_code = SDI_ASW;
			sdi_callback(sb);
			break;

		default:
			/*
			 * yet another unknown scsi command
			 */
			cmn_err(CE_WARN, "crmc5s: (_cmd) Unknown 10 Byte Command - %x\n", scm->sm_op);
			sb->SCB.sc_comp_code = (unsigned long)SDI_TCERR;
			sdi_callback(sb);
			break;
		}		/* 10 byte command */
	} else {
		/*
		 * some really bogus thing
		 */
		sb->SCB.sc_comp_code = (unsigned long) SDI_TCERR;
		sdi_callback(sb);
	}
}
