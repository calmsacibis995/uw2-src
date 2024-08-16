/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/ddicheck/ddilint.c	1.3"
/* LINTLIBRARY */
/* PROTOLIB1 */

#ifdef _KERNEL_HEADERS
#include <util/types.h>
#else
#include <sys/types.h>
#endif

struct bcb;
struct proc;

typedef struct sv sv_t;
typedef struct proc proc_t;
typedef struct uio uio_t;
typedef struct lkinfo lkinfo_t;
typedef struct sleep sleep_t;
typedef struct rwlock rwlock_t;
typedef struct buf  buf_t;
typedef struct page page_t;
#ifndef DDI_SVR42
typedef struct lock lock_t;
#endif
typedef struct msgb mblk_t;
typedef struct queue queue_t;
typedef struct free_rtn frtn_t;
typedef struct cred cred_t;
typedef struct bcb bcb_t;
typedef struct physreq physreq_t;

#if defined(DDI_SVR42)
typedef int pl_t;
typedef int toid_t;
typedef unsigned int ppid_t;
typedef int processorid_t;
#endif
#ifdef DDI_UW20
typedef uint_t rm_key_t;
#endif

/* bldhdr depends upon typedefs being on one line */

typedef enum iob_request { IOB_ENABLE, IOB_DISABLE, IOB_CHECK } iob_request_t;
typedef enum qfields { QHIWAT, QLOWAT, QMAXPSZ, QMINPSZ, QCOUNT, QFIRST, QLAST, QFLAG, QBAD } qfields_t;
typedef enum uio_rw { UIO_READ, UIO_WRITE} uio_rw_t;

typedef boolean_t bool_t;

struct dma_buf;
struct dma_cb;
struct map;

pl_t plbase, pltimeout, pldisk, plstr, plhi;
int pridisk, prinet, pritty, pritape, prihi, primed, prilo;


/* Function Prototypes as Defined in D3 Draft Delta 43.11, 1/11/93 */

int 	adjmsg (mblk_t *, int);
mblk_t *allocb(int, uint_t);
int 	bcanput(queue_t *, uchar_t);
int 	bcanputnext(queue_t *, uchar_t);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
bcb_t *	bcb_alloc(int);
void	bcb_free(bcb_t *);
#endif
int	bcmp(caddr_t, caddr_t, size_t);
void	bcopy(caddr_t, caddr_t, size_t);
void 	biodone(buf_t *);
void	bioerror(buf_t *, int);
void 	bioreset(buf_t *);
int 	biowait(buf_t *);
#ifdef DDI_UW20
boolean_t biowait_sig(buf_t *);
#endif
void 	bp_mapin(buf_t *);
void 	bp_mapout(buf_t *);
void 	brelse(buf_t *);
ulong_t	btop(ulong_t);
ulong_t	btopr(ulong_t);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
void	buf_breakup(void (*)(), buf_t *, const bcb_t *);
#endif
toid_t 	bufcall(uint_t, int, void (*)(), long);
void	bzero(caddr_t, size_t);
int 	canput(queue_t *);
int 	canputnext(queue_t *);
void 	clrbuf(buf_t *);

#ifdef DDI_UW20

	
	/* New configuration management routines */

struct cm_args;
typedef struct cm_args cm_args_t;

int cm_AT_putconf(rm_key_t, int, int, ulong, ulong, ulong, ulong, int, uint, int);
int cm_addval(cm_args_t *);
int cm_delval(cm_args_t *);
rm_key_t cm_getbrdkey(char *, int);
int cm_getnbrd(char *);
int cm_getval(cm_args_t *);
int cm_getversion(void);
int cm_intr_attach(rm_key_t, void (*)(), int *, void **);
void cm_intr_detach(void *);
int cm_read_devconfig(rm_key_t, off_t, void *, size_t);
int cm_write_devconfig(rm_key_t, off_t, void *, size_t);

#endif

/*PRINTFLIKE2*/
extern void cmn_err(int, const char *, ...);
mblk_t *copyb(mblk_t *);
int	copyin(caddr_t, caddr_t, size_t);
mblk_t *copymsg(mblk_t *);
int	copyout(caddr_t, caddr_t, size_t);
int 	datamsg(uchar_t);
void	delay(long);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
boolean_t dma_cascade(int, uchar_t);
#endif
void 	dma_disable(int);
void 	dma_enable(int);
void 	dma_free_buf(struct dma_buf *);
void 	dma_free_cb(struct dma_cb *);
uchar_t dma_get_best_mode(struct dma_cb *);
struct dma_buf *	dma_get_buf(uchar_t);
struct dma_cb *		dma_get_cb(uchar_t);
#ifdef DDI_SVR42
void 	dma_pageio(void (*)(), buf_t *);
#endif
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
void	dma_physreq(int, int, physreq_t *);
#endif
int 	dma_prog(struct dma_cb *, int, uchar_t);
void 	dma_stop(int);
void 	dma_swstart(struct dma_cb *, int, uchar_t);
int 	dma_swsetup(struct dma_cb *, int, uchar_t);
#ifdef DDI_UW20
void	drv_callback(int, int (*)(), void *);
#endif
int	drv_gethardware(ulong_t, void *);
int	drv_getparm(ulong_t, void *);
clock_t	drv_hztousec(clock_t);
int	drv_priv(cred_t *);
int	drv_setparm(ulong_t, ulong_t);
clock_t	drv_usectohz(clock_t);
void	drv_usecwait(clock_t);
toid_t	dtimeout(void (*)(), void *, long, pl_t, processorid_t);
mblk_t *dupb(mblk_t *);
mblk_t *dupmsg(mblk_t *);
void 	enableok(queue_t *);
mblk_t *esballoc(uchar_t *, int, int, frtn_t *);
toid_t 	esbbcall(int, void (*)(), long);
int 	etoimajor(major_t);
void 	flushband(queue_t *, uchar_t, int);
void 	flushq(queue_t *, int);
void 	freeb(mblk_t *);
void 	freemsg(mblk_t *);
void 	freerbuf(buf_t *);
pl_t 	freezestr(queue_t *);
buf_t *	geteblk(void);
major_t	getemajor(dev_t);
minor_t	geteminor(dev_t);
int 	geterror(buf_t *);
major_t	getmajor(dev_t);
minor_t	getminor(dev_t);
page_t *getnextpg(buf_t *, page_t *);
mblk_t *getq(queue_t *);
buf_t *	getrbuf(long);
#ifdef DDI_SVR42
uint_t	hat_getkpfnum(caddr_t);
uint_t	hat_getppfnum(paddr_t, uint_t);
#endif
uchar_t	inb(int);
ulong_t	inl(int);
int	insq(queue_t *, mblk_t *, mblk_t *);
ushort_t inw(int);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
int 	iobitmapctl(iob_request_t, ushort_t []);
#endif
toid_t	itimeout(void (*)(), void *, long, pl_t);
int	itoemajor(major_t, int);
void *	kmem_alloc(size_t, int);
#ifdef DDI_SVR42MP
void *	kmem_alloc_physcontig(size_t, physreq_t *, int);
void 	kmem_free_physcontig(void *, size_t);
#endif
#ifdef DDI_UW20
void *	kmem_alloc_physreq(size_t, const physreq_t *, int);
#endif
void 	kmem_free(void *, size_t);
void *	kmem_zalloc(size_t, int);
ppid_t	kvtoppid(caddr_t);
void	linkb(mblk_t *, mblk_t *);
#if defined(DDI_SVR42MP) || defined(DDI_UW20) 
pl_t	LOCK(lock_t *, pl_t);
lock_t *LOCK_ALLOC(uchar_t, pl_t, lkinfo_t *, int);
void	LOCK_DEALLOC(lock_t *);
#endif
dev_t	makedevice(major_t, minor_t);
int 	max(int, int);
int	min(int, int);
void	mod_drvattach(void *);
void	mod_drvdetach(void *);
int	msgdsize(mblk_t *);
mblk_t *msgpullup(mblk_t *, int);
buf_t *	ngeteblk(size_t);
void	noenable(queue_t *);
queue_t *OTHERQ(queue_t *);
void	outb(int, uchar_t);
void	outl(int, ulong_t);
void	outw(int, ushort_t);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
void	ovbcopy(caddr_t, caddr_t, size_t);
#endif
int	pcmsg(uchar_t);
struct pollhead *phalloc(int);
void	phfree(struct pollhead *);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
void	physcontig_breakup(void (*)(), buf_t *, paddr_t, size_t);
#endif
int	physiock(void (*)(), buf_t *, dev_t, int, daddr_t, uio_t *);
addr_t	physmap(paddr_t, ulong_t, uint_t);
void	physmap_free(addr_t, ulong_t, uint_t);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
physreq_t *physreq_alloc(int);
void	physreq_free(physreq_t *);
boolean_t physreq_prep(physreq_t *, int);
#endif
ppid_t	phystoppid(paddr_t);
void	pollwakeup(struct pollhead *, short);
paddr_t	pptophys(page_t *);
void *	proc_ref(void);
int	proc_signal(void *, int);
void	proc_unref(void *);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
boolean_t proc_valid(void *);
#endif
void	psignal(proc_t *, int);
ulong_t	ptob(ulong_t);
int	pullupmsg(mblk_t *, int);
void	put(queue_t *, mblk_t *);
int	putbq(queue_t *, mblk_t *);
int	putctl(queue_t *, int);
int	putctl1(queue_t *, int, int);
int 	putnext(queue_t *, mblk_t *);
int	putnextctl(queue_t *, int);
int	putnextctl1(queue_t *, int, int);
int 	putq(queue_t *, mblk_t *);
void	qeneable(queue_t *);
void	qprocsoff(queue_t *);
void	qprocson(queue_t *);
void	qreply(queue_t *, mblk_t *);
int	qsize(queue_t *);
queue_t *RD(queue_t *);
#ifdef DDI_SVR42
void	rdma_filter(void (*)(), buf_t);
#endif
void	repinsb(int, uchar_t *, int);
void	repinsd(int, ulong_t *, int);
void	repinsw(int, ushort_t *, int);
void	repoutsb(int, uchar_t *, int);
void	repoutsd(int, ulong_t *, int);
void	repoutsw(int, ushort_t *, int);
ulong_t	rmalloc(struct map *, size_t);
struct map *rmallocmap(ulong_t);
ulong_t	rmalloc_wait(struct map *, size_t);
void	rmfree(struct map *, size_t, ulong_t);
void	rmfreemap(struct map *);
#ifdef DDI_SVR42
void	rminit(struct map *, unsigned long);
void	rmsetwant(struct map *);
#endif
mblk_t *rmvb(mblk_t *, mblk_t *);
void 	rmvq(queue_t *, mblk_t *);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
rwlock_t *RW_ALLOC(uchar_t, pl_t, lkinfo_t *, int);
void	RW_DEALLOC(rwlock_t *);
pl_t	RW_RDLOCK(rwlock_t *, pl_t);
pl_t	RW_TRYRDLOCK(rwlock_t *, pl_t);
pl_t	RW_TRYWRLOCK(rwlock_t *, pl_t);
void	RW_UNLOCK(rwlock_t *, pl_t);
pl_t	RW_WRLOCK(rwlock_t *, pl_t);
#endif
int	SAMESTR(queue_t *);
int	sleep(caddr_t, int);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
sleep_t *SLEEP_ALLOC(int, lkinfo_t *, int);
void	SLEEP_LOCK(sleep_t *, int);
bool_t	SLEEP_LOCKAVAIL(sleep_t *);
bool_t	SLEEP_LOCKOWNED(sleep_t *);
bool_t	SLEEP_LOCK_SIG(sleep_t *, int);
bool_t	SLEEP_TRYLOCK(sleep_t *);
void	SLEEP_UNLOCK(sleep_t *);
#endif
pl_t	splbase(void);
pl_t	spltimeout(void);
pl_t	spldisk(void);
pl_t	splstr(void);
pl_t	spltty(void);
pl_t	splhi(void);
pl_t	spl0(void);
pl_t	spl7(void);
pl_t	splx(pl_t);
char *	strcat(char *, const char *);
char *	strcpy(char *, const char *);
size_t 	strlen(const char *);
/*PRINTFLIKE5*/
int	strlog(short, short, char, ushort_t, char *, ...);
char *	strncat(char *, const char *, size_t);
int	strncmp(const char *, const char *, size_t);
char *	strncpy(char *, const char *, size_t);
int 	strqget(queue_t *, qfields_t, uchar_t, long *);
int 	strqset(queue_t *, qfields_t, uchar_t, long);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
sv_t *	SV_ALLOC(int);
void	SV_BROADCAST(sv_t *, int);
void	SV_DEALLOC(sv_t *);
void	SV_SIGNAL(sv_t *, int);
void	SV_WAIT(sv_t *, int, lock_t *);
bool_t	SV_WAIT_SIG(sv_t *, int, lock_t *);
pl_t	TRYLOCK(lock_t *, pl_t);
#endif
#ifndef DDI_UW20
int	timeout(void (*)(), caddr_t, long);
#endif
int	uiomove(caddr_t, long, uio_rw_t, uio_t *);
void	unbufcall(toid_t);
void	unfreezestr(queue_t *, pl_t);
mblk_t *unlinkb(mblk_t *);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
void	UNLOCK(lock_t *, pl_t);
#endif
void	untimeout(toid_t);
int	ureadc(int, uio_t *);
int	uwritec(uio_t *);
paddr_t vtop(caddr_t, proc_t *);
void	wakeup(caddr_t);
queue_t	*WR(queue_t *);

/* SDI Routines */

struct sdi_edt;
struct owner;
struct sb;
struct devcfg;
struct drv_majors;
struct head;
struct jpool;
struct scsi_ad;
struct scsi_adr;
struct sense;
struct hba_info;

#ifdef DDI_UW20
#define HBA_IDATA_STRUCT struct hba_idata_v4
#else
#define HBA_IDATA_STRUCT struct hba_idata
#endif
HBA_IDATA_STRUCT;

int	sdi_access(struct sdi_edt *, int, struct owner *);
#ifdef  DDI_UW20
struct sdi_event;
void sdi_acfree(HBA_IDATA_STRUCT *, int);
int sdi_addevent(struct sdi_event *);
int sdi_edtindex(int);
struct sdi_event *sdi_event_alloc(int);
void sdi_event_free(struct sdi_event *);
HBA_IDATA_STRUCT *sdi_hba_autoconf(char *, HBA_IDATA_STRUCT *, int *);
int sdi_hba_getconf(rm_key_t, HBA_IDATA_STRUCT *);
void sdi_intr_attach(HBA_IDATA_STRUCT *, int, void (*)(), int);
void sdi_mca_conf(HBA_IDATA_STRUCT *, int, int (*)());
int sdi_notifyevent(int, struct scsi_adr *, struct sb *);
int sdi_rmevent(struct sdi_event *);
struct sdi_edt *sdi_rxedt(int, int, int, int);
struct sense *sdi_sense_ptr(struct sb *);
void sdi_target_hotregister(int (*)(), struct owner *);
int sdi_timeout(void (*)(), void *, long, pl_t, struct scsi_ad *);
#endif

void	sdi_aen(int, int, int, int);
#ifndef DDI_UW20
void	sdi_blkio(buf_t *, unsigned int, void (*)());
#endif
void	sdi_callback(struct sb *);
void	sdi_clrconfig(struct owner *, int, void (*)());
struct owner *sdi_doconfig(struct devcfg[], int, char *, \
		struct drv_majors *, void (*)());
void	sdi_errmsg(char *, struct scsi_ad *, struct sb *, struct sense *, int, int);
struct dev_spec *sdi_findspec(struct sdi_edt *, struct dev_spec *[]);
void	sdi_free(struct head *, struct jpool *);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
void	sdi_freebcb(bcb_t *);
#endif
long	sdi_freeblk(struct sb *);
struct jpool *sdi_get(struct head *, int);
bcb_t *	sdi_getbcb(struct scsi_ad *, int);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
struct sb *sdi_getblk(int);
#else
struct sb *sdi_getblk(void);
#endif
void	sdi_getdev(struct scsi_ad *, dev_t *);
int	sdi_gethbano(int);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
int	sdi_icmd(struct sb *, int);
#else
int	sdi_icmd(struct sb *);
#endif

int	sdi_started;

void	sdi_init(void);
void	sdi_name(struct scsi_ad *, char *);
void	sdi_poolinit(struct head *);
struct sdi_edt *sdi_redt(int, int, int);
int	sdi_register(struct hba_info *, HBA_IDATA_STRUCT *);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
long	sdi_send(struct sb *, int);
#endif
short	sdi_swap16(unsigned int);
int	sdi_swap24(unsigned int);
long	sdi_swap32(unsigned long);
#if defined(DDI_SVR42MP) || defined(DDI_UW20)
int	sdi_translate(struct sb *, int, proc_t *, int);
#else
int	sdi_translate(struct sb *, int, proc_t *);
#endif
int	sdi_wedt(struct sdi_edt *, int, char *);
