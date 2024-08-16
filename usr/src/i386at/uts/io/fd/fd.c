/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/fd/fd.c	1.41"
#ident	"$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <fs/buf.h>
#include <svc/errno.h>

/** New headers for DDI support **/
#include <io/open.h>
#include <io/uio.h>
#include <proc/cred.h>

#include <svc/systm.h>
#include <io/elog.h>
#include <io/iobuf.h>
#include <fs/file.h>
#include <io/ioctl.h>

#include <util/cmn_err.h>
#include <io/dma.h>
#include <io/i8237A.h>
#include <io/cram/cram.h>
#include <io/fd/fd.h>
#include <io/vtoc.h>

#include <util/debug.h>
#include <util/inline.h>
#include <io/conf.h>

#include <util/mod/moddefs.h>

/** New headers for DDI support -- MUST COME LAST **/
#include <io/ddi.h>
#include <io/ddi_i386at.h>


void fdinit(void);
void fdstart(void);
int fdopen(dev_t *, int, int, struct cred *);
int fdclose(dev_t, int, int, struct cred *);
int fdread(dev_t, struct uio *, struct cred *);
int fdwrite(dev_t, struct uio *, struct cred *);
int fdsize(dev_t);
int fdioctl(dev_t, int, caddr_t, int, struct cred *, int *);
int fdstrategy(register struct buf *);
int fdxfer(register struct buf *);
void fdiostart(void);
int fdio(register struct buf *);
int fd_doseek(register struct fdstate *);
void fdintr(int);
int fddone(register struct buf *);
int fdsense(int);
int fderror(struct buf *, int);
void fdwait(register struct buf *);
void fdnoflop(register struct fdstate *);
void fdtimer(int);
void fddooropen(register struct fdstate *);
void fdreset(void);
int fdprint(dev_t, char *);
int fdresult(caddr_t, int);
int fdcmd(caddr_t, int);
int fdrawio(register struct buf *);
void fdmtnio(register struct buf *);
int fdm_seek(unsigned char, unsigned char, unsigned char);
int fdm_rezero(unsigned char);
int fdm_snsdrv(unsigned char, unsigned char);
int fdm_setup(char, char, char);
int fdm_readid(unsigned char, unsigned char);
int fdm_snsdskchg(unsigned char);
int fdm_read(char, char, char, char, char, char, char, char, char);
int fdm_dfmt(dev_t);

STATIC pl_t	fdpl;	/* from System file */

int	fdwait_timer;
int	fdtimer_timer;
int	fdnoflop_timer;
int	fdrawio_timer;
int	fdmtnio_timer;

struct dma_cb	*fdcb;
struct dma_buf	*dbuf;

extern	int	fdloaded;	/* in fdbuf */

#define		DRVNAME		"fd - Floppy disk driver"

STATIC long fd_eisa_brdid;
STATIC	int	fd_load(), fd_unload();

MOD_DRV_WRAPPER(fd, fd_load, fd_unload, NULL, DRVNAME);

extern void mod_drvattach(), mod_drvdetach();

/*
 * Declared in space.c, set based upon tunable FD_DOOR_SENSE
 *
 * fd_door_sense = 0: Do not trust Disk Change Signal
 * fd_door_sense = 1: Trust Disk Change Signal
 */
extern int	fd_door_sense;
extern int	fd_do_config_cmd;

/*
 * STATIC int
 * fd_load()
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
fd_load()
{
	fdinit();
	mod_drvattach(&fd_attach_info);
	fdstart();
	return(0);
}

/*
 * STATIC int
 * fd_unload()
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
fd_unload()
{
	if (fdwait_timer != 0) {
		untimeout(fdwait_timer);
	}
	if (fdtimer_timer != 0) {
		untimeout(fdtimer_timer);
	}
	if (fdnoflop_timer != 0) {
		untimeout(fdnoflop_timer);
	}
	if (fdrawio_timer != 0) {
		untimeout( fdrawio_timer );
	}
	if (fdmtnio_timer != 0) {
		untimeout(fdmtnio_timer);
	}

	mod_drvdetach(&fd_attach_info);
	fdloaded = 0;

	dma_free_buf(dbuf);
	dma_free_cb(fdcb);

	return(0);
}

/*
 * End of wrapper, driver code starts here.
 */

#ifdef DEBUG
int	fddebug = 0;
#endif

#ifndef TRUE
#define TRUE 1
#endif

/*
 * Double Density Drive Parameters
 *
 *	Capacity unformatted	500Kb
 *	Capacity formatted	368.6Kb
 *	Recording density	5876 bpi
 *	Track density		48 tpi
 *	Cylinders		40
 *	Tracks			80
 *	Encoding method		MFM
 *	Rotational speed	300 rpm
 *	Transfer rate		250 kbps
 *	Latency (average)	100 ms
 *	Access time
 *		Average		81 ms
 *		Track to track	5 ms
 *		Settling time	20 ms
 *	Head load time		50 ms
 *	Motor start time	750 ms
 *
 * Quad Density Drive Parameters
 *
 *	Capacity unformatted	1604 Kb
 *	Capacity formatted	1.2 Mb
 *	Recording density	9646 bpi
 *	Track density		96 tpi
 *	Cylinders		80
 *	Tracks			160
 *	Encoding method		MFM
 *	Rotational speed	360 rpm
 *	Transfer rate		500 kbps
 *	Latency (average)	83 ms
 *	Access time
 *		Average		91 ms
 *		Track to track	3 ms
 *		Settling time	18 ms
 *	Head load time		50 ms
 *	Motor start time	750 ms
 */

/* Types of floppy drive as defined in the CMOS RAM */
#define DRV_NONE  0x00
#define DRV_5D	  0x01 
#define DRV_5H	  0x02
#define DRV_3H	  0x04
#define	DRV_3e    0x05	/** DELL 2.88 Mbyte Support **/
#define	DRV_3E    0x06	/** 2.88 Mbyte Support **/

/* mask for various types of floppy drives */
#define FD_5D	  0x01
#define FD_5H	  0x02
#define FD_3H	  0x08
#define	FD_3E     0x30	/** 2.88 Mbyte Support ( including DELL ) **/

/* Track format supported */
#define FMT_5H	  0
#define FMT_5D9	  1
#define FMT_5D8	  2
#define FMT_5D4	  3
#define FMT_5D16  4
#define FMT_5Q	  5
#define FMT_3D	  6
#define FMT_3H	  7
#define FMT_3E    8	/** 2.88 Mbyte Support **/
#define	FMT_3M	  9	/* Unisys 3-Mode 1.2Mb 3.5" 512-BPS */
#define	FMT_3N	  10	/* Unisys 3-Mode 1.2Mb 3.5" 1024-BPS */
#define FMT_AUTO  11
#define FMT_MAX	  11
#define FMT_UNKNOWN 11

/* Transfer rate encoding */
#define FD500KBPS (uchar_t)0
#define FD300KBPS (uchar_t)1
#define FD250KBPS (uchar_t)2
#define FD1MBPS	  (uchar_t)3

/* Bytes per sector encoding */
#define FD256BPS 1
#define FD512BPS 2
#define FD1024BPS 3

struct fdcstat {
	char    fd_etimer;
	char    fd_curdrv;
	char    fd_state;
	char    fd_cstat;
	char    fd_savtrk;
	ushort  fd_blkno;
	ushort  fd_bpart;
	ushort  fd_btot;
	long    fd_addr;
} fdcst;

char    fd_mtimer[NUMDRV];
ushort  fd_curmotors;
ushort  fdt_running;

#define DOOR_OPEN   0
#define DOOR_RST    1
#define CHK_RECAL   2
#define RECAL_DONE  3
#define CHK_SEEK    4
#define SEEK_DONE   5
#define XFER_BEG    6
#define XFER_DONE   7
#define	INTR_WAIT   8

struct f0 {
	unsigned char fd_st0;
	unsigned char fd_st1;
	unsigned char fd_st2;
	unsigned char fd_strack;
	unsigned char fd_shead;
	unsigned char fd_ssector;
	unsigned char fd_ssecsiz;
} fdstatus;

struct f1 {
	unsigned char fd_sst0;
	unsigned char fd_scyl;
} fdsnst;

struct fdcmdseq {
	unsigned char fd_cmdb1;
	unsigned char fd_cmdb2;
	unsigned char fd_track;
	unsigned char fd_head;
	unsigned char fd_sector;
	unsigned char fd_ssiz;
	unsigned char fd_lstsec;
	unsigned char fd_gpl;
	unsigned char fd_dtl;
} fdcs;

struct fdcmdseq fdrcs = {
	SPECIFY, 0xCF, 0x32, 0, 0, 0, 0, 0, 0
};

struct fdcmdseq fdperp_mode = {
	PERPENDICULAR, 0x80, 0, 0, 0, 0, 0, 0, 0
};

struct fdcmdseq fd_config = {
        FDC_CONFIG, 0x00, 0x58, 0x00
};

#define fd_bps  fd_track
#define fd_spt  fd_head
#define fd_gap  fd_sector
#define fd_fil  fd_ssiz

#define b_fdcmd b_priv2.un_int

int fddevflag = D_NOBRKUP | D_BLKOFF;

extern bcb_t *fdbcb;

struct	fdraw	fdraw[NUMDRV];
struct	buf	*fdrawbuf[NUMDRV];

extern	struct  fdbufstruct     fdbufstruct;
extern	int	fdbufgrow();

struct fdraw fdmtn[NUMDRV];
struct buf *fdmtnbuf[NUMDRV];
char	fdmtnbusy;
#define MTNCMD	0x7E
#define SENSE_DSKCHG 0x7D

#define FE_CMD		-1
#define FE_ITOUT	-2
#define FE_RSLT 	-3
#define FE_REZERO 	-4
#define FE_SEEK		-5
#define FE_SNSDRV	-6
#define FE_READID	-7
#define FE_READ		-8
#define FE_BARG		-9

#define NOFLOP_EMAX	4


extern	struct  fdstate fd[];	/* in fdbuf */

struct  fdparam fdparam[NUMDRV];

/*
 * floppy disk partition tables
 */

#define N_NCYL 77
struct fdpartab medpart[FDNPART] = {	/* for format 3N */
	{0, 77},
	{0,  1},
	{1, 76}
};

#define H_NCYL 80
struct fdpartab	highpart[FDNPART] = {  /* for format 5H, 3H, 3D */
	{0, 80},
	{0,  1},
	{1, 79}
};

#define Q_NCYL 79
struct fdpartab	quadpart[FDNPART] = {	/* for format 5Q */
	{0, 79},			/* dedicated for AT&T 3B2 */
	{0,  1},
	{1, 78}
};

#define D_NCYL 40
struct fdpartab	dblpart[] = { 	/* for format 5D9, 5D8, 5D4, 5D16 */
	{0, 40},
	{0,  1},
	{1, 39},
	{0, 40}		/* dedicate for AT&T unix pc */
};			/* partition started at track 1 of cylinder 0 */
int fd_secskp[NUMDRV];

/*
 * floppy disk format table
 */
struct fdsectab	fdsectab[FDNSECT] = {
	{  512,  9, 15,       FD_5H, 0x1b, 0x54 }, /* FMT_5H   */
	{  512,  9,  9, FD_5D|FD_5H, 0x2a, 0x50 }, /* FMT_5D9  */
	{  512,  9,  8, FD_5D|FD_5H, 0x2a, 0x50 }, /* FMT_5D8  */
	{ 1024, 10,  4, FD_5D|FD_5H, 0x80, 0xf0 }, /* FMT_5D4  */
	{  256,  8, 16, FD_5D|FD_5H, 0x20, 0x32 }, /* FMT_5D16 */
	{  512,  9,  9,       FD_5H, 0x1b, 0x54 }, /* FMT_5Q   */
	{  512,  9,  9, FD_3H|FD_3E, 0x1b, 0x54 }, /* FMT_3D   */
	{  512,  9, 18, FD_3H|FD_3E, 0x1b, 0x54 }, /* FMT_3H   */
	{  512,  9, 36,       FD_3E, 0x38, 0x53 }, /* FMT_3E   */	/** 3.5-Inch 2.88Mb Drive Support  **/
	{  512,  9, 15,       FD_3H, 0x1b, 0x54 }, /* FMT_3M   */	/** 3.5-Inch 1.2Mb 512BPS Support  **/
	{ 1024, 10,  8,       FD_3H, 0x35, 0x54 }, /* FMT_3N   */	/** 3.5-Inch 1.2Mb 1024BPS Support **/
};
	
struct  iotime  fdstat[NUMDRV];
struct  iotime *fdstptr[NUMDRV] = {
	(struct iotime *)&fdstat[0],
	(struct iotime *)&fdstat[1],
};

struct  iobuf   fdtab = {0,0,0,0,0,0,0,00,0,0,0,0,0,0,0,0,0,0,0};

struct  iobuf   fdutab[NUMDRV] = {
	{0,0,0,0,0,0,0,00,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,00,0,0,0,0,0,0,0,0,0,0,0}
};

/**
struct  iobuf   fdtab = tabinit(FD0, NULL);
struct  iobuf   fdutab[NUMDRV] = {
	tabinit(FD0, &fdstat[0].ios),
	tabinit(FD0, &fdstat[1].ios),
};
**/
struct  iobuf  *fdutptr[NUMDRV] = {
	(struct iobuf *)&fdutab[0],
	(struct iobuf *)&fdutab[1],
};

static char nomesg[] = "";

static char *fd_ctrerr[] = {
	"command timeout",
	"status timeout",
	"busy",
};

static char *fd_drverr[] = {
	"Missing data address mark",
	"Cylinder marked bad",
	nomesg,
	nomesg,
	"Seek error (wrong cylinder)",
	"Uncorrectable data read error",
	"Sector marked bad",
	"nomesg",
	"Missing header address mark",
	"Write protected",
	"Sector not found",
	nomesg,
	"Data overrun",
	"Header read error",
	nomesg,
	nomesg,
	"Illegal sector specified",
};

STATIC char enabint;    /* enable interrupt bit is different on the AT and MCA*/


/*
 * void
 * fdinit(void)
 *	The floppy controller can read/write  consecutive sectors,
 *	but only on a single track (and head).
 *
 * Calling/Exit State:
 *	None.
 */
void
fdinit(void)
{
	unsigned char	ddtb;

	long	bus_type;

	fdpl = pl4;

	/*
	 * The following initialization was added because makedev used 
	 * in tabinit was changed from a macro to a function. Therefore
	 * tabinit could not be used for initialization.
	 */

	fdtab.b_edev = makedevice(FD0,0);
	fdtab.io_stp = NULL;
	fdutab[0].b_edev = makedevice(FD0,0);
	fdutab[0].io_stp = &fdstat[0].ios;
	fdutab[1].b_edev = makedevice(FD0,0);
	fdutab[1].io_stp = &fdstat[1].ios;
	/** END DDI initialization changes **/

	drv_gethardware(IOBUS_TYPE, &bus_type);

	if (bus_type == BUS_MCA) {
		fdrcs.fd_cmdb2 = 0xAF;
		enabint = ENAB_MCA_INT;
	} else {
		enabint = ENABINT;
	}

	if (bus_type == BUS_EISA) {
		/*
		 * Get the ID for slot 0 (the system board).
		 */
		fd_eisa_brdid = 0;
		if (drv_gethardware(EISA_BRDID, &fd_eisa_brdid) == -1) {
			fd_eisa_brdid = 0xFFFFFFFF;
		}
#ifdef DEBUG
		else {
			cmn_err(CE_NOTE,"fdinit: fd_eisa_brdid=0x%x",fd_eisa_brdid);
		}
#endif
	}

	ddtb = CMOSread(DDTB);
	fd[0].fd_drvtype = ((ddtb >> 4) & 0x0F);
	fd[1].fd_drvtype = ddtb & 0x0F;
	fd[0].fd_hst = fd[1].fd_hst = T50MS;
	fd[0].fd_mst = fd[1].fd_mst = T750MS;

	fdcb = dma_get_cb(DMA_NOSLEEP);
	dbuf = dma_get_buf(DMA_NOSLEEP);
	fdcb->targbufs = dbuf;
	fdcb->targ_step = DMA_STEP_INC;
	fdcb->targ_path = DMA_PATH_8;
	fdcb->trans_type = DMA_TRANS_SNGL;
	fdcb->targ_type = DMA_TYPE_IO;
	fdcb->bufprocess = DMA_BUF_SNGL;

	/***
	** Check if drive 0 is a 2.88Mb drive, if it is then set
	** the perpendicular command sequence accordingly.
	***/
	if( fd[0].fd_drvtype == DRV_3e || fd[0].fd_drvtype == DRV_3E ) {
		fdperp_mode.fd_cmdb2 |= 0x04;
	}

	/***
	** Check if drive 1 is a 2.88Mb drive, if it is then set
	** the perpendicular command sequence accordingly.
	***/
	if( fd[0].fd_drvtype == DRV_3e || fd[0].fd_drvtype == DRV_3E ) {
		fdperp_mode.fd_cmdb2 |= 0x08;
	}

	/***
	** If either drive is a 2.88Mb, issue the perpendicular
	** recording mode command.
	***/
	if( fdperp_mode.fd_cmdb2 & 0x0c ) {
#ifdef DEBUG
		/*
		 *+ debugging
		 */
		cmn_err(CE_CONT,"!fdinit: FD Setting Perpendicular Mode\n");
#endif
		fdcmd((caddr_t)&fdperp_mode, 2);
	}

	fdloaded = 1;

	return;
}

/*
 * void
 * fdstart(void)
 *	allocate per-drive buf structures
 *
 * Calling/Exit State:
 *	None.
 */
void
fdstart(void)
{
	register int	i;

	for (i = 0; i < NUMDRV; ++i) {
		if (!(fdmtnbuf[i] = getrbuf(KM_NOSLEEP))) {
			/*
			 *+ Could not allocate space for floppy
			 *+ disk structures at start time, when
			 *+ plenty of memory should be available.
			 */
			cmn_err(CE_PANIC,
			"fd: cannot allocate per-driver buf structure\n");
		}

		if (!(fdrawbuf[i] = getrbuf(KM_NOSLEEP))) {
			/*
			 *+ Could not allocate space for floppy
			 *+ disk structures at start time, when
			 *+ plenty of memory should be available.
			 */
			cmn_err(CE_PANIC,
			"fd: cannot allocate per-driver buf structure\n");
		}
	}

	/* enable floppy disk interrupts */
	fdreset();

	return;
}
		
/*
 * int
 * fdopen(dev_t *devp, int flags, int otyp, struct cred *cred_p)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
fdopen(dev_t *devp, int flags, int otyp, struct cred *cred_p)
{
	register struct fdstate *f;
	int	fmt, part, unit;
	int  	rtn;
	int	oldpri;
	void	*fdproc;
	dev_t dev;

#ifdef DEBUG

	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdopen");
#endif

	dev = *devp;
	unit = UNIT(dev);
	part = PARTITION(dev);
	fmt = FRMT(dev);
	if (unit >= NUMDRV || fmt > FMT_MAX ||
	    (part >= FDNPART && fmt != FMT_5D8)) {
		return(ENXIO);
	}
	f = &fd[unit];
	if (f->fd_drvtype == DRV_NONE) {
		return(ENXIO);
	}

	if (fmt != FMT_AUTO) {
		/********************************/
		/* Unisys 3-mode floppy support */
		/********************************/
		if ((fmt == FMT_3M || fmt == FMT_3N) && 
		    (fd_eisa_brdid != 0xC255)) {
			return(ENXIO);
		}
		if (((0x01 << (f->fd_drvtype - 1)) &
				 fdsectab[fmt].fd_drvs) == 0) {
			return(ENXIO);
		}
	}

	oldpri = splhi();
	while (f->fd_status & (OPENING | CLOSING))
		sleep(&f->fd_status, PRIBIO);
	f->fd_status |= OPENING;
	splx(oldpri);

	if (f->fd_status & OPENED) {
		if (dev != f->fd_device) {
			f->fd_status &= ~OPENING;
			wakeup(&f->fd_status);
			return(EBUSY);
		}
		if ((f->fd_status & EXCLUSV) || (flags & FEXCL)) {
			f->fd_status &= ~OPENING;
			wakeup(&f->fd_status);
			return(EBUSY);
		}
	}

	if ((flags & FEXCL)) 
		f->fd_status |= EXCLUSV;

	/* check for no diskette or open door */
	fdm_seek(unit, 0, 10);
	fdm_rezero(unit);
	if (!(f->fd_drvtype == DRV_5D)) {
		if (fd_door_sense == 1 && fdm_snsdskchg(unit)) { 
			f->fd_status &= ~(OPENING | EXCLUSV);
			wakeup(&f->fd_status);
			return(EIO);
		}
	}

	if (fdm_readid(unit, 0) == FE_ITOUT) {
		f->fd_status &= ~(OPENING | EXCLUSV);
		wakeup(&f->fd_status);
		return(EIO);
	}

	rtn = fdm_snsdrv(unit, 0);
	if ((rtn == FE_SNSDRV) ||
	     ((rtn & WPROT) && ((flags & FWRITE) == FWRITE))) {
		f->fd_status &= ~(OPENING | EXCLUSV);
		wakeup(&f->fd_status);
		return(EROFS);
	}

	if (fmt == FMT_AUTO) {
		fmt = fdm_dfmt(unit);
		if (fmt == FMT_UNKNOWN) {
			f->fd_status &= ~(OPENING | EXCLUSV);
			wakeup(&f->fd_status);
			return(ENXIO);
		}
	}

	f->fd_device = dev;
	f->fd_nsides = SIDES(dev);
	f->fd_maxerr = NORM_EMAX;
	fdm_setup(unit, fmt, part);

	f->fd_status &= ~(OPENING | DOOR_OPENED);
	f->fd_status |= (OPENED | RSTDOPN);
	wakeup(&f->fd_status);
	return(0);
}

/*
 * int
 * fdclose(dev_t dev, int flags, int otyp, struct cred *cred_p)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
fdclose(dev_t dev, int flags, int otyp, struct cred *cred_p)
{
	register struct fdstate *f;
	register struct buf *bp;
	int oldpri;
	int	unit;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdclose");
#endif
	unit = UNIT(dev);
	f = &fd[unit];
	f->fd_status |= CLOSING;
	oldpri = splhi();
	bp = fdtab.b_actf;
	while (bp != NULL) {
		if (bp->b_edev == dev) {
			sleep((caddr_t)&fdtab.b_actf,PRIBIO);
			bp = fdtab.b_actf;
		} else {
			bp = bp->av_forw;
		}
	}
	fd_mtimer[unit] = 0;
	fd_curmotors &= ~(ENABMOTOR << unit);
	outb(FDCTRL, fd_curmotors|NORESET|enabint|fdcst.fd_curdrv);
	splx(oldpri);

	f->fd_status &= ~EXCLUSV;
	f->fd_proc = NULL;

	f->fd_status &= ~(OPENED|CLOSING);
	wakeup(&f->fd_status);
	return(0);
}

/*
 * int
 * fdread(dev_t dev, struct uio *uio_p, struct cred *cred_p)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
fdread(dev_t dev, struct uio *uio_p, struct cred *cred_p)
{
	register struct fdstate *f;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_CONT,"<fdread>");
#endif
	f = &fd[UNIT(dev)];
	return(physiock((void (*)())fdstrategy, (buf_t *)NULL, dev, 
				B_READ, (daddr_t)(f->fd_n512b), uio_p));
}

/*
 * int
 * fdwrite(dev_t dev, struct uio *uio_p, struct cred *cred_p)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
fdwrite(dev_t dev, struct uio *uio_p, struct cred *cred_p)
{
	register struct fdstate *f;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_CONT,"<fdwrite>");
#endif
	f = &fd[UNIT(dev)];
	return(physiock((void (*)())fdstrategy, (buf_t *)NULL, dev, 
				B_WRITE, (daddr_t)(f->fd_n512b), uio_p));
}

/*
 * int
 * fdsize(dev_t dev)
 *	return size of partition if we know; -1 if not
 *	(if device is "f0" or "f1" and device hasn't been opened,
 *	then we do not know the size).
 *
 * Calling/Exit State:
 *	None.
 */
int
fdsize(dev_t dev)
{
	register struct fdstate *f;
	int	fmt, part, unit;

	unit = UNIT(dev);
	part = PARTITION(dev);
	fmt = FRMT(dev);
	if (unit < 0 || unit >= NUMDRV || fmt < 0 || fmt > FMT_MAX ||
	    part < 0 || (part >= FDNPART && fmt != FMT_5D8))
		return(-1);

#ifdef DEBUG
	if (fddebug)
		cmn_err(CE_CONT, "DEBUG fdsize:  unit %d part %d fmt %d\n",
			unit, part, fmt);
#endif

	f = &fd[unit];
	if (f->fd_drvtype == DRV_NONE ||
	    (fmt == FMT_AUTO && !(f->fd_status & OPENED))) {
#ifdef DEBUG
		if (fddebug)
			cmn_err(CE_CONT, "\tsize unknown\n");
#endif
		return(-1);
	}

	if (!(f->fd_status & OPENED)) {
		f->fd_nsides = SIDES(dev);
		fdm_setup(unit, fmt, part);
	}

#ifdef DEBUG
	if (fddebug)
		cmn_err(CE_CONT, "\tsize = %d\n", f->fd_n512b);
#endif /* DEBUG */

	return(f->fd_n512b);
}

/*
 * table for encoded number of bytes per sector.
 */
int sectab[4] = {128, 256, 512, 1024};

/*
 * int
 * fdioctl(dev_t dev, int cmd, caddr_t arg, int mode, struct cred *cred_p,
 *	int *rval_p)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
fdioctl(dev_t dev, int cmd, caddr_t arg, int mode, struct cred *cred_p,
	int *rval_p)
{
	register struct buf *bp;
	register secno, entry;
	register ushort i;
	register ushort cylinder;
	register caddr_t bptr;
	struct fdstate	*f;
	struct disk_parms disk_parms;
	union io_arg karg;
	ushort head, secsiz;
	int ecode;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdioctl");
#endif
	ecode = 0;

	f = &fd[UNIT(dev)];

	switch(cmd) {
	case V_GETPARMS: /* get floppy parameters */
		disk_parms.dp_type = DPT_FLOPPY;
		disk_parms.dp_heads = f->fd_nsides;
		disk_parms.dp_cyls = f->fd_ncyls;
		disk_parms.dp_sectors = f->fd_nsects;
		disk_parms.dp_secsiz = f->fd_secsiz;
		disk_parms.dp_ptag = 0; /* not implemented */
		disk_parms.dp_pflag = 0; /* not implemented */
		disk_parms.dp_pstartsec = f->fd_cylskp * f->fd_nsides * f->fd_nsects + fd_secskp[UNIT(dev)];
		disk_parms.dp_pnumsec = ((long) f->fd_n512b << SCTRSHFT) >> (long)f->fd_secsft;
		if (copyout((caddr_t)&disk_parms, arg, sizeof(disk_parms))) {
			return(EFAULT);
		}
		break;

	case V_FORMAT: /* format tracks */

		/*
		 * Get the user's argument.
		 */
		if ((FRMT(f->fd_device) == FMT_AUTO) ||
		     copyin(arg, (caddr_t)&karg, sizeof(karg))) {
			return(ENXIO);
		}
		/*
		 * Calculate starting head and cylinder numbers.
		 */
		head = karg.ia_fmt.start_trk % f->fd_nsides;
		cylinder = karg.ia_fmt.start_trk / f->fd_nsides + f->fd_cylskp;
		/*
		 * Get encoded bytes/sector from table.
		 */
		for (secsiz = 0; secsiz < sizeof(sectab)/sizeof(int); secsiz++)
			if (sectab[secsiz] == f->fd_secsiz)
				break;
		bp = geteblk();
		/*
		 * Format all the requested tracks.
		 */
		for (i = 0; i < karg.ia_fmt.num_trks; i++){ 
			if ((cylinder >= f->fd_ncyls) || (head >= f->fd_nsides)) {
				brelse(bp);
				return(EINVAL);
			}
			/*
			 * Initialize the buffer.
			 */
			bioreset(bp);
			bp->b_bcount = f->fd_nsects * sizeof(struct fdformid);
			bzero(bp->b_un.b_addr, bp->b_bcount);
			/*
			 * Build the format data.  For each sector, we have to
			 * have 4 bytes: cylinder, head, sector, and encoded 
			 * sector size.
			 */
			entry = 0;
			secno = 1;  /* 1-based for DOS */
			do {
				bptr = &bp->b_un.b_addr[entry];
				if (bptr[2] == '\0') {
					*bptr++ = (char)cylinder;
					*bptr++ = (char)head;
					*bptr++ = secno++;
					*bptr = (char)secsiz;
					entry = (entry + karg.ia_fmt.intlv * sizeof(struct fdformid))
								% bp->b_bcount;
				} else {
					entry += sizeof(struct fdformid);
				}
			} while (secno <= (int)f->fd_nsects);

			f->fd_maxerr = FORM_EMAX;
			bp->b_edev = dev;
			bp->b_fdcmd = FORMAT;
			fdxfer(bp);
			biowait(bp);
			if (++head >= f->fd_nsides) {
				head = 0;
				cylinder++;
			}
		}
		brelse(bp);
		f->fd_maxerr = NORM_EMAX;
		break;

	case F_RAW:
	{
		int n = UNIT(dev);
		int oldpri;
		int rw = B_READ;
		struct fdraw *fdrawptr = &fdraw[n];
		struct buf *rbp = fdrawbuf[n];
		int status;

		if (!(f->fd_status & EXCLUSV) ||
			copyin(arg, (caddr_t)fdrawptr, sizeof(*fdrawptr))) {
			return(ENXIO);
		}

		switch(fdrawptr->fr_cmd[0] & 0xF) {
		case SEEK:
		case SENSE_INT:
		case READID:
		case REZERO:
		case SENSE_DRV:
		case SPECIFY:
			fdrawptr->fr_nbytes = 0;
			/* FALLTHRU */
		case FORMAT:
		case WRCMD:
		case WRITEDEL:
			rw = 0;
			/* FALLTHRU */
		case RDCMD & 0xF:
		case READDEL:
		case READTRACK:

			/*
			 * What F_RAW's caller thinks is the unit #
			 * doesn't necessarily have anything
			 * to do with the driver's idea of the
			 * unit #.  Better fix it up now.
			 * NOTE: SPECIFY is the only command which
			 * uses fr_cmd[1] for anything other
			 * than head/unit info.
			 */
			if (fdrawptr->fr_cmd[0] != SPECIFY) {
				fdrawptr->fr_cmd[1] &= ~0x03;
				fdrawptr->fr_cmd[1] |= n;
			} else {
				/*
				 * SPECIFY better not turn off DMA --
				 * this driver is unequipped to handle it
				 */
				fdrawptr->fr_cmd[2] &= ~0x01;
			}

			bioreset(rbp);
			rbp->b_flags &= ~B_READ;
			rbp->b_flags |= rw;
			rbp->b_resid = 0;
			rbp->b_fdcmd = RAWCMD;
			rbp->b_edev = dev;
			rbp->b_bcount = fdrawptr->fr_nbytes;   /* for fdxfer */

			/*
			 * If we should attempt to  use  the  buffer
			 * address,  when  there  is  none,  make it
			 * fault.                                    
			 */
			rbp->b_un.b_addr = (caddr_t) 0xdf000000;

			/*
			 * Are we reading or writing any bytes.   If 
			 * so, then we need exclusive use of the dma
			 * memory that fdbufstruct is set up for.  
			 */
			if (rbp->b_bcount != 0) {
				while (fdbufstruct.fbs_flags & B_BUSY) {
					fdbufstruct.fbs_flags |= B_WANTED;
					sleep ((caddr_t)&fdbufstruct, PRIBIO);
				}
				fdbufstruct.fbs_flags |= B_BUSY;

				/*
				 * If the dma  buffer  is  currently 
				 * not    big   enough   then   call
				 * fdbufgrow to attempt to grow  it.
				 * The    buffer   currently   never 
				 * shrinks. 
				 */
				if (fdrawptr->fr_nbytes > fdbufstruct.fbs_size) {
					status = fdbufgrow(fdrawptr->fr_nbytes, 0);
					if (status) {
						ecode = status;
						goto rawdone;
					}
				}

				rbp->b_un.b_addr = fdbufstruct.fbs_addr;

				/*
				 * If it's a write,  copy  the  data 
				 * from the user area. 
				 */
				if (!(rbp->b_flags & B_READ)) {
					if (copyin(fdrawptr->fr_addr, rbp->b_un.b_addr,
							fdrawptr->fr_nbytes)) {
						ecode = EFAULT;
						goto rawdone;
					}
				}
			}

			fdxfer(rbp);
			oldpri = splhi();
			biowait(rbp);
			splx(oldpri);
			if (rbp->b_resid)
				ecode = rbp->b_resid;

			/*
			 * If it was a read, must copy out from kernel buffer
			 * to user.
			 */
			if (rbp->b_flags & B_READ && fdrawptr->fr_nbytes) {
				if (copyout(rbp->b_un.b_addr, fdrawptr->fr_addr,
						fdrawptr->fr_nbytes))
					ecode = EFAULT;
			}

		rawdone:
			/*
			 * If we  had  used  the  dma  buffer,  must 
			 * release it.                  
			 */
			if (rbp->b_bcount) {
				fdbufstruct.fbs_flags &= ~B_BUSY;
				if (fdbufstruct.fbs_flags & B_WANTED) {
					fdbufstruct.fbs_flags &= ~B_WANTED;
					wakeup((caddr_t)&fdbufstruct);
				}
			}
			break;
		default:
			ecode = EINVAL;
		}
		if (copyout((caddr_t)fdrawptr, arg, sizeof(*fdrawptr))) {
			return(EFAULT);
		}
		break;
	}

	case F_DOR:
		/*
		 * The only reasonable thing for a user to request is a reset,
		 * so just check that condition.
		 */
		if (!((*(char*)&arg) & NORESET))
			fdreset();
		break;

	case F_FCR:
		/* Here only the transfer rate is pertinent */
		f->fd_trnsfr = (*(char*)&arg) & 0x03;
		break;

	case F_DTYP:
		if (copyout((caddr_t)&(f->fd_drvtype), (caddr_t)arg, sizeof(f->fd_drvtype))) {
			return(EFAULT);
		}
		break;

#ifdef MERGE386
	case V_SETPARMS:	/* set floppy parameters */
	{
		int fmt;

		if (copyin(arg, &disk_parms, sizeof disk_parms) == -1)
			return(EFAULT);
		fmt = fdm_dfmt(UNIT(dev));
		if (fmt == FMT_UNKNOWN)
			return(ENXIO);
		f->fd_nsides = disk_parms.dp_heads;
		fdm_setup(UNIT(dev), fmt, PARTITION(dev));
		if (f->fd_nsects != disk_parms.dp_sectors ||
		    f->fd_ncyls != disk_parms.dp_cyls ||
		    f->fd_secsiz != disk_parms.dp_secsiz)
			return(ENXIO);
		f->fd_status |= RECAL;
		fdreset();
		break;
	}

	case V_GETFLOPSTAT:	/* get floppy status */
	{
		struct flop_status fs;

		if (f->fd_drvtype != DRV_5D ) {
			if((f->fd_status & DOOR_OPENED) || (fdm_snsdskchg(UNIT(dev)))) {
				fs.fs_changeline = 1;
			}
			else {
				fs.fs_changeline = 0;
			}
		}

		fs.fs_lastopst = FD_NO_ERR;

		if (copyout(&fs, arg, sizeof fs) == -1) {
			return(EFAULT);
		}

		break;
	}

	case V_RESET:		/* reset floppy status */

		fdm_seek( UNIT(dev), 1, 1);
		fdm_seek( UNIT(dev), 0, 0);

		f->fd_status &= ~DOOR_OPENED;

		break;
#endif /* MERGE386 */

	default:
		ecode = EINVAL;
	}
	return(ecode);
}

/*
 * STATIC void
 * fdstrat0(buf_t *bp)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
fdstrat0(buf_t *bp)
{
	register struct fdstate *f;

	if (bp->b_bcount == 0) {
		biodone(bp);
		return;
	}

	f = &fd[UNIT(bp->b_edev)];

	if (bp->b_blkno >= (long)f->fd_n512b) {
		if (bp->b_blkno > (long)f->fd_n512b || 
		    (bp->b_flags & B_READ) == 0) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
		}
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}
	bp->b_fdcmd = (bp->b_flags & B_READ) ? RDCMD : WRCMD;
	fdxfer(bp);
#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_CONT,"<fdstrategy(0x%lx) dopne>",bp->b_blkno);
#endif
	return;
}

/*
 * int
 * fdstrategy(register struct buf *bp)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdstrategy(register struct buf *bp)
{
#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_CONT,"fdstrategy(0x%lx)",bp->b_blkno);
#endif
	buf_breakup(fdstrat0, bp, fdbcb);
	return(0);
}

/*
 * int
 * fdxfer(register struct buf *bp)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdxfer(register struct buf *bp)
{
	register int oldpri;
	clock_t lbolt_val;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdxfer(0x%lx)",bp->b_blkno);
#endif
	if (drv_getparm(LBOLT, &lbolt_val) == -1)
		return(EINVAL);

	bp->av_forw = NULL;
	bp->b_start = lbolt_val;

	{
		register struct iotime *fit;

		fit = fdstptr[UNIT(bp->b_edev)];
		fit->io_cnt++;
		fit->io_bcnt += btopr(bp->b_bcount);
	}

	{

		oldpri = splhi();
		if (fdtab.b_actf == NULL)
			fdtab.b_actf = bp;
		else
			fdtab.b_actl->av_forw = bp;
		fdtab.b_actl = bp;
		splx(oldpri);
	}
	if (fdtab.b_active == 0) {
		fdtab.b_errcnt = 0;
		fdiostart();
	}
	return(0);
}

/*
 * void
 * fdiostart(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
fdiostart(void)
{
	register struct buf *bp;
	struct fdstate *f;
	clock_t lbolt_val;

	if ((bp = fdtab.b_actf) == NULL)
		return;
#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdiostart(0x%lx)",bp->b_blkno);
#endif
	fdtab.b_active = 1;
	f = &fd[UNIT(bp->b_edev)];

	if (bp->b_fdcmd == RAWCMD) {	/* non-standard processing */
		outb(FDCSR1, f->fd_trnsfr);
		fdrawio(bp);
		return;
	}

	if (bp->b_fdcmd == MTNCMD) {	/* maintenance channel */
		outb(FDCSR1, f->fd_trnsfr);
		fdmtnio(bp);
		return;
	}

	if (bp->b_fdcmd == FORMAT) {
		fdcs.fd_track = ((struct fdformid *)paddr(bp))->fdf_track;
		fdcs.fd_head = ((struct fdformid *)paddr(bp))->fdf_head;
		fdcst.fd_btot = bp->b_bcount;
	} else {
		register int i;

		i = bp->b_blkno;
		if(i + (bp->b_bcount >> SCTRSHFT) > f->fd_n512b)
			fdcst.fd_btot = (f->fd_n512b - i) << SCTRSHFT;
		else
			fdcst.fd_btot = bp->b_bcount;
		fdcst.fd_blkno = ((long)(unsigned)i << SCTRSHFT) >> (long)f->fd_secsft;
		fdcst.fd_blkno += fd_secskp[UNIT(bp->b_edev)];
		i = f->fd_cylsiz;
		fdcs.fd_track = fdcst.fd_blkno / (ushort)i;
		i = fdcst.fd_blkno % (ushort)i;
		fdcs.fd_head = i / (int)f->fd_nsects;
		fdcs.fd_sector = (i % (int)f->fd_nsects) + 1; /* sectors start at 1 */
		fdcs.fd_track += f->fd_cylskp;
	}
	fdcst.fd_addr = vtop((caddr_t)paddr(bp), bp->b_proc);
	bp->b_resid = fdcst.fd_btot;

	{
		register struct iobuf *fdu;

		fdu = fdutptr[UNIT(bp->b_edev)];
		if (fdu->b_active == 0) {
			fdu->b_active++;
			if (drv_getparm(LBOLT, &lbolt_val) == -1)
				return;
			fdu->io_start = lbolt_val;
		}
	}
		outb(FDCSR1, f->fd_trnsfr);
	{
		register int n;

		do {
			if (fdcst.fd_state != DOOR_OPEN)
				fdcst.fd_state = CHK_RECAL;
			n = fdio(bp);
		} while (n && fderror(bp, n) == 0);
	}
	return;
}

/*
 * int
 * fdio(register struct buf *bp)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdio(register struct buf *bp)
{
	register int n;
	struct fdstate *f;
	struct fdparam  *ff;
	int	oldpri;
	int	cmdsiz;
	int	retval;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdio(0x%lx)",bp->b_blkno);
#endif

	n = UNIT(bp->b_edev);
	f = &fd[n];
	oldpri = splhi();
	if (fd_mtimer[n]) /* keep motor running */
		fd_mtimer[n] = RUNTIM;
	splx(oldpri);

	if (fd_mtimer[n] == 0) {	/* start floppy motor first */
		fd_mtimer[n] = RUNTIM;
		fd_curmotors |= (ENABMOTOR << n);
		fdcst.fd_curdrv = (char)n;
		outb(FDCTRL, fd_curmotors|enabint|NORESET|n);
		fdwait_timer =  itimeout((void (*)())fdwait, 
						(caddr_t)bp, f->fd_mst, fdpl);
		if (fdtimer_timer == 0)
			fdtimer_timer = itimeout((void (*)())fdtimer, 
						(caddr_t)0, MTIME, fdpl);
		retval = 0;
		goto done;
	}
	if (fdcst.fd_curdrv != n) {
		fdcst.fd_curdrv = (char)n;
		outb(FDCTRL, fd_curmotors|enabint|NORESET|n);
	}
	fdcs.fd_cmdb2 = (fdcs.fd_head << 2) | n;
	if (f->fd_status & RSTDOPN) {
		f->fd_status &= ~RSTDOPN;
		goto dooropen;
	}

	if (f->fd_drvtype != DRV_5D && fdcst.fd_state >= CHK_RECAL) {
		if ((fd_door_sense == 1) && (inb(FDCSR1) & DOOROPEN)) {
			fdcst.fd_state = DOOR_OPEN;
			f->fd_status |= RECAL|DOOR_OPENED;
			fd_mtimer[n] = 0;
			fd_curmotors &= ~(ENABMOTOR << n);
			outb(FDCTRL, fd_curmotors|enabint|NORESET|n);
			fddooropen(f);
			retval = 0;
			goto done;
		}
	}

	switch(fdcst.fd_state) {
dooropen:
	case DOOR_OPEN:
		fdcst.fd_savtrk = fdcs.fd_track;
		/* force a real seek */
		fdcs.fd_track++;
		if (fdcs.fd_track == f->fd_ncyls)
			fdcs.fd_track -= 2;
		retval = fd_doseek(f);
		goto done;
	case DOOR_RST:
		if (retval = fdsense(fdcs.fd_track)) {
			fdcst.fd_state = DOOR_OPEN;
			goto done;
		}
		fdcst.fd_state = CHK_RECAL;
		fdcs.fd_track = fdcst.fd_savtrk;
		/* FALLTHRU */
	case CHK_RECAL: /* check for rezero */
		if (f->fd_status & RECAL) {
			fdcs.fd_cmdb1 = REZERO;
			fdcst.fd_cstat |= WINTR;
			retval = fdcmd((caddr_t)&fdcs, 2);
			goto done;
		}
		goto seek;
	case RECAL_DONE: /* come here for REZERO completion */
		if (retval = fdsense(0))
			goto done;
		f->fd_curcyl = 0;
		f->fd_status &= ~RECAL;
seek:
		fdcst.fd_state = CHK_SEEK;
		/* FALLTHRU */
	case CHK_SEEK:
		if (f->fd_curcyl != fdcs.fd_track) {
			retval = fd_doseek(f);
			goto done;
		}
		fdcst.fd_state = XFER_BEG;
		/* FALLTHRU */
	case XFER_BEG: /* come here for r/w operation */
		ff = &fdparam[fdcst.fd_curdrv];
		if ((fdcs.fd_cmdb1 = (unsigned char)bp->b_fdcmd)
		     == (unsigned char)FORMAT) {
			fdcs.fd_bps = ff->fdf_bps;
			fdcs.fd_spt = ff->fdf_spt;
			fdcs.fd_fil = ff->fdf_fil;
			fdcs.fd_gap = ff->fdf_gplf;
			n = bp->b_bcount;
			cmdsiz = 6;
		} else {
			if ((n = ((f->fd_nsects + 1) -
			    fdcs.fd_sector) << f->fd_secsft) > bp->b_resid)
				n = bp->b_resid;
			fdcs.fd_lstsec = f->fd_nsects;
			fdcs.fd_gpl    = ff->fdf_gpln;
			fdcs.fd_ssiz   = ff->fdf_bps;
			fdcs.fd_dtl    = ff->fdf_dtl;
			cmdsiz = 9;
		}
		fdcs.fd_cmdb1 |= ff->fdf_den;
		fdcst.fd_bpart = (ushort)n;
		if (n) {
			fdcb->targbufs->address = fdcst.fd_addr;
			fdcb->targbufs->count = (ushort)n;
			if (bp->b_flags & B_READ)
				fdcb->command = DMA_CMD_READ;
			else
				fdcb->command = DMA_CMD_WRITE;
			if (dma_prog(fdcb, DMA_CH2, DMA_NOSLEEP) == FALSE) {
				retval = ENXIO;
				goto done;
			}
			dma_enable(DMA_CH2);
		}
		fdcst.fd_cstat |= WINTR;
		retval = fdcmd((caddr_t)&fdcs, cmdsiz);
		goto done;
	case SEEK_DONE: /* come here for SEEK completion */
		if (retval = fdsense(fdcs.fd_track))
			goto done;
		f->fd_curcyl = fdcs.fd_track;
		fdcst.fd_state = XFER_BEG;
		fdwait_timer = itimeout((void (*)())fdwait, 
						(caddr_t)bp, f->fd_hst, fdpl);
		retval = 0;
		goto done;
	}
	retval = 1;
done:
	return(retval);
}

/*
 * int
 * fd_doseek(register struct fdstate *f)
 *
 * Calling/Exit State:
 *	None.
 */
int
fd_doseek(register struct fdstate *f)
{
	register int n;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fddoseek");
#endif
	fdcs.fd_cmdb1 = SEEK;
	fdcst.fd_cstat |= WINTR;
	if (f->fd_dstep)
		fdcs.fd_track <<= 1;
	n = fdcmd((caddr_t)&fdcs, 3);
	if (f->fd_dstep)
		fdcs.fd_track >>= 1;
	return(n);
}

/*
 * void
 * fdintr(int ivect)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
fdintr(int ivect)
{
	register struct buf *bp;
	register int n;
	struct fdstate *f;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdintr");
#endif

	dma_disable(DMA_CH2);	 /* disable channel 2 of the DMA chip */
	if (((bp = fdtab.b_actf) == NULL) || !(fdcst.fd_cstat & WINTR)) {
		/*
		 * Reading sense data will reset interrupt. This is essential 
		 * for mca machines, since interrupt is level sensitive and 
		 * will remain high until reset.
		 */
		struct f1 fdintrsns; /* space for new interrupt sense data. */
		unsigned char cmnd;

		cmnd = SENSE_INT;
		/* issue the command to floppy drive */
		if (fdcmd((caddr_t)&cmnd, 1))
			return;
		/* get result */
		else if (fdresult((caddr_t)&fdintrsns, sizeof(fdintrsns)))
			return;
		return;
	}

	fdcst.fd_cstat &= ~WINTR;

	if (bp->b_fdcmd == RAWCMD) {	/* non-standard processing */
		if (fdraw[UNIT(bp->b_edev)].fr_cmd[0] != SEEK &&
		    fdraw[UNIT(bp->b_edev)].fr_cmd[0] != REZERO &&
		    fdresult(fdraw[UNIT(bp->b_edev)].fr_result, 7)) {
				bp->b_resid = EIO;
		}
		fdtab.b_active = 0;
		fdtab.b_actf = bp->av_forw;
		biodone(bp);
		fdiostart();
		return;
	}

	if (bp->b_fdcmd == MTNCMD) {	/* maintenance channel */
		if (fdmtn[UNIT(bp->b_edev)].fr_cmd[0] == SEEK || 
		    fdmtn[UNIT(bp->b_edev)].fr_cmd[0] == REZERO) {
			fdcs.fd_cmdb1 = SENSE_INT;
			if (fdcmd((caddr_t)&fdcs, 1))
				bp->b_resid = (unsigned int)FE_CMD;
			if (fdresult(fdmtn[UNIT(bp->b_edev)].fr_result, 2))
				bp->b_resid = (unsigned int)FE_RSLT;
		} else {	
		    if (fdresult(fdmtn[UNIT(bp->b_edev)].fr_result, 7))
				bp->b_resid = (unsigned int)FE_RSLT;
		}
		fdtab.b_active = 0;
		fdtab.b_actf = bp->av_forw;
		biodone(bp);
		fdiostart();
		return;
	}

	if (++fdcst.fd_state >= XFER_DONE) {
		if (n = fdresult((caddr_t)&fdstatus, 7)) {
			fderror(bp, n);
			fdiostart();
			return;
		}
		if ((fdstatus.fd_st0 & (ABNRMTERM|INVALID|EQCHK|NOTRDY))) {
			fderror(bp, 1);
			fdiostart();
			return;
		}
		f = &fd[fdcst.fd_curdrv];
		if ((bp->b_resid -= fdcst.fd_bpart) == 0) {
			fddone(bp);
			fdiostart();
			return;
		}
		if (++fdcs.fd_head >= f->fd_nsides) {
			fdcs.fd_head = 0;
			fdcs.fd_track++;
			fdcst.fd_state = CHK_SEEK;  /* go back to seek state */
		} else {
			fdcst.fd_state = XFER_BEG; /* stay in data xfer state */
		}
		fdcs.fd_sector = 1;
		fdcst.fd_addr += fdcst.fd_bpart;
		fdcst.fd_blkno += (fdcst.fd_bpart >> f->fd_secsft);
	}	
	if (n = fdio(bp)) {
		fderror(bp,n);
		fdiostart();
	}
	return;
}

/*
 * int
 * fddone(register struct buf *bp)
 *
 * Calling/Exit State:
 *	None.
 */
int
fddone(register struct buf *bp)
{
	int oldpri;
	register struct iotime *fit;
	struct iobuf *fdu;
	clock_t lbolt_val;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fddone");
#endif
	oldpri = splhi();
	if (fd_mtimer[fdcst.fd_curdrv])
		fd_mtimer[fdcst.fd_curdrv] = WAITTIM;
	splx(oldpri);
	fdu = fdutptr[UNIT(bp->b_edev)];
	fit = fdstptr[UNIT(bp->b_edev)];

	if (drv_getparm(LBOLT, &lbolt_val) == -1) {
		return(EINVAL);
	}

	fit->io_resp += lbolt_val - bp->b_start;
	fit->io_act += lbolt_val - fdu->io_start;
	fdu->b_active = 0;
	fdtab.b_active = 0;
	fdtab.b_errcnt = 0;
	fdtab.b_actf = bp->av_forw;
	wakeup((caddr_t)&fdtab.b_actf);
	if ((bp->b_resid += (bp->b_bcount - fdcst.fd_btot)) != 0) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
	} else {
		bp->b_error = 0;
	}
	biodone(bp);
	return(0);
}

/*
 * int
 * fdsense(int cylnum)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdsense(int cylnum)
{
	register int n;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdsense");
#endif
	fdcs.fd_cmdb1 = SENSE_INT;
	if (n = fdcmd((caddr_t)&fdcs, 1))
		goto done;
	if (n = fdresult((caddr_t)&fdsnst, 2))
		goto done;
	if (fd[fdcst.fd_curdrv].fd_dstep)
		cylnum <<= 1;
	if ((fdsnst.fd_sst0 & (INVALID|ABNRMTERM|SEEKEND|EQCHK|NOTRDY)) !=
	    SEEKEND || fdsnst.fd_scyl != cylnum)
		n = 1;
done:
	return(n);
}

/*
 * int
 * fderror(struct buf *bp, int type)
 *
 * Calling/Exit State:
 *	None.
 */
int
fderror(struct buf *bp, int type)
{
	register struct fdstate *f;
	register int i;
	unsigned char ltrack;
	int error;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fderror");
#endif

	f = &fd[fdcst.fd_curdrv];

	if (type == 1) {
		f->fd_status |= RECAL;
		if (fdcst.fd_state >= XFER_BEG && bp->b_fdcmd != FORMAT) {
			ltrack = fdstatus.fd_strack;
			if (f->fd_dstep)
				ltrack >>= 1;
			i = fdstatus.fd_ssector - fdcs.fd_sector;
			fdcs.fd_track = ltrack;
			fdcs.fd_sector = fdstatus.fd_ssector;
			fdcst.fd_blkno += i;
			i <<= f->fd_secsft;
			fdcst.fd_addr += i;
			bp->b_resid -= i;
		}
	} else {
		fdreset();
	}

	if (++fdtab.b_errcnt > f->fd_maxerr) {
		if (type == 1) {
			if (fdcst.fd_state >= XFER_BEG) {
				f->fd_lsterr = fdstatus.fd_st1;
				error = (fdstatus.fd_st1<<8)|fdstatus.fd_st2;
				for (i = 0; i < 16 && ((error&01) == 0); i++)
					error >>= 1;
			} else {
				i = 4; /* fake a seek error */
			}

#ifdef DEBUG
			/*
			 *+ debugging
			 */
			cmn_err(CE_CONT,"\n!fderror: fd_st0 ....... 0x%x\n", fdstatus.fd_st0);
			cmn_err(CE_CONT,"!         fd_st1 ....... 0x%x\n", fdstatus.fd_st1);
			cmn_err(CE_CONT,"!         fd_st2 ....... 0x%x\n", fdstatus.fd_st2);
			cmn_err(CE_CONT,"!         fd_strack .... 0x%x\n", fdstatus.fd_strack);
			cmn_err(CE_CONT,"!         fd_shead ..... 0x%x\n", fdstatus.fd_shead);
			cmn_err(CE_CONT,"!         fd_ssector ... 0x%x\n", fdstatus.fd_ssector);
			cmn_err(CE_CONT,"!         fd_ssecsiz ... 0x%x\n\n", fdstatus.fd_ssecsiz);
#endif

			if (bp->b_fdcmd == FORMAT) {
				/*
				 *+ Error occurred on floppy driver.
				 */ 
				cmn_err(CE_WARN,"!FD  drv %d, trk %d: %s",fdcst.fd_curdrv,
				    f->fd_curcyl, fd_drverr[i]);
			} else {
				/*
				 *+ Error occurred on floppy driver.
				 */ 
				cmn_err(CE_WARN,"!FD  drv %d, blk %d: %s\n",fdcst.fd_curdrv,
				    fdcst.fd_blkno, fd_drverr[i]);
			}
			fdreset();
		} else {
			/*
			 *+ Error occurred on floppy driver.
			 */ 
			cmn_err(CE_WARN,"FD controller %s",fd_ctrerr[type-2]);
		}
		bp->b_flags |= B_ERROR;
		fddone(bp);
		return(1);
	}
	if (fdtab.b_errcnt == TRYRESET)
		fdreset();
	fdcst.fd_state = CHK_RECAL;
	return(0);
}

/*
 * void
 * fdwait(register struct buf *bp)
 *	Entered after a timeout expires when waiting for
 *	floppy drive to perform something that takes too
 *	long to busy wait for.
 *
 * Calling/Exit State:
 *	None.
 */
void
fdwait(register struct buf *bp)
{
	register int n;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdwait");
#endif
	fdwait_timer = 0;

	if (n = fdio(bp)) {
		fderror(bp,n);
		fdiostart();
	}
	return;
}

/*
 * void
 * fdnoflop(register struct fdstate *f)
 *
 * Calling/Exit State:
 *	None.
 */
void
fdnoflop(register struct fdstate *f)
{
	register struct buf *bp;
#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdnoflop");
#endif
	fdnoflop_timer = 0;

	bp = fdtab.b_actf;
	if (bp == NULL)
		return;
	if (fdcst.fd_etimer == 0)
		return;
	if (--fdcst.fd_etimer == 0) {
		if (++fdtab.b_errcnt >= NOFLOP_EMAX) {
			bp->b_flags |= B_ERROR;
			fddone(bp);
		}
		fdiostart();
	} else {
		fdnoflop_timer = itimeout((void (*)())fdnoflop,
						(caddr_t)f, ETIMOUT, fdpl);
	}
	return;
}

/*
 * void
 * fdtimer(int dummy)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
fdtimer(int dummy)
{
	register int i;
	register struct fdstate *f;
	struct buf *bp;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdtimer");
#endif
	fdtimer_timer = 0;

	for (i = 0; i < NUMDRV; i++) {
		if (fd_mtimer[i] == 0)
			continue;
		if (--fd_mtimer[i] == 0) {
			fd_curmotors &= ~(ENABMOTOR << i);
			outb(FDCTRL, fd_curmotors|NORESET|enabint|fdcst.fd_curdrv);
			if (i == fdcst.fd_curdrv && (fdcst.fd_cstat&WINTR)) {
				fdcst.fd_cstat &= ~WINTR;
				bp = fdtab.b_actf;
				f = &fd[fdcst.fd_curdrv];
				fdreset();

				if ((f->fd_status & EXCLUSV)) {
					fdtab.b_active = 0;
					fdtab.b_actf = bp->av_forw;
					bp->b_resid = EBUSY;
					biodone(bp);
					fdiostart();
				} else if (bp->b_fdcmd == MTNCMD) {
					bp->b_resid = (unsigned int)FE_ITOUT;
					fdtab.b_active = 0;
					fdtab.b_actf = bp->av_forw;
					biodone(bp);
					fdiostart();
				} else {
					fddooropen(f);
				}
			}
		}
	}
	if (fd_curmotors) {
		fdtimer_timer = itimeout((void (*)())fdtimer, 
						(caddr_t)0, MTIME, fdpl);
		fdt_running = 1;
	} else {
		fdt_running = 0;
	}
	return;
}

/*
 * void
 * fddooropen(register struct fdstate *f)
 *
 * Calling/Exit State:
 *	None.
 */
void
fddooropen(register struct fdstate *f)
{
	/*
	 *+ No diskette present in floppy drive.
	 */
	cmn_err(CE_WARN,"FD(%d): No diskette present - Please insert",fdcst.fd_curdrv);
	fdcst.fd_etimer = LOADTIM;
	fdnoflop_timer = itimeout((void (*)())fdnoflop, 
					(caddr_t)f, ETIMOUT, fdpl);
	return;
}

/*
 * void
 * fdreset(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
fdreset(void)
{
	register int i;
	register int oldpri;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdreset");
#endif
	for (i = 0; i < NUMDRV; i++)
		fd[i].fd_status |= RECAL;
	fdcst.fd_cstat &= ~WINTR;
	oldpri = splhi();
	outb(FDCTRL, enabint);        /* reset floppy controller */
	for (i = 0; i < 5; i++)
		drv_usecwait(30); /* wait 30 usec for NEC */
	outb(FDCTRL, fd_curmotors|enabint|NORESET|fdcst.fd_curdrv);
	splx(oldpri);
	fdsense(0);
	fdcmd((caddr_t)&fdrcs, 3);

	/*
	 * Not all Floppy Controllers support the CONFIG command. Those
	 * that do not should simply fail the request as an invalid
	 * opcode, and the fdresult should clean up any side effects
	 * from a failure. As a safe guard, the fd_do_config_cmd symbol
	 * is set in the fd's Space.c, and that controls whether we
	 * we issue the command. If a user encounters a problem accessing
	 * the floppy, this symbol can be set to 0, thus disabling the
	 * use of the CONFIG command, just in case it is causing an
	 * unforseen problem.
	 */
	if ( fd_do_config_cmd == 1 ) {
		if ( fdcmd((caddr_t)&fd_config, 4) != 0 ) {
			fdresult(fdmtn[0].fr_result, 1);
		}
	}

	return;
}

/*
 * int
 * fdprint(dev_t dev, char *str)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdprint(dev_t dev, char *str)
{
	/*
	 *+ error from fdprint for floppy drive
	 */
	cmn_err(CE_NOTE, "%s on floppy disk unit %d, partition %d",
		str, UNIT(dev), PARTITION(dev));
	return 0;
}

/*
 * int
 * fdresult(caddr_t addr, int cnt)
 *	Routine to return diskette drive status information
 *	The diskette controller data register is read the 
 *	requested number of times and the results placed in
 *	consecutive memory locations starting at the passed
 *	address.
 *
 * Calling/Exit State:
 *	None.
 */
int
fdresult(caddr_t addr, int cnt)
{
	unsigned char	msr;
	register int	i, j;
	int		ntries;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdresult");
#endif
	for (i = cnt; i > 0; i--) {
/*
 * read main status register to see if OK to read data.
 */
		ntries = FCRETRY;
		/* CONSTANTCONDITION */
		while (TRUE) {
			msr = inb(FDSTAT);
			if ((msr & (IODIR|IORDY)) == (IODIR|IORDY))
				break;
			if (--ntries <= 0)
				return RTIMOUT;
			else
				drv_usecwait(10);
		}
		for (j = 0; j < 5; j++)
			drv_usecwait(30); /* wait 30 usec for NEC */
		*addr++ = inb(FDDATA);
	}
	for (j = 0; j < 5; j++)
		drv_usecwait(30); /* wait 30 usec for NEC */
	msr = inb(FDSTAT);
	if (msr & FCBUSY)
		return NECERR;
	return 0;
}

/*
 * int
 * fdcmd(caddr_t cmdp, int size)
 *	routine to program a command into the floppy disk controller.
 *	the requested number of bytes is copied from the given address
 *	into the floppy disk data register.
 *
 * Calling/Exit State:
 *	None.
 */
int
fdcmd(caddr_t cmdp, int size)
{
	unsigned char	msr;
	register int	i, j;
	int		ntries;

#ifdef DEBUG
	if (fddebug) {
		char *cptr;
		/*
		 *+ debugging
		 */
		cptr = cmdp;
		cmn_err(CE_CONT,"!\nfdcmd: ");
		for( i = 0; i < size; ++i, ++cptr ) {
			/*
			 *+ debugging
			 */
			cmn_err( CE_CONT, "!0x%x ", (uint) *cptr);
		}
		/*
		 *+ debugging
		 */
		cmn_err(CE_CONT, "!\n");
	}
#endif

	for (i = size; i > 0; i--) {
		ntries = FCRETRY;
		/* CONSTANTCONDITION */
		while (TRUE) {
			msr = inb(FDSTAT);
			if ((msr & (IODIR|IORDY)) == IORDY)
				break;

			if (--ntries <= 0) {
				return CTIMOUT;
			}
			else {
				drv_usecwait(10);
			}
		}
		for (j = 0; j < 5; j++)
			drv_usecwait(30); /* wait 30 usec for NEC */
		outb(FDDATA, *cmdp++);
	}
	return 0;
}


/*
 * int
 * fdrawio(register struct buf *bp)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdrawio(register struct buf *bp)
{
	int rnum;			/* number of result bytes */
	register int n = UNIT(bp->b_edev);
	int oldpri;
#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdrawio");
#endif

	if (fdrawio_timer != 0)
		fdrawio_timer = 0;

	/*
	 * This function is called directly from fdiostart() when a bp
	 * specifying a RAWCMD is found.  It is responsible for completely
	 * processing the raw request.
	 */

	oldpri = splhi();
	if (fd_mtimer[n]) /* if motor's running, keep it going */
		fd_mtimer[n] = RUNTIM;
	splx(oldpri);

	/* first turn on the specified drive's motor if it isn't running */
	if (fd_mtimer[n] == 0) {
		fd_mtimer[n] = RUNTIM;
		fdcst.fd_curdrv = (char)n;
		fd_curmotors |= (ENABMOTOR << n);
		outb(FDCTRL, fd_curmotors | enabint | NORESET | n);
		/* wait for motor */
		fdrawio_timer = itimeout((void (*)())fdrawio,
					(caddr_t)bp, fd[n].fd_mst, fdpl);
		if (fdtimer_timer == 0)
			fdtimer_timer = itimeout((void (*)())fdtimer, 
					(caddr_t)0, MTIME, fdpl);
		return(0);
	}
	if (fdcst.fd_curdrv != n) {
		fdcst.fd_curdrv = (char)n;
		outb(FDCTRL, fd_curmotors|enabint|NORESET|n);
	}

	/* if data-transfer or SENSE_DRV command, check for media change */
	if (bp->b_bcount && (fd_door_sense == 1 ) && (inb(FDCSR1) & DOOROPEN)) {
		fd[n].fd_status |= DOOR_OPENED;
		bp->b_resid = EBUSY;
		goto finish;
	}

	if (bp->b_bcount) {
		fdcb->targbufs->address = kvtophys((vaddr_t)bp->b_un.b_addr);
		fdcb->targbufs->count = bp->b_bcount;
		if (bp->b_flags & B_READ)
			fdcb->command = DMA_CMD_READ;
		else
			fdcb->command = DMA_CMD_WRITE;
		if (dma_prog(fdcb, DMA_CH2, DMA_NOSLEEP) == FALSE) {
			bp->b_resid = ENXIO;
			fdreset();
			goto finish;
		}
		dma_enable(DMA_CH2);
	}
	fdcst.fd_cstat |= WINTR;
	if (fdcmd(fdraw[n].fr_cmd, fdraw[n].fr_cnum)) { /* controller error */
		bp->b_resid = EIO;
		fdreset();
		goto finish;
	}

	switch (fdraw[n].fr_cmd[0]) {
	/* no interrupt will be received in these cases */
	case SENSE_INT:
		rnum = 2;
		goto a;
	case SPECIFY:
		rnum = 0;
		goto a;
	case SENSE_DRV:
		rnum = 1;
a:
		fdcst.fd_cstat &= ~WINTR;
		if (fdresult(fdraw[n].fr_result, rnum)) {
			bp->b_error = EIO;
			fdreset();
		}
		goto finish;
	default:
		return(0);
	}
finish:
	fdtab.b_active = 0;
	fdtab.b_actf = bp->av_forw;
	biodone(bp);
	fdiostart();
	return(0);
}

/*
 * void
 * fdmtnio(register struct buf *bp)
 *
 * Calling/Exit State:
 *	None.
 */
void
fdmtnio(register struct buf *bp)
{
	int rnum;			/* number of result bytes */
	register int n = UNIT(bp->b_edev);
	int oldpri;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdmtnio");
#endif
	if (fdmtnio_timer != 0)
		fdmtnio_timer = 0;

	oldpri = splhi();
	if (fd_mtimer[n]) /* if motor's running, keep it going */
		fd_mtimer[n] = RUNTIM;
	splx(oldpri);

	/* first turn on the specified drive's motor if it isn't running */
	if (fd_mtimer[n] == 0) {
		fd_mtimer[n] = RUNTIM;
		fdcst.fd_curdrv = (char)n;
		fd_curmotors |= (ENABMOTOR << n);
		outb(FDCTRL, fd_curmotors | enabint | NORESET | n);
		/* wait for motor */
		fdmtnio_timer = itimeout((void (*)())fdmtnio, 
					(caddr_t)bp, fd[n].fd_mst, fdpl);
		if (fdtimer_timer == 0)
			fdtimer_timer = itimeout((void (*)())fdtimer, 
					(caddr_t)0, MTIME, fdpl);
		return;
	}
	if (fdcst.fd_curdrv != n) {
		fdcst.fd_curdrv = (char)n;
		outb(FDCTRL, fd_curmotors|enabint|NORESET|n);
	}

	if (fdmtn[n].fr_cmd[0] == SENSE_DSKCHG) {
		oldpri = splhi();
		if ((fdmtn[n].fr_result[0] = (inb(FDCSR1) & DOOROPEN)) != 0)
			fd[n].fd_status |= DOOR_OPENED;
		splx(oldpri);
		goto finish;
	}

	if (bp->b_bcount){
		fdcb->targbufs->address = kvtophys((vaddr_t)bp->b_un.b_addr);
		fdcb->targbufs->count = bp->b_bcount;
		if (bp->b_flags & B_READ)
			fdcb->command = DMA_CMD_READ;
		else
			fdcb->command = DMA_CMD_WRITE;
		if (dma_prog(fdcb, DMA_CH2, DMA_NOSLEEP) == FALSE) {
			bp->b_resid = ENXIO;
			fdreset();
			goto finish;
		}
		dma_enable(DMA_CH2);
	}
	fdcst.fd_cstat |= WINTR;
	if (fdcmd(fdmtn[n].fr_cmd, fdmtn[n].fr_cnum)) { /* controller error */
		bp->b_resid = (unsigned int)FE_CMD;
		fdreset();
		goto finish;
	}

	switch (fdmtn[n].fr_cmd[0]) {
	/* no interrupt will be received in these cases */
	case SENSE_INT:
		rnum = 2;
		goto a;
	case SENSE_DRV:
		rnum = 1;
a:
		fdcst.fd_cstat &= ~WINTR;
		if (fdresult(fdmtn[n].fr_result, rnum)) {
			/* controller error */
			bp->b_resid = (unsigned int)FE_RSLT;
			fdreset();
		}
		goto finish;
	default:
		return;
	}
finish:
	fdtab.b_active = 0;
	fdtab.b_actf = bp->av_forw;
	biodone(bp);
	fdiostart();
	return;
}

/*
 * int
 * fdm_seek(unsigned char drv, unsigned char head, unsigned char cyl)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdm_seek(unsigned char drv, unsigned char head, unsigned char cyl)
{
	int oldpri, rtn;
	struct buf *bp;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdm_seek");
#endif
	while (fdmtnbusy & (0x01 << drv))
		sleep((caddr_t)&fdmtn[drv], PRIBIO);
	fdmtnbusy |= (0x01 << drv);

	fdmtn[drv].fr_cmd[0] = SEEK; 
	fdmtn[drv].fr_cmd[1] = (head << 2) | drv;
	fdmtn[drv].fr_cmd[2] = cyl;
	fdmtn[drv].fr_cnum = 3;

	bp = fdmtnbuf[drv];	
	bioreset(bp);
	bp->b_flags &= ~B_READ;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	bp->b_fdcmd = MTNCMD;
	bp->b_edev = drv;

	fdxfer(bp);
	oldpri = splhi();
	biowait(bp);
	splx(oldpri);
	if (bp->b_resid || (fdmtn[drv].fr_result[0] & 0xC0) ||
			   (fdmtn[drv].fr_result[1] != cyl))
		rtn = FE_SEEK;	
	else
		rtn = cyl;


	fdmtnbusy &= ~(0x01 << drv);
	wakeup((caddr_t)&fdmtn[drv]);
	return(rtn);
}

/*
 * int
 * fdm_rezero(unsigned char drv)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdm_rezero(unsigned char drv)
{
	int oldpri, rtn;
	struct buf *bp;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdm_rezero");
#endif
	while (fdmtnbusy & (0x01 << drv))
		sleep((caddr_t)&fdmtn[drv], PRIBIO);
	fdmtnbusy |= (0x01 << drv);

	fdmtn[drv].fr_cmd[0] = REZERO;
	fdmtn[drv].fr_cmd[1] = drv;
	fdmtn[drv].fr_cnum = 2;

	bp = fdmtnbuf[drv];
	bioreset(bp);
	bp->b_flags &= ~B_READ;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	bp->b_fdcmd = MTNCMD;
	bp->b_edev = drv;

	fdxfer(bp);
	oldpri = splhi();
	biowait(bp);
	splx(oldpri);
	if (bp->b_resid || (fdmtn[drv].fr_result[0] & 0xC0) ||
			   (fdmtn[drv].fr_result[1] != 0))
		rtn = FE_REZERO;	
	else
		rtn = 0;

	fdmtnbusy &= ~(0x01 << drv);
	wakeup((caddr_t)&fdmtn[drv]);
	return(rtn);
}

/*
 * int
 * fdm_snsdrv(unsigned char drv, unsigned char head)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdm_snsdrv(unsigned char drv, unsigned char head)
{
	int oldpri, rtn;
	struct buf *bp;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdm_snsdrv");
#endif
	while (fdmtnbusy & (0x01 << drv))
		sleep((caddr_t)&fdmtn[drv], PRIBIO);
	fdmtnbusy |= (0x01 << drv);

	fdmtn[drv].fr_cmd[0] = SENSE_DRV;
	fdmtn[drv].fr_cmd[1] = (head << 2) | drv;
	fdmtn[drv].fr_cnum = 2;

	bp = fdmtnbuf[drv];
	bioreset(bp);
	bp->b_flags &= ~B_READ;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	bp->b_fdcmd = MTNCMD;
	bp->b_edev = drv;

	fdxfer(bp);
	oldpri = splhi();
	biowait(bp);
	splx(oldpri);
	if (bp->b_resid)
		rtn = FE_SNSDRV;
	else	
		rtn = fdmtn[drv].fr_result[0];

	fdmtnbusy &= ~(0x01 << drv);
	wakeup((caddr_t)&fdmtn[drv]);
	return(rtn);
}

/*
 * int
 * fdm_setup(char drv, char fmt, char  part)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdm_setup(char drv, char fmt, char  part)
{
	struct fdstate *f;
	struct fdparam *ff;
	char dt;
	int nsectors;

unsigned char pval;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdm_setup");
#endif
	if ((fmt < 0) || (fmt > FMT_MAX)) {
		return(FE_BARG);
	}

	f = &fd[drv];
	dt = f->fd_drvtype;

	f->fd_secsiz = fdsectab[fmt].fd_ssize;
	f->fd_secsft = fdsectab[fmt].fd_sshift;
	f->fd_secmsk = fdsectab[fmt].fd_ssize - 1;
	f->fd_nsects = fdsectab[fmt].fd_nsect;
	f->fd_cylsiz = f->fd_nsides * f->fd_nsects;
	
	if (fmt == FMT_5D8 && part == 3)
		fd_secskp[drv] = 8;
	else
		fd_secskp[drv] = 0;

	if (fmt >= FMT_5D9 && fmt <= FMT_5D16) {
		f->fd_cylskp = dblpart[part].startcyl;
		f->fd_ncyls = D_NCYL;
		nsectors = dblpart[part].numcyls * f->fd_cylsiz - fd_secskp[drv];
	} else if (fmt == FMT_5Q) {
		f->fd_cylskp = quadpart[part].startcyl;
		f->fd_ncyls = Q_NCYL;
		nsectors = quadpart[part].numcyls * f->fd_cylsiz;
	} else if (fmt == FMT_3N ) {
		f->fd_cylskp = medpart[part].startcyl;
		f->fd_ncyls = N_NCYL;
		nsectors = medpart[part].numcyls * f->fd_cylsiz;
	} else {
		f->fd_cylskp = highpart[part].startcyl;
		f->fd_ncyls = H_NCYL;
		nsectors = highpart[part].numcyls * f->fd_cylsiz;
	}
	f->fd_n512b = (int)((long) nsectors << f->fd_secsft) >> SCTRSHFT;

	if ((fmt >= FMT_5D9 && fmt <= FMT_5D16) && f->fd_drvtype == DRV_5H)
		f->fd_dstep = 1;
	else
		f->fd_dstep = 0;
	
	if (fd_eisa_brdid == 0xC255) {
		/********************************/
		/* Unisys 3-mode floppy support */
		/********************************/
		if ((dt == DRV_3H && fmt == FMT_3M) || 
		    (dt == DRV_3H && fmt == FMT_3N)) {
			pval = inb(0x028);
			outb(0x028, (pval | 0x10));
		} else {
			pval = inb(0x028);
			outb(0x028, (pval & ~0x10));
		}
	}

	if (dt == DRV_5D || fmt == FMT_3D) {
		f->fd_trnsfr = FD250KBPS;
	} else if ((dt == DRV_3H && fmt == FMT_3M) || 
	    (dt == DRV_3H && fmt == FMT_3N) ||  /** Unisys 3-mode floppy  **/
	    (dt == DRV_3H && fmt == FMT_3H) ||  /** Unisys 3-mode floppy  **/
	    (dt == DRV_3e && fmt == FMT_3H) ||  /** DELL 2.88Mb Floppy Support **/
	    (dt == DRV_3E && fmt == FMT_3H) ||  /** 2.88Mb Floppy Support **/
	    (dt == DRV_5H && fmt == FMT_5H) ) {
		f->fd_trnsfr = FD500KBPS;
	} else if( (dt == DRV_3e && fmt == FMT_3E) ||
		  (dt == DRV_3E && fmt == FMT_3E) ) {
		f->fd_trnsfr = FD1MBPS;
	} else {
		f->fd_trnsfr = FD300KBPS;
	}

	ff = &fdparam[drv];
	ff->fdf_bps = f->fd_secsft - 7;
	ff->fdf_dtl = (char)0xFF;

	if (dt == DRV_3H && fmt == FMT_3N) {
		ff->fdf_fil = (char)0xE5;
	} else {
		ff->fdf_fil = (char)0xF6;
	}

	ff->fdf_den = DEN_MFM;
	ff->fdf_gpln = fdsectab[fmt].fd_gpln;
	ff->fdf_gplf = fdsectab[fmt].fd_gplf;
	ff->fdf_spt = f->fd_nsects;
	ff->fdf_nhd = f->fd_nsides;
	ff->fdf_ncyl = f->fd_ncyls;
	return(0);
}

/*
 * int
 * fdm_readid(unsigned char drv, unsigned char  head)
 *	Return byte 0: encorded byte per sector
 *		    1: recorded cylinder number
 *
 * Calling/Exit State:
 *	None.
 */
int
fdm_readid(unsigned char drv, unsigned char  head)
{
	int oldpri, rtn;
	struct buf *bp;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdm_readid");
#endif
	while (fdmtnbusy & (0x01 << drv))
		sleep((caddr_t)&fdmtn[drv], PRIBIO);
	fdmtnbusy |= (0x01 << drv);

	fdmtn[drv].fr_cmd[0] = DEN_MFM | READID; 
	fdmtn[drv].fr_cmd[1] = (head << 2) | drv;
	fdmtn[drv].fr_cnum = 2;

	bp = fdmtnbuf[drv];	
	bioreset(bp);
	bp->b_flags &= ~B_READ;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	bp->b_fdcmd = MTNCMD;
	bp->b_edev = drv;

	fdxfer(bp);
	oldpri = splhi();
	biowait(bp);
	splx(oldpri);
	if (bp->b_resid == FE_ITOUT)
		rtn = FE_ITOUT;
	else if (bp->b_resid || (fdmtn[drv].fr_result[0] & 0xC0))
		rtn = FE_READID;	
	else
		rtn = (fdmtn[drv].fr_result[3] << 8) | fdmtn[drv].fr_result[6];

	fdmtnbusy &= ~(0x01 << drv);
	wakeup((caddr_t)&fdmtn[drv]);
	return(rtn);
}

/*
 * int
 * fdm_snsdskchg(unsigned char drv)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdm_snsdskchg(unsigned char drv)
{
	int oldpri, rtn;
	struct buf *bp;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdm_snsdskchg");
#endif
	while (fdmtnbusy & (0x01 << drv))
		sleep((caddr_t)&fdmtn[drv], PRIBIO);
	fdmtnbusy |= (0x01 << drv);

	fdmtn[drv].fr_cmd[0] = SENSE_DSKCHG; 
	fdmtn[drv].fr_cmd[1] = drv;
	fdmtn[drv].fr_cnum = 2;

	bp = fdmtnbuf[drv];	
	bioreset(bp);
	bp->b_flags &= ~B_READ;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	bp->b_fdcmd = MTNCMD;
	bp->b_edev = drv;

	fdxfer(bp);
	oldpri = splhi();
	biowait(bp);
	splx(oldpri);

	rtn = fdmtn[drv].fr_result[0];

	fdmtnbusy &= ~(0x01 << drv);
	wakeup((caddr_t)&fdmtn[drv]);
	return(rtn);
}

/*
 * int
 * fdm_read(char drv, char head, char cyl, char sect, char nsects,
 *	char bps, char eot, char gpl, char dtl)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdm_read(char drv, char head, char cyl, char sect, char nsects,
	char bps, char eot, char gpl, char dtl)
{
	int oldpri, rtn;
	struct buf *bp;
	caddr_t	va;

#ifdef DEBUG
	if (fddebug)
		/*
		 *+ debugging
		 */
		cmn_err(CE_NOTE,"fdm_read");
#endif
	while (fdmtnbusy & (0x01 << drv))
		sleep((caddr_t)&fdmtn[drv], PRIBIO);
	fdmtnbusy |= (0x01 << drv);

	fdmtn[drv].fr_cmd[0] = DEN_MFM | RDCMD; 
	fdmtn[drv].fr_cmd[1] = (head << 2) | drv;
	fdmtn[drv].fr_cmd[2] = cyl;
	fdmtn[drv].fr_cmd[3] = head;
	fdmtn[drv].fr_cmd[4] = sect;
	fdmtn[drv].fr_cmd[5] = bps;
	fdmtn[drv].fr_cmd[6] = eot;
	fdmtn[drv].fr_cmd[7] = gpl;
	fdmtn[drv].fr_cmd[8] = dtl;
	fdmtn[drv].fr_cnum = 9;

	bp = fdmtnbuf[drv];	
	bioreset(bp);
	bp->b_flags |= B_READ;
	bp->b_resid = 0;
	bp->b_bcount = nsects * (0x01 << (bps + 7));
	bp->b_fdcmd = MTNCMD;
	bp->b_edev = drv;

	va = kmem_alloc_physcontig(ptob(1), fdbcb->bcb_physreqp, KM_NOSLEEP);

	if (va == NULL) {
		/*
		 *+ Unable to get memory.
		 */
		cmn_err(CE_NOTE,"Unable to get kernel pages\n");
		fdmtnbusy &= ~(0x01 << drv);
		wakeup((caddr_t)&fdmtn[drv]);
		return(FE_READ);
	}
	bp->b_un.b_addr = va;

	fdxfer(bp);
	oldpri = splhi();
	biowait(bp);
	splx(oldpri);

	kmem_free_physcontig(va, ptob(1));

	if (bp->b_resid || (fdmtn[drv].fr_result[0] & 0xC0))
		rtn = FE_READ;	
	else
		rtn = 0;

	fdmtnbusy &= ~(0x01 << drv);
	wakeup((caddr_t)&fdmtn[drv]);
	return(rtn);
}

/*
 * int
 * fdm_dfmt(dev_t dev)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdm_dfmt(dev)
dev_t dev;
{
	struct fdstate *f;
	char dt, part, cyl;
	int rtn, curcyl;
	char drv;
	unsigned char pval;

        drv = UNIT(dev);
	f = &fd[drv];
	dt = f->fd_drvtype;
	part = PARTITION(dev);
	
	fdm_seek(drv, 0, 4);	/* Position the head so following */
	fdm_rezero(drv);	/* recalibration will be successful */
	cyl = 0;
	if (part == 2)
		cyl = 2;

	f->fd_fmt = FMT_UNKNOWN;
	switch (dt) {
	case DRV_5H:
		fdm_seek(drv, 0, cyl);
		f->fd_trnsfr = FD500KBPS;
		if (fdm_readid(drv, 0) != FE_READID) {
	 	    f->fd_fmt = FMT_5H;
		    break;
		}
		fdm_seek(drv, 0, 2);
		f->fd_trnsfr = FD300KBPS;
		if ((rtn = fdm_readid(drv,0)) != FE_READID) {
		    curcyl = (rtn >> 8) & 0xFF;
		    if (curcyl == 2) {
			f->fd_fmt = FMT_5Q;
			break;
		    }
		    if (curcyl == 1) {
			switch(rtn & 0xFF) {
			case FD256BPS:		/* 256 bytes per sector */
			    f->fd_fmt = FMT_5D16;
			    break;
			case FD512BPS:		/* 512 bytes per sector */
			    if (fdm_read(drv, 0, 1, 9, 1, 2, 9, 0x2A, 0xFF)
					== FE_READ) 
				f->fd_fmt = FMT_5D8;
			    else
				f->fd_fmt = FMT_5D9;
			    break;
			case FD1024BPS:		/* 1024 bytes per sector */
			    f->fd_fmt = FMT_5D4;
			    break;
			default:
			    break;
			}
		        break;
		    }
		}

		/**
		* Default to High Density 1.2Mb Media, we must provide a default
		* to handle those drive which fail the READID command even when
		* the proper controller/drive parameters are being used.
		**/
		f->fd_fmt = FMT_5H;

		break;

	case DRV_5D:
		fdm_seek(drv, 0, cyl);
		f->fd_trnsfr = FD250KBPS;
		if ((rtn = fdm_readid(drv,0)) != FE_READID &&
		    (((rtn >> 8) & 0xFF) == cyl))

		    switch(rtn & 0xFF) {
		    case FD256BPS:
			f->fd_fmt = FMT_5D16;
			break;
		    case FD512BPS:
			if (fdm_read(drv, 0, cyl, 9, 1, 2, 9, 0x2A, 0xFF)
			    == FE_READ) 
				f->fd_fmt = FMT_5D8;
			else
				f->fd_fmt = FMT_5D9;
			break;
		    case FD1024BPS:
			f->fd_fmt = FMT_5D4;
			break;
		    default:
			break;
		    }
		break;
	case DRV_3H:
		if (fd_eisa_brdid == 0xC255) {
			/**********************************************/
			/* Unisys 3-mode floppy support:              */
			/* Test for 1.2Mb Capacity @ 1024BPS & 360RPM */
			/* or 1.2Mb Capacity @ 512BPS & 360 RPM       */
			/**********************************************/
			pval = inb(0x028);
			outb(0x028, (pval | 0x10));	/** Select 360RPM **/
			fdm_seek(drv, 0, cyl);
			f->fd_trnsfr = FD500KBPS;
			if ((rtn = fdm_readid(drv, 0)) != FE_READID) {
				if ((rtn & 0xFF) == FD1024BPS) {
					f->fd_fmt = FMT_3N;
					break;
				}

				if ((rtn & 0xFF) == FD512BPS) {
					fdm_seek(drv, 0, cyl);
					if (fdm_read(drv, 0, 0, 18, 1, 2, 18, 0x1B, 0xFF) == FE_READ)  {
						fdm_seek(drv, 0, cyl);
						if (fdm_read(drv, 0, 0, 15, 1, 2, 15, 0x1b, 0xFF) != FE_READ)  {
							f->fd_fmt = FMT_3M;
							break;
						}
					}
				}
			}
			pval = inb( 0x028 );
			outb( 0x028, (pval & ~0x10) );	/** Select 300RPM **/
		}

		/*************************************/
		/* Test for 1.44Mb Capacity @ 512BPS */
		/*************************************/
		f->fd_trnsfr = FD500KBPS;
		if (fdm_readid(drv, 0) != FE_READID) {
			f->fd_fmt = FMT_3H;
		} else {
			/************************************/
			/* Test for 720Kb Capacity @ 512BPS */
			/************************************/
			f->fd_trnsfr = FD250KBPS;
			if (fdm_readid(drv, 0) != FE_READID) {
				f->fd_fmt = FMT_3D;
			} else {
				/****************************/
				/* Default to 1.44Mb 512BPS */
				/****************************/
				f->fd_trnsfr = FD500KBPS;
				f->fd_fmt = FMT_3H;
			}
		}

		break;

	case DRV_3e :	/** DELL 2.88Mb Drive Support **/
	case DRV_3E :	/** 2.88Mb Drive Support **/
		fdm_seek ( drv, 0, cyl );
		fd->fd_trnsfr = FD1MBPS;
		if (fdm_readid(drv, 0) != FE_READID) {
			f->fd_fmt = FMT_3E;
			break;
		}
		f->fd_trnsfr = FD500KBPS;
		if (fdm_readid(drv, 0) != FE_READID) {
			f->fd_fmt = FMT_3H;
			break;
		}
		f->fd_trnsfr = FD250KBPS;
		if (fdm_readid(drv, 0) != FE_READID) {
			f->fd_fmt = FMT_3D;
			break;
		}
	default:
		break;
	}
	f->fd_dskchg = 0;
	return(f->fd_fmt);
}
