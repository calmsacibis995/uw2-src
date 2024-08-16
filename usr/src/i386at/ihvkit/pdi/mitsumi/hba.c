/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:pdi/mitsumi/hba.c	1.1"
#ident	"@(#)ihvkit:pdi/mitsumi/hba.c	1.1"
#ident	"$Header: $"

/*******************************************************************************
 *******************************************************************************
 *
 *	HBA.C
 *
 *	Generic part of an HBA
 *
 *	Notes :
 *		- contains all the upper level HBA routines
 *		
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	INCLUDES
 *
 ******************************************************************************/

#include <sys/types.h>
#include <sys/debug.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/systm.h>
#ifndef lint
#include <sys/byteorder.h>
#endif
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/moddefs.h>
#include <sys/xdebug.h>
#include <sys/conf.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/dynstructs.h>
#include <sys/cred.h>
#include "crmc5s.h"
#include <sys/hba.h>

#include <sys/ddi_i386at.h>
#include <sys/ddi.h>
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif

/*******************************************************************************
 *
 *	DEFINES AND THINGS
 *
 ******************************************************************************/

/*******************************************************************************
 *
 *	GLOBAL VARIABLES
 *
 ******************************************************************************/

int			crmc5sdevflag = 0;

HBA_INFO( crmc5s, 0, 0x10000 );	/* hba driver info */
struct hba_info *crmc5shbainfo = &crmc5shba_info;

/*******************************************************************************
 *
 *	EXTERNALS
 *
 ******************************************************************************/

extern struct head	sm_poolhead;		/* 28 byte structs */
extern char		crmc5s_vendor [];
extern char		crmc5s_prod [];
extern char		crmc5s_diskprod [];
extern int		crmc5s_gtol [];	/* global to local */
extern int		crmc5s_ltog [];	/* local to global */
extern scsi_ha_t	*crmc5s_sc_ha;		/* host adapter structs */
extern struct hba_idata	crmc5sidata [];		/* hardware info */
extern void 		mod_drvattach();
extern void 		mod_drvdetach();


/*******************************************************************************
 *
 *	PROTOTYPES
 *
 ******************************************************************************/

/*
 *	Loadable Driver Routines
 */

int			crmc5s_load( void );
int			crmc5s_unload( void );

/*
 *	queue support
 */

void 			crmc5s_putq( scsi_lu_t *q, sblk_t *sp );
void 			crmc5s_next( scsi_lu_t *q );
void 			crmc5s_flushq( scsi_lu_t *q, int cc );
int 			crmc5s_illegal( ushort_t c, uchar_t t, uchar_t l, int m );

/*
 *	scsi pass-thru
 */

void 	crmc5s_pass_thru( struct buf *bp );
void 	crmc5s_int( struct sb *sp );

/*
 *	Externals into the "custom" section
 */

extern int		crmc5sinit( void );
extern int		crmc5sstart( void );

/*******************************************************************************
 *
 *	Wrapper 
 *
 ******************************************************************************/

MOD_HDRV_WRAPPER( crmc5s, crmc5s_load, crmc5s_unload, NULL, DRVNAME );

/*******************************************************************************
 *******************************************************************************
 *
 *	Loadable Driver Stuff
 *
 *	Routines:
 *		crmc5s_load			loader load 
 *		crmc5s_unload			loader unload 
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	crmc5s_load() 
 *
 *	Load and register the driver on demand
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		0		cool
 *		x		error
 * 
 ******************************************************************************/


int
crmc5s_load( void )
{
	if (crmc5sinit()) {
		/*
		 *	unable to locate drivers
 		 */
		cmn_err( CE_CONT, "!crmc5s: Unable to locate controllers\n");
		return( ENODEV );
	}
	mod_drvattach( &crmc5s_attach_info );
	if (crmc5sstart()) {
		mod_drvdetach (&crmc5s_attach_info);
		return(ENODEV);
	}
	return( 0 );
}

/*******************************************************************************
 *
 *	crmc5s_unload( )
 *
 *	Unload and unregister the driver
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		0		cool
 *		x		error
 *
 ******************************************************************************/

int
crmc5s_unload( void )
{
	return( EBUSY );
}

/*******************************************************************************
 *******************************************************************************
 *
 *	HBA SCSI Block Routines
 *
 *	Routines:
 *		crmc5sgetblk			get a scsi block
 *		crmc5sfreeblk			free a scsi block
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	crmc5sgetblk( void )
 *
 *	Allocate a SCSI block(hbadata) for the caller
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		*		ptr to the hbadata
 *
 *	Notes :
 *		- this routine CAN sleep
 *
 ******************************************************************************/

/*ARGSUSED*/
struct hbadata *
HBAGETBLK( int sleepflag )
{
	sblk_t		*sp;


	sp =(sblk_t *) SDI_GET(&sm_poolhead, sleepflag);

	return((struct hbadata *) sp );
}

/*******************************************************************************
 *
 *	crmc5sfreeblk( struct hbadata *hbap )
 *
 *	Free a SCSI data block 
 *
 *	Entry :
 *		*hbap		ptr to block to free
 *
 *	Exit :
 *		SDI_RET_OK	cool
 *
 ******************************************************************************/

long
HBAFREEBLK( struct hbadata *hbap )
{
	sblk_t			*sp =(sblk_t *) hbap;

	/*
	 *	free any personal stuff here if necessary
	 */
	sdi_free(&sm_poolhead, (jpool_t *)sp);

	return( SDI_RET_OK );
}


/******************************************************************************
 *
 *	crmc5sxlat( struct hbadata *hbap, int flag, struct proc *procp )
 *
 *	Crunch the SCB's data pointers from virtual to physical
 *
 *	Entry :
 *		*hbap		ptr to job request
 *		flags		flags for type
 *		*procp		ptr to user's process
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *
 *****************************************************************************/
/*ARGSUSED*/
HBAXLAT_DECL
HBAXLAT( struct hbadata *hbap, int flag, struct proc *procp, int sleepflag )
{
	sblk_t			*sp = (sblk_t *) hbap;

	if (!sp->sbp->sb.SCB.sc_datapt ) {
		/*
		 *	no data -- make sure datasz is 0
		 */
		sp->sbp->sb.SCB.sc_datasz = 0;
	}
	HBAXLAT_RETURN(0);
}

/*******************************************************************************
 *******************************************************************************
 *
 *	Command Send Routines(down from the PDI)
 *
 *	Routines:
 *		crmc5ssend				send a cmd to ctrlr
 *		crmc5sicmd				send an IMMEDIATE cmd to ctrlr
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	crmc5ssend( struct hbadata *hbap )
 *	
 *	Send a SCSI command to the controller
 *
 *	Entry :
 *		*hbap		ptr to scsi command
 *
 *	Exit :
 *		SDI_RET_OK	cool
 *		SDI_RET_RETRY	queue full -- try again later
 *		SDI_RET_ERR	something went wrong
 *
 *	Notes :
 *
 *************************************************************************/

/*ARGSUSED*/
long
HBASEND( struct hbadata *hbap, int sleepflag )
{
	struct scsi_ad		*sa;
	scsi_lu_t		*q;
	sblk_t			*sp =(sblk_t *) hbap;
	int			c;
	int			t;
	int			l;
	pl_t			oip;

#ifdef CRMC5S_DEBUG
	cmn_err(CE_NOTE, "crmc5ssend entered sb %x",
		&sp->sbp->sb);
#endif

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = crmc5s_gtol [ SDI_HAN(sa) ];
	t = SDI_TCN( sa );
	l = SDI_LUN( sa );

	if ( sp->sbp->sb.sb_type != SCB_TYPE ) {
		/*
		 *	we only handle scsi command blocks
		 */
		cmn_err(CE_NOTE, "crmc5ssend sb_type != SCB_TYPE");
		return( SDI_RET_ERR );
	}

	q = &LU_Q( c, t, l );

	oip = spldisk();
	if ( q->q_flag & QPTHRU ) {
		splx( oip );
#ifdef CRMC5S_DEBUG
		cmn_err( CE_CONT, "queue busy for pass thru\n");
#endif
		return( SDI_RET_RETRY );
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;
	crmc5s_putq( q, sp );
	crmc5s_next( q );
	splx( oip );
	return( SDI_RET_OK  );
}

/*******************************************************************************
 *
 *	crmc5sicmd( struct hbadata *hbap )
 *
 *	Send an immediate command.  If the logical unit is busy, the job
 *	will be queued until the unit is free.  SFB operations will take
 *	priority over SCB operations.
 *
 *	Entry :
 *		*hbap		ptr to the command
 *
 *	Exit :
 *		SDI_RET_OK	cool
 *		SDI_RET_ERR	something went wrong
 *
 *	Notes :
 *
 ******************************************************************************/

/*ARGSUSED*/
long
HBAICMD( struct hbadata *hbap, int sleepflag )
{
	int			c;		/* controller */
	int			t;		/* target */
	int			l;		/* lun */
	sblk_t			*sp =(sblk_t *) hbap;
	struct scsi_ad		*sa;		/* scsi address */
	struct ident		*inq_data;
	struct scs		*inq_cdb;
	scsi_lu_t		*q;
	pl_t			oip;

#ifdef CRMC5S_DEBUG
	cmn_err(CE_NOTE, "crmc5sicmd entered sb %x",
		&sp->sbp->sb);
#endif

	oip  = spldisk();

	switch( sp -> sbp -> sb.sb_type ) {
	case SFB_TYPE :
		/*
		 *	special function type 
		 */
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = crmc5s_gtol [ SDI_HAN( sa ) ];
		t = SDI_TCN( sa );
		l = SDI_LUN( sa );
		q = &LU_Q(c,t,l);
		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch( sp->sbp->sb.SFB.sf_func ) {
		case SFB_RESUME :
			/*
			 *	start this guy back up again
			 */
			q->q_flag &=~QSUSP;
			crmc5s_next( q );
			break;
		case SFB_SUSPEND :
			/*
			 *	suspend this guy
			 */
			q->q_flag |= QSUSP;
			break;
		case SFB_RESETM :
			/*FALLTHRU*/
		case SFB_ABORTM :
			/*
			 *	abort a job 
			 */
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			crmc5s_putq( q, sp);
			crmc5s_next( q );
			splx( oip );
			return( SDI_RET_OK );
		case SFB_FLUSHR :
			/*
			 *	flush a queue
			 */
			crmc5s_flushq( q, SDI_QFLUSH );
			break;
		case SFB_NOPF :
			/*
			 *	a nothingness instruction
			 */
			break;
		default :
			/*
			 *	don't know this one
			 */
			sp->sbp->sb.SFB.sf_comp_code =(ulong_t) SDI_SFBERR;
		} /* switch( sp->sbp->sb.SFB.sf_func ) */

		sdi_callback( (struct sb *)sp->sbp );
		splx( oip );
		return( SDI_RET_OK  );

	case ISCB_TYPE :
		/*
		 *	scsi command block type
		 */
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = crmc5s_gtol [ SDI_HAN( sa ) ];
		t = SDI_TCN( sa );
		l = SDI_LUN( sa );
		q = &LU_Q(c, t, l );

		inq_cdb =( struct scs *)(void *) sp->sbp->sb.SCB.sc_cmdpt;
		if (( t == crmc5s_sc_ha[c].ha_id ) &&(l == 0) &&(inq_cdb->ss_op == SS_INQUIR) ) {
			/*	
			 *	controller id is being requested 
			 */
			inq_data =( struct ident * )(void *) sp->sbp->sb.SCB.sc_datapt;
			inq_data->id_type = ID_PROCESOR;
			(void)strncpy(inq_data->id_vendor,crmc5sidata[c].name,24 );
			inq_data->id_vendor[23] = NULL;
			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			splx( oip );
			return( SDI_RET_OK );
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		crmc5s_putq( q, sp );
		crmc5s_next( q );
		splx( oip );
		return( SDI_RET_OK  );

	default :
		/*
		 *	some random type -- you're outta here
		 */
		splx( oip );
		return( SDI_RET_ERR  );
	} /* switch( sp->sbp->sb.sb_type ) */
}


/*******************************************************************************
 *******************************************************************************
 *
 *	QUEUE Management
 *
 *	Routines: 
 *		crmc5s_putq			put an entry on the queue
 *		crmc5s_next			send next entry to the ctrlr
 *		crmc5s_flushq			flush a queue
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	crmc5s_putq( scsi_lu_t *q, sblk_t *sp )
 *
 *	Put a job on a logical queue 
 *
 *	Entry :
 *		*q		ptr to lun queue
 *		*sp		ptr to scsi job block
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *		- jobs are prioritized
 *
 *************/

void
crmc5s_putq( scsi_lu_t *q, sblk_t *sp )
{
	int			cls = QUECLASS( sp );
	sblk_t			*nsp;
	pl_t			ospl;

	ospl = spldisk();

	/*
	 *	insert job on the queue
	 */

	if ( !q->q_first ||(cls <= QUECLASS( q->q_last ) ) ) {
		/*
		 *	if the queue is empty OR the queue class of the new
		 *	job is LESS than that of the last on on the queue then
		 *	insert onto the END of the queue
		 */
		if ( q->q_first ) {	/* something on the queue */
			q->q_last->s_next = sp;
			sp->s_prev = q->q_last;
		}
		else {			/* nothing on the queue */
			q->q_first = sp;
			sp->s_prev = NULL;
		}
		sp->s_next = NULL;
		q->q_last = sp;
	}
	else {
		/*
		 *	sort it into the list of jobs on the queue and insert
		 */
		nsp = q->q_first;
		while ( QUECLASS( nsp ) >= cls )
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if ( nsp->s_prev )
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}
	q->q_depth ++;		/* its getting deeper */
	splx( ospl );
}

/*******************************************************************************
 *
 *	crmc5s_next( scsi_lu_t *q ) 
 *
 *	Send the next job on the logical queue
 *
 *	Entry :
 *		*q		ptr to queue
 *
 *	Exit :
 *		Nothing 
 *
 *	Notes :
 *		- if QBUSY then don't send any jobs
 *
 ******************************************************************************/

void
crmc5s_next( scsi_lu_t *q )
{
	sblk_t			*sp;
	pl_t			ospl;

	ospl = spldisk();


	if ( q->q_flag & QBUSY ) {
		/*	
		 *	queue is "busy" -- don't execute more jobs
		 */
		splx( ospl );
		return;
	}

	if (( sp = q->q_first ) == NULL ) {
		/*
		 *	queue is empty -- reset flags and leave
		 */
		q->q_depth = 0;
		splx( ospl );
		return;
	}

	if ( q->q_flag & QSUSP && sp->sbp->sb.sb_type == SCB_TYPE ) {
		/*
		 *	queue is suspended
		 */
		splx( ospl );
		return ;
	}


	/*
	 *	now check a whole bunch of things for special job
	 *	processing and such
	 */


	if ( sp->sbp->sb.sb_type == SFB_TYPE ) {
		/*
		 *	special function type
		 */
		if ( !(q->q_first = sp->s_next ) )
			q->q_last = NULL;

		q->q_depth --;
		splx( ospl );
		crmc5s_func( sp );
	}
	else {
		/*	
		 *	regular function type 	
		 */

		if ( !(q->q_first = sp->s_next ) )
			q->q_last = NULL;

		q->q_depth --;
		splx( ospl );
		crmc5s_cmd( sp );
	}
}

/*******************************************************************************
 *
 *	crmc5s_flushq( scsi_lu_t *q, int cc )
 *
 *	Empty a logical unit queue
 *
 *	Entry :
 *		*q		ptr to logical queue
 *		cc		completion code
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *		- *cc* is the job completion code for the flushed job
 *
 ******************************************************************************/

void
crmc5s_flushq( scsi_lu_t *q, int cc )
{
	sblk_t			*sp;
	sblk_t			*nsp;
	pl_t			ospl;

	ASSERT( q == NULL );

	ospl = spldisk();

	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;
	q->q_depth = 0;
	q->q_flag = 0;

	while ( sp ) {
		nsp = sp->s_next;
		sp->sbp->sb.SCB.sc_comp_code =(ulong_t) cc;
		sdi_callback( (struct sb *)sp->sbp );
		sp = nsp;
	}

	splx( ospl );
}

/*******************************************************************************
 *******************************************************************************
 *
 *	HBA Information Requests
 *
 *	Routines:
 *		crmc5sgetinfo			return info about adapter
 *		crmc5s_illegal			check for illegal requests
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	crmc5s_illegal( ushort_t c, uchar_t t, uchar_t l, int m )
 *
 *	Check to see if there is an EDT entry for this scsi device
 *
 *	Entry :
 *		c		controller number
 *		t		target number
 *		l		logical unit number
 *		m		major number( not used )
 *
 *	Exit :
 *		0		cool
 *		1		no entry
 *
 ******************************************************************************/

/*ARGSUSED*/
int
crmc5s_illegal( ushort_t c, uchar_t t, uchar_t l, int m )
{
	if ( sdi_redt( c, t, l ) ) {
		/*
	 	 *	entry exists
		 */
		return( 0 );
	}

	/*
	 *	no entry existed
	 */

	return( 1 );
}
/*******************************************************************************
 *
 *	crmc5sgetinfo( struct scsi_ad *sa, struct hbagetinfo *getinfo )
 *
 *	Fill in the structures with information about this SCSI adapter
 *
 *	Entry :
 *		*sa		ptr to scsi adapter address struct
 *		*getinfo	structure to fill in
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
HBAGETINFO( struct scsi_ad *sa, struct hbagetinfo *getinfo )
{
	char			*s1, *s2;
	static char		temp [] = "HA X TC X";

	s1 = temp;
	s2 = getinfo -> name;
	temp [ 3 ] = SDI_HAN( sa ) + '0';
	temp [ 8 ] = SDI_TCN( sa ) + '0';

	while (( *s2++ = *s1++ ) != '\0' ) ;

	getinfo->iotype = F_PIO;
#if PDI_VERSION >= PDI_SVR42MP
	if (getinfo->bcbp) {
		extern struct hba_info *crmc5shbainfo;
		getinfo->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfo->bcbp->bcb_max_xfer = crmc5shbainfo->max_xfer;
#ifdef CRMC5S_WHEN_USING_DMA
		getinfo->bcbp->bcb_flags = BCB_PHYSCONTIG;
		getinfo->bcbp->bcb_physreqp->phys_align = 2;
		getinfo->bcbp->bcb_physreqp->phys_boundary = 128 * 1024;
		getinfo->bcbp->bcb_physreqp->phys_dmasize = 24;
#endif
	}
#endif
}

/*******************************************************************************
 *******************************************************************************
 *
 *	SCSI Pass-Thru Support
 *
 *	Routines:
 *		crmc5sopen				open the device for passthru
 *		crmc5sclose			close the passthru device
 *		crmc5sioctl			special ioctls for passthru
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	crmc5sopen( dev_t *devp, int flags, int otype, cred_t *crp )
 *
 *	Pass thru open 
 *
 *	Entry : 
 *		*devp		ptr to device numbers
 *		flags		flags for opening
 *		otype		type of open
 *		*crp 		ptr to user's credentials
 *
 *	Exit :
 *		0		cool
 *		EBUSY		device busy
 *		EPERM		not allowed to do this
*/

/*ARGSUSED*/
int
HBAOPEN( dev_t *devp, int flags, int otype, cred_t *crp )
{
	dev_t			dev = *devp;
	int			c = crmc5s_gtol [ SC_HAN(dev) ];
	int			t = SC_TCN( dev );
	int			l = SC_LUN( dev );
	scsi_lu_t		*q;
	int			oip;

	if ( drv_priv( crp ) ) {
		/*
		 *	not allowed to do this
		 */
		return( EPERM );
	}

	if (crmc5s_illegal(SC_HAN(dev), t, l, 0)) {
		return(ENXIO);
	}
	if ( t == crmc5s_sc_ha[c] . ha_id ) {
		/*
		 *	opening the controller
		 */
		return( 0 );
	}

	/*
	 *	open for pass-thru
	 */

	q = &LU_Q( c, t, l );

	oip = spldisk();
	if (( q->q_count > 0 ) ||( q->q_flag &(QBUSY | QSUSP | QPTHRU ) ) ) {
		/*
		 *	sorry -- in use
		 */
		splx( oip );
		return( EBUSY );
	}

	/*
	 *	give it to him
	 */

	q->q_flag |= QPTHRU;
	splx( oip );
	return( 0 );
}

/*******************************************************************************
 *
 *	crmc5sclose( dev_t dev, int flags, int otype, cred_t *crp )
 *
 *	Close connection to board/lu
 *
 *	Entry :
 *		dev		device numbers
 *		flags		flags from open
 *		otype		type of open
 *		*crp		ptr to user's credentials
 *
 *	Exit :
 *		0		cool
 *		ENXIO 		not on this guy you don't
 *
 *	Notes :
 *
 ******************************************************************************/

/*ARGSUSED*/
int
HBACLOSE( dev_t dev, int flags, int otype, cred_t *crp )
{
	int			c = crmc5s_gtol [ SC_HAN(dev) ];
	int			t = SC_TCN( dev );
	int			l = SC_LUN( dev );
	scsi_lu_t		*q;
	pl_t			oip;

	if ( t == crmc5s_sc_ha[c].ha_id ) {
		/*
		 *	feel free to close the scsi controller
	 	 */
		return( 0 );
	}

	/*
 	 *	deal with a pass-thru
	 */

	q = &LU_Q( c, t, l );

	oip = spldisk();
	q->q_flag &= ~QPTHRU;

	if ( q->q_func != NULL ) {
		/*	
		 *	we need to tell the guy about it
		 */
		(*q->q_func)( q->q_param, SDI_FLT_PTHRU );
	}

	crmc5s_next( q );
	splx( oip );
	return( 0 );
}

/*******************************************************************************
 *
 *	crmc5sioctl( dev_t devp, int cmd, int arg, int mode, cred_t *crp,int *rval_p )
 *
 *	IOCTLs for pass-thru
 *	
 *	Entry :
 *		devp		device numbers
 *		cmd		command to do
 *		arg		passes params
 *		mode		values from open
 *		*crp		ptr to user's credentials
 *		*rval_p		ptr to return value
 *
 *	Exit :
 *		0		cool
 *		EINVAL		invalid command
 *
 *	Notes :
 *
 ******************************************************************************/

/*ARGSUSED*/
int
HBAIOCTL( dev_t devp, int cmd, caddr_t arg, int mode, cred_t *crp, int *rval_p )
{
	register int	c = crmc5s_gtol[SC_HAN(devp)];
	register int	t = SC_TCN(devp);
	register struct sb *sp;
	int  uerror = 0;
	extern struct ver_no	crmc5s_sdiver;	/* SDI version struct */
	static	char	sc_cmd [ MAX_CMDSZ ];	/* table for passthru */

	switch(cmd) {
	case SDI_SEND: 
		{
			register buf_t *bp;
			struct sb  karg;
			int  rw;
			char *save_priv;

			if (t == crmc5s_sc_ha[c].ha_id) { 	/* illegal ID */
				return(ENXIO);
			}
			if (copyin(arg,(caddr_t)&karg, sizeof(struct sb))) {
				return(EFAULT);
			}
			if ((karg.sb_type != ISCB_TYPE) ||
			    (karg.SCB.sc_cmdsz <= 0 )   ||
			    (karg.SCB.sc_cmdsz > MAX_CMDSZ )) {
				return(EINVAL);
			}

			sp = sdi_getblk( KM_SLEEP );
			save_priv = sp->SCB.sc_priv;
			bcopy((caddr_t)&karg,(caddr_t)sp, sizeof(struct sb));

			bp = getrbuf(KM_SLEEP);
			bp->b_iodone = NULL;
			sp->SCB.sc_priv = save_priv;
			sp->SCB.sc_cmdpt = sc_cmd;

			if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
			    sp->SCB.sc_cmdsz)) {
				uerror = EFAULT;
				goto done;
			}

			rw =(sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
			bp->b_private =(char *)sp;

			/*
			 * If the job involves a data transfer then the
			 * request is done thru physiock() so that the user
			 * data area is locked in memory. If the job doesn't
			 * involve any data transfer then crmc5s_pass_thru()
			 * is called directly.
			 */
			if (sp->SCB.sc_datasz > 0) {
				struct iovec  ha_iov;
				struct uio    ha_uio;

				ha_iov.iov_base = sp->SCB.sc_datapt;
				ha_iov.iov_len = sp->SCB.sc_datasz;
				ha_uio.uio_iov = &ha_iov;
				ha_uio.uio_iovcnt = 1;
				ha_uio.uio_offset = 0;
				ha_uio.uio_segflg = UIO_USERSPACE;
				ha_uio.uio_fmode = 0;
				ha_uio.uio_resid = sp->SCB.sc_datasz;

				if (uerror = physiock(crmc5s_pass_thru, bp, devp, rw, 
				    4194303, &ha_uio)) {
					goto done;
				}
			} else {
				bp->b_un.b_addr = sp->SCB.sc_datapt;
				bp->b_bcount = sp->SCB.sc_datasz;
				bp->b_blkno = NULL;
				bp->b_edev = devp;
				bp->b_flags |= rw;

				crmc5s_pass_thru(bp);  /* fake physio call */
				biowait(bp);
			}

			/* update user SCB fields */

			karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
			karg.SCB.sc_status = sp->SCB.sc_status;
			karg.SCB.sc_time = sp->SCB.sc_time;

			if (copyout((caddr_t)&karg, arg, sizeof(struct sb)))
				uerror = EFAULT;

done:
			freerbuf(bp);
			sdi_freeblk(sp);
			break;
		}

	case B_GETTYPE:
		if (copyout("scsi",((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}
		if (copyout("scsi",((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}
		break;

	case	HA_VER:
		if (copyout((caddr_t)&crmc5s_sdiver, arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	default:
		return(EINVAL);
	}
	return(uerror);
}



void
crmc5s_pass_thru( struct buf *bp )
{
	int	c = crmc5s_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	register struct scsi_lu	*q;
	register struct sb *sp;
	int  oip;

	sp =(struct sb *) (void *) bp->b_private;

	sp->SCB.sc_dev.sa_lun = (unsigned char)l;
	sp->SCB.sc_dev.sa_fill =(crmc5s_ltog[c] << 3) | t;
	sp->SCB.sc_wd =(long)bp;
	sp->SCB.sc_datapt =(caddr_t) paddr(bp);
	sp->SCB.sc_int = crmc5s_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP );

	q = &LU_Q(c, t, l);

	oip = spldisk();

	crmc5s_putq(q,(sblk_t *)((struct xsb *)sp)->hbadata_p);
	crmc5s_next( q );

	splx( oip );
}




/*******************************************************************************
 *
 *	crmc5s_int( struct sb *sp )
 *
 *	Interrupt handler for pass-thru jobs -- it just comes
 *	around and wakes up the sleeping process
 *
 *	Entry :
 *		*sp		ptr to scsi block
 *	
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *
 ******************************************************************************/

void
crmc5s_int( struct sb *sp )
{
	struct buf		*bp;

	bp =(struct buf *) sp->SCB.sc_wd;
	biodone( bp );

}
