/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cpio:i386/cmd/cpio/cpio.c	1.30.40.3"

/***************************************************************************
 * Command: cpio
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_DACWRITE,P_DACREAD, P_FSYSRANGE,
 *                         P_FILESYS,P_COMPAT,P_OWNER,P_MULTIDIR,P_SETPLEVEL,
 *			   P_SETFLEVEL
 *       Fixed Privileges: None
 *
 *
 * cpio: copy file archives in and out
 *
 *******************************************************************/ 

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <sys/secsys.h>
#include <sys/statvfs.h>
#include <sys/fs/vx_ioctl.h>
#include <utime.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <ctype.h>
#include <archives.h>
#include <locale.h>
#include "cpio.h"
#include <priv.h>
#include <acl.h>
#include <mac.h>
#include <pfmt.h>
#include <sys/param.h>
#include <libgen.h>
#include <termios.h>
#include <sys/mman.h>
#include "decompress.h"
#include <dlfcn.h>

#ifdef _REENTRANT
#include <thread.h>
#include <synch.h>
#endif

#define CPIO_PATH	"/usr/bin/cpio"
#define	NENTRIES	128	/* initial number of entries for ACL buf */
#define	cpioMAJOR(x)	(int)(((unsigned) x >> 8) & 0x7F)
#define	cpioMINOR(x)	(int)(x & 0xFF)
/* 
 * Ifile should not be closed if no file descriptor has been open (-1 value)
 * or if stdin (0).  Re-initialize to -1 to prevent wrong file descriptor
 * from been closed on later call.
 */
#define CLOSE_Ifile 	if (Ifile > 0 ) { (void)close(Ifile); Ifile=-1; } 

void (*mem_decompress)(char *, char *, unsigned long, unsigned long);

#ifdef __STDC__
void	msg(const int, const char *, const char *, ...);
#else
void	msg();
#endif

extern
int	atoi(),
	gmatch(),
	getopt(),
	g_init(),
	g_read(),
	g_write();

extern
char	*mktemp();

extern
long	lseek(),
	ulimit();

extern
mode_t	umask();

extern
void	exit(),
	free(),
	*malloc(),
	*memalign(),
	*realloc();

static
struct Lnk *add_lnk();

static
int	chg_lvl_proc(),
	chgreel(),
	ckname(),
	creat_hdr(),
	creat_lnk(),
	creat_mld(),
	creat_spec(),
	creat_tmp(),
	gethdr(),
	getname(),
	getpriv(),
	matched(),
	missdir(),
	nondigit(),
	openout(),
	non_threaded_abort(),
	read_hdr();


static
long	cksum(),
	mklong();

#ifdef _REENTRANT
static
void	*bfill_thread(),
	*bflush_thread();
#else /* _REENTRANT */
static
void	bfill(),
	bflush();
#endif /* _REENTRANT */

static
void	ckopts(),
	data_in(),
	data_out(),
	data_pass(),
	file_in(),
	file_out(),
	file_pass(),
	flush_lnks(),
	flush_tty(),
	open_tty(),
	getpats(),
	ioerror(),
	mkshort(),
	reclaim(),
	restore_lvl_proc(),
	rstfiles(),
	scan4trail(),
	setpasswd(),
	setup(),
	set_tym(),
	sigint(),
	bugout(),
	swap(),
	transferdac(),
	usage(),
	verbose(),
	write_hdr(),
	write_trail(),
	wrap(),
	/* Enhanced Application Compatibility Support     */
	truncate_sco();  /* needed for [-T] option */
	/* End Enhanced Application Compatibility Support */

static 
struct passwd	*Curpw_p,	/* Current password entry for -t option */
		*Rpw_p,		/* Password entry for -R option */
		*dpasswd;

static
struct group	*Curgr_p,	/* Current group entry for -t option */
		*dgroup;

/* Data structure for buffered I/O. */

static
struct buf_info {
	char	*b_base_p,	/* Pointer to base of buffer */
		*b_out_p,	/* Position to take bytes from buffer at */
		*b_in_p,	/* Position to put bytes into buffer at */
		*b_end_p;	/* Pointer to end of buffer */
	long	b_cnt,		/* Count of unprocessed bytes */
		b_size;		/* Size of buffer in bytes */
#ifdef _REENTRANT
	mutex_t	b_mutex;	/* Mutual Exclusion Lock for buffer */
	cond_t	b_take,		/* ok to take data from buffer? */
		b_put,		/* ok to put data in buffer? */
		b_endmedia;	/* end of media reached */
	int	b_waitfortake,	/* a thread is waiting on b_take */
		b_waitforput;	/* a thread is waiting on b_put */
#endif
} Buffr;

#ifdef _REENTRANT
mutex_t Sig_mutex;        /* Mutual Exclusion Lock for Finished */
#endif


static struct cpioinfo TmpSt;

/* Generic header format */

static
struct gen_hdr {
	ulong	g_magic,	/* Magic number field */
		g_ino;		/* Inode number of file */
	mode_t	g_mode;		/* Mode of file */
	uid_t	g_uid;		/* Uid of file */
	gid_t	g_gid;		/* Gid of file */
	ulong	g_nlink;	/* Number of links */
	time_t	g_mtime;	/* Modification time */
	long	g_filesz;	/* Length of file */
	ulong	g_dev,		/* File system of file */
		g_rdev,		/* Major/minor numbers of special files */
		g_namesz,	/* Length of filename */
		g_cksum;	/* Checksum of file */
	char	g_gname[32],
		g_uname[32],
		g_version[2],
		g_tmagic[6],
		g_typeflag;
	char	*g_tname,
		*g_prefix,
		*g_nam_p;	/* Filename */
} Gen, *G_p;

/* Data structure for handling multiply-linked files */
static
char	prebuf[PRESIZ+1],
	nambuf[NAMSIZ+1],
	fullnam[MAXNAM];


static
struct Lnk {
	short	L_cnt,		/* Number of links encountered */
		L_data;		/* Data has been encountered if 1 */
	struct gen_hdr	L_gen;	/* gen_hdr information for this file */
	struct Lnk	*L_nxt_p,	/* Next file in list */
			*L_bck_p,	/* Previous file in list */
			*L_lnk_p;	/* Next link for this file */
} Lnk_hd;

static
struct hdr_cpio	Hdr;

static
struct stat	ArchSt,	/* stat(2) information of the archive */
		SrcSt,	/* stat(2) information of source file */
		DesSt;	/* stat(2) of destination file */

/*
 * bin_mag: Used to validate a binary magic number,
 * by combining two bytes into an unsigned short.
 */

static
union bin_mag{
	unsigned char b_byte[2];
	ushort b_half;
} Binmag;

static
union tblock *Thdr_p;	/* TAR header pointer */

/*
 * swpbuf: Used in swap() to swap bytes within a halfword,
 * halfwords within a word, or to reverse the order of the 
 * bytes within a word.  Also used in mklong() and mkshort().
 */

static
union swpbuf {
	unsigned char	s_byte[4];
	ushort	s_half[2];
	ulong	s_word;
} *Swp_p;

static
char	Adir,			/* Flags object as a directory */
	Aspec,			/* Flags object as a special file */
	Time[50],		/* Array to hold date and time */
	*Ttyname = "/dev/tty",  /* Default terminal interface device */
	T_lname[NAMSIZ+1],	/* Array to hold links name for tar */
	*Buf_p,			/* Buffer for file system I/O */
	*Full_p,		/* Pointer to full pathname */
	*Effdir_p,		/* Pointer to MLD effective dir pathname */
	*Mldtmp_p,		/* tmp ptr while using MLD Effdir_p ptr */
	*Targetmld_p,		/* Pointer to Target MLD directory path name */
	*Efil_p,		/* -E pattern file string */
	*Eom_p = ":32:Change to part %d and press RETURN key. [q] ",
	*Fullnam_p,		/* Full pathname */
	*Hdr_p,			/* -H header type string */
	*Lnkend_p,		/* Pointer to end of Lnknam_p */
	*Lnknam_p,		/* Buffer for linking files with -p option */
	*Nam_p,			/* Array to hold filename */
	*Own_p,			/* New owner login id string */
	*Renam_p,		/* Buffer for renaming files */
	*Symlnk_p,		/* Buffer for holding symbolic link name */
	*Over_p,		/* Holds temporary filename when overwriting */
	**Pat_pp = 0;		/* Pattern strings */

char	*IOfil_p;		/* -I/-O input/output archive string */

static
int	Append = 0,	/* Flag set while searching to end of archive */
	Archive,	/* File descriptor of the archive */
	Bufsize = BUFSZ,	/* Default block size */
	Device,		/* Device type being accessed (used with libgenIO) */
	Dir_mode,	/* Mode of missing directories that we must create */
	Error_cnt = 0,	/* Cumulative count of I/O errors */
	Exists_flag,	/* Flag for if the directory already exists */
	Finished = 1,	/* Indicates that a file transfer has completed */
	Init_thr_id = -1, /* thr_id of initial thread */
	Hdrsz = ASCSZ,	/* Fixed length portion of the header */
	Hdr_type,		/* Flag to indicate type of header selected */
	Ifile = -1,		/* File des. of file being archived */
	Ofile,		/* File des. of file being extracted from archive */
	Oldstat = 0,	/* Create an old style -c hdr (small dev's)	*/
	Onecopy = 0,	/* Flags old vs. new link handling */
      	Orig_umask,	/* Original umask of process */
	Pad_val = 0,	/* Indicates the number of bytes to pad (if any) */
	Target_mld = 0,	/* Flag for if target dir is an mld */
	Mldmode_chg = 0, /* Flag for change of MLD mode */
	Level_chg = 0,	/* Flag indicating if we've changed levels */
	Volcnt = 1,	/* Number of archive volumes processed */
	Verbcnt = 0,	/* Count of number of dots '.' output */
	Vflag = 0,	/* Verbose flag (-V) */
	vflag = 0,	/* Verbose flag (-v) */
	Eomflag = 0,
	Dflag = 0,	/* -D, used by packaging */
	Zflag = 0,	/* Decompress flag (-Z) (undocumented) */
	Gotsigint = 0;	/* Received SIGINT signal */

static
mode_t	Def_mode = 0777;	/* Default file/directory protection modes */

static
gid_t	Lastgid = -1;	/* Used with -t & -v to record current gid */

static
uid_t	Lastuid = -1;	/* Used with -t & -v to record current uid */

static
int	extent_op = OCe_WARN;	/* Used with -e as type of propagation */

struct	vx_extcpio {
	int	magic;
	struct	vx_ext extinfo;
} vx_extcpio;

static
long	Args,		/* Mask of selected options */
	Blocks,		/* Number of full blocks transferred */
	Max_filesz,	/* Maximum file size from ulimit(2) */
	Max_namesz = APATH,	/* Maximum size of pathnames/filenames */
	SBlocks,	/* Cumulative char count for short reads */
#ifdef _REENTRANT
	Bufcnt_update = 0,	/* Amount to update buffer cnts, when we have the mutex */
#endif /* _REENTRANT */
	Pagesize;	/* Machine page size */

static
FILE	*Ef_p,			/* File pointer of pattern input file */
	*Err_p = stderr,	/* File pointer for error reporting */
	*Out_p = stdout,	/* File pointer for non-archive output */
	*Rtty_p,		/* Input file pointer for interactive rename, EOM interaction */
	*Wtty_p;		/* Output file pointer for interactive rename, EOM interaction */

static
ushort	Ftype = S_IFMT;		/* File type mask */

static
uid_t	Uid,			/* Uid of invoker */
	priv_id;		/* Super-user id if there is one, else -1 */

static int lpm = 1,	 	/* default file base mechanism installed */
	   privileged,		/* flag indicating if process is privileged */
	   aclpkg = 1,	 	/* ACL security package toggle */
	   macpkg = 1,	 	/* MAC security package toggle */
	   mldpriv = 1;  	/* indicate whether the invoking user has the
		            	   P_MULTIDIR privilege to create an MLD */

static lid_t Proc_lvl = 0, 	/* level of process */
	     File_lvl,		/* level of file */
	     Eff_lvl;		/* level of MLD effective directory */

/* Enhanced Application Compatibility Support */
#define MAX_NAMELEN	14	/* max length of file name (-T option) */
static unsigned Mediasize = 0; 	/* default: ignore media size; (-K option) */	
static unsigned Mediaused = 0; 	/* accumulated media used, Kbytes */
/* End Enhanced Application Compatibility Support */

static const char
	mutex[] = "-%c and -%c are mutually exclusive.",
	mutexid[] = ":33",
	badaccess[] = "Cannot access \"%s\"",
	badaccessid[] = ":34",
	badaccarch[] = "Cannot access the archive",
	badaccarchid[] = ":35",
	badcreate[] = "Cannot create \"%s\"",
	badcreateid[] = ":36",
	badcreatdir[] = "Cannot create directory for \"%s\"",
	badcreatdirid[] = ":37",
	badfollow[] = "Cannot follow \"%s\"",
	badfollowid[] = ":38",
	badread[] = "Read error in \"%s\"",
	badreadid[] = ":39",
	badreadsym[] = "Cannot read symbolic link \"%s\"",
	badreadsymid[] = ":40",
	badreadtty[] = "Cannot read \"%s\"",
	badreadttyid[] = ":1097",
	badreminc[] = "Cannot remove incomplete \"%s\"",
	badremincid[] = ":42",
	namclashdir[] = "Name clash; directory \"%s\" cannot be removed because it is not empty.  It has been renamed to \"%s\"",
	namclashdirid[] = ":1214",
	badremtmp[] = "Cannot remove temp file \"%s\"",
	badremtmpid[] = ":43",
	badremincdir[] = "Cannot remove incomplete \"%s\"; directory not empty",
	badremincdirid[] = ":1100",
	badremtmpdir[] = "Cannot remove temp directory \"%s\"; directory not empty",
	badremtmpdirid[] = ":1101",
	badwrite[] = "Write error in \"%s\"",
	badwriteid[] = ":44",
	badinit[] = "Error during initialization",
	badinitid[] = ":45",
	sameage[] = "Existing \"%s\" same age or newer",
	sameageid[] = ":46",
	badcase[] = "Impossible case.",
	badcaseid[] = ":47",
	badhdr[] = "Impossible header type.",
	badhdrid[] = ":48",
	nomem[] = "Out of memory",
	nomemid[] = ":49",
	badappend[] = "Cannot append to this archive",
	badappendid[] = ":50",
	badchmod[] = "chmod() failed on \"%s\"",
	badchmodid[] = ":51",
	badchown[] = "chown() failed on \"%s\"",
	badchownid[] = ":52",
	badpasswd[] = "Cannot get passwd information for %s",
	badpasswdid[] = ":53",
	badgroup[] = "Cannot get group information for %s",
	badgroupid[] = ":54",
	badorig[] = "Cannot recover original \"%s\"",
	badorigid[] = ":55",
	unformat[] = "Medium is unformatted",
	unformatid[] = ":1272";

/*
 *Procedure:     main
 *
 * Restrictions:
 *               acl(2):		none
 *               lvlproc(2):	none
 *               setlocale:	none
 */

/*
 * main: Call setup() to process options and perform initializations,
 * and then select either copy in (-i), copy out (-o), or pass (-p) action.
 */

main(argc, argv)
char **argv;
int argc;
{
	int gotname = 0, doarch = 0;
	lid_t tmp_lid;

	/* Determine whether id base or file base mechanism is installed. */
	if ((priv_id = secsys(ES_PRVID, 0)) >= 0)
		lpm = 0;	/* id base mechanism is installed */

	/* Determine if process is privileged.  If so, *
	 * the privileged flag will be set.            */
	Uid=getuid();
	if ((privileged = getpriv()) == -1)
		exit(EXIT_FATAL);

	/* Save original umask of process. */
	Orig_umask = (int)umask(0);
	(void) umask(Orig_umask);

	/* determine whether the ACL security package is installed */
	if ((acl("/", ACL_CNT, 0, 0) == -1) && (errno == ENOPKG))
		aclpkg = 0;

	/* determine whether the MAC security package is installed */
	if ((lvlproc(MAC_GET, &tmp_lid) != 0) && (errno == ENOPKG))
		macpkg = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:cpio");

	setup(argc, argv);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, sigint);
	if (Zflag) {
		void *handle;

		handle = dlopen("/usr/sadm/install/bin/libdecomp.so",RTLD_NOW);
		if (handle == NULL) {
			msg(ER,":1217", "Cannot open libdecomp.so");
			msg(EXT, ":897", "%s\n", dlerror());
		}
		mem_decompress = (void (*)())dlsym(handle,"mem_decompress");
		if (mem_decompress == NULL) {
			msg(ER,":1216", "Cannot find symbol mem_decompress in libdecomp.so");
			msg(EXT, ":897", "%s\n", dlerror());
		}
		if (spawn_reader() < 0)
			msg(EXTN,":1027", "Cannot fork\n");
	}
	switch (Args & (OCi | OCo | OCp)) {
	case OCi: /* COPY IN */
		/*
		 * If no -k, cpio detects header type. If -k 
		 * but no -c or -H, cpio detects header type.
		 */
		if ((!(Args & OCk)) || ((Args & OCk) && (!(Args & OCc) && !(Args & OCH))))
			Hdr_type = NONE;

#ifdef _REENTRANT
		if (!Dflag) {
			switch (thr_create(NULL, 0, bfill_thread, NULL, THR_INCR_CONC, NULL)) {
			case 0:
				break;
			case ENOMEM:
				msg(EXTN, nomemid, nomem);
				break;
			case EAGAIN:
			default:
				msg(EXTN, badinitid, badinit);
				break;
			}
		}
#endif /* _REENTRANT */

		while (gethdr()) {
			/* Enhanced Application Compatibility Support */
			if ((Args & OCT) && (Gen.g_namesz > MAX_NAMELEN+1)) 
				truncate_sco(Gen.g_nam_p);
			/* End Enhanced Application Compatibility Support */
			file_in();
		}
		/* Do not count "extra" "read-ahead" buffered data */
#ifdef _REENTRANT
		Buffr.b_cnt -= Bufcnt_update;
#endif /* _REENTRANT */
		if (Buffr.b_cnt > Bufsize)
			Blocks -= (Buffr.b_cnt / Bufsize);
		break;
	case OCo: /* COPY OUT */
		if (Args & OCA)
			scan4trail();

#ifdef _REENTRANT
		/*
		 * If we're in append mode (OCA flag set in Args), there's no need to use
		 * THR_INCR_CONC flag, since the concurrency was already incremented when
		 * scan4trail() created its bfill_thread().  The bfill_thread() exits
		 * before scan4trail() returns, leaving behind an unused lwp which can
		 * pick up this bflush_thread().
		 */
		switch (thr_create(NULL, 0, bflush_thread, NULL, (Args & OCA) ? 0 : THR_INCR_CONC, NULL)) {
		case 0:
			break;
		case ENOMEM:
			msg(EXTN, nomemid, nomem);
			break;
		case EAGAIN:
		default:
			msg(EXTN, badinitid, badinit);
			break;
		}
#endif /* _REENTRANT */

		while ((gotname = getname()) != 0) {
			if (gotname == 1) {
				file_out();
				doarch++;
			}
		}
		if (doarch > 0)
			write_trail();
		break;
	case OCp: /* PASS */
		while ((gotname = getname()) != 0)
			if (gotname == 1)
				file_pass();
		break;
	default:
		msg(EXT, ":56", "Impossible action.");
	}
	Blocks = ((double)Blocks * Bufsize + SBlocks + 0x1FF) / 512;
	msg(EPOST, ":57", "%lu blocks", Blocks);
	if (!Dflag)
		/*
		 * Protect access to Error_cnt with mutex.  (bfill_thread() could
		 * still be active -- reading ahead -- so might access
		 * Error_cnt.)  We don't release mutex afterwards, because
		 * we're exiting, and the exit call references Error_cnt, so
		 * needs to be protected.
		 */
		MUTEX_LOCK(&Buffr.b_mutex);

	if (Error_cnt == 1)
		msg(EPOST, ":58", "1 error");
	else if (Error_cnt > 1)
		msg(EPOST, ":771", "%d errors", Error_cnt);
	exit(Error_cnt ? EXIT_FILE : EXIT_OK);
}


/*
 * Procedure:	  getpriv
 *
 * Restrictions:  filepriv: 	none
 *		  procpriv:	none
 *
 */

/*
 * Find out if the process is privileged (return 1) or
 * not (return 0).  If the process' working privilege set
 * is at least what the privileges on the cpio binary are,
 * then the process is privileged.  Special case:  if the
 * privileges are equal, but both 0, this is NOT privileged.
 */

static
int 
getpriv()
{
	priv_t procmask[NPRIVS];	/* process' max priv mask */
	priv_t filemask[NPRIVS];	/* file's priv mask */
	priv_t procbuff[NPRIVS * 2];	/* process' max & working privs */
	priv_t filebuff[NPRIVS];	/* file's fixed & inheritable privs */
	int i, j;
	int procprivs;			/* # of process privs (max & working) */
	int fileprivs;			/* # of file privs for cpio binary */
	int workprivs = 0;		/* # of process privs in working set only */

	/* Check if lpm is installed.  If not, check *
	 * if Uid is privileged.		     */		

	if (!lpm) {
		if (Uid == priv_id)
			return(1);
	}

	/* Check the process' privileges against cpio's privileges *
	 * to determine if the process is "privileged".  	   */

	/* Initialize procmask and filemask to all 0's. */

	for (i = 0; i < NPRIVS; i++) {
		procmask[i] = 0;
		filemask[i] = 0;
	}
		
	/* Determine what privileges are associated with *
	 * process.  This includes both max and working. */

	if ((procprivs = procpriv(GETPRV, procbuff, NPRIVS * 2)) == -1) {
		(void)pfmt(stderr, MM_ERROR, 
			":825:Cannot retrieve process privileges");
		return(-1);
	}

	/* Set corresponding procmask bits when we find *
	 * what working privileges are set.             */

	for (i = 0; i < procprivs; i++) {
		for (j = 0; j < NPRIVS; j++) {
			if (procbuff[i] == pm_work(j)) {
				procmask[j] = 1;
				workprivs++;
				break;
			}
		}
	}

	/* If lpm is not installed (and we already know that  *
	 * Uid is not privileged), check if the number of     *
	 * privileges in the process' working set is equal to *
	 * the total number of privileges.  If not, the       *
	 * process is not "privileged".			      */

	if (!lpm) {
		if (workprivs == NPRIVS)
			return(1);
		else
			return(0);
	}

	/* Determine what privileges are associated with *
	 * cpio.  This includes inheritable and fixed.   */

	if ((fileprivs = filepriv(CPIO_PATH, GETPRV, filebuff, NPRIVS)) == -1) {
		(void)pfmt(stderr, MM_ERROR, 
			":890:Cannot retrieve file privileges");
		return(-1);
	}

	/* If the number of working privileges and the number of file *
	 * privileges are both 0, we can stop here; the process       *
	 * is not privileged.					      */
	if (workprivs == 0 && fileprivs == 0)
		return(0);

	/* Set corresponding filemask bits when we find   *
	 * what fixed and inheritable privileges are set. */

	for (i = 0; i < fileprivs; i++) {
		for (j = 0; j < NPRIVS; j++) {
			if ((filebuff[i] == pm_fixed(j)) || 
			    (filebuff[i] == pm_inher(j))) {
				filemask[j] = 1;
				break;
			}
		}
	}

	/* Determine if everything that is set in the filemask *
	 * is set in the procmask (i.e. if the process has at  *
	 * least the privileges that cpio has).		       */

	for (i = 0; i < NPRIVS; i++) {
		if (procmask[i] != filemask[i]) {
			if (filemask[i] == 1) {
				/* This means that cpio has a privilege *
				 * that process does not, so process is *
				 * not "privileged."                    */
				return(0);
			}
		}
	}

	/* If it gets here, then process has at least *
	 * the privileges that cpio has, so process   *
	 * is "privileged".			      */
			
	return(1);
}


/*
 * Procedure:     add_lnk
 *
 * Restrictions:  none
 */

/*
 * add_lnk: Add a linked file's header to the linked file data structure.
 * Either adding it to the end of an existing sub-list or starting
 * a new sub-list.  Each sub-list saves the links to a given file.
 */

static
struct Lnk *
add_lnk(l_p)
register struct Lnk **l_p;
{
	register struct Lnk *t1l_p, *t2l_p, *t3l_p;

	t2l_p = Lnk_hd.L_nxt_p;
	while (t2l_p != &Lnk_hd) {
		if (t2l_p->L_gen.g_ino == G_p->g_ino && t2l_p->L_gen.g_dev == G_p->g_dev)
			break; /* found */
		t2l_p = t2l_p->L_nxt_p;
	}
	if (t2l_p == &Lnk_hd)
		t2l_p = (struct Lnk *)NULL;
	t1l_p = (struct Lnk *)malloc(sizeof(struct Lnk));
	if (t1l_p == (struct Lnk *)NULL)
		msg(EXT, nomemid, nomem);
	t1l_p->L_lnk_p = (struct Lnk *)NULL;
	t1l_p->L_gen = *G_p; /* structure copy */
	t1l_p->L_gen.g_nam_p = (char *)malloc((unsigned int)G_p->g_namesz);
	if (t1l_p->L_gen.g_nam_p == (char *)NULL)
		msg(EXT, nomemid, nomem);
	(void)strcpy(t1l_p->L_gen.g_nam_p, G_p->g_nam_p);
	if (t2l_p == (struct Lnk *)NULL) { /* start new sub-list */
		t1l_p->L_nxt_p = &Lnk_hd;
		t1l_p->L_bck_p = Lnk_hd.L_bck_p;
		Lnk_hd.L_bck_p = t1l_p->L_bck_p->L_nxt_p = t1l_p;
		t1l_p->L_lnk_p = (struct Lnk *)NULL;
		t1l_p->L_cnt = 1;
		t1l_p->L_data = Onecopy ? 0 : 1;
		t2l_p = t1l_p;
	} else { /* add to existing sub-list */
		t2l_p->L_cnt++;
		t3l_p = t2l_p;
		while (t3l_p->L_lnk_p != (struct Lnk *)NULL) {
			t3l_p->L_gen.g_filesz = G_p->g_filesz;
			t3l_p = t3l_p->L_lnk_p;
		}
		t3l_p->L_gen.g_filesz = G_p->g_filesz;
		t3l_p->L_lnk_p = t1l_p;
	}
	*l_p = t2l_p;
	return(t1l_p);
}

#ifdef _REENTRANT

/*
 * The next two routines, bfill_thread and bflush_thread, are the startup
 * routines of new threads created by the initial thread.  bfill_thread() is
 * created for -i, bflush_thread() is created for -o.  (For -A,
 * bfill_thread() is created first to find the end of the archive, then it
 * exits and bflush_thread() is created to append to the archive.)
 *
 * These routines are run as separate threads to improve real time
 * performance.  The idea is that while bfill_thread (bflush_thread) is blocked
 * in the read (write) system call at one part of the buffer, the initial
 * thread can still be processing another part of the buffer.  Thus,
 * when bfill_thread (bflush_thread) returns from the read (write), the next
 * chunk of the buffer will be immediately ready for the next read (write).
 *
 * The buffer is circular: as soon as the thread gets to the end of
 * the buffer, it immediately wraps to the beginning.
 */


/*
 * COND_WAIT_LOOP:  This macro is used by bfill_thread to wait on the condition
 * variable cond_var until boolean becomes true.  The mutex must be held on
 * entry, and will remain held on return.
 *
 * The value of Append is also checked.  When cpio is invoked with the -A
 * option, it appends data to the end of an already existing archive.  It does
 * this by calling bfill_thread to read the archive, so it can search for the
 * archive trailer.  When the other thread finds the trailer, it sets Append to
 * 2, to let us know that we need to exit, so bflush_thread can take over to
 * write the new data.
 */

#define COND_WAIT_LOOP(boolean, cond_var) \
{ \
	for (;;) { \
		if (Append == 2) { \
			MUTEX_UNLOCK(&Buffr.b_mutex); \
			thr_exit(NULL); \
		} else if (boolean) \
			break; \
		(void)cond_wait(cond_var, &Buffr.b_mutex); \
	} \
}

/*
 * Procedure:     bfill_thread
 *
 * Restrictions:
 *                g_read	none
 */

/*
 * bfill_thread: This is the start routine of a thread created by main() for
 * -i or -A. It keeps reading data into the buffer until it reaches
 * end-of-file or end-of-media.
 *
 * When bfill_thread() finds it needs to wait for free space in the buffer,
 * it waits on the Buffr.b_put condition, which the initial thread
 * (via the DECR_BUFCNT macro) will signal when it has freed enough
 * space.  Every time bfill_thread() reads some data, it signals
 * the Buffr.b_take condition to alert the initial thread that there
 * is new data available to consume (the initial thread waits on the
 * Buffr.b_take condition in the WAIT_FOR_DATA macro, if necessary.)
 *
 * When end-of-file or end-of-media is reached, bfill_thread() sets Eomflag
 * and waits on the Buffr.b_endmedia condition.  If the initial thread
 * needs more data after this, it signals the Buffr.b_endmedia
 * condition which causes bfill_thread() to get a new medium (if the archive
 * is a device) or exit with an error (if the archive is a regular
 * file).
 */

static
void *
bfill_thread(void *dummy)
{
	int bytes_read, blocks_avail;
	int skipcnt = 0;
	int  read_size = Bufsize;

	for (;;) {
		MUTEX_LOCK(&Buffr.b_mutex);
		Buffr.b_waitforput = 1;
		COND_WAIT_LOOP(((blocks_avail = (Buffr.b_size - Buffr.b_cnt) / Bufsize) > 0),
			       &Buffr.b_put);
		Buffr.b_waitforput = 0;
		MUTEX_UNLOCK(&Buffr.b_mutex);

		for (; blocks_avail; blocks_avail--) {
			errno = 0;
			/*
			 * If -Z option used (Zflag set), get data from the
			 * decompression process via d_read().  Else, read
			 * straight from the archive via g_read().
			 */
			if ((bytes_read = Zflag ? d_read(Buffr.b_in_p,read_size) :
			     g_read(Device, Archive, Buffr.b_in_p, read_size)) < 0) {
				blocks_avail++;
				if (errno == ENOSPC) {
					/*
					 * End of medium.  If Buffr.b_waitfortake is set,
					 * then the other thread is waiting for that much
					 * data.  If there is not enough data in the
					 * buffer to fill this need, that means the
					 * end-of-archive was not on the current media,
					 * so we need to call chgreel.  If
					 * Buffr.b_waitfortake is <= the data in
					 * the buffer, we need to wait for the original
					 * thread to consume this data before we can know
					 * what to do next.  If the other thread finds
					 * end-of-archive, it will just exit(), causing
					 * both itself and us to die.  Else, it will
					 * signal Buffr.b_endmedia, which we wait for,
					 * and then we do the chgreel.
					 */
					MUTEX_LOCK(&Buffr.b_mutex);
					if (Buffr.b_waitfortake <= Buffr.b_cnt) {
						Eomflag = 1;
						COND_WAIT_LOOP((Eomflag != 1), &Buffr.b_endmedia);
					}
					MUTEX_UNLOCK(&Buffr.b_mutex);
					/*
					 * End of archive was not found on last medium.
					 * Ask for new one.
					 */
					bytes_read = chgreel(INPUT);
				} else if (Args & OCk) {
					if (errno == EIO && lseek(Archive, 0, SEEK_CUR) == 0)
						/*
						 * If the errno is EIO, and
						 * this is the first read/write
						 * of the file, chances are
						 * that this is an unformatted
						 * floppy.
						 */
						msg(EXT, unformatid, unformat);
					if (skipcnt++ > MX_SEEKS)
						msg(EXT, ":60", "Cannot recover.");
					if (lseek(Archive, Bufsize, SEEK_REL) < 0)
						msg(EXTN, ":61", "lseek() failed");
					/* Protect access to Error_cnt with mutex. */
					MUTEX_LOCK(&Buffr.b_mutex);
					Error_cnt++;
					MUTEX_UNLOCK(&Buffr.b_mutex);
					bytes_read = 0;
					continue;
				} else
					ioerror(INPUT);
			} /* (bytes_read = g_read(Device, Archive ... */

			MUTEX_LOCK(&Buffr.b_mutex);
			if ((Buffr.b_in_p += bytes_read) == Buffr.b_end_p) {
				/*
				 * We're at the end of the buffer, so we need to wrap
				 * around.  But if the other thread has its
				 * Buffr.b_out_p pointer in the scratch area at the end
				 * of the buffer (or newly wrapped to Buffr.b_base_p), we
				 * have to wait for it to complete its wrap, lest we
				 * sneak past it.
				 */
				Buffr.b_waitforput = 1;
				COND_WAIT_LOOP((Buffr.b_out_p < Buffr.b_end_p &&
						Buffr.b_out_p != Buffr.b_base_p), &Buffr.b_put);
				Buffr.b_waitforput = 0;
				Buffr.b_in_p = Buffr.b_base_p;
				read_size = Bufsize;
			} else if (Buffr.b_in_p + read_size > Buffr.b_end_p) {
				/* This can only(?) happen after we
				 * read a partial block at the end of one
				 * media, and are starting the next media with
				 * the rest of the partial block.  When this
				 * happens, we read the partial block.
				 */

				read_size = Buffr.b_end_p - Buffr.b_in_p;
			}
			Buffr.b_cnt += (long)bytes_read;

			/* alert other thread there's more data to consume */
			if (Buffr.b_waitfortake && Buffr.b_waitfortake <= Buffr.b_cnt)
				(void)cond_signal(&Buffr.b_take);

			MUTEX_UNLOCK(&Buffr.b_mutex);

			if (bytes_read == Bufsize)
				Blocks++;
			else if (!bytes_read) {
				/*
				 * g_read only returns 0 when we get to the end of a
				 * regular file or pipe.  (For end of media on a device
				 * special file, it returns -1 with errno == ENOSPC --
				 * see above).  If the original thread finds a trailer at
				 * the end of what we've read so far, all is well, it
				 * will just call exit() and we'll both die together.
				 * Else, it will signal the Buffr.b_endmedia condition
				 * hoping we can change media -- but since we're not
				 * reading from a device with removable media file, all
				 * we can do is exit with an error.  (If
				 * Buffr.b_waitfortake is greater then the amount of data
				 * in the buffer, we don't even need to wait, we already
				 * know that the other thread is waiting for more data
				 * that we can't provide.)
				 */
				MUTEX_LOCK(&Buffr.b_mutex);
				if (Buffr.b_waitfortake <= Buffr.b_cnt) {
					Eomflag = 1;
					COND_WAIT_LOOP((Eomflag != 1), &Buffr.b_endmedia);
				}
				MUTEX_UNLOCK(&Buffr.b_mutex);
				msg(EXT, ":124", "Unexpected end-of-archive encountered.");
			} else
				SBlocks += bytes_read;
		}
	}
}


/*
 * Procedure:     bflush_thread
 *
 * Restrictions:
 *                g_write: none
 */

/*
 * bflush_thread: This is the start routine of a thread created by main() for
 * -o or -A. It keeps writing data from the I/O buffer to the archive,
 * as long as there is data to write.
 *
 * When bflush_thread() finds it needs to wait for the buffer to be populated
 * with data, it waits on the Buffr.b_take condition, which the
 * initial thread (via the INCR_BUFCNT macro) will signal when it has
 * put enough data in the buffer.  Every time bflush_thread() writes some
 * data, it signals the Buffr.b_put condition to alert the initial
 * thread that there is more room available in the buffer to put data
 * (the initial thread waits on the Buffr.b_put condition in the
 * WAIT_FOR_SPACE macro, if necessary.)
 */

static
void *
bflush_thread(void *dummy)
{
	int bytes_written, blocks_avail;
	int write_size = Bufsize;
	int eof = 0;

	for (;;) {

		MUTEX_LOCK(&Buffr.b_mutex);
		while ((blocks_avail = Buffr.b_cnt / Bufsize) == 0) {
			Buffr.b_waitfortake = 1;
			(void)cond_wait(&Buffr.b_take, &Buffr.b_mutex);
		}
		Buffr.b_waitfortake = 0;
		MUTEX_UNLOCK(&Buffr.b_mutex);

		for (; blocks_avail; blocks_avail--) {

			errno = 0;

			/* Enhanced Application Compatibility Support		*/

			if ((Mediasize) && 
				(Mediaused + ((write_size + 0x3ff) >> 10) > Mediasize)) {
				if (vflag | Vflag) {
					/*
					 * Wait for the other thread to stop before we
					 * prompt for a new media.  Otherwise, the other
					 * thread might print verbose output after we
					 * have prompted.
					 *
					 * Spinning is not the most efficient way to
					 * handle this, but it's the simplest, and we
					 * need not be overly concerned with performance,
					 * since it shouldn't take the other thread too
					 * long to fill the buffer, and this won't happen
					 * all that often anyway.
					 */
					for (;;) {
						MUTEX_LOCK(&Buffr.b_mutex);
						if (Buffr.b_waitforput)
							break;
						MUTEX_UNLOCK(&Buffr.b_mutex);
					}
					MUTEX_UNLOCK(&Buffr.b_mutex);
				}
				bytes_written = chgreel(OUTPUT);
				Mediaused = 0;
				eof = 0;

			/* End Enhanced Application Compatibility Support   	*/

			} else if ((bytes_written = g_write(Device, Archive, Buffr.b_out_p, write_size)) < 0) {
				if (errno == ENOSPC && !Dflag){
					if (vflag | Vflag) {
						/*
						 * See comments above.
						 */
						for (;;) {
							MUTEX_LOCK(&Buffr.b_mutex);
							if (Buffr.b_waitforput)
								break;
							MUTEX_UNLOCK(&Buffr.b_mutex);
						}
						MUTEX_UNLOCK(&Buffr.b_mutex);
					}
					bytes_written = chgreel(OUTPUT);
					eof = 0;
				}
				else
					ioerror(OUTPUT);
			}
			else if (bytes_written == 0){
				/* If write returns 0, we are expecting that it ran
				 * out of space.  If so, the next write attempt will
				 * return -1 and ENOSPC.
				 */
				if (!eof) {
					eof = 1;
					continue;
				}
				/* If write returns 0 twice, something is wrong, maybe
			 	 * a floppy problem, so bail out now.
			  	 */
				ioerror(OUTPUT);
			}

			/* Enhanced Application Compatibility Support   	*/
			if (Mediasize)
				Mediaused += (bytes_written + 0x3ff) >> 10;  /* round up to 1K units */
			/* End Enhanced Application Compatibility Support   	*/

			if (bytes_written == Bufsize)
				Blocks++;
			else if (bytes_written > 0)
				SBlocks += bytes_written;

			MUTEX_LOCK(&Buffr.b_mutex);
			if ((Buffr.b_out_p += bytes_written) == Buffr.b_end_p) {
				/*
				 * We're at the end of the buffer, so we need to wrap
				 * around.  But if the other thread has its Buffr.b_in_p
				 * pointer in the scratch area at the end of the buffer,
				 * we have to wait for it to complete its wrap, lest we
				 * sneak past it.
				 */
				while (Buffr.b_in_p >= Buffr.b_end_p) {
					Buffr.b_waitfortake = 1;
					(void)cond_wait(&Buffr.b_take, &Buffr.b_mutex);
				}
				Buffr.b_waitfortake = 0;
				Buffr.b_out_p = Buffr.b_base_p;
				write_size = Bufsize;
			} else if (Buffr.b_out_p + Bufsize > Buffr.b_end_p) {
				/* This can only(?) happen after we write a
				 * partial block at the end of one media, and
				 * are starting the next media with the rest of
				 * the partial block.  When this happens, we
				 * write the partial block.
				 */

				write_size = Buffr.b_end_p - Buffr.b_out_p;
			}
			Buffr.b_cnt -= (long)bytes_written;

			/* alert other thread there's more space in buffer */
			if (Buffr.b_waitforput)
				(void)cond_signal(&Buffr.b_put);

			MUTEX_UNLOCK(&Buffr.b_mutex);
		}
	}
}

#else /* _REENTRANT */

/*
 * Procedure:     bflush
 *
 * Restrictions:
 *                g_write: none
 */

/*
 * bflush: Write the I/O buffer to the archive.
 */

static
void
bflush()
{
	register int bytes_written;
	register int eof = 0;
	static int write_size = 0;

	if (write_size == 0)
		write_size = Bufsize;
	while (Buffr.b_cnt >= Bufsize) {
		errno = 0;

		/* Enhanced Application Compatibility Support		*/

		if ((Mediasize) && 
			(Mediaused + (write_size >> 10)  > Mediasize)) {
			bytes_written = chgreel(OUTPUT);
			Mediaused = 0;
			eof = 0;

		/* End Enhanced Application Compatibility Support   	*/

		} else if ((bytes_written = g_write(Device, Archive, Buffr.b_out_p, write_size)) < 0) {
			if (errno == ENOSPC && !Dflag){
				bytes_written = chgreel(OUTPUT);
				eof = 0;
			}
			else
				ioerror(OUTPUT);
		}
		else if (bytes_written == 0){
			/* If write returns 0, we are expecting that it ran
			 * out of space.  If so, the next write attempt will
			 * return -1 and ENOSPC.
			 */
			if (!eof) {
				eof = 1;
				continue;
			}
			/* If write returns 0 twice, something is wrong, maybe
		 	 * a floppy problem, so bail out now.
		  	 */
			ioerror(OUTPUT);
		}

		/* Enhanced Application Compatibility Support   	*/
		if (Mediasize)
			Mediaused += (bytes_written + 0x3ff) >> 10;  /* round up to 1K units */
		/* End Enhanced Application Compatibility Support   	*/

		if ((Buffr.b_out_p += bytes_written) == Buffr.b_end_p) {
			/*
			 * We're at the end of the buffer, so we need to wrap
			 * around.  But before we do, we need to make sure that
			 * 
			 */
			Buffr.b_out_p = Buffr.b_base_p;
			write_size = Bufsize;
		} else if (Buffr.b_out_p + Bufsize > Buffr.b_end_p) {
			/* This can only(?) happen after we write a
			 * partial block at the end of one media, and
			 * are starting the next media with the rest of
			 * the partial block.  When this happens, we
			 * write the remaining partial block.
			 */

			write_size = Buffr.b_end_p - Buffr.b_out_p;
		}
		Buffr.b_cnt -= (long)bytes_written;
		if (bytes_written == Bufsize)
			Blocks++;
		else if (bytes_written > 0)
			SBlocks += bytes_written;
	}
}

#endif /* _REENTRANT */

/*
 * Procedure:     bfill
 *
 * Restrictions:
 *                g_read	none
 */

/*
 * bfill: Read from the archive into the I/O buffer.
 */

static
void
bfill()
{
	int bytes_read;
	int i = 0;
	static int eof = 0;
	static int read_size = 0;

	if (read_size == 0) {
		read_size = Bufsize;
	}
	do {
		errno = 0;
		/*
		 * If -Z option used (Zflag set), get data from the
		 * decompression process via d_read().  Else, read
		 * straight from the archive via g_read().
		 */
		if ((bytes_read = Zflag ? d_read(Buffr.b_in_p,read_size)
		     : g_read(Device, Archive, Buffr.b_in_p, read_size)) < 0){
			if (Dflag)
				msg(EXT, ":124", "Unexpected end-of-archive encountered.");
			if (errno == ENOSPC)
				if (((Buffr.b_end_p - Buffr.b_in_p) >= Bufsize) && (Eomflag == 0)) {
					Eomflag = 1;
					return;
				}
			if (Eomflag) {
				bytes_read = chgreel(INPUT);
			} else if (Args & OCk) {
				if (errno == EIO && lseek(Archive, 0, SEEK_CUR) == 0)
					/*
					 * If the errno is EIO, and this is the
					 * first read/write of the file,
					 * chances are that this is an
					 * unformatted floppy.
					 */
					msg(EXT, unformatid, unformat);
				if (i++ > MX_SEEKS)
					msg(EXT, ":60", "Cannot recover.");
				if (lseek(Archive, Bufsize, SEEK_REL) < 0)
					msg(EXTN, ":61", "lseek() failed");
				Error_cnt++;
				bytes_read = 0;
				continue;
			} else
				ioerror(INPUT);
		}
		if ((Buffr.b_in_p += bytes_read) == Buffr.b_end_p) {
			Buffr.b_in_p = Buffr.b_base_p;
			read_size = Bufsize;
		} else if (Buffr.b_in_p + read_size > Buffr.b_end_p) {
			read_size = Buffr.b_end_p - Buffr.b_in_p;
		}
		Buffr.b_cnt += (long)bytes_read;
		if (bytes_read == Bufsize)
			Blocks++;
		else if (!bytes_read) {
			if (!eof) {
				eof = 1;
				return;
			} else
				msg(EXT, ":124", "Unexpected end-of-archive encountered.");
		} else
			SBlocks += bytes_read;
	} while ((Buffr.b_size - Buffr.b_cnt) >= Bufsize && !Dflag);
}


static
void
wrap(int cnt)
{
	if ((Args & OCi) || Append == 1) {
		/*
		 * If we're near the end of the buffer, copy enough to satisfy our
		 * needs from the beginning of the buffer to the scratch area past the
		 * end of the buffer (this is the "else if" below).  If we are already
		 * past the end of the buffer, wrap around to the beginning, skipping
		 * over the stuff at the beginning that had been copied to the scratch
		 * area and consumed.
		 */
		if (Buffr.b_out_p >= Buffr.b_end_p)
			Buffr.b_out_p = Buffr.b_base_p + (Buffr.b_out_p - Buffr.b_end_p);
		else if ((Buffr.b_out_p + cnt) > Buffr.b_end_p)
			memcpy(Buffr.b_end_p, Buffr.b_base_p,
			       (cnt - (Buffr.b_end_p - Buffr.b_out_p)));
	} else { /* OCo */
		/*
		 * If we're past the end of the buffer (in the scratch area there),
		 * wrap around to the beginning: copy scratch area to the beginning and
		 * set pointer there.
		 */
		if (Buffr.b_in_p >= Buffr.b_end_p) {
			memcpy(Buffr.b_base_p, Buffr.b_end_p,
			       Buffr.b_in_p - Buffr.b_end_p);
			Buffr.b_in_p = Buffr.b_base_p +
				(Buffr.b_in_p - Buffr.b_end_p);
		}
	}
}

/*
 * Procedure:     chgreel
 *
 * Restrictions:
 *                fopen:	none
 *                fgets:	none
 *                open(2):	none
 *                g_init:	none
 *                g_write:	none
 */

/*
 * chgreel: Determine if end-of-medium has been reached.  If it has,
 * close the current medium and prompt the user for the next medium.
 */

static
int
chgreel(dir)
register int dir;
{
	register int lastchar, tryagain, bytes_xferred, oflag;
	int tmpdev;
	char str[APATH];

	if (Rtty_p == (FILE *)NULL) {
		/*
	 	* For -G, this should not happen.
		* Ttyname should have already been
	 	* opened in setup().
	 	*/
		open_tty();
	}


	if (dir) {
		(void)pfmt(Wtty_p, MM_NOSTD, ":1071:\007End of medium on output.\n");
		(void)fflush(Wtty_p);
	}
	else {
		(void)pfmt(Wtty_p, MM_NOSTD, ":1072:\007End of medium on input.\n");
		(void)fflush(Wtty_p);
	}
	(void)close(Archive);
	Volcnt++;
	for (;;) {
		do {  /* while tryagain */
			tryagain = 0;
			if (IOfil_p) {
				flush_tty(Wtty_p);
				if (Args & OCM) {
					/* If user specified message,
					 * must use MM_NOGET so it
					 * won't try to retrieve it 
					 * from the message catalog.
					 */
					(void)pfmt(Wtty_p, MM_NOGET|MM_NOSTD, Eom_p, Volcnt);
					(void)fflush(Wtty_p);
				}
				else {
					(void)pfmt(Wtty_p, MM_NOSTD, Eom_p, Volcnt);
					(void)fflush(Wtty_p);
				}
				if (fgets(str, sizeof(str), Rtty_p) == (char *)NULL)
					msg(EXT, badreadttyid, badreadtty);
				/*
				 * When doing input and output on a pseudo-tty
				 * (-G option), the file position must be reset
				 * after a read. 
				 */
				rewind(Wtty_p);

				switch (*str) {
				case '\n':
					(void)strcpy(str, IOfil_p);
					break;
				case 'q':
					/* Protect access to Error_cnt with mutex. */
					MUTEX_LOCK(&Buffr.b_mutex);
					exit(Error_cnt ? EXIT_FILE : EXIT_OK);
					MUTEX_UNLOCK(&Buffr.b_mutex);
				default:
					sleep(1);
					tryagain = 1;
					continue;
				}
			} else {
				flush_tty(Wtty_p);
				(void)pfmt(Wtty_p, MM_NOSTD, ":1070:To continue, type device/file name when ready.\n");
				(void)fflush(Wtty_p);
				if (fgets(str, sizeof(str), Rtty_p) == (char *)NULL)
					msg(EXT, badreadttyid, badreadtty, Ttyname);
				lastchar = strlen(str) - 1;
				if (*(str + lastchar) == '\n') /* remove '\n' */
					*(str + lastchar) = '\0';
				if (!*str) { /* user hit RETURN */
					tryagain = 1;
					continue;
				}
			}
			if (dir)
				oflag = (O_WRONLY | O_EXCL);
			else
				oflag = O_RDONLY;
			if ((Archive = open(str, oflag)) < 0) {
				msg(ERN, ":65", "Cannot open \"%s\"", str);
				sleep(1);
				tryagain = 1;
				/* If trying again, don't count error. */
				/* Protect access to Error_cnt with mutex. */
				MUTEX_LOCK(&Buffr.b_mutex);
				Error_cnt--;
				MUTEX_UNLOCK(&Buffr.b_mutex);
			}
		} while (tryagain);

		(void)g_init(&tmpdev, &Archive);
		if (tmpdev != Device)
			msg(EXT, ":66", "Cannot change media types in mid-stream.");
		if (dir == INPUT) {
			errno = 0;
			if ((bytes_xferred = g_read(Device, Archive, Buffr.b_in_p, Bufsize)) == Bufsize)
				break;
			else {
				if (errno == EIO)
					/*
					 * If the errno is EIO, and this is the
					 * first read of the file,
					 * chances are that this is an
					 * unformatted floppy.
					 */
					msg(ER, unformatid, unformat);

				msg(ER, ":939", "Read failure--check medium and try again");
				(void)close(Archive);
			}
		} else { /* dir == OUTPUT */
			errno = 0;
			if ((bytes_xferred = g_write(Device, Archive, Buffr.b_out_p, Bufsize)) == Bufsize)
				break;
			else {
				if (errno == EIO)
					/*
					 * If the errno is EIO, and this is the
					 * first write of the file,
					 * chances are that this is an
					 * unformatted floppy.
					 */
					msg(ER, unformatid, unformat);

				msg(ER, ":67", "Cannot write on this medium, try another.");
				(void)close(Archive);
			}
		}
	} /* ;; */
	Eomflag = 0;
	return(bytes_xferred);
}

/*
 * flush_tty: Flush tty input stream.  If user accidentally
 * bounced on <RETURN> key on responding to end-of-medium prompt,
 * this will throw out those extra characters. Otherwise,
 * those characters would be taken as the response to next
 * end-of-medium prompt, and not allow time to change medium.
 */
static
void
flush_tty(tty_p)
	FILE *tty_p;
{
	register int fd;

	fd = fileno(tty_p);
	if (isatty(fd)) {
		tcflush(fd, TCIFLUSH);
		fflush(tty_p);
	}
}

/*
 * open_tty: Open the "tty".  This is either "/dev/tty", or a file (typically a
 * pseudo terminal) specified with the -G option.  This "tty" is used when cpio
 * needs to interact with the user: when a change of media is necessary, or
 * when interactive renaming (-r option) is being done.
 *
 * When -r is specified, this function is called during setup, since the "tty"
 * will be needed from the onset.
 *
 * When -r is not specified, the "tty" may or may not be needed, depending on
 * whether an end-of-medium is reached.  However, if -G is specified, we take
 * this is a clue that the user expects EOM handling, and so we do the
 * open_tty() at the start, so any problems will be detected early.
 *
 * If neither -r nor -G is specified, the call to open_tty is postponed until
 * the "tty" is actually needed (in chgreel()).  This allows cpio to run
 * successfully (as long as there's no EOM) in situations where it does not
 * have a controling terminal.
 */
static
void
open_tty()
{
	Rtty_p = Wtty_p = fopen(Ttyname, "r+");
	if (Rtty_p != (FILE *)NULL)
		setbuf(Rtty_p, NULL);
	else {
		msg(ER, ":65", "Cannot open \"%s\"", Ttyname);
		exit(EXIT_FATAL);
	}
}

/*
 * Procedure:     ckname
 *
 * Restrictions:
 *                pfmt		none
 *                fflush	none
 *                fgets		none
 */

/*
 * ckname: Check filenames against user specified patterns,
 * and/or ask the user for new name when -r is used.
 */

static
int
ckname()
{
	register int lastchar;

	if (G_p->g_namesz > Max_namesz) {
		msg(ER, ":68", "Name exceeds maximum length - skipped.");
		return(F_SKIP);
	}
	if (Pat_pp && !matched())
		return(F_SKIP);
	if ((Args & OCr) && !Adir) { /* rename interactively */
		(void)pfmt(Wtty_p, MM_NOSTD, ":69:Rename \"%s\"? ", G_p->g_nam_p);
		(void)fflush(Wtty_p);
		if (fgets(Renam_p, Max_namesz, Rtty_p) == (char *)NULL)
			msg(EXT, badreadttyid, badreadtty, Ttyname);
		if (feof(Rtty_p)) {
			if (!Dflag)
				/* Protect access to Error_cnt with mutex. */
				MUTEX_LOCK(&Buffr.b_mutex);
			exit(Error_cnt ? EXIT_FILE : EXIT_OK);
		}
		lastchar = strlen(Renam_p) - 1;
		if (*(Renam_p + lastchar) == '\n') /* remove trailing '\n' */
			*(Renam_p + lastchar) = '\0';
		if (*Renam_p == '\0') {
			msg(POST, ":70", "%s Skipped.", G_p->g_nam_p);
			*G_p->g_nam_p = '\0';
			return(F_SKIP);
		} else if (strcmp(Renam_p, ".")) {
			G_p->g_nam_p = Renam_p;
		}
	}
	VERBOSE((Args & OCt), G_p->g_nam_p);
	if (Args & OCt)
		return(F_SKIP);
	return(F_EXTR);
}

/*
 * Procedure:     ckopts
 *
 * Restrictions:
 *                fopen:		none
 *                open(2):	none
 *                getpwnam:	none
 */

/*
 * ckopts: Check the validity of all command line options.
 */

static
void
ckopts(mask)
register long mask;
{
	register int oflag;
	register char *t_p;
	register long errmsk;
	char inval_opt;

	/* Check for invalid options with -i. */
	if (mask & OCi) {
		errmsk = mask & INV_MSK4i;
		if (errmsk) {  /* if non-zero, invalid options */
			if (mask & OCa)
				inval_opt = 'a';
			else if (mask & OCo)
				inval_opt = 'o';
			else if (mask & OCp)
				inval_opt = 'p';
			else if (mask & OCA)
				inval_opt = 'A';
			else if (mask & OCL)
				inval_opt = 'L';
			else if (mask & OCO)
				inval_opt = 'O';
			/* Enhanced Application Compatibility Support */
			else if (mask & OCK)
				inval_opt = 'K';
			/* End Enhanced Application Compatibility Support */
			msg(USAGE, ":1073", "-%c cannot be used with -%c", inval_opt, 'i');
		}
	}

	/* Check for invalid options with -o. */
	else if (mask & OCo) {
		errmsk = mask & INV_MSK4o;
		if (errmsk) {  /* if non-zero, invalid options */
			if (mask & OCi)
				inval_opt = 'i';
			else if (mask & OCk)
				inval_opt = 'k';
			else if (mask & OCm)
				inval_opt = 'm';
			else if (mask & OCp)
				inval_opt = 'p';
			else if (mask & OCr)
				inval_opt = 'r';
			else if (mask & OCt)
				inval_opt = 't';
			else if (mask & OCE)
				inval_opt = 'E';
			else if (mask & OCI)
				inval_opt = 'I';
			else if (mask & OCR)
				inval_opt = 'R';
			else if (mask & OC6)
				inval_opt = '6';
			/* Enhanced Application Compatibility Support */
			else if (mask & OCT)
				inval_opt = 'T';
			/* End Enhanced Application Compatibility Support */
			msg(USAGE, ":1073", "-%c cannot be used with -%c", inval_opt, 'o');
		}
	}

	/* Check for invalid options with -p. */
	else if (mask & OCp) {
		errmsk = mask & INV_MSK4p;
		if (errmsk) {  /* if non-zero, invalid options */
			if (mask & OCf)
				inval_opt = 'f';
			else if (mask & OCi)
				inval_opt = 'i';
			else if (mask & OCk)
				inval_opt = 'k';
			else if (mask & OCo)
				inval_opt = 'o';
			else if (mask & OCr)
				inval_opt = 'r';
			else if (mask & OCt)
				inval_opt = 't';
			else if (mask & OCA)
				inval_opt = 'A';
			else if (mask & OCE)
				inval_opt = 'E';
			else if (mask & OCG)
				inval_opt = 'G';
			else if (mask & OCH)
				inval_opt = 'H';
			else if (mask & OCI)
				inval_opt = 'I';
			else if (mask & OCO)
				inval_opt = 'O';
			else if (mask & OC6)
				inval_opt = '6';
			/* Enhanced Application Compatibility Support */
			else if (mask & OCK)
				inval_opt = 'K';
			else if (mask & OCT)
				inval_opt = 'T';
			/* End Enhanced Application Compatibility Support */
			msg(USAGE, ":1073", "-%c cannot be used with -%c", inval_opt, 'p');
		}
	}

	/* None of -i, -o, or -p were specified. */
	else
		msg(USAGE, ":71", "One of -i, -o or -p must be specified.");

	/* Check for mutually exclusive options. */
	/* This bunch are the mutually exclusive header formats. */
	if ((mask & OCc) && (mask & OCH) && (strcmp("tar", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OCH) && (strcmp("TAR", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OCH) && (strcmp("ustar", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OCH) && (strcmp("USTAR", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OCH) && (strcmp("odc", Hdr_p) == 0))
		msg(USAGE, mutexid, mutex, 'c', 'H');
	if ((mask & OCc) && (mask & OC6))
		msg(USAGE, mutexid, mutex, 'c', '6');
	if ((mask & OCH) && (mask & OC6))
		msg(USAGE, mutexid, mutex, 'H', '6');

	/* These are various other mutually exclusive options. */
	if ((mask & OCB) && (mask & OCC))
		msg(USAGE, mutexid, mutex, 'B', 'C');

	if ((mask & OCM) && !((mask & OCI) || (mask & OCO)))
		msg(USAGE, ":72", "-M not meaningful without -O or -I.");
	if ((mask & OCA) && !(mask & OCO))
		msg(USAGE, ":73", "-A requires the -O option.");

	if (Bufsize <= 0) {
		msg(ER, ":74", "Illegal size given for -C option.");
		exit(EXIT_USAGE);
	}

	if (mask & OCH) {
		t_p = Hdr_p;
		while (*t_p != NULL) {
			if (isupper(*t_p))
				*t_p = 'a' + (*t_p - 'A');
			t_p++;
		}
		if (!strcmp("odc", Hdr_p)) {
			Hdr_type = CHR;
			Max_namesz = CPATH;
			Onecopy = 0;
			Oldstat = 1;
		} 
		else if (!strcmp("crc", Hdr_p)) {
			Hdr_type = CRC;
			Max_namesz = APATH;
			Onecopy = 1;
		} 
		else if (!strcmp("tar", Hdr_p)) {
			if(Args & OCo) {
				Hdr_type = USTAR;
				Max_namesz = HNAMLEN - 1;
			} 
			else {
				Hdr_type = TAR;
				Max_namesz = TNAMLEN - 1;
			}
			Onecopy = 0;
		} 
		else if (!strcmp("ustar", Hdr_p)) {
			Hdr_type = USTAR;
			Max_namesz = HNAMLEN - 1;
			Onecopy = 0;
		} 
		else {
			msg(ER, ":75", "Invalid header \"%s\" specified", Hdr_p);
			exit(EXIT_USAGE);
		}
	}

	if (mask & (OCr|OCG)) {
		if (!strcmp(Ttyname, "STDIO")) {
			Rtty_p = stdin;
			Wtty_p = stdout;
		} else {
			open_tty();
		}
	}

	if ((mask & OCE) && (Ef_p = fopen(Efil_p, "r")) == (FILE *)NULL) {
		msg(ER, ":77", "Cannot open \"%s\" to read patterns", Efil_p);
		exit(EXIT_FATAL);
	}

	if ((mask & OCI) && (Archive = open(IOfil_p, O_RDONLY)) < 0) {
		msg(ER, ":78", "Cannot open \"%s\" for input", IOfil_p);
		exit(EXIT_FATAL);
	}

	if (mask & OCO) {
		if (mask & OCA) {
			if ((Archive = open(IOfil_p, O_RDWR | O_EXCL)) < 0) {
				msg(ER, ":79", "Cannot open \"%s\" for append", IOfil_p);
				exit(EXIT_FATAL);
			}
		} else {
			/* 
		 	* We want exclusive access to archive.  open(2) will 
			* fail if O_EXCL and O_CREAT are set and the file 
			* exists,  so we must determine if the file exists.
		 	*/
			if (stat(IOfil_p, &ArchSt) == 0) /* file exists */
				oflag = (O_WRONLY | O_TRUNC | O_EXCL);
			else /* file does not exist */
				oflag = (O_WRONLY | O_CREAT | O_EXCL);
			if ((Archive = open(IOfil_p, oflag, 0666)) < 0) {
				msg(ERN, ":80", "Cannot open \"%s\" for output", IOfil_p);
				exit(EXIT_FATAL);
			}
		}
	}

	if (mask & OCR) {
		if (!privileged) {
			msg(ER, ":891", "Must be privileged for %s option", "-R");
			exit(EXIT_USAGE);
		}
		if ((Rpw_p = getpwnam(Own_p)) == (struct passwd *)NULL) {
			msg(ER, ":82", "Unknown user id: %s", Own_p);
			exit(EXIT_FATAL);
		}
	}

	/* if forced extent attribute save - must be char archive if -o */
	if ((mask & OCo) && extent_op == OCe_FORCE &&
	    Hdr_type != ASC && Hdr_type != CRC) {
		msg(ER, ":1087", "Attribute preservation requires -c option or -Hcrc option");
		exit(EXIT_USAGE);
	}

	if ((mask & OCo) && !(mask & OCO))
		Out_p = stderr;

	/* Enhanced Application Compatibility Support */

	/* K option */
	if (Mediasize)	{
		if (Bufsize & 0x3ff) {
			if (mask & OCC)
				msg(ER, ":1074", "Invalid argument \"%d\" for -C; must be multiple of 1K when used with -K", Bufsize);
			else
				msg(ER, ":1075", "Must use -C to specify a 1K-multiple bufsize when using -K"); 
			exit(EXIT_USAGE);
		
		}
	}

	/* End Enhanced Application Compatibility Support 	*/
}

/*
 * Procedure:     cksum
 *
 * Restrictions:
 *                read(2):	none
 */

/*
 * cksum: Calculate the simple checksum of a file (CRC) or header
 * (TARTYP (TAR and USTAR)).  For -o and the CRC header, the file is opened and 
 * the checksum is calculated.  For -i and the CRC header, the checksum
 * is calculated as each block is transferred from the archive I/O buffer
 * to the file system I/O buffer.  The TARTYP (TAR and USTAR) headers calculate 
 * the simple checksum of the header (with the checksum field of the 
 * header initialized to all spaces (\040).
 */

static
long 
cksum(hdr, byt_cnt)
char hdr;
int byt_cnt;
{
	register char *crc_p, *end_p;
	register int cnt;
	register long checksum = 0L, lcnt, have;

	switch (hdr) {
	case CRC:
		if (Args & OCi) { /* do running checksum */
			end_p = Buffr.b_out_p + byt_cnt;
			for (crc_p = Buffr.b_out_p; crc_p < end_p; crc_p++)
				checksum += (long)*crc_p;
			break;
		}
		/* OCo - do checksum of file */
		lcnt = G_p->g_filesz;
		while (lcnt > 0) {
			have = (lcnt < CPIOBSZ) ? lcnt : CPIOBSZ;
			errno = 0;
			if (read(Ifile, Buf_p, have) != have) {
				msg(ER, ":83", "Error computing checksum.");
				checksum = -1L;
				break;
			}
			end_p = Buf_p + have;
			for (crc_p = Buf_p; crc_p < end_p; crc_p++)
				checksum += (long)*crc_p;
			lcnt -= have;
		}
		if (lseek(Ifile, 0, SEEK_ABS) < 0)
			msg(ERN, ":84", "Cannot reset file after checksum");
		break;
	case TARTYP: /* TAR and USTAR */
		crc_p = Thdr_p->tbuf.t_cksum;
		for (cnt = 0; cnt < TCRCLEN; cnt++) {
			*crc_p = '\040';
			crc_p++;
		}
		crc_p = (char *)Thdr_p;
		for (cnt = 0; cnt < TARSZ; cnt++) {
			checksum += (long)*crc_p;
			crc_p++;
		}
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	} /* hdr */
	return(checksum);
}

/*
 * Procedure:     creat_hdr
 *
 * Restrictions:
 *                open:	none
 *                getpwuid:	none
 *                getgrgid:	P_MACREAD
 */

/*
 * creat_hdr: Fill in the generic header structure with the specific
 * header information based on the value of Hdr_type.
 */

static
int
creat_hdr()
{
	register ushort ftype;
	char goodbuf[MAXNAM+1];
	static char junkbuf[NAMSIZ+1];
	static char abuf[PRESIZ+1];
	char *tmpbuf, *lastcomp;
	int split, i, Alink;

	ftype = SrcSt.st_mode & Ftype;
	Adir = (ftype == S_IFDIR);
	Aspec = (ftype == S_IFBLK || ftype == S_IFCHR || ftype == S_IFIFO || ftype == S_IFNAM);
	Alink = (ftype == S_IFLNK);

	/* We want to check here if the file is really readable --
	   If we wait till later it's too late and cpio will fail */

	if (!Adir && !Aspec && !Alink) {
		if ((Ifile = open(Gen.g_nam_p, O_RDONLY)) < 0) {
			msg(ER, ":65", "Cannot open \"%s\"", Gen.g_nam_p);
			return(0);
		}
	}
	switch (Hdr_type) {
	case BIN:
		Gen.g_magic = CMN_BIN;
		break;
	case CHR:
		Gen.g_magic = CMN_BIN;
		break;
	case ASC:
		Gen.g_magic = CMN_ASC;
		break;
	case CRC:
		Gen.g_magic = CMN_CRC;
		break;
	case USTAR:
		/* If the length of the fullname is greater than 256,
	   	print out a message and return.
		*/
		for (i = 0; i <= MAXNAM; i++)
			goodbuf[i] = '\0';
		for (i = 0; i <= NAMSIZ; i++)
			junkbuf[i] = '\0';
		for (i = 0; i <= PRESIZ; i++)
			abuf[i] = '\0';
		i = 0;

		if ((split = strlen(Gen.g_nam_p)) > MAXNAM) {
			CLOSE_Ifile;
			msg(ER, ":85", "%s: file name too long", Gen.g_nam_p);
			return(0);
		} else if (split > NAMSIZ) {
			/* The length of the fullname is greater than 100, so
		   	we must split the filename from the path
			*/
			(void)strcpy(&goodbuf[0], Gen.g_nam_p);
			tmpbuf = goodbuf;
			lastcomp = basename(tmpbuf);
			i = split - (lastcomp - tmpbuf);
			/* If the filename is greater than 100 we can't
		   	archive the file
			*/
			if (i > NAMSIZ) {
				CLOSE_Ifile;
				msg(WARN, ":86", "%s: filename is greater than %d",
				lastcomp, NAMSIZ);
				return(0);
			}
			(void)strcpy(&junkbuf[0], lastcomp);
			/* If the prefix is greater than 155 we can't archive the
		   	file.
			*/
			if ((split - i) > PRESIZ) {
				CLOSE_Ifile;
				msg(WARN, ":87", "%s: prefix is greater than %d",
				Gen.g_nam_p, PRESIZ);
				return(0);
			}
			(void)strncpy(&abuf[0], &goodbuf[0], split - i);
			Gen.g_tname = junkbuf;
			Gen.g_prefix = abuf;
		} else {
			Gen.g_tname = Gen.g_nam_p;
			Gen.g_prefix = NULL;
		}
		(void)strcpy(Gen.g_tmagic, "ustar");
		(void)strncpy(Gen.g_version, "00", 2);
		dpasswd = getpwuid(SrcSt.st_uid);
		if (dpasswd == (struct passwd *) NULL) {
			msg(WARN, badpasswdid, badpasswd, Gen.g_nam_p);
			Gen.g_uname[0]='\0';
		}
		else
			(void)strncpy(&Gen.g_uname[0], dpasswd->pw_name, 32);
		procprivl (CLRPRV, MACREAD_W, 0);
		dgroup = getgrgid(SrcSt.st_gid);
		procprivl (SETPRV, MACREAD_W, 0);
		if (dgroup == (struct group *) NULL) {
			msg(WARN, badgroupid, badgroup, Gen.g_nam_p);
			Gen.g_gname[0]='\0';
		}	
		else
			(void)strncpy(&Gen.g_gname[0], dgroup->gr_name, 32);
		switch(ftype) {
			case S_IFDIR:
				Gen.g_typeflag = DIRTYPE;
				break;
			case S_IFREG:
				Gen.g_typeflag = REGTYPE;
				break;
			case S_IFLNK:
				Gen.g_typeflag = SYMTYPE;
				break;
			case S_IFBLK:
				Gen.g_typeflag = BLKTYPE;
				break;
			case S_IFCHR:
				Gen.g_typeflag = CHRTYPE;
				break;
			case S_IFIFO:
				Gen.g_typeflag = FIFOTYPE;
				break;
			case S_IFNAM:
				Gen.g_typeflag = NAMTYPE;
				break;
		}
	/* FALLTHROUGH*/
	case TAR:
		T_lname[0] = '\0';
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	}

	Gen.g_namesz = strlen(Gen.g_nam_p) + 1;
	Gen.g_uid = SrcSt.st_uid;
	Gen.g_gid = SrcSt.st_gid;
	Gen.g_dev = SrcSt.st_dev;
	Gen.g_ino = SrcSt.st_ino;
	Gen.g_mode = SrcSt.st_mode;
	Gen.g_mtime = SrcSt.st_mtime;
	Gen.g_nlink = SrcSt.st_nlink;
	if (ftype == S_IFREG || ftype == S_IFLNK)
		Gen.g_filesz = SrcSt.st_size;
	else
		Gen.g_filesz = 0L;
	Gen.g_rdev = SrcSt.st_rdev;
	return(1);
}

/*
 * Procedure:     creat_lnk
 *
 * Restrictions:
 *                link(2):	none
 *                unlink(2):	none
 */

/*
 * creat_lnk: Create a link from the existing name1_p to name2_p.
 */

static
int
creat_lnk(name1_p, name2_p)
register char *name1_p, *name2_p;
{
	int ret;

	if ((ret = link(name1_p, name2_p)) != 0 && errno != EEXIST) {
		if (missdir(name2_p)) {
			msg(ERN, badcreatdirid, badcreatdir, name2_p);
			return(1);
		} else {
			ret = link(name1_p, name2_p);
		}
	}
	if (ret != 0) {
		if (errno == EEXIST) {
			/*
			 * If this next statement is true (i.e. same age or
			 * newer and no -u), no need to keep trying; just
			 * return.
			 */
			if (!(Args & OCu) && stat(name2_p, &DesSt) >= 0 &&
			    G_p->g_mtime <= DesSt.st_mtime) {
				msg(POST, sameageid, sameage, name2_p);
				return(0);
			} else {
				/*
				 * The simplest thing to do here would be to unlink the
				 * existing destination file, and retry the link.  But
				 * this would create a brief window where the destination
				 * path did not exist.  This window can be deadly in
				 * situations like boot floppy installation, where Really
				 * Important Stuff like /dev/zero and libc.so.1 are being
				 * cpio-ed into place from the base package, replacing the
				 * copies put in place by the boot floppy.  The code below
				 * avoids such a window.
				 */
				char *t_p;

				(void)strcpy(Over_p, name2_p);
				if ((t_p = strrchr(Over_p, '/')) == NULL)
					t_p = Over_p;
				else
					t_p++;
				strcpy(t_p, "XXXXXX");
				(void)mktemp(Over_p);
				if (*Over_p == '\0') {
					msg(ER, ":95", "Cannot get temporary file name.");
				} else if (link(name1_p, Over_p) == 0) {
					if ((ret = rename(Over_p, name2_p) != 0)) {
						/* warning message printed below */
						(void)unlink(Over_p);
					}
				}
				*Over_p = '\0';
			}
		} 
	}
	if (!ret) {
		if (vflag)
			(void)pfmt(Err_p, MM_NOSTD, ":89:%s linked to %s\n", name1_p, name2_p);
		VERBOSE((Vflag|vflag), name2_p);
	} else
		msg(WARNN, ":90", "Cannot link \"%s\" and \"%s\"", name1_p, name2_p);
	return(ret);
}

/*
 * Procedure:     creat_spec
 *
 * Restrictions:
 *                stat(2):	none
 *                mkdir(2):	none
 *                mknod(2):	none
 */

/*
 * creat_spec:
 */

static
int
creat_spec()
{
	register char *nam_p;
	register char *Snam_p;
	register int cnt, result, rv = 0;
	char *curdir;
	int lvl_change = 0; /* indicate whether level of process is changed */

	if (Args & OCp)
		nam_p = Fullnam_p;
	else
		nam_p = G_p->g_nam_p;
	if (Target_mld && chg_lvl_proc(Nam_p)) {
		result = stat(Fullnam_p, &DesSt);
		restore_lvl_proc();
	} else
		result = stat(nam_p, &DesSt);
	if ((Snam_p = (char *)malloc((unsigned int)strlen(nam_p) + 1)) == (char *)NULL)
			msg(EXT, nomemid, nomem);
	(void)strcpy(Snam_p, nam_p);
	if (Adir) {
		/* strip-off ending '.'s and '/'s */
		curdir = strrchr(Snam_p, '.');
		if (curdir != NULL && curdir[1] == '\0'){
			while((curdir > Snam_p) && ((*curdir == '.') || (*curdir == '/'))){
				if (*curdir == '/')
					*curdir = '\0';
				curdir--;
			}
			if (curdir == Snam_p) {
				free(Snam_p);
				return(1);
			}
		}
		if (!result && (Args & OCd)) {
			if ((DesSt.st_mode & Ftype ) == S_IFDIR) {
				/* If here, directory already exists.*/
				Exists_flag = 1;
				rstfiles(U_KEEP);
				free(Snam_p);
				return(1);
			}
			else {
				/* file exists with this name */
				if (Args & OCu){
					remove(nam_p);
				}
				result = -1;
			}
		}
		if (!result || !(Args & OCd)) {
			free(Snam_p);
			return(1);
		}
	} else if (!result && creat_tmp(nam_p) < 0) {
		free(Snam_p);
		return(0);
	}
	cnt = 0;

	do {
		/* change level of process to that of the source file */
		if (macpkg && (Args & OCp) && (Args  & OCm)) {
			lvl_change = chg_lvl_proc(Nam_p);
			if (Mldmode_chg) {
				/*
				 * If the target directory is an MLD,
				 * we may have modified the Full path name
				 * by inserting the effective directory.
				 */
				nam_p = Snam_p;
				Snam_p = Fullnam_p;
			}
		}
		if (Adir) {
			/* create an MLD if source directory is an MLD */
			if (macpkg && (Args & OCp) && (Args  & OCd)) {
				result = creat_mld(Nam_p, Snam_p, G_p->g_mode);
			}
			else /* a regular directory is to be created */
				result = 1;

			/* a regular directory is to be created */
			if (result == 1) {
				result = mkdir(Snam_p, G_p->g_mode);
			}
		}
		else if (Aspec)
			result = mknod(nam_p, (int)G_p->g_mode, (int)G_p->g_rdev);

		/* restore level of process */
		if (lvl_change) {
			if (Mldmode_chg) {
				restore_lvl_proc();
				Snam_p = nam_p;
				nam_p = Fullnam_p;
			} else {
				restore_lvl_proc();
			}
		}

		if (result >= 0) {
			/* transfer ACL of source file to destination file */
			if (Args & OCp)
				transferdac(Nam_p, nam_p, G_p->g_mode);
			cnt = 0;
			break;
		}
		cnt++;
	} while (cnt < 2 && !missdir(Snam_p));

	switch (cnt) {
	case 0:
		rv = 1;
		rstfiles(U_OVER);
		break;
	case 1:
		msg(ERN, badcreatdirid, badcreatdir, nam_p);
		if (*Over_p != '\0')
			rstfiles(U_KEEP);
		break;
	case 2:
		if (Adir)
			msg(ERN, ":91", "Cannot create directory \"%s\"", nam_p);
		else if (Aspec)
			msg(ERN, ":92", "mknod() failed for \"%s\"", nam_p);

		if (*Over_p != '\0')
			rstfiles(U_KEEP);
		break;
	default:
		msg(EXT, badcaseid, badcase);
	}
	free(Snam_p);
	return(rv);
}

/*
 * Procedure:     creat_tmp
 *
 * Restrictions:
 *                mktemp(2):	none
 *                rename(2):	none
 */

/*
 * creat_tmp:
 */

static
int
creat_tmp(nam_p)
char *nam_p;
{
	register char *t_p;

	if ((Args & OCp) && G_p->g_ino == DesSt.st_ino && G_p->g_dev == DesSt.st_dev) {
		msg(ER, ":93", "Attempt to pass a file to itself.");
		return(-1);
	}
	if (G_p->g_mtime <= DesSt.st_mtime && !(Args & OCu)) {
		msg(POST, sameageid, sameage, nam_p);
		CLOSE_Ifile;
		return(-1);
	}
	if (Aspec) {
		msg(ER, ":94", "Cannot overwrite \"%s\"", nam_p);
		return(-1);
	}
	(void)strcpy(Over_p, nam_p);
	t_p = Over_p + strlen(Over_p);
	while (t_p != Over_p) {
		if (*(t_p - 1) == '/')
			break;
		t_p--;
	}
	(void)strcpy(t_p, "XXXXXX");
	(void)mktemp(Over_p);
	if (*Over_p == '\0') {
		msg(ER, ":95", "Cannot get temporary file name.");
		return(-1);
	}
	if (rename(nam_p, Over_p) < 0) {
		msg(ERN, ":772", "Cannot rename %s", nam_p);
		*Over_p = '\0';
		return(-1);
	}
	return(1);
}

/*
 * Procedure:     data_in
 *
 * Restrictions:
 *                write(2):	none
 */

/*
 * data_in:  If proc_mode == P_PROC, write(2) the file's data to the open fdes
 * gotten from openout().  If proc_mode == P_SKIP, or becomes P_SKIP (due to
 * errors etc), skip the file's data.  If the user specified any of the "swap"
 * options (b, s or S), and the length of the file is not appropriate for that
 * action, do not perform the "swap", otherwise perform the action on a buffer
 * by buffer basis. If the CRC header was selected, calculate a running
 * checksum as each buffer is processed.
 */

static
void
data_in(proc_mode)
register int proc_mode;
{
	register char *nam_p;
	register int cnt, pad;
	register long filesz, cksumval = 0L;
	register int bytes_written, swapfile = 0;
	int	exterr = 0;
	int	noextend = 0;
	struct	vx_ext extbuf;
	struct	statvfs statvfsbuf;
	int	do_rst = 0;

	nam_p = G_p->g_nam_p;
	if ((G_p->g_mode & Ftype) == S_IFLNK && proc_mode != P_SKIP) {
		do_rst = 1;		/* remember to call rstfiles */
		proc_mode = P_SKIP;
		VERBOSE((Vflag|vflag), nam_p);
	}
	if (Args & (OCb | OCs | OCS)) { /* verfify that swapping is possible */
		swapfile = 1;
		if (Args & (OCs | OCb) && G_p->g_filesz % 2) {
			msg(ER, ":98", "Cannot swap bytes of \"%s\", odd number of bytes", nam_p);
			swapfile = 0;
		}
		if (Args & (OCS | OCb) && G_p->g_filesz % 4) {
			msg(ER, ":99", "Cannot swap halfwords of \"%s\", odd number of halfwords", nam_p);
			swapfile = 0;
		}
	}

	extbuf.ext_size = 0;
	extbuf.a_flags = 0;
	noextend = 0;
	filesz = G_p->g_filesz;
	/* see if need to preserve extent information */
	if (proc_mode != P_SKIP && extent_op != OCe_IGNORE &&
	    vx_extcpio.magic == VX_CPIOMAGIC &&
	    (vx_extcpio.extinfo.ext_size ||
	     vx_extcpio.extinfo.reserve ||
	     vx_extcpio.extinfo.a_flags)) {
		if (fstatvfs(Ofile, &statvfsbuf) ||
		    strcmp(statvfsbuf.f_basetype, "vxfs") ||
		    ((long)vx_extcpio.extinfo.ext_size * 1024L) %
				(long)statvfsbuf.f_frsize ||
		    ((long)vx_extcpio.extinfo.reserve * 1024L) %
				(long)statvfsbuf.f_frsize)
			exterr = 1;
		 vx_extcpio.extinfo.ext_size /=
			(long)statvfsbuf.f_frsize / 1024L;
		 vx_extcpio.extinfo.reserve /=
			(long)statvfsbuf.f_frsize / 1024L;
		if ((vx_extcpio.extinfo.a_flags & VX_NOEXTEND) &&
		     filesz > vx_extcpio.extinfo.reserve) {
			noextend++;
			vx_extcpio.extinfo.a_flags &= ~VX_NOEXTEND;
		}
		if (exterr == 0) {
			if (ioctl(Ofile, VX_SETEXT, &vx_extcpio.extinfo) != 0) {
				exterr = 1;
			}
			extbuf = vx_extcpio.extinfo;
		}
		if (exterr) {
			if (extent_op == OCe_WARN) {
				exterr = 0;
				msg(WARNN, ":1096", "Cannot maintain attributes of %s", nam_p);
			} else {
				msg(ERN, ":1096", "Cannot maintain attributes of %s", nam_p);
			}
		}
	}
	if (proc_mode != P_SKIP && !fstatvfs(Ofile, &statvfsbuf) &&
	    !strcmp(statvfsbuf.f_basetype, "vxfs") && filesz > MAXBSIZE) {
		extbuf.reserve = (daddr_t)(filesz + statvfsbuf.f_frsize - 1) /
			(long)statvfsbuf.f_frsize;
		extbuf.a_flags |= VX_NORESERVE;
		(void)ioctl(Ofile, VX_SETEXT, &extbuf);
		extbuf.a_flags &= ~VX_NORESERVE;
	}
	while (filesz > 0) {
		if (Gotsigint)
			bugout();
		cnt = (int)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		WAIT_FOR_DATA(cnt);
		if (proc_mode != P_SKIP && exterr == 0) {
			if (Hdr_type == CRC)
				cksumval += cksum(CRC, cnt);
			if (swapfile)
				swap(Buffr.b_out_p, cnt);
			errno = 0;
			bytes_written = write(Ofile, Buffr.b_out_p, cnt);
			if (bytes_written < cnt) {
				if (bytes_written < 0)
	 				msg(ERN, badwriteid, badwrite, nam_p);
				else
					msg(EXTN, badwriteid, badwrite, nam_p);
				proc_mode = P_SKIP;
				rstfiles(U_KEEP);
				(void)close(Ofile);
			}
		}
		DECR_BUFCNT(cnt);
		filesz -= (long)cnt;
	} /* filesz */
	pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
	if (pad != 0) {
		WAIT_FOR_DATA(pad);
		DECR_BUFCNT(pad);
	}
	/* 
	 * Do delayed NOEXTEND in cases where isize > reservation.
	 */
	if (proc_mode != P_SKIP && noextend && !exterr) {
		extbuf.a_flags |= VX_NOEXTEND;
		if (ioctl(Ofile, VX_SETEXT, &extbuf)) {
			exterr++;
			if (extent_op == OCe_WARN) {
				msg(WARN, ":712", "%s: ioctl() %s failed: %s", nam_p, "setext", strerror(errno));
			} else {
				msg(ER, ":712", "%s: ioctl() %s failed: %s", nam_p, "setext", strerror(errno));
			}
		}
	}
	if (proc_mode != P_SKIP) {
		if (Hdr_type == CRC && G_p->g_cksum != cksumval) {
			msg(ER, ":100", "\"%s\" - checksum error", nam_p);
			rstfiles(U_KEEP);
		} else if (exterr) {
			rstfiles(U_KEEP);
		} else
			rstfiles(U_OVER);
		(void)close(Ofile);
	} 
	else {
		/* For symbolic link, still need to call rstfiles()
	 	* to restore some attributes. 
	 	*/
		if ((G_p->g_mode & Ftype) == S_IFLNK && do_rst && !(Args & OCt))
			rstfiles(U_OVER);
	}

	VERBOSE((proc_mode != P_SKIP && (Vflag|vflag)), nam_p);
	if (!Dflag) MUTEX_LOCK(&Sig_mutex);
	Finished = 1;
	if (!Dflag) MUTEX_UNLOCK(&Sig_mutex);
}

/*
 * Procedure:     data_out
 *
 * Restrictions:
 *                readlink(2):	none
 *                read(2):	none
 */

/*
 * data_out:  open the file to be archived, compute the checksum
 * of its data if the CRC header was specified and write the header.
 * read(2) each block of data into the buffer.  For TARTYP (TAR and USTAR)
 * archives, pad the data with NULLs to the next 512 byte boundary.
 */

static
void
data_out()
{
	register char *nam_p;
	register int cnt, bytes_read, pad;
	register long filesz;
	char *oldBufp;
	struct	statvfs statvfsbuf;
	struct	vx_ext extbuf;

	nam_p = G_p->g_nam_p;
	if (Aspec) {
		write_hdr();
		rstfiles(U_KEEP);
		VERBOSE((Vflag|vflag), nam_p);
		return;
	}
	if ((G_p->g_mode & Ftype) == S_IFLNK && (Hdr_type != USTAR && Hdr_type != TAR)) { /* symbolic link */
		write_hdr();
		WAIT_FOR_SPACE(G_p->g_filesz);
		errno = 0;
		if (readlink(nam_p, Buffr.b_in_p, G_p->g_filesz) < 0) {
			msg(ERN, badreadsymid, badreadsym, nam_p);
			return;
		}
		INCR_BUFCNT(G_p->g_filesz);
		pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
		if (pad != 0) {
			WAIT_FOR_SPACE(pad);
			(void)memset(Buffr.b_in_p, 0, pad);
			INCR_BUFCNT(pad);
		}
		VERBOSE((Vflag|vflag), nam_p);
		return;
	} else if ((G_p->g_mode & Ftype) == S_IFLNK && (Hdr_type == USTAR || Hdr_type == TAR)) {
		if (G_p->g_filesz > NAMSIZ) {
			msg(ER, ":1255", "\"%s\": Symbolic link too long", nam_p);
			return;
		}
		if (readlink(nam_p, T_lname, G_p->g_filesz) < 0) {
			msg(ERN, badreadsymid, badreadsym, nam_p);
			return;
		}
		T_lname[G_p->g_filesz] = '\0';
		G_p->g_filesz = 0L;
		write_hdr();
		VERBOSE((Vflag|vflag), nam_p);
		return;
	}

	if (Hdr_type == CRC && (G_p->g_cksum = cksum(CRC, 0)) < 0) {
		msg(POST, ":102", "\"%s\" skipped", nam_p);
		CLOSE_Ifile;
		return;
	}

	/* Copy extent information into header */
	memset(&vx_extcpio.extinfo, '\0', sizeof (struct vx_ext));
	if (extent_op != OCe_IGNORE && (G_p->g_mode & Ftype) == S_IFREG) {
		if (fstatvfs(Ifile, &statvfsbuf)) {
			msg(ERN, ":1095", "fstatvfs() on %s failed", nam_p);
			CLOSE_Ifile;
			return;
		}
		if (!strcmp(statvfsbuf.f_basetype, "vxfs")) {
			if (ioctl(Ifile, VX_GETEXT, &extbuf)) {
				msg(ER, ":712", "%s: ioctl() %s failed: %s", nam_p, "getext", strerror(errno));
				CLOSE_Ifile;
				return;
			} else {
				extbuf.reserve *= (daddr_t)statvfsbuf.f_frsize /
					(daddr_t)1024;
				extbuf.ext_size *= 
					(daddr_t)statvfsbuf.f_frsize / 
					(daddr_t)1024;
				vx_extcpio.extinfo = extbuf;
				if ((extbuf.reserve ||
				     extbuf.ext_size ||
				     extbuf.a_flags) &&
				    ((Hdr_type != CRC && Hdr_type != ASC) ||
				     G_p->g_namesz > (EXPNLEN - VX_CPIONEED))) {
					if (extent_op == OCe_FORCE) {
						msg(ER, ":1096", "Cannot maintain attributes of %s", nam_p);
						CLOSE_Ifile;
						return;
					} else {
						msg(WARN, ":1096", "Cannot maintain attributes of %s", nam_p);
					}
				}
			}
		}
	}

	write_hdr();
	filesz = G_p->g_filesz;

	while (filesz > 0) {
		cnt = (unsigned)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		WAIT_FOR_SPACE(cnt);
		errno = 0;
		if ((bytes_read = read(Ifile, Buffr.b_in_p, (unsigned)cnt)) <= 0) {
			msg(EXTN, badreadid, badread, nam_p);
			break;
		}
		INCR_BUFCNT(bytes_read);
		filesz -= (long)bytes_read;
	}
	pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
	if (pad != 0) {
		WAIT_FOR_SPACE(pad);
		(void)memset(Buffr.b_in_p, 0, pad);
		INCR_BUFCNT(pad);
	}
	rstfiles(U_KEEP);
	CLOSE_Ifile;
	VERBOSE((Vflag|vflag), nam_p);
}

/*
 * Procedure:     data_pass
 *
 * Restrictions:
 *                read(2):	none
 *                write(2):	none
 */

/*
 * data_pass:  If not a special file (Aspec), open(2) the file to be 
 * transferred, read(2) each block of data and write(2) it to the output file
 * Ofile, which was opened in file_pass().
 */

static
void
data_pass()
{
	register int cnt, done = 1;
	register long filesz, orig_filesz;
	struct statvfs istatvfs, ostatvfs;
	struct vx_ext extbuf;
	long extinbytes, resinbytes;
	int exterr = 0;
	int noextend, doext = 0;
	char *addr1, *addr2;

	if (Aspec) {
		rstfiles(U_KEEP);
		(void)close(Ofile);
		VERBOSE((Vflag|vflag), Nam_p);
		return;
	}
	

	/* check if need to preserve extent attributes */
	memset(&extbuf, '\0', sizeof (extbuf));
	noextend = 0;
	filesz = G_p->g_filesz;
	if (fstatvfs(Ifile, &istatvfs) < 0) {
		msg(ERN, ":1095", "fstatvfs() on %s failed", Nam_p);
		exterr = 1;
	}
	if (exterr == 0 && fstatvfs(Ofile, &ostatvfs) < 0) {
		msg(ERN, ":1095", "fstatvfs() on %s failed", Fullnam_p);
		exterr = 1;
	}
	if (extent_op != OCe_IGNORE && exterr == 0 &&
	    !strcmp(istatvfs.f_basetype, "vxfs") &&
	    ioctl(Ifile, VX_GETEXT, &extbuf) < 0) {
		msg(ER, ":712", "%s: ioctl() %s failed: %s", Nam_p, "getext", strerror(errno));
		exterr = 1;
	}
	if (exterr == 0 && 
	    (extbuf.ext_size || extbuf.reserve || extbuf.a_flags)) {
		doext = 1;
		resinbytes = 
		     ((long)extbuf.reserve * (long)istatvfs.f_frsize);
		extinbytes = 
		     ((long)extbuf.ext_size * (long)istatvfs.f_frsize);
	}
	if (doext &&
	    (strcmp(ostatvfs.f_basetype, "vxfs") ||
	     resinbytes % (long)ostatvfs.f_frsize ||
	     extinbytes % (long)ostatvfs.f_frsize)) {
		doext = 0;
		if (extent_op == OCe_WARN) {
			msg(WARN, ":1096", "Cannot maintain attributes of %s", Nam_p);
		} else {
			exterr = 1;
			msg(ER, ":1096", "Cannot maintain attributes of %s", Nam_p);
		}
	}
	if (doext) {
		extbuf.reserve =
		     (daddr_t)(resinbytes / (long)ostatvfs.f_frsize);
		extbuf.ext_size =
		     (off_t)(extinbytes / (long)ostatvfs.f_frsize);
	}
	if (extbuf.a_flags & VX_NOEXTEND && filesz > extbuf.reserve) {
		noextend++;
		extbuf.a_flags &= ~VX_NOEXTEND;
	}
	if (doext && ioctl(Ofile, VX_SETEXT, &extbuf)) {
		memset(&extbuf, '\0', sizeof (extbuf));
		noextend = 0;
		if (extent_op == OCe_WARN) {
			msg(WARN, ":1096", "Cannot maintain attributes of %s", Nam_p);
		} else {
			exterr = 1;
			msg(ER, ":1096", "Cannot maintain attributes of %s", Nam_p);
		}
	}
	if (!doext) {
		memset(&extbuf, '\0', sizeof (extbuf));
	}
	if (exterr) {
		rstfiles(U_KEEP);
		CLOSE_Ifile;
		(void)close(Ofile);
		return;
	}
	/*
	 * Try to map the input and write it all in one write.
	 * Iff the monolithic write only partially succeeds
	 * is there a problem; total failures fall through to
	 * the old fashioned way; sucesses clear "filesz" to skip 
	 * the old fashioned way.
	 */
	if ((filesz > 0) && ((addr1 = (char *)sbrk(0)) != (char *)-1)) {
		addr1 += Pagesize - 1;
		addr1 = (char *)((long)addr1 & (~(Pagesize - 1)));
		addr2 = mmap(addr1, (size_t)filesz, PROT_READ,
			     MAP_SHARED|MAP_FIXED, Ifile, 0);
		if (addr2 != (char *)-1) {
			/* Save original filesz for munmap() below. */
			orig_filesz = filesz;
			if ((cnt = write(Ofile, addr2, (size_t)filesz)) > 0) {
				if (cnt != filesz) {
					msg(ERN, badwriteid, badwrite, Fullnam_p);
					done = 0;
				}
				/*
				 * success or unsalvagable failure.
				 */
				filesz = 0;
			}
			(void) munmap(addr2, (size_t)orig_filesz);
			Blocks += ((cnt + (BUFSZ - 1)) / BUFSZ);
		}
	};
	if (filesz > MAXBSIZE && !strcmp(ostatvfs.f_basetype, "vxfs")) {
		extbuf.reserve = (daddr_t)(filesz + ostatvfs.f_frsize - 1) /
			(long)ostatvfs.f_frsize;
		extbuf.a_flags |= VX_NORESERVE;
		(void)ioctl(Ofile, VX_SETEXT, &extbuf);
		extbuf.a_flags &= ~VX_NORESERVE;
	}
	while (filesz > 0) {
		if (Gotsigint)
			bugout();
		cnt = (unsigned)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		errno = 0;
		if (read(Ifile, Buf_p, (unsigned)cnt) < 0) {
			msg(ERN, badreadid, badread, Nam_p);
			done = 0;
			break;
		}
		errno = 0;
		if (write(Ofile, Buf_p, (unsigned)cnt) < 0) {
			msg(ERN, badwriteid, badwrite, Fullnam_p);
			done = 0;
			break;
		}
		Blocks += ((cnt + (BUFSZ - 1)) / BUFSZ);
		filesz -= (long)cnt;
	}
	if (done)
		rstfiles(U_OVER);
	else
		rstfiles(U_KEEP);
	CLOSE_Ifile;
	(void)close(Ofile);

	/* -p option already checked before entry to data_pass() */
	/* transfer ACL of source file to destination file */
	transferdac(Nam_p, Fullnam_p, G_p->g_mode);

	VERBOSE((Vflag|vflag), Fullnam_p);
	if (!Dflag) MUTEX_LOCK(&Sig_mutex);
	Finished = 1;
	if (!Dflag) MUTEX_UNLOCK(&Sig_mutex);
}

/*
 * Procedure:     file_in
 *
 * Restrictions:  none
 */

/*
 * file_in:  Process an object from the archive.  If a TARTYP (TAR or USTAR)
 * archive and g_nlink == 1, link this file to the file name in t_linkname 
 * and return.  Handle linked files in one of two ways.  If Onecopy == 0, this
 * is an old style (binary or -Hodc) archive, create and extract the data for the first 
 * link found, link all subsequent links to this file and skip their data.
 * If Oncecopy == 1, save links until all have been processed, and then 
 * process the links first to last checking their names against the patterns
 * and/or asking the user to rename them.  The first link that is accepted 
 * for xtraction is created and the data is read from the archive.
 * All subsequent links that are accepted are linked to this file.
 */

static
void
file_in()
{
	register struct Lnk *l_p, *tl_p;
	int lnkem = 0, cleanup = 0;
	int proc_file;
	struct Lnk *ttl_p;
	ulong keepsum = 0;

	G_p = &Gen;
	if ((Hdr_type == USTAR || Hdr_type == TAR) && G_p->g_nlink == 1) { /* TAR and USTAR */
		if (ckname() != F_SKIP)
			(void)creat_lnk(T_lname, G_p->g_nam_p);
		return;
	}
	if (Adir) {
		/*
		 * Initialize directory exists flag.  We don't
		 * know yet if the directory already exists.
		 */
		Exists_flag = 0;

		if (ckname() != F_SKIP && creat_spec() > 0) {
			VERBOSE((Vflag|vflag), G_p->g_nam_p);
		}
		return;
	}
	if (G_p->g_nlink == 1 || (Hdr_type == TAR || Hdr_type == USTAR)) {
		if (Aspec) {
			if (ckname() != F_SKIP && creat_spec() > 0)
				VERBOSE((Vflag|vflag), G_p->g_nam_p);
		} else {
			if ((ckname() == F_SKIP) || (Ofile = openout()) < 0) {
				data_in(P_SKIP);
			}
			else {
				data_in(P_PROC);
			}
		}
		return;
	}
	tl_p = add_lnk(&ttl_p);
	l_p = ttl_p;
	if (l_p->L_cnt == l_p->L_gen.g_nlink)
		cleanup = 1;
	if (!Onecopy) {
		lnkem = (tl_p != l_p) ? 1 : 0;
		G_p = &tl_p->L_gen;
		if (ckname() == P_SKIP) {
			data_in(P_SKIP);
		}
		else {
			if (!lnkem) {
				if (Aspec) {
					if (creat_spec() > 0)
						VERBOSE((Vflag|vflag), G_p->g_nam_p);
				} else if ((Ofile = openout()) < 0) {
					data_in(P_SKIP);
					reclaim(l_p);
				} else {
					data_in(P_PROC);
				}
			} else {
				(void)strcpy(Lnkend_p, l_p->L_gen.g_nam_p);
				(void)strcpy(Full_p, tl_p->L_gen.g_nam_p);
				(void)creat_lnk(Lnkend_p, Full_p); 
				data_in(P_SKIP);
				l_p->L_lnk_p = (struct Lnk *)NULL;
				free(tl_p->L_gen.g_nam_p);
				free(tl_p);
			}
		}
	} else { /* Onecopy */
		if (tl_p->L_gen.g_filesz)
			cleanup = 1;
		if (!cleanup)
			return; /* don't do anything yet */
		tl_p = l_p;
		/* 
		 * here we have a real checksum; the other linked headers don't
		 * have this info; keep it for when the file is installed
		 */
		if (Hdr_type == CRC)
			keepsum = G_p->g_cksum;
		while (tl_p != (struct Lnk *)NULL) {
			G_p = &tl_p->L_gen;
			if ((proc_file = ckname()) != F_SKIP) {
				if (l_p->L_data) {
					(void)creat_lnk(l_p->L_gen.g_nam_p, G_p->g_nam_p);
				} else if (Aspec) {
					(void)creat_spec();
					l_p->L_data = 1;
					VERBOSE((Vflag|vflag), G_p->g_nam_p);
				} else if ((Ofile = openout()) < 0) {
					proc_file = F_SKIP;
				} else {
					if (Hdr_type == CRC)
						G_p->g_cksum = keepsum;
					data_in(P_PROC);
					l_p->L_data = 1;
				}
			} /* (proc_file = ckname()) != F_SKIP */
			tl_p = tl_p->L_lnk_p;
			if (proc_file == F_SKIP && !cleanup) {
				tl_p->L_nxt_p = l_p->L_nxt_p;
				tl_p->L_bck_p = l_p->L_bck_p;
				l_p->L_bck_p->L_nxt_p = tl_p;
				l_p->L_nxt_p->L_bck_p = tl_p;
				free(l_p->L_gen.g_nam_p);
				free(l_p);
			}
		} /* tl_p->L_lnk_p != (struct Lnk *)NULL */
		if (l_p->L_data == 0) {
			data_in(P_SKIP);
		}
	}
	if (cleanup)
		reclaim(l_p);
}

/*
 * Procedure:     file_out
 *
 * Restrictions:  none
 */

/*
 * file_out:  If the current file is not a special file (!Aspec) and it
 * is identical to the archive, skip it (do not archive the archive if it
 * is a regular file).  If creating a TARTYP (TAR or USTAR) archive, the first time
 * a link to a file is encountered, write the header and file out normally.
 * Subsequent links to this file put this file name in their t_linkname field.
 * Otherwise, links are handled in one of two ways, for the old headers
 * (i.e. binary and -c), linked files are written out as they are encountered.
 * For the new headers (ASC and CRC), links are saved up until all the links
 * to each file are found.  For a file with n links, write n - 1 headers with
 * g_filesz set to 0, write the final (nth) header with the correct g_filesz
 * value and write the data for the file to the archive.
 */

static
void
file_out()
{
	register struct Lnk *l_p, *tl_p;
	register int cleanup = 0;
	struct Lnk *ttl_p;

	G_p = &Gen;
	if (!Aspec && IDENT(SrcSt, ArchSt))
		return; /* do not archive the archive if it's a regular file */
	if (Hdr_type == USTAR || Hdr_type == TAR) { /* TAR and USTAR */
		if (Adir) {
			write_hdr();
			VERBOSE((Vflag|vflag), G_p->g_nam_p);
			return;
		}
		if (G_p->g_nlink == 1) {
			data_out();
			return;
		}
		tl_p = add_lnk(&ttl_p);
		l_p = ttl_p;
		if (tl_p == l_p) { /* first link to this file encountered */
			data_out();
			return;
		} else {
			G_p->g_typeflag = LNKTYPE;
			G_p->g_filesz = 0;
		}
		CLOSE_Ifile;

		/* g_namesz include null terminator */
		if (l_p->L_gen.g_namesz > NAMSIZ + 1) {
			msg(ER, ":1256", "\"%s\" linked to \"%s\": linked name too long",
			    G_p->g_nam_p, l_p->L_gen.g_nam_p);
		} else {
			(void)strcpy(T_lname, l_p->L_gen.g_nam_p);
			write_hdr();
			VERBOSE((Vflag|vflag), tl_p->L_gen.g_nam_p);
		}
		free(tl_p->L_gen.g_nam_p);
		free(tl_p);
		return;
	}
	if (Adir) {
		write_hdr();
		VERBOSE((Vflag|vflag), G_p->g_nam_p);
		return;
	}
	if (G_p->g_nlink == 1) {
		data_out();
		return;
	} else {
		tl_p = add_lnk(&ttl_p);
		l_p = ttl_p;
		if (l_p->L_cnt == l_p->L_gen.g_nlink)
			cleanup = 1;
		else if (Onecopy) {
			CLOSE_Ifile;
			return; /* don't process data yet */
		}
	}
	if (Onecopy) {
		tl_p = l_p;
		while (tl_p->L_lnk_p != (struct Lnk *)NULL) {
			G_p = &tl_p->L_gen;
			G_p->g_filesz = 0L;
			write_hdr();
			VERBOSE((Vflag|vflag), G_p->g_nam_p);
			tl_p = tl_p->L_lnk_p;
		}
		G_p = &tl_p->L_gen;
	}
	data_out();
	if (cleanup)
		reclaim(l_p);
}

/*
 * Procedure:     file_pass
 *
 * Restrictions:  
 *		  readlink(2):	none
 *		  symlink(2):	none
 *		  lvlfile(2):	none
 */

/*
 * file_pass:  If the -l option is set (link files when possible), and the 
 * source and destination file systems are the same, link the source file 
 * (G_p->g_nam_p) to the destination file (Fullnam_p) and return.  If not a 
 * linked file, transfer the data.  Otherwise, the first link to a file 
 * encountered is transferred normally and subsequent links are linked to it.
 */

static
void
file_pass()
{
	register struct Lnk *l_p, *tl_p;
	struct Lnk *ttl_p;
	char *save_name;
	int lvl_change = 0; /* indicate whether level of process is changed */
	int result;

	G_p = &Gen;

	if (Adir && !(Args & OCd)) {
		msg(ER, ":104", "Use -d option to copy \"%s\"", G_p->g_nam_p);
		return;
	}

	save_name = G_p->g_nam_p;
	while (*(G_p->g_nam_p) == '/')
		G_p->g_nam_p++;
	(void)strcpy(Full_p, G_p->g_nam_p);

	if ((Args & OCl) && !Adir && creat_lnk(save_name, Fullnam_p) == 0) {
		CLOSE_Ifile;
		return;
	}

	if ((G_p->g_mode & Ftype) == S_IFLNK && !(Args & OCL)) {
		errno = 0;
		if (readlink(save_name, Symlnk_p, G_p->g_filesz) < 0) {
			msg(ERN, badreadsymid, badreadsym, save_name);
			return;
		}
		errno = 0;
		(void)missdir(Fullnam_p);
		*(Symlnk_p + G_p->g_filesz) = '\0';

		/* change level of process to that of the source file */
		/* -p option already checked before entry to file_pass() */
		if (macpkg && (Args  & OCm)) {
			lvl_change = chg_lvl_proc(save_name);
			if (Mldmode_chg) {
				save_name = Fullnam_p;
			}
		}

		result = symlink(Symlnk_p, Fullnam_p);

		/* restore level of process */
		if (lvl_change) {
			if (Mldmode_chg) {
				restore_lvl_proc();
				save_name = Fullnam_p;
			} else {
				restore_lvl_proc();
			}
		}

		if (result < 0) {
			if (errno == EEXIST) {
				if (openout() < 0) {
					return;
				}
			} else {
				msg(ERN, badcreateid, badcreate, Fullnam_p);
				return;
			}
		}

		rstfiles(U_OVER);
		VERBOSE((Vflag|vflag), Fullnam_p);
		return;
	}


	if (!Adir && G_p->g_nlink > 1) {
		tl_p = add_lnk(&ttl_p);
		l_p = ttl_p;
		if (tl_p == l_p) /* was not found */
			G_p = &tl_p->L_gen;
		else { /* found */
			(void)strcpy(Lnkend_p, l_p->L_gen.g_nam_p);
			(void)strcpy(Full_p, tl_p->L_gen.g_nam_p);
			(void)creat_lnk(Lnknam_p, Fullnam_p);
			l_p->L_lnk_p = (struct Lnk *)NULL;
			free(tl_p->L_gen.g_nam_p);
			free(tl_p);
			if (l_p->L_cnt == G_p->g_nlink) 
				reclaim(l_p);
			CLOSE_Ifile;
			return;
		}
	}
	if (Adir || Aspec) {
		/*
		 * Initialize directory exists flag.  We don't
		 * know yet if the directory already exists.
		 */
		Exists_flag = 0;

		if (creat_spec() > 0)
			VERBOSE((Vflag|vflag), Fullnam_p);
	} else if ((Ofile = openout()) > 0)
		data_pass();
}

/*
 * Procedure:     flush_lnks
 *
 * Restrictions:
 *                stat(2): none
 */

/*
 * flush_lnks: With new linked file handling, linked files are not archived
 * until all links have been collected.  When the end of the list of filenames
 * to archive has been reached, all files that did not encounter all their links
 * are written out with actual (encountered) link counts.  A file with n links 
 * (that are archived) will be represented by n headers (one for each link (the
 * first n - 1 have g_filesz set to 0)) followed by the data for the file.
 */

static
void
flush_lnks()
{
	register struct Lnk *l_p, *tl_p;
	long tfsize;

	l_p = Lnk_hd.L_nxt_p;
	while (l_p != &Lnk_hd) {
		(void)strcpy(Gen.g_nam_p, l_p->L_gen.g_nam_p);
		if (stat(Gen.g_nam_p, &SrcSt) == 0) { /* check if file exists */
			tl_p = l_p;
			(void)creat_hdr();
			Gen.g_nlink = l_p->L_cnt; /* "actual" link count */
			tfsize = Gen.g_filesz;
			Gen.g_filesz = 0L;
			G_p = &Gen;
			while (tl_p != (struct Lnk *)NULL) {
				Gen.g_nam_p = tl_p->L_gen.g_nam_p;
				Gen.g_namesz = tl_p->L_gen.g_namesz;
				if (tl_p->L_lnk_p == (struct Lnk *)NULL) {
					Gen.g_filesz = tfsize;
					data_out();
					break;
				}
				write_hdr(); /* archive header only */
				VERBOSE((Vflag|vflag), Gen.g_nam_p);
				tl_p = tl_p->L_lnk_p;
			}
			Gen.g_nam_p = Nam_p;
		} else /* stat(Gen.g_nam_p, &SrcSt) == 0 */
			msg(ER, ":773", "\"%s\" has disappeared", Gen.g_nam_p);
		tl_p = l_p;
		l_p = l_p->L_nxt_p;
		reclaim(tl_p);
	} /* l_p != &Lnk_hd */
}

/*
 * Procedure:     gethdr
 *
 * Restrictions:  none
 */

/*
 * gethdr: Get a header from the archive, validate it and check for the trailer.
 * Any user specified Hdr_type is ignored (set to NONE in main).  Hdr_type is 
 * set appropriately after a valid header is found.  Unless the -k option is 
 * set a corrupted header causes an exit with an error.  I/O errors during 
 * examination of any part of the header cause gethdr to throw away any current
 * data and start over.  Other errors during examination of any part of the 
 * header cause gethdr to advance one byte and continue the examination.
 */

static
int
gethdr()
{
	register ushort ftype;
	register int hit = NONE, cnt = 0;
	int goodhdr, hsize, offset;
	char *preptr;
	int k = 0;
	int j;
	static int firstpass = 1;
	static int lasthdr = GOOD;

	Gen.g_nam_p = Nam_p;
	do { /* hit == NONE && (Args & OCk) && Buffr.b_cnt > 0 */
		WAIT_FOR_DATA(Hdrsz);
		switch (Hdr_type) {
		case NONE:
		case BIN:
			Binmag.b_byte[0] = Buffr.b_out_p[0];
			Binmag.b_byte[1] = Buffr.b_out_p[1];
			if (Binmag.b_half == CMN_BIN) {
				hit = read_hdr(BIN);
				hsize = HDRSZ + Gen.g_namesz;
				break;
			}
			else if (Binmag.b_half == CMN_BBS) {
				if (Hdr_type == NONE)
					Args |= OCs;
				hit = read_hdr(BIN);
				hsize = HDRSZ + Gen.g_namesz;
				break;
			}
			if (Hdr_type != NONE)
				break;
			/*FALLTHROUGH*/
		case CHR:
			if (!strncmp(Buffr.b_out_p, CMS_CHR, CMS_LEN)) {
				hit = read_hdr(CHR);
				hsize = CHRSZ + Gen.g_namesz;
				break;
			}
			if (Hdr_type != NONE)
				break;
			/*FALLTHROUGH*/
		case ASC:
			if (!strncmp(Buffr.b_out_p, CMS_ASC, CMS_LEN)) {
				hit = read_hdr(ASC);
				hsize = ASCSZ + Gen.g_namesz;
				break;
			}
			if (Hdr_type != NONE)
				break;
			/*FALLTHROUGH*/
		case CRC:
			if (!strncmp(Buffr.b_out_p, CMS_CRC, CMS_LEN)) {
				hit = read_hdr(CRC);
				hsize = ASCSZ + Gen.g_namesz;
				break;
			}
			if (Hdr_type != NONE)
				break;
			/*FALLTHROUGH*/
		case USTAR:
			Hdrsz = TARSZ;
			WAIT_FOR_DATA(Hdrsz);
			if ((hit = read_hdr(USTAR)) == NONE) {
				Hdrsz = ASCSZ;
				break;
			}
			hit = USTAR;
			hsize = TARSZ;
			break;
			/*FALLTHROUGH*/
		case TAR:
			Hdrsz = TARSZ;
			WAIT_FOR_DATA(Hdrsz);
			if ((hit = read_hdr(TAR)) == NONE) {
				Hdrsz = ASCSZ;
				break;
			}
			hit = TAR;
			hsize = TARSZ;
			break;
		default:
			msg(EXT, badhdrid, badhdr);
		} /* Hdr_type */
		if (hit != NONE) {
			WAIT_FOR_DATA(hsize);
			goodhdr = 1;
			if (Gen.g_filesz < 0L || Gen.g_namesz < 1)
				goodhdr = 0;
			if ((hit == USTAR) || (hit == TAR)) {
				Gen.g_nam_p = &nambuf[0];
				/*
				 * If we get a null and last header was good,
				 * then we're probably at tar trailer.
				 */
				if ((*Gen.g_nam_p == '\0') && (lasthdr == GOOD))
					goodhdr = 1;
				else {
					G_p = &Gen;
					if (G_p->g_cksum != cksum(TARTYP, 0))
						goodhdr = 0;
				}
			} else { /* binary, -c, ASC and CRC */
				if (Gen.g_nlink <= (ulong)0)
					goodhdr = 0;
				if (*(Buffr.b_out_p + hsize - 1) != '\0')
					goodhdr = 0;
			}
			if (!goodhdr) {
				hit = NONE;
				if (!(Args & OCk))
					break;
				/*
				 * If last hdr was bad, we already printed
				 * this msg, so don't print it again for each 
				 * bad byte. But if we've just read good stuff
				 * we want to know that we've hit bad again.
				 */
				if (lasthdr == GOOD) {
					msg(ER, ":107", "Corrupt header, file(s) may be lost.");
					lasthdr = BAD;
				}
			} else {
				WAIT_FOR_DATA(hsize);
			}
		} /* hit != NONE */
		if (hit == NONE) {
			Buffr.b_out_p++;
			Buffr.b_cnt--;
			if (!(Args & OCk))
				break;
			if (!cnt++)
				msg(ER, ":108", "Searching for magic number/header.");
		}
	} while (hit == NONE);
	if (hit == NONE) {
		if (Hdr_type == NONE)
			msg(EXT, ":109", "Not a cpio file, bad header.");
		else
			msg(EXT, ":110", "Bad magic number/header.");
	} else if (cnt > 0) {
		msg(EPOST, ":111", "Re-synchronized on magic number/header.");
	}
	if (Hdr_type == NONE)
		Hdr_type = hit;

	if (firstpass) {
		firstpass = 0;
		switch (Hdr_type) {
		case BIN:
			Hdrsz = HDRSZ;
			Max_namesz = CPATH;
			Pad_val = HALFWD;
			Onecopy = 0;
			break;
		case CHR:
			Hdrsz = CHRSZ;
			Max_namesz = CPATH;
			Pad_val = 0;
			Onecopy = 0;
			break;
		case ASC:
		case CRC:
			Hdrsz = ASCSZ;
			Max_namesz = APATH;
			Pad_val = FULLWD;
			Onecopy = 1;
			break;
		case USTAR:
			Hdrsz = TARSZ;
			Max_namesz = HNAMLEN - 1;
			Pad_val = FULLBK;
			Onecopy = 0;
			break;
		case TAR:
			Hdrsz = TARSZ;
			Max_namesz = TNAMLEN - 1;
			Pad_val = FULLBK;
			Onecopy = 0;
			break;
		default:
			msg(EXT, badhdrid, badhdr);
		} /* Hdr_type */
	} /* Hdr_type == NONE */
	if ((Hdr_type == USTAR) || (Hdr_type == TAR)) { /* TAR and USTAR */
		Gen.g_namesz = 0;
		if (Gen.g_nam_p[0] == '\0')
			return(0);
		else {
			preptr = &prebuf[0];
			if (*preptr != (char) NULL) {
				k = strlen(&prebuf[0]);
				if (k < PRESIZ) {
					(void)strcpy(&fullnam[0], &prebuf[0]);
					j = 0;
					fullnam[k++] = '/';
					while ((j < NAMSIZ) && (&nambuf[j] != (char) NULL)) {
						fullnam[k] = nambuf[j];
						k++; j++;
					} 
					fullnam[k] = '\0';
				} else if (k >= PRESIZ) {
					k = 0;
					while ((k < PRESIZ) && (prebuf[k] != (char) NULL)) {
						fullnam[k] = prebuf[k];
						k++;
					}
					fullnam[k++] = '/';
					j = 0;
					while ((j < NAMSIZ) && (nambuf[j] != (char) NULL)) {
						fullnam[k] = nambuf[j];
						k++; j++;
					} 
					fullnam[k] = '\0';
				}
				Gen.g_nam_p = &fullnam[0];
			} else
				Gen.g_nam_p = &nambuf[0];
		}
	} else {
		(void)memcpy(Gen.g_nam_p, Buffr.b_out_p + Hdrsz, Gen.g_namesz);
		if (!strcmp(Gen.g_nam_p, "TRAILER!!!"))
			return(0);

		/* extract the extent information from the file name */
		memset(&vx_extcpio, '\0', sizeof(vx_extcpio));
		if (Hdr_type == ASC || Hdr_type == CRC) {
			k = strlen(Gen.g_nam_p) + 1;
			if (k == Gen.g_namesz - VX_CPIONEED) {
				Gen.g_namesz = k;
				sscanf(Gen.g_nam_p + k,
					"%8lx%8lx%8lx%8lx",
					&(vx_extcpio.magic),
					&(vx_extcpio.extinfo.ext_size),
					&(vx_extcpio.extinfo.reserve),
					&(vx_extcpio.extinfo.a_flags));
				if (vx_extcpio.magic != VX_CPIOMAGIC)
					memset(&vx_extcpio, '\0', 
						sizeof(vx_extcpio));
			}
		}
	}
	offset = ((hsize + Pad_val) & ~Pad_val);
	DECR_BUFCNT(offset);
	if (Hdr_type == USTAR) {
		/*
		 * The mode read off of the archive does not necessarily have
		 * the file type bits set (not required by POSIX, XPG).
		 * Rather, the type of the file is determined from the typeflag
		 * field.  But later, we depend on the file type bits of the
		 * mode field being set correctly (for example, when we call
		 * mknod(2)).  So we set them here.
		 */
		Gen.g_mode &= ~Ftype;
		switch (Gen.g_typeflag) {
		case CHRTYPE:
			Gen.g_mode |= S_IFCHR;
			break;
		case BLKTYPE:
			Gen.g_mode |= S_IFBLK;
			break;
		case FIFOTYPE:
			Gen.g_mode |= S_IFIFO;
			break;
		case NAMTYPE:
			Gen.g_mode |= S_IFNAM;
			break;
		case DIRTYPE:
			Gen.g_mode |= S_IFDIR;
			break;
		case SYMTYPE:
			Gen.g_mode |= S_IFLNK;
			break;
		}
	}
	ftype = Gen.g_mode & Ftype;
	Adir = (ftype == S_IFDIR);
	Aspec = (ftype == S_IFBLK || ftype == S_IFCHR || ftype == S_IFIFO || ftype == S_IFNAM);
	lasthdr = GOOD;
	return(1);
}

/*
 * Procedure:     getname
 *
 * Restrictions:
 *                lstat(2):	none
 *                stat(2):	none
 */

/*
 * getname: Get file names for inclusion in the archive.  When end of file
 * on the input stream of file names is reached, flush the link buffer out.
 * For each filename, remove leading "./"s and multiple "/"s, and remove
 * any trailing newline "\n".  Finally, verify the existance of the file,
 * and call creat_hdr() to fill in the gen_hdr structure.
 */

static
int
getname()
{
	register int goodfile = 0, lastchar;
	extern int svr32lstat();
	extern int svr32stat();
	char *parent;  /* parent directory of the file */
	char *name;
	struct stat statbuf;

	Gen.g_nam_p = Nam_p;
	while (!goodfile) {
		if (fgets(Gen.g_nam_p, Max_namesz, stdin) == (char *)NULL) {
			if (Onecopy && !(Args &OCp))
				flush_lnks();
			return(0);
		}
		while (*Gen.g_nam_p == '.' && Gen.g_nam_p[1] == '/') {
			Gen.g_nam_p += 2;
			while (*Gen.g_nam_p == '/')
				Gen.g_nam_p++;
		}
		lastchar = strlen(Gen.g_nam_p) - 1;
		if (*(Gen.g_nam_p + lastchar) == '\n')
			*(Gen.g_nam_p + lastchar) = '\0';

		/* 
		 * Check if type "proc" file; if so, SILENTLY skip it.
		 * We've chosen to be silent, since one NEVER wants to
		 * backup proc files, and an ugly error message for each
		 * proc file is of no value to the user. So silently 
		 * skipping /proc files is a feature!
		 * NOTE: We're going to check for type proc on the parent
		 * directory, since conceivably a proc file could get in the
		 * namelist but then disappear and cause problems for stat().
		 * Checking the parent will allow us to easily weed out proc
		 * files, even if corresponding processes disappear on us.
		 */
		parent = dirname(strdup(Gen.g_nam_p));
		if (strcmp(parent, "/") == 0)
			/*
			 * If parent is "/", it could be /proc directory,
			 * so use the filename itself, and not the parent.
			 */
			name = Gen.g_nam_p;
		else
			name = parent;

		if (stat(name, &statbuf) == 0) {
	        	if (strcmp(statbuf.st_fstype, "proc") == 0) {
				/* Skip proc file; get next filename. */
				continue;
        		}
		}
		
		if (Oldstat) {
			if (svr32lstat(Gen.g_nam_p, &TmpSt) == 0) {
				goodfile = 1;
			} else {
				if (errno != EOVERFLOW)
					msg(ERN, badaccessid, badaccess, Gen.g_nam_p);
				else if (!(Args & OCL) || lstat(Gen.g_nam_p, &SrcSt) < 0 || (SrcSt.st_mode & Ftype) != S_IFLNK)
					msg(ERN, ":112", "Old format cannot support expanded types on %s", Gen.g_nam_p);
				else {
					goodfile = 1;
					TmpSt.st_mode = SrcSt.st_mode;
				}
			}
			if (goodfile && (TmpSt.st_mode & Ftype) == S_IFLNK && (Args & OCL)) {
				if (svr32stat(Gen.g_nam_p, &TmpSt) < 0) {
					msg(ERN, badfollowid, badfollow, Gen.g_nam_p);
					goodfile = 0;
				}
			}

			/* svr32stat and svr32lstat will *not* fail with errno
			 * EOVERFLOW in the case that the st_dev field was too
			 * large, it will just set st_dev to NODEV.  But the
			 * only reason cpio saves the st_dev field is to
			 * determine multiple links, so if st_nlink is 1, or
			 * the file is a dir, it doesn't matter.  However, if
			 * st_nlink > 1 and the file is not a dir, then not
			 * knowing the dev number makes it impossible to
			 * determine what it's linked to.  In this case, we
			 * skip the file.
			 */
			if(goodfile && TmpSt.st_dev < 0 && TmpSt.st_nlink > 1 && (TmpSt.st_mode & Ftype) != S_IFDIR) {
				msg(ER , ":112", "Old format cannot support expanded types on %s", Gen.g_nam_p);
				goodfile = 0;
			} else {
				/* 
				 * This is not a linked file if st_dev is -1, 
				 * and this value will not be used.  Just 
				 * save 0 in the header.  The -1 value will
				 * not fit in the 6 bytes of the old format.
				 */
				if (TmpSt.st_dev < 0)
					SrcSt.st_dev = 0;
				else
					SrcSt.st_dev = (dev_t)TmpSt.st_dev;
				SrcSt.st_uid = (uid_t)TmpSt.st_uid;
				SrcSt.st_gid = (gid_t)TmpSt.st_gid;
				SrcSt.st_ino = (ino_t)TmpSt.st_ino;
				SrcSt.st_mode = (mode_t)TmpSt.st_mode;
				SrcSt.st_mtime = (ulong)TmpSt.st_modtime;
				SrcSt.st_atime = (ulong)TmpSt.st_actime;
				SrcSt.st_nlink = (nlink_t)TmpSt.st_nlink;
				SrcSt.st_size = (off_t)TmpSt.st_size;
				SrcSt.st_rdev = (dev_t)TmpSt.st_rdev;
			}
		} else {
			if (!lstat(Gen.g_nam_p, &SrcSt)) {
				goodfile = 1;
				if ((SrcSt.st_mode & Ftype) == S_IFLNK && (Args & OCL)) {
					errno = 0;
					if (stat(Gen.g_nam_p, &SrcSt) < 0) {
						msg(ERN, badfollowid, badfollow, Gen.g_nam_p);
						goodfile = 0;
					}
				}
			} else
				msg(ERN, badaccessid, badaccess, Gen.g_nam_p);
		}
	}
	if (creat_hdr())
		return(1);
	else return(2);
}

/*
 * Procedure:     getpats
 *
 * Restrictions:
 *                fgets		none
 */

/*
 * getpats: Save any filenames/patterns specified as arguments.
 * Read additional filenames/patterns from the file specified by the
 * user.  The filenames/patterns must occur one per line.
 */

static
void
getpats(largc, largv)
int largc;
register char **largv;
{
	register char **t_pp;
	register int len;
	register unsigned numpat = largc, maxpat = largc + 2;
	
	if ((Pat_pp = (char **)malloc(maxpat * sizeof(char *))) == (char **)NULL)
		msg(EXT, nomemid, nomem);
	t_pp = Pat_pp;
	while (*largv) {
		if ((*t_pp = (char *)malloc((unsigned int)strlen(*largv) + 1)) == (char *)NULL)
			msg(EXT, nomemid, nomem);
		(void)strcpy(*t_pp, *largv);
		t_pp++;
		largv++;
	}
	while (fgets(Nam_p, Max_namesz, Ef_p) != (char *)NULL) {
		if (numpat == maxpat - 1) {
			maxpat += 10;
			if ((Pat_pp = (char **)realloc((char *)Pat_pp, maxpat * sizeof(char *))) == (char **)NULL)
				msg(EXT, nomemid, nomem);
			t_pp = Pat_pp + numpat;
		}
		len = strlen(Nam_p); /* includes the \n */
		*(Nam_p + len - 1) = '\0'; /* remove the \n */
		*t_pp = (char *)malloc((unsigned int)len);
		if(*t_pp == (char *) NULL)
			msg(EXT, nomemid, nomem);
		(void)strcpy(*t_pp, Nam_p);
		t_pp++;
		numpat++;
	}
	*t_pp = (char *)NULL;
}

/*
 * Procedure:     ioerror
 *
 * Restrictions:  none
 */

static
void
ioerror(dir)
register int dir;
{
	register int t_errno;
	register int archtype;	/* archive file type */

	t_errno = errno;
	errno = 0;
	if (fstat(Archive, &ArchSt) < 0)
		msg(EXTN, badaccarchid, badaccarch);
	errno = t_errno;

	archtype = (ArchSt.st_mode & Ftype);
	if ((archtype != S_IFCHR) && (archtype != S_IFBLK)) {
		if (dir) {  /* OUTPUT */
			if (errno == EFBIG)
				msg(EXT, ":113", "ulimit reached for output file.");
			else if (errno == ENOSPC)
				msg(EXT, ":114", "No space left for output file.");
			else
				msg(EXT, ":115", "I/O error - cannot continue");
		}
		else  /* INPUT */
			msg(EXT, ":116", "Unexpected end-of-file encountered.");
	}
	else if (errno == EIO && lseek(Archive, 0, SEEK_CUR) == 0)
		/*
		 * If the errno is EIO, and this is the first read/write of the
		 * file, chances are that this is an unformatted floppy.
		 */
		msg(EXT, unformatid, unformat);
	else if (dir)  /* OUTPUT */
		msg(EXT, ":907", "Cannot write to device");
	else  /* INPUT */
		msg(EXT, ":908", "Cannot read from device");
}

/*
 * Procedure:     matched
 *
 * Restrictions:  none
 */

/*
 * matched: Determine if a filename matches the specified pattern(s).  If the
 * pattern is matched (the first return), return 0 if -f was specified, else
 * return 1.  If the pattern is not matched (the second return), return 0 if
 * -f was not specified, else return 1.
 */

static
int
matched()
{
	register char *str_p = G_p->g_nam_p;
	register char **pat_pp = Pat_pp;

	while (*pat_pp) {
		if ((**pat_pp == '!' && !gmatch(str_p, *pat_pp + 1)) || gmatch(str_p, *pat_pp))
			return(!(Args & OCf)); /* matched */
		pat_pp++;
	}
	return(Args & OCf); /* not matched */
}

/*
 * Procedure:     missdir
 *
 * Restrictions:
 *                stat(2):	none
 *                mkdir(2):	none
 */

/*
 * missdir: Create missing directories for files.
 * (Possible future performance enhancement, if missdir is called, we know
 * that at least the very last directory of the path does not exist, therefore,
 * scan the path from the end) 
 */

static
int
missdir(nam_p)
register char *nam_p;
{
	register char *c_p;
	register int cnt = 2;
	char str[APATH];
	register char *tmp_p;
	int i, stflag;
	int lvl_change = 0; /* indicate whether level of process is changed */
	if (*(c_p = nam_p) == '/') /* skip over 'root slash' */
		c_p++;
	for (; *c_p; ++c_p) {
		if (*c_p == '/') {
			*c_p = '\0';
			stflag = stat(nam_p, &DesSt);
			if (Args & OCu) {
				if (stflag >= 0 ) { /* file exists */
					if ((DesSt.st_mode & Ftype) != S_IFDIR){
						remove(nam_p);
						stflag = -1;
					}
				}
			}
			if (stflag < 0) {
				if (Args & OCd) {

					/* determine source directory */
					if (Args & OCp) {
						tmp_p = Nam_p;
						i = 0;
						while ((*tmp_p) == '/') {
							str[i++] = '/';
							tmp_p++;
						}
						strcpy(str+i, Full_p);
					}
					
					if (macpkg && (Args & OCp)) {

						/* change level of process to
						   that of the source dir */
						if (Args & OCm) {
							lvl_change = chg_lvl_proc(str);
							if (Mldmode_chg) {
								nam_p = Fullnam_p;
							}
						}

						/* create an MLD if source dir is an MLD */
						cnt = creat_mld(str, nam_p, Def_mode);
					} else /* create a regular dir */
						cnt = 1;

					/* a regular dir is to be created */
					if (cnt == 1) {  
						cnt = mkdir(nam_p, Def_mode);
						if ((cnt == -1) && 
							(errno == EEXIST) &&
							Target_mld) {
							/* 
							 * directory exists
							 * we stat'ed the wrong
							 * effective dir above.
							 */
							cnt = 0;
						}
					}

					/* restore level of process */
					if (lvl_change) {
						if (Mldmode_chg) {
							restore_lvl_proc();
							nam_p = Fullnam_p;
						} else {
							restore_lvl_proc();
						}
					}

					if (cnt != 0) {
						*c_p = '/';
						return(cnt);
					}
				} else {
					msg(ER, ":119", "Missing -d option.");
					*c_p = '/';
					return(-1);
				}

			} /* end if (stflag < 0) */
			*c_p = '/';
		} /* end if (*c_p == '/') */
	} /* end for */
	if (cnt == 2) /* the file already exists */
		cnt = 0;
	return(cnt);
}

/*
 * Procedure:     mklong
 *
 * Restrictions:  none
 */

/*
 * mklong: Convert two shorts into one long.  For VAX, Interdata ...
 */

static
long
mklong(v)
register short v[];
{
	
	register union swpbuf swp_b;

	swp_b.s_word = 1;
	if (swp_b.s_byte[0]) {
		swp_b.s_half[0] = v[1];
		swp_b.s_half[1] = v[0];
	} else {
		swp_b.s_half[0] = v[0];
		swp_b.s_half[1] = v[1];
	}
	return(swp_b.s_word);
}

/*
 * Procedure:     mkshort
 *
 * Restrictions:  none
 */

/*
 * mkshort: Convert a long into 2 shorts, for VAX, Interdata ...
 */

static
void
mkshort(sval, v)
register short sval[];
register long v;
{
	register union swpbuf *swp_p, swp_b;

	swp_p = (union swpbuf *)sval;
	swp_b.s_word = 1;
	if (swp_b.s_byte[0]) {
		swp_b.s_word = v;
		swp_p->s_half[0] = swp_b.s_half[1];
		swp_p->s_half[1] = swp_b.s_half[0];
	} else {
		swp_b.s_word = v;
		swp_p->s_half[0] = swp_b.s_half[0];
		swp_p->s_half[1] = swp_b.s_half[1];
	}
}

/*
 * Procedure:     msg
 *
 * Restrictions:
 *                fputc		none
 *                fflush	none
 *                pfmt		none
 *                vfprintf	none
 *                gettxt	none
 *                fprintf	none
 */

/*
 * msg: Print either a message (no error) (POST), an error message with or 
 * without the errno (ERN or ER), print an error message with or without
 * the errno and exit (EXTN or EXT), or print an error message with the
 * usage message and exit (USAGE).
 */

void
/*VARARGS*/
#ifdef __STDC__
msg(const int severity, const char *fmt_pid, const char *fmt_p, ...)
#else
msg(va_alist)
va_dcl
#endif
{
#ifndef __STDC__
	register char *fmt_p, *fmt_pid;
	register int severity;
#endif
	register FILE *file_p;
	va_list v_Args;
	int save_errno;

	save_errno = errno;
	if (Vflag && Verbcnt) { /* clear current line of dots */
		(void)fputc('\n', Out_p);
		Verbcnt = 0;
	}
#ifdef __STDC__
	va_start(v_Args, fmt_p);
#else
	va_start(v_Args);
	severity = va_arg(v_Args, int);
	fmt_pid = va_arg(v_Args, char *);
	fmt_p = va_arg(v_Args, char *);
#endif
	if (severity == POST)
		file_p = Out_p;
	else
		if (severity == EPOST || severity == WARN || severity == WARNN)
			file_p = Err_p;
		else {
			file_p = Err_p;
			if (!Dflag) {
				/* Protect access to Error_cnt with mutex. */
				MUTEX_LOCK(&Buffr.b_mutex);
				Error_cnt++;
				MUTEX_UNLOCK(&Buffr.b_mutex);
			} else
				Error_cnt++;
		}
	(void)fflush(Out_p);
	(void)fflush(Err_p);

        switch (severity) {
        case EXT:
        case EXTN:
                (void)pfmt(file_p, MM_HALT|MM_NOGET, "");
                break;
        case ER:
        case ERN:
	case USAGE:
                (void)pfmt(file_p, MM_ERROR|MM_NOGET, "");
                break;
        case WARN:
        case WARNN:
                (void)pfmt(file_p, MM_WARNING|MM_NOGET, "");
                break;
        case POST:
        case EPOST:
        default:
                break;
        }

	(void)vfprintf(file_p, gettxt(fmt_pid, fmt_p), v_Args);

	if (severity == ERN || severity == EXTN || severity == WARNN) {
		(void)pfmt(file_p, MM_NOSTD|MM_NOGET, ":  %s\n", strerror(save_errno));
	} else
		(void)fprintf(file_p, "\n");

	(void)fflush(file_p);
	va_end(v_Args);

	if (severity == EXT || severity == EXTN) {
		if (!Dflag)
			/*
			 * Protect access to Error_cnt with mutex.  Releasing
			 * mutex before exiting would be possible, but is not
			 * necessary, so we skip it.
			 */
			MUTEX_LOCK(&Buffr.b_mutex);
		if (Error_cnt == 1)
			(void)pfmt(file_p, MM_NOSTD, ":121:1 error\n");
		else
			(void)pfmt(file_p, MM_NOSTD, ":122:%d errors\n", Error_cnt);
		exit(EXIT_FATAL);
	}

	if (severity == USAGE)
		usage();
}

/*
 * Procedure:     openout
 *
 * Restrictions:
 *                lstat(2):	none
 *                symlink(2):	none
 *                unlink(2):	none
 *                creat(2):	none
 */

/*
 * openout: Open files for output and set all necessary information.
 * If the u option is set (unconditionally overwrite existing files),
 * and the current file exists, get a temporary file name from mktemp(3C),
 * link the temporary file to the existing file, and remove the existing file.
 * Finally either creat(2), mkdir(2) or mknod(2) as appropriate.
 * 
 */

static
int
openout()
{
	register char *nam_p;
	register int cnt, result;
	int lvl_change = 0; /* indicate whether level of process is changed */

	if (Args & OCp)
		nam_p = Fullnam_p;
	else
		nam_p = G_p->g_nam_p;
	if (Max_filesz < ((G_p->g_filesz + 0x1ff) >> 9)) { /* / 512 */
		msg(ER, ":123", "Skipping \"%s\": exceeds ulimit by %d bytes",
			nam_p, G_p->g_filesz - (Max_filesz << 9)); /* * 512 */
		return(-1);
	}
	if (!lstat(nam_p, &DesSt) && creat_tmp(nam_p) < 0)
		return(-1);
	cnt = 0;

	do {
		errno = 0;
		if ((G_p->g_mode & Ftype) == S_IFLNK) {
			if ((!(Args & OCp)) && !(Hdr_type == USTAR)) {
				WAIT_FOR_DATA(G_p->g_filesz);
				(void)strncpy(Symlnk_p, Buffr.b_out_p, G_p->g_filesz);
				*(Symlnk_p + G_p->g_filesz) = '\0';
			} else if ((!(Args & OCp)) && (Hdr_type == USTAR)) {
				(void)strcpy(Symlnk_p, T_lname);
			}

			/* change level of process to that of the source file */
			if (macpkg && (Args & OCp) && (Args  & OCm)) {
				lvl_change = chg_lvl_proc(Nam_p);
				if (Mldmode_chg) {
					/*
					 * If the target directory is an MLD,
					 * we may have modified the Full path
					 * name by inserting the effective
					 * directory.
					 */
					nam_p = Fullnam_p;
				}
			}

			result = symlink(Symlnk_p, nam_p);

			/* restore level of process */
			if (lvl_change) {
				if (Mldmode_chg) {
					restore_lvl_proc();
					nam_p = Fullnam_p;
				} else {
					restore_lvl_proc();
				}
			}

			if (result >= 0) {
				cnt = 0;
				if (*Over_p != '\0') {
					(void)unlink(Over_p);
					*Over_p = '\0';
				}
				break;
			}
		} else {
			/* change level of process to that of the source file */
			if (macpkg && (Args & OCp) && (Args  & OCm)) {
				lvl_change = chg_lvl_proc(Nam_p);
				if (Mldmode_chg) {
					/*
					 * If the target directory is an MLD,
					 * we may have modified the Full path
					 * name by inserting the effective
					 * directory.
					 */
					nam_p = Fullnam_p;
				}
			}

			result = creat(nam_p, (int)G_p->g_mode);

			/* restore level of process */
			if (lvl_change) {
				if (Mldmode_chg) {
					restore_lvl_proc();
					nam_p = Fullnam_p;
				} else {
					restore_lvl_proc();
				}
			}

			if (result >= 0) {
				cnt = 0;
				break;
			}
		}
		cnt++;
	} while (cnt < 2 && !missdir(nam_p));

	switch (cnt) {
	case 0:
		break;
	case 1:
		msg(ERN, badcreatdirid, badcreatdir, nam_p);
		break;
	case 2:
		msg(ERN, badcreateid, badcreate, nam_p);
		break;
	default:
		msg(EXT, badcaseid, badcase);
	}
	if (!Dflag) MUTEX_LOCK(&Sig_mutex);
	Finished = 0;
	if (!Dflag) MUTEX_UNLOCK(&Sig_mutex);
	return(result);
}

/*
 * Procedure:     read_hdr
 *
 * Restrictions:
 *                sscanf	none
 *                makedev	none
 */

/*
 * read_hdr: Transfer headers from the selected format 
 * in the archive I/O buffer to the generic structure.
 */

static
int
read_hdr(hdr)
int hdr;
{
	register int rv = NONE;
	major_t maj, rmaj;
	minor_t min, rmin;
	char tmpnull;

	if (Buffr.b_end_p != (Buffr.b_out_p + Hdrsz)) {
        	tmpnull = *(Buffr.b_out_p + Hdrsz);
        	*(Buffr.b_out_p + Hdrsz) = '\0';
	}

	switch (hdr) {
	case BIN:
		(void)memcpy(&Hdr, Buffr.b_out_p, HDRSZ);
		if (Hdr.h_magic == (short)CMN_BBS) {
                        swap((char *)&Hdr,HDRSZ);
		}
		Gen.g_magic = Hdr.h_magic;
		Gen.g_mode = Hdr.h_mode;
		Gen.g_uid = Hdr.h_uid;
		Gen.g_gid = Hdr.h_gid;
		Gen.g_nlink = Hdr.h_nlink;
		Gen.g_mtime = mklong(Hdr.h_mtime);
		Gen.g_ino = Hdr.h_ino;
		maj = cpioMAJOR(Hdr.h_dev);
		rmaj = cpioMAJOR(Hdr.h_rdev);
		min = cpioMINOR(Hdr.h_dev);
		rmin = cpioMINOR(Hdr.h_rdev);
		Gen.g_dev = makedev(maj, min);
		Gen.g_rdev = makedev(rmaj,rmin);
		Gen.g_cksum = 0L;
		Gen.g_filesz = mklong(Hdr.h_filesize);
		Gen.g_namesz = Hdr.h_namesize;
		rv = BIN;
		break;
	case CHR:
		if (sscanf(Buffr.b_out_p, "%6lo%6lo%6lo%6lo%6lo%6lo%6lo%6lo%11lo%6o%11lo",
		&Gen.g_magic, &Gen.g_dev, &Gen.g_ino, &Gen.g_mode, &Gen.g_uid, &Gen.g_gid,
		&Gen.g_nlink, &Gen.g_rdev, &Gen.g_mtime, &Gen.g_namesz, &Gen.g_filesz) == CHR_CNT)
			rv = CHR;
			maj = cpioMAJOR(Gen.g_dev);
			rmaj = cpioMAJOR(Gen.g_rdev);
			min = cpioMINOR(Gen.g_dev);
			rmin = cpioMINOR(Gen.g_rdev);
			Gen.g_dev = makedev(maj, min);
			Gen.g_rdev = makedev(rmaj,rmin);
		break;
	case ASC:
	case CRC:
		if (sscanf(Buffr.b_out_p, "%6lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8x%8x%8x%8x%8x%8lx",
		&Gen.g_magic, &Gen.g_ino, &Gen.g_mode, &Gen.g_uid, &Gen.g_gid, &Gen.g_nlink, &Gen.g_mtime,
		&Gen.g_filesz, &maj, &min, &rmaj, &rmin, &Gen.g_namesz, &Gen.g_cksum) == ASC_CNT) {
			Gen.g_dev = makedev(maj, min);
			Gen.g_rdev = makedev(rmaj, rmin);
			rv = hdr;
		}
		break;
	case USTAR: /* TAR and USTAR */
		if (*Buffr.b_out_p == '\0') {
			*Gen.g_nam_p = '\0';
			nambuf[0] = '\0';
			/* 
			 * If Hdr_type is NONE, we have not found
			 * a valid header yet, so return NONE.
			 * If we have found a previous tar header,
			 * then we're probably at the trailer.
			 */
			rv = Hdr_type;
		 } else {
			Thdr_p = (union tblock *)Buffr.b_out_p;
			Gen.g_nam_p[0] = '\0';
			(void)strncpy(nambuf, Thdr_p->tbuf.t_name, NAMSIZ);
			(void)sscanf(Thdr_p->tbuf.t_mode, "%8lo", &Gen.g_mode);
			(void)sscanf(Thdr_p->tbuf.t_uid, "%8lo", &Gen.g_uid);
			(void)sscanf(Thdr_p->tbuf.t_gid, "%8lo", &Gen.g_gid);
			(void)sscanf(Thdr_p->tbuf.t_size, "%12lo", &Gen.g_filesz);
			(void)sscanf(Thdr_p->tbuf.t_mtime, "%12lo", &Gen.g_mtime);
			(void)sscanf(Thdr_p->tbuf.t_cksum, "%8lo", &Gen.g_cksum);
			if (Thdr_p->tbuf.t_linkname[0] != (char)NULL) {
				/*
				 * If the size of the link name is exactly the
				 * maximum, it will not be null terminated in
				 * the header, so we copy it into a buffer with
				 * a null terminator.  This buffer will be used
				 * whenever the link name is needed.
				 */
				strncpy(T_lname, Thdr_p->tbuf.t_linkname, NAMSIZ);
				T_lname[NAMSIZ] = '\0';
				Gen.g_nlink = 1;
			} else
				Gen.g_nlink = 0;
			if (Thdr_p->tbuf.t_typeflag == SYMTYPE)
				Gen.g_nlink = 2;
			Gen.g_typeflag = Thdr_p->tbuf.t_typeflag;
			(void)sscanf(Thdr_p->tbuf.t_magic, "%6lo", &Gen.g_tmagic);
			(void)sscanf(Thdr_p->tbuf.t_version, "%2lo", &Gen.g_version);
			(void)sscanf(Thdr_p->tbuf.t_uname, "%32s", &Gen.g_uname);
			(void)sscanf(Thdr_p->tbuf.t_gname, "%32s", &Gen.g_gname);
			(void)sscanf(Thdr_p->tbuf.t_devmajor, "%8lo", &rmaj);
			(void)sscanf(Thdr_p->tbuf.t_devminor, "%8lo", &rmin);
			(void)strncpy(prebuf, Thdr_p->tbuf.t_prefix, PRESIZ);
			Gen.g_namesz = strlen(Gen.g_nam_p) + 1;
			Gen.g_dev = 0;  /* not used for ustar header */
			Gen.g_rdev = makedev(rmaj,rmin);
			rv = USTAR;
		}
		break;
	case TAR:
		if (*Buffr.b_out_p == '\0') {
			*Gen.g_nam_p = '\0';
			/* 
			 * If Hdr_type is NONE, we have not found
			 * a valid header yet, so return NONE.
			 * If we have found a previous tar header,
			 * then we're probably at the trailer.
			 */
			rv = Hdr_type;
		}
		else {
			Thdr_p = (union tblock *)Buffr.b_out_p;
			Gen.g_nam_p[0] = '\0';
			(void)sscanf(Thdr_p->tbuf.t_mode, "%lo", &Gen.g_mode);
			(void)sscanf(Thdr_p->tbuf.t_uid, "%lo", &Gen.g_uid);
			(void)sscanf(Thdr_p->tbuf.t_gid, "%lo", &Gen.g_gid);
			(void)sscanf(Thdr_p->tbuf.t_size, "%lo", &Gen.g_filesz);
			(void)sscanf(Thdr_p->tbuf.t_mtime, "%lo", &Gen.g_mtime);
			(void)sscanf(Thdr_p->tbuf.t_cksum, "%lo", &Gen.g_cksum);
			if (Thdr_p->tbuf.t_typeflag == LNKTYPE)
				Gen.g_nlink = 1;
			else
				Gen.g_nlink = 0;
			(void)sscanf(Thdr_p->tbuf.t_name, "%s", Gen.g_nam_p);
			Gen.g_namesz = strlen(Gen.g_nam_p) + 1;
			rv = TAR;
		}
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	}
	if (Buffr.b_end_p != (Buffr.b_out_p + Hdrsz))
		*(Buffr.b_out_p + Hdrsz) = tmpnull;
	return(rv);
}

/*
 * Procedure:     reclaim
 *
 * Restrictions:  none
 */

/*
 * reclaim: Reclaim linked file structure storage.
 */

static
void
reclaim(l_p)
register struct Lnk *l_p;
{
	register struct Lnk *tl_p;
	
	l_p->L_bck_p->L_nxt_p = l_p->L_nxt_p;
	l_p->L_nxt_p->L_bck_p = l_p->L_bck_p;
	while (l_p != (struct Lnk *)NULL) {
		tl_p = l_p->L_lnk_p;
		free(l_p->L_gen.g_nam_p);
		free(l_p);
		l_p = tl_p;
	}
}

/*
 * Procedure:     setpasswd
 *
 * Restrictions:
 *                getpwnam:	none
 *                getgrnam:	P_MACREAD
 */

static
void
setpasswd(nam)
char *nam;
{
	if ((dpasswd = getpwnam(&Gen.g_uname[0])) == (struct passwd *)NULL) {
		msg(WARN, badpasswdid, badpasswd, &Gen.g_uname[0]);
		msg(WARN, ":125", "%s: owner not changed", nam);
		Gen.g_uid = Uid;  /* uid of invoker (got it in main) */
	} else
		Gen.g_uid = dpasswd->pw_uid;

	/*
	 * restrict the use of P_MACREAD privilege for opening 
	 * /etc/group in getgrnam()
	 */
	procprivl (CLRPRV, MACREAD_W, 0);
	if ((dgroup = getgrnam(&Gen.g_gname[0])) == (struct group *)NULL) {
		msg(WARN, badgroupid, badgroup, &Gen.g_gname[0]);
		msg(WARN, ":126", "%s: group not changed", nam);
		Gen.g_gid = getgid(); /* gid of invoker */
	} else
		Gen.g_gid = dgroup->gr_gid;
	G_p = &Gen;
	procprivl (SETPRV, MACREAD_W, 0);
}

/*
 * Procedure:     rstfiles
 *
 * Restrictions:
 *                unlink(2):	none
 *                link(2):	none
 *                chmod(2):	none
 *                chown(2):	none
 *		  lchown(2):	none
 */

/*
 * rstfiles:  Perform final changes to the file.  If the -u option is set,
 * and overwrite == U_OVER, remove the temporary file, else if overwrite
 * == U_KEEP, unlink the current file, and restore the existing version
 * of the file.  In addition, where appropriate, set the access or modification
 * times, change the owner and change the modes of the file.
 */

static
void
rstfiles(over)
register int over;
{
	register char *inam_p, *onam_p, *nam_p;
	int	lvl_change = 0;

	/*
	 * If target MLD, we must change level of process to
	 * what it was when file was created, so that we'll
	 * deflect at the right effective directory while we
	 * restore attributes of the file.
	 */
	if (Target_mld && (Eff_lvl != Proc_lvl)) {
		if (lvlproc(MAC_SET, &Eff_lvl) == -1) {
			msg(WARNN, ":906", 
			    "Cannot restore attributes of \"%s\"", Fullnam_p);
			return;
		}
		lvl_change = 1;
		Level_chg = 1;
	} else if (macpkg && (Args & OCp) && !Target_mld && 
			(File_lvl != Proc_lvl)) {
		if (lvlproc(MAC_SET, &File_lvl) != -1) {
			lvl_change = 1;
			Level_chg = 1;
		}
	}
			
	if (Args & OCp)
		nam_p = Fullnam_p;
	else
		if (Gen.g_nlink > (ulong)0) 
			nam_p = G_p->g_nam_p;
		else
			nam_p = Gen.g_nam_p;

	if ((Args & OCi) && (privileged) && (Hdr_type == USTAR))
		setpasswd(nam_p);

	if (over == U_KEEP && *Over_p != '\0') {
		msg(POST, ":127", "Restoring existing \"%s\"", nam_p);
		(void)unlink(nam_p);
		if (link(Over_p, nam_p) == -1)
			msg(EXTN, badorigid, badorig, nam_p);
		if (remove(Over_p) == -1) {
			if (errno == EEXIST)
				/* If EEXIST, it's a non-empty directory. */
				msg(WARN, badremtmpdirid, badremtmpdir, Over_p);
			else
				msg(WARNN, badremtmpid, badremtmp, Over_p);
		}
		*Over_p = '\0';
		if (lvl_change) {
			restore_lvl_proc();
		}
		return;
	} else if (over == U_OVER && *Over_p != '\0') {
		if (remove(Over_p) == -1) {
			if (errno == EEXIST)
				/* If EEXIST, it's a non-empty directory, keep renamed dir */
				msg(WARN, namclashdirid, namclashdir, nam_p, Over_p);
			else
				msg(WARNN, badremtmpid, badremtmp, Over_p);
		}
		*Over_p = '\0';
	}

	if (Args & OCp) {
		inam_p = Nam_p;
		onam_p = Fullnam_p;
			
	} else /* OCi only uses onam_p, OCo only uses inam_p */
		inam_p = onam_p = G_p->g_nam_p;

	/* 
	 * For symbolic link, the only attributes to restore are
	 * owner and group.  (This used to be done in openout().)
	 */
	if (!(Args & OCo) && ((G_p->g_mode & Ftype) == S_IFLNK)) {
		if ((privileged) && (lchown(onam_p, G_p->g_uid, G_p->g_gid) < 0))
			msg(ERN, badchownid, badchown, nam_p);
		if (lvl_change) {
			restore_lvl_proc();
		}
		return;
	}

	/*
	 * For the case when a directory already existed (either it
	 * it previously existed, or it was created earlier because of
	 * -depth), we need to transfer acl info (-p only) and change 
	 * the mode.
	 */
	if (!(Args & OCo) && Adir && Exists_flag) {
		if (Args & OCp)
			transferdac(inam_p, onam_p, G_p->g_mode);
		Dir_mode = G_p->g_mode & ~Orig_umask;
		chmod(onam_p, Dir_mode);
		Exists_flag = 0;
	}

	if ((Args & OCm) && !Adir)
		set_tym(onam_p, G_p->g_mtime, G_p->g_mtime);
	if (!(Args & OCo) && (privileged) && (chmod(onam_p, (int)G_p->g_mode) < 0))
		msg(ERN, badchmodid, badchmod, onam_p);
	if (Args & OCa)
		set_tym(inam_p, SrcSt.st_atime, SrcSt.st_mtime);
	if ((Args & OCR) && (chown(onam_p, Rpw_p->pw_uid, Rpw_p->pw_gid) < 0))
		msg(ERN, badchownid, badchown, onam_p);
	if ((Args & OCp) && !(Args & OCR) && (privileged)) {
		if ((chown(onam_p, G_p->g_uid, G_p->g_gid)) < 0)
			msg(ERN, badchownid, badchown, onam_p);
	} else { /* OCi only uses onam_p, OCo only uses inam_p */
		if (!(Args & OCR)) {
			if ((Args & OCi) && (privileged) && (chown(inam_p, G_p->g_uid, G_p->g_gid) < 0))
				msg(ERN, badchownid, badchown, onam_p);
		}
	}
	if (lvl_change) {
		restore_lvl_proc();
	}
}

/*
 * Procedure:     scan4trail
 *
 * Restrictions:
 *                g_read:	none
 */


/*
 * scan4trail: Scan the archive looking for the trailer.
 * When found, back the archive up over the trailer and overwrite 
 * the trailer with the files to be added to the archive.
 */

static
void
scan4trail()
{
	register int bytes_read;
	register long off1, off2;

	Append = 1;
#ifdef _REENTRANT
	if (!Dflag) {
		switch (thr_create(NULL, 0, bfill_thread, NULL, THR_INCR_CONC, NULL)) {
		case 0:
			break;
		case ENOMEM:
			msg(EXTN, nomemid, nomem);
			break;
		case EAGAIN:
		default:
			msg(EXTN, badinitid, badinit);
			break;
		}
	}
#endif /* _REENTRANT */
	Hdr_type = NONE;
	G_p = (struct gen_hdr *)NULL;
	while (gethdr()) {
		G_p = &Gen;
		data_in(P_SKIP);
	}
	MUTEX_LOCK(&Buffr.b_mutex);
	Append = 2;
	MUTEX_UNLOCK(&Buffr.b_mutex);
#ifdef _REENTRANT
	if (!Dflag) {
		MUTEX_LOCK(&Buffr.b_mutex);
		if (Buffr.b_waitforput)
			(void)cond_signal(&Buffr.b_put);
		if (Eomflag) {
			Eomflag = 0;
			(void)cond_signal(&Buffr.b_endmedia);
		}
		MUTEX_UNLOCK(&Buffr.b_mutex);
		(void)thr_join(0, NULL, NULL);
	}
	/*
	 * mutex not needed for the rest of this routine, since there are no
	 * sibling threads.
	 */
#endif /* _REENTRANT */
	off1 = Buffr.b_cnt;
	off2 = Bufsize - (Buffr.b_cnt % Bufsize);
	Buffr.b_out_p = Buffr.b_in_p = Buffr.b_base_p;
	Buffr.b_cnt = 0L;
	if (lseek(Archive, -(off1 + off2), SEEK_REL) < 0)
		msg(EXTN, badappendid, badappend);
	/*
	 * If -Z option used (Zflag set), get data from the
	 * decompression process via d_read().  Else, read
	 * straight from the archive via g_read().
	 */
	if ((bytes_read = Zflag ? d_read(Buffr.b_in_p,Bufsize)
			   : g_read(Device, Archive, Buffr.b_in_p, Bufsize)) < 0)
		msg(EXTN, badappendid, badappend);
	if (lseek(Archive, -bytes_read, SEEK_REL) < 0)
		msg(EXTN, badappendid, badappend);
	Buffr.b_cnt = off2;
	Buffr.b_in_p = Buffr.b_base_p + Buffr.b_cnt;
}

/*
 * Procedure:     setup
 *
 * Restrictions:
 *                access(2):	none
 *                g_init:	none
 *                stat(2):	none
 *                ulimit(2):	none
 *		  lvlfile(2):	none
 */

/*
 * setup:  Perform setup and initialization functions.  Parse the options
 * using getopt(3C), call ckopts to check the options and initialize various
 * structures and pointers.  Specifically, for the -i option, save any
 * patterns, for the -o option, check (via stat(2)) the archive, and for
 * the -p option, validate the destination directory.
 */

static
void
setup(largc, largv)
register int largc;
register char **largv;
{
	extern int optind;
	extern char *optarg;
	register char	*opts_p = "abcde:fiklmoprstuvABC:DE:G:H:I:K:LM:O:R:STVZ6",
			*dupl_p = "Only one occurrence of -%c allowed",
			*dupl_pid = ":128";
	register int option;
	int blk_cnt;
	int orig_mldmode = MLD_QUERY;  /* Original MLD mode of process */
	int result;

	Hdr_type = BIN;
	Efil_p = Hdr_p = Own_p = IOfil_p = NULL;
	memset(&vx_extcpio, '\0', sizeof (vx_extcpio));
	vx_extcpio.magic = VX_CPIOMAGIC;
	while ((option = getopt(largc, largv, opts_p)) != EOF) {
		switch (option) {
		case 'a':	/* reset access time */
			Args |= OCa;
			break;
		case 'b':	/* swap bytes and halfwords */
			Args |= OCb;
			break;
		case 'c':	/* select character header */
			Args |= OCc;
			Hdr_type = ASC;
			Onecopy = 1;
			break;
		case 'd':	/* create directories as needed */
			Args |= OCd;
			break;
		case 'e':	/* extent info op */
			/* 
			 * We would like to have a bit defined for the
			 * -e option and set that in Args here but, 
			 * with the addition of -K and -T for version 
			 * 4, we've run out of bits.  There is no check
			 * on option conflicts for -e so we simply let
			 * it slide.     Args |= OCe; 
			 */ 
			if (!strcmp(optarg, "warn"))
				extent_op = OCe_WARN;
			else if (!strcmp(optarg, "ignore"))
				extent_op = OCe_IGNORE;
			else if (!strcmp(optarg, "force"))
				extent_op = OCe_FORCE;
			else
				msg(USAGE, ":1076", "Invalid argument \"%s\" for -%c", optarg, option);
			break;
		case 'f':	/* select files not in patterns */
			Args |= OCf;
			break;
		case 'i':	/* "copy in" */
			Args |= OCi;
			Archive = 0;
			break;
		case 'k':	/* retry after I/O errors */
			Args |= OCk;
			break;
		case 'l':	/* link files when possible */
			Args |= OCl;
			break;
		case 'm':	/* retain modification time */
			Args |= OCm;
			break;
		case 'o':	/* "copy out" */
			Args |= OCo;
			Archive = 1;
			break;
		case 'p':	/* "pass" */
			Args |= OCp;
			break;
		case 'r':	/* rename files interactively */
			Args |= OCr;
			break;
		case 's':	/* swap bytes */
			Args |= OCs;
			break;
		case 't':	/* table of contents */
			Args |= OCt;
			break;
		case 'u':	/* copy unconditionally */
			Args |= OCu;
			break;
		case 'v':	/* verbose - print file names */
			if (Vflag)  /* -V option */
				msg(USAGE, mutexid, mutex, 'v', 'V');
			else
				vflag = 1;
			break;
		case 'A':	/* append to existing archive */
			Args |= OCA;
			break;
		case 'B':	/* set block size to 5120 bytes */
			Args |= OCB;
			Bufsize = 5120;
			break;
		case 'C':	/* set arbitrary block size */
			if (Args & OCC)
				msg(USAGE, dupl_pid, dupl_p, 'C');
			else {
				Args |= OCC;
				/*
				 * Check arg for nondigit characters
				 * before converting to integer.
				 */
				if (nondigit(optarg))
					msg(USAGE, ":1076", "Invalid argument \"%s\" for -%c", optarg, option);
				else
					Bufsize = atoi(optarg);
			}
			break;
		case 'D':
			Dflag = 1;
			break;
		case 'E':	/* alternate file for pattern input */
			if (Args & OCE)
				msg(USAGE, dupl_pid, dupl_p, 'E');
			else {
				Args |= OCE;
				Efil_p = optarg;
			}
			break;
		case 'G':	/* alternate interface (other than /dev/tty) */
			if (Args & OCG)
				msg(USAGE, dupl_pid, dupl_p, 'G');
			else {
				Args |= OCG;
				Ttyname = optarg;
			}
			break;
		case 'H':	/* select header type */
			if (Args & OCH)
				msg(USAGE, dupl_pid, dupl_p, 'H');
			else {
				Args |= OCH;
				Hdr_p = optarg;
			}
			break;
		case 'I':	/* alternate file for archive input */
			if (Args & OCI)
				msg(USAGE, dupl_pid, dupl_p, 'I');
			else {
				if (Zflag == 0) Args |= OCI;
				IOfil_p = optarg;
			}
			break;

		/* Enhanced Application Compatibility Support */
		case 'K':	/* media size option */
			Args |= OCK;
			Mediasize = atoi(optarg);
			if (Mediasize == 0) {
				msg(ER, ":1076", "Invalid argument \"%s\" for -%c", optarg, option);
				exit(EXIT_USAGE);
			}
			break;
		/* End Enhanced Application Compatibility Support */

		case 'L':	/* follow symbolic links */
			Args |= OCL;
			break;
		case 'M':	/* specify new end-of-media message */
			if (Args & OCM)
				msg(USAGE, dupl_pid, dupl_p, 'M');
			else {
				Args |= OCM;
				Eom_p = optarg;
			}
			break;
		case 'O':	/* alternate file for archive output */
			if (Args & OCO)
				msg(USAGE, dupl_pid, dupl_p, 'O');
			else {
				Args |= OCO;
				IOfil_p = optarg;
			}
			break;
		case 'R':	/* change owner/group of files */
			if (Args & OCR)
				msg(USAGE, dupl_pid, dupl_p, 'R');
			else {
				Args |= OCR;
				Own_p = optarg;
			}
			break;
		case 'S':	/* swap halfwords */
			Args |= OCS;
			break;

		/* Enhanced Application Compatibility Support */
		case 'T':	/* truncate long file names to 14 chars */
			Args |= OCT;
			break;
		/* End Enhanced Application Compatibility Support */

		case 'V':	/* print a dot '.' for each file */
			if (vflag)  /* -v option */
				msg(USAGE, mutexid, mutex, 'v', 'V');
			else
				Vflag = 1;
			break;
		case 'Z':	/* read from Shared Memory */
			/*
			 * The 'Z' flag decompresses compressed cpio archives.
			 * This flag is not documented, since there is no way
			 * for users to create compressed cpio archives.
			 * Compressed cpio archives are created for
			 * installation packages by a local tool that the
			 * integration folks have.  The 'Z' flag of cpio is
			 * only used by the packaging tools to read these
			 * package archives.
			 */
			Zflag = 1;
			break;
		case '6':	/* for old, sixth-edition files */
			Args |= OC6;
			Ftype = SIXTH;
			break;
		default:
			/*
			 * No need to protect Error_cnt with mutex -- no other
			 * thread yet.
			 */
			Error_cnt++;
			usage();
		} /* End option */

	}  /* End (option = getopt(largc, largv, opts_p)) != EOF */

	largc -= optind;
	largv += optind;
	ckopts(Args);

	/* Get the page size, to use with memalign(). */
	Pagesize = sysconf(_SC_PAGESIZE);

	/* Check for memory problems. */
	if ((Buf_p = memalign(Pagesize, CPIOBSZ)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Args & OCr) && (Renam_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Symlnk_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Over_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Nam_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Fullnam_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Lnknam_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Effdir_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);
	if ((Targetmld_p = (char *)malloc(APATH)) == (char *)NULL)
		msg(EXTN, nomemid, nomem);

	*Fullnam_p = '\0';
	*Over_p = '\0';
	Gen.g_nam_p = Nam_p;

	if (Args & OCi) {
		if (largc > 0) /* save patterns for -i option, if any */
			Pat_pp = largv;
		if (Args & OCE)
			getpats(largc, largv);

	} else if (Args & OCo) {
		if (largc != 0) {  /* error if arguments left with -o */
			/*
			 * No need to protect Error_cnt with mutex -- no other
			 * thread yet.
			 */
			Error_cnt++;
			msg(USAGE, ":928", "Extra arguments at end");
		}
		else if (fstat(Archive, &ArchSt) < 0)
			msg(EXTN, badaccarchid, badaccarch);

		switch (Hdr_type) {
		case BIN:
			Hdrsz = HDRSZ;
			Pad_val = HALFWD;
			Oldstat = 1;
			break;
		case CHR:
			Hdrsz = CHRSZ;
			Pad_val = 0;
			break;
		case ASC:
		case CRC:
			Hdrsz = ASCSZ;
			Pad_val = FULLWD;
			break;
		case TAR:
		/* FALLTHROUGH */
		case USTAR: /* TAR and USTAR */
			Hdrsz = TARSZ;
			Pad_val = FULLBK;
			break;
		default:
			msg(EXT, badhdrid, badhdr);
		}

	} else { /* directory must be specified */
		if (largc != 1)
			msg(USAGE, ":1081", "Directory must be specified with -p");
		else {
			
			if (macpkg && (Args & OCp) && (Args  & OCm))
				result = chg_lvl_proc(*largv);
			if (access(*largv, 2) < 0)
				msg(EXTN, badaccessid, badaccess, *largv);
			if (result)
				restore_lvl_proc();
		}
	}
	
	if (Args & (OCi | OCo)) {
		if (!Dflag) {
			if (Args & (OCB | OCC)) {
				if (g_init(&Device, &Archive) < 0)
					msg(EXTN, badinitid, badinit);
			} else {
				if ((Bufsize = g_init(&Device, &Archive)) < 0)
					msg(EXTN, badinitid, badinit);
			}
		}

		blk_cnt = _20K / Bufsize;
		blk_cnt = (blk_cnt >= MX_BUFS) ? blk_cnt : MX_BUFS;
		while (blk_cnt > 1) {
			/*
			 * The extra CPIOBSZ bytes allocated past the "end" of
			 * the buffer are a scratch area used for wrapping
			 * partial chunks of data.  This scratch area must be
			 * as big as the largest possible request to
			 * WAIT_FOR_DATA or WAIT_FOR_SPACE.
			 */
			if ((Buffr.b_base_p = memalign(Pagesize, (Bufsize * blk_cnt) + CPIOBSZ)) != (char *)NULL) {
				Buffr.b_out_p = Buffr.b_in_p = Buffr.b_base_p;
				Buffr.b_cnt = 0L;
				Buffr.b_size = (long)(Bufsize * blk_cnt);
				Buffr.b_end_p = Buffr.b_base_p + Buffr.b_size;
				break;
			}
			blk_cnt--;
		}

		if (blk_cnt < 2 || Buffr.b_size < (2 * CPIOBSZ))
			msg(EXT, nomemid, nomem);
#ifdef _REENTRANT
		if (!(Dflag && (Args & OCi))) {
			void *handle;

			/*
			 * When the kernel detects that all lwp's of a multiple
			 * lwp process are blocked interuptibly, it sends
			 * SIGWAITING to the process, so the threads library
			 * can create more lwps if there are additional
			 * runnable threads.  But in our case, we have as many
			 * lwps as threads, so SIGWAITING will just slow us
			 * down (because the signal handler is called for
			 * nothing.)  The function _thr_sigwaiting_disable
			 * prevents the sending of SIGWAITING.
			 *
			 * This function is implementation specific, though, so
			 * if it's not found by dlsym() (because we're running
			 * with someone else's implementation of libthread),
			 * silently skip it.  The only consequence is a
			 * potential slight performance degradation
			 */
			if ((handle = dlopen(NULL, RTLD_LAZY)) != NULL &&
			    dlsym(handle, "_thr_sigwaiting_disable") != NULL)
				_thr_sigwaiting_disable();

			(void)mutex_init(&Buffr.b_mutex, USYNC_THREAD, NULL);
			(void)mutex_init(&Sig_mutex, USYNC_THREAD, NULL);
			(void)cond_init(&Buffr.b_put, USYNC_THREAD, NULL);
			(void)cond_init(&Buffr.b_take, USYNC_THREAD, NULL);

			if (Args & (OCi | OCA))
				(void)cond_init(&Buffr.b_endmedia, USYNC_THREAD, NULL);
			Init_thr_id = thr_self();
		}
#endif /* _REENTRANT */
	}

	if (Args & OCp) { /* get destination directory */
		(void)strcpy(Fullnam_p, *largv);

		/*
		 * If the mldmode of the process is virtual, change to
		 * real mode for stat() and see if target dir is an MLD.
		 * This only applies for -m, since without -m, 
		 * everything comes in at the same level and so will
		 * go in the same effective directory; no special work
		 * has to be done.
		 */
		if ((Args & OCm) && (macpkg) && ((orig_mldmode = mldmode(MLD_QUERY)) == MLD_VIRT)) {
			if (mldmode(MLD_REAL) < 0) {
                        	pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
                                	strerror(errno));
                        	exit(EXIT_FATAL);
                	}
		}
		else if (orig_mldmode < 0) {
                	pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
                	exit(EXIT_FATAL);
		}

		if (stat(Fullnam_p, &DesSt) < 0)
			msg(EXTN, badaccessid, badaccess, Fullnam_p);
		if ((DesSt.st_mode & Ftype) != S_IFDIR)
			msg(EXT, ":130", "\"%s\" is not a directory", Fullnam_p);

		/*
		 * If the original mldmode of the process was virtual, 
		 * determine if target dir is an MLD.  If so, set flag.
		 * But if we were originally in real mode, we don't 
		 * want to do anything here; no special treatment needed.  
		 */
		if (orig_mldmode == MLD_VIRT) {
			if ((DesSt.st_flags & S_ISMLD) == S_ISMLD)
				Target_mld = 1;
			/* Change back to virtual mode. */
			if (mldmode(MLD_VIRT) < 0) {
                      		pfmt(stderr, MM_ERROR|MM_NOGET, 
					"%s\n", strerror(errno));
                       		exit(EXIT_FATAL);
			}
        	}

		if (Target_mld) {
			strcpy(Targetmld_p, Fullnam_p);
		}
	}

	Full_p = Fullnam_p + strlen(Fullnam_p);
	if (Full_p != Fullnam_p && Full_p[-1] != '/') {
		*Full_p++ = '/';
		*Full_p = '\0';
	}
	(void)strcpy(Lnknam_p, Fullnam_p);

	Lnkend_p = Lnknam_p + (Full_p - Fullnam_p);
	Max_filesz = ulimit(1, 0L);
	Lnk_hd.L_nxt_p = Lnk_hd.L_bck_p = &Lnk_hd;
	Lnk_hd.L_lnk_p = (struct Lnk *)NULL;
}

/*
 * Procedure:     set_tym
 *
 * Restrictions:
 *                utime(2):	none
 */

/*
 * set_tym: Set the access and modification times for a file.
 */

static
void
set_tym(nam_p, atime, mtime)
register char *nam_p;
register time_t atime, mtime;
{
	struct utimbuf timev;

	timev.actime = atime;
	timev.modtime = mtime;
	if (utime(nam_p, &timev) < 0) {
		if (Args & OCa)
			msg(ERN, ":131", "Cannot reset access time for \"%s\"", nam_p);
		else
			msg(ERN, ":132", "Cannot reset modification time for \"%s\"", nam_p);
	}
}

/*
 * Procedure:     sigint
 *
 * Restrictions:
 *                link(2):	none
 *                unlink(2):	none
 */

/*
 * sigint:  Catch interrupts.  If the Finished flag is set, we can
 * exit immediately.  If Finished is not set, we have to work to do to
 * recover the partially restored current file.  But it may not be
 * safe to do this work here from the interrupt handler, depending on
 * what we were doing when we were interrupted, so we just set a flag
 * for now, which cause bugout() to be called shortly, where the real
 * work is done.
 */

static
void
sigint(intr)
int	intr;
{
	int gotlock = 0;

	(void)signal(SIGINT, SIG_IGN); /* block further signals */

	/*
	 * Checking the Finished flag is slightly tricky.  Not
	 * counting interrupts, thread 1 is the only one that will do
	 * anything with the flag.  So, if it is thread 2 handling the
	 * interrupt, we need to acquire the mutex to synchronize
	 * access to the flag with thread 1.  If it is thread 1
	 * handling the interrupt, we must *not* acquire the mutex,
	 * since we may have already gotten the mutex before we got
	 * the interrupt
	 */
	if (!Dflag && !I_AM_INIT_THR()) {
		MUTEX_LOCK(&Sig_mutex);
		gotlock = 1;
	}
	if (Finished)
		/*
		 * call _exit(), since exit() is not safe from signal
		 * handler
		 */ 
		_exit(EXIT_FATAL);
	Gotsigint++;
	if (gotlock) {
		MUTEX_UNLOCK(&Sig_mutex);
	}
	(void)signal(SIGINT, sigint);
}

/*
 * bugout:  Exit due to interrupt.  If an interrupt occurs
 * during the extraction of a file from the archive with the -u option
 * set, and the filename did exist, remove the current file and
 * restore the original file.  Then exit.
 */

static
void
bugout()
{
	register char *nam_p;

	if (!Finished) {
		if (Args & OCi)
			nam_p = G_p->g_nam_p;
		else /* OCp */
			nam_p = Fullnam_p;
		if (*Over_p != '\0') { /* There is a temp file */
			if (remove(nam_p) == -1) {
				if (errno == EEXIST)
					/* If EEXIST, it's a non-empty directory. */
					msg(WARN, badremincdirid, badremincdir, nam_p);
				else
					msg(WARNN, badremincid, badreminc, nam_p);
			}
			if (link(Over_p, nam_p) == -1)
				msg(ERN, badorigid, badorig, nam_p);
			if (remove(Over_p) == -1) {
				if (errno == EEXIST)
					/* If EEXIST, it's a non-empty directory. */
					msg(WARN, badremtmpdirid, badremtmpdir, Over_p);
				else
					msg(WARNN, badremtmpid, badremtmp, Over_p);
			}
		} else if (remove(nam_p) == -1) {
			if (errno == EEXIST)
				/* If EEXIST, it's a non-empty directory. */
				msg(WARN, badremincdirid, badremincdir, nam_p);
			else
				msg(WARNN, badremincid, badreminc, nam_p);
		}
	}
	exit(EXIT_FATAL);
}

/*
 * Procedure:     swap
 *
 * Restrictions:  none
 */

/*
 * swap: Swap bytes (-s), halfwords (-S) or or both halfwords and bytes (-b).
 */

static
void
swap(buf_p, cnt)
register char *buf_p;
register int cnt;
{
	register unsigned char tbyte;
	register int tcnt;
	register int rcnt;

        rcnt = cnt % 4;
	cnt /= 4;
	if (Args & (OCb | OCs)) {
		tcnt = cnt;
		Swp_p = (union swpbuf *)buf_p;
		while (tcnt-- > 0) {
			tbyte = Swp_p->s_byte[0];
			Swp_p->s_byte[0] = Swp_p->s_byte[1];
			Swp_p->s_byte[1] = tbyte;
			tbyte = Swp_p->s_byte[2];
			Swp_p->s_byte[2] = Swp_p->s_byte[3];
			Swp_p->s_byte[3] = tbyte;
			Swp_p++;
		}
                if (rcnt >= 2) {
                        tbyte = Swp_p->s_byte[0];
                        Swp_p->s_byte[0] = Swp_p->s_byte[1];
                        Swp_p->s_byte[1] = tbyte;
                        tbyte = Swp_p->s_byte[2];
		}
	}
	if (Args & (OCb | OCS)) {
		tcnt = cnt;
		Swp_p = (union swpbuf *)buf_p;
		while (tcnt-- > 0) {
			tbyte = Swp_p->s_byte[0];
			Swp_p->s_byte[0] = Swp_p->s_byte[2];
			Swp_p->s_byte[2] = tbyte;
			tbyte = Swp_p->s_byte[1];
			Swp_p->s_byte[1] = Swp_p->s_byte[3];
			Swp_p->s_byte[3] = tbyte;
			Swp_p++;
		}
	}
}

/*
 * Procedure:     usage
 *
 * Restrictions:
 *                fflush	none
 *                pfmt		none
 */

/*
 * usage: Print the usage message on stderr and exit.
 */

static
void
usage()
{
	(void)fflush(stdout);
	(void)pfmt(stderr, MM_ACTION, ":133:Usage:\n");
	(void)pfmt(stderr, MM_NOSTD, ":1118:\tcpio -i[bcdfkmrstuvBSTV6] [-C size] ");
	(void)pfmt(stderr, MM_NOSTD, ":1098:[-E file] [-G file] [-H hdr] [[-I file] [-M msg]] ");
	(void)pfmt(stderr, MM_NOSTD, ":1088:[-R id] [-e ignore|warn|force] [patterns]\n");
	(void)pfmt(stderr, MM_NOSTD, ":1099:\tcpio -o[acvABLV] [-C size] [-G file]");
	(void)pfmt(stderr, MM_NOSTD, ":1089:[-H hdr] [-e ignore|warn|force] [-K mediasize] [[-M msg] [-O file]]\n");
	(void)pfmt(stderr, MM_NOSTD, ":1090:\tcpio -p[adlmuvLV] [-e ignore|warn|force] [-R id] directory\n");
	(void)fflush(stderr);
	exit(EXIT_USAGE);
}

/*
 * Procedure:     verbose
 *
 * Restrictions:
 *                getpwuid:	none
 *                getgrgid:	P_MACREAD
 *                cftime:	P_MACREAD
 *                fputs:		none
 *                fputc:		none
 *                fflush:	none
 */

/*
 * verbose: For each file, print either the filename (-v) or a dot (-V).
 * If the -t option (table of contents) is set, print either the filename,
 * or if the -v option is also set, print an "ls -l"-like listing.
 * -v -> vflag=1
 * -V -> Vflag=1
 */

static
void
verbose(nam_p)
register char *nam_p;
{
	register int i, j, temp;
	mode_t mode;
	char modestr[11];

	if ((Args & OCt) && vflag) {
		for (i = 0; i < 10; i++)
			modestr[i] = '-';
		modestr[i] = '\0';

		mode = Gen.g_mode;
		for (i = 0; i < 3; i++) {
			temp = (mode >> (6 - (i * 3)));
			j = (i * 3) + 1;
			if (S_IROTH & temp)
				modestr[j] = 'r';
			if (S_IWOTH & temp)
				modestr[j + 1] = 'w';
			if (S_IXOTH & temp)
				modestr[j + 2] = 'x';
		}
		if (Hdr_type != USTAR && Hdr_type != TAR) {
			temp = Gen.g_mode & Ftype;
			switch (temp) {
			case (S_IFIFO):
				modestr[0] = 'p';
				break;
			case (S_IFCHR):
				modestr[0] = 'c';
				break;
			case (S_IFDIR):
				modestr[0] = 'd';
				break;
			case (S_IFBLK):
				modestr[0] = 'b';
				break;
			case (S_IFNAM):
                        	if (Gen.g_rdev == S_INSEM)  /* Xenix semaphore */
                                	modestr[0] = 's';
                        	else if (Gen.g_rdev == S_INSHD)  /* Xenix shared data */
                                	modestr[0] = 'm';
				else
					msg(ER, ":140", "Impossible file type");
                        	break;
			case (S_IFREG): /* was initialized to '-' */
				break;
			case (S_IFLNK):
				modestr[0] = 'l';
				break;
			default:
				msg(ER, ":140", "Impossible file type");
			}
		} else { /* if (Hdr_type == USTAR || Hdr_type == TAR) */
		/* 
		 * When tar creates the archive it does not maintain the
		 * the file type as part of the mode; for TAR and USTAR
		 * header types, look at the typeflag field.
		 */
			switch (Gen.g_typeflag) {
			case (FIFOTYPE):
				modestr[0] = 'p';
				break;
			case (CHRTYPE):
				modestr[0] = 'c';
				break;
			case (DIRTYPE):
				modestr[0] = 'd';
				break;
			case (BLKTYPE):
				modestr[0] = 'b';
				break;
			case (NAMTYPE):
                        	if (Gen.g_rdev == S_INSEM)  /* Xenix semaphore */
                                	modestr[0] = 's';
                        	else if (Gen.g_rdev == S_INSHD)  /* Xenix shared data */
                                	modestr[0] = 'm';
				else
					msg(ER, ":140", "Impossible file type");
                        	break;
			case (REGTYPE): /* was initialized to '-' */
			case (LNKTYPE): /* don't know what type file it was
					 * linked to, but REG is most likely */
				break;
			case (SYMTYPE):
				modestr[0] = 'l';
				break;
			default:
				msg(ER, ":140", "Impossible file type");
			}
		}

		if ((S_ISUID & Gen.g_mode) == S_ISUID)
			modestr[3] = 's';
		if ((S_ISVTX & Gen.g_mode) == S_ISVTX)
			modestr[9] = 't';
		if ((S_ISGID & G_p->g_mode) == S_ISGID && modestr[6] == 'x')
			modestr[6] = 's';
		else if ((S_ENFMT & Gen.g_mode) == S_ENFMT && modestr[6] != 'x')
			modestr[6] = 'l';
		if ((Hdr_type == USTAR || Hdr_type == TAR) && Gen.g_nlink == 0)
			(void)printf("%s%4d ", modestr, Gen.g_nlink+1);
		else
			(void)printf("%s%4d ", modestr, Gen.g_nlink);
		if (Lastuid == (int)Gen.g_uid)
			(void)printf("%-9s", Curpw_p->pw_name);
		else {
			if (Curpw_p = getpwuid((int)Gen.g_uid)) {
				(void)printf("%-9s", Curpw_p->pw_name);
				Lastuid = (int)Gen.g_uid;
			} else {
				(void)printf("%-9d", Gen.g_uid);
				Lastuid = -1;
			}
		}
		/*
		 * restrict the use of P_MACREAD privilege for opening 
		 * /etc/group in all the group related library routines.
		 */
		procprivl (CLRPRV, MACREAD_W, 0);
		if (Lastgid == (int)Gen.g_gid)
			(void)printf("%-9s", Curgr_p->gr_name);
		else {
			if (Curgr_p = getgrgid((int)Gen.g_gid)) {
				(void)printf("%-9s", Curgr_p->gr_name);
				Lastgid = (int)Gen.g_gid;
			} else {
				(void)printf("%-9d", Gen.g_gid);
				Lastgid = -1;
			}
		}
		if (!Aspec || (Gen.g_mode & Ftype) == S_IFIFO || (Gen.g_mode & Ftype) == S_IFNAM)
			(void)printf("%-7ld ", Gen.g_filesz);
		else
			(void)printf("%3d,%3d ", major(Gen.g_rdev), minor(Gen.g_rdev));
		(void)cftime(Time, gettxt(FORMATID, FORMAT), (time_t *)&Gen.g_mtime);
		procprivl (SETPRV, MACREAD_W, 0);
		(void)printf("%s, %s", Time, nam_p);
		if ((Gen.g_mode & Ftype) == S_IFLNK) {
			if (Hdr_type == USTAR)
				(void)strcpy(Symlnk_p, T_lname);
			else {
				WAIT_FOR_DATA(G_p->g_filesz);
				(void)strncpy(Symlnk_p, Buffr.b_out_p, Gen.g_filesz);
				*(Symlnk_p + Gen.g_filesz) = '\0';
			}
			(void)printf(" -> %s", Symlnk_p);
		}
		(void)printf("\n");
	} else if ((Args & OCt) || vflag) {
		(void)fputs(nam_p, Out_p);
		(void)fputc('\n', Out_p);
	} else { /* -V */
		(void)fputc('.', Out_p);
		if (Verbcnt++ >= 49) { /* start a new line of dots */
			Verbcnt = 0;
			(void)fputc('\n', Out_p);
		}
	}
	(void)fflush(Out_p);
}

/*
 * Procedure:     write_hdr
 *
 * Restrictions:
 *                sprintf	none
 */

/*
 * write_hdr: Transfer header information for the generic structure
 * into the I/O buffer in the format for the selected header.
 */

static
void
write_hdr()
{
	register int cnt, pad, len;

	switch (Hdr_type) {
	case BIN:
	case CHR:
	case ASC:
	case CRC:
		cnt = Hdrsz + G_p->g_namesz;
		break;
	case TAR:
	/*FALLTHROUGH*/
	case USTAR: /* TAR and USTAR */
		cnt = TARSZ;
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	}
	WAIT_FOR_SPACE(cnt);
	switch (Hdr_type) {
	case BIN:
		Hdr.h_magic = (short)G_p->g_magic;
		Hdr.h_dev = (short)G_p->g_dev;
		Hdr.h_ino = (ushort)G_p->g_ino;
		Hdr.h_mode = G_p->g_mode;
		Hdr.h_uid = G_p->g_uid;
		Hdr.h_gid = G_p->g_gid;
		Hdr.h_nlink = G_p->g_nlink;
		Hdr.h_rdev = (short)G_p->g_rdev;
		mkshort(Hdr.h_mtime, (long)G_p->g_mtime);
		Hdr.h_namesize = (short)G_p->g_namesz;
		mkshort(Hdr.h_filesize, (long)G_p->g_filesz);
		(void)strcpy(Hdr.h_name, G_p->g_nam_p);
		(void)memcpy(Buffr.b_in_p, &Hdr, cnt);
		break;
	case CHR:
		(void)sprintf(Buffr.b_in_p, "%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.11lo%.6lo%.11lo%s",
			G_p->g_magic, G_p->g_dev, G_p->g_ino, G_p->g_mode, G_p->g_uid, G_p->g_gid, 
			G_p->g_nlink, G_p->g_rdev, G_p->g_mtime, G_p->g_namesz, G_p->g_filesz, G_p->g_nam_p);
		break;
	case ASC:
	case CRC:
		/* save extent info after name */
		if (extent_op != OCe_IGNORE && 
		    G_p->g_namesz <= (EXPNLEN - VX_CPIONEED) &&
		    (vx_extcpio.extinfo.ext_size ||
		     vx_extcpio.extinfo.reserve ||
		     vx_extcpio.extinfo.a_flags)) {
			(void)sprintf(Buffr.b_in_p + cnt,
				"%.8lx%.8lx%.8lx%.8lx",
				vx_extcpio.magic, vx_extcpio.extinfo.ext_size,
				vx_extcpio.extinfo.reserve,
				vx_extcpio.extinfo.a_flags);
			cnt += VX_CPIONEED;
			G_p->g_namesz += VX_CPIONEED;
			memset(&vx_extcpio.extinfo, '\0', 
				sizeof (struct vx_ext));
		}
		(void)sprintf(Buffr.b_in_p, "%.6lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%s",
			G_p->g_magic, G_p->g_ino, G_p->g_mode, G_p->g_uid, G_p->g_gid, G_p->g_nlink, G_p->g_mtime,
			G_p->g_filesz, major(G_p->g_dev), minor(G_p->g_dev), major(G_p->g_rdev), minor(G_p->g_rdev),
			G_p->g_namesz, G_p->g_cksum, G_p->g_nam_p);
		break;
	case USTAR: /* USTAR */
		Thdr_p = (union tblock *)Buffr.b_in_p;
		(void)memset(Thdr_p, 0, TARSZ);
		if (strlen(G_p->g_tname) < NAMSIZ)
			(void)strcpy(Thdr_p->tbuf.t_name, G_p->g_tname);
		else
			(void)strncpy(Thdr_p->tbuf.t_name, G_p->g_tname, NAMSIZ);
		(void)sprintf(Thdr_p->tbuf.t_mode, "%07o", G_p->g_mode);
		(void)sprintf(Thdr_p->tbuf.t_uid, "%07o", G_p->g_uid);
		(void)sprintf(Thdr_p->tbuf.t_gid, "%07o", G_p->g_gid);
		(void)sprintf(Thdr_p->tbuf.t_size, "%011lo", G_p->g_filesz);
		(void)sprintf(Thdr_p->tbuf.t_mtime, "%011lo", G_p->g_mtime);
		Thdr_p->tbuf.t_typeflag = G_p->g_typeflag;
		if (T_lname[0] != '\0') {
			/*
			 * We want the terminating null copied, unless there's
			 * no room for it (length of the link name is exactly
			 * the size of the field).
			 */
			len = strlen(T_lname);
			len = len == NAMSIZ ? NAMSIZ : len + 1;
			(void)strncpy(Thdr_p->tbuf.t_linkname, T_lname, len);
		}
		(void)sprintf(Thdr_p->tbuf.t_magic, "%s", TMAGIC);
		(void)sprintf(Thdr_p->tbuf.t_version, "%2s", TVERSION);
		(void)sprintf(Thdr_p->tbuf.t_uname, "%s",  G_p->g_uname);
		(void)sprintf(Thdr_p->tbuf.t_gname, "%s", G_p->g_gname);
		if (Aspec) {
			(void)sprintf(Thdr_p->tbuf.t_devmajor, "%07o", major(G_p->g_rdev));
			(void)sprintf(Thdr_p->tbuf.t_devminor, "%07o", minor(G_p->g_rdev));
		} else {
			/* devmajor and devminor will not be used if 
			 * not a special file, so set them to 0.
			 */
			(void)strcpy(Thdr_p->tbuf.t_devmajor, "0000000");
			(void)strcpy(Thdr_p->tbuf.t_devminor, "0000000");
		}

		if (Gen.g_prefix)
			strncpy(Thdr_p->tbuf.t_prefix, Gen.g_prefix, PRESIZ);
		(void)sprintf(Thdr_p->tbuf.t_cksum, "%07o", (int)cksum(TARTYP, 0));
		break;
	case TAR:
		Thdr_p = (union tblock *)Buffr.b_in_p;
		(void)memset(Thdr_p, 0, TARSZ);
		(void)strncpy(Thdr_p->tbuf.t_name, G_p->g_nam_p, G_p->g_namesz);
		(void)sprintf(Thdr_p->tbuf.t_mode, "%07o ", G_p->g_mode);
		(void)sprintf(Thdr_p->tbuf.t_uid, "%07o ", G_p->g_uid);
		(void)sprintf(Thdr_p->tbuf.t_gid, "%07o ", G_p->g_gid);
		(void)sprintf(Thdr_p->tbuf.t_size, "%011o ", G_p->g_filesz);
		(void)sprintf(Thdr_p->tbuf.t_mtime, "%011o ", G_p->g_mtime);
		if (T_lname[0] != '\0') {
			Thdr_p->tbuf.t_typeflag = LNKTYPE;
			/*
			 * We want the terminating null copied, unless there's
			 * no room for it (length of the link name is exactly
			 * the size of the field).
			 */
			len = strlen(T_lname);
			len = len == NAMSIZ ? NAMSIZ : len + 1;
			(void)strncpy(Thdr_p->tbuf.t_linkname, T_lname, len);
		} else
			Thdr_p->tbuf.t_typeflag = AREGTYPE;
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	} /* Hdr_type */
	INCR_BUFCNT(cnt);
	pad = ((cnt + Pad_val) & ~Pad_val) - cnt;
	if (pad != 0) {
		WAIT_FOR_SPACE(pad);
		(void)memset(Buffr.b_in_p, 0, pad);
		INCR_BUFCNT(pad);
	}
}

/*
 * Procedure:     write_trail
 *
 * Restrictions:  none
 */

/*
 * write_trail: Create the appropriate trailer for the selected header type.
 * Pad the buffer with nulls out to the next Bufsize boundary.  Wait until
 * bflush (or bflush_thread) has written everything, then return.
 */

static
void
write_trail()
{
	register int cnt, tmp;

	switch (Hdr_type) {
	case BIN:
	case CHR:
	case ASC:
	case CRC:
		Gen.g_mode = Gen.g_uid = Gen.g_gid = 0;
		Gen.g_nlink = 1;
		Gen.g_mtime = Gen.g_ino = Gen.g_dev = 0;
		Gen.g_rdev = Gen.g_cksum = Gen.g_filesz = 0;
		Gen.g_namesz = strlen("TRAILER!!!") + 1;
		(void)strcpy(Gen.g_nam_p, "TRAILER!!!");
		G_p = &Gen;
		write_hdr();
		break;
	case TAR:
	/*FALLTHROUGH*/
	case USTAR: /* TAR and USTAR */
		for (cnt = 0; cnt < 3; cnt++) {
			WAIT_FOR_SPACE(TARSZ);
			(void)memset(Buffr.b_in_p, 0, TARSZ);
			INCR_BUFCNT(TARSZ);
		}
		break;
	default:
		msg(EXT, badhdrid, badhdr);
	}
	cnt = 0;
	MUTEX_LOCK(&Buffr.b_mutex);
	while (Buffr.b_cnt > cnt) {
		/*
		 * Some of the trailer remains to be written.  Need to pad at
		 * the end to the next Bufsize, since bflush(_thread) will
		 * (usually) not write less than Bufsize bytes.
		 */
		tmp = Bufsize - (Buffr.b_cnt % Bufsize);
		if(tmp == Bufsize)
			tmp = 0;
	
		MUTEX_UNLOCK(&Buffr.b_mutex);
		WAIT_FOR_SPACE(tmp);
		(void)memset(Buffr.b_in_p, 0, tmp);
		INCR_BUFCNT(tmp);
		cnt += tmp;
		WAIT_FOR_SPACE(0);	/* Wrap Buffr.b_in_p, if it's in scratch area */
		MUTEX_LOCK(&Buffr.b_mutex);
		while (Buffr.b_cnt >= Bufsize) {
#ifdef _REENTRANT
			Buffr.b_waitforput = 1;
			(void)cond_wait(&Buffr.b_put, &Buffr.b_mutex);
#else /* _REENTRANT */
			bflush();
#endif /* _REENTRANT */
		}
#ifdef _REENTRANT
		Buffr.b_waitforput = 0;
#endif /* _REENTRANT */
	}
	MUTEX_UNLOCK(&Buffr.b_mutex);
}

/*
 * Procedure:     chg_lvl_proc
 *
 * Restrictions:
 *                lvlfile(2):	none
 *                lvlproc(2):	none
 */

/*
 * Change the level of a process to that of the given file:
 *
 * 1. If the user does not have the P_SETPLEVEL privilege
 *    or user cannot obtain level of process,
 *    no level change is necessary.
 * 2  If the user has the P_SETPLEVEL privilege and the process level
 *    is successfully obtained,
 *    a. get level of the file
 *    b. get level of process if not obtained yet previously;
 *	 if the process level is not obtained successfully,
 *	 lvlpriv is set to 0, so that subsequent level change is
 *	 not attempted.
 *    c. change level of process to the file's level via lvlproc() only
 * 	 if the file's level is different from the level of the process
 *    d. If lvlproc() returns failure with errno EPERM, 
 *	 indicating that the invoking user does not have the P_SETPLEVEL
 *	 privilege to change process level, lvlpriv is set to 0,
 *	 so that subsequent level change is not attempted.
 *
 * Return value : 
 *	lvl_change - indicate whether level of process is changed :
 *			1 - changed
 *			0 - not changed
 *			
 */
static
int
chg_lvl_proc(namep)
char *namep;
{
	struct stat sb;
	int result;
	int lvl_change = 0;
	int mk_effdir = 0;
	char *p;  		/* pointer to file name */
	char savechar = '\0';
	static int lvlpriv = 1; /* indicate whether the invoking user has the
				 * P_SETPLEVEL privilege to change the level of
				 * the process and whether the process level is
				 * successfully obtained
				 */
	/*
	 * User does not have P_SETPLEVEL privilege
  	 * or user cannot get level of process.  
	 */
	if (!lvlpriv) {
		return (lvl_change);
	}

	result = lvlfile(namep, MAC_GET, &File_lvl);

	/* unable to get level of file */
	if (result == -1) {
		msg(ERN, ":774", "Cannot get level of \"%s\"", namep);
		Target_mld = 0;
		return (lvl_change);
	}

	/*
	 * If target dir is MLD and process is in virtual mode,
	 * find out appropriate place (path component) at which
	 * to deflect in the MLD.  NOTE:  Don't check for virtual mode 
	 * here.  It's enough to check if Target_mld is set, since 
	 * it only gets set if we're in virtual mode (see setup()).
	 */
	if (Target_mld) {
		p = namep;

		/* Skip over "/"s at beginning of string. */
		while (*p == '/')
			p++;

		/* Skip over "./"s and multiple "/"s at beginning of string. */
		while (*p == '.' && p[1] == '/') {
			p += 2;
			while (*p == '/')
				p++;
		}

		/*
		 * If no more slashes in p, then the appropriate
                 * effective directory of the target MLD is the
                 * level of the file itself.  Otherwise, it's the level
                 * of the first component of the pathname of the file.
		 */
		if ((p = strchr(p, '/')) != NULL) {
			/* There is at least another '/' in the path. */
			savechar = '/';
			*p = '\0';
		}
		result = lvlfile(namep, MAC_GET, &Eff_lvl);
		/* unable to get level of file */
		if (result == -1) {
			msg(ERN, ":774", "Cannot get level of \"%s\"", namep);
			Target_mld = 0;	/* Quit Target_mld mode forever. */
			if (p)
				*p = savechar;
			return (lvl_change);
		}
		if (p)
			*p = savechar;
		if (Eff_lvl != File_lvl) {
			/*
			 * Effective directory and file are at different
			 * levels.  :-(
			 * Switch to real MLD mode, and append the LID 
			 * to the target dir name.
			 * Then, set lvl_change, so that restore_lvl_proc()
			 * may reset the mldmode even if the actual process
			 * level has not been modified.
			 */
			if (mldmode(MLD_REAL) == -1) {
				pfmt(stderr, MM_ERROR | MM_NOGET, "%s\n",
					strerror(errno));
				exit(EXIT_FATAL);
			}
			lvl_change = 1;
			Mldmode_chg = 1;
			sprintf(Effdir_p, "%s/%X", Targetmld_p, Eff_lvl);
			if (stat(Effdir_p, &sb) == -1) {
				mk_effdir = 1;
			}
			if (*namep != '/')
				strcat(Effdir_p, "/");
			strcat(Effdir_p, namep);
			Mldtmp_p = Fullnam_p;
			Fullnam_p = Effdir_p;
		}
	}
	
	/* level of process is not obtained yet */
	if (!Proc_lvl) {
		result = lvlproc(MAC_GET, &Proc_lvl);
		/* unable to get level of process */
		if (result == -1) {
			if (Mldmode_chg) {
				if (mldmode(MLD_VIRT) == -1) {
					pfmt(stderr, MM_ERROR | MM_NOGET,
						"%s\n", strerror(errno));
					exit(EXIT_FATAL);
				}
				Mldmode_chg = 0;
				Fullnam_p = Mldtmp_p;
			}
			/* no need to do level change the next time and    *
			 * no need to do anything for target MLDs anymore. */
			lvlpriv = 0;  
			Target_mld = 0;
			msg(ERN, ":775", "Cannot get level of process");
			return (lvl_change);
		}
	}

	if (mk_effdir) {
		/*
		 * We've got to create the effective directory.
		 */
		if (mldmode(MLD_VIRT) == -1) {
			pfmt(stderr, MM_ERROR | MM_NOGET, "%s\n",
				strerror(errno));
			exit(EXIT_FATAL);
		}

		result = lvlproc(MAC_SET, &Eff_lvl);

		if ((result == -1) && (errno == EPERM)) {
			/* 
			 * no need to do level change the next time
			 * no need to do processing for target MLDs 
			 * anymore
			 */
			lvlpriv = 0;
			Target_mld = 0;
			Mldmode_chg = 0;
			Fullnam_p = Mldtmp_p;
			return (0);
		}


		/* 
		 * level of process is changed successfully
		 * Now access the effective directory, thereby
		 * creating the directory at the correct level.
		 */

		if (access(Targetmld_p, F_OK) == -1) {
			pfmt(stderr, MM_ERROR | MM_NOGET, "%s\n",
				strerror(errno));
			exit(EXIT_FATAL);
		}

		if (mldmode(MLD_REAL) == -1) {
			pfmt(stderr, MM_ERROR | MM_NOGET, "%s\n",
				strerror(errno));
			exit(EXIT_FATAL);
		}
	}
	if (Proc_lvl == File_lvl) {
		if (mk_effdir) {
			result = lvlproc(MAC_SET, &Proc_lvl);

			if ((result == -1) && (errno == EPERM)) {
				/* 
				 * no need to do level change the next time
				 * no need to do processing for target MLDs 
				 * anymore
				 */
				if (mldmode(MLD_VIRT) == -1) {
					pfmt(stderr, MM_ERROR | MM_NOGET,
						"%s\n", strerror(errno));
					exit(EXIT_FATAL);
				}
				lvlpriv = 0;
				Target_mld = 0;
				Mldmode_chg = 0;
				Fullnam_p = Mldtmp_p;
				return (0);
			}
		}
		return (lvl_change);
	}

	result = lvlproc(MAC_SET, &File_lvl);

	if (!result) {
		/* level of process is changed successfully */
		Level_chg = 1;
		lvl_change = 1;
	} else if (errno == EPERM) {
		/* user does not have P_SETPLEVEL priv */
		lvlpriv = 0;     /* no need to do level change the next time */
		Target_mld = 0;  /* no need to do processing for target MLDs anymore */
		if (Mldmode_chg) {
			if (mldmode(MLD_VIRT) == -1) {
				pfmt(stderr, MM_ERROR | MM_NOGET,
				     "%s\n", strerror(errno));
				exit(EXIT_FATAL);
			}
			Mldmode_chg = 0;
			Fullnam_p = Mldtmp_p;
		}
 	}

	return (lvl_change);
}

/*
 * Procedure:     restore_lvl_proc
 *
 * Restrictions:  lvlproc(2):	none
 */

/* restore level of process to what is stored in Proc_lvl */
static
void
restore_lvl_proc()
{
	/*
	 * If the target directory is an MLD and we had to
	 * switch into real mode to creat a file, switch
	 * back into virtual mode now.
	 */
	if (Mldmode_chg) {
		if (mldmode(MLD_VIRT) == -1) {
			pfmt(stderr, MM_ERROR | MM_NOGET, "%s\n",
				strerror(errno));
			exit(EXIT_FATAL);
		}
		Fullnam_p = Mldtmp_p;
		Mldmode_chg = 0;
	}
	if (Level_chg) {
		if (lvlproc(MAC_SET, &Proc_lvl) == -1)
			msg(WARNN, ":776", "Cannot restore level of process");
		Level_chg = 0;
	}
}

/*
 * Procedure:     creat_mld
 *
 * Restrictions:
 *                stat(2):	none
 *                mkmld(2):	none
 */

/*
 * Attempt to create the target directory as an MLD if the given source
 * directory is an MLD.
 *
 * 1. If the user does not have the P_MULTIDIR privilge, no MLD is created.
 *    A regular directory will be created on return.
 * 2. If the user has the P_MULTIDIR privilege:
 *    a. test whether the source directory is an MLD.
 *    b. If the source directory is not an MLD, 
 *	 a regular directory will be created on return.
 *    c. If the source directory is an MLD, an MLD is created via mkmld().  
 *	 - If mkmld() returns errno EPERM,
 *  	   indicating that the invoking user lacks the P_MULTIDIR priv,
 *         mldpriv is set to 0, so that subsequent creation of MLD is
 *	   not attempted.  A regular directory will be created on return.  
 *	 - If mkmld() returns error other than EPERM,
 *         -1 is returned.  No dircectory got created.
 *
 * return value:
 *	-1 : Failure in creating an MLD
 * 	 0 : Success in creating an MLD
 * 	 1 : A regular directory is to be created on return
 *
 */
static
int
creat_mld(src_dir, target_dir, mode)
char *src_dir;		/* source directory */
char *target_dir;	/* target directory to be created */
mode_t mode;
{
	int result;
	int ret = 1;
	int changed_mode = 0;	/* flag to tell if we changed MLD mode */
	int curr_mode;		/* holds return from MLD_QUERY */


	/* user does not have the P_MULTIDIR privilege to create an MLD */
	if (!mldpriv)
		/* a regular directory is to be created on return */
		return (ret);

	/* If process is not in real mode, change to real MLD mode. */
	if ((curr_mode=mldmode(MLD_QUERY)) == MLD_VIRT){
		if (mldmode(MLD_REAL) < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
				strerror(errno));
			exit(EXIT_FATAL);
		}
		changed_mode = 1;
	} else if (curr_mode < 0) {
		pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
		exit(EXIT_FATAL);
	}
	/* determine whether the source directory is an MLD */
	if ( stat(src_dir, &SrcSt) < 0) {
		msg(ERN, badaccessid, badaccess, src_dir);
	}
	if (changed_mode) {
		if (mldmode(MLD_VIRT) < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(EXIT_FATAL);
		}
	}

	if (SrcSt.st_flags & S_ISMLD == S_ISMLD) { /* an MLD */
		/* create the target directory as an MLD */
		result = mkmld(target_dir, mode);
		/* error in creating the MLD */
		if (result == -1)
		{
			/* user does not have the P_MULTIDIR priv */
			if (errno == EPERM)
			/* no need to create MLD the next time */
		   	/* a regular dir to be created on return */
				mldpriv = 0;
			/* fail to create the MLD other than ENOSYS */
			/* no directory is created */
			else if (errno != ENOSYS)
				ret = -1;
			/* ENOSYS:a regular dir to be created on return */
		}
		else  /* succeed in creating an MLD */
			ret = 0;
	} /* an MLD */

	return (ret);
}

/*
 * Procedure:     transferdac
 *
 * Restrictions:
 *                acl(2):	none
 *                chmod(2):	none
 */

/*
 * transferdac() will transfer the discretionary access control information
 * of the source to the target.
 *
 * If ACLs are not supported, it sets the target file's permission bits
 * via chmod() to the source file's permission bits.
 *
 * If ACLs are installed, it performs the following actions:
 *   1. It calls acl() with command ACL_GET to get the source file's ACL.
 *   2. If acl() with ACL_GET command returns successfully,
 *      it sets the target file's ACL via acl() with command ACL_SET.
 *      If acl() with command ACL_SET returns ENOSYS, indicating
 *      that the target file system does not support ACLs
 *	and additional entries are specified,
 *      it sets the target file's permission bits via chmod()
 *      to the source file's permission bits,
 *      with the middle 3 bits set to the
 *      the source file's ACL class entry permissions masked by
 *      the group entry permissions.
 *   3. If acl() with ACL_GET command returns error,
 *      it skips the transfer of ACL.
 *      However, if the error is ENOSPC, indicating that there is not
 *      enough space to hold the ACL,
 *      it doubly allocates more buffer space and calls acl() again.
 *      This is repeated until acl() returns succesfully or when
 *      there is not enough memory for more buffers.
 *      If acl() returns successfully, it proceeds as in (2).
 *      If there is not enough memory for more buffers,
 *      it skips the transfer of ACL.
 */

void
transferdac(source, target, modebits)
char *source;
char *target;
ulong modebits;
{
	struct acl aclbuf[NENTRIES]; /* initial number of entries allocated */
	struct acl *aclbufp = aclbuf;/* pointer to allocated buffer */
	int max_entries = NENTRIES;  /* max entries allowed for buffer */
	int nentries;		     /* actual number of entries */
	int aclerr = 0;
	struct acl *tmpbufp;
	int tmpentries;
	int gpbits, clbits = 0;	     /* ACL group entry and class entry */
	int gpdone, cldone = 0;	
	int i;
	char *parentp;	     	     /* pointer to parent directory path */
	char *targetcopy;	     /* copy of target pathname */


	if (aclpkg) { /* ACL security package is installed */

		/*
		 * Must first check if parent directory of target has
		 * default ACL entry.  If so, for the unprivileged user,
		 * we don't want to transfer the ACL from the source.  
		 * We want the default ACL to remain (from creat()).
		 */

		if (!privileged) {
			/*
			 * Get pathname of parent directory.  A copy
			 * of target is used because dirname() may
			 * alter its pathname argument, so the argument
			 * must be disposable.
			 */
			if ((targetcopy = strdup(target)) == NULL) 
				msg(EXT, nomemid, nomem);
			parentp = dirname(targetcopy);

			/* Get acl of the parent directory. */
			nentries = acl(parentp, ACL_GET, max_entries, aclbufp);
			if (nentries == -1) {
				msg(ER, ":899", "Cannot get ACL for parent directory for \"%s\"", target);
				return;
			}
			
			/*
			 * Look through the ACL entries to see if there
			 * are any with default type (aclbufp->a_type). 
			 */
			for (i = 0; i < nentries; i++) {
				if ((aclbufp->a_type & ACL_DEFAULT) == ACL_DEFAULT)
					return;
				aclbufp++;
			}
		}

		/*
		 * If here, proceed with transfer of dac information 
		 * from source to target.  (Either privileged user or
		 * parent directory has no default ACL entry.)
		 */

		nentries = -1;
		while (nentries == -1) {
			nentries = acl(source, ACL_GET, max_entries, aclbufp);
			if ((nentries != -1) ||
			    ((nentries == -1) && (errno != ENOSPC)))
				break;
			tmpentries = max_entries * 2;
			if ((tmpbufp = 
(struct acl *)malloc(tmpentries * sizeof(struct acl))) == (struct acl *)NULL) {
				msg(ER, ":777",
				"Not enough memory to get ACL for \"%s\"", source);
				break;
			}
			if (aclbufp != aclbuf)
				free(aclbufp);
			aclbufp = tmpbufp;
			max_entries = tmpentries;
		} /* end while */

		errno = 0;
		if (nentries != -1) {
			aclerr = acl(target, ACL_SET, nentries, aclbufp);
			/* If user is not privileged, clear umask bits. */
			if ((aclerr == 0) && (!privileged)) {
				modebits = modebits & ~Orig_umask;
				(void) chmod(target, modebits);
			}
		}
	} /* end "if (aclpkg)" */

	if (aclpkg && aclerr != 0 && errno == ENOSYS) {
		tmpbufp = aclbufp;
		/* search ACL for group and class entries */
		for (i = 0; i < nentries; i++, tmpbufp++) {
			if (tmpbufp->a_type == GROUP_OBJ) {
				gpbits = tmpbufp->a_perm & 07;
				gpdone = 1;
			}
			else if (tmpbufp->a_type == CLASS_OBJ) {
				clbits = tmpbufp->a_perm & 07;
				cldone = 1;
			}
			if (gpdone && cldone)
				break;
		} /* end for */	

		/* both ACL group and class entries are obtained */
		if (gpdone && cldone)
			modebits = (modebits & ~070) | ((gpbits & clbits) << 3);
		/* If not privileged, clear umask bits. */
		if (!privileged)
			modebits = modebits & ~Orig_umask;
		(void)chmod(target, modebits);
	} /* end if (aclerr != 0 && errno == ENOSYS) */	
}

/* Enhanced Application Compatibility Support 	*/
/* 	Implementation of [-T] option		*/
/* 	Truncate filenames greater than 14 characters */

static
void
truncate_sco(fname)
register char *fname;
{
	register char *cp;

	cp = fname + Gen.g_namesz;

	while (*cp != '/' && cp > fname)
		cp--;
	if (*cp == '/')
		cp++;
	if ((int)strlen(cp) <= MAX_NAMELEN)
		return;

	if (cp == fname)
		*(fname + MAX_NAMELEN) = '\0';
	else
		*(cp + MAX_NAMELEN) = '\0';
	return;
}
/* End Enhanced Application Compatibility Support */

/*
 * nondigit(arg):  Returns 1 if a character other than
 * a digit is found in the string <arg>; return 0 otherwise.
 * 
 */
static
int
nondigit(arg)
char *arg;
{
	char ch;
	char *ch_p;

	for (ch_p = arg; *ch_p; ch_p++) {
		ch = *ch_p;
		if (!isdigit(ch))
			return(1);
	}
	return(0);
}
