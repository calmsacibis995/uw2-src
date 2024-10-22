/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/target/sd01/sd01flt.c	1.20"
#ident	"$Header: $"

#include	<util/types.h>
#include	<io/vtoc.h>
#include	<io/target/sd01/fdisk.h>  /* Included for 386 disk layout */

#include	<fs/buf.h>		/* Included for dma routines   */
#include	<io/conf.h>
#include	<io/elog.h>
#include	<io/open.h>
#include	<io/target/altsctr.h>
#include	<io/target/alttbl.h>
#include	<io/target/scsi.h>
#include	<io/target/sd01/sd01.h>
#include	<io/target/sd01/sd01_ioctl.h>
#include	<io/target/sdi/dynstructs.h>
#include	<io/target/sdi/sdi.h>
#include	<io/target/sdi/sdi_edt.h>
#include	<io/target/sdi/sdi_hier.h>
#include	<io/uio.h>	/* Included for uio structure argument*/
#include	<mem/kmem.h>	/* Included for DDI kmem_alloc routines */
#include	<proc/cred.h>	/* Included for cred structure arg   */
#include	<proc/proc.h>	/* Included for proc structure arg   */
#include	<svc/errno.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/mod/moddefs.h>
#include	<util/param.h>

#ifndef PDI_SVR42
#include	<util/ipl.h>
#include	<util/ksynch.h>
#endif /* PDI_SVR42 */

#include	<io/ddi_i386at.h>
#include	<io/ddi.h>	/* must come last */

int sd01alloc_wkamp(struct disk *, struct partition *, struct alts_parttbl *),
    sd01alts_assign(unchar memmap[], daddr_t, daddr_t, int, int),
    sd01altsmap_vfy(struct alts_mempart *, daddr_t),
    sd01asgn_altsctr(struct disk *),
    sd01asgn_altsec(struct disk *),
    sd01ast_init(struct disk *),
    sd01bsearch(struct alts_ent buf[], int, daddr_t, int),
    sd01ck_badalts(struct disk *, int),
    sd01ck_gbad(struct disk *),
    sd01ent_merge(struct alts_ent buf[], struct alts_ent list1[], int, struct alts_ent list2[], int),
    sd01gen_altsctr(struct disk *),
    sd01get_altsctr(struct disk *, struct  partition *),
    sd01get_altsec(struct disk *),
    sd01icmd(struct disk *, char, uint, char *, uint, uint, ushort, void (*)()),
    sd01remap_altsctr(struct disk *),
    sd01remap_altsec(struct disk *),
    sd01wr_altsctr(struct disk *);

void sd01_bbremap(struct disk *, int, int),
     sd01ast_free(struct disk *),
     sd01_bbdone(struct job *),
     sd01compress_ent(struct alts_ent buf[], int),
     sd01compress_map(struct alts_mempart *),
     sd01expand_map(struct alts_mempart *),
     sd01flt(struct disk *, long),
     sd01flterr(struct disk *, int),
     sd01fltjob(struct disk *),
     sd01flts(struct disk *, struct job *),
     sd01free_wkamp(struct disk *),
     sd01intalts(struct sb *),
     sd01intb(struct sb *),
     sd01intfq(struct sb *),
     sd01intres(struct sb *),
     sd01intrq(struct sb *),
     sd01ints(struct sb *),
     sd01isfb(struct disk *, ulong),
     sd01rel_altsctr(struct alts_mempart *),
     sd01sort_altsctr(struct alts_ent buf[], int),
     sd01upd_altsec(struct disk *),
     sd01upd_altstbl(struct disk *),
     sd01xalt(struct disk *, struct alt_info *, struct alts_ent *);
struct alts_ent *sd01_findaltent(struct disk *);

extern struct job *sd01getjob();
extern	void	fshadbad();
extern struct head	lg_poolhead;	/* head for dync struct routines */
extern struct head	sm_poolhead;	/* head for dync struct routines */

extern sleep_t	*sdi_rinit_lock;
extern int	sd01_rinit_waiting;

#ifdef SD01_DEBUG
extern daddr_t sd01alts_badspot;
extern struct disk *dk_badspot;
extern uint sd01alts_debug;
#endif /* SD01_DEBUG */
/*
 * Messages used by bad block handling.
 */

static char *hde_mesg[] = {
        "!Disk Driver: Soft read error corrected by ECC algorithm: block %d",

	"Disk Driver: A potential bad block has been detected (block %d)\n\
in a critical area of the hard disk. If this block goes bad, the UNIX System\n\
might fail during the next reboot.  Please backup your system now.",

	"!Disk Driver: A potential bad block has been detected (block %d).\n\
The drive is out of spare blocks for surrogates. The block will be \n\
remapped to the alternate sector/track partition.",

	"!Disk Driver: A potential bad block has been detected (block %d).\n\
The controller can not reassign a surrogate block. The block will be \n\
remapped to the alternate sector/track partition.",

	"Disk Driver: An alternate block has been assigned to block %d.",

	"Disk Driver: Data cannot be written to reassigned block. The block\n\
will be remapped to the alternate sector/track partition.",

	"Disk Driver: A bad block (block %d) has been detected in a critical\n\
area of the hard disk.  The disk drive is not useable in this state.\n\
The drive may need a low level format or repair.",

	"Disk Driver: An alternate block has been assigned to block %d.",

	"!Disk Driver: Data from block %d is not readable.\nData of this block \
has been lost. Trying to reassign an alternate block.",

	"!Disk Driver: The drive is out of spare blocks for surrogates.\n\
Block %d will be remapped to alternate sector/track partition.",

	"!Disk Driver: The controller can not reassign a surrogate block.\n\
Block %d will be remapped to alternate sector/track partition.",

	"!Disk Driver: Block %d isn't writable. Failed to initialize block.",

	"Disk Driver: Block %d remapped to an alternate.",

};

/*===========================================================================*/
/* Debugging mechanism that is to be compiled only if -DDEBUG is included
/* on the compile line for the driver                                  
/* DPR provides levels 0 - 9 of debug information.                  
/*      0: No debugging.
/*      1: Entry points of major routines.
/*      2: Exit points of some major routines. 
/*      3: Variable values within major routines.
/*      4: Variable values within interrupt routine (inte, intn).
/*      5: Variable values within sd01logerr routine.
/*      6: Bad Block Handling
/*      7: Multiple Major numbers
/*      8: - 9: Not used
/*============================================================================*/

#define O_DEBUG         0100

/*
 * void
 * sd01_bbhndlr(struct disk *dk, struct job *jp, int cond, char *datap, 
 *	daddr_t hdesec)
 * 
 * Bad block handler - entered when an unrecoverable error requires
 * either hardware reassignment or software remapping.  The following
 * cases are handled:
 *	1) SD01_ECCRDWR - ECC occurred on read/write.
 *		If write, do a reassign/remap, then retry original job.
 *		If read, don't do reassign/remap, and just put interim
 *		entry in alternate table.
 *	2) SD01_ECCDWR - ECC occurred on previous read, now doing write.
 *		do a reassign/remap, then do original job.
 *	3) SD01_ECCDRD - ECC occurred on previous read, now read succeeded.
 *		now do the reassign/remap and restore data to the
 *		alternate sector.
 *	4) SD01_REENTER - done processing a bad block, check queue for
 *		possible job queued while we processed the last one.
 *
 * This routine synchronizes the entry to bad block handling, and if
 * another bad block is being processed on the SAME disk, this job is
 * queued on dk->bbhque.
 * When this routine returns from cases 1-3, the bad block processing
 * has been initiated, but not necessasarily completed, so the caller
 * should also just return, and expect everything will happen eventually.
 * The first step of the bad block processing is hardware reassign, and
 * if the hardware reassign fails, then the sd01_bbhdwrea() routine takes
 * the job to the next step, sd01_bbremap(), software remapping.
 *
 * When this routine is entered with SD01_REENTER, the job is finished,
 * and the next job on the queue is processed, or, if none, DKMAPBAD is
 * cleared and the queue is resumed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *	Sets DKMAPBAD state, causing single thread of bad block code.
 *		(dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
void
sd01_bbhndlr(struct disk *dk, struct job *jp, int cond, char *datap, daddr_t hdesec)
{
	struct sd01_bbhque *bbhp;

	SD01_DK_LOCK(dk);
	if(dk->dk_state & DKMAPBAD)	{
		if (cond == SD01_REENTER) {
			dk->hde_state = 0;  /* This resets HDESWMAP and state */

			if ((bbhp = dk->bbhque) == NULL) {
				/* ALL DONE! put disk back in business */
				if (dk->dk_state & DKSUSP)
					sd01qresume(dk);
				dk->dk_state &= ~DKMAPBAD;
				SD01_DK_UNLOCK(dk);
				SV_BROADCAST(dk->dk_sv, 0);
				return;
			} else {
				/* NOT DONE!  Got another one. */
				jp = bbhp->bbh_jp;
				cond = bbhp->bbh_cond;
				datap = bbhp->bbh_datap;
				hdesec = bbhp->bbh_hdesec;
				dk->bbhque = bbhp->bbh_next;
				sdi_free(&sm_poolhead, (jpool_t *)bbhp);
			}
		} else {
			/* Disk is already processing a bad block, 
			 * so just queue it since alternate table
		 	 * access requires single threading (as does
		 	 * use of preallocated data structures such as 
		 	 * dk_bbh_rbuf, dk_bbh_wbuf etc.).
		 	 */
			bbhp = (struct sd01_bbhque *)sdi_get(&sm_poolhead, 
				KM_NOSLEEP);
			if(bbhp == NULL) {
				jp->j_bp->b_error =  EIO;
				jp->j_bp->b_flags |= B_ERROR;
				SD01_DK_UNLOCK(dk);
				return;
			}
			bbhp->bbh_jp = jp;
			bbhp->bbh_cond = cond;
			bbhp->bbh_datap = datap;
			bbhp->bbh_hdesec = hdesec;
			bbhp->bbh_next = dk->bbhque;
			dk->bbhque = bbhp;
			SD01_DK_UNLOCK(dk);
			return;
		}
	}
	dk->dk_state |= DKMAPBAD;
	SD01_DK_UNLOCK(dk);
	dk->dk_jp_orig = jp;	/* Save original job pointer for retry, etc. */
	dk->dk_datap = datap;	/* Indicates whether data must be recovered  */
	dk->dk_hdesec = hdesec;	/* Block that is bad.*/
	
	/* Now running single threaded within DKMAPBAD.  dk_lock is not
	 * needed to protect dk->hde_state, dk->dk_jp_orig, dk->dk_datap,
	 * or dk->dk_hdesec since they is used solely within the confines
	 * of the DKMAPBAD thread.
	 */

	if (jp->j_flags & J_HDEECC)
		dk->hde_state |= HDEECCERR;	/* Unrecoverable error	*/
	if (jp->j_flags & J_HDEREC)
		dk->hde_state |= HDERECERR;	/* Marginal error	*/
	jp->j_flags &= ~J_HDEBADBLK;

	ASSERT ((cond == SD01_ECCRDWR) || 
		(cond == SD01_ECCDWR) || 
		(cond == SD01_ECCDRD));

	switch (cond) {

	case SD01_ECCRDWR:	/* ECC on read/write, no previous ECC */
		if(jp->j_bp->b_flags & B_READ) {
			dk->hde_state &= HDEHWMASK;
			jp->j_flags |= J_DATINVAL; /* Indicates data invalid */
			sd01_bbremap(dk, SD01_ADDINTRM, NULL);
			break;
		}
		/* Write case handled same as delayed write */
		/* FALLTHRU */

	case SD01_ECCDWR:	/* ECC on previous read, delayed write */
		/* Indicate remap on failure and retry after remap */
		jp->j_flags |= (J_ROF | J_RETRY);
		dk->hde_state |= HDEHWREA;
		sd01_bbhdwrea(jp);
		break;

	case SD01_ECCDRD:	/* ECC on previous read, delayed read success */
		/* Indicate remap on failure (no retry after remap) */
		jp->j_flags |= J_ROF;
		dk->hde_state |= HDEHWREA;
		sd01_bbhdwrea(jp);
		break;
	}

	return;
}

/*
 * void
 * sd01_bbhdwrea(struct job *jp) - Bad block hardware reassign
 *
 * This routine is entered successively, with hde_state indicating
 * how far we have gotten.  The initial call is from the bad block
 * handler, sd01_bbhndlr(), (or sd01_addbad()) when a block must
 * be hardware reassigned or software remapped.
 * Each successive call is made by sd01intb(), the
 * interrupt handler catching the completion of the immediate commands
 * being executed in each step.
 * In the final call, either the hardware reassign was successful, 
 * the data is optionally restored. The J_ROF flag in the jp will
 * be set if the software remap must begin immediately
 * with a call to sd01_bbremap().
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
void
sd01_bbhdwrea(struct job *jp)
{
	struct disk *dk = jp->j_dk;	/* Pointer to disk structure	*/
	daddr_t	blkaddr;		/* Block address of error 	*/
	int part;			/* Partition number		*/
	major_t	 maj;			/* Device major number		*/
	minor_t	 min;			/* Device minor number		*/
	int	 critical,		/* critical flag		*/
		 ptag,			/* Partition tag		*/
		 resflag,		/* Reassign bad block flag	*/
		 i;			/* Loop index			*/

	maj = dk->dk_addr.sa_major;
	min = dk->dk_addr.sa_minor; 

	part    = DKSLICE(min); 
	ptag    = dk->dk_vtoc.v_part[part].p_tag;
	blkaddr = dk->dk_hdesec;

#ifdef SD01_DEBUG
	if(sd01alts_debug & HWREA) {
		cmn_err(CE_CONT, "sd01_hdwrea: blkaddr= 0x%x\n", blkaddr);
	}
#endif

#ifdef DEBUG
        DPR (1)("sd01_hdwrea: (blkaddr=0x%x)\n", blkaddr);
        DPR (6)("sd01_hdwrea: (blkaddr=0x%x) state=0x%x\n",blkaddr,dk->hde_state);
        DPR (6)("part=0x%x p_tag=0x%x\n",part, ptag);
#endif

	/* Clear reassign and critical flags */
	resflag = 0;
	critical  = 0;

	/* Check if bad block is in critical area of UNIX System partition. */
	if(blkaddr >= dk->unixst && blkaddr <= (dk->unixst + HDPDLOC))
		critical = 1;

	/* Determine the step for proceeding */
	if (!(dk->dk_iotype & F_HDWREA)) {
		/* NOTE: Skip hardware reassign step */
		dk->hde_state ++;
	}

	switch(dk->hde_state & HDESMASK) {
	case HDESINIT:	/* Start by sending a Reassign command */

#ifdef DEBUG
	if((dk->hde_state & HDEBADBLK) == HDERECERR)
        	DPR (6)("marginal bad block 0x%x\n", blkaddr);
	else
        	DPR (6)("actual bad block 0x%x\n", blkaddr);
#endif

		if(critical)
			if((dk->hde_state & HDEBADBLK) == HDERECERR)
				/*
				 *+ Disk Driver: A potential bad block has been
				 *+ detected in a critical area of the hard
				 *+ disk.  If this block goes bad, the UNIX
				 *+ system might fail during the next reboot.
				 *+ Please backup your system now.
				 */
				cmn_err(CE_WARN, hde_mesg[HDESACRED], blkaddr);
			else
				/*
				 *+ Disk Driver: A bad block has been
				 *+ detected in a critical area of the hard
				 *+ disk.  The disk drive is not useable in
				 *+ this state.  The drive may need a low
				 *+ level format or repair.
				 */
				cmn_err(CE_NOTE, hde_mesg[HDEBSACRD], blkaddr);

		else if(Sd01log_marg == 1)
			/*
			 *+ Disk Driver: Soft read error corrected by ECC
			 *+ algorithm: (block).
			 */
			cmn_err(CE_NOTE, hde_mesg[HDEECCMSG], blkaddr);

		/* Set up defect list header */
		dk->dk_dl_data[0] = 0;
		dk->dk_dl_data[1] = 0;
		dk->dk_dl_data[2] = 0;
		dk->dk_dl_data[3] = 4;

		/* Swap defect address */
		dk->dk_dl_data[4] = ((blkaddr & (unsigned) 0xFF000000) >> 24);
		dk->dk_dl_data[5] = ((blkaddr & 0x00FF0000) >> 16);
		dk->dk_dl_data[6] = ((blkaddr & 0x0000FF00) >> 8);
		dk->dk_dl_data[7] = ( blkaddr & 0x000000FF);

		/* Send REASSIGN BLOCKS to map out the bad sector */
		sd01icmd(dk,SS_REASGN,0,dk->dk_dl_data,RABLKSSZ,0,SCB_WRITE,sd01intb);

		return;

	case HDESI:	/* Reassign failed */
		if(dk->hde_state & HDEENOSPR) {
			if((dk->hde_state & HDEBADBLK) == HDERECERR)
				/*
				 *+ Disk Driver: A potential bad block has been
				 *+ detected (block).  The drive is out of spare
				 *+ sectors.  The block will be software 
				 *+ remapped to the alternate sector partition.
				 */
				cmn_err(CE_WARN, hde_mesg[HDENOSPAR], blkaddr);
			else
				/*
				 *+ Disk Driver: A bad block has been
				 *+ detected (block).  The drive is out of spare
				 *+ sectors.  The block will be software 
				 *+ remapped to the alternate sector partition.
				 */
				cmn_err(CE_WARN, hde_mesg[HDEBNOSPR], blkaddr);
		} else {
			if((dk->hde_state & HDEBADBLK) == HDERECERR)
				/*
				 *+ Disk Driver: A potential bad block has been
				 *+ detected (block).  The controller cannot
				 *+ reassign a sector.  The block will be
				 *+ software remapped to the alternate sector 
				 *+ partition.
				 */
				cmn_err(CE_WARN, hde_mesg[HDEBADMAP], blkaddr);
		}
		dk->hde_state |= HDEREAERR;
		break;

	case HDESII:	/* Reassign passed */
		resflag = 1;

		jp = dk->dk_jp_orig;
		if (jp->j_bp->b_flags & B_READ)
			if((dk->hde_state & HDEBADBLK) == HDERECERR)
				/*
				 *+ Disk Driver: An alternate block as been
				 *+ reassigned.
				 */
				cmn_err(CE_NOTE, hde_mesg[HDEREASGN], blkaddr);
			else
				/*
				 *+ Disk Driver: An alternate block as been
				 *+ reassigned.
				 */
				cmn_err(CE_WARN, hde_mesg[HDEREASGN], blkaddr);

		/* Send WRITE command to restore data to sector
		 * if there is data. */
		if(dk->dk_datap) {

			sd01icmd(dk,SM_WRITE,blkaddr,dk->dk_datap,
				BLKSZ(dk),1,SCB_WRITE,sd01intb);

			return;
		}
		/* Otherwise we are done with hardware reassignment */
		break;

	case HDESIII:	/* Write failed */
		/* handle this case as though the reassign was 
		 * unsuccessful, and defer to software remap */
		resflag = 0;
		/*
		 *+ Disk Driver: Data cannot be written to reassigned
		 *+ block.  The block will be software remapped to 
		 *+ to the alternate sector partition.
		 */
		cmn_err(CE_WARN, hde_mesg[HDEBADWRT], blkaddr);
		break;

	case HDESIV:	/* Write passed */
		resflag = 1;
		break;

	default:
		break;

	}

	/* Clear hardware bad block reassign state */
	dk->hde_state &= HDEHWMASK;
	dk->hde_state &= ~HDEHWREA;
	SV_BROADCAST(dk->dk_sv, 0);
	jp = dk->dk_jp_orig;

	if(resflag) {
		/* Hardware reassign was successful */
		/* Check if block was in a valid file system. */
		if (dk->dk_vtoc.v_part[part].p_flag & V_VALID && ptag == V_ROOT
			|| ptag == V_USR) {
			/* Mark the file system dirty */
			fshadbad(makedevice(maj,min), blkaddr - dk->dk_vtoc.v_part[part].p_start);
		}
		/* Remove interim entry from alternate table
		 * and retry the job, if necessary */
		dk->dk_datap = NULL;  /* Data was already restored, as necessary */
		sd01_bbremap(dk, SD01_RMINTRM, NULL);
	} else {
		/* Hardware reassign was unsuccessful
		 * so now initiate the software remap */
		if (jp->j_flags & J_ROF) {
			jp->j_flags &= ~J_ROF;
			sd01_bbremap(dk, SD01_REMAP, NULL);
		}
	}

#ifdef DEBUG
        DPR (2)("sd01_bbhdwrea: - exit\n");
#endif
}

/*
 * void
 * sd01_bbremap(struct disk *dk, int cond, int status)
 *	bad block software remapping. 
 *
 * This routine handles the following cases:
 *	1) SD01_REMAP -  do a software remap since sd01_bbhdwrea()
 *		failed to hardware reassign a bad block.
 *	2) SD01_RMINTRM - remove the interim alternate entry from the
 *		disk and core alternate table, since sd01_bbhdwrea()
 *		was successful in hardware reassigning the bad block.
 *	3) SD01_ADDINTRM - add the interim alternate entry to the 
 *		disk and core alternate table, since a read with ECC
 *		just occurred and data is now invalid.
 *	4) SD01_ADDBAD - add the growing bad sectors from sd01addbad.
 *	5) SD01_REMAPEND - entered when either of the above 4 cases
 *		has completed, so end up.
 * In the final call, SD01_REMAPEND, if the previous processing was successful,
 * the data is either optionally restored or the job is optionally retried.
 * status of non-zero indicates an error condition, and the bp is returned 
 * with errors.  Then the bad block handler is reentered to complete the loop.
 *
 * This routine is guaranteed to be single threaded for each disk by the
 * controlled flow through sd01_bbhndlr().
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
void
sd01_bbremap(struct disk *dk, int cond, int status)
{
	struct 	job *jp;
	buf_t	*bp;
	struct alts_ent *altp;
	int flags;

	switch (cond) {

	case SD01_RMINTRM:	/* remove alternate entry, if it exists 
				 * and update the disk alts table */
		if ((altp = sd01_findaltent(dk)) == (struct alts_ent *)NULL
            	     || (altp->bad_start != altp->good_start)) {
			/* Retry job if necessary, and end off */
			/* (data may have already been restored) */

			jp = dk->dk_jp_orig;
			sd01_bbdone(jp);
			sd01_bbhndlr(dk, NULL, SD01_REENTER, NULL, NULL);
			return;
		}
		/* Set the in-core interim alternate entry to EMPTY
		 * to get it cleared out, and dummy up the new entry
		 * just to get the table compressed, and written out
		 */
		altp->bad_start = ALTS_ENT_EMPTY;
		dk->dk_hdesec = ALTS_ENT_EMPTY;
		/* FALLTHRU */

	case SD01_ADDINTRM:	/* Add interim alternate entry that 
				 * indicates invalid data in sector */
	case SD01_REMAP:	/* Start by initiating a remap command */

		if (dk->dk_amp == 0) {
			/*
			 *+ Disk was not generated with spare sectors for
			 *+ software remapping.  No bad block remapping
			 *+ can be done for this bad sector.
			 */
			cmn_err(CE_WARN, "No alternate sectors have been reserved on this disk\n");
			cmn_err(CE_CONT, "Software remapping is not possible.\n");
			jp = dk->dk_jp_orig;
			bp = jp->j_bp;
			bp->b_error =  EIO;
			bp->b_flags |= B_ERROR;
			flags = jp->j_flags;
			if (sd01releasejob(jp) && !(flags & J_FREEONLY)) {
				biodone(bp);
				if (flags & J_TIMEOUT)
					jp->j_flags |= J_FREEONLY;
			}
			sd01_bbhndlr(dk, NULL, SD01_REENTER, NULL, NULL);
			break;
		}
		/* set growing bad sector info	*/
		dk->dk_gbadsec.bad_start = dk->dk_hdesec;
		dk->dk_gbadsec.bad_end   = dk->dk_hdesec;
		dk->dk_gbadsec_cnt = 1;
		dk->dk_gbadsec_p = &(dk->dk_gbadsec);
		if (cond == SD01_ADDINTRM)
			dk->dk_gbadsec.good_start= dk->dk_gbadsec.bad_start;
		else
			dk->dk_gbadsec.good_start= 0;
		/* FALLTHRU to common remap code */

	case SD01_ADDBAD:
		/* Now in SW remap state */
		dk->hde_state &= ~HDEREAERR;
		dk->hde_state |= HDESWMAP;	

		if (dk->dk_remapalts == NULL)
			status = 1; /* Fake failed call */
		else
			status = (*dk->dk_remapalts)(dk);

		/* If good status, return. We have initiated the first
		 * WRITE and must wait for completion.  We will
		 * re-enter in sd01intalts,  and eventually
		 * come back to sd01_bbremap to end up.
		 * If bad return, fall thru to REMAPEND.
		 */
		if (!status)
			return;
		/* FALLTHRU */

	case SD01_REMAPEND:	/* Finish REMAP or ADDINTRM/RMINTRM */
		/* Check completion status */
		jp = dk->dk_jp_orig;
		if (status || (jp->j_flags & J_DATINVAL)) {
			/* NO GOOD! or must report an error because data inval.
			 * get rid of original job and return up an error */
			/* XXX make sure that alternate table is as before
			   i.e. that the interim is still there */
			bp = jp->j_bp;
			bp->b_flags |= B_ERROR;
			bp->b_error =  EIO;
			flags = jp->j_flags;
			if (sd01releasejob(jp) && !(flags & J_FREEONLY)) {
				biodone(bp);
				if (flags & J_TIMEOUT)
					jp->j_flags |= J_FREEONLY;
			}
		} else {
			/*
			 *+ SUCCESSFUL REMAP!!!
			 */
			cmn_err(CE_NOTE, hde_mesg[HDESWALTS], dk->dk_hdesec);

			/* Retry job if necessary, and end off */
			/* (data may have already been restored) */

			sd01_bbdone(jp);
		}
		sd01_bbhndlr(dk, NULL, SD01_REENTER, NULL, NULL);
		break;

	} 
#ifdef SD01_DEBUG
	if(sd01alts_debug & DKD) {
		/*
		 *+
		 */
		cmn_err(CE_NOTE, "sd01_bbremap: return\n");
	}
#endif
	return;
}

/*
 * void
 * sd01_bbdone(struct job *jp)
 *	Done routine for bad block processing.
 *	Resets error flags, optionally retries the job, and removes original
 *	job from the job chain
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
void
sd01_bbdone(struct job *jp)
{
	buf_t *bp = jp->j_bp;
	struct 	disk *dk = jp->j_dk;
	struct job *njp = NULL;
	struct job *ojp;
	int part;
	int jflags;
	int ret = 0;
	int release;

	jflags = jp->j_flags;

	part = DKSLICE(geteminor(bp->b_edev));
	if ((jp->j_fwmate) || (jp->j_bkmate)) {
		/* Find the end of chain... move forward until the end.
		 * If already there, back up.
		 */
		njp = jp;
		while (njp->j_fwmate != NULL) {
			njp = njp->j_fwmate;
		}
		if (jp == njp)
			njp = jp->j_bkmate;

	}

	release = sd01releasejob(jp);

	if(jflags & J_RETRY) {

		/* Remaining pieces of the old job are linked to the
		 * newly created job when njp is not NULL.  Mark the
		 * pieces of the old job as J_OREM, so that they can
		 * be ignored in completion.
		 */
		ojp = njp;
		while (ojp) {
			ojp->j_flags |= J_OREM;
			ojp = ojp->j_bkmate;
		}

		/* Requeue the job */
		ret = sd01gen_sjq(dk, part, bp, SD01_IMMED|SD01_FIFO, njp);
	}
	if (ret) {
		bp->b_error =  EIO;
		bp->b_flags |= B_ERROR;
	} else {
		bp->b_error = 0;
		bp->b_flags &= ~B_ERROR;
	}
 	if (release && !(jflags & (J_RETRY|J_FREEONLY))) {
		biodone(bp);
		if (jflags & J_TIMEOUT)
			jp->j_flags |= J_FREEONLY;
	}
}

/*
 * int
 * sd01icmd(struct disk *dk, char op_code, uint addr, char *buffer,
 * uint size, uint length, ushort mode, void (*intfunc)())
 *	This funtion performs a SCSI command such as Reassign Blocks for
 *	the drivers bad block handling routine. The op code determines 
 *	the SCB for the job. The data area is supplied by the caller and 
 *	assumed to be in kernel space. 
 * Called by: sd01hdelog
 * Side Effects: 
 *	A immediate command is sent to the host adapter. It is NOT
 *	sent via the drivers job queue.
 * Errors:
 *	The host adapter rejected a request from the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01icmd(struct disk *dk, char op_code, uint addr, char *buffer,
	uint size, uint length, ushort mode, void (*intfunc)())
{
	/*
	 * Args are: disk pointer, command opcode, address field of command,
	 *	buffer for CDB data, size of buffer, block length in CDB,
	 *	direction of transfer, interrupt handler,
	 */
	register struct job *jp;
	register struct scb *scb;
	register buf_t  *bp;
	int	 mystatus = 1;

#ifdef DEBUG
        DPR (1)("sd01icmd: (dk=0x%x)\n", dk);
        DPR (6)("sd01icmd: (dk=0x%x)\n", dk);
#endif

	/* Set up SB, jp, and bp pointers */
	jp = dk->dk_bbhjob;
	bp = dk->dk_bbhbuf;

	if (op_code == SS_REASGN)
		jp->j_cont = dk->dk_bbhmblk;
	else
		jp->j_cont = dk->dk_bbhblk;

	bioreset(bp);
	bp->b_iodone = NULL;

	jp->j_cont->sb_type = ISCB_TYPE;
	
	/* Set up buffer header */
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_un.b_addr = (caddr_t) buffer;	/* not used in sd01intb */
	bp->b_bcount = size;
	bp->b_error  = 0;
	
	/* Set up job structure */
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = 0;				/* not used in sd01intb */
	
	/* Set up SCB pointer */
	scb = &jp->j_cont->SCB;

	if (op_code & 0x20) { /* Group 1 commands */
		register struct scm *cmd;

		cmd = &jp->j_cmd.cm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = dk->dk_addr.sa_lun;
		cmd->sm_res1 = 0;
		cmd->sm_addr = addr;
		cmd->sm_res2 = 0;
		cmd->sm_len  = length;
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
	else { /* Group 0 commands */
		register struct scs *cmd;

		cmd = &jp->j_cmd.cs;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = dk->dk_addr.sa_lun;
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = (addr & 0xFFFF);
		cmd->ss_len   = (unsigned char) length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
	
	/* Swap bytes in the address and length fields */
	if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
		jp->j_cmd.cs.ss_addr = sdi_swap16(jp->j_cmd.cs.ss_addr);
	else {
		jp->j_cmd.cm.sm_addr = sdi_swap32(jp->j_cmd.cm.sm_addr);
		jp->j_cmd.cm.sm_len  = (short) sdi_swap16(jp->j_cmd.cm.sm_len);
	}
	
	/* Initialize SCB */
	scb->sc_int    = intfunc;
	scb->sc_dev    = dk->dk_addr;
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode   = mode;
	scb->sc_resid  = 0;
	scb->sc_time   = JTIME;
	scb->sc_wd     = (long) jp;

	if ((jp->j_cont == dk->dk_bbhblk) &&
	    (sdi_translate(jp->j_cont, bp->b_flags, NULL, KM_NOSLEEP) !=
	     SDI_RET_OK)) {
#ifdef DEBUG
		cmn_err(CE_CONT,"Disk Driver: xlat failed for read/write icmd");
#endif
		if ((*intfunc) == sd01intb) {
			/* Fail the job */
			dk->hde_state += 1;
			sd01_bbhdwrea(jp);
		}
		return (mystatus);
	}
	sd01timeout(jp);
	/* Send the job */
	if (sdi_icmd(jp->j_cont, KM_NOSLEEP) != SDI_RET_OK) {
#ifdef DEBUG
		cmn_err(CE_CONT, "Disk Driver: Bad icmd to HA");
#endif
		sd01untimeout(jp, SDI_ERROR);
		if ((*intfunc) == sd01intb) {
			/* Fail the job */
			dk->hde_state += 1;
			sd01_bbhdwrea(jp);
		}
		return (mystatus);
	}

#ifdef DEBUG
        DPR (2)("sd01icmd: - exit(%d)\n", jp->j_bp->b_error);
        DPR (6)("sd01icmd: - exit(%d)\n", jp->j_bp->b_error);
#endif
	return(0);
}

/*
 * void
 * sd01logerr(struct sb *sbp, struct disk *dk, int err_code)
 *	This function will log and print the error messages for errors
 *	detected by the host adapter driver.  No message will be printed
 *	for thoses errors that the host adapter driver has already 
 *	detected.  Other errors such as write protection are
 *	not reported.  The job argument maybe NULL.
 * Called by: sd01comp1, sd01intf, sd01intn, sd01ints, sd01intrq, sd01intres
 * Side Effects: An error report is generated.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
void
sd01logerr(struct sb *sbp, struct disk *dk, int err_code)
{
#ifdef DEBUG
        DPR (1)("sd01logerr: (sbp=0x%x dk=0x%x err_code=0x%x)\n", sbp, dk, err_code);
#endif
	
	/* If OOS, then don't complain */
	if (sbp->sb_type == SFB_TYPE && sbp->SFB.sf_comp_code != SDI_OOS)
	{
#ifdef DEBUG
		DPR (4)("SD01: SFB failed. SB addr = %x\n", sbp);
		DPR (4)("Completion code = %x\n", sbp->SFB.sf_comp_code);
		DPR (4)("Interrupt function address = %x\n", sbp->SFB.sf_int);
		DPR (4)("Major number = %d\n", sbp->SFB.sf_dev.sa_major);
		DPR (4)("Minor number = %d\n", sbp->SFB.sf_dev.sa_minor);
		DPR (4)("Logical unit = %d\n", sbp->SFB.sf_dev.sa_lun);
		DPR (4)("Function code = %x\n", sbp->SFB.sf_func);
		DPR (4)("Word = %x\n", sbp->SFB.sf_wd);
#endif
		sdi_errmsg("Disk",&sbp->SFB.sf_dev,sbp,&dk->dk_sense,SDI_SFB_ERR,err_code);
		return;
	}

	
#ifdef DEBUG
	DPR(5)("Disk Driver: SCB failed. SB addr = %x, Disk addr = %x\n", sbp, dk);
	DPR(5)("Completion code = %x\n", sbp->SCB.sc_comp_code);
	DPR(5)("Device address = %d, %d, %d\n", sbp->SCB.sc_dev.sa_major,
		sbp->SCB.sc_dev.sa_minor, sbp->SCB.sc_dev.sa_lun);
	DPR(5)("Command Addr = %x, Size = %x\n",
		sbp->SCB.sc_cmdpt, sbp->SCB.sc_cmdsz);
	DPR(5)("Data = %x, Size = %x\n",
		sbp->SCB.sc_datapt, sbp->SCB.sc_datasz);
	DPR(5)("Word = %x\n", sbp->SCB.sc_wd);

	DPR(5)("\nStatus = %x\n", sbp->SCB.sc_status);
	DPR(5)("Interrupt function address = %x\n", sbp->SCB.sc_int);
	DPR(5)("Residue = %d\n", sbp->SCB.sc_resid);
	DPR(5)("Time = %d\n", sbp->SCB.sc_time);
	DPR(5)("Mode = %x\n", sbp->SCB.sc_mode);
	DPR(5)("Link = %x\n", sbp->SCB.sc_link);
#endif

	/* Ignore the error if it is unequipped or out of service. */
	if (sbp->SCB.sc_comp_code == SDI_OOS ||
		sbp->SCB.sc_comp_code == SDI_NOTEQ)
	{
		return;
	}

	/* Ignore the error if we are doing a test unit ready	*/
	/* test unix ready fails if the LU is not equipped	*/
	if ((char) *sbp->SCB.sc_cmdpt == SS_TEST)
	{
		return;
	}

	if (sbp->SCB.sc_comp_code == SDI_CKSTAT ) {

		/* Don't report RESERVATION Conflicts   */
		/* User will know them from errno value */
		if (sbp->SCB.sc_status == S_RESER)
		{
			return;
		}
	
		/* Now check for a Check Status error */

		if (sbp->SCB.sc_status == S_CKCON &&
		(char) *sbp->SCB.sc_cmdpt != SS_REQSEN)
		{

			sdi_errmsg("Disk",
				&sbp->SCB.sc_dev,sbp,&dk->dk_sense,
				SDI_CKCON_ERR, err_code);

			dk->dk_sense.sd_key = 0;
			dk->dk_sense.sd_sencode = 0;
			return;
		}
		sdi_errmsg("Disk",&sbp->SCB.sc_dev,sbp,&dk->dk_sense,
			SDI_CKSTAT_ERR,err_code);
		return;
	}

	
	sdi_errmsg("Disk",&sbp->SCB.sc_dev,sbp,&dk->dk_sense,SDI_DEFAULT_ERR,err_code);


#ifdef DEBUG
        DPR (2)("sd01logerr: - exit\n");
#endif
}

/*
 * void
 * sd01flt(struct disk *dk, long flag)
 *	This function is called by the host adapter driver when either
 *	a Bus Reset has occurred or a device has been closed after
 *	using Pass-Thru. This function begins a series of steps that
 *	ensure that the device is RESERVED before trying further jobs.
 *	If this function is called due to Pass-Thru, then the PD Sector
 *	and VTOC should be updated. This function cannot set the
 *	job pointer since you may be overwriting a valid job pointer.
 * Called by:  Host Adapter Driver
 * Side Effects:
 *	If due to Pass-Thru, PD sector and VTOC will be updated.
 * Errors:
 *	The HAD called this function due to a fault condition on the
 *	SCSI bus but passed an unknown parameter to this function.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01flt(struct disk *dk, long flag)
{

#ifdef DEBUG
        DPR (1)("sd01flt: (dk=0x%x flag=%d)\n", dk, flag);
#endif

	switch (flag)
	{
	  case	SDI_FLT_RESET:		/* LU was reset */
		break;

	  case	SDI_FLT_PTHRU:		/* Pass-Thru was used */
		SD01_DK_LOCK(dk);
		/*
		*  If the device had previously been RESERVED,
		*  then try to re-RESERVE it.
		*/
		if (dk->dk_state & DKRESDEVICE)
		{
			SD01_DK_UNLOCK(dk);
			/*
			*  If the RESERVE fails, then the gauntlet will
			*  have been entered and the appropriate message
			*  will have been printed.
			*/
			if (sd01cmd(dk, SS_RESERV, 0,(char *) NULL, 0, 0, SCB_WRITE) == 0)
			{
				SD01_DK_LOCK(dk);
				dk->dk_state |= DKRESERVE;
				SD01_DK_UNLOCK(dk);
			}
		}
		else	{
			SD01_DK_UNLOCK(dk);
		}

		return;

	  default:			/* Unknown type */
#ifdef DEBUG
		cmn_err(CE_CONT, "Disk Driver: Bad type from HA");
#endif
		return;
	}

	/* Go on to next step */
	sd01flts(dk, NULL);

#ifdef DEBUG
        DPR (2)("sd01flt: - exit\n");
#endif
}

/*
 * void
 * sd01flts(struct disk *dk, struct job *jp)
 *	This is the beginning of the 'gauntlet'. All faults must
 *	come thru this function in case something caused the device
 *	to become released. Before any corrective action can be taken,
 *	the device must be reserved again. A suspend SFB is sent to HAD
 *	to stop any further jobs to the device. The interrupt routine
 *	will handle the next step of the gauntlet.
 * Called by: sd01flt, sd01intn
 * Side Effects:
 *	A suspend SFB is issued for this device.
 * Errors:
 *	The HAD rejected a Function request to Suspend the LU queue
 *	for the current device. The disk driver cannot proceed with the
 *	handling of the fault so the original I/O request will be failed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *	Sets DKFLT state (i.e. the fault detection single thread state.)
 */
void
sd01flts(struct disk *dk, struct job *jp)
{
#ifdef DEBUG
        DPR (1)("sd01flts: (dk=0x%x)\n", dk);
#endif

	SD01_DK_LOCK(dk);

	/* If disk is already in the gauntlet, put job in fault queue */
	if (dk->dk_state & DKFLT) {
		if (jp != NULL) {
			/*
			 * Put on fault queue for retry on exit to gauntlet.
			 */
			jp->j_next = dk->dk_fltque;
			dk->dk_fltque = jp;
		}
		SD01_DK_UNLOCK(dk);
		return;
	}
	dk->dk_state |= DKFLT;
	SD01_DK_UNLOCK(dk);
	dk->dk_fltres->SCB.sc_wd = (long)jp;
	/* Initialize the RESERVE Reset counter for later in the gauntlet */
	dk->dk_rescnt = 0;

	dk->dk_spcount = 0;
	dk->dk_fltsus->sb_type = SFB_TYPE;
	dk->dk_fltsus->SFB.sf_int = sd01ints;
	dk->dk_fltsus->SFB.sf_func = SFB_SUSPEND;
	dk->dk_fltsus->SFB.sf_dev = dk->dk_addr;

	if (sdi_icmd(dk->dk_fltsus, KM_NOSLEEP) != SDI_RET_OK)
	{
#ifdef DEBUG
		cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
		/* Clean up after the job */
		sd01flterr(dk, 0);
	}

#ifdef DEBUG
        DPR (2)("sd01flts: - exit\n");
#endif
}

/*
 * void
 * sd01ints(struct sb *sbp)
 *	This function is called by the Host Adapter Driver when the
 *	SUSPEND function has completed. If it failed, retry the job until
 *	the retry limit is exceeded. Once the disk queue is suspended,
 *	send a Request Sense job to find out what happened to the device.
 * Called by:  Host Adapter Driver
 * Side Effects:  Disk queue SUSPEND flag will be set. A Request Sense
 *	job will have been started.
 * Errors:
 *	The SCSI disk driver retried a Function request. The retry
 *	was performed because the HAD detected an error.
 *
 *	The HAD detected an error with the SUSPEND function request
 *	and the retry count has already been exceeded.
 *
 *	The HAD rejected a Request Sense job from the SCSI disk
 *	driver. The original job will also be failed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *	In DKFLT state (i.e. the fault detection single thread state.)
 */
void
sd01ints(struct sb *sbp)
{
	register struct disk *dk;
	dev_t	dev;			/* External device number 	*/
	struct job *jp;
	struct sb *osbp;
	struct sense *sensep;

	dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
	dk  = DKPTR(geteminor(dev));

#ifdef DEBUG
        DPR (1)("sd01ints: (sbp=0x%x) dk=0x%x\n", sbp, dk);
#endif

	if (sbp->SFB.sf_comp_code & SDI_RETRY && dk->dk_spcount < SD01_RETRYCNT)
	{
		/* Retry the Suspend SFB */
		dk->dk_spcount++;
		dk->dk_error++;

		sd01logerr(sbp, dk, 0x4dd25001);

		if (sdi_icmd(sbp, KM_NOSLEEP) != SDI_RET_OK)
		{
#ifdef DEBUG
			cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
			sd01flterr(dk, 0);
		}
		return;
	}

	if (sbp->SFB.sf_comp_code != SDI_ASW)
	{
		sd01logerr(sbp, dk, 0x6dd25002);

		sd01flterr(dk, 0);
		return;
	}

	/*
	 *  The device is now SUSPENDED. Start the Request Sense Job.
	 */
	SD01_DK_LOCK(dk);
	dk->dk_state |= DKSUSP;
	SD01_DK_UNLOCK(dk);

#ifdef SD01_DEBUG
	if (sd01alts_debug & DKD || sd01alts_debug & HWREA) {
		jp = (struct job *)dk->dk_fltres->SCB.sc_wd;
		/*
		 * This is part of the bad block hook to simulate an ECC
		 * error.  The original good completion occurred and the
		 * block was made to look bad in sd01intn.  The purpose
		 * of this intercept is to exit the gaunlet, since we don't
		 * need to do a SS_REQSEN/SS_RESERV.
		 */
		if (dk_badspot == dk && sd01alts_badspot && 
		    (sd01alts_badspot >= jp->j_daddr) &&
		    (sd01alts_badspot<(jp->j_daddr+jp->j_seccnt))) {
			jp->j_flags |= J_HDEECC;
			dk->dk_sense.sd_ba = sd01alts_badspot;
			jp->j_cont->SCB.sc_comp_code = SDI_RETRY;
			sd01alts_badspot = 0;
			sd01flterr(dk, 0);
			return;
		}
	}
#endif
	/*
	 * Check if the original job has sense data.
	 * If not, send SS_REQSEN to get it.
	 */
	jp = (struct job *)dk->dk_fltres->SCB.sc_wd;
	osbp = jp->j_cont;
	sensep = sdi_sense_ptr(osbp);
	if (sensep->sd_key != SD_NOSENSE) {
		bcopy(sensep, &dk->dk_sense, sizeof(struct sense));
		dk->dk_fltreq->SCB.sc_dev = sbp->SFB.sf_dev;
		dk->dk_fltreq->SCB.sc_comp_code = SDI_ASW;
		sd01timeout((struct job *)dk->dk_fltreq->SCB.sc_wd);
		sd01intrq(dk->dk_fltreq);
		return;
	}
	dk->dk_spcount = 0;
	dk->dk_fltreq->sb_type = ISCB_TYPE;
	dk->dk_fltreq->SCB.sc_int = sd01intrq;
	dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
	dk->dk_fltreq->SCB.sc_link = 0;
	dk->dk_fltreq->SCB.sc_resid = 0;
	dk->dk_fltreq->SCB.sc_time = JTIME;
	dk->dk_fltreq->SCB.sc_mode = SCB_READ;
	dk->dk_fltreq->SCB.sc_dev = sbp->SFB.sf_dev;
	dk->dk_fltcmd.ss_op = SS_REQSEN;
	dk->dk_fltcmd.ss_lun = sbp->SFB.sf_dev.sa_lun;
	dk->dk_fltcmd.ss_addr1 = 0;
	dk->dk_fltcmd.ss_addr  = 0;
	dk->dk_fltcmd.ss_len = SENSE_SZ;
	dk->dk_fltcmd.ss_cont = 0;
	dk->dk_sense.sd_key = SD_NOSENSE; /* Clear old sense key */

	sd01timeout((struct job *)dk->dk_fltreq->SCB.sc_wd);
	if (sdi_icmd(dk->dk_fltreq, KM_NOSLEEP) != SDI_RET_OK)
	{

#ifdef DEBUG
		cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
		sd01untimeout((struct job *)dk->dk_fltreq->SCB.sc_wd, SDI_ERROR);
		sd01flterr(dk, 0);
		return;
	}

#ifdef DEBUG
        DPR (2)("sd01ints: - exit\n");
#endif
}

/*
 * void
 * sd01intrq(struct sb *sbp)
 *	This function is called by the host adapter driver when a
 *	Request Sense job completes. This function will not examine the
 *	Sense data because there is still one more step before a normal
 *	I/O job can be restarted. Send the RESERVE command to the device
 *	to prevent some other host from using the device.
 * Called by: Host Adapter Driver
 * Side Effects:
 *	Either the Request Sense will be retried or the RESERVE command
 *	is sent to the device.
 * Errors:
 *	The SCSI disk driver retried a Request Sense job that the
 *	HAD failed.
 *
 *	The HAD rejected a Request Sense job issued by the SCSI disk driver.
 *	The original job will also be failed.
 *
 *	The HAD detected an error in the last Request Sense job issued by the
 *	SCSI disk driver. The retry count has been exceeded so the original
 *	I/O request will also be failed.
 *
 *	The HAD rejected a Reserve job requested by the SCSI disk
 *	driver. The original job will also be failed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKFLT state (i.e. the fault detection single thread state.)
 */
void
sd01intrq(struct sb *sbp)
{
	register struct	disk *dk;
	dev_t	 dev;			/* External device number 	*/

	dev = makedevice(sbp->SCB.sc_dev.sa_major, sbp->SCB.sc_dev.sa_minor);
	dk  = DKPTR(geteminor(dev));

	sd01untimeout((struct job *)sbp->SCB.sc_wd, sbp->SCB.sc_comp_code);

#ifdef DEBUG
        DPR (1)("sd01intrq: (sbp=0x%x)\n", sbp);
        DPR (6)("sd01intrq: (sbp=0x%x)\n", sbp);
#endif

	if (sbp->SCB.sc_comp_code != SDI_CKSTAT &&
	    sbp->SCB.sc_comp_code & SDI_RETRY &&
	    dk->dk_spcount <= SD01_RETRYCNT)
	{
		dk->dk_spcount++;
		dk->dk_error++;
		sbp->SCB.sc_time = JTIME;
		sd01logerr(sbp, dk, 0x4dd26001);

		sd01timeout((struct job *)sbp->SCB.sc_wd);
		if (sdi_icmd(sbp, KM_NOSLEEP) != SDI_RET_OK)
		{
#ifdef DEBUG
			cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
			sd01untimeout((struct job *)sbp->SCB.sc_wd, SDI_ERROR);
			sd01flterr(dk, 0);
		}
		return;
	}

	if (sbp->SCB.sc_comp_code != SDI_ASW)
	{
		dk->dk_error++;
		sd01logerr(sbp, dk, 0x6dd26003);
		sd01flterr(dk, 0);
		return;
	}


	/*
	*  The sc_wd field must have been filled in when the
	*  fault was first detected by either sd01flt or sd01intn.
	*  It indicates if there is a real job associated with this fault!
	*/
	dk->dk_spcount = 0;
	dk->dk_fltres->sb_type = ISCB_TYPE;
	dk->dk_fltres->SCB.sc_int = sd01intres;
	dk->dk_fltres->SCB.sc_cmdsz = SCS_SZ;
	dk->dk_fltres->SCB.sc_datapt = NULL;
	dk->dk_fltres->SCB.sc_datasz = 0;
	dk->dk_fltres->SCB.sc_link = 0;
	dk->dk_fltres->SCB.sc_resid = 0;
	dk->dk_fltres->SCB.sc_time = JTIME;
	dk->dk_fltres->SCB.sc_mode = SCB_WRITE;
	dk->dk_fltres->SCB.sc_dev = sbp->SCB.sc_dev;
	dk->dk_fltcmd.ss_op = SS_RESERV;
	dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
	dk->dk_fltcmd.ss_addr1 = 0;
	dk->dk_fltcmd.ss_addr  = 0;
	dk->dk_fltcmd.ss_len = 0;
	dk->dk_fltcmd.ss_cont = 0;

	/*
	*  If the device is not suppose to be reserved,
	*  then go directly to the function to restart the original job.
	*  Put the check here so that the SB is initialized.
	*  Some of its data will be used even if no RESERVE is issued.
	*/
	if ((dk->dk_state & DKRESDEVICE) == 0)
	{
		sd01fltjob(dk);
		return;
	}

	sd01timeout((struct job *)dk->dk_fltres->SCB.sc_wd);
	if (sdi_icmd(dk->dk_fltres, KM_NOSLEEP) != SDI_RET_OK)
	{

#ifdef DEBUG
		cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
		sd01untimeout((struct job *)dk->dk_fltres->SCB.sc_wd, SDI_ERROR);
		sd01flterr(dk, 0);
	}

#ifdef DEBUG
        DPR (2)("sd01intrq: - exit\n");
#endif
}

/*
 * void
 * sd01intres(struct sb *sbp)
 *	This function is called by the host adapter driver when a RESERVE
 *	job completes. The job will be retried if it failed.
 *	If a RESET or CRESET prevented the RESERVE from completing, then
 *	the gauntlet must be started again. The SUSPEND job need not be redone
 *	since the queue is already suspended so go back to the Request Sense.
 *	If the SUSPEND succeeded, then if there was a real I/O job in progress
 *	the Request Sense data read previously will be examined to determine
 *	what to do.  Clear the disk fault flag that indicates this disk is in
 *	the gauntlet.
 * Called by: Host Adapter Driver
 * Side Effects: Either the RESERVE will be retried, the gauntlet will
 *	be restarted, or an I/O job will be restarted.
 * Errors:
 *	The HAD rejected a Request Sense job issued by the SCSI disk
 *	driver. The original job will also be failed.
 *
 *	The HAD rejected a Reserve job issued by the SCSI disk driver.
 *	The original job will also be failed.
 *
 *	The HAD detected a failure in the last Reserve job issued
 *	by the SCSI disk driver. The retry count has been exceeded
 *	so the original job has been failed.
 *
 *	The SCSI disk driver is retrying an I/O request because of an error
 *	detected by the target controller. The cause of the error is
 *	indicated by the second and third error codes. These error
 *	codes are the sense key and extended sense code respectively.
 *	See the disk target controller code for more information.
 *	(OBSOLETE ERROR)
 *
 *	The disk controller performed retry or ECC which was
 *	successful. The cause of the error is indicated by the second
 *	and third error codes. There error codes are the sense key and
 *	extended sense code respectively. See the disk target controller
 *	codes for more information.  (OBSOLETE ERROR)
 *
 *	The RESERVE command caused the bus to reset and has exceeded
 *	its maximum retry count. The original job will be failed and the
 *	error handling code will be exited.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *	In DKFLT state (i.e. the fault detection single thread state.)
 */
void
sd01intres(struct sb *sbp)
{
	struct	disk *dk;
	dev_t	 dev;			/* External device number 	*/

	dev = makedevice(sbp->SCB.sc_dev.sa_major, sbp->SCB.sc_dev.sa_minor);
	dk  = DKPTR(geteminor(dev));
	sd01untimeout((struct job *)sbp->SCB.sc_wd, sbp->SCB.sc_comp_code);

#ifdef DEBUG
        DPR (1)("sd01intres: (sbp=0x%x)\n", sbp);
        DPR (6)("sd01intres: (sbp=0x%x)\n", sbp);
#endif

	if (sbp->SCB.sc_comp_code & SDI_RETRY && dk->dk_spcount <= SD01_RETRYCNT)
	{
		if (sbp->SCB.sc_comp_code == SDI_RESET ||
		    sbp->SCB.sc_comp_code == SDI_CRESET ||
		    (sbp->SCB.sc_comp_code == SDI_CKSTAT &&
		     sbp->SCB.sc_status == S_CKCON))
		{
			/*
			*  Must restart the gauntlet!
			*  The queue has already been SUSPENDED, so go
			*  back and do the Request Sense job.
			*/
			if (sbp->SCB.sc_comp_code == SDI_CRESET && dk->dk_rescnt > SD01_RST_ERR)
			{
				/* This job has caused to many resets */
				sd01logerr(sbp, dk, 0x6dd27006);
				sd01flterr(dk, 0);
				return;
			}

			dk->dk_rescnt++;
			dk->dk_spcount = 0;
			dk->dk_fltreq->sb_type = ISCB_TYPE;
			dk->dk_fltreq->SCB.sc_int = sd01intrq;
			dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltreq->SCB.sc_link = 0;
			dk->dk_fltreq->SCB.sc_resid = 0;
			dk->dk_fltreq->SCB.sc_time = JTIME;
			dk->dk_fltreq->SCB.sc_mode = SCB_READ;
			dk->dk_fltreq->SCB.sc_dev = sbp->SCB.sc_dev;
			dk->dk_fltcmd.ss_op = SS_REQSEN;
			dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
			dk->dk_fltcmd.ss_addr1 = 0;
			dk->dk_fltcmd.ss_addr  = 0;
			dk->dk_fltcmd.ss_len = SENSE_SZ;
			dk->dk_fltcmd.ss_cont = 0;
			dk->dk_sense.sd_key = SD_NOSENSE; /* Clear old sense key */
			sd01timeout((struct job *)dk->dk_fltreq->SCB.sc_wd);
			if (sdi_icmd(dk->dk_fltreq, KM_NOSLEEP) != SDI_RET_OK)
			{

#ifdef DEBUG
				cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
				sd01untimeout((struct job *)
					      dk->dk_fltreq->SCB.sc_wd, SDI_ERROR);
				sd01flterr(dk, 0);
			}
			return;
		}
		else 	/* Not RESET or CRESET */
		{
			dk->dk_fltres->sb_type = ISCB_TYPE;
			dk->dk_fltres->SCB.sc_int = sd01intres;
			dk->dk_fltres->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltres->SCB.sc_link = 0;
			dk->dk_fltres->SCB.sc_resid = 0;
			dk->dk_fltres->SCB.sc_time = JTIME;
			dk->dk_fltres->SCB.sc_mode = SCB_WRITE;
			dk->dk_fltres->SCB.sc_dev = sbp->SCB.sc_dev;
			dk->dk_fltcmd.ss_op = SS_RESERV;
			dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
			dk->dk_fltcmd.ss_addr1 = 0;
			dk->dk_fltcmd.ss_addr  = 0;
			dk->dk_fltcmd.ss_len = 0;
			dk->dk_fltcmd.ss_cont = 0;
			dk->dk_sense.sd_key = 0;

			dk->dk_spcount++;
			dk->dk_error++;
		
			sd01timeout((struct job *)dk->dk_fltres->SCB.sc_wd);
			if (sdi_icmd(dk->dk_fltres, KM_NOSLEEP) != SDI_RET_OK)
			{
#ifdef DEBUG
				cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
				sd01untimeout((struct job *)
					      dk->dk_fltres->SCB.sc_wd, SDI_ERROR);
				sd01flterr(dk, 0);
			}
			return;
		}
	}

	if (sbp->SCB.sc_comp_code != SDI_ASW)
	{
		dk->dk_error++;
		sd01logerr(sbp, dk, 0x6dd27003);
		sd01flterr(dk, 0);
		return;
	}

	/* RESERVE Job completed ASW */

	SD01_DK_LOCK(dk);
	dk->dk_state |= DKRESERVE;
	SD01_DK_UNLOCK(dk);

	sd01fltjob(dk);

#ifdef DEBUG
        DPR (2)("sd01intres: - exit\n");
#endif
}

/*
 * void
 * sd01fltjob(struct disk *dk)
 *	This function uses the Request Sense information to determine
 *	what to do with the original job. Of course, there may not be an
 *	original job if the gauntlet had been entered via the HAD bus
 *	reset entry point.
 * Called by: sd01intrq, sd01intres
 * Side Effects: May restart the original job.
 * Errors:
 *	The SCSI disk driver is retrying an I/O request because of an error
 *	detected by the target controller. The cause of the error is
 *	indicated by the second and third error codes. These error codes
 *	are the sense key and extended sense code respectively. See the
 *	disk target controller code for more information.
 * 
 *	The disk controller performed retry or ECC which was successful.
 *	The cause of the error is indicated by the second and third
 *	error codes. These error codes are the sense key and extended
 *	sense codes respectively. See the disk target controller codes
 *	for more information.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKFLT state (i.e. the fault detection single thread state.)
 */
void
sd01fltjob(struct disk *dk)
{
	struct job *jp;		/* Job structure to be restarted */
	struct sb  *osbp;	/* Original job SB pointer */

#ifdef DEBUG
        DPR (1)("sd01fltjob: (dk=0x%x)\n", dk);
        DPR (6)("sd01fltjob: (dk=0x%x)\n", dk);
#endif

	if ((jp = (struct job *) dk->dk_fltres->SCB.sc_wd) != NULL)
		osbp = jp->j_cont;	/* SB of a real job */
	else
		osbp = dk->dk_fltres;	/* No Job but still need an SB */

	dk->dk_sense.sd_ba = sdi_swap32(dk->dk_sense.sd_ba);

        if (jp && (jp->j_flags & J_BADBLK) && !(jp->j_bp->b_flags & B_READ) &&
	    (jp->j_cont->SCB.sc_comp_code == SDI_ASW)) {
                /* ECC occurred on a previous read, and this
                 * is a write that succeeded, and we have suspended
		 * the queue and can continue to do the reassign/remap now.
                 */
		sd01flterr(dk, 0);
		return;
	}

	/* Request Sense information */
	switch(dk->dk_sense.sd_key){
		case SD_NOSENSE:
		case SD_ABORT:
		case SD_VENUNI:
			sd01logerr(osbp, dk, 0x4dd2b001);
			/*FALLTHRU*/
		case SD_UNATTEN: /* Don't log unit attention */

			/* Is there a real job to retry */
			if (jp !=  (struct job *) NULL)
			{
				
				/*
				*  If the job retry count or the reset count
				*  has exceeded it's limit, then fail the job.
				*  Otherwise try it again.
				*/
				if ((osbp->SCB.sc_comp_code == SDI_CRESET &&
				    jp->j_error >= SD01_RST_ERR) ||
				    jp->j_error >= SD01_RETRYCNT)	{
					sd01flterr(dk, DKRESERVE);
				}
				else
				{
					dk->dk_fltres->SCB.sc_wd = NULL;
					/*
					*  Exit the gauntlet before
					*  retrying the original job.
					*/
					sd01retry(jp);
					sd01flterr(dk, 0);
				}
			}
			else	{
				/* No job to retry! Clean up as usual */
				sd01flterr(dk, DKRESERVE);
			}
#ifdef DEBUG
        DPR (2)("sd01fltjob: - skey(0x%x)\n", dk->dk_sense.sd_key);
#endif
			return;

		case SD_RECOVER:
			dk->dk_error++;
			if (Sd01log_marg) 
			{
				switch(dk->dk_sense.sd_sencode)
				{
			  	case SC_DATASYNCMK:
			  	case SC_RECOVRRD:
			  	case SC_RECOVRRDECC:
			  	case SC_RECOVRIDECC:
					/* Indicate marginal bad block found */
					jp->j_flags |= J_HDEREC; 
					break;
			  	default:
					sd01logerr(osbp, dk, 0x4dd2b002);
				}
			}

			osbp->SCB.sc_comp_code = SDI_ASW;
			sd01flterr(dk, DKRESERVE);
#ifdef DEBUG
        DPR (2)("sd01fltjob: - skey(0x%x)\n", dk->dk_sense.sd_key);
#endif
			return;

		case SD_MEDIUM:
			switch(dk->dk_sense.sd_sencode)
			{
			  case SC_IDERR:
			  case SC_UNRECOVRRD:
			  case SC_NOADDRID:
			  case SC_NOADDRDATA:
			  case SC_NORECORD:
			  case SC_DATASYNCMK:
			  case SC_RECOVRIDECC:
				/* Indicate actual bad block found */
				jp->j_flags |= J_HDEECC; 
			}
			/*FALLTHRU*/
		default:
			dk->dk_error++;
			sd01flterr(dk, DKRESERVE);
#ifdef DEBUG
        DPR (2)("sd01fltjob: - skey(0x%x)\n", dk->dk_sense.sd_key);
#endif
			return;
	}
}

/*
 * void
 * sd01flterr(struct disk *dk, int	res_flag)
 *	This function is called in the gauntlet when an error
 *	occurs. If the Gauntlet was unable to re-RESERVE the device,
 *	then let the user know with the logged error.
 *	If there is a job waiting to be retried, call
 *	sd01comp1 to fail the job and resume the queue.
 *	Otherwise, just resume the queue. A separate function was
 *	made since this test will need to be made all thru the gauntlet.
 *	It is assumed that the error has already been logged before
 *	this function is called.
 * Called by: sd01intn, sd01flt, sd01flts, sd01ints, sd01intrq, sd01intres
 * Side Effects: None.
 * Errors:
 *	The SCSI Disk Driver was unable to re-RESERVE a device.
 *	Some hardware problem or the device was reserved by some
 *	other host is probably causing the driver to fail in its attempt
 *	to RESERVE the device.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock
 *	In DKFLT state on entry (i.e. the fault detection single thread state.)
 *	Exits DKFLT state.
 *	
 */
void
sd01flterr(struct disk *dk, int	res_flag)
{
	register struct job *jp, *qjp, *fltque;
#ifdef DEBUG
	char sd01name[SDI_NAMESZ];
#endif

#ifdef DEBUG
        DPR (1)("sd01flterr: (dk=0x%x res_flag=%d)\n", dk, res_flag);
#endif

	jp = (struct job *)dk->dk_fltres->SCB.sc_wd;
#ifdef DEBUG
        DPR (2)("jp in comp 0x%x\n",jp);
#endif
	SD01_DK_LOCK(dk);
	/*
	*  If the device was RESERVED but the gauntlet was unable to
	*  re-RESERVE the device, clear the RESERVE flag, let user
	*  know there was a problem and log the error.
	*/
	if (dk->dk_state & DKRESERVE && res_flag == 0)
	{
		dk->dk_state &= ~DKRESERVE;
#ifdef DEBUG
		sdi_name(&dk->dk_addr, sd01name);
		cmn_err(CE_CONT, "Disk Driver: Dev not RESERVED");
		cmn_err(CE_CONT, "%s, Unit = %d", sd01name, dk->dk_addr.sa_lun);
#endif
	}

	/*
	*  The gauntlet is finished.
	*  Reset the job pointer, retry jobs on fault queue, and clear state,
	*  so the next time the gauntlet is entered, it has been initialized
	*  to the proper state.
	*/
	dk->dk_fltres->SCB.sc_wd = NULL;

	fltque = dk->dk_fltque;
	dk->dk_fltque = NULL;
	dk->dk_state &= ~DKFLT;		/* Exit DKFLT */

	while (qjp = fltque) {
		fltque = qjp->j_next;
		SD01_DK_UNLOCK(dk);
		sd01retry(qjp);
		SD01_DK_LOCK(dk);
	}
	
	/* Is there a job to restart */
	if (jp == NULL)	{
		sd01qresume(dk);	/* No job */
		SD01_DK_UNLOCK(dk);
	}
	else	{
		SD01_DK_UNLOCK(dk);
		sd01comp1(jp);		/* Return the job */
	}

#ifdef DEBUG
        DPR (2)("sd01flterr: - exit\n");
#endif
}

/*
 * void
 * sd01intb(struct sb *sbp)
 *	This function is called by the host adapter driver when a
 *	SCSI Bad Block job or Request Sense job completes.  If the job fails, 
 *	the job is retried.  If the retry fails, the error is marked in the 
 *	buffer and the function returns.  However, if the job that failed 
 *	was a Reassign Blocks command, this function will send a Request 
 *	Sense to determine if the drive has run out of spare sectors.
 * Called by: Host adapter driver
 * Side Effects: None
 * Errors:
 *	The host adapter rejected a retry job request from the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 *	The host adapter rejected a Request Sense request from the SCSI disk 
 *	driver.  This is caused by a parameter mismatch within the driver. The 
 *	system should be rebooted.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
void
sd01intb(struct sb *sbp)
{
	register struct job *jp;
	register struct disk *dk;
	register struct scb *scb;
	register struct scs *cmd;

	jp  = (struct job *) sbp->SCB.sc_wd;
	sd01untimeout(jp, sbp->SCB.sc_comp_code);
	scb = &jp->j_cont->SCB;
	cmd = &jp->j_cmd.cs;
	dk  = jp->j_dk;


#ifdef DEBUG
        DPR (1)("sd01intb: (sbp=0x%x) jp=0x%x\n", sbp, jp);
        DPR (6)("sd01intb: (sbp=0x%x) jp=0x%x\n", sbp, jp);
#endif

	/* Check if interrupt was due to a Request Sense job */
	if (sbp == dk->dk_fltreq) {
		dk->dk_sense.sd_ba = sdi_swap32(dk->dk_sense.sd_ba);
#ifdef SD01_DEBUG
	if(sd01alts_debug & HWREA) {
		cmn_err(CE_CONT,
		"sd01intb: Request Sense sd_ba= 0x%x sd_valid= 0x%x\n", 
		dk->dk_sense.sd_ba, dk->dk_sense.sd_valid);
	}
#endif
		/* Fail the job */
		dk->hde_state += 1;

		/* Check if the Request Sense was ok */
		if (sbp->SCB.sc_comp_code == SDI_ASW) 
		{
			switch(dk->dk_sense.sd_key)
			{
				case SD_RECOVER:
					switch(dk->dk_sense.sd_sencode)
					{
			  		case SC_DATASYNCMK:
			  		case SC_RECOVRRD:
			  		case SC_RECOVRRDECC:
			  		case SC_RECOVRIDECC:
						/* Pass the job */
						dk->hde_state += 1;
					}
					break;
				case SD_MEDIUM:
					/* Check for Reassign command */
					if (cmd->ss_op == SS_REASGN)
						/* Indicate no spare sectors */
						dk->hde_state |= HDEENOSPR;
			}
		}
		/* Resume execution */
		sd01_bbhdwrea(jp);
		return;
	}
	
	/* Check if Bad Block job completed successfully. */
	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW)
	{
		/* Retry the job */
		if (jp->j_bp->b_error < SD01_RETRYCNT)
		{
#ifdef DEBUG
			DPR (6)("retry\n");
#endif
			jp->j_bp->b_error++;	  	 /* Update error count*/
			jp->j_cont->SCB.sc_time = JTIME; /* Reset the job time*/

			sd01timeout(jp);
			if (sdi_icmd(jp->j_cont, KM_NOSLEEP) != SDI_RET_OK)
			{
#ifdef DEBUG
				cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
				/* Fail the job */
				dk->hde_state += 1;
				sd01untimeout(jp, SDI_ERROR);
			}
			else
				return;
		}
		else {
#ifdef DEBUG
			DPR (6)("send REQSEN\n");
#endif
			/* Send a Request Sense job */
			dk->dk_fltreq->sb_type 	    = ISCB_TYPE;
			dk->dk_fltreq->SCB.sc_int   = sd01intb;
			dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltreq->SCB.sc_link  = 0;
			dk->dk_fltreq->SCB.sc_resid = 0;
			dk->dk_fltreq->SCB.sc_time  = JTIME;
			dk->dk_fltreq->SCB.sc_mode  = SCB_READ;
			dk->dk_fltreq->SCB.sc_dev   = scb->sc_dev;
			dk->dk_fltreq->SCB.sc_wd    = (long) jp;
			dk->dk_fltcmd.ss_op         = SS_REQSEN;
			dk->dk_fltcmd.ss_lun        = cmd->ss_lun;
			dk->dk_fltcmd.ss_addr1      = 0;
			dk->dk_fltcmd.ss_addr       = 0;
			dk->dk_fltcmd.ss_len        = SENSE_SZ;
			dk->dk_fltcmd.ss_cont       = 0;
			dk->dk_sense.sd_key         = SD_NOSENSE; 

			sd01timeout(jp);
			if (sdi_icmd(dk->dk_fltreq, KM_NOSLEEP) == SDI_RET_OK)
				return;

			/* Fail the job */
			sd01untimeout(jp, SDI_ERROR);
			dk->hde_state += 1;
		}
	}
	else	{
		dk->hde_state += 2;
	}

#ifdef DEBUG
        DPR (2)("sd01intb: - exit\n");
        DPR (6)("sd01intb: - exit\n");
#endif

	/* Resume execution */
	sd01_bbhdwrea(jp);
}

/*
 * int
 * sd01getalts(struct disk *dk)
 *	get bad sector/track alternate tables
 *
 * Calling/Exit State:
 *	Disk struct's dk_lock held on entry and return, but
 *	may be dropped in middle.
 */
int
sd01getalts(struct disk *dk)
{
	struct	partition *pp = &(dk->dk_vtoc.v_part[0]);
	int	i;
	int	status = 0;
	ushort_t tag;

#ifdef SD01_DEBUG
	if(sd01alts_debug & DKADDR) {
		cmn_err(CE_CONT,
		  "sd01getalts: ADDR &dk_altcount= 0x%x &dk_firstalt= 0x%x\n",
			&(dk->dk_altcount[0]), &(dk->dk_firstalt[0]));
		cmn_err(CE_CONT,
			"  &dk_alts_parttbl= 0x%x &dk_amp= 0x%x \n",
			&(dk->dk_alts_parttbl), dk->dk_amp);
		cmn_err(CE_CONT,
			"  &dk_wkamp= 0x%x &dk_ast_p= 0x%x\n",
			dk->dk_wkamp, dk->dk_ast);
		cmn_err(CE_CONT,
			"  &dk= 0x%x &dk_fltreq= 0x%x\n",
			dk, &(dk->dk_fltreq));
#ifdef PDI_SVR42
		cmn_err(CE_CONT,
			"  &dk_stat.maxqlen = 0x%x\n", &(dk->dk_stat.maxqlen));
#endif
	}
#endif

	/* search for partition ALTSCTR */
	for (i=V_NUMPAR-1; i>=0; i--) {
		tag =pp[i].p_tag;
		if (((tag == V_ALTSCTR) || (tag == V_ALTS))
		 && (pp[i].p_flag & V_VALID))
			break;
	}

	switch (tag) {
	case V_ALTSCTR:
		dk->dk_remapalts = sd01remap_altsctr;
		SD01_DK_UNLOCK(dk);
		status = sd01get_altsctr(dk, &pp[i]);
		break;
	case V_ALTS:
		dk->dk_remapalts = sd01remap_altsec;
		SD01_DK_UNLOCK(dk);
		status = sd01get_altsec(dk);
		break;
	default:
		ASSERT(dk->dk_amp == 0);
		return(status);
	}

	SD01_DK_LOCK(dk);
	return(status);
}


/*
 * void
 * sd01xalt(struct disk *dk, struct  alt_info *alttbl, struct  alts_ent *enttbl)
 *	translate AT&T alternates table into common entry table
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
void
sd01xalt(struct disk *dk, struct alt_info *alttbl, struct alts_ent *enttbl)
{
	unsigned long spt = dk->dk_parms.dp_sectors;
	daddr_t good;
	int	idx;
	int 	j; 
	int	entused;

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Entering sd01xalt\n");
	}
#endif

	entused = alttbl->alt_trk.alt_used + alttbl->alt_sec.alt_used;

	/* get base of AT&T good tracks for bad track mapping 
	 * all good sectors are contiguous from here
	 */
	good = alttbl->alt_trk.alt_base;

	/* Process each AT&T bad track */
	for (idx=0; idx <(int)alttbl->alt_trk.alt_used; idx++) {  
		enttbl[idx].bad_start  = alttbl->alt_trk.alt_bad[idx]*spt;
		enttbl[idx].bad_end    = enttbl[idx].bad_start + spt - 1;
		enttbl[idx].good_start = good; 
		good = good + spt;
	}

	/* get base of AT&T good sectors for bad sector mapping
	 * translate the sectors
	 */
	good = alttbl->alt_sec.alt_base;

	for (j=0; j<(int)alttbl->alt_sec.alt_used; idx++, good++, j++) {
		enttbl[idx].bad_start  = alttbl->alt_sec.alt_bad[j];
		enttbl[idx].bad_end    = enttbl[idx].bad_start;
		enttbl[idx].good_start = good; 
	}

	/* sort the alternate entry table in ascending bad sector order */
	sd01sort_altsctr(enttbl,entused);
}


/*
 * int
 * sd01get_altsec(struct disk *dk)
 *	read from disk the alternate table information
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *	In DKVTOC state (i.e. disk is in process of being opened).
 */
int
sd01get_altsec(struct disk *dk)
{
	struct  phyio alt_phyarg;	
	struct 	alt_info *alttblp;	/* AT&T disk alt structure 	*/
	int	mystatus = 1;
	void	*tp;

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Entering sd01get_altsec\n");
	}
#endif
	if (sd01ast_init(dk))
		return(mystatus);

	alt_phyarg.sectst = dk->unixst + (dk->dk_pdsec.alt_ptr >> BLKSHF(dk));
	alt_phyarg.memaddr = (long) dk->dk_ast->ast_alttblp;
	alt_phyarg.datasz = dk->dk_ast->ast_altsiz;

	sd01phyrw(dk, (long) V_RDABS, &alt_phyarg, SD01_KERNEL);

	if (alt_phyarg.retval) {       /* Error reading alts table */
		/*
		 *+ The disk driver is unable to read the alternates
		 *+ table.
		 */
		cmn_err(CE_NOTE, "!Disk Driver: error %d reading ALT_TBL on dskp 0x%x", alt_phyarg.retval, dk);
		sd01ast_free(dk);
		return(mystatus);
	}

	alttblp = dk->dk_ast->ast_alttblp;
	/* sanity check */
	if (alttblp->alt_sanity != ALT_SANITY) {
		sd01ast_free(dk);
		return(mystatus);
	}

	dk->dk_ast->ast_entused = alttblp->alt_trk.alt_used + 
				alttblp->alt_sec.alt_used;

	if (!dk->dk_ast->ast_entused) {
		dk->dk_ast->ast_entp = (struct alts_ent *)NULL;
	} else {
		/* allocate storage for common incore alternate entry tbl */
		if ((tp = (struct alts_ent *)kmem_alloc(
		    byte_to_blksz((ALTS_ENT_SIZE * dk->dk_ast->ast_entused),
		    BLKSZ(dk)), KM_SLEEP)) == NULL) {
			sd01ast_free(dk);
			return(mystatus);
		}
		dk->dk_ast->ast_entp = tp;
		/* translate AT&T altsec format to common entry table */
		sd01xalt(dk,dk->dk_ast->ast_alttblp, dk->dk_ast->ast_entp);
	}

	/* The update and setalts_idx routines expect the dk_lock to
	 * be held, but it is overkill here since this call is already
	 * protected by DKVTOC, and this is the open, so no one else
	 * is reading the alternate table for i/o startup yet.
	 */
	SD01_DK_LOCK(dk);

	sd01upd_altsec(dk);

	/* set index table for each partition to the alts entry table */
	sd01setalts_idx(dk);

	SD01_DK_UNLOCK(dk);

	return (0);
}

/*
 * void
 * sd01upd_altsec(struct disk *dk)
 *	Update with new alts info 
 *
 * Calling/Exit State:
 *	dk_lock held on entry and exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
void
sd01upd_altsec(struct disk *dk)
{
	if (dk->dk_amp != NULL) {
		if (dk->dk_amp->ap_rcnt == 0) {
			/* release the original alts entry table */
			sd01rel_altsctr (dk->dk_amp);
		} else {
			/* Reference count non-zero, so set data obsolete */
			dk->dk_amp->ap_flag |= ALT_OBSOLETE;
		}
		dk->dk_amp = NULL;
	}

	if (dk->dk_alttbl) {
		kmem_free(dk->dk_alttbl,byte_to_blksz(dk->dk_pdsec.alt_len,BLKSZ(dk)));
		dk->dk_alttbl = NULL;
	}

	/* initialize the alternate partition table */
	bzero((caddr_t)&(dk->dk_alts_parttbl), ALTS_PARTTBL_SIZE);
	dk->dk_alts_parttbl.alts_sanity   = dk->dk_ast->ast_alttblp->alt_sanity;
	dk->dk_alts_parttbl.alts_version  = dk->dk_ast->ast_alttblp->alt_version;
	dk->dk_alts_parttbl.alts_ent_used = dk->dk_ast->ast_entused;

	/* record new alts info  and alts entry tables */
	if (dk->dk_amp == NULL) {
		/* The dk_amp structure needs to be allocated once for
		 * AT&T style tables, at open time,
		 * (and since its open time we can safely unlock since
		 * we are single threaded by the DK_VTOC state).
		 */
		SD01_DK_UNLOCK(dk);
		dk->dk_amp = (struct alts_mempart *)kmem_zalloc(
			ALTS_MEMPART_SIZE, KM_SLEEP);
		SD01_DK_LOCK(dk);
	}
	dk->dk_amp->ap_entp = dk->dk_ast->ast_entp;
	dk->dk_amp->ap_ent_secsiz = byte_to_blksz((ALTS_ENT_SIZE * 
				dk->dk_ast->ast_entused),BLKSZ(dk));
	dk->dk_alttbl = dk->dk_ast->ast_alttblp;
	bzero((caddr_t)dk->dk_ast, ALTSECTBL_SIZE);
}


/* ARGSUSED */
/*
 * int
 * sd01addbad(dev_t dev, caddr_t arg)
 *	Handler for growing bad sector
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sd01addbad(dev_t dev, caddr_t arg)
{
	register struct disk *dk;	/* Pointer to disk structure	*/
	struct 	alts_mempart amp;
	int	gbadsz;
	struct	alts_ent *gbp;
	int	i;
	int 	status = 0;
        struct job *jp;

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		/*
		 *+
		 */
		cmn_err(CE_NOTE, "Entering sd01addbad\n");
	}
#endif
	dk  = DKPTR(geteminor(dev));

/*	get growing bad sectors from user				*/
	if (copyin(arg, (caddr_t)&amp, ALTS_MEMPART_SIZE)) 
		return(EFAULT);

	gbadsz = amp.ap_gbadcnt * ALTS_ENT_SIZE;
	if ((gbp = (struct alts_ent *)kmem_alloc(gbadsz,KM_SLEEP)) == NULL)
		return(EFAULT);

	if (copyin((caddr_t)amp.ap_gbadp, (caddr_t)gbp, gbadsz))
		return(EFAULT);
/*
 *	set spl before altering the incore alternate entry table.
 */
	SD01_DK_LOCK(dk);
	while(dk->dk_state & DKMAPBAD) {
		SV_WAIT(dk->dk_sv, pridisk, dk->dk_lock);
		SD01_DK_LOCK(dk);
	}
	dk->dk_state |= DKMAPBAD;
	/*
	 * Now in the DKMAPBAD single thread
	 */

	dk->dk_gbadsec_cnt = amp.ap_gbadcnt; 
	dk->dk_gbadsec_p   = gbp;

/*	assume add bad sector succeed					*/
	status = 0;
/*	suspend the hba queue						*/
	sd01isfb(dk, SFB_SUSPEND);

        /* check hba queue being suspended */
        if ((dk->dk_state & DKSUSP) &&
                (jp = (struct job *)sdi_get(&lg_poolhead, KM_NOSLEEP))) {
                jp->j_dk = dk;
		jp->j_bp = dk->dk_bbhbuf;
                dk->dk_jp_orig = jp;
                dk->dk_datap = (char *)NULL;    /* Disable data restore   */

/*		attempt hardware reassign				*/
		for (gbp=dk->dk_gbadsec_p, i=0; i<dk->dk_gbadsec_cnt; i++) {
			if (gbp[i].bad_start == ALTS_ENT_EMPTY)
				continue;
			dk->dk_hdesec = gbp[i].bad_start;
			dk->hde_state = (HDEHWREA | HDEECCERR);
#ifdef SD01_DEBUG
	if(sd01alts_debug & HWREA) {
		/*
		 *+
		 */
		cmn_err(CE_NOTE, "sd01addbad: HW REASGN sector= %d hde_state= 0x%x\n", 
			gbp[i].bad_start, dk->hde_state);
	}
#endif
			sd01_bbhdwrea(jp);

			SD01_DK_LOCK(dk);
			while (dk->hde_state & HDEHWREA) {
				SV_WAIT(dk->dk_sv, pridisk, dk->dk_lock);
				SD01_DK_LOCK(dk);
			}

/*			check for HW reassign error			*/
			if (dk->hde_state & HDEREAERR) {
				dk->hde_state &= ~HDEREAERR;
#ifdef SD01_DEBUG
	if(sd01alts_debug & HWREA) {
		/*
		 *+
		 */
		cmn_err(CE_NOTE, "sd01addbad: HW REASGN FAIL sector= %d hde_state= 0x%x\n", 
			gbp[i].bad_start, dk->hde_state);
	}
#endif
				break;
			} else 
				gbp[i].bad_start = ALTS_ENT_EMPTY;
		}

		if (i<dk->dk_gbadsec_cnt) {	
			/* software remap */
                        sd01_bbremap(dk, SD01_ADDBAD, NULL);

			SD01_DK_LOCK(dk);
                        while (dk->hde_state & HDESWMAP)        {
				SV_WAIT(dk->dk_sv, pridisk, dk->dk_lock);
				SD01_DK_LOCK(dk);
			}

/*			flush job queue					*/
			sd01flush_job(dk);

/*			flush hba queue					*/
			sd01isfb(dk, SFB_FLUSHR);
		}
		sdi_free(&lg_poolhead, (jpool_t *)jp);
/*		resume the queue					*/
		sd01qresume(dk);
	} else
		status = 1;

	kmem_free(gbp, gbadsz);
        /*
         * NOTE: Do not reset DKMAPBAD here... the end-of-remap
         * code either reset it, or a new bad block is being processed
         * and DKMAPBAD will be reset at the end of that.
         */

#ifdef SD01_DEBUG
	if(sd01alts_debug & HWREA) {
		/*
		 *+
		 */
		cmn_err(CE_NOTE, "sd01addbad: return with status= %d\n", status);
	}
#endif
	if (status) 
		return(EIO);

	return(0);
}

/*
 * int
 * sd01get_altsctr(struct disk *dk, struct  partition *partp)
 *	read the alternate sector/track partition information from disk
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *	In DKVTOC state (i.e. disk is in process of being opened).
 */
int
sd01get_altsctr(struct disk *dk, struct  partition *partp)
{
	char 	*dsk_tblp;
	char 	*mem_tblp;
	int	dsk_tbl_secsiz;
	int	mem_tbl_secsiz;
	struct  alts_parttbl *ap;
	struct  phyio alt_phyarg;	
	int	mystatus = 1;


	/* allocate incore alternate partition table */
	dsk_tbl_secsiz = byte_to_blksz(ALTS_PARTTBL_SIZE, BLKSZ(dk));
	if ((dsk_tblp = (char *)kmem_alloc(dsk_tbl_secsiz,KM_SLEEP)) == NULL)
		return (mystatus);

	alt_phyarg.sectst  = partp->p_start;
	alt_phyarg.memaddr = (long) dsk_tblp;
	alt_phyarg.datasz  = dsk_tbl_secsiz;

	sd01phyrw(dk, (long) V_RDABS, &alt_phyarg, SD01_KERNEL);
	if (alt_phyarg.retval) {       	/* Error reading alts table 	*/
		/*
		 *+ The disk driver is unable to read the alternates
		 *+ table.
		 */
		cmn_err(CE_NOTE,
		  "!Disk Driver: error %d reading alts_parttbl dk 0x%x",
			alt_phyarg.retval, dk);
		kmem_free(dsk_tblp, dsk_tbl_secsiz);
		dsk_tblp = NULL;
		return(mystatus);
	}

	/* sanity checking */
	ap = (struct alts_parttbl *)(void *) dsk_tblp;
	if (ap->alts_sanity != ALTS_SANITY) {
		kmem_free(dsk_tblp, dsk_tbl_secsiz);
		dsk_tblp = NULL;
		return(mystatus);
	}

	if (ap->alts_ent_used == 0) {
		mem_tblp = (char *) NULL;
		mem_tbl_secsiz = 0;
	} else {
		/* allocate incore alternate entry table */
		mem_tbl_secsiz=(ap->alts_ent_end-ap->alts_ent_base+1)*BLKSZ(dk);
		if ((mem_tblp = (char *)kmem_alloc(mem_tbl_secsiz,KM_SLEEP)) 
			== NULL) {
			kmem_free(dsk_tblp, dsk_tbl_secsiz);
			dsk_tblp = NULL;
			return (mystatus);
		}

		alt_phyarg.sectst  = partp->p_start +
		    ((struct alts_parttbl *)(void *)dsk_tblp)->alts_ent_base;
		alt_phyarg.memaddr = (long) mem_tblp;
		alt_phyarg.datasz  = mem_tbl_secsiz;

		sd01phyrw(dk, (long) V_RDABS, &alt_phyarg, SD01_KERNEL);

		if (alt_phyarg.retval) {
			/*
			 *+ The disk driver is unable to read the alternates
			 *+ table.
			 */
			cmn_err(CE_NOTE,
			"!Disk Driver: error %d reading alts_enttbl dk 0x%x",
				alt_phyarg.retval, dk);
			kmem_free(dsk_tblp, dsk_tbl_secsiz);
			dsk_tblp = NULL;
			kmem_free(mem_tblp, mem_tbl_secsiz);
			mem_tblp = NULL;
			return(mystatus);
		}
	}

	if (sd01alloc_wkamp(dk, partp, ap)) {
		kmem_free(dsk_tblp, dsk_tbl_secsiz);
		dsk_tblp = NULL;
		if (mem_tblp) {
			kmem_free(mem_tblp, mem_tbl_secsiz);
			mem_tblp = NULL;
		}
		return(mystatus);
	}

	kmem_free(dk->dk_wkamp->ap_tblp, dk->dk_wkamp->ap_tbl_secsiz);
	dk->dk_wkamp->ap_tblp = NULL;

	dk->dk_wkamp->ap_tblp = (struct alts_parttbl *)(void *)dsk_tblp;
	dk->dk_wkamp->ap_entp = (struct alts_ent *)(void *)mem_tblp;
	dk->dk_wkamp->ap_tbl_secsiz = dsk_tbl_secsiz;
	dk->dk_wkamp->ap_ent_secsiz = mem_tbl_secsiz;

	/* get the alts map */
	alt_phyarg.sectst  = partp->p_start + ap->alts_map_base;
	alt_phyarg.memaddr = (long) dk->dk_wkamp->ap_mapp;
	alt_phyarg.datasz  = dk->dk_wkamp->ap_map_secsiz;

	sd01phyrw(dk, (long) V_RDABS, &alt_phyarg, SD01_KERNEL);

	/* if error reading alts map  */
	if (alt_phyarg.retval) {
		/*
		 *+ The disk driver is unable to read the alternates map.
		 */
		cmn_err(CE_NOTE,
			"!Disk Driver: error %d reading alts_map dk 0x%x",
			alt_phyarg.retval, dk);
		sd01free_wkamp(dk);
		return(mystatus);
	}

	/* transform the disk image bit-map to incore char map */
	sd01expand_map(dk->dk_wkamp);

	/* The update and setalts_idx routines expect the dk_lock to
	 * be held, but it is overkill here since this call is already
	 * protected by DKVTOC, and this is the open, so no one else
	 * is reading the alternate table for i/o startup yet.
	 */
	SD01_DK_LOCK(dk);
	/* assign new alts partition and entry tables */
	sd01upd_altstbl(dk);

	/* set index table for each partition to the alts entry table */
	sd01setalts_idx(dk);

	SD01_DK_UNLOCK(dk);
	return (0);
}



/*
 * void
 * sd01setalts_idx(struct disk *dk)
 *	For each partition, except partition 0 which is the whole disk,
 *	find the first alternate entry for that partition and count how
 *	many apply to it.
 *
 * Calling/Exit State:
 *	dk_lock held on entry and exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
void
sd01setalts_idx(struct disk *dk)
{
	struct 	partition *pp = &(dk->dk_vtoc.v_part[0]);
	struct 	alts_ent *ap = dk->dk_amp->ap_entp;
	daddr_t lastsec;
	int 	i, j;

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Entering sd01setalts_idx\n");
	}
#endif

	/* go through all partitions  */
	for (i=0; i<V_NUMPAR; i++) {	
		/* initialize the index table for each partition */
		dk->dk_firstalt[i] = NULL;
		dk->dk_altcount[i] = 0;
		/* if no bad sector, then skip */
		if ((dk->dk_alts_parttbl.alts_ent_used == 0) || !ap)
			continue;
		/* if partition is empty, then skip */
		if (pp[i].p_size == 0)
			continue;
		lastsec = pp[i].p_start + pp[i].p_size - 1;
		for (j=0; j < dk->dk_alts_parttbl.alts_ent_used; j++) {
			/*
			 * if bad sector cluster is less than partition range
			 * then skip
			 */
			if ((ap[j].bad_start < pp[i].p_start) &&
			    (ap[j].bad_end   < pp[i].p_start))
				continue;
			/*
			 *if bad sector cluster passed the end of the partition
			 * then stop
			 */
			if (ap[j].bad_start > lastsec)
				break;
			if (dk->dk_firstalt[i] == NULL) 
				dk->dk_firstalt[i] =(struct alt_ent *)&ap[j];
			dk->dk_altcount[i]++;
		}
#ifdef SD01_DEBUG
		if(sd01alts_debug & DXALT) {
			cmn_err(CE_CONT,"sd01setalts_idx: firstalt= 0x%x cnt= %d\n",
			dk->dk_firstalt[i], dk->dk_altcount[i]);
		}
#endif
	}

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Leaving sd01setalts_idx\n");
	}
#endif
}


/*
 * int
 * sd01bsearch(struct alts_ent buf[], int cnt, daddr_t key, int exp_ret)
 *	binary search for key entry in the buf array. Return -1; if
 *	key is not found.
 *
 *	When exp_ret is set to DK_FIND_BIG, then 
 *	binary search for the largest bad sector index in the alternate
 *	entry table which overlaps or BIGGER than the given key entry
 *	Return -1; if given key is bigger than all bad sectors.
 *
 * Calling/Exit State:
 *	If buf points into a dk, it is assumed the caller has the dk locked.
 *	May be in DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
int
sd01bsearch(struct alts_ent buf[], int cnt, daddr_t key, int exp_ret)
{
	int	i;
	int	ind;
	int	interval;
	int	mystatus = -1;

	if (!cnt) return (mystatus);

	for (i=1; i<=cnt; i<<=1)
		ind=i;

	for (interval=ind; interval; ) {
		if ((key >= buf[ind-1].bad_start) && 
		(key <= buf[ind-1].bad_end))
			return(ind-1);

		interval >>= 1;
		if (key < buf[ind-1].bad_start) {
			/* record the largest bad sector index */
			if (exp_ret & DK_FIND_BIG)
				mystatus = ind-1;
			if (!interval)
				break; /* for */
			ind = ind - interval;
		} else {
			/* if key is larger than the last element then break */
			if ((ind == cnt) || !interval)
				break;
			if ((ind+interval) <= cnt)
				ind += interval;
		}
	}
	return(mystatus);
}


/*
 * void
 * sd01sort_altsctr(struct alts_ent buf[], int cnt)
 * 	bubble sort the entry table into ascending order
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
void
sd01sort_altsctr(struct alts_ent buf[], int cnt)
{
	struct	alts_ent temp;
	int	flag;
	int	i,j;

	for (i=0; i<cnt-1; i++) {
		temp = buf[cnt-1];
		flag = 1;
	    
		for (j=cnt-1; j>i; j--) {
			if (buf[j-1].bad_start < temp.bad_start) {
				buf[j] = temp;
				temp = buf[j-1];
			} else {
				buf[j] = buf[j-1];
				flag = 0;
			}
		}
		buf[i] = temp;
		if (flag)
			break;
	}

}

/*
 * void
 * sd01compress_ent(struct alts_ent buf[], int cnt)
 *	compress all the contiguous bad sectors into a single entry 
 *	in the entry table. The entry table must be sorted into ascending
 *	before the compression.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
void
sd01compress_ent(struct alts_ent buf[], int cnt)
{
	int	keyp;
	int	movp;
	int	i;

	for (i=0; i<cnt; i++) {
	    if (buf[i].bad_start == ALTS_ENT_EMPTY)
		continue;
	    if (buf[i].bad_start == buf[i].good_start)
		/* This is an interim alternate entry that
		   must be kept isolated */
		continue;
	    for (keyp=i, movp=i+1; movp<cnt; movp++) {
		if (buf[movp].bad_start == buf[movp].good_start)
			/* This is an interim alternate entry that
			   must be kept isolated */
			break;
	    	if (buf[movp].bad_start == ALTS_ENT_EMPTY)
			continue;
		/* check for duplicate bad sectors */
		if (buf[keyp].bad_end == buf[movp].bad_start) {
			buf[movp].bad_start = ALTS_ENT_EMPTY;
			continue;
		}
		/* check for contiguous bad sector */
		if (buf[keyp].bad_end+1 != buf[movp].bad_start)
		    break;
		buf[keyp].bad_end++;
		buf[movp].bad_start = ALTS_ENT_EMPTY;
	    }
	    if (movp == cnt) break;
	}
}

/*
 * void
 * sd01rel_altsctr(struct alts_mempart *ap)
 *	release the existing bad sector/track alternate entry table
 *
 * Calling/Exit State:
 *	Either called:
 *	1)In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or
 *	2)called with dk_lock set, but removing an obsolete copy (not
 *		the one pointed to by dk->amp.
 */
void
sd01rel_altsctr(struct alts_mempart *ap)
{

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Entering sd01rel_altsctr\n");
	}
#endif
	if (ap->ap_entp) {
		kmem_free(ap->ap_entp, ap->ap_ent_secsiz);
	}

	if (ap->ap_mapp) {
		kmem_free(ap->ap_mapp, ap->ap_map_secsiz);
	}

	if (ap->ap_memmapp) {
		kmem_free(ap->ap_memmapp, ap->part.p_size);
	}

	kmem_free(ap, ALTS_MEMPART_SIZE);

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Leaving sd01rel_altsctr\n");
	}
#endif
}

/*
 * void
 * sd01expand_map(struct alts_mempart *amp_p)
 *	transform the disk image alts bit map to incore char map
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *	In DKVTOC state (i.e. disk is in process of being opened).
 */
void
sd01expand_map(struct alts_mempart *amp_p)
{
	int 	i;

	for (i=0; i<amp_p->part.p_size; i++) {
	    (amp_p->ap_memmapp)[i] = sd01altsmap_vfy(amp_p, i);
	}
}


/*
 * int
 * sd01altsmap_vfy(struct alts_mempart *amp_p, daddr_t badsec)
 *	given a bad sector number, search in the alts bit map
 *	and identify the sector as good or bad
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKVTOC state (i.e. disk is in process of being opened).
 */
int
sd01altsmap_vfy(struct alts_mempart *amp_p, daddr_t badsec)
{
	int	slot = badsec / 8;
	int	field = badsec % 8;	
	unchar	mask;

	mask = ALTS_BAD<<7; 
	mask >>= field;
	if ((amp_p->ap_mapp)[slot] & mask)
	     return(ALTS_BAD);
	return(ALTS_GOOD);
}

/*
 * int
 * sd01remap_altsctr(struct disk *dk)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01remap_altsctr(struct disk *dk)
{
	int	mystatus = 1;

	if (sd01gen_altsctr(dk)) 
		return (mystatus);

	sd01compress_map(dk->dk_wkamp);

	/* Begin to sync the alternate table updates back to disk.
	 * The sd01wr_altsctr() routine will be called multiply until
	 * all writes are complete.
	 */
	dk->hde_state = 0;	/* Start with a cleared state */

	if (sd01wr_altsctr(dk)) {
		sd01free_wkamp(dk);
		return (mystatus);
	}
	return (0);
}



/*
 * int
 * sd01gen_altsctr(struct disk *dk)
 *	generate alternate entry table by merging the existing and
 *	the growing entry list.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01gen_altsctr(struct disk *dk)
{
	int	ent_used;
	int	ent_secsiz;
	struct	alts_ent *entp;
	int	mystatus = 1;
	void	*tp;


	if (sd01alloc_wkamp(dk,&(dk->dk_amp->part),&(dk->dk_alts_parttbl))) {
		/*
		 *+ The disk driver is unable to allocate the working
		 *+ alternates partition information.
		 */
 		cmn_err(CE_NOTE,
		"!Disk Driver: unable to allocate working alts partition info for disk 0x%x.",
		dk);
		return (mystatus);
	}

	bcopy((caddr_t)&(dk->dk_alts_parttbl), (caddr_t)dk->dk_wkamp->ap_tblp,
			ALTS_PARTTBL_SIZE);
	bcopy((caddr_t)dk->dk_amp->ap_mapp, (caddr_t)dk->dk_wkamp->ap_mapp, 
		dk->dk_wkamp->ap_map_secsiz);
	bcopy((caddr_t)dk->dk_amp->ap_memmapp,(caddr_t)dk->dk_wkamp->ap_memmapp,
		dk->dk_wkamp->part.p_size);

	if (dk->dk_alts_parttbl.alts_ent_used) {
		if ((tp = (struct alts_ent *)kmem_alloc(
		    dk->dk_amp->ap_ent_secsiz, KM_NOSLEEP)) == NULL ) {
			sd01free_wkamp(dk);
			return (mystatus);
		}

		dk->dk_wkamp->ap_entp = tp;
		bcopy((caddr_t)dk->dk_amp->ap_entp,(caddr_t)dk->dk_wkamp->ap_entp,
			dk->dk_amp->ap_ent_secsiz);
		dk->dk_wkamp->ap_ent_secsiz = dk->dk_amp->ap_ent_secsiz;
	}

	dk->dk_wkamp->ap_gbadcnt   = dk->dk_gbadsec_cnt;
	dk->dk_wkamp->ap_gbadp	 = dk->dk_gbadsec_p;

	if (sd01ck_gbad(dk)) {
		sd01free_wkamp(dk);
		return (mystatus);
	}

	ent_used = dk->dk_wkamp->ap_tblp->alts_ent_used + dk->dk_wkamp->ap_gbadcnt;
	ent_secsiz = byte_to_blksz(ent_used*ALTS_ENT_SIZE,BLKSZ(dk));

	entp=(struct alts_ent *)kmem_alloc(ent_secsiz, KM_NOSLEEP);

	if (entp == NULL) {
		sd01free_wkamp(dk);
		return (mystatus);
	} 

	sd01sort_altsctr(dk->dk_wkamp->ap_gbadp, dk->dk_wkamp->ap_gbadcnt);
	sd01compress_ent(dk->dk_wkamp->ap_gbadp, dk->dk_wkamp->ap_gbadcnt);

	ent_used = sd01ent_merge(entp, dk->dk_wkamp->ap_entp,
			dk->dk_wkamp->ap_tblp->alts_ent_used,
			dk->dk_wkamp->ap_gbadp, dk->dk_wkamp->ap_gbadcnt);

	kmem_free(dk->dk_wkamp->ap_entp, dk->dk_wkamp->ap_ent_secsiz); 
	dk->dk_wkamp->ap_entp = entp;
	dk->dk_wkamp->ap_tblp->alts_ent_used = ent_used;
	dk->dk_wkamp->ap_ent_secsiz = ent_secsiz;
	dk->dk_wkamp->ap_gbadp = NULL;
	dk->dk_wkamp->ap_gbadcnt = 0;

	if (!ent_used) {
		if (entp) {
			kmem_free(entp, ent_secsiz); 
			entp = NULL;
		}
		dk->dk_wkamp->ap_entp = NULL;
		dk->dk_wkamp->ap_ent_secsiz = 0;

		return (0);
	}

	/* assign alternate sectors to the bad sectors */
	if (sd01asgn_altsctr(dk)) {
		sd01free_wkamp(dk);
		return (mystatus);
	} 

	/* allocate the alts_entry on disk skipping possible bad sectors */
	dk->dk_wkamp->ap_tblp->alts_ent_base = 
		sd01alts_assign(dk->dk_wkamp->ap_memmapp, 
			dk->dk_wkamp->ap_tblp->alts_map_base + 
			dk->dk_wkamp->ap_map_sectot, dk->dk_wkamp->part.p_size, 
			dk->dk_wkamp->ap_ent_secsiz/(uint)BLKSZ(dk), ALTS_MAP_UP);

	if (dk->dk_wkamp->ap_tblp->alts_ent_base == NULL) {
		/*
		 *+ The disk driver is unable to allocate space in
		 *+ alternates partition for entry table.
		 */
 		cmn_err(CE_NOTE, "!Disk Driver: can't alloc space in alt partition for entry table.");
		sd01free_wkamp(dk);
		return (mystatus);
	}

	dk->dk_wkamp->ap_tblp->alts_ent_end= dk->dk_wkamp->ap_tblp->alts_ent_base + 
			(dk->dk_wkamp->ap_ent_secsiz / (uint)BLKSZ(dk)) - 1; 

	return (0);
}

/*
 * int
 * sd01alloc_wkamp(struct disk *dk, struct partition *partp, 
 * 	struct alts_parttbl *ap)
 *		Allocate a working copy of alternate partition info table
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
int
sd01alloc_wkamp(struct disk *dk, struct partition *partp, 
	struct alts_parttbl *ap)
{
	struct	alts_mempart	*wkamp_p;
	size_t	sz;
	int	mystatus = 1;


	/* allocate the working incore alts partition info */
	if ((wkamp_p=(struct alts_mempart *)kmem_zalloc(
			ALTS_MEMPART_SIZE, KM_NOSLEEP)) == NULL) {
		return (mystatus);
	}

	wkamp_p->ap_tbl_secsiz = byte_to_blksz(ALTS_PARTTBL_SIZE, BLKSZ(dk));
	if ((wkamp_p->ap_tblp=(struct alts_parttbl *)kmem_alloc(
			wkamp_p->ap_tbl_secsiz, KM_NOSLEEP)) == NULL ) {
		kmem_free(wkamp_p, ALTS_MEMPART_SIZE);
		return (mystatus);
	}

	/* allocate buffer for the alts partition map (sector size)	
	 * ( the disk image bit map )
 	 */
	wkamp_p->ap_map_secsiz = byte_to_blksz(ap->alts_map_len,BLKSZ(dk));
	wkamp_p->ap_map_sectot = wkamp_p->ap_map_secsiz / BLKSZ(dk);
	if ((wkamp_p->ap_mapp=(unchar *)kmem_zalloc(wkamp_p->ap_map_secsiz,
	    KM_NOSLEEP)) == NULL) {
		kmem_free(wkamp_p->ap_tblp, wkamp_p->ap_tbl_secsiz);
		kmem_free(wkamp_p, ALTS_MEMPART_SIZE);
		return(mystatus);    
	}
	
	/* allocate buffer for the incore transformed char map */
	sz = partp->p_size;
	if ((wkamp_p->ap_memmapp = (unchar *)kmem_zalloc(sz, KM_NOSLEEP)) == NULL) {
		kmem_free(wkamp_p->ap_mapp, wkamp_p->ap_map_secsiz);
		kmem_free(wkamp_p->ap_tblp, wkamp_p->ap_tbl_secsiz);
		kmem_free(wkamp_p, ALTS_MEMPART_SIZE);
		return(mystatus);    
	}

	dk->dk_wkamp = wkamp_p;
	dk->dk_wkamp->part = *partp;	/* struct copy			*/

	return(0);
}


/*
 * void
 * sd01free_wkamp(struct disk *dk)
 *	Deallocate the working copy of alternate partition info table
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
void
sd01free_wkamp(struct disk *dk)
{
	struct	alts_mempart *ap;

	if ((ap = dk->dk_wkamp) == NULL) {
		return;
	}

	if (ap->ap_memmapp) {
		kmem_free(ap->ap_memmapp, ap->part.p_size);
		ap->ap_memmapp = NULL;
	}
	if (ap->ap_mapp) {
		kmem_free(ap->ap_mapp, ap->ap_map_secsiz);
		ap->ap_mapp = NULL;
	}
	if (ap->ap_tblp) {
		kmem_free(ap->ap_tblp, ap->ap_tbl_secsiz);
		ap->ap_tblp = NULL;
	}
	if (ap->ap_entp) {
		kmem_free(ap->ap_entp, ap->ap_ent_secsiz);
		ap->ap_entp = NULL;
	}
	kmem_free(ap, ALTS_MEMPART_SIZE);

	dk->dk_wkamp = NULL;

}

/*
 * int
 * sd01alts_assign(unchar memmap[], daddr_t srt_ind, daddr_t end_ind,
 * 	int cnt, int dir)
 *		allocate a range of sectors from the alternate partition
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01alts_assign(unchar memmap[], daddr_t srt_ind, daddr_t end_ind,
	int cnt, int dir)
{
	int	i;
	int	total;
	int	first_ind;

	for (i=srt_ind, first_ind=srt_ind, total=0; i!=end_ind; i+=dir) {
	    	if (memmap[i] == ALTS_BAD) {
			total = 0;
			first_ind = i + dir;
			continue;
	    	}
	    	total++;
	    	if (total == cnt)
			return(first_ind);

	}
	return(NULL);
}



/*
 * int
 * sd01ent_merge(struct alts_ent buf[], struct alts_ent list1[],
 *	int lcnt1, struct alts_ent list2[], int lcnt2)
 *		merging two entry tables into a single table. In addition,
 *		all empty slots in the entry table will be removed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01ent_merge(struct alts_ent buf[], struct alts_ent list1[],
	int lcnt1, struct alts_ent list2[], int lcnt2)
{
	int	i;
	int	j1,j2;
	daddr_t	list1_bad_start, list2_bad_start;

	for (i=0, j1=0, j2=0; j1<lcnt1 && j2<lcnt2;) {

		list1_bad_start = list1[j1].bad_start;
		list2_bad_start = list2[j2].bad_start;

		if (list1_bad_start == ALTS_ENT_EMPTY) {
			j1++;
			continue;	
	    	}
	    	if (list2_bad_start == ALTS_ENT_EMPTY) {
			j2++;
			continue;
	    	}
	    	if (list1_bad_start < list2_bad_start)
			buf[i++] = list1[j1++];
	    	else {
	    		if (list1_bad_start == list2_bad_start)
				j1++;
			buf[i++] = list2[j2++];
		}
	}
	for (; j1<lcnt1; j1++) {
    		if (list1[j1].bad_start == ALTS_ENT_EMPTY) 
			continue;	
    		buf[i++] = list1[j1];
	}
	for (; j2<lcnt2; j2++) {
	    	if (list2[j2].bad_start == ALTS_ENT_EMPTY) 
			continue;	
	    	buf[i++] = list2[j2];
	}
	return (i);
}


/*
 * int
 * sd01asgn_altsctr(struct disk *dk)
 *	assign alternate sectors for bad sector mapping
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01asgn_altsctr(struct disk *dk)
{
	int	i;
	int	j;
	daddr_t	alts_ind;
	int	cluster;
	int	mystatus = 0;

	for (i=0; i<dk->dk_wkamp->ap_tblp->alts_ent_used; i++) {
		if ((dk->dk_wkamp->ap_entp)[i].bad_start == ALTS_ENT_EMPTY)
			continue;
		if ((dk->dk_wkamp->ap_entp)[i].good_start != 0)
			continue;
		cluster = (dk->dk_wkamp->ap_entp)[i].bad_end-
			  (dk->dk_wkamp->ap_entp)[i].bad_start +1; 
		alts_ind = sd01alts_assign(dk->dk_wkamp->ap_memmapp,
				dk->dk_wkamp->part.p_size-1, 
				dk->dk_wkamp->ap_tblp->alts_map_base + 
				dk->dk_wkamp->ap_map_sectot - 1, 
				cluster, ALTS_MAP_DOWN);
		if (alts_ind == NULL) {
			/*
			 *+ The disk driver is unable to allocate
			 *+ alternates for a bad sector.
			 */
			cmn_err(CE_NOTE,"!Disk Driver: can't alloc alts for bad sector %d.\n", 
			(dk->dk_wkamp->ap_entp)[i].bad_start);
			return (mystatus);
		}
		alts_ind = alts_ind - cluster + 1;
		(dk->dk_wkamp->ap_entp)[i].good_start =
				alts_ind +dk->dk_wkamp->part.p_start; 
		/* flag out assigned alternate sectors */
		for (j=0; j<cluster; j++) {
			(dk->dk_wkamp->ap_memmapp)[alts_ind+j] = ALTS_BAD;
		}
	}

	return (0);
}

/*
 * void
 * sd01compress_map(struct alts_mempart *ap)
 *	transform the incore alts char map to the disk image bit map
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
void
sd01compress_map(struct alts_mempart *ap)
{

	int 	i;
	int	bytesz;
	char	mask = 0;
	int	maplen=0;

	for (i=0, bytesz=7; i<ap->part.p_size; i++) {
	    	mask |= ((ap->ap_memmapp)[i] << bytesz--);
	    	if (bytesz < 0) {
			(ap->ap_mapp)[maplen++] = mask;
			bytesz = 7;
			mask = 0;
	    	}
	}
	/*
	 *	if partition size != multiple number of bytes	
	 *	then record the last partial byte			
	 */
	if (bytesz != 7)
	    	(ap->ap_mapp)[maplen] = mask;
	    
}


/*
 * int
 * sd01wr_altsctr(struct disk *dk)
 *	update the new alternate partition tables on disk
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01wr_altsctr(struct disk *dk)
{
	int	mystatus = 1;
	int	status;
	daddr_t	srtsec;

	if (!dk->dk_wkamp || !dk->dk_wkamp->ap_tblp)	{
		return(mystatus);
	}

	srtsec = dk->dk_wkamp->part.p_start;

	switch (dk->hde_state & HDESMASK) {
	case HDEREMAP_0:
		status = sd01icmd(dk,SM_WRITE,
			(uint)srtsec, (char *)dk->dk_wkamp->ap_tblp,
			dk->dk_wkamp->ap_tbl_secsiz, 
			dk->dk_wkamp->ap_tbl_secsiz/BLKSZ(dk),
			SCB_WRITE, sd01intalts);
		if (status)
			/*
			 *+ The disk driver is unable to write the
			 *+ alternate sector partition.
			 */
	    		cmn_err(CE_NOTE,"!Disk Driver: Unable to write alternate sector partition.");
		return(status);

	case HDEREMAP_1:
		status = sd01icmd(dk,SM_WRITE,
			(uint)(srtsec+dk->dk_wkamp->ap_tblp->alts_map_base),
			(char *)dk->dk_wkamp->ap_mapp,dk->dk_wkamp->ap_map_secsiz,
			dk->dk_wkamp->ap_map_secsiz/BLKSZ(dk),
			SCB_WRITE, sd01intalts);
		if (status)
			/*
			 *+ The disk driver is unable to write the
			 *+ alternate sector partition map.
			 */
	    		cmn_err(CE_NOTE,"!Disk Driver: Unable to write alternate partition map.");
		return(status);

	case HDEREMAP_2:
		if (dk->dk_wkamp->ap_tblp->alts_ent_used != 0) {
			status = sd01icmd(dk,SM_WRITE,
				(uint)(srtsec+dk->dk_wkamp->ap_tblp->alts_ent_base),
				(char *)dk->dk_wkamp->ap_entp,
				dk->dk_wkamp->ap_ent_secsiz,
				dk->dk_wkamp->ap_ent_secsiz/BLKSZ(dk),
				SCB_WRITE, sd01intalts);
			if (status)
				/*
				 *+ The disk driver is unable to write the
				 *+ alternate sector entry table.
				 */
	    			cmn_err(CE_NOTE,"!Disk Driver: Unable to write alternate sector entry table.");
			return(status);
		} else {
			return(-1);
		}
	}
	/*
	 *+ the routine to update the new alternate partition tables on disk
	 *+ was called with a bad remap state.
	 */
	cmn_err(CE_NOTE,"!Disk Driver: Unknown REMAP state.");
	return(1);
}

/*
 * void
 * sd01upd_altstbl(struct disk *dk)
 *	Update the incore alternate partition info tables
 *
 * Calling/Exit State:
 *	Disk struct dk_lock held on entry and exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
void
sd01upd_altstbl(struct disk *dk)
{
	if (dk->dk_amp != NULL) {
		if (dk->dk_amp->ap_rcnt == 0) {
			/* release the original alts entry table */
			sd01rel_altsctr (dk->dk_amp);
		} else {
			/* Reference count non-zero, so set data obsolete */
			dk->dk_amp->ap_flag |= ALT_OBSOLETE;
		}
		dk->dk_amp = NULL;
	}

	bcopy((caddr_t)dk->dk_wkamp->ap_tblp, (caddr_t)&(dk->dk_alts_parttbl),
		ALTS_PARTTBL_SIZE);
	kmem_free(dk->dk_wkamp->ap_tblp, dk->dk_wkamp->ap_tbl_secsiz);
	dk->dk_wkamp->ap_tblp = NULL;

	dk->dk_amp = dk->dk_wkamp;
	dk->dk_wkamp = NULL;
}

/* 
 * void
 * sd01intalts(struct sb *sbp)
 *	interrupt handler for bad sector software remap
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct dk_lock.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
void
sd01intalts(struct sb *sbp)
{
	register struct job *jp, *njp;
	register struct disk *dk;
	register struct scb *scb;
	register struct scs *cmd;
	register buf_t *bp;
	int 	part;
	int	status = 1;

	jp  = (struct job *) sbp->SCB.sc_wd;
	sd01untimeout(jp, sbp->SCB.sc_comp_code);
	scb = &jp->j_cont->SCB;
	cmd = &jp->j_cmd.cs;
	dk  = jp->j_dk;

	/* Check if interrupt was due to a Request Sense job */
	if (sbp == dk->dk_fltreq) {

		dk->dk_sense.sd_ba = sdi_swap32(dk->dk_sense.sd_ba);

		/* Check if the Request Sense was ok */
		if (sbp->SCB.sc_comp_code == SDI_ASW) {

			switch(dk->dk_sense.sd_key) {
				case SD_RECOVER:
					switch(dk->dk_sense.sd_sencode) {
				  		case SC_DATASYNCMK:
				  		case SC_RECOVRRD:
				  		case SC_RECOVRRDECC:
				  		case SC_RECOVRIDECC:
							status = 0;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
		} 
#ifdef SD01_DEBUG
	if (sd01alts_debug & HWREA) {
		/*
		 *+
		 */
        	cmn_err(CE_NOTE, "sd01intalts: return from request sense b_flags= 0x%x\n",
			jp->j_bp->b_flags);
	}
#endif
		/* Go check status, and continue accordingly */
		goto sd01alts_cont;
	}
	
	/* Check if Bad Block job completed successfully. */
	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {
#ifdef SD01_DEBUG
	if (sd01alts_debug & HWREA) {
		/*
		 *+
		 */
        	cmn_err(CE_NOTE, "sd01intalts: job completed with error. cnt= 0x%x\n", 
			jp->j_bp->b_error);
	}
#endif
		/* Retry the job */
		if (jp->j_bp->b_error < SD01_RETRYCNT) {
			jp->j_bp->b_error++;	  	 /* Update error count*/
			jp->j_cont->SCB.sc_time = JTIME; /* Reset the job time*/

			sd01timeout(jp);
			if (sdi_icmd(jp->j_cont, KM_NOSLEEP) == SDI_RET_OK)
			{
				return;
			}
			sd01untimeout(jp, SDI_ERROR);
#ifdef DEBUG
			/*
		 	 *+
		 	 */
			cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd33001");
#endif
		} else {
			/* Send a Request Sense job */

			dk->dk_fltreq->sb_type 	    = ISCB_TYPE;
			dk->dk_fltreq->SCB.sc_int   = sd01intalts;
			dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltreq->SCB.sc_link  = 0;
			dk->dk_fltreq->SCB.sc_resid = 0;
			dk->dk_fltreq->SCB.sc_time  = JTIME;
			dk->dk_fltreq->SCB.sc_mode  = SCB_READ;
			dk->dk_fltreq->SCB.sc_dev   = scb->sc_dev;
			dk->dk_fltreq->SCB.sc_wd    = (long) jp;
			dk->dk_fltcmd.ss_op         = SS_REQSEN;
			dk->dk_fltcmd.ss_lun        = cmd->ss_lun;
			dk->dk_fltcmd.ss_addr1      = 0;
			dk->dk_fltcmd.ss_addr       = 0;
			dk->dk_fltcmd.ss_len        = SENSE_SZ;
			dk->dk_fltcmd.ss_cont       = 0;
			dk->dk_sense.sd_key         = SD_NOSENSE; 

			sd01timeout(jp);
			if ((sdi_translate(dk->dk_fltreq, B_READ, NULL,
				KM_NOSLEEP) == SDI_RET_OK) &&
			   (sdi_icmd(dk->dk_fltreq, KM_NOSLEEP) == SDI_RET_OK))
			{
				return;
			}
			sd01untimeout(jp, SDI_ERROR);
#ifdef DEBUG
			cmn_err(CE_WARN, "Disk Driver: Bad type to HA Err: 8dd33002");
#endif
		}
	} else {
		status = 0;
	}

	if (!status && ((dk->hde_state & HDESMASK) != HDEREMAP_RSTR)) {

		/* Good status and we are not done! */

		if (dk->dk_remapalts == sd01remap_altsctr) {
			++dk->hde_state;
			if ((dk->hde_state & HDESMASK) != HDEREMAP_DONE) {
				status = sd01wr_altsctr(dk);
				if (status == 0) 
					return;
				if (status != -1) 
					goto sd01alts_cont;
			}
			/* All writes completed, now finish the remap */

			SD01_DK_LOCK(dk);
			sd01upd_altstbl(dk);
		} else {
			/* The write completed, now finish the remap */

			SD01_DK_LOCK(dk);
			sd01upd_altsec(dk);
		}
		sd01setalts_idx(dk);
		SD01_DK_UNLOCK(dk);

		if (dk->dk_datap) {

			/* There is data to restore to new sector */

			struct alts_ent *altp;

			if((altp = sd01_findaltent(dk)) != (struct alts_ent *)NULL){
				/* Alt entry found, update good sector 
				 * with good data */

				dk->dk_hdesec = altp->good_start;
				if (!sd01icmd(dk,SM_WRITE,dk->dk_hdesec,
					dk->dk_datap, BLKSZ(dk),1,SCB_WRITE,
					sd01intalts)) {
					dk->hde_state &= ~HDESMASK;
					dk->hde_state |= HDEREMAP_RSTR;
					return;
				}
			}
			status = -1;
		}
	}
sd01alts_cont:
	if(status)
		cmn_err(CE_CONT, "!Disk Driver: Unable to software remap, state 0x%x, status %d.", (dk->hde_state & HDESMASK), status);

	sd01_bbremap(dk, SD01_REMAPEND, status);

#ifdef SD01_DEBUG
	if (sd01alts_debug & HWREA) {
		/*
		 *+
		 */
        	cmn_err(CE_NOTE, "sd01intalts: return b_flags=0x%x\n", jp->j_bp->b_flags);
	}
#endif
	return;
}

/*
 * struct alts_ent *
 * sd01_findaltent(struct disk *dk)
 *
 *	Find the interim alternate entry in the alternate table
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
struct alts_ent *
sd01_findaltent(struct disk *dk)
{
        minor_t min;
        int i, part, alts_used;
        struct alts_ent *altp;
        min = dk->dk_addr.sa_minor;

        part = DKSLICE(min);
        alts_used = dk->dk_altcount[part];
        altp = (struct alts_ent *)dk->dk_firstalt[part];
        i = sd01bsearch(altp, alts_used, dk->dk_hdesec, DK_FIND_BIG);
        altp += i;
        if ((i != -1) && (dk->dk_hdesec == altp->bad_start))
                return(altp);
        return(0);

}
/*
 * int
 * sd01ck_gbad(struct disk *dk)
 *	checking growing bad sectors in ALTSCTR partition
 *	and sectors that may have already been remapped
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01ck_gbad(struct disk *dk)
{
	daddr_t	badsec;
	daddr_t	altsp_srtsec;
	daddr_t	altsp_endsec;
	int	cnt;
	int	status;
	int	mystatus = 1;
	struct alts_ent *altp;

	altsp_srtsec = dk->dk_wkamp->part.p_start;
	altsp_endsec = dk->dk_wkamp->part.p_start +
		       dk->dk_wkamp->part.p_size - 1;

	for (cnt=0; cnt < dk->dk_wkamp->ap_gbadcnt; cnt++) {
	    badsec = (dk->dk_wkamp->ap_gbadp)[cnt].bad_start;

	    /* check if bad sector is within the ATLSCTR partition */
	    if ((badsec >= altsp_srtsec) && (badsec <= altsp_endsec)) {

		/* check for assigned alternate sector */
		if ((dk->dk_wkamp->ap_memmapp)[badsec - altsp_srtsec]==ALTS_BAD) {

		    	status = sd01ck_badalts(dk,cnt);
			if (status)
				return (mystatus);

		} else {	/* non-assigned alternate sector	*/
		    if ((badsec >= altsp_srtsec) && (badsec <= (altsp_srtsec +
			dk->dk_wkamp->ap_tbl_secsiz / (uint)BLKSZ(dk) - 1))) {
			/*
			 *+ The disk driver's alternates partition table
			 *+ is bad.
			 */
		    	cmn_err(CE_NOTE,"!Disk Driver: Alt partition table is bad.");
		    	return (mystatus);
	    	    }
		    if ((badsec >= altsp_srtsec+dk->dk_wkamp->ap_tblp->alts_map_base) && 
			(badsec <= (altsp_srtsec + dk->dk_wkamp->ap_tblp->alts_map_base +
			   dk->dk_wkamp->ap_map_sectot - 1))) {
			/*
			 *+ The disk driver's alternates partition map
			 *+ is bad.
			 */
		    	cmn_err(CE_NOTE, "!Disk Driver: Alt part map is bad.");
		    	return (mystatus);
	    	    }
		    if ((badsec >= altsp_srtsec+dk->dk_wkamp->ap_tblp->alts_ent_base) && 
			(badsec <= (altsp_srtsec + dk->dk_wkamp->ap_tblp->alts_ent_base +
			dk->dk_wkamp->ap_ent_secsiz / (uint)BLKSZ(dk) - 1))) {
			/*
			 *+ The disk driver's alternates partition entry
			 *+ is bad.
			 */
		    	cmn_err(CE_NOTE, "!Disk Driver: Alt partition entry is bad.");
		    	return(mystatus);
	    	    }

		    (dk->dk_wkamp->ap_memmapp)[badsec - altsp_srtsec] = ALTS_BAD;
	            (dk->dk_wkamp->ap_gbadp)[cnt].bad_start = ALTS_ENT_EMPTY;

		}
	    } else {
		/* binary search for bad sector in the alts entry table */
		status = sd01bsearch(dk->dk_wkamp->ap_entp, 
				(int) dk->dk_wkamp->ap_tblp->alts_ent_used,
				(dk->dk_wkamp->ap_gbadp)[cnt].bad_start, 0);
		/*
		 * if the bad sector had already been remapped
		 * (found in alts_entry) and it is not an interim entry,
		 * then ignore the bad sector
		 */
		if (status != -1) {
		    altp = dk->dk_wkamp->ap_entp + status;
		    if(altp->bad_start != altp->good_start)
			(dk->dk_wkamp->ap_gbadp)[cnt].bad_start = ALTS_ENT_EMPTY;
		}
	    }
	}

	return (0);
}



/*
 * int
 * sd01ck_badalts(struct disk *dk, int gidx)
 *	check for bad sector in assigned alternate sectors
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01ck_badalts(struct disk *dk, int gidx)
{
	daddr_t	badsec;
	int	i;
	int	j;
	daddr_t	numsec;
	int	cnt;
	daddr_t intv[3];
	struct	alts_ent ae[3];
	struct	alts_ent *atmp_entp;
	int	atmp_ent_secsiz;
	int	mystatus = 1;

	cnt = dk->dk_wkamp->ap_tblp->alts_ent_used;
	badsec = (dk->dk_wkamp->ap_gbadp)[gidx].bad_start;

	for (i=0; i<cnt; i++) {
	    numsec = (dk->dk_wkamp->ap_entp)[i].bad_end - 
			(dk->dk_wkamp->ap_entp)[i].bad_start;
	    if ((badsec >= (dk->dk_wkamp->ap_entp)[i].good_start) &&
		(badsec <= ((dk->dk_wkamp->ap_entp)[i].good_start + numsec))) {
		/*
		 *+ An assigned alternates sector is bad.
		 */
		cmn_err(CE_NOTE,
		"!Disk Driver: Bad sector %ld is an assigned alt", badsec);

		/* if the assigned alternate sector is mapped to 
		 * a single bad sector , then reassign the mapped bad sector
		 */ 
		if (!numsec) {
        		(dk->dk_wkamp->ap_gbadp)[gidx].bad_start = ALTS_ENT_EMPTY;
			(dk->dk_wkamp->ap_entp)[i].good_start = 0;
			return(0);
		}

		intv[0] = badsec - (dk->dk_wkamp->ap_entp)[i].good_start;
		intv[1] = 1;
		intv[2] = (dk->dk_wkamp->ap_entp)[i].good_start + numsec - badsec;

		if (intv[0]) {
			ae[0].bad_start = (dk->dk_wkamp->ap_entp)[i].bad_start;
			ae[0].bad_end = ae[0].bad_start + intv[0] -1;
			ae[0].good_start = (dk->dk_wkamp->ap_entp)[i].good_start;
		}

		ae[1].bad_start = (dk->dk_wkamp->ap_entp)[i].bad_start + intv[0];
		ae[1].bad_end = ae[1].bad_start;
		ae[1].good_start = 0;

		if (intv[2]) {
			ae[2].bad_start = ae[1].bad_end + 1;
			ae[2].bad_end = ae[2].bad_start + intv[2] -1;
			ae[2].good_start = (dk->dk_wkamp->ap_entp)[i].good_start+
						intv[0] + intv[1];
		}

		/* swap in the original bad sector */
        	(dk->dk_wkamp->ap_gbadp)[gidx] = ae[1];

		/* check for bad sec at beginning or end of the cluster */
		if (!intv[0] || !intv[2]) {
			if (!intv[0])
		    		(dk->dk_wkamp->ap_entp)[i] = ae[2];
			else
		    		(dk->dk_wkamp->ap_entp)[i] = ae[0];
			return(0);
		}

		/* allocate an expanded alternate entry table */
	        atmp_ent_secsiz = byte_to_blksz(((cnt+1)*ALTS_ENT_SIZE),
				BLKSZ(dk));

	        if ((atmp_entp = (struct alts_ent *)kmem_alloc(atmp_ent_secsiz,
		     KM_NOSLEEP)) == NULL) {
			/*
			 *+ The disk driver is unable to allocate
			 *+ alternates entry table.
			 */
			cmn_err(CE_NOTE,
			"!Disk Driver: Unable to alloc alternate entry table.");
			return (mystatus);
	    	}

		for (j=0; j<i; j++)
			atmp_entp[j] = (dk->dk_wkamp->ap_entp)[j];
		atmp_entp[i] = ae[0];
		atmp_entp[i+1] = ae[2];
		for (j=i+1; j<cnt; j++)
			atmp_entp[j+1] = (dk->dk_wkamp->ap_entp)[j];

		kmem_free(dk->dk_wkamp->ap_entp, dk->dk_wkamp->ap_ent_secsiz);
		dk->dk_wkamp->ap_tblp->alts_ent_used++;
		dk->dk_wkamp->ap_ent_secsiz = atmp_ent_secsiz; 
		dk->dk_wkamp->ap_entp = atmp_entp;

		return (0);
	    } 
	}

	/* the bad sector has already been identified as bad */
        (dk->dk_wkamp->ap_gbadp)[gidx].bad_start = ALTS_ENT_EMPTY;

	/*
	 *+ found a bad sector that has already been identified as bad
	 */
	cmn_err(CE_NOTE,
	"!Disk Driver: hitting a bad sector that has been identified as bad.");
	return(0);
}


/*
 * int
 * sd01ast_init(struct disk *dk)
 *
 * Allocate for 4.0 style alternates.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
int
sd01ast_init(struct disk *dk)
{
	void	*tp;
	int	mystatus = 1;

	if (!dk->dk_ast) {
		if ((tp = (struct altsectbl *)kmem_zalloc(ALTSECTBL_SIZE,
		     KM_NOSLEEP)) == NULL)	{
			return (mystatus);
		}

		dk->dk_ast = tp;
	}

	dk->dk_ast->ast_altsiz = byte_to_blksz(dk->dk_pdsec.alt_len,BLKSZ(dk));

	/* allocate storage for disk image alternates info table */
	tp = (struct alt_info *)kmem_alloc(dk->dk_ast->ast_altsiz, KM_NOSLEEP);

	dk->dk_ast->ast_alttblp = tp;

	if (tp == NULL)
		return(mystatus);

	return (0);
}

/*
 * void
 * sd01ast_free(struct disk *dk)
 *
 * Free allocation for 4.0 style alternates.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 *	or in DKVTOC state (i.e. disk is in process of being opened).
 */
void
sd01ast_free(struct disk *dk)
{

	if (!dk->dk_ast)	{
		return; 
	}

	if (dk->dk_ast->ast_alttblp)  {
		kmem_free(dk->dk_ast->ast_alttblp, dk->dk_ast->ast_altsiz);
		dk->dk_ast->ast_alttblp = NULL;
	}

	if (dk->dk_ast->ast_entp)  {
		kmem_free(dk->dk_ast->ast_entp, byte_to_blksz(
			dk->dk_ast->ast_entused*ALTS_ENT_SIZE,BLKSZ(dk)));
		dk->dk_ast->ast_entp = NULL;
	}

	bzero((caddr_t)dk->dk_ast, ALTSECTBL_SIZE);

}


/*
 * int
 * sd01remap_altsec(struct disk *dk)
 *
 * Remap for 4.0 style alternates.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01remap_altsec(struct disk *dk)
{
	int	status;
	int	mystatus = 1;
	daddr_t	srtsec;
	size_t	ts;
	void	*tp;

	/* check for reserved alternate space */
	if (dk->dk_alttbl->alt_sec.alt_reserved == 0) {
		/*
		 *+ No alternate table in VTOC
		 */
		cmn_err(CE_NOTE, "!Disk Driver: No alternate table in VTOC");
		return(mystatus);
	}

	if (sd01ast_init(dk))
		return(mystatus);

	bcopy((caddr_t)dk->dk_alttbl,(caddr_t)dk->dk_ast->ast_alttblp, 
		dk->dk_pdsec.alt_len);

	if (((dk->dk_ast->ast_gbadp = dk->dk_gbadsec_p)==NULL) ||
	    ((dk->dk_ast->ast_gbadcnt = dk->dk_gbadsec_cnt)==0)) {
		sd01ast_free(dk);
		return(mystatus);
	}

	if (status = sd01asgn_altsec(dk)) {
		sd01ast_free(dk);
		return(mystatus);
	}

	dk->dk_ast->ast_entused = dk->dk_ast->ast_alttblp->alt_trk.alt_used 
			+ dk->dk_ast->ast_alttblp->alt_sec.alt_used;

	if (!dk->dk_ast->ast_entused) {
		dk->dk_ast->ast_entp = (struct alts_ent *)NULL;
	} else {
		/* allocate storage for common incore alternate entry tbl */
		ts =  byte_to_blksz((ALTS_ENT_SIZE * dk->dk_ast->ast_entused),
		      BLKSZ(dk));

		if ( !(tp = (struct alts_ent *)kmem_alloc(ts, KM_NOSLEEP)) ) {
			sd01ast_free(dk);
			return(mystatus);
		}
		dk->dk_ast->ast_entp = tp;

		/* translate AT&T altsec format to common entry table */
		sd01xalt(dk,dk->dk_ast->ast_alttblp, dk->dk_ast->ast_entp);
	}

	/* write new altsec info back to disk */
	srtsec = dk->unixst + (dk->dk_pdsec.alt_ptr >> BLKSHF(dk));

	status = sd01icmd(dk,SM_WRITE,(uint) srtsec,
			(char *) dk->dk_ast->ast_alttblp,dk->dk_ast->ast_altsiz,
			dk->dk_ast->ast_altsiz/BLKSZ(dk),SCB_WRITE, sd01intalts);

	if (status) {
	    	/*
		 *+ The disk driver can't write alternate table.
		 */
	    	cmn_err(CE_NOTE,"!Disk Driver: can't write alternate table");
		sd01ast_free(dk);
		return(mystatus);
	}

	return (0);
}

/*
 * int
 * sd01asgn_altsec(struct disk *dk)
 *	assign bad sectors based on the AT&T mapping scheme
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	In DKMAPBAD state (i.e. in the bad block single thread state.
 *		dk_hdestate, dk_jp_orig, dk_datap, dk_hdesec are all
 *		protected by this state.)
 */
int
sd01asgn_altsec(struct disk *dk)
{
	struct	alts_ent *growbadp;
	daddr_t curbad;
	int	cnt;
	int	i;
	struct	alt_info *alttblp = dk->dk_ast->ast_alttblp;
	int	chgflg = 0;
	int	mystatus = 1;

	growbadp=dk->dk_ast->ast_gbadp;
	for (cnt=0; cnt<dk->dk_ast->ast_gbadcnt; cnt++) {
		curbad = growbadp[cnt].bad_start;
		if (curbad == ALTS_ENT_EMPTY)
			continue;
		if (growbadp[cnt].good_start == curbad)
			continue;  /* This is an interim entry that will not
				    * be put into the 4.0 alt disk table */

		/* search through the bad sector entry table */
		for (i=0; i < (int)alttblp->alt_sec.alt_used;i++) {
			/* check for any assigned alternate is bad */
			if (alttblp->alt_sec.alt_base + i == curbad) {
				/* skip if it is a known bad sector */
				if (alttblp->alt_sec.alt_bad[i] == -1) {
					growbadp[cnt].bad_start=ALTS_ENT_EMPTY;
					break;
				}

				/*
				 *+ An assigned alternate is bad.
				 */
				cmn_err(CE_NOTE,"!Disk Driver: Bad sector %ld is an assigned alternate for sector %ld\n",
				    curbad, alttblp->alt_sec.alt_bad[i]);
				/* swap growing bad sector to become
				 * original mapped bad sector
				 */
				growbadp[cnt].bad_start =
					alttblp->alt_sec.alt_bad[i];
				alttblp->alt_sec.alt_bad[i] = -1;
				chgflg++;
				break;
			}

			/* check if bad block already mapped  */
			if (alttblp->alt_sec.alt_bad[i] == curbad) {
				growbadp[cnt].bad_start = ALTS_ENT_EMPTY;
				break;
			}
		}

		curbad = growbadp[cnt].bad_start;
		if (curbad == ALTS_ENT_EMPTY)
			continue;

		/* if unused alternate is bad, then excise it from the list. */
		for (i = alttblp->alt_sec.alt_used; i < (int)alttblp->alt_sec.alt_reserved; i++) {
			if (alttblp->alt_sec.alt_base + i == curbad) {
				alttblp->alt_sec.alt_bad[i] = -1;
				growbadp[cnt].bad_start = ALTS_ENT_EMPTY;
				chgflg++;
				break;
			}
		}

		curbad = growbadp[cnt].bad_start;
		if (curbad == ALTS_ENT_EMPTY)
			continue;

		/* make sure alt_used is an index to an available 
		 * alternate entry for remapped
		 */
		while (alttblp->alt_sec.alt_used<alttblp->alt_sec.alt_reserved){
			if (alttblp->alt_sec.alt_bad[alttblp->alt_sec.alt_used]
			    != -1)
				break;
			alttblp->alt_sec.alt_used++;
		}

		/* check for reserved table overflow */
		if (alttblp->alt_sec.alt_used >= alttblp->alt_sec.alt_reserved){
			/*
			 *+ The disk driver has too few alternates for
			 *+ this sector.
			 */
			cmn_err(CE_NOTE, "!Disk Driver: Insufficient alts for sect %ld!\n", curbad);
			if (chgflg)
				return(0);
			else
				return(mystatus);
		}

		alttblp->alt_sec.alt_bad[alttblp->alt_sec.alt_used] = curbad;
		alttblp->alt_sec.alt_used++;
		chgflg++;
	}

	return (0);
}

/*
 * void
 * sd01isfb(struct disk *dk, ulong func)
 *
 * Perform the function, suspend queue or flush queue.
 *
 * Calling/Exit State:
 *	Disk struct's dk_lock held on entry and return, but
 *	may be dropped in middle.
 */
void
sd01isfb(struct disk *dk, ulong func)
{
	switch (func) {
		case SFB_FLUSHR:
			if (dk->dk_state & DKFLUSHQ)
				return;
			dk->dk_state |= DKFLUSHQ;
			break;
		case SFB_SUSPEND:
			if (dk->dk_state & DKSUSP)
				return;
			dk->dk_state |= DKSUSP_WAIT;
			break;
		default:
			return;
	}

	SD01_DK_UNLOCK(dk);

	dk->dk_spcount = 0;	/* Reset special count 			*/
	dk->dk_fltfqblk->SFB.sf_dev = dk->dk_addr;
	dk->dk_fltfqblk->SFB.sf_int = sd01intfq;
	dk->dk_fltfqblk->SFB.sf_func = func;


	if (sdi_icmd(dk->dk_fltfqblk, KM_SLEEP) != SDI_RET_OK) {
		SD01_DK_LOCK(dk);
#ifdef DEBUG
		cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
		switch (func) {
			case SFB_FLUSHR:
				dk->dk_state &= ~DKFLUSHQ;
				break;
			case SFB_SUSPEND:
				dk->dk_state &= ~DKSUSP_WAIT;
				break;
		}
		return;
	}
	else	{
		SD01_DK_LOCK(dk);
	}

#ifdef SD01_DEBUG
	if(sd01alts_debug & HWREA) {
		cmn_err(CE_CONT,
		"sd01isfb: WAIT: dk_state= 0x%x\n", dk->dk_state);
	}
#endif
	switch (func) {
		case SFB_FLUSHR:
			while (dk->dk_state & DKFLUSHQ)	{
				SV_WAIT(dk->dk_sv, pridisk, dk->dk_lock);
				SD01_DK_LOCK(dk);
			}
			break;
		case SFB_SUSPEND:
			while (dk->dk_state & DKSUSP_WAIT)	{
				SV_WAIT(dk->dk_sv, pridisk, dk->dk_lock);
				SD01_DK_LOCK(dk);
			}
			break;
	}
}



/*
 * void
 * sd01intfq(struct sb *sbp)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01intfq(struct sb *sbp)
{
	register struct disk *dk;
	dev_t dev;			/* External device number	*/

	dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
	dk  = DKPTR(geteminor(dev));
	sd01untimeout((struct job *)sbp->SCB.sc_wd, sbp->SFB.sf_comp_code);

	if (sbp->SFB.sf_comp_code & SDI_RETRY && dk->dk_spcount<SD01_RETRYCNT) {
		/* Retry the function request  */
		dk->dk_spcount++;
		dk->dk_error++;
		sd01logerr(sbp, dk, 0x4dd0f001);

		if (sdi_icmd(sbp, KM_SLEEP) == SDI_RET_OK) 
			return;
#ifdef DEBUG
		else {	
			cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
		}
#endif
	} else {
		if (sbp->SFB.sf_comp_code != SDI_ASW) {
			sd01logerr(sbp, dk, 0x6dd0f003);
		}

		dk->dk_spcount = 0;	/* Zero retry count 		*/
	}
	SD01_DK_LOCK(dk);
	switch (sbp->SFB.sf_func) {
		case SFB_FLUSHR:
			if (dk->dk_state & DKFLUSHQ) {
				dk->dk_state &= ~DKFLUSHQ;
				SD01_DK_UNLOCK(dk);
				SV_BROADCAST(dk->dk_sv, 0);
			}
			break;
		case SFB_SUSPEND:
			if (sbp->SFB.sf_comp_code == SDI_ASW) 
				dk->dk_state |= DKSUSP;
			if (dk->dk_state & DKSUSP_WAIT) {
				dk->dk_state &= ~DKSUSP_WAIT;
				SD01_DK_UNLOCK(dk);
				SV_BROADCAST(dk->dk_sv, 0);
			}
			break;
		default:
			SD01_DK_UNLOCK(dk);
			return;
	}

#ifdef SD01_DEBUG
	if(sd01alts_debug & HWREA) {
		cmn_err(CE_CONT,"sd01intfq: dk_state= 0x%x\n", dk->dk_state);
	}
#endif
}

/*
 * int
 * sd01alt_badsec(struct disk *dk, struct job *diskhead, ushort partition,
 *	int sleepflag)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *	Synchronizes the reading of the alternate table with the bad block
 *		writing of alternates by incrementing/decrementing reference
 *		count in alts_mempart, ap_rcnt.  Both ap_rcnt and ap_flags
 *		are protected by dk_lock.
 *	
 */
int
sd01alt_badsec(struct disk *dk, struct job *diskhead, ushort partition,
	int sleepflag)
{
	register struct alts_ent *altp;
	register struct job *curdisk = diskhead;
	register struct job *jp;   	
	int  	alts_used;
	ushort  secsiz;
	daddr_t lastsec;        	   /* last sec of section 	*/
	int  	i;
	struct alts_mempart *ap;
	int	ret = 0;

#ifdef SD01_DEBUG
	uint  flag = 0;
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,
		"Entering sd01alt_badsec, partition= %d, dk= 0x%x curdisk= 0x%x\n",
		partition, dk, curdisk);
	}
	if(sd01alts_debug & DCLR) {
		cmn_err(CE_CONT,
		"sd01alt_badsec: sector= %d count= %d mem= 0x%x\n",
		sjq_daddr(curdisk), curdisk->j_seccnt, sjq_memaddr(curdisk));
	}
#endif

	/* The dk->dk_amp->ap_rcnt is being used as a reference count on
	 * the alternate table.  The bad block routines depend on this
	 * count reflecting the correct number of read references.
	 * Pull the dk->dk_amp pointer out of the dk, because there is
	 * no guarantee that it will still be there at the end of this
	 * function, since a new alternate table may have been created
	 * by the bad block routines to handle an intervening fault.
	 */
	SD01_DK_LOCK(dk);
	ap = dk->dk_amp;
	ap->ap_rcnt++;

	secsiz = BLKSZ(dk);
	alts_used = dk->dk_altcount[partition];
	altp = (struct alts_ent *)dk->dk_firstalt[partition];

	lastsec = sjq_daddr(curdisk) + ((daddr_t)(curdisk->j_seccnt) - 1);
	/*
	 * binary search for the largest bad sector index in the alternate
	 * entry table which overlaps or larger than the starting 
	 * sjq_daddr(curdisk)
	 */
	i = sd01bsearch(altp, alts_used, sjq_daddr(curdisk), DK_FIND_BIG);
	SD01_DK_UNLOCK(dk);

	/* if starting sector is > the largest bad sector, return  */
	if (i == -1)
		return (ret);
	/* i is the starting index, and set altp to the starting entry addr*/
	altp += i;

	for (; i<alts_used; ) {
		/* CASE 1: */
		while (lastsec < altp->bad_start) { 
			curdisk = curdisk->j_fwmate;
			if (curdisk)
				lastsec = sjq_daddr(curdisk) +
					  ((daddr_t)(curdisk->j_seccnt) - 1);
			else break;
		}
		if (!curdisk) break;    

		/* CASE 3: */
		if (sjq_daddr(curdisk) > altp->bad_end) {
			i++;
			altp++;
			continue;      
		}

#ifdef SD01_DEBUG
		if(sd01alts_debug & DXALT) {
			flag = 1;
			cmn_err(CE_CONT,
			"sd01alt_badsec: sector= %d count= %d mem= 0x%x\n",
			sjq_daddr(curdisk), curdisk->j_seccnt,
			sjq_memaddr(curdisk));
		}
#endif
		/* CASE 2 and 7: */
		if ((sjq_daddr(curdisk) >= altp->bad_start) &&
		    (lastsec <= altp->bad_end)) {       
#ifdef SD01_DEBUG
			if(sd01alts_debug & DCLR) {
				cmn_err(CE_CONT,
				"sd01alt_badsec: CASE 2 & 7 \n");
			}
#endif
			set_sjq_daddr (curdisk, (altp->good_start + 
				sjq_daddr(curdisk) - altp->bad_start));
			if (altp->bad_start == altp->good_start)
				curdisk->j_flags |= J_BADBLK;
			curdisk=curdisk->j_fwmate;
			if (curdisk) {
				lastsec = sjq_daddr(curdisk) +
					  ((daddr_t)(curdisk->j_seccnt) - 1);
				continue;       /* do remaining sects 	*/
			}
			else    break;  /* that's all for this guy. 	*/
		}

		/* at least one bad sector in our section.  break it.  */
		if ((jp = sd01getjob(dk, sleepflag)) == NULL) {
			ret = 1;
			break;
		}
		jp->j_fwmate = curdisk->j_fwmate;
		jp->j_bkmate = curdisk;
		if (curdisk->j_fwmate)
			curdisk->j_fwmate->j_bkmate = jp;
		curdisk->j_fwmate = jp;
		/* CASE 6: */
		if ((sjq_daddr(curdisk) <= altp->bad_end) &&
		    (sjq_daddr(curdisk) >= altp->bad_start)) {       
#ifdef SD01_DEBUG
			if(sd01alts_debug & DCLR) {
				cmn_err(CE_CONT,"sd01alt_badsec: CASE 6 \n");
			}
#endif
			jp->j_seccnt = curdisk->j_seccnt 
				- (altp->bad_end - sjq_daddr(curdisk) + 1); 
			curdisk->j_seccnt -= jp->j_seccnt;
			set_sjq_daddr (jp, altp->bad_end + 1);
			set_sjq_memaddr(jp, ((int)sjq_memaddr(curdisk) +
					curdisk->j_seccnt * secsiz));
			set_sjq_daddr (curdisk, (altp->good_start +
				sjq_daddr(curdisk) - altp->bad_start));
			if (altp->bad_start == altp->good_start)
				curdisk->j_flags |= J_BADBLK;
			curdisk = jp;
			continue;       /* check rest of section 	*/
		}

		/* CASE 5: */
		if ((lastsec >= altp->bad_start) && (lastsec <=altp->bad_end)) {
#ifdef SD01_DEBUG
			if(sd01alts_debug & DCLR) {
				cmn_err(CE_CONT,"sd01alt_badsec: CASE 5 \n");
			}
#endif
			jp->j_seccnt = lastsec - altp->bad_start + 1;
			curdisk->j_seccnt -= jp->j_seccnt;
			set_sjq_daddr (jp, altp->good_start);
			if (altp->bad_start == altp->good_start)
				jp->j_flags |= J_BADBLK;
			set_sjq_memaddr (jp, ((int)sjq_memaddr(curdisk) +
					 curdisk->j_seccnt*secsiz));
			curdisk=jp->j_fwmate;
			if (curdisk) {
				lastsec = sjq_daddr(curdisk) +
					  ((daddr_t)(curdisk->j_seccnt) - 1);
				continue;       /* do remaining sects 	*/
			} else    break;  /* that's all for this guy. 	*/
		}
		/* CASE 4: */
#ifdef SD01_DEBUG
		if(sd01alts_debug & DCLR) {
			cmn_err(CE_CONT,"sd01alt_badsec: CASE 4\n");
		}
#endif
		jp->j_seccnt = altp->bad_end - altp->bad_start + 1;
		curdisk->j_seccnt = altp->bad_start - sjq_daddr(curdisk);
		set_sjq_memaddr (jp, ((int)sjq_memaddr(curdisk) +
				 curdisk->j_seccnt * secsiz));
		set_sjq_daddr (jp, altp->good_start);
		if (altp->bad_start == altp->good_start)
			jp->j_flags |= J_BADBLK;
		curdisk = jp;
		if ((jp = sd01getjob(dk, sleepflag)) == NULL) {
			ret = 1;
			break;
		}
		jp->j_fwmate = curdisk->j_fwmate;
		jp->j_bkmate = curdisk;
		if (curdisk->j_fwmate)
			curdisk->j_fwmate->j_bkmate = jp;
		curdisk->j_fwmate = jp;
		jp->j_seccnt = lastsec - altp->bad_end;
		set_sjq_daddr (jp, altp->bad_end + 1);
		set_sjq_memaddr (jp, ((int)sjq_memaddr(curdisk) +
				curdisk->j_seccnt * secsiz));
		curdisk = jp; /* continue with rest of section 		*/
	}
	SD01_DK_LOCK(dk);
	if (--ap->ap_rcnt == 0 && (ap->ap_flag & ALT_OBSOLETE)) {
		/* There are no more references and the data is obsolete
		 * so remove it now.
		 */
		sd01rel_altsctr (ap);
        }
	SD01_DK_UNLOCK(dk);

#ifdef SD01_DEBUG
	if(sd01alts_debug & DCLR) {
		for (i=0,jp=diskhead; flag&&jp; i++,jp=jp->j_fwmate) {
			cmn_err(CE_CONT,"sd01alt_badsec: [%d]", i);
			cmn_err(CE_CONT,
			" sector= %d count= %d mem= %x\n", sjq_daddr(jp), 
			jp->j_seccnt, sjq_memaddr(jp));
		}
	}
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Leaving sd01alt_badsec\n");
	}
#endif
	return (ret);
}
