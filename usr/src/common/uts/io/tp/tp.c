/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/tp/tp.c	1.12"
#ident  "$Header: $"

#include <util/types.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/cred.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <io/ttydev.h>
#include <io/termios.h>
#include <io/termio.h>
#include <io/strtty.h>
#include <fs/file.h>
#include <util/cmn_err.h>
#include <io/tp/tp.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>	/* MUST COME LAST */

/*
* external function declarations
*/

/*
 * function prototype definitions
*/

/*
* externally visible routines 
*/

void			tp_disconnect(dev_t );
int			tp_getmajor(major_t *);
int			tpstart();
			
/*
* TP private routines
*/

STATIC void		tp_admioctl(queue_t *, mblk_t *);
STATIC struct tpchan	*tp_allocchan(ulong );
STATIC struct tpdev	*tp_allocdev(dev_t );
STATIC mblk_t		*tp_allocioctlmsg(int , size_t );
STATIC int		tp_conncons(int );
STATIC int		tp_conndata(int , struct tp_info *);
STATIC void		tp_consioctl(queue_t *, mblk_t *);
STATIC int		tp_consset(struct tp_info *, struct tpdev **);
STATIC void		tp_ctrlioctl(queue_t *, mblk_t *);
STATIC void		tp_dataioctl(queue_t *, mblk_t *);
STATIC int		tp_disccons();
STATIC void		tp_discdata(struct tpdev *);
STATIC void		tp_discinput(struct tpdev *);
STATIC pl_t		tp_doputnext_low(struct tpdev *, pl_t, int);
STATIC pl_t		tp_doputnextctl1_upper(struct tpchan *, pl_t , int );
STATIC void		tp_fillinfo(struct tpchan *, struct tpdev *, struct tp_info *, int );
STATIC struct tpchan	*tp_findchan(minor_t );
STATIC struct tpdev	*tp_finddev(dev_t , int );
STATIC void		tp_freechan(struct tpchan *);
STATIC void		tp_freedev(struct tpdev *);
STATIC int		tp_growlist(int *, caddr_t *);
STATIC void		tp_hookchan(struct tpchan *, queue_t *);
STATIC int		tp_ioctlok(struct tpchan *, cred_t *);
STATIC void		tp_lctrlioctl(queue_t *, mblk_t *);
STATIC void		tp_newmsgs(struct tpchan *);
STATIC void		tp_putsak(struct tpdev *, struct sak *);
STATIC void		tp_resetcons(struct tpdev *);
STATIC void		tp_senddiscioctl(struct tpdev *);
STATIC int		tp_sendioctl(struct tpdev *, struct tpchan *, queue_t *, mblk_t *);
STATIC pl_t		tp_sendmsg(struct tpchan *, queue_t *, int , struct tpdev *, pl_t );
STATIC pl_t		tp_sendmsg_trl(struct tpchan *, queue_t *, int , struct tpdev *, pl_t );
STATIC void		tp_setcons(struct tpdev *);
STATIC void		tp_setsak(struct tpchan *, struct tp_info *);
STATIC int		tp_setupcons(struct tpchan *, queue_t *, pl_t );
STATIC int		tp_setupctrl(struct tpchan *, queue_t *, pl_t );
STATIC int		tp_setupdata(struct tpchan *, queue_t *, pl_t );
STATIC int		tp_stdioctl(int , mblk_t *, struct tpchan *);
STATIC int		tp_verifysak(struct tp_info *);
STATIC void		tp_wakeall(struct tpdev *);
STATIC int		tpclose(queue_t *, int , cred_t *);
STATIC int		tplrput(queue_t *, mblk_t *);
STATIC int		tplrsrv(queue_t *);
STATIC int		tplwsrv(queue_t *);
STATIC int		tpopen(queue_t *, dev_t *, int , int , cred_t *);
STATIC int		tpursrv(queue_t *);
STATIC int		tpuwput(queue_t *, mblk_t *);
STATIC int		tpuwsrv(queue_t *);
STATIC void		tp_saktypeswitch(struct tpdev *);



/*
* Initialization of required STREAMS data structures 
*/

/*
* External variables used by streams
*/
STATIC struct module_info tp_minfo = {	/* module/driver information */
	0xbeef,			/* module/driver id number */
	"tp",			/* module/driver name */
	0,			/* min packet size acceptable */
	INFPSZ,			/* max packet size acceptable */
	512,			/* hi-water mark for message flow control */
	128			/* lo-water mark for message flow control */
};

STATIC struct qinit tp_urinit = {	/* Queue info for upper read side */
	NULL,			/* ...put() */
	tpursrv,		/* ...srv() */
	tpopen,			/* ...open() */
	tpclose,		/* ...close() */
	NULL,			/* ...admin() */
	&tp_minfo,		/* module_info */
	NULL			/* stat */
};

STATIC struct qinit tp_uwinit = {	/* Queue info for upper write side */
	tpuwput,		/* put procedure*/
	tpuwsrv,		/* service procedure*/
	NULL,			/* open procedure*/
	NULL,			/* close procedure*/
	NULL,			/* admin procedure*/
	&tp_minfo,		/* module info*/
	NULL			/* stat*/
};

STATIC struct qinit tp_lrinit = {	/* Queue info for lower read side */
	tplrput,		/* put procedure*/
	tplrsrv,		/* service procedure*/
	NULL,			/* open procedure*/
	NULL,			/* close procedure*/
	NULL,			/* admin procedure*/
	&tp_minfo,		/* module info*/
	NULL			/* stat*/
};

STATIC struct qinit tp_lwinit = {	/* Queue info for lower write side */
	NULL,			/* put procedure*/
	tplwsrv,		/* service procedure*/
	NULL,			/* open procedure*/
	NULL,			/* close procedure*/
	NULL,			/* admin procedure*/
	&tp_minfo,		/* module info*/
	NULL			/* stat*/
};

/*
* External variables used by streams
*/
struct streamtab tpinfo = {	/* 
				* saved in cdewsw.  external access point to
				* driver
				*/
	&tp_urinit,
	&tp_uwinit,
	&tp_lrinit,
	&tp_lwinit
};


/*
* tuneable values from tp.cf
*/ 
extern major_t		tp_imaj;		/* TP's internal major number*/
extern int		tp_listallocsz;		/* List expansion chunk size*/
extern clock_t          tp_saktypeDATA_switchdelay; /* Delay time to change saktype to DATA*/
extern struct termios	tp_charmasktermios;	/* Char SAK termios mask*/
extern struct termios	tp_dropmasktermios;	/* Linedrop SAK termios mask*/
extern struct termios	tp_breakmasktermios;	/* Break SAK termios mask*/
extern struct termios	tp_charvalidtermios;	/* Char SAK valid termios*/
extern struct termios	tp_dropvalidtermios;	/* Linedrop SAK valid termios*/
extern struct termios	tp_breakvalidtermios;	/* Break SAK valid termios*/
extern major_t          tp_consoledevmaj;       /* Default Major Device number
                                                * of console
                                                */
extern minor_t          tp_consoledevmin;       /* Default Minor Device number
                                                * of console
                                                */

/*
* Multi threading declarations
*/
#define	TPHIER_PLUMB_RWLCK	1	/*Hierarchy for plumbing read/write*/
#define	TPHIER_TPC		2	/*Hierarchy for tpc_data_mutex*/
#define	TPHIER_TPD		3	/*Hierarchy for tpd_muetx*/
#define	TPHIER_CONSSV		2	/*Hierarchy for tpcons mutex */

#define TPPL	plstr			/*lowest TP priority*/

LKINFO_DECL(tp_plumb_rwlck_lkinfo, "TP:Plumbing rwlock", 0);
LKINFO_DECL(tp_chan_mutex_lkinfo, "TP:Individual Channel Lock", 0);
LKINFO_DECL(tp_dev_mutex_lkinfo, "TP:Individual Device Lock", 0);
LKINFO_DECL(tp_conssv_mutex_lkinfo, "TP:Console SV mutex", 0);


/*
* the plumbing lock. it protects all chan/dev pointers in tpdev and tpchan
* as well as any access through tpdevhead, tpchanhead or tpconsdev;
* it should be held for reading if any pointer is to be used and for
* writing if anything will change. It also protects tpchan_curcnt, 
* tpchan_connid and tpdev_curcnt which are really adjuncts of tpdevhead and
* tpchanhead.
*
* NOTE: in each comment block before a subroutine that needs this lock
* we have noted if the lock must be held for WRITE. If it need not
* be held for WRITE we simply say 'the lock must be held' this means
* that it can be read for READ or WRITE
*/
rwlock_t *tp_plumb_rwlck;


/*
* local external statics, used only in tp.c
*/

STATIC dev_t tp_consoledev;     /* Default Device number of console assigned
                                * from tp_consoledevmaj and tp_consoledevmin in
                                * tpstart().
                                */
STATIC major_t tpemaj;		/* external TP major # calculated by tpstart()*/

STATIC int tpchan_curcnt = 0;	/* Current length of TP channel list*/ 
STATIC int tpchan_connid = 0;	/*
				*  Unique channel ID, moved from static 'connid'
				* in tp_allocchan to be protected by 
				* tpchan_list_rwlck;
				*/

STATIC int tpdev_curcnt = 0;	/* Current length of TP device list*/ 

STATIC struct tpchan **tpchanhead;	/* TP channel list*/

STATIC struct tpdev **tpdevhead;	/* TP device list*/

STATIC struct tpdev *tpconsdev = (struct tpdev *)0;/*the current console MUX*/
STATIC lock_t *tpcons_mutex;		/*lock for tpconsdev. only used for
					* use with SVWAIT, the real lock for
					* tpconsdev is tp_plumb_rwlck,that lock
					* is held to text tpconsdev and then
					* if we need to SV, we get this (if
					* SV_WAIT wokrk with rw_LOCKs we 						* wouldn't have to do this
					*/
STATIC sv_t *tpcons_sv;			/*sync variable for setting of tpconsdev*/






int tpdevflag = D_MP;		/* This is expected by cunix for new DDI
				* say we are multithreaded
				*/




#define	HI16		(0xFFFF0000)
#define	TPINFOSZ	(sizeof(struct tp_info))
#define	DATACONNECTFLAGS (TPINF_DISCONNDATA_ONHANGUP | TPINF_FAILUNLINK_IFDATA)
#define	MSIZ(a)		((a)->b_cont->b_datap->db_lim - (a)->b_cont->b_datap->db_base)
#define	ASCII_7BITMASK	(0x7f)
#define	FINDDEVLINKED (1)
#define FILLINFO_ALL (0)
#define FILLINFO_RESTRICTED (1)
#define MILLITOMICRO(time) ((time) * 1000)

/* loadable module wrapper */
STATIC int tp_load(), tp_unload();
MOD_DRV_WRAPPER(tp, tp_load, tp_unload, NULL, "Loadable Trusted Path");

STATIC int
tp_load(void)
{
	tpstart();
	return (0);
}

STATIC int
tp_unload(void)
{

	int i;
	pl_t	pl_1;

	/*	
	 * search through the lowers to see if all I_PUNLINKs have been 
	 * done. If not return EBUSY since we are not as yet allowed to
	 * unload.
	 */

	pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
	for (i = 0; i < tpdev_curcnt; i++) {
		if (tpdevhead[i]) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			return (EBUSY);
		}
	}

	/*
	 * no need for locks anymore since all uppers are closed, all lowers
	 * gone and since 'I_PUNLINK' of lowers did un-timeouts, all timeouts
	 * are gone therefore no one else can call us.
	 */
	RW_UNLOCK(tp_plumb_rwlck, pl_1);

	/*
	 * free up all that was allocated by tp_start in reverse order.
	 */

	SV_DEALLOC(tpcons_sv);
	LOCK_DEALLOC(tpcons_mutex);
	RW_DEALLOC(tp_plumb_rwlck);
	kmem_free(tpdevhead, tpdev_curcnt * sizeof(struct tpdev *));
	kmem_free(tpchanhead, tpchan_curcnt * sizeof(struct tpchan *));
	tpchan_curcnt = 0;
	tpdev_curcnt = 0;

	return(0);
}

/*
* int
* tpstart()
* 	allocate tpchan and tpdev structure pointers 
*
* Calling/Exit State:
*	 Locks: Enter with: None
*	        Leave with: None
*	        Sets:       allocates: tpchan_list_rwlck, tpdev_list_rwlck,
*					tpcons_mutex and tpcons_sv 
*/
int
tpstart()
{
	ulong	sz,cnt;

	if (!tp_listallocsz){
		/*
		*+ tp_listallocsz is a tunable in Space.c. it is the size of
		*+ chunks to allocate for head lists. size of zero is a err
		*/
		cmn_err(CE_WARN,
		"Tuneable value 'tp_listallocsz' cannot be 0, forcing to 1\n");
		tp_listallocsz = 1;
	}

	/*
	* Did not put in error handling if kmem_zalloc() was not able to
	* allocate memory.  If it could not, it will be handled by tpopen(),
	* If the system has not already panicked.
	*/
	/*
	* The initial allocation of channel and device entries must not be
	* less than TP_NONCLONE because the special channels must be
	* allocated.  Once the initial allocation is made, the lists can 
	* grow by any non-zero increment. 
	*/
	cnt = (tp_listallocsz < TP_NONCLONE) ? TP_NONCLONE : tp_listallocsz;
	sz = cnt * sizeof(struct tpchan *);
	tpchanhead = (struct tpchan **)kmem_zalloc(sz, KM_NOSLEEP);
	ASSERT(tpchanhead);

	tpdevhead = (struct tpdev **)kmem_zalloc(sz, KM_NOSLEEP);
	ASSERT(tpdevhead);

	tpchan_curcnt = cnt;
	tpdev_curcnt = cnt;


	/*
	* calculate external major device number for tp 
	*/

	tpemaj = itoemajor(tp_imaj, -1);

	/*
	* allocate the global locks etc 
	*/

	tp_plumb_rwlck = RW_ALLOC(TPHIER_PLUMB_RWLCK, TPPL, &tp_plumb_rwlck_lkinfo, KM_NOSLEEP);
	ASSERT(tp_plumb_rwlck);


	tpcons_mutex = LOCK_ALLOC(TPHIER_CONSSV, TPPL, &tp_conssv_mutex_lkinfo, KM_NOSLEEP);
	ASSERT(tpcons_mutex);
	tpcons_sv = SV_ALLOC(KM_NOSLEEP);
	ASSERT(tpcons_sv);

        /* calculate default device number for system console */
        tp_consoledev = makedevice(tp_consoledevmaj, tp_consoledevmin);


	return (0);
}

/*
* int
* tp_getmajor(major_t *)
*	 Get Trusted Path's major device number.
*
* Calling/Exit State:
*	 Locks: Enter with: None
*	        Leave with: None
*	        Sets:        None
*        
*/
int
tp_getmajor(major_t *majornump)
{
	*majornump = itoemajor(tp_imaj, -1);
	return (1);	/* Return "TRUE" value, since stub function
			* returns "FALSE" value.
			*/
}


/*
* STATIC int
* tpopen(queue_t *, dev_t *, int , int , cred_t *)
*
* Calling/Exit State:
*	 Locks: Enter with: None
*	        Leave with: None
*	        Sets: tpchan_list_rwlck, tpc_data_mutex, tpcons_mutex, tpcons_sv
*/

/* ARGSUSED */
STATIC int
tpopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *credp)
{

	ulong		type;
	int		(*setup)();
	minor_t	min;
	struct tpchan	*tpcp;
	pl_t		pl_1;
	pl_t		pl_2;


	/*
	* TP devices cannot be opened as STREAMS modules (sflag == MODOPEN)
	* or via the CLONE device (sflag == CLONEOPEN).  TP device must be
	* opened as a normal driver (sflag == 0).
	*/
	if (sflag)
		return (ENXIO);

	/*
	* The way this switch works, all clone masters set up necessary
	* information and break out of the switch to finish allocating and
	* initializing the clone.  All non-clone devices must do any necessary
	* work and return directly from the switch.
	*/
	min = geteminor(*devp);
	switch(min){
	case TP_CONSDEV:
		/*
		* If "delay open" is set, wait until tpconsdev gets set.
		* When tpconsdev gets set, the real/physical device is linked
		* under the TP device labeled the 'console' device.
		*/
		if (!(oflag & (FNDELAY|FNONBLOCK))){
			/* CONSTCOND */
			while (1) {
				/*
				* tpconsdev is really protected by tp_plumb_rw
				* tpcons_mutex is just to satidfy the SV_WAIT
				* criteria
				*/
				pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
				pl_2 = LOCK(tpcons_mutex, plstr);
				if(tpconsdev) {
					UNLOCK(tpcons_mutex, pl_2);
					RW_UNLOCK(tp_plumb_rwlck, pl_1);
					break;
				}
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
				if(SV_WAIT_SIG(tpcons_sv, primed, tpcons_mutex) == B_FALSE) {
					return(EINTR);
				}
			}
		}
		/*
		* most probably going to write into plumbing so get it
		* for writing now
		*/
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		tpcp = tp_findchan(min);
		if (!tpcp){
			tpcp = tp_allocchan(TPC_CONS);
			if (!tpcp){
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
				return (EAGAIN);
			}
			/*
			* tp_setupcons unlocks all because it does a putnext
			*/
			return(tp_setupcons(tpcp, q, pl_1));
		}
		else {
			/*
			* already opened, ptrs set up and allocated for CONSDEV
			* so just say all is ok. No need to save the pointers
			* This allows us to not have to lock everything to
			* change the pointersreset the pointers.
			*/
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			return(0);
		}
	
	case TP_CTRLCLONE:
		type  = TPC_CTRL;
		setup = tp_setupctrl;
		break;
	case TP_DATACLONE:
		type = TPC_DATA;
		setup = tp_setupdata;
		break;
	case TP_ADMDEV:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		tpcp = tp_findchan(min);
		if (!tpcp){
			tpcp = tp_allocchan(TPC_ADM);
			if (!tpcp){
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
				return (EAGAIN);
			}
			tp_hookchan(tpcp,q);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			qprocson(q);
			return (0);
		}
		else {
			/*
			* already opened, ptrs set up and allocated for ADMSDEV
			* so just say all is ok. No need to save the pointers
			* This allows us to not have to lock everything to
			* change the pointersreset the pointers.
			*/
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			return(0);
		}

	case TP_RESERVDEV1:
	case TP_RESERVDEV2:
	case TP_RESERVDEV3:
	case TP_RESERVDEV4:
	case TP_RESERVDEV5:
	case TP_RESERVDEV6:
		return (ENXIO);
	default:
		/*
		* The request is for a specific clone.  The clone must have
		* a non-NULL channel entry and be connected.  All channels
		* are disconnected on close, so, if the device is connected
		* it is open.
		* NOTE: Since a control channel can only be opened by one
		* process at a time. There is, therefore, no legal, direct way
		* to open a control clone.
		*/
		/*
		* nothing is going to be written to plumbing so just
		* hold as reader
		*/
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		tpcp = tp_findchan(min);
		if (!tpcp) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			return (ENXIO);	/*Channel does not exist*/
		}

		pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
		if (tpcp->tpc_type != TPC_DATA) {
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			return (EBUSY);	/*Channel is CONTROL (see NOTE above)*/
		}

		if (tpcp->tpc_flags & TPC_BLOCKED){
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			return (EIO);
		}

		/*
		* unlock tpc_data_mutex because tpc_devp is protected by
		* plumbing stuff
		*/
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		if (!tpcp->tpc_devp) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			return (EIO);	/*IO Error, channel not connected*/
		}

		/*
		* no need for qprocson cause already done for this q
		* by CTRL or DATA clode
		*/
	
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return (0);
	}

	/*
	* If we got here the channel was either a CONTROL or DATA clone master
	* device and the type specific information has been initialized. 
	* Proceed to allocate the clone and fill it out.
	*/
	pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
	tpcp = tp_allocchan(type);
	if (!tpcp){
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return (EAGAIN);
	}

	/*
	* Send back the major minor pair created by tp_allocchan.
	*/

	*devp = tpcp->tpc_dev;

	return((*setup)(tpcp,q, pl_1));
}

/*
* STATIC	int
* tp_setupcons(struct tpchan *, queue_t *, pl_t )
*	Set the cons channel TP device pointer to 'console' TP device.
*	Send the M_SETOPTS message up stream to tell the stream head
*	that this stream is a tty.
*
* Calling/Exit State:
*	 Locks: Enter with: tp_plumb_rwlck set to WRITE
*	        Leave with: tp_plumb_rwlck unlocked (putnext done) 
*	        Sets:        
*	 Note: no need to lock data in tpcp because we just alloced the data
*	 and qprocs are off
*/
STATIC	int
tp_setupcons(struct tpchan *tpcp, queue_t *q, pl_t pl_1)
{
	mblk_t			*bp;
	struct stroptions	*sop;

	tp_hookchan(tpcp,q);
	tpcp->tpc_devp = tpconsdev;

	bp = allocb(sizeof(struct stroptions),BPRI_HI);
	if (!bp){
		tp_freechan(tpcp);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return (EAGAIN);
	}
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof(struct stroptions);
	/* LINTED pointer alignment */
	sop = (struct stroptions *)bp->b_rptr;
	sop->so_flags = SO_ISTTY;

	
	RW_UNLOCK(tp_plumb_rwlck, pl_1);

	qprocson(q);

	putnext(q, bp);
	return (0);
}

/*
* STATIC	int
* tp_setupdata(struct tpchan *, queue_t *, pl_t )
*	 Allocate a M_HANGUP message to be sent up the data channel when a SAK
*	 is detected.  Also send the M_SETOPTS message up stream to tell the
*	 stream head that this stream is a tty.
*	
*
* Calling/Exit State:
*	 Locks: Enter with: tp_plumb_rwlck held for WRITE,
*				pass in pl so we can unlockJ before putnext
*	        Leave with: tp_plumb_rwlck unlocked 
*	        Sets:        
*	 Note: no need to lock data in tpcp because we just alloced the data
*	 and qprocs are off
*/
STATIC	int
tp_setupdata(struct tpchan *tpcp, queue_t *q, pl_t pl_1)
{
	mblk_t			*bp;
	struct stroptions	*sop;

	/*
	* Allocate necessary messages.
	*/
	tp_newmsgs(tpcp);
	if (!tpcp->tpc_hupmsg || !tpcp->tpc_trailmsg){
		tp_freechan(tpcp);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return (EAGAIN);
	}

	tp_hookchan(tpcp,q);
	/*
	* Allocate and send the M_SETOPTS message.
	*/
	bp = allocb(sizeof(struct stroptions),BPRI_HI);
	if (!bp){
		tp_freechan(tpcp);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return (EAGAIN);
	}
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof(struct stroptions);
	/* LINTED pointer alignment */
	sop = (struct stroptions *)bp->b_rptr;
	sop->so_flags = SO_ISTTY;

	RW_UNLOCK(tp_plumb_rwlck, pl_1);

	qprocson(q);

	putnext(q, bp);
	return (0);
}

/*
* STATIC	int
* tp_setupctrl(struct tpchan *, queue_t *, pl_t )
*	 Allocate the M_PCPROTO buffer to be sent up the control channel to
*	 the stream head when a SAK is detected.
*
* Calling/Exit State:
*	 Locks: Enter with: tp_plumb_rwlck held for WRITE,
*				pass in pl so we can unlock. pl_1 and unlock
*				done for consistency with tp_setupcons
*				and tp_setupdata
*	        Leave with: tp_plumb_rwlck unlocked 
*	        Sets:        
*	 Note: no need to lock data in tpcp because we just alloced the data
*	 and qprocs are off
*/
STATIC	int
tp_setupctrl(struct tpchan *tpcp, queue_t *q, pl_t pl_1)
{
	tp_newmsgs(tpcp);
	if ((!tpcp->tpc_sakmsg) || (!tpcp->tpc_hupmsg)){
		tp_freechan(tpcp);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return (EAGAIN);
	}
	tp_hookchan(tpcp,q);
	RW_UNLOCK(tp_plumb_rwlck, pl_1);
	qprocson(q);
	return (0);
}

/*
* STATIC	void
* tp_hookchan(struct tpchan *, queue_t *)
*	 Set up channel queue pointers.
*
* Calling/Exit State:
*	 Locks: Enter with: None needed
*	        Leave with: None Needed
*	        Sets:        
*	 Note, no need to lock tpcp data because
*	 tp_hookchan is done when data is allocated and before qprocson
*/
STATIC	void
tp_hookchan(struct tpchan *tpcp, queue_t *q)
{
	q->q_ptr = (caddr_t)tpcp;
	WR(q)->q_ptr = (caddr_t)tpcp;
	tpcp->tpc_rq = q;
	tpcp->tpc_devp = (struct tpdev *)0;
}

/*
* STATIC int
* tpclose(queue_t *, int , cred_t *)
*
* Calling/Exit State:
*	 Locks: Enter with: None
*	        Leave with: None
*	        Sets:    tpchan_list_rwlck, tpdev_list_rwlck, tpcons_mutex
*			and various tpc_data_mutexes and tpd_data_mutexes.
*			Much of the locking is for the benifit of lower layers
*			and is done here to avoid dead lock.   
*/
/* ARGSUSED */
STATIC int
tpclose(queue_t *q, int flag, cred_t *credp)
{
	struct tpchan	*tpcp;
	struct	tpdev	*devp;
	struct tpchan *datap;
	pl_t pl_1;
	pl_t pl_2;
	pl_t pl_3;
	toid_t saveid;


	/*
	* q->q_ptr is now locked by qprocsoff
	*/
	tpcp = (struct tpchan *)q->q_ptr;

	if (!tpcp){
		/*
		*+ This should never happen!
		*+ Nothing further can be done so just return.
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tpclose()\n");
		return (0);
	}

	/*
	* assume we will be mucking with the plumbing and get the 
	* plumbing lock for write
	*/
	pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);

	/*
	* we'll be going in and out of data in the dev/chan so
	* lock all data now
	*/
	pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
	devp = tpcp->tpc_devp;

	/*
	* if the associated lower is in a putnext through this upper then
	* sleep waiting for it to finish
	*/
	while( tpcp->tpc_flags & TPC_BUSYNEXT) {
		tpcp->tpc_flags |= TPC_CLOSING;

		/*
		* need all unlocked to call SV_WAIT
		* unlock tp_plumb at pl_2 to not get inverted
		*/
		RW_UNLOCK(tp_plumb_rwlck, pl_2);

		/*
		* SV_WAIT will unlock tpc_data_mutex
		*/
		/*
		* Allow us to be bumped out of the wait. Unfortunatly
		* we don't have much choice but to give it another try.
		* if we get bumped out of the wait
		*/
		SV_WAIT_SIG(tpcp->tpc_close_sv, primed, tpcp->tpc_data_mutex);

		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);

		tpcp->tpc_flags &= ~TPC_CLOSING;
	}
		
	/*
	* qprocsoff must be here so as to allow the busy stuff above to work
        * need to unlock for qprocsoff and lock again after return.
	*/
        UNLOCK(tpcp->tpc_data_mutex, pl_2);
        RW_UNLOCK(tp_plumb_rwlck, pl_1);
	qprocsoff(q);
        pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
        pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);


	/*
	* If the channel is not connected or it is the adm channel,
	* just free the channel and get out. 
	*
	* this check was moved to after the SV_WAIT because it is
	* theoreticlly possible for devp to have gone to zero
	* while SV_waiting.
	*/
	devp = tpcp->tpc_devp;
	if (!devp || tpcp->tpc_type == TPC_ADM) {
		tpcp->tpc_devp = (struct tpdev *)0;
		q->q_ptr = (struct tpchan *)0;
		WR(q)->q_ptr = (struct tpchan *)0;
		/*
		* unlock data (in preperation for returning.
		* freechan needs plumbing set to write
		*/
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		tp_freechan(tpcp);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return (0);
	}

	/*
	* If we got to here we have a DATA or CTRL channel and it is
	* connected, or we have a the CONS channel that is connected for
	* output and it may be connected for input.  The associated TP device
	* may or may not have a real/physical device linked underneath.
	*/

	/*
	* pl_1 is tp_plumb_rwlck, pl_2 is tpc_data_mutex
	*/
	switch (tpcp->tpc_type){
	case TPC_CTRL:
		/*
		* Clear out the fields in the TP device that will indicate
		* to the interrupt level that the ctrl channel is closed.
		* This includes the tdp_ctrlchp, and tpd_ioctlchan if it
		* matches the channel being closed.
		*/
		pl_3 = LOCK(devp->tpd_data_mutex, plstr);
		devp->tpd_ctrlchp = (struct tpchan *)0;
		if (devp->tpd_ioctlchan == tpcp){
			devp->tpd_ioctlchan = (struct tpchan *)0;
			devp->tpd_ioctlid = 0;
		}
		if (tpcp->tpc_ioctlmp){
			freemsg(tpcp->tpc_ioctlmp);
			tpcp->tpc_ioctlmp = (mblk_t *)0;
		}

		devp->tpd_ctrlrq = (queue_t *)0;
		tpcp->tpc_devp = (struct tpdev *)0;

		/*
		* If the SAKSET flag is set and the SAK type is none (if SAK
		* type is none, the only time SAKSET would be set is the
		* special case of SAK type none indicated by saktypeDATA),
		* then a 'fake' SAK has been received (see tplrput() for
		* details).  Since the control channel is going away there is
		* no need to remember the fake SAK, so clear the SAKSET flag.
		*/
		if (devp->tpd_sak.sak_type == saktypeDATA){
			devp->tpd_flags &= ~TPD_SAKSET;
		}
		/*
		* If the device is not persistently linked, the real/physical
		* device was already unlinked (called internally by Stream
		* Head functions) before the close of this channel was called.
		* We will not get any more messages from tplrput() so it is
		* safe to dismantle the TP device.  Since the TP device is
		* being dismantled, the data and cons channel must be
		* disconnected if they are connected to the TP device.
		*/
		if (!(devp->tpd_flags & TPD_PERSIST)){
			/*
			* unlock data locks so we are free to do
			* anything (like putnexts) below
			*/
			UNLOCK(devp->tpd_data_mutex, pl_3);
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			/*
			* If device is the 'console' deivce reset
			* 'console' device
			*/
			if (devp == tpconsdev){
				tp_resetcons((struct tpdev *)NULL);
			}
			datap = devp->tpd_datachp;
			if (datap){
				tp_discdata(devp);
			}

                        /*
                        * Before freeing TP device call untimeout() if there
                        * is a pending function to be called that was setup
                        * by timeout().  This is necessary since the pending
                        * function's argument is a pointer to the TP device
                        * structure.
                        * NOTE: Other candidate areas to call untimeout()
                        * are when the data channel is connected to a TP
                        * device, tp_conndata(), and when the sak type is
                        * changed to a type other than NONE, tp_putsak().
                        * It is not done since the pending function,
                        * tp_saktypeswitch() makes checks that prevents
                        * the sak type from being changed to DATA when it
                        * is not appropriate, and having the splhi() called
                        * for every data channel connect and change of sak is
                        * not worth it.
                        *
                        */
			pl_2 = LOCK(devp->tpd_data_mutex, plstr);
                        if (devp->tpd_timeoutid){
				saveid=devp->tpd_timeoutid;
                                devp->tpd_timeoutid = 0;
				UNLOCK(devp->tpd_data_mutex, pl_2);
                                untimeout(saveid);
                        }
			else {
				UNLOCK(devp->tpd_data_mutex, pl_2);
			}

                	/*
			* routines above may have had 'put's to do, we
                	* saved them for now so we could unlock everything
                	* note: we use device numbers because the special
                	* put rouites will get the pointers again and lock it
                	* there. It is done before the tp_freechan because it
			* depends on a built channel
                	*/
			/*
			* this is here so it is done before the putnext
			*/
			pl_1 = tp_doputnext_low(devp, pl_1, 'W');


			tp_freedev(devp);
		}else{
			/*
			* Device is persistently linked, if a SAK
			* is pending, disconnect and error out the
			* data channel.  This is to handle the case
			* were a SAK was detected before tpclose()
			* marked the ctrl channel as closed.
			* tplrput() would have sent up SAK
			* notification up the ctrl channel, but since
			* we are closing the the ctrl channel, the
			* notification would go unprocessed and the
			* data channel would remain connected.  This
			* would be a problem if the intent was to
			* disconnect the data channel.  So to be safe
			* the data channel is disconnected.  This is
			* the same action as is in tplrput() when it
			* detects a SAK and there is no ctrl channel.
			*/
			if (devp->tpd_datachp && (devp->tpd_flags & TPD_SAKSET)){
				/*
				* unlock data locks so we are free to do
				* anything (like putnexts) below
				*/
				UNLOCK(devp->tpd_data_mutex, pl_3);
				UNLOCK(tpcp->tpc_data_mutex, pl_2);
				tp_discdata(devp);
				tp_discinput(devp);
			}
			else {
				UNLOCK(devp->tpd_data_mutex, pl_3);
				UNLOCK(tpcp->tpc_data_mutex, pl_2);
			}
                	/*
			* routines above may have had 'put's to do, we
                	* saved them for now so we could unlock everything
                	* note: we use device numbers because the special
                	* put rouites will get the pointers again and lock it
                	* there. It is done before the tp_freechan because it
			* depends on a built channel
                	*/
			pl_1 = tp_doputnext_low(devp, pl_1, 'W');
		}


		/*
		* only tp_plumb_rwlck still held
		*/
		q->q_ptr = (caddr_t)0;
		WR(q)->q_ptr = (caddr_t)0;
		tp_freechan(tpcp);
		break;

	case TPC_DATA:
		/*
		* unlock data locks so we are free to do
		* anything (like putnexts) below
		*/
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		tp_discdata(devp);

                /*
		* routines above may have had 'put's to do, we
                * saved them for now so we could unlock everything
                * note: we use device numbers because the special
                * put rouites will get the pointers again and lock it
                * there. It is done before the tp_freechan because it
		* depends on a built channel
                */
		pl_1 = tp_doputnext_low(devp, pl_1, 'W');

		q->q_ptr = (caddr_t)0;
		WR(q)->q_ptr = (caddr_t)0;

		tp_freechan(tpcp);

		break;

	case TPC_CONS:
		/*
		* unlock data locks so we are free to do
		* anything (like putnexts) below
		*/
		UNLOCK(tpcp->tpc_data_mutex, pl_2);

		(void)tp_disccons();
		/*
		* The tpd_ioctlchan is only cleared here, ( if the
		* tpd_ioctlchan is equal to the TPC_CONS channel) and not
		* in tp_disccons() (like it is done in tp_discdata()) because
		* tp_disccons() also gets called from other areas (eg when
		* switching the 'console' TP device) that still have the
		* cons channel retaining output access to a TP device.
		*/
		pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
		pl_3 = LOCK(devp->tpd_data_mutex, plstr);
		if (devp->tpd_ioctlchan == tpcp){
			devp->tpd_ioctlchan = (struct tpchan *)0;
			devp->tpd_ioctlid = 0;
		}
		if (tpcp->tpc_ioctlmp){
			freemsg(tpcp->tpc_ioctlmp);
			tpcp->tpc_ioctlmp = (mblk_t *)0;
		}


		UNLOCK(devp->tpd_data_mutex, pl_3);
		UNLOCK(tpcp->tpc_data_mutex, pl_2);

                /*
		* routines above may have had 'put's to do, we
                * saved them for now so we could unlock everything
                * note: we use device numbers because the special
                * put rouites will get the pointers again and lock it
                * there. It is done before the tp_freechan because it
		* depends on a built channel
                */
		pl_1 = tp_doputnext_low(devp, pl_1, 'W');

		q->q_ptr = (caddr_t)0;
		WR(q)->q_ptr = (caddr_t)0;

		tp_freechan(tpcp);
		break;

	default:
		/*
		* give up all the locks 
		*/
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		break;

	} /* end switch */
	/*
	* come here with only tp_plumb_rwlck held
	*/
	RW_UNLOCK(tp_plumb_rwlck, pl_1);
	return (0);
}


/*
* STATIC int
* tpuwput(queue_t *, mblk_t *)
*	 Upper write put procedure.
*
*	This routine handles only M_FLUSH and M_IOCTL.  Other message types
*	 are passed on down the stream, provided the channel on which they were
*	 sent supports messages other than M_FLUSH and M_IOCTL.
*	 There is special handling for M_IOCDATA messages.
*	 NOTE:The TP driver does not support Transparent ioctls but it does send
*	 all non-TP ioctls downstream including Transparent ioctls.
*
* Calling/Exit State:
*	 Locks: Enter with: None
*	        Leave with: None
*	        Sets:   tp_plumb_rwlck at various times. dosn't pass it set to
*			lower ioctls because some need it for read and
*			others for write also sets tpc_datamutex and
*			tpd_data_mutex
*/


STATIC int
tpuwput(queue_t	*q, mblk_t *mp)
{
	struct tpdev *tpdp = (struct tpdev *)0;
	struct tpchan *tpcp = (struct tpchan *)0;
	queue_t *realwq = (queue_t *)0;
	pl_t	pl_1;
	pl_t	pl_2;
	pl_t	pl_3;
	int	type;

	tpcp = (struct tpchan *)q->q_ptr;

	if (mp->b_datap->db_type == M_FLUSH){
		type = *mp->b_rptr;
		if (type & FLUSHRW) {
			/*
			* perform our flush duties as a 'driver'
			*/
			/*
			* save flush type for possible propogation down
			*/
			if (type & FLUSHW){
				if (type & FLUSHBAND){
					flushband(q, *(mp->b_rptr + 1),
				 	FLUSHDATA);
				}else{
					flushq(q, FLUSHDATA);
				}
			}
			if (type & FLUSHR){
				if (type & FLUSHBAND){
					flushband(RD(q), *(mp->b_rptr +1),
				 	FLUSHDATA);
				}else{
					flushq(RD(q), FLUSHDATA);
				}
				*mp->b_rptr &= ~FLUSHW;
				qreply (q, mp);
			} else {
				freemsg(mp);
			}
			

			/*
			* If this is the channel that is connected for input
			* and a real/physical device is linked underneath, the
			* M_FLUSH message will be sent down to the
			* real/physical device driver.
			*/ 
			tpdp = (struct tpdev *)0;
			if (tpcp){
				pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
				tpdp = tpcp->tpc_devp;
				if (tpdp) {
					if(tpdp->tpd_realrq){
						realwq = WR(tpdp->tpd_realrq);
					}
				}
			}
			if (tpdp && (tpdp->tpd_inputchp == tpcp) && realwq){
				pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
				tpdp->tpd_flags |= TPD_BUSYNEXT;
				/*
				* if we successfully got here, both lock ARE
				* held, so no need to test pl
				*/
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
				RW_UNLOCK(tp_plumb_rwlck, pl_1);

				putnextctl1(realwq, M_FLUSH, type );
				pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
				/*
				* tpdp is locked by TPD_BUSYNEXT
				*/
				pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
				tpdp->tpd_flags &= ~TPD_BUSYNEXT;
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
			}
			else if (tpcp) {
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
			}
		}else{
			freemsg(mp);
		}
		return (0);
	}

	if (!tpcp){
		/*
		*+ This should never happen! But if it does, the test is
		*+ put after M_FLUSH processing since putctl1() M_ERROR
		*+ sends an M_FLUSH back to here.  If it was before M_FLUSH,
		*+ an inifinite loop of putctl1(), strrput(), qreply(), and
		*+ tpuwput() will occur.
		*+ DO NOT MOVE THIS TEST BEFORE M_FLUSH PROCESSING!! 
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tpuwput()\n");
		freemsg(mp);
		putnextctl1(RD(q),M_ERROR,EIO);
		return (0);
	}

	
	switch (mp->b_datap->db_type){

	/*
	* M_IOCDATA messages are handled a little differently then all
	* the other message types (besides M_IOCTL). They are not
	* enqueued if the channel becomes blocked or no physical device
	* is linked underneath; they are turned into an M_IOCNAK and sent
	* back upstream if the M_IOCDATA indicated that the M_COPYIN or
	* M_COPYOUT had succeeded.
	*/
	case M_IOCDATA:

		switch(tpcp->tpc_type){
		case TPC_CTRL:
		case TPC_DATA:
		case TPC_CONS:
			break;
		default:
			/*
			* ADM channel does not pass transparent ioctls
			* downstream, therefore it should not recieve
			* M_IOCDATA messages
			*/
			freemsg(mp);
			return (0);
		}
		/*
		* need'em all (the locks)
		*/
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		tpdp = tpcp->tpc_devp;
		pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
		if (tpdp) {
			pl_3 = LOCK(tpdp->tpd_data_mutex, plstr);
			if(tpdp->tpd_realrq){
				realwq = WR(tpdp->tpd_realrq);
			}
		}
		if (!(tpcp->tpc_flags & TPC_BLOCKED) && tpdp && realwq &&
		/* LINTED pointer alignment */
		 (tpdp->tpd_ioctlid == ((struct copyresp *)(mp->b_rptr))->cp_id)){
			tpdp->tpd_flags |= TPD_BUSYNEXT;
			
			UNLOCK(tpdp->tpd_data_mutex, pl_3);
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);

			putnext(realwq, mp);

			pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
			/*
			* tpdp is locked by TPD_BUSYNEXT
			*/
			pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
			tpdp->tpd_flags &= ~TPD_BUSYNEXT;
			UNLOCK(tpdp->tpd_data_mutex, pl_2);

			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			
		}else{
			/* LINTED pointer alignment */
			struct iocblk	*iocp = (struct iocblk *)(mp->b_rptr); 
			/* LINTED pointer alignment */
			struct copyresp	*cpresp = (struct copyresp *)(mp->b_rptr);

			/*
			* If the ioctl id in the M_IOCDATA is the same as the
			* ioctl id saved in the TP device, clear out the saved
			* ioctl id and channel the ioctl originated from in
			* the TP device.  If there are other ioctls waiting
			* to be sent downstream, schedule the lower ctrl
			* channel write Queue.  The TP driver's lower write
			* service function will schedule the write Queue's
			* service function of the channel that has a pending
			* ioctl to be sent downstream.
			*/
			if (tpdp && realwq &&
			 (tpdp->tpd_ioctlid ==  cpresp->cp_id)){
				tpdp->tpd_ioctlid = 0;
				tpdp->tpd_ioctlchan = (struct tpchan *)0;

				if (tpdp->tpd_flags & TPD_WAIT_IOCTL){
					enableok(realwq);
					qenable(realwq);
				}
			}

			/*
			* cp_rval == 0 indicates that the previous M_COPYIN
			* or M_COPYOUT was successful.  If it was not
			* successful, the Stream Head is not expecting a
			* M_IOCACK or M_IOCNAK, so just free the message.
			*/
			/*
			*  unlock all, no more need for them
			*/
			if(tpdp)
				UNLOCK(tpdp->tpd_data_mutex, pl_3);
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			if (cpresp->cp_rval){
				if (cpresp->cp_private){
					freemsg(cpresp->cp_private);
				}
				freemsg(mp);
			}else{
				freemsg(unlinkb(mp));
				if (cpresp->cp_private){
					freemsg(cpresp->cp_private);
				}
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_count = 0;
				iocp->ioc_error = EIO;
				iocp->ioc_rval = 0;
				qreply(q, mp);
			}
		}
		/*
		* no locks at this point so O.K. to ret 
		*/
		return (0);

	case M_IOCTL:
	{
                struct iocblk *iocbp;

                iocbp = (struct iocblk *)mp->b_rptr;
                if (iocbp->ioc_count == TRANSPARENT) {
                        tpdp = tpcp->tpc_devp;
                        pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
                        tpdp->tpd_flags |= TPD_TRANSPARENT;
                        UNLOCK(tpdp->tpd_data_mutex, pl_2);
                }

		switch(tpcp->tpc_type){
		case TPC_CTRL:
			tp_ctrlioctl(q,mp);
			break;
		case TPC_DATA:
			tp_dataioctl(q,mp);
			break;
		case TPC_ADM:
			tp_admioctl(q,mp);
			break;
		case TPC_CONS:
			tp_consioctl(q,mp);
			break;
		default:
			mp->b_datap->db_type = M_IOCNAK;
			/* LINTED pointer alignment */
			((struct iocblk *)mp->b_rptr)->ioc_error = EINVAL;
			qreply(q, mp);
			return (0);
		}
		return (0);
	}
	default:
		break;
	}

	/*
	* If we got this far it is not a flush or ioctl() or M_IOCDATA.
	* Since ctrl and adm channels can only handle flush or ioctl message
	* types (ctrl can also handle M_IOCDATA messages), ignore the message
	* if the channel is ctrl or adm.
	*/
	if ((tpcp->tpc_type == TPC_CTRL) || (tpcp->tpc_type == TPC_ADM)){
		freemsg(mp);
		return (0);
	}

	/*
	* If channel is blocked, just free the message.
	* Once the channel is blocked, it has to be closed inorder to become
	* eligible to be re-used again.
	*/
	pl_1 = LOCK(tpcp->tpc_data_mutex, plstr);
	if (tpcp->tpc_flags & TPC_BLOCKED){
		UNLOCK(tpcp->tpc_data_mutex, pl_1);
		freemsg(mp);
		return (0);
	}
	/*
	* If it is a data or console channel and is connected to TP device
	* and a physical device is linked underneath, send the message
	* (or queue it) down the stream.
	* If the channel is not connected to a TP device, or there
	* is no physical device linked underneath,  prevent the Queue from
	* being schedule and enqueue the message on the Queue.  When the
	* channel is connected or physical device linked underneath the TP
	* device, the channel Queue's service function will be scheduled.
	*/


	/*
	* dont need tpc_data-Mutex anymore
	*/
	UNLOCK(tpcp->tpc_data_mutex, pl_1);

	pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
	tpdp = tpcp->tpc_devp;

	if (tpdp){
		if(tpdp->tpd_realrq){
			realwq = WR(tpdp->tpd_realrq);
		}
		if (realwq){

			/*
			* for MP we have made all data go through
			* the upper write service procedure
			* in order to avoid the possibility of out
			* of order messages. Note, if this should
			* prove to be a performance problem, an
			* elaborate locking mehanisim can be done
			*/
			if (pcmsg(mp->b_datap->db_type)){
				pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
				tpdp->tpd_flags |= TPD_BUSYNEXT;
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
				RW_UNLOCK(tp_plumb_rwlck, pl_1);

				putnext(realwq, mp);

				pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
				/*
				* tpdp is locked by TPD_BUSYNEXT
				*/
				pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
				tpdp->tpd_flags &= ~TPD_BUSYNEXT;
				UNLOCK(tpdp->tpd_data_mutex, pl_2);

				RW_UNLOCK(tp_plumb_rwlck, pl_1);
			}else{
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
				putq(q, mp);
			}		
		}else{
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			noenable(q);
			putq(q, mp);
		}
		return (0);
	}
	else {
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		noenable(q);
		putq(q, mp);
		return (0);
	}
}

/*
* STATIC int
* tpuwsrv(queue_t *)
*	Upper write service procedure:
*
*	 Get messages off queue when flow control permits.
*
*	 NOTE: The write queue is enabled from the lower write service
*	       procedure when the lower service procedure is back-enabled. It
*	       is also enabled whenever a physical device is linked underneath
*	       the queue.
*	
* Calling/Exit State:
*	 Locks: Enter with: None
*	        Leave with: None
*	        Sets:   tp_plumb_rwlck at various times. also tpd_datamutex and 
*			tpcdata mutex
*
*		NOTE: The locking scheme here is very costly since we must
*			re-aquire all locks after a putnext. Since we made the
*			decision that all data comes through here (tpuwput no
*			longer does a putnext) this is a very used subroutine.
*			Should we need to fix up performance, the locking
*			scheme here should the first to be fine tuned.
*			
*/

STATIC int
tpuwsrv(queue_t *q)
{
	struct tpchan *tpcp;
	struct tpdev *tpdp;
	queue_t *realrq;
	mblk_t *mp;
	pl_t	pl_1;
	pl_t	pl_2;

	/*
	* If there is no channel, issue an error and get out. This should
	* not happen.
	*/
	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		*+ Messages will be flushed when Stream Head sends M_FLUSH
		*+ as a result of the M_ERROR being sent upstream.
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tpuwsrv()\n");
		putnextctl1(RD(q), M_ERROR, EIO);
		return (0);
	}

	/*
	* If we find no data here it could have falsely caused put to
	* do a putq. We don't have to worry about this because all it
	* is a is an extra trip here
	*/


	/*
	* The "queued" ioctl for the channel gets handled before any other
	* messages.
	*/
	pl_1 = LOCK(tpcp->tpc_data_mutex, plstr);
	if ((mp = tpcp->tpc_ioctlmp) != (mblk_t *)0){
                struct iocblk *iocbp;

		tpcp->tpc_ioctlmp = (mblk_t *)0;
		UNLOCK(tpcp->tpc_data_mutex, pl_1);
		iocbp = (struct iocblk *)mp->b_rptr;
                if (iocbp->ioc_count == TRANSPARENT) {
                        tpdp = tpcp->tpc_devp;
                        pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
                        tpdp->tpd_flags |= TPD_TRANSPARENT;
                        UNLOCK(tpdp->tpd_data_mutex, pl_2);
                }
		switch(tpcp->tpc_type){
		case TPC_CTRL:
			tp_ctrlioctl(q, mp);
			break;
		case TPC_DATA:
			tp_dataioctl(q, mp);
			break;
		case TPC_CONS:
			tp_consioctl(q, mp);
			break;
		default:
			/*
			*+ anything on adm channel because it has no lower
			*+ or another channel means we are corrupt
			*/
			cmn_err(CE_WARN,
			 "tpuwsrv(): Invalid tp channel %d specified for processing \"queued\" ioctl\nmessage type = %d\n",
			 tpcp->tpc_type, mp->b_datap->db_type);
			if (mp->b_datap->db_type == M_IOCTL){
				/*

				*+ if we had an ioctl on this bad Q let them
				*+ know what kind of IOCTL   it was
				*/
				cmn_err(CE_WARN,
				 "tpuwsrv(): cmd = %d\n",
				/* LINTED pointer alignment */
				 ((struct iocblk *)(mp->b_rptr))->ioc_cmd);
			}
			freemsg(mp);
			break; /*Ignore all other types*/
		}
	}
	else {
		UNLOCK(tpcp->tpc_data_mutex, pl_1);
	}
		

	/*
	* If the channel is not connected to a TP device or no real/physical
	* device is linked under the TP device, no service can
	* happen, prevent the queue from being schedule and get out but do not
	* issue an error.  If the channel becomes connected its queue will
	* be enabled.
	*/
	pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
	tpdp = tpcp->tpc_devp;
	if (!tpdp){
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		noenable(q);
		return (0);
	}
	realrq = tpdp->tpd_realrq;
	if (!realrq){
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		noenable(q);
		return (0);
	}
	/*
	* Everything is set up, serve the messages one at a time as long as
	* there are messages to send and flow control allows.
	*/
	while (((mp = getq(q)) != (mblk_t *)0) && (canputnext(WR(realrq)))){
		if ((tpcp->tpc_type == TPC_CTRL) || (tpcp->tpc_type == TPC_ADM)){
			/*
			*+ The TPC_CTRL and TPC_ADM channels should never have any
			*+ messages on their write Queue.  The service function for
			*+ TPC_CTRL should only be called if the channel has an M_IOCTL
			*+ message to send downstream.  TPC_ADM does not handle
			*+ downstream ioctls.
			*/
			cmn_err(CE_WARN,
			 "tpuwsrv(): Messages should not be on CTRL or ADM channel, message type = %d\n",
			 mp->b_datap->db_type);
			freemsg(mp);
			mp = (mblk_t *)0;
			flushq(q, FLUSHDATA);
			continue;
		}
		/*
		* TPC_BLOCKED is checked for every loop iteration since it
		* may be set if an interrupt occurs (input arrives) and a
		* SAK or M_HANGUP is detected in the lower read procedure
		* (tplrput()).
		*/
		pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
		if (tpcp->tpc_flags & TPC_BLOCKED){
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			freemsg(mp);
			mp = (mblk_t *)0;
			flushq(q, FLUSHDATA);
			continue; 
		}
		UNLOCK(tpcp->tpc_data_mutex, pl_2);

		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		tpdp->tpd_flags |= TPD_BUSYNEXT;
		/*
		* free all locks before doing our putnext 
		*/
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		
		putnext(WR(realrq), mp);
		
		/*
		* need to aquire all again 
		*/
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		/*
		* tpdp is locked by TPD_BUSYNEXT
		*/
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		tpdp->tpd_flags &= ~TPD_BUSYNEXT;
		UNLOCK(tpdp->tpd_data_mutex, pl_2);

		realrq = tpdp->tpd_realrq;
		if (!realrq){
			noenable(q);
			break;
		}
	/*
	* loop around to top of while with everything held 
	*/
	}
	if (mp){
		putbq(q, mp);
	}

	RW_UNLOCK(tp_plumb_rwlck, pl_1);
		
	return (0);
}


/*
* STATIC int
* tplwsrv(queue_t *)
*    lower write service procedure
*
*    This routine
*    1. sends internally generated ioctls downsteam.  Internally generated
*    ioctls get enqueued on the lower ctrl channel write Queue, when the
*    lower Stream has an ioctl message that it has not ACK/NAK yet.  tplrput()
*    enables the lower ctrl channel's write Queue when the it receives the
*    ACK/NAK.
*    2. enables write Queues of the connected channels (the upper Streams) on
*    the TP device.  Messages sent downstream from the connected channels on
*    the TP device get enqueued on the lowest write Queue (the upper side of TP
*    Multiplexor)  of the connected channels (all tp channels are upper Streams).
*    Enabling the write Queues of the connected channels schedules tpuwsrv()
*    to run which actually does the work of sending the messages downstream.
*   
*    This function gets scheduled to run:
*    1. when the TP device lower ctrl channel's write Queue
*    (aka the real/physical device's upper most write Queue) is back-enabled by
*    the STREAMS scheduler.  This can happen when the next downstream queue falls
*    below its low water mark.
*    2. by tplrput after recieving the negative or positive acknowlegement of
*    an ioctl.  This is done to schedule Queues of channels that have an ioctl
*    to send downstream.
*
*    NOTE: tplwsrv has to enable tpuwsrv, because back-enabling can not
*    cross multiplexer boundaries 
*
* Calling/Exit State:
*    Locks: Enter with: None
*           Leave with: None
*           Sets:   tp_plumb_rwlck at various times. also tpd_data_mutex and 
*/

STATIC int
tplwsrv(queue_t *q)
{
	struct tpdev *tpdp;
	queue_t *uwq;		/* for write queue of upper channels */
	mblk_t	*mp;
	struct tpchan *constpcp;
	int		ioctlsvc = 0;	/* indicates whether or not an ioctl
					* has been serviced or is in progress.
					* 0 indicates no.
					*/
	pl_t pl_1;	

	tpdp = (struct tpdev *)q->q_ptr;
	if (!tpdp){
		/*
		*+ This should not happen!
		*/
		cmn_err(CE_WARN,
			"Queue without TP device pointer in tplwsrv()\n");
		return (0);
	}

	/*
	* Servicing ioctls on this Write Queue (the lower CTRL channel) and
	* scheduling Queues for channels with ioctl to be sent downstream
	* is done first.  Pending ioctls are handled in the following order;
	* internally generated, ctrl channel, channel connected for input,
	* data channel, and cons channel.
	*
	* Only M_IOCTL messages should be on this Queue.
	* NOTE: the q_flag for QNOENB check is done to prevent an infinite
	* loop.  When tp_sendioctl() can not send the message it puts the
	* message back on the Queue and no-enables the Queue.
	* It may not be able to send the ioctl message if the lower Stream
	* still has an outstanding ioctl.
	*/
	while (!(q->q_flag & QNOENB) && (mp = getq(q))){
		if (mp->b_datap->db_type == M_IOCTL){
			/*
			* tp_lctrlioctl expects no locks to be held 
			*/
			tp_lctrlioctl(q, mp);
			break;
		}else{
			/*
			*+ we only put ioctls here in tpuwput or
			*+ tpuwsrv (or even tplrput). If we find
			*+ anything wlse here, we have a programming err
			*/
			cmn_err(CE_WARN,
			 "tplwsrv(): Got a non-ioctl message on lower ctrl channel write Queue; freeing message\n");
			freemsg(mp);
		}
	}
	ioctlsvc++;
	

	/*
	* The Write Queues for the channels are scheduled for service if
	* they are connected to the TP device AND (there is a message on
	* the Queue OR (there is an ioctl message the channel AND there is
	* not another ioctl scheduled or in progress downstream)).
	*
	* NOTE: The ctrl channel may not be connected even though the data
	* channel and TP device are intact.  This can occur if the
	* real/physical device was linked via I_PLINK. 
	*/
	pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
	if (tpdp->tpd_ctrlrq && tpdp->tpd_ctrlchp){
		uwq = WR(tpdp->tpd_ctrlrq);
		if (uwq) {
			enableok(uwq);
			qenable(uwq);
		}
	}

	if (tpdp->tpd_inputrq && tpdp->tpd_inputchp){
		uwq = WR(tpdp->tpd_inputrq);
		if (uwq) {
			enableok(uwq);
			qenable(uwq);
		}
	}

	/*
	* If the data channel is not the channel connected for input,  its
	* Write Queue may need to be scheduled for service.
	*/
	if (tpdp->tpd_datarq &&  tpdp->tpd_datachp &&
	 (tpdp->tpd_datarq != tpdp->tpd_inputrq)){
		uwq = WR(tpdp->tpd_datarq);
		if (uwq) { 
			enableok(uwq);
			qenable(uwq);
		}
	}


	/*
	* If this TP device is also the console TP device enable the cons
	* channel write queue, if it is connected (for at least output)
	* to this TP device.
	*/
	if (tpdp == tpconsdev){
		if (((constpcp = tp_findchan(TP_CONSDEV)) != (struct tpchan *)0)
		 && (constpcp->tpc_rq)){
			uwq = WR(constpcp->tpc_rq);
			if (uwq) {
				enableok(uwq);
				qenable(uwq);
			}
		}
	}

	RW_UNLOCK(tp_plumb_rwlck, pl_1);
	return (0);
}

/*
* STATIC void
* tp_ctrlioctl(queue_t *, mblk_t *)
*     process ioctls sent via the ctrl channel
*
*     This function handles all tp specific ioctl's.  The rules are as
*     follows:
* 	-The first ioctl on a ctrl channel must be TP_CONNECT.
* 	-TP_DEFSAK must be issued before a physical device can be
* 	 linked.
* 	-A physical device must be linked before any ioctl will be sent
* 	 further downstream.
* 	-A TCSET* must be sent down the control channel and an ACK must
* 	 be received from the lower stream before a TP_DATACONNECT may
* 	 occur.
* 	-TP_DATACONNECT clears the sakset flag and unblocks the data
* 	 channel.
*
* When the real/physical device is being linked,  an internal TCGETS
* M_IOCTL message is allocated and enqueued on the real device's
* (lower ctrl channel's) write Queue.  When the service proceedure
* is run (tplwsrv),  the TCGETS M_IOCTL message will be sent down to the
* real/physical tty device. The termios value returned from the
* real/physical tty device will be used to initialize the TP device's
* termios, tpd_curterm.

* 
* Calling/Exit State:
*      Locks: Enter with: None
*             Leave with: None
*             Sets:   tp_plumb_rwlck at various times to either READ or WRITE,
*     		sets tpd_data_mutex or tpc_data_mutex
*/

STATIC void
tp_ctrlioctl(queue_t *q, mblk_t	*mp)
{
	struct tpchan	*tpcp;
	struct tpdev	*tpdp;
	struct iocblk	*iocp;
	struct linkblk	*linkp;
	struct tp_info	*infp;
	struct tpdev		*newtpdp;
	int			cmd;
	unsigned char		reply = M_IOCACK;
	int			error = 0;
	pl_t			pl_1;
	pl_t			pl_2;
	mblk_t			*ioctlmp;

	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		*+ This should never happen!
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tp_ctrlioctl()\n");
		mp->b_datap->db_type = M_IOCNAK;
		/* LINTED pointer alignment */
		((struct iocblk *)(mp->b_rptr))->ioc_error = EIO;
		qreply(q, mp);
		return;
	}
	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;
	/*
	* The ctrl channel must be connected to a tp device before any
	* other ioctl can be considered. 
	*/
	/*
	* before MP there used to be a test of no lower channel and cmd != 
	* TP_CONNECT here. it was moved to the individual ioctls 
	* so we don't have to do an extra lock round trip. (some
	* below set to READ, others to WRITE
	*/

	/*
	* Do common functionality for all TP type ioctls.
	*/
	switch (cmd){
	case TP_CONNECT:
	case TP_DATACONNECT:
	case TP_DATADISCONNECT:
	case TP_DEFSAK:
	case TP_CONSCONNECT:
	case TP_CONSDISCONNECT:
	case TP_CONSSET:
	case TP_GETINF:
		if ( iocp->ioc_count < TPINFOSZ){
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q,mp);
			return;
		}
		/* LINTED pointer alignment */
		infp = (struct tp_info *)mp->b_cont->b_rptr;
		break;
	default:
		break;
	}
	switch (cmd){
	case TP_CONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		/*
		* if already have a lower, then error
		*/
		if (tpcp->tpc_devp){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EBUSY;
			break;
		}

		/*
		* If the ctrl channel is not already connected, allocate
		* a free tp device.
		*/
		/*
		* note tp_allocdev includes a tp_findev
		*/
		tpdp = tp_allocdev(infp->tpinf_rdev);
		if (!tpdp){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EAGAIN;
			break;
		}

		tpcp->tpc_devp = tpdp;

		/* 
		* tp_allocdev would have found an already used lower device
		* if the lower device had been I_PLINKed. We can tell
		* the difference beetween that case and a double TP_CONNECT
		* (which is the err below) becase in the PLINK case
		* tpd_ctrlchp would be null
		*/ 
		if (tpdp->tpd_ctrlchp){
			tpcp->tpc_devp = (struct tpdev *)0;
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EBUSY;
			break;
		}
		/*
		* Connect the ctrl channel to the allocated tpdev and
		* set up the control channel TP device queue.
		*/
		tpdp->tpd_ctrlchp = tpcp;
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		tpdp->tpd_ctrlrq = RD(q);
		if (infp->tpinf_flags & TPINF_CONSOLE){
			tpdp->tpd_userflags |= (infp->tpinf_flags & TPINF_CONSOLE);
		/*
		* Do not mark console output to be swithed to the default
		* console (indicated by tp_consoledev) if a console is
		* already defined (indicated by tpconsdev).  The switch
		* back to the default console (provided that it is still
		* linked) is made when the real device referenced by
		* tpconsdev is unlinked.
		*/
		}else if ((infp->tpinf_rdev == tp_consoledev) && (!tpconsdev)){
			/*tpdp->tpd_userflags |= (infp->tpinf_flags & TPINF_CONSOLE);*/
			tpdp->tpd_userflags |= TPINF_CONSOLE;
		}
		/*
		 * Copy in real device's stat information from tpinf to the
		 * TP device.
		 */ 
		tpdp->tpd_realdevfsdev = infp->tpinf_rdevfsdev;
		tpdp->tpd_realdevino = infp->tpinf_rdevino;
		tpdp->tpd_realdevmode = infp->tpinf_rdevmode;

		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		/*
		* leaves only  tpdev_list_rwlck to lock the passed tpdp
		*/
		tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		
		RW_UNLOCK(tp_plumb_rwlck, pl_1);

		break;

	case TP_DATACONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		
		if (error = tp_conndata(TPC_CTRL, infp)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_DATADISCONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		if (!tpdp->tpd_datachp){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		/*
		* To prevent a process from disconnecting a channel that
		* it does not own, the TP driver assigns a new
		* "connection id" to a channel every time it allocates a
		* channel.  This connection id must be passed to the
		* disconnect routine in the info block and will be checked
		* against the one on the channel to be disconnected.  If
		* the two don't match, the disconnect will fail.
		*
		* If the dconnid supplied by the user is 0, the check is
		* not applied (this allows an unconditional disconnect
		* since the control channel is a form of privileged access
		* anyway).
		*/
		pl_2 = LOCK(tpdp->tpd_datachp->tpc_data_mutex, plstr);
		if (infp->tpinf_dconnid &&
		   (infp->tpinf_dconnid != tpdp->tpd_datachp->tpc_connid)){
			UNLOCK(tpdp->tpd_datachp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			error = EINVAL;
			reply = M_IOCNAK;
			break;
		}
		UNLOCK(tpdp->tpd_datachp->tpc_data_mutex, pl_2);
		tp_discdata(tpdp);
		pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		pl_1 = tp_doputnextctl1_upper(tpcp, pl_1, 'R');
		tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_DEFSAK:
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		if (error = tp_verifysak(infp)){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			break;
		}
		tp_newmsgs(tpcp);
		pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
		if (!(tpcp->tpc_sakmsg && tpcp->tpc_hupmsg)){
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			error = EAGAIN;
			reply = M_IOCNAK;
			break;
		}
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		tp_setsak(tpcp,infp);
		tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSCONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		if (error = tp_conncons(TPC_CTRL)){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			break;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSDISCONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		/*
		* Disconnect the read/input side of the cons channel from
		* the 'console' TP device
		*/
		if (error = tp_disccons()){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
			pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSSET:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		if (error = tp_consset(infp, &newtpdp)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, newtpdp, infp, FILLINFO_ALL);
			pl_1 = tp_doputnext_low(newtpdp, pl_1, 'R');
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);

		break;

	case TP_GETINF:
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case I_LINK:
	case I_PLINK:
		/*
		* need to set write lock because realrq is now protected
		* by tp_plumb_rwlck. also tp_resetcons needs it
		*/
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		if (tpdp->tpd_realrq){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EINVAL;
			break;
		}

		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		if (tpdp->tpd_sak.sak_type == saktypeUNDEF){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EINVAL;
			break;
		}


                /*
                * Allocated internal M_IOCTL message to be sent to the
                * real/physicall tty device.
                */
                if (!(ioctlmp = tp_allocioctlmsg(TCGETS, sizeof(struct termios))
)){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
                        reply = M_IOCNAK;
                        error = EAGAIN;
                        break;
                }


		/* LINTED pointer alignment */
		linkp = (struct linkblk *)mp->b_cont->b_rptr;
		tpdp->tpd_muxid = linkp->l_index;
		/*
		* Save the kind of link so the control channel close
		* routine can handle closes correctly.
		*/
		if (cmd == I_PLINK){
			tpdp->tpd_flags |= TPD_PERSIST;
		}
		/*
		* Save the tpdp in the bottom mux q_ptr for flow control
		* from the bottom mux to the top mux.
		*
		* NOTE: For Future Enhancement.  If/When the STREAMS linkblk
		* struct is expanded to include the device number of the
		* lower Stream, put an additional check here to verify that
		* the device being linked is the device that indicated
		* in TP_CONNECT.
		*/
		tpdp->tpd_realrq = RD(linkp->l_qbot);
		tpdp->tpd_realrq->q_ptr = tpdp;
		WR(tpdp->tpd_realrq)->q_ptr = tpdp;

		/*
		* If this is a 'console' device being linked (indicated by
		* the TPINF_CONSOLE flag, set it up as the console.
		*/
		if (tpdp->tpd_userflags & TPINF_CONSOLE){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);	
			tp_resetcons(tpdp);
			pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		}
		else {
			UNLOCK(tpdp->tpd_data_mutex, pl_2);	
		}


                /*
                * enqueue internal TCGETS M_IOCTL messsage on lower ctrl
                * channel's (aka real/physical tty device's) write Queue
                * and enable the Queue.
                */
                enableok(WR(tpdp->tpd_realrq));
                putq(WR(tpdp->tpd_realrq), ioctlmp);

		RW_UNLOCK(tp_plumb_rwlck, pl_1);
			
		break;

	case I_UNLINK:
	case I_PUNLINK:

		/*
		* set to write lock because realrq is protected by tp_plumb
		*/
		pl_1 = 	RW_WRLOCK(tp_plumb_rwlck, plstr);

		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		/*
		* if some upper write put/service procedure is
		* using this lower q, it would have set the TPD_BUSYNEXT
		* flag. If the flag is on we can't release the lower
		* because we are using it. in this case return a err to user
		*/
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		if(tpdp->tpd_flags & TPD_BUSYNEXT) {
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EAGAIN;
			break;
		}
			

		if (!tpdp->tpd_realrq){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EINVAL;
			break;
		}
		/*
		* TPINF_FAILUNLINK_IFDATA is only valid for I_PUNLINK.
		* (See tp.h for details)
		*/
		if (tpdp->tpd_datachp && (cmd == I_PUNLINK) &&
		   (tpdp->tpd_userflags & TPINF_FAILUNLINK_IFDATA)){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EBUSY;
			break;
		}
		/*
		* If this is the console device and the ioctl is I_PUNLINK
		* do not allow the unlink to occur.
		* NOTE:  We must allow the unlink to occur if the ioctl
		* is I_UNLINK, since the ioctl could have been issued from
		* a close.  In that case the mux is dismantled as far as
		* the Stream Head is concerned, regardless of whether or
		* not this function returns indication that the unlink
		* did not occur.
		*/
		if (tpdp == tpconsdev){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EBUSY;
			break;
		} 
		/* LINTED pointer alignment */
		linkp = (struct linkblk *)mp->b_cont->b_rptr;
		/*
		* If the mux_id does not match the mux_id of the channel,
		* disallow the unlink request (since this is not the channel
		* to be unlinked).  OK to fail I_UNLINKs here becuase we
		* have to assume if I_UNLINK was sent as a result of the ctrl
		* channel closing, the Stream Head calling functions are
		* sending the correct multiplexor id (l_index).
		*/
		if (linkp->l_index != tpdp->tpd_muxid){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = EINVAL;
			break;
		}

		/*
		* From the TP driver's point of view, mark the TP device
		* unlinked.  When this returns, the Stream Head function
		* will have to complete the unlink by re-assigning all
		* the the real/physical device Top Queue's (which is now
		* the TP devices lower ctrl channel Top Queue) fields to
		* Stream Head related information.  But between now ("now"
		* is after the code is executed between the next set of
		* Locks and when the Stream Head re-assigns
		* the Queue, tplrput() can still receive messages from
		* downstream.  If tplrput() receives any messages, it will
		* act as if if did not receive any.
		*/
		tpdp->tpd_realrq->q_ptr = (caddr_t)0;
		WR(tpdp->tpd_realrq)->q_ptr = (caddr_t)0;
		tpdp->tpd_realrq = (queue_t *)0;
		tpdp->tpd_flags &= ~TPD_PERSIST;
		if (tpdp->tpd_tcsetp){
			freemsg(tpdp->tpd_tcsetp);
			tpdp->tpd_tcsetp = (mblk_t *)0;
		}

		UNLOCK(tpdp->tpd_data_mutex, pl_2);

		/*
		* If this is the console being unlinked free or redirect
		* console information, and disconnect read/input side of
		* cons channel from the TP device (tpconsdev).
		*/
		if (tpdp == tpconsdev){
			tp_resetcons((struct tpdev *)NULL);
			pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		}
	
		RW_UNLOCK(tp_plumb_rwlck, pl_1);	
		break;
	/*
	* When a TCSET* ioctl comes down the control channel and the
	* TPD_DEFSAK flag is set, the TCSET message is held until the new
	* SAK is set.  This prevents disruption of the current data channel.
	* See tp_setsak() for a description of the TPD_DEFSAK flag and
	* tp_discdata() for a description of what happens when a data
	* channel disconnects.
	*/
	case TCSETS:
	case TCSETSW:
	case TCSETSF:
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);	
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}

		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		if (tpdp->tpd_realrq && (tpdp->tpd_flags & TPD_DEFSAK)){
			if (tpdp->tpd_tcsetp){
				freemsg(tpdp->tpd_tcsetp);
			}
			tpdp->tpd_tcsetp = copymsg(mp);
			if (!tpdp->tpd_tcsetp){
				reply = M_IOCNAK;
				error = EAGAIN;
			}else{
				/*
				* Set the credentials to sys_cred, since
				* this ioctl has now become an internal
				* ioctl.  When execution is completed on the
				* the ioctl (occurs after a data channel is
				* disconnected), it is executed on behalf of
				* the kernel.
				*/
				/* LINTED pointer alignment */
				((struct iocblk *)(tpdp->tpd_tcsetp->b_rptr))->ioc_cr = sys_cred;
			}
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			break;
		}
		/*
		*unlock even though falling through so as to not destroy
		* the code 
		*/
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		/* FALLTHRU */
	default:	/*Falls through*/
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		if (!(tpdp = tpcp->tpc_devp)) {
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		if (error = tp_stdioctl(cmd,mp,tpcp)){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			break;
		}
		if (error = tp_sendioctl(tpdp,tpcp,q,mp)){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			break;
		}
		pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		/*
		* we return here instead of 'break' and 'qreply' because
		* we will get the qreply from below
		*/

		return;
	} /*end switch*/
	/*
	* NO LOCKS at this point
	*/
	iocp->ioc_error = error;
	mp->b_datap->db_type = reply;
	qreply(q,mp);
}

/*
* STATIC void
* tp_lctrlioctl(queue_t *, mblk_t *)
*     process ioctls enqueued on the lower ctrl channel's write Queue.
*
* This function handles TCSET* type ioctls and the TCGETS ioctl.
* All other types of ioctls are thrown away.
*
* Calling/Exit State:
*     Locks: Enter with: None
*            Leave with: None
*            Sets:   tp_plumb_rwlck
*/

STATIC void
tp_lctrlioctl(queue_t *q, mblk_t *mp)
{
	struct tpdev	*tpdp;
	struct iocblk	*iocp;
	int			cmd;
	pl_t			pl_1;

	tpdp = (struct tpdev *)q->q_ptr;
	if (!tpdp){
		/*
		*+ This should not happen!
		*/
		cmn_err(CE_WARN,
			"Queue without TP device pointer in tp_lctrlioctl()\n");
		return;
	}

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	switch (cmd){
	case TCSETS:
	case TCSETSW:
	case TCSETSF:
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
	case TCGETS:
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		if (tp_sendioctl(tpdp, (struct tpchan *)0, q, mp)){
			freemsg(mp);
		}
		pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;
	default:
		/*
		*+ not only do we only have ioctls on the lower write queue,
		*+ the only ioctls allowed are the TCSET series.
		*/
		cmn_err(CE_WARN,
		 "tp_lctrlioctl(): ioctl not a TCSET* type or TCGETS; freeing message\n");
		freemsg(mp);
		break;
	}
}


/*
* STATIC	void
* tp_admioctl(queue_t *, mblk_t *)
*	Process ioctls on the administrative channel.
*
* Calling/Exit State:
*       Locks: Enter with: None
*              Leave with: None
*              Sets:   tp_plumb_rwlck at various times to either READ or WRITE,
*/
STATIC	void
tp_admioctl(queue_t *q, mblk_t *mp)
{
	struct tpchan	*tpcp;
	struct tpdev	*tpdp;
	struct iocblk	*iocp;
	struct tp_info	*infp;
	struct tpdev		*newtpdp;
	int			cmd;
	unsigned char		reply = M_IOCACK;
	int			error = 0;
	pl_t			pl_1;

	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		*+ This should never happen!
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tp_admioctl()\n");
		mp->b_datap->db_type = M_IOCNAK;
		/* LINTED pointer alignment */
		((struct iocblk *)(mp->b_rptr))->ioc_error = EIO;
		qreply(q, mp);
		return;
	}

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	/*
	* Do common functionality for all TP type ioctls.
	*/
	switch (cmd){
	case TP_DATACONNECT:
	case TP_CONSCONNECT:
	case TP_CONSDISCONNECT:
	case TP_CONSSET:
	case TP_GETINF:
		if ( iocp->ioc_count < TPINFOSZ){
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q,mp);
			return;
		}
		/* LINTED pointer alignment */
		infp = (struct tp_info *)mp->b_cont->b_rptr;
		break;
	default:
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_error = EINVAL;
		qreply(q,mp);
		return;
	}

	switch (cmd){
	case TP_DATACONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (error = tp_conndata(TPC_ADM, infp)){
			reply = M_IOCNAK;
		}else{
			tpdp = tp_finddev(infp->tpinf_rdev, 0);
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSCONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (error = tp_conncons(TPC_ADM)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpconsdev, infp, FILLINFO_ALL);
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSDISCONNECT:
		/*
		* Disconnect the read/input side of the cons channel from
		* the 'console' TP device
		*/
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (error = tp_disccons()){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpconsdev, infp, FILLINFO_ALL);
			pl_1 = tp_doputnext_low(tpconsdev, pl_1, 'R');
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSSET:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (error = tp_consset(infp, &newtpdp)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, newtpdp, infp, FILLINFO_ALL);
			pl_1 = tp_doputnext_low(newtpdp, pl_1, 'R');
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_GETINF:
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		tpdp = tp_finddev(infp->tpinf_rdev, 0);
		if (!tpdp){
			reply = M_IOCNAK;
			error = ENXIO;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;
	default:
		break;
	}

	mp->b_datap->db_type = reply;
	iocp->ioc_error = error;
	qreply(q, mp);
}

/*
* STATIC	int
* tp_conndata(int , struct tp_info *)
*     Connect a data channel to a real device based on information found in
*     the tp_info block pointed to by infp. 
*
* Calling/Exit State:
*     This routine returns 0 on success and an errno value on failure.
*
*     Locks: Enter with: tp_plumb_rwlck heald for WRITE
*            Leave with: Same
*            Sets:   tpc_data_mutex and tpd_data_mutex at various times
*/
STATIC	int
tp_conndata(int type, struct tp_info *infp)
{
	struct  tpchan	*tpdatap;
	struct	tpchan	*tpctrlp;
	struct	tpdev	*tpdp;
	pl_t	pl_1;
	pl_t	pl_2;

	/*
	* Make sure the specified external major device number is the
	* same as the TP external major device number.
	*/
	if (getemajor(infp->tpinf_ddev) != tpemaj){
		return (EINVAL);
	}
	tpdatap = tp_findchan(geteminor(infp->tpinf_ddev));
	if (!tpdatap){
		return (ENXIO);
	}
	/*
	* Make sure the data channel is not already connected and that
	* the data channel is not dirty (was not previously connected).
	*/
	pl_1 = LOCK(tpdatap->tpc_data_mutex, plstr);
	if (tpdatap->tpc_devp || (tpdatap->tpc_flags & TPC_DIRTY)){
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
		return (EBUSY);
	}
	tpdp = tp_finddev(infp->tpinf_rdev, 0);
	if (!tpdp){
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
		return (ENXIO);
	}
	pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
	if ((type == TPC_ADM) && (tpdp->tpd_flags & TPD_SAKSET)){
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
		return (EIO);
	}
	if (tpdp->tpd_flags & TPD_WAITTCSET){
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
		return (EBUSY);
	}
	/*
	* Check for both.  May be in the middle of disconnecting data channel.
	*/
	if (tpdp->tpd_datachp || tpdp->tpd_datarq){
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
		return (EBUSY);
	}
	/*
	* Allocate space for a M_IOCTL message, TCSETS ioctl,
	* which is to be sent to the real/physical device when all
	* channels connected for receiving input messages are
	* disconnected.
	*/
	if (!tpdp->tpd_discioctl){
		/*
		* tp_allocioctlmsg doesn't need or care about locks, we
		* need to call it though with tpd_data_mutex set because 
		* the return value is stored in tpdp
		*/
		if (!(tpdp->tpd_discioctl =
		 tp_allocioctlmsg(TCSETS, sizeof(struct termios)))){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			UNLOCK(tpdatap->tpc_data_mutex, pl_1);
			return (EAGAIN);
		}
	}
	/*
	* Make sure all appropriate message buffers are allocated.
	*/

	tpctrlp = tpdp->tpd_ctrlchp;
	if (tpctrlp){
		/*
		* pointing to a different upper channel now so must release locks
		* and lock the new one
		*/
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
		tp_newmsgs(tpctrlp);
		pl_1 = LOCK(tpctrlp->tpc_data_mutex, plstr);
		if (!(tpdp->tpd_ctrlchp->tpc_sakmsg &&
		     tpdp->tpd_ctrlchp->tpc_hupmsg)){
			UNLOCK(tpctrlp->tpc_data_mutex, pl_1);
			return (EAGAIN);
		}
		UNLOCK(tpctrlp->tpc_data_mutex, pl_1);
		
	}
	else {
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
	}
	tp_newmsgs(tpdatap);
	pl_1 = LOCK(tpdatap->tpc_data_mutex, plstr);
	pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
	if (!tpdatap->tpc_hupmsg || !tpdatap->tpc_trailmsg){
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
		return (EAGAIN);
	}
	if (!tpdatap->tpc_rq){
		/*
		*+ This should never happen, but make sure. This prevents
		*+ a panic when the SAK is NONE and data goes up the data
		*+ channel.
		*/
		cmn_err(CE_WARN,"Data channel has no queue.\n");
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		UNLOCK(tpdatap->tpc_data_mutex, pl_1);
		return (EIO);
	}
	/*
	* All is well, connect the data channel and clear the TPD_SAKSET
	* flag so SAK can be detected again.  If the SAK type is DATA,
	* also set the the SAK type to NONE, to prevent a SAK from being
	* generated by the next data packet.
	* If there is no other channel receiving input messages (currently
	* only the cons channel could be a possiblity) mark the data channel
	* as the channel to receive input messages. Don't allow interrupts
	* from stream data while connection is being made and the SAK type
	* is being changed.
	*/
	tpdp->tpd_datarq = tpdatap->tpc_rq;
	tpdp->tpd_datachp = tpdatap;
	if (tpdp->tpd_sak.sak_type == saktypeDATA){
		tpdp->tpd_sak.sak_type = saktypeNONE;
	}
	tpdp->tpd_userflags |= infp->tpinf_flags & DATACONNECTFLAGS;
	tpdp->tpd_flags &= ~TPD_SAKSET;
	tpdatap->tpc_devp = tpdp;
	if (!tpdp->tpd_inputchp){
		tpdp->tpd_inputrq = tpdatap->tpc_rq;
		tpdp->tpd_inputchp = tpdatap;
		/*
		* just to be safe enable the queues
		*/
		enableok(tpdp->tpd_inputrq);
		qenable(tpdp->tpd_inputrq);
	}
	/*
	* Always enable upper write queue because in MP we don't
	* have a right to look at q_first. service procedure will
	* handle case where there was no data
	*/
	enableok(WR(tpdatap->tpc_rq));
	qenable(WR(tpdatap->tpc_rq));
	UNLOCK(tpdp->tpd_data_mutex, pl_2);
	UNLOCK(tpdatap->tpc_data_mutex, pl_1);

	return (0);
}


/*
* STATIC void
* tp_dataioctl(queue_t *, mblk_t *)
*      process ioctls sent via the data channel
*
*      This routine filters ioctls sent down the data chan. I_LINK/I_PLINK and
*      I_UNLINK/I_PUNLINK ioctls are not allowed on the data channel. 
*      TCSET* type operations are checked to make sure they do not mask or
*      otherwise interfere with the SAK processing.  If a problem is detected,
*      an error is sent up the stream.
*
* Calling/Exit State:
*      Locks: Enter with: None
*             Leave with: None
*             Sets: tp_plumb_rwlck, tpd_data_mutex and tpc_data_mutex
*/
STATIC void
tp_dataioctl(queue_t *q, mblk_t *mp)
{
	struct tpchan	*tpcp;
	struct tpdev	*tpdp;
	struct iocblk	*iocp;
	struct   tp_info	*infp;
	unsigned char		reply = M_IOCACK;
	int			error = 0;
	int			cmd;
	pl_t			pl_1;
	pl_t			pl_2;

	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		*+ This should never happen!
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tp_dataioctl()\n");
		mp->b_datap->db_type = M_IOCNAK;
		/* LINTED pointer alignment */
		((struct iocblk *)(mp->b_rptr))->ioc_error = EIO;
		qreply(q, mp);
		return;
	}


	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	/* 
	* tp_dataioctl doesn't muck with setting any plumbing pointers
	* so we are O.K. to set tp_plumb_rwlck to READ and to acquie
	* the other locks
	*/
	pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
	pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
	
	/*
	* If the data channel is not connected or it is write blocked, no
	* ioctls are allowed.  Issue an EIO and get out right away. 
	*/

	if (!(tpdp = tpcp->tpc_devp) || (tpcp->tpc_flags&TPC_BLOCKED)){
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_error = EIO;
		qreply(q,mp);
		return;
	}

	/*
	* Do common functionality for all TP type ioctls.
	*/
	switch (cmd){
	case TP_GETINF:
		if ( iocp->ioc_count < TPINFOSZ){
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q,mp);
			return;
		}
		/* LINTED pointer alignment */
		infp = (struct tp_info *)mp->b_cont->b_rptr;
		break;
	default:
		break;
	}

	switch (cmd){
	case I_LINK:
	case I_PLINK:
	case I_UNLINK:
	case I_PUNLINK:
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		reply = M_IOCNAK;
		error = EINVAL;
		break;
	case TP_GETINF:
		/*
		* Must have privilege to get all tpinf information.
		*/
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		if (drv_priv(iocp->ioc_cr)){ /* returns 0 on success */
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_RESTRICTED);
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;
	default:
		if (!tpdp->tpd_realrq ){
			UNLOCK(tpcp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			error = ENXIO;
			break;
		}
		/*
		* NOTE: For Future Enhancement: May eventuall want to enqueue
		* ioctl onto data channel if it is not currently receiving
		* input messages and send the ioctl down once this channel
		* became the channel receiving input messages. (The case
		* that the data channel would not be the channel receiving
		* input will occur very rarely and only with the 'console'
		* TP device.
		*/

		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		if (error = tp_stdioctl(cmd,mp,tpcp)){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			break;
		}
		if (error = tp_sendioctl(tpdp,tpcp,q,mp)){
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			reply = M_IOCNAK;
			break;
		}
		pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		/*
		* Everything was successful. We return here instead of
		* replying since the reply will come back up stream from
		* the driver below us.
		*/
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return;
	}
	mp->b_datap->db_type = reply;
	iocp->ioc_error = error;
	qreply(q,mp);
}

/*
* STATIC int
* tp_conncons(int )
*     connect cons channel to read/input side of the TP device
*     labeled as the 'console' device (indicated by tpconsdev).
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck heald for WRITE
*            Leave with: Same
*            Sets:   tpc_data_mutex and tpd_data_mutex at various times
*/
STATIC int
tp_conncons(int type)
{
	struct tpchan	*tpcp;
	pl_t		pl_1;
	pl_t		pl_2;

	/*
	* Verify that a 'console' TP device exists. 
	*/
	if (!tpconsdev){
		return (ENXIO);
	}

	/*
	* Verify that the cons chan is open. 
	*/
	tpcp = tp_findchan(TP_CONSDEV);
	if (!tpcp){
		return (ENXIO);
	}

	/*
	* Verify that the cons chan is not already connected to the
	* read/input side of 'console' TP device.
	*/
	if (tpconsdev->tpd_inputchp == tpcp){
		return (EBUSY);
	}

	pl_1 = LOCK(tpcp->tpc_data_mutex, plstr);
	pl_2 = LOCK(tpconsdev->tpd_data_mutex, plstr);

	if (((type == TPC_ADM) || (type == TPC_CONS)) &&
	 (tpconsdev->tpd_flags & TPD_SAKSET)){
		UNLOCK(tpconsdev->tpd_data_mutex, pl_2);
		UNLOCK(tpcp->tpc_data_mutex, pl_1);
		return (EIO);
	}
	if (tpconsdev->tpd_flags & TPD_WAITTCSET){
		UNLOCK(tpconsdev->tpd_data_mutex, pl_2);
		UNLOCK(tpcp->tpc_data_mutex, pl_1);
		return (EBUSY);
	}
	/*
	* If the SAK type is DATA, set the the SAK type to NONE,
	* to prevent a SAK from being generated by the next data packet.
	* Clear indication that a SAK was entered.
	*/
	if (tpconsdev->tpd_sak.sak_type == saktypeDATA){
		tpconsdev->tpd_sak.sak_type = saktypeNONE;
	}
	tpconsdev->tpd_flags &= ~TPD_SAKSET;

	tpconsdev->tpd_inputchp = tpcp;
	tpconsdev->tpd_inputrq = tpcp->tpc_rq;

	/*
	* Always enable upper wrtte queue because in MP we don't
	* have a right to look at q_first. service procedure will
	* handle case where there was no data
	*/
	enableok(WR(tpcp->tpc_rq));
	qenable(WR(tpcp->tpc_rq));

	UNLOCK(tpconsdev->tpd_data_mutex, pl_2);
	UNLOCK(tpcp->tpc_data_mutex, pl_1);
	return (0);
}

/*
* STATIC void
* tp_consioctl(queue_t *, mblk_t *)
*       process ioctls sent via the cons channel.
* 	process restricted set of TP ioctls.
* 	I_LINK/I_PLINK and I_UNLINK/I_PUNLINK ioctls are not allowed.
* 	process all other ioctls if cons channel is connected to the
* 	'console' TP device for at least output and 'console' TP device
* 	has a real/physical device linked underneath.
* 
*       Must have privilege to do TP_CONSCONNECT, TP_CONSDISCONNECT, and
*       TP_CONSSET ioctls.
*
*      TCSET* type operations are checked to make sure they do not mask or
*      otherwise interfere with the SAK processing.  If a problem is detected,
*      an error is sent up the stream.
*     
* Calling/Exit State:
*      Locks: Enter with: None
*             Leave with: None
*             Sets:   tp_plumb_rwlck at various times to either READ or WRITE,
*     		  also tpd_data_mutex.
*/
STATIC void
tp_consioctl(queue_t *q, mblk_t *mp)
{
	struct tpchan	*tpcp;
	struct tpdev	*tpdp;
	struct iocblk	*iocp;
	struct   tp_info	*infp;
	struct	 tpdev		*newtpdp;
	unsigned char		reply = M_IOCACK;
	int			error = 0;
	int			cmd;
	pl_t			pl_1;


	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp){
		/*
		*+ This should never happen!
		*/
		cmn_err(CE_WARN,
			"Queue without channel pointer in tp_consioctl()\n");
		mp->b_datap->db_type = M_IOCNAK;
		/* LINTED pointer alignment */
		((struct iocblk *)(mp->b_rptr))->ioc_error = EIO;
		qreply(q, mp);
		return;
	}
	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	/*
	* Do common functionality for all TP type ioctls.
	*/
	switch (cmd){
	case TP_CONSCONNECT:
	case TP_CONSDISCONNECT:
	case TP_CONSSET:
		if (error = drv_priv(iocp->ioc_cr)){ /* returns 0 on success */
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = error;
			qreply(q,mp);
			return;
		}
		/* FALLTHRU */
	case TP_GETINF:	/*Falls through*/
		if ( iocp->ioc_count < TPINFOSZ){
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q,mp);
			return;
		}
		/* LINTED pointer alignment */
		infp = (struct tp_info *)mp->b_cont->b_rptr;
		break;
	default:
		break;
	}

	switch (cmd){
	case I_LINK:
	case I_PLINK:
	case I_UNLINK:
	case I_PUNLINK:
		reply = M_IOCNAK;
		error = EINVAL;
		break;
	case TP_GETINF:
		/*
		* Must have privilege to get all tpinf information.
		*/
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		tpdp = tpcp->tpc_devp;
		if (drv_priv(iocp->ioc_cr)){ /* returns 0 on success */
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_RESTRICTED);
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSCONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		tpdp = tpcp->tpc_devp;
		if (error = tp_conncons(TPC_CONS)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSDISCONNECT:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		tpdp = tpcp->tpc_devp;
		/*
		* Disconnect the read/input side of the cons channel from
		* the 'console' TP device
		*/
		if (error = tp_disccons()){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, tpdp, infp, FILLINFO_ALL);
			pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		}
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	case TP_CONSSET:
		pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
		if (error = tp_consset(infp, &newtpdp)){
			reply = M_IOCNAK;
		}else{
			tp_fillinfo(tpcp, newtpdp, infp, FILLINFO_ALL);
			pl_1 = tp_doputnext_low(newtpdp, pl_1, 'R');
		}

		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		break;

	default:
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
		tpdp = tpcp->tpc_devp;
		/*
		* If the cons channel is not connected or the 'console' device
		* does not have a real/physical device linked underneath,
		* all other ioctls are not allowed.
		*/
		if (!tpdp){
			reply = M_IOCNAK;
			error = EIO;
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			break;
		}
		if (!tpdp->tpd_realrq ){
			reply = M_IOCNAK;
			error = ENXIO;
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			break;
		}

		if (error = tp_stdioctl(cmd,mp,tpcp)){
			reply = M_IOCNAK;
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			break;
		}
		if (error = tp_sendioctl(tpdp,tpcp,q,mp)){
			reply = M_IOCNAK;
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			break;
		}
		pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		/*
		* Everything was successful. We return here instead of
		* replying since the reply will come back up stream from
		* the driver below us.
		*/
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return;
	}
	mp->b_datap->db_type = reply;
	iocp->ioc_error = error;
	qreply(q,mp);
}

/*
* STATIC int
* tplrput(queue_t *, mblk_t *)
*     lower read put procedure
*
*     This routine processes all M_DATA, M_BREAK, M_HANGUP, M_SIG and
*     M_PCSIG messages to detect SAK conditions.  If no SAK is detected the
*     message is sent up the channel indicated by tpd_inputchp;
*     It also makes sure M_IOCNACK and M_IOCACK messages are directed to the
*     correct channel and handles M_FLUSH messages.
*
*     When an M_IOCACK message is received and the ioctl was a TCSET* the 
*     TPD_WAITTCSET flag is cleared.
*
* Calling/Exit State:
*     Locks: Enter with: None
*            Leave with: None
*            Sets:   tp_plumb_rwlck at various times to either READ or WRITE,
*    		  also tpd_data_mutex.
*/
STATIC int
tplrput(queue_t *q, mblk_t *mp)
{
	struct tpdev	*tpdp;
	struct tpchan	*datap;
	struct tpchan	*inputp;
	struct tpchan	*ctrlp;
	struct tpchan	*chanp;
	char		*cp,*endp;
	char		sakc;
	uint		sakfound = 0;
	pl_t		pl_1;
	pl_t		pl_2;
	int		type;

	tpdp = (struct tpdev *)q->q_ptr;

	/*
	* If the message is an M_FLUSH process it regardless of whether
	* or not the device is linked and return.
	* Handling this M_FLUSH may have come from stream head as a result of
	* sending a putctl1() for a M_ERROR message from tp_discdata().
	* tp_discdata() does not clear tpd_inputrq until after the putctl1()
	* completes which is after this M_FLUSH has completed (assuming
	* M_FLUSH message are not enqueued on any Queues).
	* NOTE: checks for tpd_inputrq rather then tpd_inputchp since the
	* channel may be in the middle of disconnecting.  (See tp_discdata()).
	*/
	if (mp->b_datap->db_type == M_FLUSH){
		type = *mp->b_rptr;
		if (type & FLUSHRW){
			/*
			* do our duty as a 'STREAM HEAD'
			*/
			if (type & FLUSHR){
				if (type & FLUSHBAND){
					flushband(q, *(mp->b_rptr + 1),
					 FLUSHDATA);
				}else{
					flushq(q, FLUSHDATA);
				}
			}
			if (type & FLUSHW){
				if (type & FLUSHBAND){
					flushband(WR(q), *(mp->b_rptr + 1),
					 FLUSHDATA);
				}else{
					flushq(WR(q), FLUSHDATA);
				}
				*mp->b_rptr &= ~FLUSHR;
				qreply(q, mp);
			}else{
				freemsg(mp);
			}

			/*
			* If there is a channel connected for input, send
			* M_FLUSH message up that channel.  Otherwise send
			* the M_FLUSH message back down stream if the M_FLUSH
			* message indicated to flush the write Queue.
			*/
			pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
			if (tpdp && tpdp->tpd_inputchp){
				inputp = tpdp->tpd_inputchp;
				pl_2 = LOCK(inputp->tpc_data_mutex, plstr);
				inputp->tpc_flags |= TPC_BUSYNEXT;
				UNLOCK(inputp->tpc_data_mutex, pl_2);
				RW_UNLOCK(tp_plumb_rwlck, pl_1);

				putnextctl1(tpdp->tpd_inputrq, M_FLUSH, type);

				pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);

				/*
				* inputp is locked by TPC_BUSYNEXT
				*/
				pl_2 = LOCK(inputp->tpc_data_mutex, plstr);
				inputp->tpc_flags &= ~TPC_BUSYNEXT;
				UNLOCK(inputp->tpc_data_mutex, pl_2);

				tp_wakeall(tpdp);
			}
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
		}else{
			freemsg(mp);
		}
		return (0);		
	}

	/*
	* come here with no locks held
	*/

	if (!tpdp){
		/*
		* Most likely in the middle of an I_UNLINK and the 
		* TP driver may have marked us as unlinked but the
		* Stream Head has not re-assigned the Queue's fields
		* (ie. q_qinfo etc.).  Just free message and return.
		*/
		freemsg(mp);
		mp = (mblk_t *)0;
		return (0);
	}

	pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);

	datap = tpdp->tpd_datachp;
	inputp = tpdp->tpd_inputchp;
	ctrlp = tpdp->tpd_ctrlchp;

	if (!tpdp->tpd_realrq){
		/*
		*+ If the message was not an M_FLUSH the device must be linked,
		*+ otherwise the message is ignored.
		*/
		cmn_err(CE_WARN,
		 "tplrput:Message received TP device indicated no read queue.\n");
	}

	/*
	* enter switch plumb lock, leave with it set
	*/
	switch (mp->b_datap->db_type){
	case M_DATA:
		/*
		* Handle M_DATA messages as fast as possible for
		* saktypeNONE.  There is no need to test for sakfound or
                * TPC_BLOCKED at end of switch since neither will be set
                * while sak type is NONE
                *
                * HISTORICAL NOTE: (Prior to the delay sak type change
                * functionality; When a non-zero delay time value is defined
                * can no longer assume that there is a data channel connected
                * when the SAK type is NONE)
                * If the SAK type is NONE a data or the cons channel must be
                * connected (and the data or cons queue in place), so there
                * is no need to verify that these pointers are okay.
		*/
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		if (tpdp->tpd_sak.sak_type == saktypeNONE){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			inputp = tpdp->tpd_inputchp;
			if (inputp){
				/*
				*for MP we have made all data go through
				* the lower read service procedure
				* in order to avoid the possibility of out
				* of order messages. Note, if this should
				* prove to be a performance problem, an
				* elaborate locking mehanisim can be done
				*/
				if (pcmsg(mp->b_datap->db_type)){
					pl_2 = LOCK(inputp->tpc_data_mutex, plstr);
					inputp->tpc_flags |= TPC_BUSYNEXT;

					UNLOCK(inputp->tpc_data_mutex, pl_2);
					RW_UNLOCK(tp_plumb_rwlck, pl_1);

					putnext(tpdp->tpd_inputrq, mp);

					pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
					/*
					* must have TPD_BUSYNEXT off 
				        * before calling tp_wakeall
					*/
					/*
					* inputp is locked by TPC_BUSYNEXT
					*/
					pl_2 = LOCK(inputp->tpc_data_mutex, plstr);
					inputp->tpc_flags &= ~TPC_BUSYNEXT;
					UNLOCK(inputp->tpc_data_mutex, pl_2);

					tp_wakeall(tpdp);
				}else{
					putq(q, mp);
				}
			}else{
				/*
				* A data or the cons channel is not connected
				* so "drop the message on the floor".
				*/
				freemsg(mp);
				mp = (mblk_t *)0;
			}
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			return (0);
		}
		/*
		* If the SAK type is DATA and there is a ctrl channel
		* connected, then issue a 'fake' SAK to the ctrl channel
		* and set the SAKSET flag to prevent adm channel connections.
		* A data channel or the cons channel is not connected
		* to receive input so the code to handle re-directing input
		* messages upstream is skipped
		*/
		if (tpdp->tpd_sak.sak_type == saktypeDATA){
			if (tpdp->tpd_ctrlchp){
				sakfound = 1;
			}
		}else if (tpdp->tpd_sak.sak_type == saktypeCHAR){
			sakc = tpdp->tpd_sak.sak_char;
			cp = (char *)mp->b_rptr;
			endp = (char *)mp->b_wptr;
			while (cp < endp){
				/*
				* Mask out the high order (8th) bit to prevent
				* potential spoofing from a terminal set for 7
				* bits or sending parity
				*/
				if (((*cp) & ASCII_7BITMASK) == sakc){
					sakfound = 1;
					break;
				}
				cp++;
			}
		}
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		break;

	/*
	* M_SIG, M_PCSIG cases and test for SIGHUP put here in case a
	* module or driver below converts a M_HANGUP to M_SIG or M_PCSIG
	*/
	case M_SIG:
	case M_PCSIG:
		if (*mp->b_rptr != SIGHUP){
			break;
		}

	/* FALLTHRU */
	case M_HANGUP:	/* If the signal was SIGHUP fall through*/
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		if (((tpdp->tpd_sak.sak_type == saktypeLINECOND) &&
		    (tpdp->tpd_sak.sak_linecond == saklinecondLINEDROP)) ||
		    (tpdp->tpd_sak.sak_secondary == saksecYES)){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			sakfound = 1;
			break;
		}
		/* 
		* This is not a SAK, but just a hangup condition.  In this
		* case, send the M_HANGUP up the data channel. If a ctrl
		* channel is connected, send the hangup notification up the
		* ctrl channel as well.
		* NOTE: The hangup message is specifically sent up the data
		* channel and not the channel labeled input (although the
		* data and input are usually the same channel) since this
		* could be the cons channel and we do not want to put the
		* cons channel in the STRHUP state.
		*/
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		if (datap){
			pl_2 = LOCK(datap->tpc_data_mutex, plstr);
			datap->tpc_flags |= TPC_BUSYNEXT;
			UNLOCK(datap->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);

			putnext(tpdp->tpd_datarq, mp);

			/*
			* this time need write lock for possible tp_discdata
			*/
			pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
			/*
			* re-aquire all ptrs again
			*/
			inputp = tpdp->tpd_inputchp;
			ctrlp = tpdp->tpd_ctrlchp;

			/*
			* must have TPC_BUSYNEXT off 
			* before calling tp_wakeall
			*/
			/* 
			* datap is locked by TPC_BUSYNEXT
			*/
			pl_2 = LOCK(datap->tpc_data_mutex, plstr);
			datap->tpc_flags &= ~TPC_BUSYNEXT;
			UNLOCK(datap->tpc_data_mutex, pl_2);
			/*
			* even though TPC_BUSYNEXT locked the actual data
			* channel in place so that we had a right to use it
			* above, the pointer from tpdp may indeed have
			* changed. so we re-acquire it now
			*/

			datap = tpdp->tpd_datachp;

			tp_wakeall(tpdp);
			pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
			if (tpdp->tpd_flags & TPINF_DISCONNDATA_ONHANGUP){
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
				tp_discdata(tpdp);
				pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
				pl_1 = tp_doputnextctl1_upper(datap, pl_1, 'R');
			}
			else {
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
			}
		}else{
			freemsg(mp);
		}
		/*
		* only tp_plumb held here
		*/
		if (ctrlp){ 
			/*
			* unlock pl_1 for us
			*/
			pl_1 = tp_sendmsg(ctrlp, tpdp->tpd_ctrlrq, TP_M_HANGUP, tpdp, pl_1);
			/*
			* re-aquire all ptrs again
			*/
			datap = tpdp->tpd_datachp;
			inputp = tpdp->tpd_inputchp;
			ctrlp = tpdp->tpd_ctrlchp;
			pl_1 = tp_sendmsg_trl(ctrlp, tpdp->tpd_ctrlrq, TP_M_HANGUP, tpdp, pl_1);
			/*
			* re-aquire all ptrs again
			*/
			datap = tpdp->tpd_datachp;
			inputp = tpdp->tpd_inputchp;
			ctrlp = tpdp->tpd_ctrlchp;
		}
		mp = (mblk_t *)0;
		break;

	case M_BREAK:
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		if ((tpdp->tpd_sak.sak_type == saktypeLINECOND) &&
		    (tpdp->tpd_sak.sak_linecond == saklinecondBREAK))
			sakfound = 1;
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		break;

	case M_IOCACK:
		/*
		* If ioctl command is a TCSET* type copy tpd_nextterm to
		* tpd_curterm.
		*	-NOTE: termios values are actually not reflected
		*	 in tp device until the M_IOCACK is passed back up
		*	 from the physcial device linked underneath.
		*
		* The TPD_WAITTCSET flag is cleared so data channels can
		* subsequently be connected.
                *
                * If ioctl command is a TCGETS and was internally generated
                * (i.e. tpd_ioctlchan == NULL) (refer to tp_ctrlioctl()
                * case I_LINK, I_PLINK) initialize tpd_curterm with the
                * termios values retrieved from the real/physicall tty device
                * linked below.
		*/

		/* LINTED pointer alignment */
		switch (((struct iocblk *)(mp->b_rptr))->ioc_cmd){

		case TCSETS:
		case TCSETSW:
		case TCSETSF:
		case TCSETA:
		case TCSETAW:
		case TCSETAF:

			pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
			bcopy((caddr_t)&(tpdp->tpd_nextterm),
			      (caddr_t)&(tpdp->tpd_curterm),
			      sizeof(struct termios));
			if (tpdp->tpd_flags & TPD_WAITTCSET){
				tpdp->tpd_flags &= ~(TPD_WAITTCSET|TPD_SAKSET);
			}
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			if (!tpdp->tpd_ioctlchan){
				freemsg(mp);
				mp = (mblk_t *)0;
			}
			break;

                case TCGETS:

                        if (!tpdp->tpd_ioctlchan){
                                if (mp->b_cont){
					pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
					/* LINTED pointer alignment */
                                        bcopy((struct termios *)mp->b_cont->b_rptr,
                                         &tpdp->tpd_curterm,
                                         sizeof(struct termios));
					UNLOCK(tpdp->tpd_data_mutex, pl_2);
                                }
                                freemsg(mp);
                                mp = (mblk_t *)0;
                        }
                        break;
		case TCGETA:
		default:
			break;
		}
		/* FALLTHRU */
	case M_IOCNAK:	/* fall through */
	case M_COPYIN:
	case M_COPYOUT:
		/*
		* If the channel for the ioctl response is connected and the
		* saved ioctl id on the TP device matches the ioctl id of the
		* M_IOCACK or M_IOCNAK message send the message up appropiate
		* channel.  If some other channel is waiting for the current
		* ioctl to complete, enable the write queue of that channel.
		* This is done be enabling (scheduling) the lower ctrl
		* channel's Write Queue service function, tplwsrv().
		* There can be more than one channel including the lower ctrl
		* channel waiting for the current ioctl to complete. tplwsrv()
		* enables (schedules) the Write Queue service function of
		* any upper Stream channel that has messages enqueued on its
		* write Queues or has an ioctl (saved in the channel structure)
		* to send downstream.  tplwsrv() also processes messages on
		* the lower Stream channel if there are any.
		* NOTE: Internally generated ioctls are enqueued on the
		* lower ctrl channel's write Queue (aka the real write Queue).
		*
		* NOTE: For Future Enhancement: The TP driver's ioctl
		* serialization relies on M_IOCTLs returning M_IOCACK/M_IOCNAK
		* messages.  After processing the M_IOCACK/M_IOCNAK,  the
		* lower ctrl channel Write Queue service function, tplwsrv()
		* would be scheduled if there were any ioctls waiting to be
		* sent downstream.  If tty type device drivers, which get
		* linked underneath, ever flushed M_IOCTL messages (this would
		* not be an expected functionality), the TP driver's ioctl
		* handling functionality would be impared.  The result is that
		* any other ioctl waiting to be sent downstream would not have
		* their respective channel Write Queue service function
		* scheduled, unless the Write Queue is enabled by some other
		* means (eg. backenabled by the lower Stream {the one
		* associated with the real/physical device} if its Write Queue
		* were previously full and subsequently the Queue message
		* count dropped below the lo-water mark.)
		* The impairment could be reduced, if checks for saved
		* (pending) ioctls on the channel were put in the Write Queue's
		* put function, but this would slow down write processing.
		*/

		/*
		* If the ioctlchan is NULL, (the ioctl came from inside the
		* driver), the channel is no longer connected to the TP
		* device, or the ioctl id of the message (M_IOCACK, M_IOCNAK,
		* M_COPYIN, or M_COPYOUT) does not match the saved ioctl id in
		* the TP device, the message is freed.
		* NOTE:  The iocblk casting is used on the message even if
		* the message is a M_COPYIN or M_COPYOUT.  This is all right
		* to do since the definition of the first three fields
		* (including the ioctl id) of a iocblk and copyreq structure
		* correspond to each other.
		*/
		chanp = tpdp->tpd_ioctlchan;
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		if (chanp && chanp->tpc_devp &&
		/* LINTED pointer alignment */
		 (((struct iocblk *)(mp->b_rptr))->ioc_id == tpdp->tpd_ioctlid)){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			pl_2 = LOCK(chanp->tpc_data_mutex, plstr);
			chanp->tpc_flags |= TPC_BUSYNEXT;
			UNLOCK(chanp->tpc_data_mutex, pl_2);

			RW_UNLOCK(tp_plumb_rwlck, pl_1);

			putnext(tpdp->tpd_ioctlchan->tpc_rq, mp);

			pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
			/*
			* re-aquire all ptrs again
			*/
			datap = tpdp->tpd_datachp;
			inputp = tpdp->tpd_inputchp;
			ctrlp = tpdp->tpd_ctrlchp;
			/* 
			* chanp is locked by TPC_BUSYNEXT
			*/
			/*
			* must have TPC_BUSYNEXT off 
			* before calling tp_wakeall
			*/
			pl_2 = LOCK(chanp->tpc_data_mutex, plstr);
			chanp->tpc_flags &= ~TPC_BUSYNEXT;
			UNLOCK(chanp->tpc_data_mutex, pl_2);
                	if (!(tpdp->tpd_flags & TPD_TRANSPARENT) ||
                        	(mp->b_datap->db_type == M_IOCACK) ||
                        	(mp->b_datap->db_type == M_IOCNAK)) {
                        	tpdp->tpd_ioctlchan = (struct tpchan *)0;
                        	pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
                        	tpdp->tpd_ioctlid = 0;
                        	tpdp->tpd_flags &= ~(TPD_BUSY_IOCTL|TPD_TRANSPARENT);
                        	if (tpdp->tpd_flags & TPD_WAIT_IOCTL){
                                	tpdp->tpd_flags &= ~(TPD_WAIT_IOCTL);
                                	enableok(WR(tpdp->tpd_realrq));
                                	qenable(WR(tpdp->tpd_realrq));
                        	}
                        	UNLOCK(tpdp->tpd_data_mutex, pl_2);
                	}
                	else {
                        	pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
                        	tpdp->tpd_flags &= ~TPD_TRANSPARENT;
                        	UNLOCK(tpdp->tpd_data_mutex, pl_2);
                	}
			tp_wakeall(tpdp);
		}else if (mp){
                	if (!(tpdp->tpd_flags & TPD_TRANSPARENT) ||
                        	(mp->b_datap->db_type == M_IOCACK) ||
                        	(mp->b_datap->db_type == M_IOCNAK)) {
                        	tpdp->tpd_ioctlchan = (struct tpchan *)0;
                        	tpdp->tpd_ioctlid = 0;
                        	tpdp->tpd_flags &= ~(TPD_BUSY_IOCTL|TPD_TRANSPARENT);
                        	if (tpdp->tpd_flags & TPD_WAIT_IOCTL){
                                	tpdp->tpd_flags &= ~(TPD_WAIT_IOCTL);
                                	enableok(WR(tpdp->tpd_realrq));
                                	qenable(WR(tpdp->tpd_realrq));
                        	}
                        	UNLOCK(tpdp->tpd_data_mutex, pl_2);
                	}
                	else {
                        	tpdp->tpd_flags &= ~TPD_TRANSPARENT;
                        	UNLOCK(tpdp->tpd_data_mutex, pl_2);
                	}
			freemsg(mp);
		}
		else {
			tpdp->tpd_ioctlchan = (struct tpchan *)0;
                        tpdp->tpd_ioctlid = 0;
                        tpdp->tpd_flags &= ~(TPD_BUSY_IOCTL|TPD_TRANSPARENT);
                        if (tpdp->tpd_flags & TPD_WAIT_IOCTL){
                                tpdp->tpd_flags &= ~(TPD_WAIT_IOCTL);
                                enableok(WR(tpdp->tpd_realrq));
                                qenable(WR(tpdp->tpd_realrq));
                        }
                        UNLOCK(tpdp->tpd_data_mutex, pl_2);
		}
		/*
		* only plumb_mutext held at this point
		*/
		mp = (mblk_t *)0;
		break;

	default:
		break;
	}

	/*
	* come here with plumbing lock set 
	*/

	/*
	* If the SAK is found, ignore all data in the message containing
	* the SAK.
	*
	* If no other SAK is pending (the SAKSET flag is not set on the TP
	* device) set the SAKSET flag.
	*
	*	If the data channel is connected send a HUP up the data
	*	channel and set the BLOCKED flag in the data channel.
	*
	*	If the ctrl channel is connected, send the SAK
	*	notification up the control channel. 
	*
	*	If the ctrl channel is not connected, disconnect the
	*	data channel (this will send an EIO up the data channel)
	*	and the channel receiving input messages.
	* 
	* If no SAK was found, and there is a data message to send, and the
	* read queue is not blocked either send or queue the message.
	* If the read queue is blocked, ignore the message.
	*/

	if (sakfound){
		if (mp){
			freemsg(mp);
			mp = (mblk_t *)0;
		}
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		if (!(tpdp->tpd_flags & TPD_SAKSET)){
			tpdp->tpd_flags |= TPD_SAKSET;
			if (datap && datap->tpc_devp){
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
				/*
				* tp_sendmsg will unlock pl_1 for us
				*/
				pl_1 = tp_sendmsg(datap, tpdp->tpd_datarq, TP_M_HANGUP, tpdp, pl_1);
				/*
				* re-aquire ptrs again
				*/
				datap = tpdp->tpd_datachp;
				pl_1 = tp_sendmsg_trl(datap, tpdp->tpd_datarq, TP_M_HANGUP, tpdp, pl_1);
				/*
				* woops! need to release lock and
				* get it for write for the tp_discinput
				*/
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
				pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
				/*
				* re-aquire ptrs again
				*/
				datap = tpdp->tpd_datachp;

				pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
				datap->tpc_flags |= TPC_BLOCKED;
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
				tp_discinput(tpdp);
				pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
			}
			if (ctrlp){
				/*
				* both locks held
				*/
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
				/*
			        * tp_sendmsg will unlock then lock for us
				*/
				pl_1 = tp_sendmsg(ctrlp, tpdp->tpd_ctrlrq, TP_M_SAK, tpdp, pl_1);
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
			}else{
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
				pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
			
				tp_discinput(tpdp);
				tp_discdata(tpdp);
				pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
				pl_1 = tp_doputnextctl1_upper(datap, pl_1, 'R');
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
			}
		}
		else {
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
		}
	}else if (mp){
		
		if (inputp) {
			pl_2 = LOCK(inputp->tpc_data_mutex, plstr);
			if (inputp->tpc_devp && !(inputp->tpc_flags & TPC_BLOCKED)){
				/*
				*for MP we have made all data go through
				* the lower read service procedure
				* in order to avoid the possibility of out
				* of order messages. Note, if this should
				* prove to be a performance problem, an
				* elaborate locking mehanisim can be done
				*/
				if (pcmsg(mp->b_datap->db_type)){
					inputp->tpc_flags |= TPC_BUSYNEXT;
					UNLOCK(inputp->tpc_data_mutex, pl_2);
					RW_UNLOCK(tp_plumb_rwlck, pl_1);

					putnext(tpdp->tpd_inputrq, mp);

					pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
					/*
					* inputp is locked by TPC_BUSYNEXT
					*/
					/*
					* must have TPD_BUSYNEXT off 
					* before calling tp_wakeall
					*/
					pl_2 = LOCK(inputp->tpc_data_mutex, plstr);
					inputp->tpc_flags &= ~TPC_BUSYNEXT;
					UNLOCK(inputp->tpc_data_mutex, pl_2);

					tp_wakeall(tpdp);
					RW_UNLOCK(tp_plumb_rwlck, pl_1);
				}else{
					UNLOCK(inputp->tpc_data_mutex, pl_2);
					RW_UNLOCK(tp_plumb_rwlck, pl_1);
					putq(q, mp);
				}
			} else {
				UNLOCK(inputp->tpc_data_mutex, pl_2);
				RW_UNLOCK(tp_plumb_rwlck, pl_1);
				freemsg(mp);
				mp = (mblk_t *)0;
			}
		}else{
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			freemsg(mp);
			mp = (mblk_t *)0;
		}
	}
	else {
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
	}

	/*
	* no more locks held 
	*/
	return (0);
}


/*
* STATIC	pl_t
* tp_sendmsg(struct tpchan *, queue_t *, int , struct tpdev *, pl_t )
*     Allocate new messages for a ctrl or data channels and send the one that
*     corresponds to the specified message type up the q.  If the message is not
*     currently allocated and cannot be allocated, don't send anything.  Having
*     separate sak and hup messages ensures that there will always be a SAK
*     message prepared to go up the stream (at least the first time a SAK is
*     detected) even if a HANGUP message comes at a time when no memory is
*     available.
*
* Calling/Exit State:
*     tpdp is passed in order to provide a context for setting the TPD_PUTNEXT
*     flag
*
*     Locks: Enter with: tp_plumb_rwlck, it will release it then set it again.
*			NOTE: 'q' is entered here unlocked
*            Leave with: None
*            Sets:   tpc_data_mutex, tpd_data_mutex
*/
STATIC	pl_t
tp_sendmsg(struct tpchan *chan, queue_t *q, int msg, struct tpdev *tpdp, pl_t pl_1)
{
	mblk_t	*mp;
	pl_t	pl_2;

	tp_newmsgs(chan);
	pl_2 = LOCK(chan->tpc_data_mutex, plstr);

	switch (chan->tpc_type){
	case TPC_CTRL:
		switch (msg){
		case TP_M_HANGUP:
			mp = chan->tpc_hupmsg;
			chan->tpc_hupmsg = (mblk_t *)0;
			break;
		case TP_M_SAK:
			mp = chan->tpc_sakmsg;
			chan->tpc_sakmsg = (mblk_t *)0;
			break;
		default:
			mp = (mblk_t *)0;
			break;
		}
		break;
	case TPC_DATA:
		switch (msg){
		case TP_M_HANGUP:
			mp = chan->tpc_hupmsg;
			/* 
			* if don't have allocated trailmsg, don't send up
			* 'priming' byte
			*/
			if(!chan->tpc_trailmsg) {
				if(mp->b_wptr) {
					mp->b_wptr -= 1;
				}
			}
			chan->tpc_hupmsg = (mblk_t *)0;
			break;
		default:
			mp = (mblk_t *)0;
			break;
		}
		break;
	default:
		UNLOCK(chan->tpc_data_mutex, pl_2);
		return(pl_1);
	}
	if (mp){
		chan->tpc_flags |= TPC_BUSYNEXT;
		UNLOCK(chan->tpc_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		putnext(q, mp);
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr); 
		/*
		* chan is ok (don't need to reacquire)
		* to use because TPC_BUSYNEXT is
		* preventing it from going away
		*/
		pl_2 = LOCK(chan->tpc_data_mutex, plstr);
		chan->tpc_flags &= ~TPC_BUSYNEXT;
		UNLOCK(chan->tpc_data_mutex, pl_2);
		tp_wakeall(tpdp);
	}
	else {
		UNLOCK(chan->tpc_data_mutex, pl_2);
	}

	return(pl_1);
}



/*
* STATIC	pl_t
* tp_sendmsg_trl(struct tpchan *, queue_t *, int , struct tpdev *, pl_t )
*     always called after tp_sendmsg to send a M_TRAIL if needed. It
*     was separated out from tp_sendmsg because we had to unlock
*     all from the M_HANGUP putnext, we didn't have a right to re-use chan
*     and tp pointers. we had no way of re-generating them at this level so
*     we leave it up to the higher level code to do it.
*    
* Calling/Exit State:
*     tpdp is passed in order to provide a context for setting the TPD_PUTNEXT
*     flag
*    
*     Locks: Enter with: tp_plumb_rwlck, it will release it then set it again.
*			NOTE: 'q' is entered here unlocked
*            Leave with: None
*            Sets:   tpc_data_mutex, tpd_data_mutex
*/
STATIC	pl_t
tp_sendmsg_trl(struct tpchan *chan, queue_t *q, int msg, struct tpdev *tpdp, pl_t pl_1)
{
	mblk_t	*mp;
	pl_t	pl_2;

	tp_newmsgs(chan);
	pl_2 = LOCK(chan->tpc_data_mutex, plstr);

	switch (chan->tpc_type){
	case TPC_DATA:
		switch (msg){
		case TP_M_HANGUP:
			/*
			* if there was no trailmsg for tp_sendmsg, and there
			* is one now, we are cool, since even though we
			* turned off the priming in m_hangup, the stream
			* head will just ignore this trailmsg
			*/
			mp = chan->tpc_trailmsg;
			chan->tpc_trailmsg = (mblk_t *)0;
			break;
		default:
			mp = (mblk_t *)0;
			break;
		}
		break;
	default:
		UNLOCK(chan->tpc_data_mutex, pl_2);
		return(pl_1);
	}
	if (mp){
		chan->tpc_flags |= TPC_BUSYNEXT;
		UNLOCK(chan->tpc_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		putnext(q, mp);
		pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr); 
		/*
		* chan is ok (don't need to reacquire)
		* to use because TPC_BUSYNEXT is
		* preventing it from going away
		*/
		pl_2 = LOCK(chan->tpc_data_mutex, plstr);
		chan->tpc_flags &= ~TPC_BUSYNEXT;
		UNLOCK(chan->tpc_data_mutex, pl_2);
		tp_wakeall(tpdp);
	}
	else {
		UNLOCK(chan->tpc_data_mutex, pl_2);
	}

	return(pl_1);
}

/*
* STATIC	void
* tp_newmsgs(struct tpchan *)
*     Allocate new messages for a channel based on the type of the
*     channel. Currently, all channels have the potential to have a SAK
*     message and a HANGUP message preallocated.  This routine is
*     responsible for preallocating those messages.
*    
*     NOTE: The initial allocation of SAK and HANGUP messages is not done
*     by this routine because this is done in the tpopen() process and is
*     allowed to sleep waiting for free memory.  This routine is only used
*     by parts of the driver that cannot reasonably sleep.
*    
* Calling/Exit State:
*     Locks: Enter with: tpcp locked (probably by tp_plumb_rwlck, or by 'q')
*            Leave with: SAME
*            Sets:       tpc_data_mutex 
*/
STATIC	void
tp_newmsgs(struct tpchan *tpcp)
{
	mblk_t	*p;
	pl_t pl_1;
	
	pl_1 = LOCK(tpcp->tpc_data_mutex, plstr);

	switch(tpcp->tpc_type){
	case TPC_CTRL:
		if (!tpcp->tpc_sakmsg){
			p=tpcp->tpc_sakmsg = allocb(sizeof(tpproto_t),BPRI_HI);
			if (p){
				p->b_datap->db_type = M_PCPROTO;
				/* LINTED pointer alignment */
				((tpproto_t *)(p->b_rptr))->tpp_type = TP_M_SAK;
				p->b_wptr = p->b_rptr + sizeof(tpproto_t);
			}
		}
		if (!tpcp->tpc_hupmsg){
			p=tpcp->tpc_hupmsg = allocb(sizeof(tpproto_t),BPRI_HI);
			if (p){
				p->b_datap->db_type = M_PROTO;
				/* LINTED pointer alignment */
				((tpproto_t *)(p->b_rptr))->tpp_type =
				 TP_M_HANGUP;
				p->b_wptr = p->b_rptr + sizeof(tpproto_t);
			}				
		}
		break;
	case TPC_DATA:
		tpcp->tpc_sakmsg = (mblk_t *)0;
		if (!tpcp->tpc_hupmsg){
			p = tpcp->tpc_hupmsg = allocb(0,BPRI_HI);
			if (p){
				p->b_datap->db_type = M_HANGUP;
				/*
				* phantom data byte to tell stream head 
				* no to set strhup flag yet, cause M_TRAIL
				* is to be sent
				*/
				p->b_wptr += 1;
			}				
		}
		if (!tpcp->tpc_trailmsg){
			p = tpcp->tpc_trailmsg = allocb(0,BPRI_HI);
			if (p){
				p->b_datap->db_type = M_TRAIL;
			}
		}
		break;
	default:
		/*
		* Messages are not needed for either ADM or CONS channels
		*/
		break;
	}

	UNLOCK(tpcp->tpc_data_mutex, pl_1);
}

/*
* STATIC int
* tplrsrv(queue_t *)
*     lower read service procedure
*
*     When the upper read service routine enables the read queue the lower
*     read service routine is called to dequeue and send the pending
*     message upstream.
*    
*    
* Calling/Exit State:
*     Locks: Enter with: None
*            Leave with: None
*            Sets:   tp_plumb_rwlck, tpc_data_mutex and tpd_data_mutex
*
*    	NOTE: The locking scheme here is very costly since we must re-aquire 
*    		all locks after a putnext. Since we made the decision that
*    		all data comes through here (tpuwput no longer does a putnext)
*    		this is a very used subroutine. Should we need to fix
*    		up performance, the locking scheme here should the first to be
*    		fine tuned.
*    		
*/

STATIC int
tplrsrv(queue_t *q)
{
	struct tpdev *tpdp;
	struct tpchan *inputp;
	mblk_t *mp;
	pl_t pl_1;
	pl_t pl_2;

	tpdp = (struct tpdev *)q->q_ptr;
	if (!tpdp){
		/*
		*+ This should never happen
		*/
		cmn_err(CE_WARN,
			"Encountered queue with no device in tplrsrv()\n");
		flushq(q, FLUSHALL);
		return (0);
	}

	/*
	* say we are in service procedure in order to force the put
	* procedure to putq. This is to prevent a race condition where
	* the service procedure is alive and dumping data, the put
	* comes along and does a putnext in the middle of the service
	* procedure, thus resulting in out of order data. 
	*
	* If we find no data here it could have falsely caused put to
	* do a putq. We don't have to worry about this because all it
	* is a is an extra trip here
	*/
	
	pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);

	/*
	** for historical purposes, datachp was used here. however,
	** due to MR ul92-31514 we needed to use inputchp. In reality inputchp
	** is the correct one.However the code analysis didn't show that this
        ** is always the case so just in case we use datachp if there is one
	** and inputchp otherwise.
	*/
	
	inputp = tpdp->tpd_datachp ? tpdp->tpd_datachp : tpdp->tpd_inputchp;

	/*
	* If the channel receiving input messages is
	* a) not connected, b) not linked, or c) blocked,
	* just flush the queue.
	*/
	if(inputp) {
		pl_2 = LOCK(inputp->tpc_data_mutex, plstr);
	}
	if (!inputp ||  (inputp->tpc_flags & TPC_BLOCKED)){
		if(inputp)
			UNLOCK(inputp->tpc_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		flushq(q, FLUSHALL);
	}else{
		/*
		* Channel receiving input is okay, send any pending messages
		* up the * channel until flow control says to stop.
		*/
		while (((mp = getq(q)) != (mblk_t *)0) &&
		 (canputnext(tpdp->tpd_inputrq))){
			inputp->tpc_flags |= TPC_BUSYNEXT;
			UNLOCK(inputp->tpc_data_mutex, pl_2);
			RW_UNLOCK(tp_plumb_rwlck, pl_1);
			putnext(tpdp->tpd_inputrq, mp);
			/*
			*re-aquire all locks
			*/
			pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
			/*
			* inputp locked by TPC_BUSYNEXT
			*/
			pl_2 = LOCK(inputp->tpc_data_mutex, plstr);
			inputp->tpc_flags &= ~TPC_BUSYNEXT;
			if (inputp->tpc_flags & TPC_BLOCKED){
				freemsg(mp);
				mp = (mblk_t *)0;
				flushq(q, FLUSHALL);
				break;

			}
		}
		if (mp){
			putbq(q, mp);
		}
		UNLOCK(inputp->tpc_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
	
	}
	return (0);
}

/*
* STATIC int
* tpursrv(queue_t *)
*      upper read service procedure
*
*      When a module upstream from the multiplexer is ready for more data
*      from the read queue it will back enable the queue which causes this
*      routine to run.  This routine enables the lower read service routine
*      which actually does the read.  This routine only operates on a data
*      or cons channel.
*
* Calling/Exit State:
*      Locks: Enter with: None
*             Leave with: None
*             Sets:   tpc_data_mutex, tp_plumb_rwlck, tpd_data_mutex
*/

STATIC int
tpursrv(queue_t *q)
{
	struct tpchan *tpcp;
	struct tpdev *tpdp;
	pl_t pl_1;

	tpcp = (struct tpchan *)q->q_ptr;
	if (!tpcp)
		return (0);
	pl_1 = RW_RDLOCK(tp_plumb_rwlck, plstr);
	tpdp = tpcp->tpc_devp;
	if (tpdp) {
		if ((tpcp->tpc_type == TPC_DATA) ||
	 	(tpcp->tpc_type == TPC_CONS) && tpdp->tpd_realrq){
			qenable(tpdp->tpd_realrq);
		}
	}
	RW_UNLOCK(tp_plumb_rwlck, pl_1);
	return (0);
}

/*
* STATIC	int
* tp_verifysak(struct tp_info *)
*     Make sure the specified SAK structure contains valid legal values. 
*     If the SAK is a character, it must be 0x00 <= c < 0x0E  OR  0x0F < c <= 1F.
*     If the SAK is a line condition it must be line drop or break. If all
*     is well return 0 otherwise return EINVAL.
*    
*     NOTE: 0x0E and 0x0F are excluded because their 0x8E and 0x8F are "single
*     shift" announcement characters for multi-byte character.  Since TP does
*     not test the 8th bit, 0x8E and 0x8F would be interpreted incorrectly. 
*    
* Calling/Exit State:
*     Locks: Enter with: None needed (it verifies mp coming down which is
*    			locked by streams. 
*            Leave with: SAME
*            Sets:       none 
*/

STATIC	int
tp_verifysak(struct tp_info *infp)
{
	switch(infp->tpinf_sak.sak_type){
	case saktypeNONE:
		return (0);
	case saktypeCHAR:
		{
			ulong	sakchar;
			sakchar = infp->tpinf_sak.sak_char;
			if ((!(sakchar & ~0x1F)) && (sakchar != 0x0E) &&
			 (sakchar != 0x0F))
				break;
			return (EINVAL);
		}
	case saktypeLINECOND:
		if ((infp->tpinf_sak.sak_linecond == saklinecondLINEDROP) ||
		   (infp->tpinf_sak.sak_linecond == saklinecondBREAK))
			break;
		return (EINVAL);
	default:
		return (EINVAL);
	}
	switch(infp->tpinf_sak.sak_secondary){
	case saksecYES:
	case saksecNO:
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

/*
* STATIC void
* tp_setsak(struct tpchan *, struct tp_info *)
*     Handle the decision making associated with setting a SAK on a TP
*     device. This routine is called when a TP_DEFSAK ioctl is recognized.
*    
*     If a TP_DEFSAK ioctl comes down the control channel and a channel is
*     connected to receive input (typically the data channel), the new SAK is
*     copied into a holding buffer in the TP device structure, the TPD_DEFSAK
*     flag is set and any pending TCSET* ioctl is freed.  This way, the TCSET*
*     must be sent down after the SAK is defined inorder for the tp device to be
*     able to connect another data channel (after the data channel has
*     disconnected) or the cons channel.
*
*     See tp_discdata() and tp_disccons() for an explanation of what happens
*     at a data and cons channel disconnect, and tp_ctrlioctl() for an
*     explanation of what happens to TCSET* ioctls.
*
*     NOTE: always call tp_verifysak() before calling tp_setsak()
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held
*            Leave with: SAME
*            Sets:       tpd_data_mutex 
*/
STATIC void
tp_setsak(struct tpchan *tpcp, struct tp_info *infp)
{
	struct sak	*sakp;
	struct	tpdev	*tpdp;
	pl_t		pl_1;

	sakp = &(infp->tpinf_sak);
	tpdp = tpcp->tpc_devp;
	/*
	* Since the data channel and the channel connected to receive input
	* will be disconnected before the SAK is actually put on the device,
	* translate SAKs of type NONE to type DATA so the initial SAK will
	* be correct.  This code should not be interrupt sensitive since a
	* SAK can't cause the data channel to disconnect when a ctrl channel
	* is connected.
	*/
	if (sakp->sak_type == saktypeNONE){
		sakp->sak_type = saktypeDATA;
	}

	if (tpcp->tpc_devp->tpd_inputchp){
	/*
	* If there is a data channel connected to the device,  hold the SAK. 
	*/
		pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
		tpdp->tpd_flags |= TPD_DEFSAK;
		if (tpdp->tpd_tcsetp){
			freemsg((mblk_t *)(tpdp->tpd_tcsetp));
			tpdp->tpd_tcsetp = (mblk_t *)0;
		}
		bcopy(sakp,&(tpdp->tpd_heldsak),sizeof(struct sak));
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
	}else{
		tp_putsak(tpdp,sakp);
	}
	/* 
	* IF SAK type is DATA, switch it back to NONE in the tp_info (infp)
	* structure, since infp is returned to user.
	*/
	if (sakp->sak_type == saktypeDATA){
		sakp->sak_type = saktypeNONE;
	}
}

/*
* STATIC void
* tp_putsak(struct tpdev *, struct sak *)
*
* NOTE: The delay time to change the SAK type from NONE to DATA, after the
* data or cons channel is disconnected, can be circumvented as follows:
* If the SAK for the TP device is defined again to saktypeNONE while a
* channel is connected for input it is translated into a saktypeDATA and
* held (made pending) until all channels to the TP device are disconnected
* for input.  When the channels have been disconnected tp_putsak() is called
* to load the new SAK definition.  If there was a delay time defined, the SAK
* would get set to saktypeDATA, before the delay time expired.  The code
* to prevent this is not implemented here since it is unlikely to occur
* and code needs to be surrounded by splhi()s which should be used as little
* as possible.
* If it becomes an issue and needs to be implemented, the following is a
* code fragment to be included:
*
*      -Before the bcopy (NOTE: NOT MP SAFE)
*       int s;
*       if (sakp->sak_type == saktypeDATA){
*              -prevent the timeout function from being called (via clock
*               interrput) while checking tpd_timeoutid.
*              s = splhi();
*              if (tpdp->tpd_timeoutid){
*                      -will get changed to saktypeDATA when the timeout
*                       function tp_saktypeswitch() is called
*                      sakp->sak_type = saktypeNONE;
*              }
*
*              -Where checking whether or not to set TPD_WAITTCSET need to
*               include case for saktypeNONE since that is now a valid sak
*               type in this function
*              if ((tpdp->tpd_sak.sak_type != saktypeDATA) &&
*               (tpdp->tpd_sak.sak_type != saktypeNONE){
*
* Calling/Exit State:
*    Locks: Enter with: tp_plumb_rwlck set
*           Leave with:	same 
*           Sets:  various tpd_data_mutex and tpc_data_mutex
*/
STATIC	void
tp_putsak(struct tpdev *tpdp, struct sak *sakp)
{
	queue_t	*q;
	struct termios	*maskp,*validp;
	pl_t pl_1;
	mblk_t	*mp;

	/*
	* since no partical way to lock sakp, there is a window from
	* the time it is unlocked before this call and this copy that
	* the data can change. In practice this will never change at the same
	* time. After the copt we take from the place just copied to
	* instead of sakp to be on the safe side
	*/
	pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
	bcopy(sakp,&(tpdp->tpd_sak),sizeof(struct sak));
	sakp = &(tpdp->tpd_sak);

	tpdp->tpd_flags &= ~TPD_DEFSAK;
	/*
	* Set termio(s) protection mask.  The choice of the mask to use
	* depends on the type of sak and possibly the value of the sak.
	*/
	switch(sakp->sak_type){
	case saktypeLINECOND:
		switch(sakp->sak_linecond){
		/*
		* tp_drop/tpbreak stuff is set in space.c so can never
		* change, so no locks needed
		*/
		case saklinecondLINEDROP:
			maskp =  &tp_dropmasktermios;
			validp = &tp_dropvalidtermios;
			break;
		case saklinecondBREAK:
			maskp =  &tp_breakmasktermios;
			validp = &tp_breakvalidtermios;
			break;
		default:
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
			return;
		}

		break;

	case saktypeCHAR:
		maskp = &tp_charmasktermios;
		validp = &tp_charvalidtermios;
		break;

	default:
		bzero(&(tpdp->tpd_mask), sizeof(struct termios));
		bzero(&(tpdp->tpd_valid), sizeof(struct termios));
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
		return;
	}
	bcopy(maskp, &tpdp->tpd_mask, sizeof(struct termios));
	bcopy(validp, &tpdp->tpd_valid, sizeof(struct termios));


	/* 
	* If saktype is saktypeNONE (initially == saktypeDATA before data
	* channel is connected) do not set TPD_WAITTCSET.  This will allow
	* the DATA channel to be connected whether or not a TCSET* type
	* of ioctl has been sent down.  There is no need to verify whether
	* or not a TCSET* type ioctl can comprise SAK detection, since the
	* SAK is defined to be NONE.
	*/
	if (tpdp->tpd_sak.sak_type != saktypeDATA){
		tpdp->tpd_flags |= TPD_WAITTCSET;
	}
	if (tpdp->tpd_tcsetp){
		q = WR(tpdp->tpd_realrq);
		/*
		* This is a queued ioctl that was sent down the ctrl channel
		* (after a TP_DEFSAK) and already "ACKed" (eventhough it was
		* not sent downstream).  tp_sendioctl()'s second arguement
		* (the ioctl chan) is set to NULL so that the ACK or NAK
		* that comes back upstream is not sent up the ctrl channel
		* since this ioctl was already "ACKed".
		*/

		/* 
		* make a copy of tcset so dont need tpd_data mutex
		* to go into tp_sendioctl
		*/
		mp = copymsg(tpdp->tpd_tcsetp);
		freemsg(tpdp->tpd_tcsetp);
		tpdp->tpd_tcsetp = (mblk_t *)0;

		UNLOCK(tpdp->tpd_data_mutex, pl_1);
	
		if (tp_sendioctl(tpdp,(struct tpchan *)0,q,mp)){
			freemsg(mp);
		}

	}
	else {
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
	}
}

/*
* STATIC int
* tp_sendioctl(struct tpdev *, struct tpchan *, queue_t *, mblk_t *)
*     Send an ioctl request downstream.  If an ioctl is currently being
*     processed downstream save this ioctl on the channel's data structure if
*     a channel arguement, tpcp, has been defined.  If tpcp is NULL, (indicates
*     an internally generated ioctl),  queue the ioctl at the beginning of
*     lower ctrl write Queue and disable that Queue for scheduling.
*     The write Queues for the channels that have the ioctl message saved or
*     the lower ctrl write Queue will be scheduled to run when the acknowledgement
*     for ioctl downstream is received.
*     If this is ioctl originated from the same channel as the ioctl that is
*     currently being processed downstream, the ioctl currently being processed
*     downstream either timed out or was interrupted at the Stream Head.  The
*     tpd_ioctlchan and tpd_ioctl fields are cleared out so that when the
*     acknowledgement for the current ioctl is returned to the TP device,
*     it is just freed.  NOTE: If the acknolwedgement was sent upstream, it
*     would have just been freed at the Stream Head.
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_lock. mp should be in a place that can't be
*    			changed under us. it usually comes from the real 'mp'
*    			but when internal it should come from a fresh
*    			local copy that can't be changed from another ioctl
*            Leave with:	same 
*            Sets:  various tpd_data_mutex and tpc_data_mutex. It actually unlocks
*    			tp_plumb_rwlck and then re-locks it back to the way it
*    			was
*/
STATIC int
tp_sendioctl(struct tpdev *tpdp, struct tpchan *tpcp, queue_t *q, mblk_t *mp)
{
	queue_t	*realwq;
	pl_t			pl_1;
	pl_t			pl_2;

	if(!tpdp) {
		return(EIO);
	}

	if(tpcp)
		pl_1 = LOCK(tpcp->tpc_data_mutex, plstr);

	pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
	if (!tpdp->tpd_realrq || !(realwq = WR(tpdp->tpd_realrq))){
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		if(tpcp)
			UNLOCK(tpcp->tpc_data_mutex, pl_1);
		return (EIO);
	}
	/*
	* If there currently is an ioctl saved on this channel, free it.
	* This indicates that the ioctl saved on this channel had either
	* interrupted or timed out at the Stream Head.  We free the
	* message here since it would just be freed at the Stream Head
	* if we were to send a negative ackowledgement upstream.
	*/
	if (tpcp && tpcp->tpc_ioctlmp){
		freemsg(tpcp->tpc_ioctlmp);
		tpcp->tpc_ioctlmp = (mblk_t *)0;
	}
	if (tpdp->tpd_flags & TPD_BUSY_IOCTL){
		/*
		* If ioctl originated from same channel as the ioctl currently
		* being processed downstream, clear tpd_ioctlchan and
		* tpd_ioctlid fields before saving ioctl on the channel.
		*/
		if (tpcp) {
		 	if (tpcp == tpdp->tpd_ioctlchan){
				tpdp->tpd_ioctlchan = (struct tpchan *)0;
				tpdp->tpd_ioctlid = 0; /* 0 is not a valid ioctl id */
				tpdp->tpd_flags &= ~TPD_TRANSPARENT;
			}
			tpdp->tpd_flags |= TPD_WAIT_IOCTL;
			tpcp->tpc_ioctlmp = mp;
		}else{
			tpdp->tpd_flags |= TPD_WAIT_IOCTL;
			noenable(q);
			putbq(q, mp);
		}
	}else{
		tpdp->tpd_flags |= TPD_BUSY_IOCTL;
		tpdp->tpd_ioctlchan = tpcp;
		/* LINTED pointer alignment */
		tpdp->tpd_ioctlid = ((struct iocblk *)(mp->b_rptr))->ioc_id;
		

                /*
		* signal for tp_doputnext to really do the putnext.
                * (before MP this used to be a putnext, see comments on
                * tp_doputnext for more explanataion)
                */
		ASSERT(!tpdp->tpd_putnextq);

                tpdp->tpd_putnextq = realwq;
                tpdp->tpd_putnextmp = mp;

	}
	UNLOCK(tpdp->tpd_data_mutex, pl_2);
	if(tpcp)
		UNLOCK(tpcp->tpc_data_mutex, pl_1);
	return (0);
}

/*
* STATIC	void
* tp_fillinfo(struct tpchan *, struct tpdev *, struct tp_info *, int )
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held
*            Leave with:	same 
*    	     use: tpd_data_mutex and tpc_data_mutex
*/
STATIC	void
tp_fillinfo(struct tpchan *tpcp, struct tpdev *tpdp, struct tp_info *infp, int fillflag)
{
	pl_t	pl_1;


	if (fillflag == FILLINFO_ALL){
		if (tpdp){
			if (tpdp->tpd_datachp){
				infp->tpinf_ddev = tpdp->tpd_datachp->tpc_dev;
				infp->tpinf_dconnid = tpdp->tpd_datachp->tpc_connid;
			}else{
				infp->tpinf_ddev = NODEV;
				infp->tpinf_dconnid = 0;
			}
			if (tpdp->tpd_ctrlchp){
				infp->tpinf_cdev = tpdp->tpd_ctrlchp->tpc_dev;
				infp->tpinf_cconnid = tpdp->tpd_ctrlchp->tpc_connid;
			}else{
				infp->tpinf_cdev = NODEV;
				infp->tpinf_cconnid = 0;
			}

			pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
			infp->tpinf_rdev = tpdp->tpd_realdev;
			infp->tpinf_rdevfsdev = tpdp->tpd_realdevfsdev;
			infp->tpinf_rdevino = tpdp->tpd_realdevino;
			infp->tpinf_rdevmode = tpdp->tpd_realdevmode;
			infp->tpinf_muxid = tpdp->tpd_muxid;
			bcopy(&(tpdp->tpd_sak), &(infp->tpinf_sak),sizeof(struct sak));
			infp->tpinf_flags = tpdp->tpd_userflags;
			bcopy(&(tpdp->tpd_valid),&(infp->tpinf_valid),
							sizeof(struct termios));
			bcopy(&(tpdp->tpd_mask), &(infp->tpinf_mask),
						sizeof(struct termios));
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
		}else{
			infp->tpinf_rdev = NODEV;
			infp->tpinf_rdevfsdev = NODEV;
			infp->tpinf_rdevino = 0;
			infp->tpinf_rdevmode = 0;
			if (tpcp->tpc_type == TPC_DATA){
				infp->tpinf_ddev = tpcp->tpc_dev;
				infp->tpinf_dconnid = tpcp->tpc_connid;
				infp->tpinf_cdev = NODEV;
				infp->tpinf_cconnid = 0;
			}			
			if (tpcp->tpc_type == TPC_CTRL){
				infp->tpinf_cdev = tpcp->tpc_dev;
				infp->tpinf_cconnid = tpcp->tpc_connid;
				infp->tpinf_ddev = NODEV;
				infp->tpinf_dconnid = 0;
			}else{			
				infp->tpinf_cdev = NODEV;
				infp->tpinf_cconnid = 0;
				infp->tpinf_ddev = NODEV;
				infp->tpinf_dconnid = 0;
			}
			infp->tpinf_muxid = 0;
			bzero(&(infp->tpinf_sak), sizeof(struct sak));
			infp->tpinf_flags = 0;
			bzero(&(infp->tpinf_valid), sizeof(struct termios));
			bzero(&(infp->tpinf_mask), sizeof(struct termios));
		}
		infp->tpinf_dev = tpcp->tpc_dev;
		infp->tpinf_connid = tpcp->tpc_connid;
	}else{ /* FILLINFO_RESTRICTED */
		if (tpdp){
			pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
			infp->tpinf_rdev = tpdp->tpd_realdev;
			infp->tpinf_rdevfsdev = tpdp->tpd_realdevfsdev;
			infp->tpinf_rdevino = tpdp->tpd_realdevino;
			infp->tpinf_rdevmode = tpdp->tpd_realdevmode;
			infp->tpinf_muxid = tpdp->tpd_muxid;
			bcopy(&(tpdp->tpd_sak), &(infp->tpinf_sak),sizeof(struct sak));
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
		}else{
			infp->tpinf_rdev = NODEV;
			infp->tpinf_rdevfsdev = NODEV;
			infp->tpinf_rdevino = 0;
			infp->tpinf_rdevmode = 0;
			infp->tpinf_muxid = 0;
		}
		infp->tpinf_ddev = NODEV;
		infp->tpinf_dconnid = 0;
		infp->tpinf_cdev = NODEV;
		infp->tpinf_cconnid = 0;
		infp->tpinf_flags = 0;
		bzero(&(infp->tpinf_valid), sizeof(struct termios));
		bzero(&(infp->tpinf_mask), sizeof(struct termios));
		infp->tpinf_dev = NODEV;
		infp->tpinf_connid = 0;
	}
}

/* 
* STATIC int
* tp_ioctlok(struct tpchan *, cred_t *)
*     Verify TCSET* type ioctls down from the data or cons channel can not
*     potentially interfere with SAK detection.  If the process has the
*     P_DRIVER privilege, do not make the check.
*     Privileged processes may need to set the initial termio setting via the 
*     data channels or cons channel to sync up termios structures in modules
*     and drivers above us.  If saktype is saktypeNONE (saktypeDATA when no
*     data channel is connected) do not do check.
*     If all is well return 0.
*    
*     NOTE: credp contains calling user's process credentials when MAC is
*     installed and the file's (associated with the ioctl) credentials when MAC is
*     NOT installed.
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held
*            Leave with:	same 
*    	     use: tpd_data_mutex and tpc_data_mutex
*/
STATIC int
tp_ioctlok(struct tpchan *tpcp, cred_t *credp)
{
	struct termios	*maskp,*nextp,*curp;
	struct tpdev		*tpdp;
	int			i;
	char			sak_cc;
	pl_t			pl_1;

	tpdp = tpcp->tpc_devp;
	if (!tpdp){
		return (EIO);
	}
	pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
	nextp = &tpdp->tpd_nextterm;
	if (((tpcp->tpc_type == TPC_DATA) || (tpcp->tpc_type == TPC_CONS)) &&
	 !((tpdp->tpd_sak.sak_type == saktypeDATA) ||
	 (tpdp->tpd_sak.sak_type == saktypeNONE))){
		/*
		* assume for now that can hold my locks across this call
		* code for it shows it is just a standard way of accessing
		* two bits in credp so this assumption is O.K.
		*/
		if (!drv_priv(credp)){ /* returns 0 on success */
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
			return (0);
		}
		maskp = &(tpdp->tpd_mask);
		curp = &(tpdp->tpd_curterm);
		/*
		* Make sure requested modes are compatible with the SAK.
		*/
		if ((nextp->c_iflag ^ curp->c_iflag) & maskp->c_iflag){
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
			return (EPERM);
		}
		if ((nextp->c_oflag ^ curp->c_oflag) & maskp->c_oflag){
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
			return (EPERM);
		}
		if ((nextp->c_cflag ^ curp->c_cflag) & maskp->c_cflag){
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
			return (EPERM);
		}
		if ((nextp->c_lflag ^ curp->c_lflag) & maskp->c_lflag){
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
			return (EPERM);
		}
		/*
		* Make sure all special special characters are compatible
		* with SAK.
		*/
		if (tpdp->tpd_sak.sak_type == saktypeCHAR){
			sak_cc = tpdp->tpd_sak.sak_char;
			for (i = 0;i < NCCS;i++){
				if (tpdp->tpd_nextterm.c_cc[i] == sak_cc){
					UNLOCK(tpdp->tpd_data_mutex, pl_1);
					return (EPERM);
				}
			}
		}
	}
	UNLOCK(tpdp->tpd_data_mutex, pl_1);
	return (0);
}

/*
* STATIC	void
* tp_discdata(struct tpdev *)
*     Disconnect the data channel.
*    
*     When a data channel disconnects from the TP device for any reason,
*     the TPD_DEFSAK flag is checked.  If the flag is set, tp_setsak() is called
*     and does the following:
*    	-the queued (pending) SAK defintion is set and the TPD_WAITTCSET flag
*    	 is set to prevent a data channel connection until an M_IOCACK is
*    	 received for a TCSET* command.
*    	-if a TCSET* ioctl is queued (pending), it is sent to the driver below.
*    
*     See tplrput() for an explanation of how TPD_WAITTCSET gets cleared.
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE
*            Leave with: same
*            Sets: tpd_data_mutex , tpc_data_mutex
*/
STATIC	void
tp_discdata(struct tpdev *tpdp)
{
	struct	tpchan	*tpcp;
	pl_t pl_1;
	pl_t pl_2;
	toid_t saveid;

	tpcp = tpdp->tpd_datachp;
	if (!tpcp){
		return;		/*No data channel to disconnect*/
	}
	pl_1 = LOCK(tpcp->tpc_data_mutex, plstr);
	pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
	tpdp->tpd_userflags &= ~DATACONNECTFLAGS;

	/*
	* Clear out the ioctl channel if it is the
	* data channel.
	*/
	if (tpdp->tpd_ioctlchan == tpcp){
		tpdp->tpd_ioctlchan = (struct tpchan *)0;
		tpdp->tpd_ioctlid = 0;
	}
	/*
	* Free any saved ioctl messages on the channel.
	*/
	if (tpcp->tpc_ioctlmp){
		freemsg(tpcp->tpc_ioctlmp);
		tpcp->tpc_ioctlmp = (mblk_t *)0;
	}
	tpcp->tpc_flags |= TPC_DIRTY;
	tpdp->tpd_datachp = (struct tpchan *)0;
	if (tpdp->tpd_inputchp == tpcp){
		tpdp->tpd_inputchp = (struct tpchan *)0;
	}
	
	/*
	* If the SAK type is NONE when a data channel is disconnected,
	* the SAK type changes to DATA.  This means that the first data
	* packet received from the real device will cause a SAK to be sent
	* up the control channel, notifying the process that has the
        * control channel of activity on the device.
        * NOTE: The SAK type is not changed from NONE to DATA if a time
        * interval to delay the change is defined.  The SAK type will be
        * changed from NONE to DATA when the timer expires.
	* NOTE: If there still is another channel on this TP device
	* (the cons channel may still be connected for input), do not
	* set to saktypeDATA.  We do not want a 'fake' SAK issued while
	* there is a channel connected that is receiving input messages.
        *
        * HISTORICAL NOTE: (Prior to the delay sak type change functionality;
        * When a non-zero delay time value is defined can no longer assume
        * that there is a data channel connected when the SAK type is NONE)
        * The processing for SAK type NONE in the lower read put routine
        * assumes that whenever the SAK is NONE there is a data channel
        * connected and and a read queue in place, so setting the SAK
        * type to DATA should not be interrupted by data coming up the stream.
	*/

	if ((tpdp->tpd_sak.sak_type == saktypeNONE) && !(tpdp->tpd_inputchp)){
                if (tp_saktypeDATA_switchdelay){
                        /*
                        * If an outstanding timeout is pending from a previous
                        * call to tp_discdata() of a previous data channel,
                        * clear it (via untimeout()) before calling timeout()
                        * for this data channel disconnect.
                        * If this is not done and the TP device's memory
                        * is freed  (in tpclose()) before the outstanding
                        * timeout() function, tp_saktypeswitch() is called,
                        * the system will panic.
                        * NOTE: tpclose() could not clear the outstanding
                        * timeout since its timeoutid would have been
                        * overwritten by this call to timeout().
                        */
                        if (tpdp->tpd_timeoutid){
				saveid=tpdp->tpd_timeoutid;
				UNLOCK(tpdp->tpd_data_mutex, pl_2);
                                untimeout(saveid);
				pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
                        }
                        tpdp->tpd_timeoutid = timeout(tp_saktypeswitch,
                         (caddr_t)tpdp, drv_usectohz(MILLITOMICRO(tp_saktypeDATA_switchdelay)));
                } else {
                        tpdp->tpd_sak.sak_type = saktypeDATA;
                }

	}

	/*
	* Once the data channel has been disconnected, any pending
	* requests should be woken up and receive an EIO. putctrl1()
	* M_ERROR will send back an M_FLUSH for read and write queues.
	*
	* putctl1() is called before the the data channel's tpc_devp is
	* cleared, so M_FLUSH will also flush every thing from the lower
	* mux on down (assuming that the M_FLUSH, is not enqueued onto any
	* Queue.
	*/

        /*
        * tell upper stuff to do a putnextctl1 after all locks have been
        * released
        */
	ASSERT(!tpcp->tpc_putnextctl1q);

        tpcp->tpc_putnextctl1q = tpcp->tpc_rq;
        tpcp->tpc_putnextctl1type = M_ERROR;
        tpcp->tpc_putnextctl1param = EIO;

	tpdp->tpd_datarq = (queue_t *)0;
	if (tpdp->tpd_inputrq == tpcp->tpc_rq){
		tpdp->tpd_inputrq = (queue_t *)0;
	}
	tpcp->tpc_devp = (struct tpdev *)0;
	/*
	* If no channel is connected to receive input and a new SAK has been
	* defined, make it the active SAK.
	* If no channel is connected to receive input drop real/physical
	* device's DTR
	*/
	if (!(tpdp->tpd_inputchp)){
 		if ((tpdp->tpd_flags & TPD_DEFSAK)){
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			UNLOCK(tpcp->tpc_data_mutex, pl_1);
			tp_putsak(tpdp, &(tpdp->tpd_heldsak));
		}
		else {
			UNLOCK(tpdp->tpd_data_mutex, pl_2);
			UNLOCK(tpcp->tpc_data_mutex, pl_1);
		}
		tp_senddiscioctl(tpdp);
	}
	else {
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		UNLOCK(tpcp->tpc_data_mutex, pl_1);
	}
}

/*
* STATIC int
* tp_disccons()
*     Disconnect the read side of cons channel from the TP device.
*
*     Restore ("re-connect") the data channel (if connected) as the channel to
*     direct upstream messages (ie. make data channel the active channel).
*
*     NOTE for a Future Enhancement: May eventually want to issue an internal
*     TCSET* ioctl to reset termios values when ever a channel (currently only
*     the data channel) is restored as the active channel.  Additional
*     functionality would also be needed to save the termios information at the
*     time the channel was made inactive (ie. would not receive upstream
*     messages)... For now since making channels "inactive" and "active" 
*     (currently only data channels) is rare occurrence (currently only the cons
*     channel would effect the active state other channels), we rely on the
*     user level application to restore termios information if neccessary.
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE
*            Leave with:	same 
*            Sets: tpd_data_mutex
*/
STATIC int
tp_disccons()
{
	struct tpchan	*tpcp;
	pl_t	pl_1;

	tpcp = tp_findchan(TP_CONSDEV);

	if (!tpcp || !tpconsdev) {
			return (ENXIO);
	}
	/*
	* tpcp and tpconsdev not null so O.K. to use 
	*/
	if(tpconsdev->tpd_inputchp != tpcp){
		return (ENXIO);
	}

	/*
	* No need to check whether channel becoming the channel to receive
	* input is connected or not.
	*/
	tpconsdev->tpd_inputchp = tpconsdev->tpd_datachp;
	pl_1 = LOCK(tpconsdev->tpd_data_mutex, plstr);
	tpconsdev->tpd_inputrq = tpconsdev->tpd_datarq;
	/*
	* If the SAK type is NONE the SAK type changes to DATA if there is
	* not another channel (the data channel) connected to this
	* TP device.  We do not want a 'fake' SAK issued while
	* there is a channel connected that is receiving input messages.
        * NOTE: The SAK type is not changed from NONE to DATA if a time
        * interval to delay the change is defined.  The SAK type will be
        * changed from NONE to DATA when the timer expires.
	*/
	if ((tpconsdev->tpd_sak.sak_type == saktypeNONE) &&
	 !(tpconsdev->tpd_inputchp)){
	        if (tp_saktypeDATA_switchdelay){
                        tpconsdev->tpd_timeoutid = timeout(tp_saktypeswitch,
                         (caddr_t)tpconsdev, drv_usectohz(MILLITOMICRO(tp_saktypeDATA_switchdelay)));
                } else {
                        tpconsdev->tpd_sak.sak_type = saktypeDATA;
                }
	}
	/*
	* If no channel is connected to receive input and a new SAK has been
	* defined, make it the active SAK.
	* If no channel is connected to receive input drop real/physical
	* devices DTR.
	*/
	if (!(tpconsdev->tpd_inputchp)){
 		if ((tpconsdev->tpd_flags & TPD_DEFSAK)){
			/*
			* the pointer inside tpconsev for tpd_heldsak
			* is locked by tp_plumb_rwlck. don't need to hold
			* tpd_data_mutex for it.
			*/
			UNLOCK(tpconsdev->tpd_data_mutex, pl_1);
			tp_putsak(tpconsdev, &(tpconsdev->tpd_heldsak));
		}
		else {
			UNLOCK(tpconsdev->tpd_data_mutex, pl_1);
		}
		tp_senddiscioctl(tpconsdev);
	}
	else {
		UNLOCK(tpconsdev->tpd_data_mutex, pl_1);
	}
	
	return (0);
}

/*
* STATIC void
* tp_discinput(struct tpdev *)
*     Disconnect the read/input side of TP device.
*
*     Since only disconnected read/input side, only needed to clear data
*     structures in the TP device (inputchp and inputrq).
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE
*            Leave with: same
*            Sets:      tpd_data_mutex
*/
STATIC void
tp_discinput(struct tpdev *tpdp)
{
	pl_t pl_1;

	pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
	
	tpdp->tpd_inputchp = (struct tpchan *)0;
	tpdp->tpd_inputrq = (queue_t *)0;
	
	UNLOCK(tpdp->tpd_data_mutex, pl_1);
}


/*
* STATIC void
* tp_senddiscioctl(struct tpdev *)
*     Sets up a TCSETS ioctl with the baud rate set to B0
*     and sends it downstream to the real/physical device.  "Setting" the baud
*     rate to B0, will cause the port to drop its out going DTR.
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held
*            Leave with:	same 
*            Sets:  various tpd_data_mutex;
*/

STATIC void
tp_senddiscioctl(struct tpdev *tpdp)
{
	mblk_t		*mp;
	struct termios	*tiosp;
	pl_t pl_1;

	pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
	mp = tpdp->tpd_discioctl;
	tpdp->tpd_discioctl = (mblk_t *)0;

	if (!mp){
		/*
		* This is OK. tpd_discioctl may not be set up if only the
		* cons channel was connected for input for the TP device.
		* tpd_discioctl is only set up when connecting the data
		* channel
		*/
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
		return;
	}

	/*
	* If no real/physical device is linked underneath or HUPCL flag in the
	* current termios settings (ie. tpd_curterm.c_cflag) is not set,
	* do not send ioctl.
	*/
	if ((!tpdp->tpd_realrq) || !(tpdp->tpd_curterm.c_cflag & HUPCL)){
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
		freemsg(mp);
		return;
	}

	/* LINTED pointer alignment */
	tiosp = (struct termios *)mp->b_cont->b_rptr;
	bcopy(&tpdp->tpd_curterm, tiosp, sizeof(struct termios));
	tiosp->c_cflag &= ~CBAUD;

	/* turn on B0 , which if B0 wasn't = 0 would look like :
        * 	tiosp->c_cflag |= B0; 
	* the statement is shown here for clarity
	*/

	/*
	* Schedule the ioctl, with the STREAMS scheduler, to be sent down
	* to the real/physical device.  This ioctl is always scheduled
	* since this function may be called from the interrupt level and
	* we want to minimize the number of stack frames on the interrupt
	* stack as much as possible.
	*/
	putq(WR(tpdp->tpd_realrq), mp);
	if (tpdp->tpd_flags & TPD_BUSY_IOCTL)
		tpdp->tpd_flags |= TPD_WAIT_IOCTL;
	else {
		enableok(WR(tpdp->tpd_realrq));
		qenable(WR(tpdp->tpd_realrq));
	}

	UNLOCK(tpdp->tpd_data_mutex, pl_1);
	return;
}


/*
* STATIC int
* tp_stdioctl(int , mblk_t *, struct tpchan *)
*     Filter sensitive standard ioctl calls.
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held
*            Leave with:	same 
*    	     use:  tpd_data_mutex;
*/
STATIC int
tp_stdioctl(int cmd, mblk_t *mp, struct tpchan *tpcp)
{
	struct termios	*nexttiosp;
	struct termios	*tiosp;
	struct termio	*tiop;
	struct tpdev	*tpdp;
	struct iocblk	*iocp;
	pl_t		pl_1;

	tpdp = tpcp->tpc_devp;
	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
	nexttiosp = &(tpdp->tpd_nextterm);

	switch(cmd){
	case TCSETS:
	case TCSETSW:
	case TCSETSF:
		/* 
		* Fail if no termios buffer attached
		*/
		if (!mp->b_cont){
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
			return (EINVAL);
		}
		/* LINTED pointer alignment */
		tiosp = (struct termios *)mp->b_cont->b_rptr;
		bcopy((caddr_t)tiosp,(caddr_t)nexttiosp,
		 sizeof(struct termios));
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
		return (tp_ioctlok(tpcp, iocp->ioc_cr));

	/*
	* old style termio 
	*/
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
		/*
		* Fail if no termio buffer attached
		*/
		if (!mp->b_cont){
			UNLOCK(tpdp->tpd_data_mutex, pl_1);
			return (EINVAL);
		}
		/* LINTED pointer alignment */
		tiop = (struct termio *)mp->b_cont->b_rptr;
		/*
		* Copy current termios to next termios to save the non-termio
		* portion of the termios definition.
		* Replace termio portion of tpd_nextterm (which has been
		* copied in from tpd_curterm) with the termio values sent
		* via the ioctl.
		*/
		bcopy((caddr_t)&(tpdp->tpd_curterm), (caddr_t)nexttiosp,
		 sizeof(struct termios));
		nexttiosp->c_iflag = (nexttiosp->c_iflag & HI16)|tiop->c_iflag;
		nexttiosp->c_oflag = (nexttiosp->c_oflag & HI16)|tiop->c_oflag;
		nexttiosp->c_cflag = (nexttiosp->c_cflag & HI16)|tiop->c_cflag;
		nexttiosp->c_lflag = (nexttiosp->c_lflag & HI16)|tiop->c_lflag;
		bcopy((caddr_t)tiop->c_cc,(caddr_t)nexttiosp->c_cc,NCC);
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
		return (tp_ioctlok(tpcp, iocp->ioc_cr));
	default:
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
		break;
	}
	return (0);
}

/*
* The following routines manage the minor device space of the trusted
* path driver.
*/

/*
* STATIC	void
* tp_freechan(struct tpchan *)
*     Free a minor device and all associated storage objects.  A
*     performance improvement is contemplated which would invalidate the
*     storage and hold on to it. Since the total number of trusted path
*     devices should remain fairly constant, this would reduce the number
*     of kmem_alloc() calls without eating significant memory. For now the
*     code just frees the storage.
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE. Also qprocoff on effect
*    			anmd q->q_ptr already zeroed (cause struct is to be 
*    			dealloced now
*            Leave with: Same 
*            Sets: NONE
*/
STATIC	void
tp_freechan(struct tpchan *tpcp)
{
	int	min;
	struct tpdev *tpdp;
	pl_t pl_1;


	/* 
	* go through each of lower devices references to us
	*/
	if(tpcp->tpc_devp)  {
		tpdp = tpcp->tpc_devp;
		if (tpdp->tpd_ioctlchan == tpcp) {
			tpdp->tpd_ioctlchan = (struct tpchan *)0;
		}
		if (tpdp->tpd_ctrlchp == tpcp) {
			tpdp->tpd_ctrlchp = (struct tpchan *)0;
		}
		if (tpdp->tpd_datachp == tpcp) {
			tpdp->tpd_datachp = (struct tpchan *)0;
		}
		if (tpdp->tpd_inputchp == tpcp) {
			tpdp->tpd_inputchp = (struct tpchan *)0;
		}

		/*
		* if a message was hanging out on the lower, free it
		*/
		if(tpdp->tpd_putnextq && tpdp->tpd_putnextmp) {
			freemsg(tpdp->tpd_putnextmp);
		}

	}
	
 	min = geteminor(tpcp->tpc_dev);
	tpchanhead[min] = (struct tpchan *)0;

	pl_1 = LOCK(tpcp->tpc_data_mutex, plstr);	
	if (tpcp->tpc_hupmsg){
		freemsg(tpcp->tpc_hupmsg);
	}
	if (tpcp->tpc_sakmsg){
		freemsg(tpcp->tpc_sakmsg);
	}
	if (tpcp->tpc_trailmsg){
		freemsg(tpcp->tpc_trailmsg);
	}

	/*
	* safe to unlock tpchan and to free tpc_data_mutex because 
	* all referneces to tpchan have been removed.
	*/
	UNLOCK(tpcp->tpc_data_mutex, pl_1);
	LOCK_DEALLOC(tpcp->tpc_data_mutex);
	SV_DEALLOC(tpcp->tpc_close_sv);

	kmem_free((caddr_t)tpcp, sizeof(struct tpchan));
}

/*
* STATIC	struct	tpchan	*
* tp_allocchan(ulong )
*     Allocate a new minor device and its associated storage.  First
*     search the current list of minor devices for a free one.  If one is
*     found, use it, otherwise reallocate a larger list of minor devices
*     and use the next available one.  The type  argument specifies the kind of
*     channel (data, ctrl, cons, or adm) to be allocated. 
*    
*     Assign a unique connection id in tpc_connid.  Currently the only applicable
*     use of a connection id is for data channels although a connection id is
*     assigned for every type of tp channel.  Connection ids provide an advisory
*     protection mechanism from ctrl channels disconnecting data channels when
*     the ctrl channel receives hangup notification. Since data channels
*     can be connected to a TP device via more than one way (the TP device's ctrl
*     channel and the adm channel) and potentially more than one process, the
*     ctrl channel may not want to disconnect the data channel if it was not
*     the channel that connected the data channel to the TP device.
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE
*            Leave with: Same
*            Sets:       Allocs tpc_data_mutex for this chan
*     Note: no need to lock data in tpcp because we just alloced the data
*     and qprocs are off
*/
STATIC	struct	tpchan	*
tp_allocchan(ulong type)
{
	minor_t	i;
	struct tpchan	*tpcp;

	if (type == TPC_ADM){
		i = TP_ADMDEV;
	}else if (type == TPC_CONS){
		i = TP_CONSDEV;
	}else{
		for (i = TP_NONCLONE; (i < tpchan_curcnt) && tpchanhead[i]; i++);
	}
	if (i >= tpchan_curcnt){
		if (!tp_growlist(&tpchan_curcnt,(caddr_t *)&tpchanhead)){
			return ((struct tpchan *)0);
		}
	}
	if (!tpchanhead[i]){
		tpcp =  kmem_zalloc(sizeof(struct tpchan), KM_NOSLEEP);
		if (!tpcp){
			return ((struct tpchan *)0);
		}
		tpcp->tpc_type = type;
		tpcp->tpc_flags = TPC_ISOPEN;
		tpcp->tpc_dev = makedevice(tpemaj, i);
		/*
		* can't KM_SLEEP because can only sleep if no locks held, need
		* tpdp to stay valid so can't release locks
		* so we KM_NOSLEEP and return an err if can't get a lock
		*/
		if(( tpcp->tpc_data_mutex = LOCK_ALLOC(TPHIER_TPC, TPPL, &tp_chan_mutex_lkinfo, KM_NOSLEEP)) == (lock_t *)0) {
			kmem_free((caddr_t)tpcp, sizeof(struct tpchan));
			return ((struct tpchan *)0);
		}
		if(( tpcp->tpc_close_sv = SV_ALLOC(KM_NOSLEEP)) == (sv_t *)0) {
			LOCK_DEALLOC(tpcp->tpc_data_mutex);
			kmem_free((caddr_t)tpcp, sizeof(struct tpchan));
			return ((struct tpchan *)0);
		}

		tpchanhead[i] = tpcp;
	}else{
		tpcp = tpchanhead[i];
	}
	if (++tpchan_connid == 0)
		tpchan_connid = 1;		/*Make sure it doesn't wrap to zero*/
	tpcp->tpc_connid = tpchan_connid;
	return (tpcp);
}

/*
* STATIC	void
* tp_setcons(struct tpdev *)
*     Set the static variable (tpconsdev) to the device pointer for the current
*     console.
*     If the tp console channel is opened set its tpc_devp to tpconsdev.
*     If the tp console channel has messages on its write queue, enable the queue.
*     This can happen the tp console channel had been opened for "no delay" and
*     tpconsdev was not yet set or the tp console channel had been opened and
*     the 'console' device had been ulinked and subsequently re-linked.
*     If the console changes before the tp console channel is
*     open, the tpconsdev will be used at open to set the channel's device
*     pointer.  Once the tp console channel is open its device pointer is kept up
*     to date.
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck set to WRITE
*            Leave with:	same 
*            Sets:  various tpd_data_mutex and tpc_data_mutex
*/
STATIC	void
tp_setcons(struct tpdev *devp)
{
	queue_t *conschanrq;
	struct tpchan *conschanp;
	pl_t pl_1;

	pl_1 = LOCK(devp->tpd_data_mutex, plstr);
	devp->tpd_userflags &= ~TPINF_CONSOLE;
	UNLOCK(devp->tpd_data_mutex, pl_1);

	tpconsdev = devp;

	conschanp = tp_findchan(TP_CONSDEV);
	if (conschanp){
		pl_1 = LOCK(conschanp->tpc_data_mutex, plstr);
		conschanp->tpc_devp = tpconsdev;
		conschanrq = conschanp->tpc_rq;
		if (conschanrq){
			/*
			* Always enable upper wrtte queue because in MP we don't
			* have a right to look at q_first. service procedure will
			* handle case where there was no data
			*/
			enableok(WR(conschanrq));
			qenable(WR(conschanrq));
		}else{
			/*
			*+ shold never be able to succeed tp_findchan and
			*+ have a null conschanrq. this is a programming err
			*/
			cmn_err(CE_WARN,
		 	 "tp_setcons:TP console channel (cons) has no read queue.\n");
		}
		UNLOCK(conschanp->tpc_data_mutex, pl_1);
	}
	/*
	* Wakeup any processes that were waiting for tpconsdev to be set.
	* its cool to hold all the locks we currently have
	*/
	SV_SIGNAL(tpcons_sv, 0);
}

/*
* STATIC	void
* tp_resetcons(struct tpdev *)
*     If the cons channel is connected for input to the 'console' TP device, call
*     the internal form of TP_CONSDISCONNECT (tp_disccnos()) to disconnect the
*     read/input side of the cons channel.
*    
*     If the argument, tpdp, is not NULL, reset the 'console' TP device to tpdp.
*    
*     If tpdp is NULL:
*     If the default console tp_consoledev is connected and has a real device
*     linked underneath, make the default console the 'console' device.
*     If the default console tp_consoledev is not connected or linked, clear
*     tpconsdev and if the tp console channel is opened, clear its console device
*     pointer (tpchanhead[TP_CONSDEV]->tpc_devp).
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE
*            Leave with:	same 
*            Sets:  various tpd_data_mutex and tpc_data_mutex
*/
STATIC	void
tp_resetcons(struct tpdev *tpdp)
{
	queue_t	*dfltcondevrealrq;
	struct tpchan	*conschanp;
	struct tpdev	*dfltcondevp;
	pl_t	pl_1;

	conschanp = tp_findchan(TP_CONSDEV);
	if (conschanp && tpconsdev ) {
		if(tpconsdev->tpd_inputchp == conschanp){
			(void)tp_disccons();
		}
	}

	if (tpdp){
		tp_setcons(tpdp);
	}else{
		/*
		* tp_consoledev is set in Sassign file to the real dev number
		* of the default console on the system
		*/
		dfltcondevp = tp_finddev(tp_consoledev, FINDDEVLINKED);
		if (dfltcondevp){
			tpconsdev = dfltcondevp;
			pl_1 = LOCK(tpconsdev->tpd_data_mutex, plstr);
			dfltcondevrealrq = dfltcondevp->tpd_realrq;
			if (dfltcondevrealrq){
				UNLOCK(tpconsdev->tpd_data_mutex, pl_1);
				tp_setcons(dfltcondevp);
				return;
			}
			else {
				UNLOCK(tpconsdev->tpd_data_mutex, pl_1);
			}
		}
		tpconsdev = (struct tpdev *)NULL;
		if (conschanp){
			conschanp->tpc_devp = (struct tpdev *)NULL;
		}
	}
}

/*
* STATIC int
* tp_consset(struct tp_info *, struct tpdev *)
*     Switch the current 'console' TP device to the real/physical device indicated
*     by tpinf_rdev.
*    
*     If the flag TPINF_ONLYIFLINKED is set, the 'console' device is changed
*     to the real/physical device only if the real/physical device is linked
*     under a TP device.
*     If TPINF_ONLYIFINKED is not set and a TP device for real/physical device
*     (indicated by tpinf_rdev) does not exist, a TP device for the
*     real/physical device is created.
*     tp_resetcons() is called to actually switch the 'console' TP device.
*    
*     NOTE: The new 'console' TP device does not inherit "connected for input"
*     status from the previously current 'console' TP device.  The default for
*     the new 'console' TP device is not to be "connected for input".
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE
*            Leave with:	same 
*            Sets:  tpd_data_mutex
*/
STATIC int
tp_consset(struct tp_info *tpinfp, struct tpdev **newconstpdpp)
{

	struct tpdev	*tpdp = (struct tpdev *)NULL;
	dev_t		rdev = tpinfp->tpinf_rdev;
	pl_t		pl_1;

	if (tpinfp->tpinf_flags & TPINF_ONLYIFLINKED){
		if (!(tpdp = tp_finddev(rdev, FINDDEVLINKED))){ 
			return (ENXIO);
		}
	}else{
		if (!(tpdp = tp_allocdev(rdev))){
			return (EAGAIN);
		}
		/*
		 * Copy in real device's stat information from
		 * tpinf to the TP device.
		 */ 
		pl_1 = LOCK(tpdp->tpd_data_mutex, plstr);
		tpdp->tpd_realdevfsdev = tpinfp->tpinf_rdevfsdev;
		tpdp->tpd_realdevino = tpinfp->tpinf_rdevino;
		tpdp->tpd_realdevmode = tpinfp->tpinf_rdevmode;
		UNLOCK(tpdp->tpd_data_mutex, pl_1);
	}
	tp_resetcons(tpdp);
	*newconstpdpp = tpdp;
	return (0);
}

/*
* STATIC	struct	tpchan	*
* tp_findchan(minor_t )
*     Return a pointer to the channel information indicated by the minor
*     device number provided.
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck
*            Leave with: Same
*            Sets:        None
*/
STATIC	struct	tpchan	*
tp_findchan(minor_t min)
{
	if (min < (minor_t)0)
		return ((struct tpchan *)0);
	return ((min < tpchan_curcnt) ? tpchanhead[min] : (struct tpchan *)0);
}


/*
* The following routines maintain the device list information.
*/

/*
* STATIC	void
* tp_freedev(struct tpdev *)
*     Free a device block and all associated storage objects.  A
*     performance improvement is contemplated which would invalidate the
*     storage and hold on to it. Since the total number of trusted path
*     devices should remain fairly constant, this would reduce the number
*     of kmem_alloc() calls without changing memory usage. For now the code
*     just frees the storage.
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE. q->q_ptr already zeroed
*    			the zeroing already occured in the I_UNLINK (tp_freedev
*    			is only called on close of CTRL chan that had an
*    			I_LINK active (not I_PUNLINK). the stream head
*    			would have done the I_UNLINK before calling this close
*            Leave with: Same 
*            Sets: Locks tpd_data_mutex. to be on safe side it will
*    		lock and then zero references
*    		to itself in tpchan and tpconsdev, Lock=tpc_data_mutex
*    		at that point no one else can get to us so we are safe to
*		unlock tpd_data_mutex and  free it
*/

STATIC	void
tp_freedev(struct tpdev *devp)
{
	minor_t	min;
	pl_t pl_1;

	/*
	* zero refrences to this dev from each of the possible tpchans 
	*/
	if(devp->tpd_ioctlchan) {
		devp->tpd_ioctlchan->tpc_devp = (struct tpdev *)0;
	}
	if(devp->tpd_ctrlchp) {
		devp->tpd_ctrlchp->tpc_devp = (struct tpdev *)0;
	}
	if(devp->tpd_datachp) {
		devp->tpd_datachp->tpc_devp = (struct tpdev *)0;
	}
	if(devp->tpd_inputchp) {
		devp->tpd_inputchp->tpc_devp = (struct tpdev *)0;
	}

	/*
	* zero tpcons reference if this is the console 
	*/
	if(tpconsdev == devp) {
		tpconsdev = (struct tpdev *)0;
	}

	/*
	* zero references to this by dev head (locked by lpdev_list_rwlck
	* which was passed to us.
	*/
	pl_1 = LOCK(devp->tpd_data_mutex, plstr);
	min = devp->tpd_minordev;
	tpdevhead[min] = (struct tpdev *)0;

	/*
	* zero out struct to be on safe side 
	*/
	devp->tpd_realrq = (queue_t *)0;
	devp->tpd_ctrlrq = (queue_t *)0;
	devp->tpd_datarq = (queue_t *)0;
	devp->tpd_inputrq = (queue_t *)0;
	devp->tpd_ctrlchp = (struct tpchan *)0;
	devp->tpd_datachp = (struct tpchan *)0;
	devp->tpd_inputchp = (struct tpchan *)0;

	/*
	* no more references to this device structure so can unlock and
	* free the tpd_data_mutex 
	*/
	UNLOCK(devp->tpd_data_mutex, pl_1);
	LOCK_DEALLOC(devp->tpd_data_mutex);
	devp->tpd_data_mutex = (lock_t *)0;

	kmem_free((caddr_t)devp, (ulong)sizeof(struct tpdev));
}

/*
* STATIC struct tpdev *
* tp_finddev(dev_t , int )
*
* Calling/Exit State:
*     If set to FINDDEVLINKED, returns a tpdev only if it
*     has a real/physical device linked underneath.
*		
*     Locks: Enter with: tp_plumb_rwlck held
*            Leave with:	same 
*    	  use: tpd_data_mutex
*    
*     NOTE: this is a very expensive routine since it locks all data items as
*     it goes along.
*/
STATIC struct tpdev *
tp_finddev(dev_t realdev, int flag)
{
	struct tpdev	**tpdpp;
	pl_t	pl_1;

	if (!tpdevhead){
		return ((struct tpdev *)0);
	}
	if (realdev == NODEV){
		return ((struct tpdev *)0);
	}
	for (tpdpp = tpdevhead; tpdpp < &tpdevhead[tpdev_curcnt]; tpdpp++){
		if (*tpdpp) {
			pl_1 = LOCK((*tpdpp)->tpd_data_mutex, plstr);
			if ((*tpdpp)->tpd_realdev == realdev){
				if (flag == FINDDEVLINKED){
					if ((*tpdpp)->tpd_realrq){
						UNLOCK((*tpdpp)->tpd_data_mutex, pl_1);
						return (*tpdpp);
					}else{
						UNLOCK((*tpdpp)->tpd_data_mutex, pl_1);
						return ((struct tpdev *)0);
					}
				}else{
					UNLOCK((*tpdpp)->tpd_data_mutex, pl_1);
					return (*tpdpp);
				}
			}
			UNLOCK((*tpdpp)->tpd_data_mutex, pl_1);
		}
	}
	return ((struct tpdev *)0);
}

/*
* STATIC struct	tpdev	*
* tp_allocdev(dev_t )
*     Allocate a device block for the specified real device.  First search
*     the the current list of TP devices for an already allocated device
*     for that real device, then, if the device is not found, search for a
*     a free entry.  As a last resort, reallocate a larger list of TP devices
*     and use the next available one.  If a new device is allocated, fill
*     out the tpd_realdev field with the specified real device number.
*    
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck held for WRITE
*            Leave with:	same 
*    	     use: allocates tpd_data_mutex. no need to lock since no one else
*    		can get to it yet (tp_plumb_rwlck held for WRITE and
*    		we have not set lower q->q_ptr to this dev yet)
*/

STATIC struct	tpdev	*
tp_allocdev(dev_t rdev)
{
	int		i;
	struct tpdev	*tpdp;

	if (tpdp = tp_finddev(rdev, 0)){
		return (tpdp);
	}

	for (i = 0; (i < tpdev_curcnt) && tpdevhead[i]; i++);
	if (i >= tpdev_curcnt){
		/*
		* grow array of tp devices pointers
		*/
		if (!tp_growlist(&tpdev_curcnt,(caddr_t *)&tpdevhead)){
			return ((struct tpdev *)0);
		}
	}
	/*
	* allocate memory for a tp device and set tp device up.
	*/
	tpdp = tpdevhead[i] = kmem_zalloc(sizeof(struct tpdev),KM_NOSLEEP);
	if (!tpdp){
		return ((struct tpdev *)0);
	}
	tpdp->tpd_flags = 0;
	tpdp->tpd_minordev = (minor_t)i;
	tpdp->tpd_realdev = rdev;
	/*
	* can't KM_SLEEP because can only sleep if no locks held, need
	* tpdp to stay valid so can't release locks
	* if alloc fails, fail the tp_allocdev
	*/
	if((tpdp->tpd_data_mutex = LOCK_ALLOC(TPHIER_TPD, TPPL, &tp_dev_mutex_lkinfo, KM_NOSLEEP)) == (lock_t *)0) {
		kmem_free((caddr_t)tpdp, (ulong)sizeof(struct tpdev));
		return ((struct tpdev *)0);
	}
	return (tpdp);
}

/*
* STATIC int
* tp_growlist(int *, caddr_t *)
*     allocate "tp_listallocsz" more pointers in a list.
*    
*     NOTE: to maintain the indexed array "feel", a new set of 
*     cnt + growcnt pointers are allocated and the values of the
*     current set are copied into the new set of tp channel pointers.
*
* Calling/Exit State:
*     cntp =  pointer to current number of pointers
*     listp = pointer to list
*
*     Locks: Enter with: tp_plumb_rwlck held for WRITE
*            Leave with:	same 
*    	  use: None 
*/

STATIC int
tp_growlist(int *cntp, caddr_t *listp)
{
	ulong		sz,osz;
	caddr_t	tmp;
	caddr_t	olst;

	olst = *listp;
	osz = (*cntp) * sizeof(caddr_t);
	sz = osz + (tp_listallocsz * sizeof(caddr_t));
	tmp = kmem_zalloc(sz, KM_NOSLEEP);
	if (!tmp)
		return (0);

	if (olst){
		bcopy(olst,tmp, osz);
	}
	*listp = tmp;
	(*cntp) += tp_listallocsz;
	if (olst){
		kmem_free(olst, osz);
	}
	return (1);
}


/*
* STATIC mblk_t *
* tp_allocioctlmsg(int , size_t )
*     Allocate an internal M_IOCTL message.
*
* Calling/Exit State:
*     cmd = ioctl cmd 
*     datasz = size of data part of message 
*
*     Locks: Enter with: whatever locks the place that the return value
*    	  is to be put in (e.g. tpx_data_mutex).
*            Leave with:	same 
*    	  use: none (o.k. to have others set on entry)
*/

STATIC mblk_t *
tp_allocioctlmsg(int cmd, size_t datasz)
{

	mblk_t		*mp;	/* pointer to message */ 
	mblk_t		*bp;	/* pointer to message block */ 
	mblk_t		*dbp;	/* pointer to current message data						* block
					*/ 
	struct iocblk	*iocp;
	size_t			count;
	size_t			bufsz;


	if (!(mp = bp = allocb(sizeof(struct iocblk), BPRI_HI))){
		return (mblk_t *)0;
	}

	/*
	* Set up ioctl block.  Hold credentials, credentials will be freed
	* in tplrput when it receives the ACK/NAK.
	*/
	/* LINTED pointer alignment */
	iocp = (struct iocblk *)(bp->b_rptr);
	iocp->ioc_count = datasz;
	iocp->ioc_cmd = cmd;
	/*
	* sys_cred, the generic credential structure for the kernel, is used
	* when the credentials may be required by functions that are executed
	* on behalf of the kernel and not the user.
	* NOTE:sys_cred has all privileges.
	*/  
	iocp->ioc_cr = sys_cred;
	iocp->ioc_error = 0;
	iocp->ioc_rval = 0;
	bp->b_datap->db_type = M_IOCTL;
	bp->b_wptr += sizeof(struct iocblk);

	/*
	* Allocate space for data, if any
	*/
	if (datasz){
		count = datasz;
		while (count){
			bufsz = (count <= MAXIOCBSZ ? count : MAXIOCBSZ);
			if (!(dbp = allocb(bufsz, BPRI_HI))){
				freemsg(mp);
				return ((mblk_t *)0);
			}
			dbp->b_datap->db_type = M_DATA;
			dbp->b_wptr += bufsz;
			bp = (bp->b_cont = dbp);
			count -= bufsz;
		}
	}else{
		bp->b_cont = (mblk_t *)0;
	}

	return (mp);
}

/*
* void
* tp_disconnect(dev_t )
*     Disconnect data channel and channel connected for input (if they are
*     different) from the TP device, given the external dev_t of a TP channel
*     (typically the data channel).
*     This meant to be called, when the controlling terminal process exits.
*     It immediately denies access to the real/physical tty device (via the
*     TP data channel {and input access if the cons channel is associated
*     with the TP device}) if any other process has access to the data or cons
*     channel associated with the TP device.
*
*     NOTE: If the TP device number passed as an argument is the cons channel, only
*     disconnect the cons channel for input if it is connected for input.  Since
*     the cons channel gets registered as a tty type device at the Stream Head,
*     it can become a controlling terminal.  We do not want to disconnect the
*     data channel (if any) associated with the TP device that the cons channel
*     is also connected to, since the controlling terminal process'es controlling
     terminal is the cons channel, not the data channel.
*    
* Calling/Exit State:
*     Locks: Enter with: None
*            Leave with:	None 
*    	  use: tp_plumb_rwlck
*/
void
tp_disconnect(dev_t tpdev)
{

	struct tpchan	*tpcp;
	struct tpdev	*tpdp;
	minor_t		tpminordev;
	pl_t		pl_1;

	/*
	 * If there is no TP channel associated with the minor device number,
	 * or the channel is not connected to a TP device, just return.
	*/
	tpminordev = geteminor(tpdev);
	/*
	 * can call tp_plumb with possibility of sleeping becuse it comes from 
	 * a user level close of the controlling tty
	 */
	pl_1 = RW_WRLOCK(tp_plumb_rwlck, plstr);
	if ((tpcp = tp_findchan(tpminordev)) == (struct tpchan *)0){
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		return;
	}
	tpdp = tpcp->tpc_devp;
	if (tpdp){
		if (tpminordev == TP_CONSDEV){
			if (tpdp->tpd_inputchp == tpcp){
				(void) tp_disccons();
			}
		}else{
			tp_discinput(tpdp);
			tp_discdata(tpdp);
		}
		pl_1 = tp_doputnext_low(tpdp, pl_1, 'R');
		pl_1 = tp_doputnextctl1_upper(tpcp, pl_1, 'R');
	}
	RW_UNLOCK(tp_plumb_rwlck, pl_1);
	return;
}

/*
* STATIC void
* tp_wakeall(struct tpdev *)
*     tp_wakeall is called after a putnext from a lower to a upper.
*     if the setting of TPD_BUSYNEXT had caused a close to sleep this
*     routine wakes them up. Since any or all of the attached uppers
*     could have been affected we check all of them.
*     note, we don't need to do a wakeup on ioctlchan because it is undoubtdly
*     one of the other 3. Also, we don't do a inputchp one if (as most likely)
*     it is the same as the datap.
*
* Calling/Exit State:
*     Locks: Enter with: tp_plumb_rwlck, tpd_data_mutex must not be set
*    		because it sets tpc_data_mutex
*            Leave with:	same 
*      	     sets: tpc_data_mutex
*/

STATIC void
tp_wakeall(struct tpdev *tpdp)
{
	struct tpchan	*datap;
	struct tpchan	*inputp;
	struct tpchan	*ctrlp;
	pl_t	pl_1;

	datap = tpdp->tpd_datachp;
	inputp = tpdp->tpd_inputchp;
	ctrlp = tpdp->tpd_ctrlchp;

	if(datap) {
		pl_1 = LOCK(datap->tpc_data_mutex, plstr);
		if(datap->tpc_flags & TPC_CLOSING) {
			SV_SIGNAL(datap->tpc_close_sv, 0);
		}
		UNLOCK(datap->tpc_data_mutex, pl_1);
	}
	
	if(inputp && (inputp != datap)) {
		pl_1 = LOCK(inputp->tpc_data_mutex, plstr);
 		if(inputp->tpc_flags & TPC_CLOSING) {
			SV_SIGNAL(inputp->tpc_close_sv, 0);
		}
		UNLOCK(inputp->tpc_data_mutex, pl_1);
	}
	
	if(ctrlp) {
		pl_1 = LOCK(ctrlp->tpc_data_mutex, plstr);
		if(ctrlp->tpc_flags & TPC_CLOSING) {
			SV_SIGNAL(ctrlp->tpc_close_sv, 0);
		}
		UNLOCK(ctrlp->tpc_data_mutex, pl_1);
	}

}
	





/*
* STATIC pl_t
* tp_doputnext_low(struct tpdev *, pl_t , int)
*     this routine is here to make locking simpler. It is part of the algoritm
*     for taking 'putnext' out of tp_sendioctl. The problem was that tp_sendioctl
*     was called at the end of possibly 6 or 7 subroutine level. Since it
*     contained a 'putnext' we needed to UNLOCK all locks before calling. The
*     the problem was that at that subroutine level we had no idea what locks
*     were held that needed to be released or which ones needed to be put back
*     on.
*    
*     we then broke tp_sendioctl to three parts. tp_sendioctl simply
*     puts a pointer to the queue and mp it was going to do a putnext into
*     the tpdev structure that is to send the ioctl. You clear these fields out
*     (step 1) before calling tp_sendioctl (step 2). tp_sendioctl will then
*     the tpdev structure only at the point it was to do a 'putnext'. setep
*     3 calls this routine to do the putnext. It should be called at a level
*     where you know what locks
*    
*     this is designed to be called at the level we would normally lock the
*     plumbing.
*    
* Calling/Exit State:
*
*     you enter the tpdp of the lower device, 
*     also have old tp_plumb_rwlck value, and a flag = 'W' to re-acquire as a 
*     WRITE lock
*    
*     returns the old pl.
*    
*     Locks: Enter with:tp_plumb_rwlck
*            Leave with: same
*            Sets: releases plumb_rwlck before putnext, reacquires  afterwards
*/

STATIC pl_t
tp_doputnext_low(struct tpdev *tpdp, pl_t pl_1, int type)
{
	queue_t *q;
	mblk_t *mp;
	pl_t pl_2;


	if(!tpdp) {
		/*
		* the real device went away (freed beneath our feet)
		* so give up
		*/
		return(pl_1);
	}
		

	pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
	if(tpdp->tpd_putnextq) {
		q = tpdp->tpd_putnextq;
		mp = tpdp->tpd_putnextmp;
		/*
		* signal that we sent it by zeroing it out
		*/
		tpdp->tpd_putnextq = (queue_t *)0;
		tpdp->tpd_putnextmp = (mblk_t *)0;
	
		tpdp->tpd_flags |= TPD_BUSYNEXT;
			
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		/*
		* all locks released now, so can do the putnext
		*/

		putnext(q, mp);

		/* 
		* need to get chan pointer again to reset TPC_BUSYNEXT
		* so must lock it
		*/
		pl_1 = (type == 'W') ? RW_WRLOCK(tp_plumb_rwlck, plstr) : RW_RDLOCK(tp_plumb_rwlck, plstr);
		/*
		* tpdp is locked in place by TPD_BUSYNEXT 
		*/
		pl_2 = LOCK(tpdp->tpd_data_mutex, plstr);
		tpdp->tpd_flags &= ~TPD_BUSYNEXT;
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
	
	}
	else {
		/*
		* nothing to send, syntax of this routine is to unlock locks given
		* so unlock them
		*/
		UNLOCK(tpdp->tpd_data_mutex, pl_2);
	}
	return(pl_1);
}
			


/*
* STATIC pl_t
* tp_doputnextctl1_upper(struct tpchan *, pl_t , int )
*
*    this routine is here to make locking simpler. It is part of the algoritm
*    for taking 'putctrl1' out of tp_datadisc. The problem was that tp_datadisc
*    was called at the end of possibly 6 or 7 subroutine level. Since it
*    contained a 'putnextctl1' we needed to UNLOCK all locks before calling. The
*    the problem was that at that subroutine level we had no idea what locks
*    were held that needed to be released or which ones needed to be put back
*    on.
*
*    we then broke tp_datadisc to three parts. tp_datadisc simply
*    puts a pointer to the queue and mp it was going to do a putnextctl1 into
*    the tpchan structure that is to send the ioctl. You clear these fields out
*    (step 1) before calling tp_datadisc (step 2). tp_datadisc will then
*    the tpchan structure only at the point it was to do a 'putnextctl1'. step
*    3 calls this routine to do the putnextctl1. It should be called at a level
*    where you know what locks
*
* Calling/Exit State:
*    you enter the tpcp of the upper,
*    also set can_sleep flag= TRUE if we have user context to sleep
*    also have old tp_plumb_rwlck value, and a flag = 'W' to re-acquire as a 
*    WRITE lock
*
*    this is designed to be called at the level we would normally lock the
*    plumbing.
*
*    returns old pl
*
*    Locks: Enter with:tp_plumb_rwlck
*           Leave with: same
*	    Sets: releases plumb_rwlck before putnext, reacquires  afterwards
*/

STATIC pl_t
tp_doputnextctl1_upper(struct tpchan *tpcp, pl_t pl_1, int rw_type)
{
	queue_t *q;
	int type;
	int param;
	pl_t pl_2;

	if(!tpcp) {
		/*
		* the real device went away (freed beneath our feet)
		* so give up
		*/
		return(pl_1);
	}
		

	pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
	if(tpcp->tpc_putnextctl1q) {
		q = tpcp->tpc_putnextctl1q;
		type = tpcp->tpc_putnextctl1type;
		param = tpcp->tpc_putnextctl1param;
		/*
		* signal that we sent it by zeroing it out
		*/
		tpcp->tpc_putnextctl1q = (queue_t *)0;
		
		tpcp->tpc_flags |= TPC_BUSYNEXT;

		UNLOCK(tpcp->tpc_data_mutex, pl_2);
		RW_UNLOCK(tp_plumb_rwlck, pl_1);
		
		putnextctl1(q, type, param);

		pl_1 = (rw_type == 'W') ? RW_WRLOCK(tp_plumb_rwlck, plstr) : RW_RDLOCK(tp_plumb_rwlck, plstr);
		/*
		* tpcp locked in place by TPC_BUSYNEXT
		*/
		pl_2 = LOCK(tpcp->tpc_data_mutex, plstr);
		tpcp->tpc_flags &= ~TPC_BUSYNEXT;
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
	}
	else {
		/*
		* nothing to send, syntax of this routine is to unlock locks given
		* so unlock them
		*/
		UNLOCK(tpcp->tpc_data_mutex, pl_2);
	}
	return(pl_1);
}


/*
* STATIC void
* tp_saktypeswitch(struct tpdev *)
*
*  Change sak type to DATA if the sak type is NONE and there are no
*  channels connected for input.
*  This is called via a setup from timeout() from tp_discdata() and
*  tp_disccons().  It delays the change to sak type DATA if a delay
*  time interval is defined.
*  NOTE: Any calls to tp_freedev() must call untimeout() if TP device
*  has a pending timeout() function.  This is to insure the integrity
*  of the argument, tpdp, to this function.
*
* Calling/Exit State:
*  called at intr level on timeout. assumes that tp_freedev to free the tptp
*  passed to it can not occur. This is enforced by doing an un-timeout
*  before freeing
*    
*/
STATIC void
tp_saktypeswitch(struct  tpdev *tpdp)
{
	pl_t pl_1;
	
	pl_1 = LOCK(tpdp->tpd_data_mutex, plhi);

        if ((tpdp->tpd_sak.sak_type == saktypeNONE) && !(tpdp->tpd_inputchp)){
                tpdp->tpd_sak.sak_type = saktypeDATA;
        }
        tpdp->tpd_timeoutid = 0;
	UNLOCK(tpdp->tpd_data_mutex, pl_1);
}

