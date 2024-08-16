/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idmkunix.c	1.38"
#ident	"$Header: $"

/* Config for Installable Drivers and Tunable Parameters */

/*
 * Invoked by idbuild to do the main work.
 * Not intended to be invoked directly.
 */

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include "mdep.h"

/*
 * In a cross-environment, make sure these headers are for the host system:
 */
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>

/*
 * In a cross-environment, make sure this header is for the target system:
 */
#include <sys/mod.h>


#define USAGE	":247:Usage: idmkunix [-i|o|r|I dir] [-T|d|t|a|O file] [-S|Q] [[-D|U symbol] ...] [[-M mod] ...]\n"


typedef struct tune tune_t;
typedef struct per_assign assign_t;

/* Parameter table - store mtune and stune */
struct tune {
	struct mtune	mtune;		/* config info from mtune */
	long		value;		/* actual numeric value */
	char		*str_val;	/* actual string value */
	short		conf;		/* was it specified in stune? */
	tune_t		*next;		/* next tune struct in tune_info list */
} *tune_info;

/* Sassign table */
struct per_assign {
	struct sassign	sasgn;		/* config info from sassign */
	driver_t	*driver;	/* driver for this object */
	assign_t	*next;		/* next assign in assign_info list */
} *assign_info;


/* extern declarations */
ctlr_t *sfind();
driver_t *mfind();
tune_t *tfind();
FILE    *open1();       /* open a file */
char    *uppermap();

void defentries(), rddev(), drvpostproc();
void rdftab(), rdintfc(), rdmtune(), rdstune(), rdsassign();
void prconf(), compconf(), prdef(), compfiles(), linkedit();
extern void ckerror();

static char *getpref();
static int proc(), compile();
static void comp_drv();
static void prsec();

static void print_func_tables(), print_func_table();
static void print_cpubind_table();
static void print_vfssw();
static void print_execsw();
static void print_mod_stub_tab();
static void print_static();
static void print_interfaces();
static void print_name_to_modwrapper_list();
static void prmod(), prmodreg();
static void rd_res_major_max();

static void add_pragma();
static void print_mod_pragma();

static void check_interfaces();
static int check_intfc_symbol();
static int is_ftab();


/* flags */
int debug = 0;		/* debug flag */
int rflag = 0;		/* root directory */
int iflag = 0;		/* directory containing input files */
int oflag = 0;		/* directory for output files */
int mflag = 0;		/* loadable module specified */
int qflag = 0;		/* quick linkedit specified */
int outflag = 0;	/* output file name specified */

/* buffers for pathnames */
static char root[PATH_MAX];	/* root directory */
static char input[PATH_MAX];	/* directory containing input files */
static char output[PATH_MAX];	/* directory for output files */
static char cfdir[PATH_MAX];	/* configuration directory */
static char pkdir[PATH_MAX];	/* "package" directory (has .o and .c files) */
static char moddir[PATH_MAX];	/* directory for loadable modules */
static char header[PATH_MAX];	/* include directory */
static char dfile_path[PATH_MAX]; /* deflist file */
static char outfile[PATH_MAX];	/* output file name */
extern char current[PATH_MAX];	/* current directory */

/* extern variables which control getinst() filenames */
extern char instroot[];
extern char pathinst[];

/* input file names */
char	*dfile = "deflist";	/* list of cc -D options */

/* output file names */
char    *cfile = "conf.c";      /* configuration table file */
char    *hfile = "config.h";    /* configuration header file */
char    *mfile = "mod_conf.c";  /* per loadable module configuration file */
char    *sfile = "mod_sec.s";   /* per loadable module special section file */
char    *gfile = "mod_reg";     /* per loadable module registration file */
char	*ifile = "ifile";	/* "ifile" for link editor (lists .o files) */

int	new_cfile;

/* CCS components */
#define LD	"ld"
#define CC	"cc"
#define IDLD	"/bin/idld"
#define IDCC	"/bin/idcc"
#define IDBIN	"/bin"
#define IDLIB	"/lib"

static char *prefix;

char ld_path[PATH_MAX];
char cc_path[PATH_MAX];

/* lists of -D, -U, and -I options */
char deflist[256] = "-DSYSV -D_KERNEL -DINKERNEL";
char inclist[1024];

struct LIST  {
	void (*add)();			/* functions called by main program */
	char *name;			/* name of function */
} list[] = {
    defentries,		"defentries",	/* Setup entry-point definitions */
    rdftab,		"rdftab",	/* Ftab files for extra func tables */
    rdintfc,		"rdintfc",	/* Interface files */
    rdmtune,		"rdmtune",      /* Master tunable parameter file */
    rdstune,		"rdstune",      /* System tunable parameter file */
    rddev,		"rddev",	/* Master and System device files */
    drvpostproc,	"drvpostproc",	/* Per-driver post-processing */
    rdsassign,		"rdsassign",    /* System assign file */
    ckerror,		"ckerror",      /* check for errors */
    prdef,		"prdef",        /* generate config.h */
    compfiles,		"compfiles",    /* compile/link files into modules */
    ckerror,		"ckerror",      /* check for errors again */
    rd_res_major_max,	"rd_res_major_max",
    prconf,		"prconf",	/* generate conf.c */
    ckerror,		"ckerror",      /* check for errors again */
    compconf,		"compconf",	/* compile conf.c */
    linkedit,		"linkedit",	/* link together unix kernel */
    NULL,		""
};

unsigned short	file_mode = 0664;	/* File mode for new files */
unsigned short	cur_umask;

#define DIR_MODE 0755
#define MOD_MODE 0644

/* Built-in entry-point types */
struct entry_def *edef_open, *edef_close, *edef_read, *edef_write, *edef_ioctl;
struct entry_def *edef_chpoll, *edef_mmap, *edef_segmap, *edef_size;
struct entry_def *edef_devinfo;
struct entry_def *edef_strategy, *edef_print, *edef_intr, *edef_msgio;
struct entry_def *edef_exec, *edef_core, *edef_textinfo;
struct entry_def *edef__init, *edef_init, *edef_start;

/* "Entry-point" types for variables */
struct entry_def *edef_devflag, *edef__wrapper;
struct entry_def *edef_info;
struct entry_def *edef_conssw;
struct entry_def *edef__vfsops, *edef__fsflags;

/* switch table sizes */
int bdevswsz;
int cdevswsz;
int vfsswsz;
int fmodswsz;

int bdev_reserve, cdev_reserve, vfs_reserve, fmod_reserve;

short res_bdev_max = 0;		/* max bdev major used in res_major */
short res_cdev_max = 0;		/* max cdev major used in res_major */

/* default console selection */
driver_t *consdrv;
int consminor;

static int no_def_delay;
static struct modlist *modlist;

extern int noloadable;
extern struct intfc *interfaces;
extern int suppress_mdep_checks;


main(argc,argv)
int argc;
char *argv[];
{
        int m;
        struct LIST *p;
	struct modlist *mod;
	extern char *optarg;

	umask(022);

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxidtools");
	(void) setlabel("UX:idmkunix");

        while ((m=getopt(argc, argv, "?#D:I:M:O:QST:U:a:d:i:o:r:t:c")) != EOF)
                switch(m) {

		case 'c':
			suppress_mdep_checks = 1;
			break;

                case 'T':
                        ftypes[MTUN].fname = optarg;
                        break;
                case 'd':
                        ftypes[SDEV].fname = optarg;
                        break;
                case 't':
                        ftypes[STUN].fname = optarg;
                        break;
                case 'a':
                        ftypes[SASN].fname = optarg;
                        break;
                case 'r':
                        strcpy(root, optarg);
                        rflag++;
                        break;
                case 'i':
                        strcpy(input, optarg);
                        iflag++;
                        break;
                case 'o':
                        strcpy(output, optarg);
                        oflag++;
                        break;
		case 'M':
			mflag++;
			if (strlen(optarg) >= NAMESZ)
				fatal("argv[0]: module name \"%s\" specified is longer than %d characters",
					optarg, NAMESZ - 1);
			mod = (struct modlist *)malloc(sizeof(struct modlist));
			strncpy(mod->name, optarg, NAMESZ);
			mod->next = modlist;
			modlist = mod;
			break;
		case 'S':
			noloadable = 1;
			break;
		case 'D':
			strcat(deflist, " -D");
			strcat(deflist, optarg);
			break;
		case 'U':
			strcat(deflist, " -U");
			strcat(deflist, optarg);
			break;
		case 'I':
			strcat(inclist, " -I");
			strcat(inclist, optarg);
			break;
		case 'O':
			strcpy(outfile, optarg);
			outflag++;
			break;
		case 'Q':
			qflag++;
			break;
                case '#':
                        debug++;
                        break;
                case '?':
                        pfmt(stderr, MM_ERROR, USAGE);
                        exit(1);
                }

	if (mflag)
		noloadable = 0;

	(void) getcwd(current, sizeof current);

        /* Get full path names for root, input, and output directories */
        getpath(rflag, root, "/");
	if (strcmp(root, "/") == 0)
		strcpy(instroot, ROOT);
	else
		sprintf(instroot, "%s/%s", root, ROOT);
        sprintf(cfdir, "%s/%s", instroot, CFDIR);
        getpath(iflag, input, cfdir);
        getpath(oflag, output, cfdir);

        if (debug)
                fprintf(stdout, "Root: %s\nInput: %s\nOutput: %s\n\n",
                        root, input, output);

	/*
	 * Construct path names for CCS components,
	 * and input and output directories.
	 */
	prefix = getpref(argv[0]);
	if (strcmp(prefix, "") == 0) {
		/* native build */
		strcpy(ld_path, IDLD);
		strcpy(cc_path, IDCC);
	} else {
		/* cross environment */
		sprintf(ld_path, "%s%s", prefix, LD);
		sprintf(cc_path, "%s%s", prefix, CC);
	}

	if (!outflag)
		sprintf(outfile, "%s%s", output, "/unix");

	sprintf(inclist + strlen(inclist),
		" -I%s -I%s%s", output, root, SYSINC);

	sprintf(pkdir, "%s/%s", instroot, PKDIR);

	if (dfile[0] == '/')
		strcpy(dfile_path, dfile);
	else
		sprintf(dfile_path, "%s/%s", output, dfile);

	if (mflag)
		sprintf(moddir, "%s/mod.d", instroot);
	else
		sprintf(moddir, "%s/modnew.d", instroot);

	/* Tell getinst() about input directory name */
	strcpy(pathinst, input);

	/* Modify file_mode by umask */
	cur_umask = umask(0);
	umask(cur_umask);
	file_mode &= ~cur_umask;

        /* call each function */
        for (p = &list[0]; p->add != NULL; p++) {
                if (debug)
                        fprintf(stderr, "Main: Before %s\n", p->name);
                (*p->add)();
        }

        exit(0);
}


/* This routine is used to search the Parameter table
 * for the keyword that was specified in the configuration.  If the
 * keyword cannot be found in this table, a NULL is returned.
 * If the keyword is found, a pointer to that entry is returned.
 */
tune_t *
tfind(keyword)
char *keyword;
{
        register tune_t *tune;

	for (tune = tune_info; tune != NULL; tune = tune->next) {
                if (equal(keyword, tune->mtune.name))
                        return (tune);
        }
        return (NULL);
}



/* This routine is used to map lower case alphabetics into upper case. */

char *
uppermap(device,caps)
char *device;
char *caps;
{
        register char *ptr;
        register char *ptr2;
        ptr2 = caps;
        for (ptr = device; *ptr != NULL; ptr++) {
                if ('a' <= *ptr && *ptr <= 'z')
                        *ptr2++ = *ptr + 'A' - 'a';
                else
                        *ptr2++ = *ptr;
        }
        *ptr2 = NULL;
        return (caps);
}



/* open a file */

FILE *
open1(file, mode, dir)
char *file, *mode;
int dir;
{
	char path[PATH_MAX];
        FILE *fp;
        char *p;

        switch (dir) {
        case IN:
                sprintf(path, "%s/%s", input, file);
                p = path;
                break;
        case OUT:
                sprintf(path, "%s/%s", output, file);
                p = path;
                break;
        case FULL:
                p = file;
                break;
        }

        if (debug)
                fprintf(stderr, "Open: mode=%s path=%s\n", mode, p);

        if ((fp = fopen(p, mode)) == NULL) {
		if (dir == IN)
			return NULL;
                pfmt(stderr, MM_ERROR, FOPEN, p, mode);
                fatal(0);
        }
        return (fp);
}


/* set up entry-point definitions */

#define DEFINE_ENTRY(edef_var, suffix) \
	if (((edef_var) = define_entry(suffix, 0)) == NULL) { \
		pfmt(stderr, MM_ERROR, TABMEM, "entry-point"); \
		fatal(1); \
	}
#define DEFINE_VAR(edef_var, suffix) \
	if (((edef_var) = define_entry(suffix, 1)) == NULL) { \
		pfmt(stderr, MM_ERROR, TABMEM, "built-in variable"); \
		fatal(1); \
	}
#define DEFINE_FTAB(suffix, tabname, ret_type, flags) \
	if (define_ftab(suffix, tabname, ret_type, flags) == NULL) { \
		pfmt(stderr, MM_ERROR, TABMEM, "function table"); \
		fatal(1); \
	}

void
defentries()
{
	/* Define built-in entry-point types */
	DEFINE_ENTRY(edef_open, "open");
	DEFINE_ENTRY(edef_close, "close");
	DEFINE_ENTRY(edef_read, "read");
	DEFINE_ENTRY(edef_write, "write");
	DEFINE_ENTRY(edef_ioctl, "ioctl");
	DEFINE_ENTRY(edef_chpoll, "chpoll");
	DEFINE_ENTRY(edef_msgio, "msgio");
	DEFINE_ENTRY(edef_mmap, "mmap");
	DEFINE_ENTRY(edef_segmap, "segmap");
	DEFINE_ENTRY(edef_size, "size");
	DEFINE_ENTRY(edef_strategy, "strategy");
	DEFINE_ENTRY(edef_devinfo, "devinfo");
	DEFINE_ENTRY(edef_print, "print");
	DEFINE_ENTRY(edef_intr, "intr");
	DEFINE_ENTRY(edef_exec, "exec");
	DEFINE_ENTRY(edef_core, "core");
	DEFINE_ENTRY(edef_textinfo, "textinfo");
	DEFINE_ENTRY(edef_init, "init");
	DEFINE_ENTRY(edef_start, "start");
	DEFINE_ENTRY(edef__init, "_init");

	/* Define built-in variable names */
	DEFINE_VAR(edef_devflag, "devflag");
	DEFINE_VAR(edef_info, "info");
	DEFINE_VAR(edef__wrapper, "_wrapper");
	DEFINE_VAR(edef_conssw, "conssw");
	DEFINE_VAR(edef__vfsops, "_vfsops");
	DEFINE_VAR(edef__fsflags, "_fsflags");
}


/* read ftab.d - function table definitions */

void
rdftab()
{
	struct ftab ftab;
	int stat;

	(void)getinst(FTAB_D, RESET, NULL);

	while ((stat = getinst(FTAB_D, NEXT, &ftab)) == 1) {
		DEFINE_FTAB(ftab.entry, ftab.tabname, ftab.type, ftab.fflags);
	}

	if (stat != 0)
		insterror(stat, FTAB_D, "");
}

/* read interface files - (symbol remapping tables) */

void
rdintfc()
{
	load_interfaces();
	if (debug)
		dump_interfaces();
}

/* read mtune - Master tune file */

void
rdmtune()
{
	struct mtune mtune;
        register tune_t *tun;
	int stat;

	(void)getinst(MTUN, RESET, NULL);

	while ((stat = getinst(MTUN, NEXT, &mtune)) == 1) {
		if ((tun = (tune_t *)malloc(sizeof(tune_t))) == NULL) {
			pfmt(stderr, MM_ERROR, TABMEM, "mtune");
			fatal(1);
		}
                tun->mtune = mtune;
		if (mtune.str_def)
			tun->str_val = mtune.str_def;
		else
			tun->value = mtune.def;
		tun->conf = 0;
		tun->next = tune_info;
		tune_info = tun;
        }

	if (stat != 0)
		insterror(stat, MTUN, "");
}


/* check System device conflicts */

int
chksdev(drv, sdp)
	driver_t *drv;
	struct sdev *sdp;
{
	if (INSTRING(drv->mdev.mflags, ONCE) && (sfind(sdp->name) != NULL)) {
		pfmt(stderr, MM_ERROR, ONESPEC, sdp->name);
		error(1);
		return (0);
	}

	/* Check for out-of-range values and conflicts in interrupt vectors,
	 * I/O addresses, and controller memory addresses.
	 * Skip this check for execsw modules because the fields are interpreted
	 * differently. Filesystem and scheduler modules are also skipped since
	 * they don't use those fields.
	 */
	if (!INSTRING(drv->mdev.mflags, EXECSW) && 
		!INSTRING(drv->mdev.mflags, FILESYS) &&
		!INSTRING(drv->mdev.mflags, DISP) &&
		!mdep_check(sdp, drv))
		return (0);

	return (1);
}


/* read Master and System info */

void
rddev()
{
	(void) getdevconf(chksdev);
}


/* per-driver post-processing */

void
drvpostproc()
{
        register driver_t *drv;
	struct modlist *mod;

	mdep_drvpostproc();

	/*
         * check for missing required devices
	 * and get function lists for configured drivers
	 */
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		/*
		 * If -M option used, skip any modules not selected.
		 */
		drv->skip = 0;
		if (mflag) {
			for (mod = modlist; mod != NULL; mod = mod->next)
				if (strcmp(mod->name, drv->mdev.name) == 0)
					break;
			if (mod == NULL) {
				drv->skip = 1;
				continue;
			}
		}

		if (drv->n_ctlr == 0)
			continue;

		/*
		 * Make sure that all the configured exec modules have magic
		 * numbers specified in their Master files.
		 */
		if (drv->mdev.magics == NULL) {
			if (INSTRING(drv->mdev.mflags, EXECSW)) {
                        	pfmt(stderr, MM_ERROR, NOMAGIC, drv->mdev.name);
	                        fatal(0);
			}
		} else {
			if (!INSTRING(drv->mdev.mflags, EXECSW)) {
                        	pfmt(stderr, MM_ERROR, INVMAGIC, drv->mdev.name);
	                        fatal(0);
			}
		}

		if (drv->loadable) {
			lookup_entry(edef__wrapper->suffix,
				 &drv->mdev.entries, 0);
		}

		/* For version 0 compatibility, FCOMPAT flag implies devflag */
		/* devflag is now required for drivers and STREAMS modules */
		if (drv->mdev.over == 0 &&
		    !INSTRING(drv->mdev.mflags, FCOMPAT) &&
		    (INSTRING(drv->mdev.mflags, BLOCK) ||
		     INSTRING(drv->mdev.mflags, CHAR) ||
		     INSTRING(drv->mdev.mflags, STREAM))) {
			pfmt(stderr, MM_ERROR, NOFCOMPAT, drv->mdev.name);
			fatal(0);
		}
	}
}


/* rdstune - System tunable parameter file */

void
rdstune()
{
	struct stune stune;
	register tune_t *tune;
	int stat;

	(void)getinst(STUN, RESET, NULL);

	while ((stat = getinst(STUN, NEXT, &stune)) == 1) {
		/* find tunable in Parameter table */
		tune = tfind(stune.name);
		if (tune == NULL) {
			pfmt(stderr, MM_ERROR, TUNE, stune.name);
			error(0);
			continue;
		}

                /* check if already specified */
                if (tune->conf) {
                        pfmt(stderr, MM_ERROR, RESPEC, stune.name);
                        error(0);
                        continue;
                }

		/* store value in Parameter table */
		tune->str_val = stune.value;
		if (!tune->mtune.str_def) {
			tune->value = strtol(stune.value, NULL, 0);
	                /* check whether parameter is within min and max */
			if (tune->value < tune->mtune.min ||
				tune->value > tune->mtune.max) {
				pfmt(stderr, MM_ERROR, PARM, stune.name, tune->value,
					tune->mtune.min, tune->mtune.max);
				error(1);
				continue;
			}

		}

                /* indicate tunable parameter specified */
                tune->conf = 1;
        }

	if (stat != 0)
		insterror(stat, STUN, "");
}


/* read sassign - System assignment file */

void
rdsassign()
{
	struct sassign sassign;
	register assign_t *assign;
	driver_t *drv;
	int highminor;
	int stat;

	(void)getinst(SASN, RESET, NULL);

	while ((stat = getinst(SASN, NEXT, &sassign)) == 1) {
		if ((drv = mfind(sassign.major)) == NULL) {
			pfmt(stderr, MM_ERROR, UNK, sassign.major);
			error(1);
			continue;
		}

		highminor = max_minor(drv);
		if (sassign.minor < 0 || sassign.minor > highminor) {
			pfmt(stderr, MM_ERROR, MINOR, 0, highminor);
			error(1);
			continue;
		}

		if (strcmp(sassign.device, "console") == 0) {
			if (!INSTRING(drv->mdev.mflags, CONSDEV)) {
				pfmt(stderr, MM_ERROR, NOTCONS, drv->mdev.name);
				error(1);
				continue;
			}
			consdrv = drv;
			consminor = sassign.minor;
			continue;
		}

		if (!INSTRING(drv->mdev.mflags, BLOCK) &&
		    !INSTRING(drv->mdev.mflags, CHAR)) {
			pfmt(stderr, MM_ERROR, DEVREQ, sassign.major);
			error(1);
			continue;
		}

                if ((assign = (assign_t *)malloc(sizeof(assign_t))) == NULL) {
			pfmt(stderr, MM_ERROR, TABMEM, "sassign");
                        fatal(1);
                }

		assign->driver = drv;
		assign->sasgn = sassign;
		assign->next = assign_info;
		assign_info = assign;
        }

	if (stat != 0)
		insterror(stat, SASN, sassign.device);
}


int
max_minor(driver)
	driver_t *driver;
{
	register tune_t *t_maxminor;

	if ((t_maxminor = tfind("MAXMINOR")) == NULL) {
		pfmt(stderr, MM_ERROR, NOMAXMINOR, (long)OMAXMIN);
		warning(0);
		return OMAXMIN;
	}
	return (int)(t_maxminor->conf ?
		     t_maxminor->value : t_maxminor->mtune.def);
}



/* print out configuration header file */

void
prdef()
{
        register FILE *fp;
        register driver_t *drv;
        register ctlr_t *ctlr;
        char caps[PFXSZ];
	int do_hw;

	if (qflag && access(hfile, F_OK) == 0)
		return;

        fp = open1(hfile, "w", OUT);
        chmod(hfile, file_mode);

        /* go through Master table */
        {
	register int j;
	int HowMany;

        fprintf(fp, "/* defines for each device */\n");

        for (drv = driver_info; drv != NULL; drv = drv->next) {

                /* skip devices that are not configured */
                if (drv->n_ctlr == 0)
                        continue;

		/* skip devices with no prefix */
		if (drv->mdev.prefix[0] == '\0' ||
		    drv->mdev.prefix[0] == '-')
			continue;

                uppermap(drv->mdev.prefix, caps);
                fprintf(fp, "\n#define\t%s\t\t1\n", caps);
                fprintf(fp, "#define\t%s_MODNAME\t\"%s\"\n",
			    caps, drv->mdev.extname);

		do_hw = ((INSTRING(drv->mdev.mflags, HARDMOD) ||
			  drv->mdev.over < 2) && !drv->autoconf);

		if (do_hw) {
			fprintf(fp, "#define\t%s_CNTLS\t%hd\n", caps,
							drv->n_ctlr);
		}

		if (!drv->autoconf) {
			fprintf(fp, "#define\t%s_UNITS\t%ld\n", caps,
							drv->tot_units);
		}

		if (INSTRING(drv->mdev.mflags, HARDMOD)) {
			fprintf(fp, "#define\t%s_CPUBIND\t%d\n",
				caps, drv->bind_cpu);
		}
		if (INSTRING(drv->mdev.mflags, BLOCK)) {
			HowMany = drv->mdev.blk_end - drv->mdev.blk_start + 1;
                	fprintf(fp, "#define\t%s_BMAJORS\t%hd\n",
					caps, HowMany);
			for (j = 0; j < HowMany; j++)
				fprintf(fp, "#define\t%s_BMAJOR_%hd\t%hd\n",
					caps, j, drv->mdev.blk_start + j);
		}
		if (INSTRING(drv->mdev.mflags, CHAR)) {
			HowMany = drv->mdev.chr_end - drv->mdev.chr_start + 1;
                	fprintf(fp, "#define\t%s_CMAJORS\t%hd\n",
					caps, HowMany);
			for (j = 0; j < HowMany; j++)
				fprintf(fp, "#define\t%s_CMAJOR_%hd\t%hd\n",
					caps, j, drv->mdev.chr_start + j);
		}

		if (do_hw)
			mdep_prdrvconf(fp, drv, caps);
        }
        }

        /* go through per-controller table */
        {
        fprintf(fp, "\n\n/* defines for each controller */\n");

        for (ctlr = ctlr_info; ctlr != NULL; ctlr = ctlr->next) {
		drv = ctlr->driver;

		do_hw = ((INSTRING(drv->mdev.mflags, HARDMOD) ||
			  drv->mdev.over < 2) && !drv->autoconf);

		if (!do_hw)
			continue;

		/* skip special modules */
		if (INSTRING(drv->mdev.mflags, FILESYS) ||
		    INSTRING(drv->mdev.mflags, EXECSW) ||
		    INSTRING(drv->mdev.mflags, DISP))
			continue;

		/* skip devices with no prefix */
		if (drv->mdev.prefix[0] == '\0' ||
		    drv->mdev.prefix[0] == '-')
			continue;

                uppermap(drv->mdev.prefix, caps);

		fprintf(fp, "\n#define\t%s_%hd\t\t%ld\n",
			caps, ctlr->num, ctlr->sdev.unit);

		mdep_prctlrconf(fp, ctlr, caps);
        }
        }

        /* go through tunable Parameter table */
        {
        register tune_t *tune;
        
        fprintf(fp, "\n/* defines for each tunable parameter */\n");

	for (tune = tune_info; tune != NULL; tune = tune->next) {
                fprintf(fp, "#define\t%s\t", tune->mtune.name);
		if (tune->mtune.str_def)
			fprintf(fp, "\"%s\"\n", tune->str_val);
		else
			fprintf(fp, "%ld\n", tune->value);
	}
        }

        fclose(fp);
}


find_ddi_level(drv)
driver_t *drv;
{
	struct interface_list *p;
	int level = -1;
	int i;
	int n;

	for (p = drv->mdev.interfaces; p; p = p->next)
	{
		if (strcmp(p->name, "ddi") != 0)
			continue;

		for (i = 0; p->versions[i]; i++)
		{
			n = atoi(p->versions[i]);
			if (n > level)
				level = n;
		}

		break;
	}

	return level;
}


/* print out configuration table file (conf.c) */
void
prconf()
{
	register FILE *fp;
	register driver_t *drv, *rdrv;
	driver_t nodrv;
	assign_t *assign;
	char *pfx, *name;
	char *mflags;
	int is_blk, is_chr;
	int i, j;
	int driver, module;
	char	xbf[256];	/* buffer for external symbol definitions */
	char	caps[NAMESZ];	/* buffer for upper case device prefix */
	int ndrv_static;	/* number of statically linked modules */
	tune_t *tune;
	int empty_slot;
	int cpu;

	if (mflag)
		return;
	if (qflag && access(cfile, F_OK) == 0)
		return;

	new_cfile = 1;

	fp = open1(cfile, "w", OUT);
	chmod(cfile, file_mode);

	fprintf(fp, "#include <sys/types.h>\n");
	fprintf(fp, "#include <sys/sysmacros.h>\n");
	fprintf(fp, "#include <sys/conf.h>\n");
	fprintf(fp, "#include <sys/conssw.h>\n");
	fprintf(fp, "#include <sys/class.h>\n");
	fprintf(fp, "#include <sys/exec.h>\n");
	fprintf(fp, "#include <sys/vfs.h>\n");
	fprintf(fp, "#include <sys/errno.h>\n");

	fprintf(fp, "\nextern int nodev(), nxio(), nulldev(), mod_enosys();\n");
	fprintf(fp, "#define nulliob (struct iobuf *)0\n");
	fprintf(fp, "#define nullinfo (struct streamtab *)0\n");
	fprintf(fp, "int nodevflag = 0;\n");

/*
 * Print the function tables for miscellaneous entry points.
 */
	print_func_tables(fp);

	fprintf(fp, "\n\n");

/*
 * Search the Master table and generate an extern statement for
 * any routines that are needed.
 *
 * Declare the required streamtab structures here, as well.
 */
	ndrv_static = 0;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* if not configured or loadable, continue */
		if (drv->n_ctlr == 0 || drv->loadable)
			continue;

		ndrv_static++;

		pfx = drv->mdev.prefix;
		mflags = drv->mdev.mflags;

		/* declare streamtab structures here */

		if (INSTRING(mflags, STREAM))
			fprintf(fp, "extern struct streamtab %sinfo;\n", pfx);

		/* declare exec switch structures here */

		if (INSTRING(mflags, EXECSW)) {
			int has_core = 0;
			int has_textinfo = 0;
			struct magic_list *mlist;

			if (drv->n_ctlr != 1) {
				pfmt(stderr, MM_ERROR, ONESPEC, drv->mdev.name);
				error(0);
			}
			if (!drv_has_entry(drv, edef_exec)) {
				pfmt(stderr, MM_ERROR, EXRT, drv->mdev.name);
				error(0);
			}

			mlist = drv->mdev.magics;
			if (mlist != NULL && mlist->nmagic) {
				fprintf(fp,
				"static ushort_t %smagic[%d] = {\n\t0x%x",
					pfx, mlist->nmagic, mlist->magics[0]);
				for (i = 1; i < mlist->nmagic; )
					fprintf(fp, ",\n\t0x%x",
						mlist->magics[i++]);
				fprintf(fp, "\n};\n");
			}
			
			fprintf(fp, "extern int %sexec();\n", pfx);

			if (drv_has_entry(drv, edef_core)) {
				fprintf(fp, "extern int %score();\n", pfx);
				has_core = 1;
			}
			if (drv_has_entry(drv, edef_textinfo)) {
				fprintf(fp, "extern int %stextinfo();\n", pfx);
				has_textinfo = 1;
			}

			fprintf(fp,
				"static struct execsw_info %sexecsw_info = {\n", pfx);
			fprintf(fp, "\t%sexec, ", pfx);
			if (has_core)
				fprintf(fp, "%score, ", pfx);
			else
				fprintf(fp, "mod_enosys, ");

			if (has_textinfo)
				fprintf(fp, "%stextinfo, NULL };\n", pfx);
			else
				fprintf(fp, "mod_enosys, NULL };\n");
		}

		/* declare conssw structures here */

                if (INSTRING(drv->mdev.mflags, CONSDEV)) {
			fprintf(fp, "extern struct conssw %sconssw;\n",
				drv->mdev.prefix);
		}

		/* skip special modules */
		if (INSTRING(mflags, FILESYS) ||
		    INSTRING(mflags, EXECSW) ||
		    INSTRING(mflags, DISP))
			continue;

		/* is this a block device? */
		is_blk = INSTRING(mflags, BLOCK);
		is_chr = INSTRING(mflags, CHAR);

		/* the rest only applies to drivers and streams modules */
		if (!is_blk && !is_chr && !INSTRING(mflags, STREAM))
			continue;

		fprintf(fp, "extern int %sdevflag;\n", pfx);

		/* the rest only applies to drivers */
		if (!is_blk && !is_chr)
			continue;

		strcpy(xbf, "extern int ");

		if (drv_has_entry(drv, edef_open))
			sprintf(xbf + strlen(xbf), "%sopen(), ", pfx);
		else if (is_blk) {
			pfmt(stderr, MM_ERROR, OPRT, drv->mdev.name);
			error(0);
		}
		if (drv_has_entry(drv, edef_close))
			sprintf(xbf + strlen(xbf), "%sclose(), ", pfx);
		else if (is_blk) {
			pfmt(stderr, MM_ERROR, CLRT, drv->mdev.name);
			error(0);
		}
		if (drv_has_entry(drv, edef_read))
			sprintf(xbf + strlen(xbf), "%sread(), ", pfx);
		if (drv_has_entry(drv, edef_write))
			sprintf(xbf + strlen(xbf), "%swrite(), ", pfx);
		if (drv_has_entry(drv, edef_ioctl))
			sprintf(xbf + strlen(xbf), "%sioctl(), ", pfx);
		if (is_blk) {
			if (!drv_has_entry(drv, edef_strategy)) {
				pfmt(stderr, MM_ERROR, STRAT, drv->mdev.name);
				error(0);
			}
			sprintf(xbf + strlen(xbf), "%sstrategy(), ", pfx);
			if (drv_has_entry(drv, edef_size))
				sprintf(xbf + strlen(xbf), "%ssize(), ", pfx);
		}
		if (drv_has_entry(drv, edef_devinfo))
			sprintf(xbf + strlen(xbf), "%sdevinfo(), ", pfx);
		if (drv_has_entry(drv, edef_chpoll))
			sprintf(xbf + strlen(xbf), "%schpoll(), ", pfx);
		if (drv_has_entry(drv, edef_msgio))
			sprintf(xbf + strlen(xbf), "%smsgio(), ", pfx);
		if (drv_has_entry(drv, edef_mmap))
			sprintf(xbf + strlen(xbf), "%smmap(), ", pfx);
		if (drv_has_entry(drv, edef_segmap))
			sprintf(xbf + strlen(xbf), "%ssegmap(), ", pfx);

		/* If there were no decls, trunc xbf[] to 0 length */
		if (xbf[strlen(xbf)-2] != ',')
			xbf[0] = '\0';
		else
			strcpy(&xbf[strlen(xbf)-2], ";");
		fprintf(fp, "%s\n", xbf);

		mdep_devsw_decl(fp, drv);
	}

/* 
 * get the number of reserved slot for bdevsw, cdevsw, vfssw, and fmodsw
 */
	if ((tune = tfind("BDEV_RESERVE")) == NULL)
		bdev_reserve = DEF_BDEV_RESERVE;
	else
		bdev_reserve = tune->value;

	if ((tune = tfind("CDEV_RESERVE")) == NULL)
		cdev_reserve = DEF_CDEV_RESERVE;
	else
		cdev_reserve = tune->value;

	if ((tune = tfind("VFS_RESERVE")) == NULL)
		vfs_reserve = DEF_VFS_RESERVE;
	else
		vfs_reserve = tune->value;

	if ((tune = tfind("FMOD_RESERVE")) == NULL)
		fmod_reserve = DEF_FMOD_RESERVE;
	else
		fmod_reserve = tune->value;

/*
 * Go through block device table and indicate addresses of required routines.
 * If a particular device is not present, fill in "nxio/nodev" entries.
 */

	memset((char *)&nodrv, 0, sizeof nodrv);
	strcpy(nodrv.mdev.prefix, "no");
	strcpy(nodrv.mdev.extname, "nodev");
	nodrv.bind_cpu = -1;

	empty_slot = 0;
	fprintf(fp, "\nstruct bdevsw bdevsw[] = {\n");
	for (rdrv = bdevices, j = 0; rdrv != NULL; ++j) {
		if (j >= rdrv->mdev.blk_start && !rdrv->loadable)
			drv = rdrv;
		else {
			empty_slot++;
			drv = &nodrv;
		}

		pfx = drv->mdev.prefix;
		name = drv->mdev.extname;
		mflags = drv->mdev.mflags;
		is_blk = INSTRING(mflags, BLOCK);

		if (j != 0)
			fprintf(fp, ",\n");
		fprintf(fp, "/* %2d */  {\t", j);

		if (drv_has_entry(drv, edef_open))
			fprintf(fp, "%sopen,", pfx);
		else {
			if (drv == rdrv)
				fprintf(fp, "nodev,");
			else
				fprintf(fp, "nxio,");
		}
		if (drv_has_entry(drv, edef_close))
			fprintf(fp, "\t%sclose,", pfx);
		else
			fprintf(fp, "\tnodev,");

		if (is_blk)
			fprintf(fp, "\t%sstrategy,", pfx);
		else
			fprintf(fp, "\tnodev,");

		if (is_blk && drv_has_entry(drv, edef_size))
			fprintf(fp, "\t\t%ssize,", pfx);
		else
			fprintf(fp, "\t\tnulldev,");

		if (is_blk && drv_has_entry(drv, edef_devinfo))
			fprintf(fp, "\t\t%sdevinfo,", pfx);
		else
			fprintf(fp, "\t\tmod_enosys,");

		fprintf(fp, "\t\"%s\",\n", name);

		fprintf(fp, "\t\tnulliob,");

		mdep_bdevsw(fp, drv);

		fprintf(fp, "\t&%sdevflag,", pfx);

		if ((cpu = drv->bind_cpu) == -1 &&
		    INSTRING(drv->mdev.mflags, HARDMOD))
			cpu = -2;
		fprintf(fp, "\t%d }", cpu);

		if (j == rdrv->mdev.blk_end)
			rdrv = rdrv->bdev_link;
	}

	if (empty_slot >= bdev_reserve)
		bdevswsz = j;
	else
		bdevswsz = j + bdev_reserve - empty_slot;

	if (bdevswsz < res_bdev_max)
		bdevswsz = res_bdev_max;

	if (bdevswsz == 0)
		fprintf(fp, "\t{ nodev }");

	while (j < bdevswsz) {
		fprintf(fp, ",\n/* %2d */  {\t", j);
		fprintf(fp, "nxio,\tnodev,\tnodev,");
		fprintf(fp, "\tnulldev,\tnodev,\t\"nodev\",\n");
		fprintf(fp, "\t\tnulliob,\t&nodevflag,\t-1 }");
		j++;
	}
		
	fprintf(fp,"\n};\n\n");
	fprintf(fp, "int bdevcnt = %d;\n", j);
	fprintf(fp, "int bdevswsz = %d;\n", bdevswsz);

/*
 * Go through character device table and indicate addresses of required
 * routines, or indicate "nulldev" if routine is not present.  If a
 * particular device is not present, fill in "nxio/nodev" entries.
 *
 * Add streamtab pointers for STREAMS drivers; they don't need
 * any other fields in this table to be filled in. 
 */
	empty_slot = 0;
	fprintf(fp, "\nstruct cdevsw cdevsw[] = {\n");
	for (rdrv = cdevices, j = 0; rdrv != NULL; ++j) {
		if (j >= rdrv->mdev.chr_start && !rdrv->loadable)
			drv = rdrv;
		else {
			empty_slot++;
			drv = &nodrv;
		}

		pfx = drv->mdev.prefix;
		name = drv->mdev.extname;
		mflags = drv->mdev.mflags;

                if (j != 0)
			fprintf(fp, ",\n");
                fprintf(fp, "/* %2d */  {\t", j);

		/*
		 * OPEN & CLOSE for char devices are special:
		 * if they are missing, they get nulldev instead of nodev;
		 * but if there's no driver at all, they should be nodev
		 * for CLOSE and nxio for OPEN.
		 */
		if (drv_has_entry(drv, edef_open))
			fprintf(fp, "%sopen,", pfx);
		else if (drv == rdrv)
			fprintf(fp, "nulldev,");
		else
			fprintf(fp, "nxio,");
		if (drv_has_entry(drv, edef_close))
			fprintf(fp, "\t%sclose,", pfx);
		else if (drv == rdrv)
			fprintf(fp, "\tnulldev,");
		else
			fprintf(fp, "\tnodev,");

		if (drv_has_entry(drv, edef_read))
			fprintf(fp, "\t%sread,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (drv_has_entry(drv, edef_write))
			fprintf(fp, "\t%swrite,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (drv_has_entry(drv, edef_ioctl))
			fprintf(fp, "\t%sioctl,\n", pfx);
		else
			fprintf(fp, "\tnodev,\n");
		if (drv_has_entry(drv, edef_mmap))
			fprintf(fp, "\t\t%smmap,", pfx);
		else
			fprintf(fp, "\t\tnodev,");
		if (drv_has_entry(drv, edef_segmap))
			fprintf(fp, "\t%ssegmap,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (drv_has_entry(drv, edef_chpoll))
			fprintf(fp, "\t%schpoll,", pfx);
		else
			fprintf(fp, "\tnodev,");
		if (drv_has_entry(drv, edef_msgio))
			fprintf(fp, "\t%smsgio,",pfx);
		else
			fprintf(fp, "\tnodev,");
		if (drv_has_entry(drv, edef_devinfo))
			fprintf(fp, "\t%sdevinfo,",pfx);
		else
			fprintf(fp, "\tmod_enosys,");

		{
			int ddi_level = find_ddi_level(drv);

			if (ddi_level == -1 || ddi_level > 5)
				fprintf(fp, "\t0,\n");
			else if (ddi_level == 5)
				fprintf(fp, "\n\t\tDAF_REQDMA,\n");
			else
				fprintf(fp, "\n\t\tDAF_REQDMA|DAF_PHYSREQ,\n");
		}

		if (INSTRING(mflags, STREAM))
			fprintf(fp, "\t\t&%sinfo,", pfx);
		else
			fprintf(fp, "\t\tnullinfo,");
		fprintf(fp, "\t\"%s\",", name);

		mdep_cdevsw(fp, drv);

		fprintf(fp, "\t&%sdevflag,", pfx);

		if ((cpu = drv->bind_cpu) == -1 &&
		    INSTRING(drv->mdev.mflags, HARDMOD))
			cpu = -2;
		fprintf(fp, "\t%d }", cpu);

		if (j == rdrv->mdev.chr_end)
			rdrv = rdrv->cdev_link;
        }

	if (empty_slot >= cdev_reserve)
		cdevswsz = j;
	else
		cdevswsz = j + cdev_reserve - empty_slot;

	if (cdevswsz < res_cdev_max)
		cdevswsz = res_cdev_max;

	if (cdevswsz == 0)
		fprintf(fp, "\t{ nodev }");
	while (j < cdevswsz) {
		fprintf(fp, ",\n/* %2d */  {\t", j);
		fprintf(fp, "nxio,\tnodev,\tnodev,\tnodev,\tnodev,\n");
		fprintf(fp, "\t\tnodev,\tnodev,\tnodev,\tnodev,\tnodev,\t0,\tnullinfo,\n");
		fprintf(fp, "\t\t\"nodev\",\t&nodevflag,\t-1 }");
		j++;
	}
        fprintf(fp, "\n};\n\n");
        fprintf(fp, "int cdevcnt = %d;\n", j);
        fprintf(fp, "int cdevswsz = %d;\n", cdevswsz);

/*
 * Print the constab table.  All CONSDEV console-capable devices are listed.
 */

	fprintf(fp, "\nstruct constab constab[] = {\n");
	j = 0;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* not configured or loadable */
		if (drv->n_ctlr == 0 || drv->loadable)	
                        continue;
                if (INSTRING(drv->mdev.mflags, CONSDEV)) {
			if (j++ != 0 )
				fprintf(fp, ",\n");
			if ((cpu = drv->bind_cpu) == -1 &&
			    INSTRING(drv->mdev.mflags, HARDMOD))
				cpu = -2;
			fprintf(fp, "\t{ \"%s\", &%sconssw, %d }",
				drv->mdev.extname, drv->mdev.prefix,
				cpu);
		}
	}
	if (j == 0)
		fprintf(fp, "\n\t{ NULL }");
        fprintf(fp, "\n};\n");
        fprintf(fp, "int conscnt = %d;\n", j);

/*
 * Generate consswp and consminor, which indicate our choice of
 * the default console.
 */
	if (consdrv == NULL) {
		/*
		 * If no default console specified, pick the first one
		 * and minor 0.
		 */
		for (drv = driver_info; drv != NULL; drv = drv->next) {
			/* not configured or loadable */
			if (drv->n_ctlr == 0 || drv->loadable)	
				continue;
			if (INSTRING(drv->mdev.mflags, CONSDEV))
				break;
		}
		consdrv = drv;
		consminor = 0;
	}
	fprintf(fp, "struct conssw *consswp = ");
	if (consdrv == NULL)
		fprintf(fp, "NULL;\n");
	else
		fprintf(fp, "&%sconssw;\n", consdrv->mdev.prefix);
	fprintf(fp, "minor_t consminor = %d;\n\n", consminor);

/*
 * Print the fmodsw table.  STREAMS installables are processed as follows:
 *
 * if mdevice flags field has:
 *      "S", this is a module, give it fmodsw entry (retain for back compat.)
 *      "Sm", this is a module, give it fmodsw entry (correct form)
 *      "Sc", this is a driver, no fmodsw entry.
 *      "Smc", this is a driver & module, give it fmodsw entry.
 */

        fprintf(fp, "\nstruct fmodsw fmodsw[] = {\n");
	j = 0;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* not configured or loadable */
		if (drv->n_ctlr == 0 || drv->loadable)	
                        continue;
                driver = module = 0;
                if (INSTRING(drv->mdev.mflags, STREAM)) {
                        if (INSTRING(drv->mdev.mflags, CHAR))
                                driver++;
                        if (INSTRING(drv->mdev.mflags, MOD))
                                module++;
                        if (!driver)
                                module++;
                        if (module) {
                                /* This one's a stream module. */
                                if (j++ != 0 )
					fprintf(fp, ",\n");
				fprintf(fp,
					"\t{ \"%s\", &%sinfo, &%sdevflag }",
					drv->mdev.extname,
					drv->mdev.prefix,
					drv->mdev.prefix);
                	}
        	}
	}

	fmodswsz = j + fmod_reserve;

        if (fmodswsz == 0)
                fprintf(fp, "\t{ \"\" }");
	for (i = j; i < fmodswsz; i++)
		fprintf(fp, ",\n\t{ \"\",\tNULL,\tNULL }");
        fprintf(fp, "\n};\n\n");
        fprintf(fp, "int fmodcnt = %d;\n\n", j);
        fprintf(fp, "int fmodswsz = %d;\n\n", fmodswsz);

/*
 * Print out device variable assignments.
 */

	for (assign = assign_info; assign != NULL; assign = assign->next) {
		int is_blk;

		is_blk = INSTRING(assign->driver->mdev.mflags, BLOCK);

		if (assign->driver->loadable) {
			pfmt(stderr, MM_ERROR, DEV_NOTLOAD, assign->sasgn.device);
			error(0);
		} else if (is_blk) {
			fprintf(fp, "dev_t\t%sdev = makedevice(%hd, %hd);\n",
				assign->sasgn.device,
				assign->driver->mdev.blk_start,
				assign->sasgn.minor);
			fprintf(fp, "boolean_t\t%sdev_ischar = B_FALSE;\n",
				assign->sasgn.device);
		} else {
			pfmt(stderr, MM_ERROR, DEV_NOTBLK, assign->sasgn.device);
			error(0);
		}
	}

/*
 * Print out dispatcher class table.
 */

	fprintf(fp, "\nextern void sys_init();\n");
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (drv->n_ctlr == 0 || !INSTRING(drv->mdev.mflags, DISP))
			continue;
		if (!drv_has_entry(drv, edef__init)) {
			pfmt(stderr, MM_ERROR, DINITRT, drv->mdev.name);
			error(0);
		}
		fprintf(fp, "extern void %s_init();\n", drv->mdev.prefix);
	}

	fprintf(fp, "class_t class[] = {\n");
	fprintf(fp, "\t{ \"SYS\", sys_init },\n");
	j = 1;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (drv->n_ctlr == 0 || !INSTRING(drv->mdev.mflags, DISP))
			continue;
		uppermap(drv->mdev.extname, caps);
		fprintf(fp, "\t{ \"%s\", %s_init },\n",
			caps, drv->mdev.prefix);
		j++;
	}
	fprintf(fp, "};\n\n");

	fprintf(fp, "int nclass = %d;\n\n", j);

	print_execsw(fp);

	print_vfssw(fp);

	mdep_prconf(fp);

	mdep_prvec(fp);
	print_static(fp, ndrv_static);

	print_mod_stub_tab(fp);

	print_interfaces(fp);

	print_name_to_modwrapper_list(fp);

        fclose(fp);
}


/*
 *  Output a table mapping module name to wrapper addresses
 *  for each loadable module which is statically configured.
 */

static void
print_name_to_modwrapper_list(fp)
FILE *fp;
{
	driver_t *drv;

	fprintf(fp, "\n");

	for (drv = driver_info; drv != NULL; drv = drv->next)
	{
		if (drv->n_ctlr == 0 || !drv->conf_static ||
		    !INSTRING(drv->mdev.mflags, LOADMOD))
                        continue;

		fprintf(fp, "extern struct modwrapper %s_wrapper;\n",
					drv->mdev.prefix);
	}

	fprintf(fp, "\nstruct {char *name; struct modwrapper *mw;} name_to_modwrapper[] = {\n");

	for (drv = driver_info; drv != NULL; drv = drv->next)
	{
		if (drv->n_ctlr == 0 || !drv->conf_static ||
		    !INSTRING(drv->mdev.mflags, LOADMOD))
                        continue;

		fprintf(fp, "\t{\"%s\", &%s_wrapper},\n",
			drv->mdev.name, drv->mdev.prefix);
	}

	fprintf(fp, "\t{0, 0}\n};\n");
}


/* is_ftab -- true if sym is an ftab table name */
static int
is_ftab(sym)
char *sym;
{
	extern struct ftab_def *ftab_defs;
	struct ftab_def *ftab;

	for (ftab = ftab_defs; ftab != NULL; ftab = ftab->next) {
		if (strcmp(ftab->tabname, sym) == 0)
			return 1;
	}
	return 0;
}


static void
print_func_tables(fp)
FILE *fp;
{
	extern struct ftab_def *ftab_defs;
	struct ftab_def *ftab;

	for (ftab = ftab_defs; ftab != NULL; ftab = ftab->next) {
		print_func_table(fp, ftab);
		if (INSTRING(ftab->fflags, DRV))
			print_cpubind_table(fp, ftab);
	}
}

static void
print_func_table(fp, ftab)
FILE *fp;
register struct ftab_def *ftab;
{
	register driver_t *drv;

	/* First pass for declarations */
	fprintf(fp, "\n");
        for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* For loadable modules or not configured modules, just skip it */
		if (drv->n_ctlr == 0 || drv->loadable)
                        continue;
		/* For "init" table, skip filesystem modules */
		if (ftab->entry == edef_init &&
		    INSTRING(drv->mdev.mflags, FILESYS))
			continue;
		if (drv_has_entry(drv, ftab->entry)) {
			fprintf(fp, "extern %s %s%s();\n",
				    ftab->ret_type,
				    drv->mdev.prefix, ftab->entry->suffix);
		}
        }

	/* Second pass to generate table */
        fprintf(fp, "\n%s (*%s[])() = {\n", ftab->ret_type, ftab->tabname);
        for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* For loadable modules or not configured modules, just skip it */
		if (drv->n_ctlr == 0 || drv->loadable)
                        continue;
		/* For "init" table, skip filesystem modules */
		if (ftab->entry == edef_init &&
		    INSTRING(drv->mdev.mflags, FILESYS))
			continue;
		if (drv_has_entry(drv, ftab->entry)) {
			fprintf(fp, "\t%s%s,\n",
				    drv->mdev.prefix, ftab->entry->suffix);
		}
        }
        fprintf(fp, "\t(%s (*)())0\n};\n", ftab->ret_type);
}

static void
print_cpubind_table(fp, ftab)
FILE *fp;
register struct ftab_def *ftab;
{
	register driver_t *drv;
	int cpu;

        fprintf(fp, "\nint bindcpu_%s[] = {\n", ftab->entry->suffix);
        for (drv = driver_info; drv != NULL; drv = drv->next) {
		/* For loadable modules or not configured modules, just skip it */
		if (drv->n_ctlr == 0 || drv->loadable)
                        continue;
		/* For "init" table, skip filesystem modules */
		if (ftab->entry == edef_init &&
		    INSTRING(drv->mdev.mflags, FILESYS))
			continue;
		if (drv_has_entry(drv, ftab->entry)) {
			if ((cpu = drv->bind_cpu) == -1 &&
			    INSTRING(drv->mdev.mflags, HARDMOD))
				cpu = -2;
			if (INSTRING(drv->mdev.mflags, BLOCK) ||
			    INSTRING(drv->mdev.mflags, CHAR) ||
			    INSTRING(drv->mdev.mflags, HARDMOD))
				fprintf(fp, "\t%d,\n", cpu);
			else
				fprintf(fp, "\t-1,\n");
		}
        }
        fprintf(fp, "};\n");
}


static void
print_execsw(fp)
FILE *fp;
{
	register driver_t *drv;
	register char *pfx;
	int n_magic, dflt_hdlr;
	int j, k;

        fprintf(fp, "static struct execsw static_execsw[] = {\n");
	j = 0;
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!INSTRING(drv->mdev.mflags, EXECSW) || drv->n_ctlr == 0 ||
		    drv->loadable)
			continue;

		n_magic = drv->mdev.magics->nmagic;
		dflt_hdlr = drv->mdev.magics->wildcard;
		pfx = drv->mdev.prefix;

		for (k = 0; k < n_magic + dflt_hdlr; k++) {
			if (j++ != 0) {
				fprintf(fp, "static_execsw+%d },\n", j-1);
			}
			if (k < n_magic)
				fprintf(fp, "\t{ %smagic+%d", pfx, k);
			else
				fprintf(fp, "\t{ NULL");  /* dflt_hdlr */

			fprintf(fp, ", %d, \"%s\", &%sexecsw_info, ",
				drv->mdev.order, drv->mdev.extname, pfx);
		}
	}
	if (j == 0)	/* empty table */
		fprintf(fp, "\t{ 0, 0, 0, 0, 0 }");
	else
		fprintf(fp, "NULL }");

        fprintf(fp, "\n};\n\n");  /* end of execsw initialization */

        fprintf(fp, "int nexectype = %d;\n", j);
	fprintf(fp, "struct execsw *execsw = &static_execsw[0];\n");
}

static void
print_mod_stub_tab(fp)
FILE	*fp;
{
	driver_t	*drv;
	char		*pfx;

	fprintf(fp, "\n");
	for (drv = driver_info; drv != NULL; drv = drv->next)	{
		if (drv->n_ctlr == 0 || !drv->loadable ||
		    !INSTRING(drv->mdev.mflags, STUBMOD))
			continue;

		fprintf(fp, "extern struct mod_stub_modinfo %s_modinfo;\n",
			drv->mdev.name);
	}

	fprintf(fp, "struct mod_stub_modinfo *mod_stub_tab[] = {\n");

	for (drv = driver_info; drv != NULL; drv = drv->next)	{
		if (drv->n_ctlr == 0 || !drv->loadable ||
		    !INSTRING(drv->mdev.mflags, STUBMOD))
			continue;

		fprintf(fp, "\t&%s_modinfo,\n", drv->mdev.name);
	}

	fprintf(fp, "\tNULL\n};\n");
}


/*
 *  Print the configuration info for file system types that
 *  are to be included in the system.
 */
static void
print_vfssw(fp)
FILE *fp;
{
	register driver_t *drv;
        register int j, i;

/* First, search through the fstype table and print an
 * extern declaration for all the switch functions that
 * are supposed to be provided in the configured file types.
 */
        /*  
         *  start declaring the file system external init routines
         *  format is 
         *  extern int prefix+init();
         */

	fprintf(fp, "\n");

        /* loop for each configured type */
        for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!INSTRING(drv->mdev.mflags, FILESYS) || drv->n_ctlr == 0
			|| drv->loadable)
			continue;

		if (!drv_has_entry(drv, edef_init)) {
			pfmt(stderr, MM_ERROR, FINITRT, drv->mdev.name);
			error(0);
		}
                fprintf(fp, "extern int %sinit();\n", drv->mdev.prefix);
        }

/*
 * Next, set up and initialize the vfssw data structure table.
 */
        fprintf(fp, "\nstruct vfssw vfssw[] = {\n");
	/* first, initialize the NULL fs type  */
        fprintf(fp, "\t{ \"EMPTY\",\t(int (*)())0 }");
	j = 1;

        for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!INSTRING(drv->mdev.mflags, FILESYS) || drv->n_ctlr == 0
			|| drv->loadable)
			continue;

		fprintf(fp, ",\n\t{ \"%s\", %sinit }",
			drv->mdev.extname, drv->mdev.prefix);
		j++;
        }

	vfsswsz = j + vfs_reserve;

	for (i = j; i < vfsswsz; i++)
		fprintf(fp, ",\n\t{ \"\", (int (*)())0 }");

        fprintf(fp, "\n};\n\n");

        fprintf(fp, "int nfstype = %d;\n", j);
        fprintf(fp, "int vfsswsz = %d;\n", vfsswsz);
	fprintf(fp, "\n");
}


/* Compile and link together necessary files for each module;
 * create linker input file which lists modules to be linked together.
 */

void
compfiles()
{
	ctlr_t *kctlr;
	driver_t *kernel;
        driver_t *drv;
        FILE *ifp;

	if (tfind("DEF_UNLOAD_DELAY") == NULL)
		no_def_delay++;

	if (access(moddir, F_OK) != 0) {
		if (mkdir(moddir, DIR_MODE) != 0) {
			pfmt(stderr, MM_ERROR, ":236:Can't make directory %s\n",
					moddir);
			fatal(0);
		}
	}

	if (!mflag) {
		ifp = open1(ifile, "w", OUT);
		chmod(ifile, file_mode);
	}

	/* process "kernel" module first */
	if ((kctlr = sfind("kernel")) != NULL &&
	    !(kernel = kctlr->driver)->skip)
		comp_drv(kernel, ifp);

        /* process other modules */
	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (drv->skip || drv == kernel)
			continue;
		comp_drv(drv, ifp);
	}

	if (!mflag) {
		/*
		 * Tell linker to include conf.o;
		 * it has to be compiled later, though.
		 */
		fprintf(ifp, "%s/conf.o\n", output);

		fclose(ifp);
	}
}

/*
 * Write files in the directory 'name' to the file of objects for
 * the kernel we're building.
 */
static void
comp_drv(drv, ifp)
driver_t *drv;
FILE *ifp;
{
	char files[512];
	struct entry_list *elistp;
	char outfile[PATH_MAX];
	int stat;

	do_chdir(pkdir);
	do_chdir(drv->mdev.name);

	strcpy(outfile, "_drv.o");

        /* pick up stubs.c if driver's not configured */
        if (drv->n_ctlr == 0) {
		if (access("stubs.c", F_OK) == 0) {
			strcpy(files, "stubs.c");
			goto got_files;
		}
                return; /* done with this one */
        }

        /* check for Driver.o and add to file list */
	if (access(drv->fname, F_OK) != 0) {
		/* Require a Driver.o if 'Y' in sdevice  */
		pfmt(stderr, MM_ERROR, ":237:Cannot find Driver.o for %s (%s).\n",
			drv->mdev.name, drv->fname);
		error(0);
		return;
        }
	strcpy(files, drv->fname);

	if (access("space.c", F_OK) == 0) {
		if (drv->loadable) {
			add_pragma("space.c", "_space.c", "drv_");
			strcat(files, " _space.c");
		} else
			strcat(files, " space.c");
	}

	if (access("tune.c", F_OK) == 0) {
		if (drv->loadable) {
			add_pragma("tune.c", "_tune.c", "drv_");
			strcat(files, " _tune.c");
		} else
			strcat(files, " tune.c");
	}

	if (drv->loadable) {
		prmod(drv);
		prsec(drv);
		strcat(files, " mod_conf.c mod_sec.s");

		sprintf(outfile, "%s/%s", moddir, drv->mdev.name);

		prmodreg(drv);
	}

got_files:
	/* compile/link the file(s) */
	stat = compile(files, outfile, drv->mdev.name);

	if (drv->loadable) {
		(void) unlink("_space.c");
		(void) unlink("_tune.c");
	}

	if (stat != 0) {
		(void) unlink(outfile);
		exit(1);
	}

	/* examine and/or modify the symbol table */
	if (load_symtab(outfile, drv->loadable) != 0) {
		pfmt(stderr, MM_ERROR, ":238:Error loading %s module symbol table from %s\n",
				drv->mdev.name, outfile);
		error(0);
		return;
	}

	if (drv->n_ctlr != 0 && drv->loadable) {
		if (!mflag && access("Modstub.o", F_OK) == 0) {
			/* tell linker to pick up the stubs */
			fprintf(ifp, "%s/Modstub.o\n", drv->mdev.name);
		}
        } else {
		/* check symbols for interface conformance; remap if needed */
		check_interfaces(drv);

		if (!mflag) {
			/* tell linker to pick up this module */
			fprintf(ifp, "%s/_drv.o\n", drv->mdev.name);
		}
	}

	if (drv->n_ctlr == 0) {
		close_symtab();
		return;
	}

	/* Determine which entry-points are actually present */
	lookup_entries(drv->mdev.prefix, scan_symbols);

	/* Make sure all specified entry-points are present */
	elistp = drv->mdev.entries;
	for (; elistp != NULL; elistp = elistp->next) {
		if (elistp->edef->has_sym)
			continue;

		pfmt(stderr, MM_ERROR, ENTRYNP, drv->mdev.name, elistp->edef->suffix);
		if (strcmp(elistp->edef->suffix, "_wrapper") == 0)
			pfmt(stderr, MM_ERROR, ":239:%s is not loadable\n", drv->mdev.name);

		fatal(0);
	}

	/* Make sure required variables are present */
	if (INSTRING(drv->mdev.mflags, BLOCK) ||
	    INSTRING(drv->mdev.mflags, CHAR) ||
	    INSTRING(drv->mdev.mflags, STREAM)) {
		if (!edef_devflag->has_sym) {
			pfmt(stderr, MM_ERROR, ":240:%sdevflag missing from '%s' module\n",
				drv->mdev.prefix, drv->mdev.name);
			fatal(0);
		}
	}
	if (INSTRING(drv->mdev.mflags, STREAM)) {
		if (!edef_info->has_sym) {
			pfmt(stderr, MM_ERROR, STRTAB, drv->mdev.name);
			fatal(0);
		}
	}
	if (INSTRING(drv->mdev.mflags, CONSDEV)) {
		if (!edef_conssw->has_sym) {
			pfmt(stderr, MM_ERROR, CONSTAB, drv->mdev.name);
			fatal(0);
		}
	}
	if (INSTRING(drv->mdev.mflags, FILESYS) && drv->loadable) {
		if (!edef__vfsops->has_sym) {
			pfmt(stderr, MM_ERROR, ":241:%s_vfsops missing from '%s' module\n",
				drv->mdev.prefix, drv->mdev.name);
			fatal(0);
		}
		if (!edef__fsflags->has_sym) {
			pfmt(stderr, MM_ERROR, ":242:%s_fsflags missing from '%s' module\n",
				drv->mdev.prefix, drv->mdev.name);
			fatal(0);
		}
	}

	close_symtab();
}

int
try_file(fname, files)
char *fname;
char files[];
{
	if (access(fname, F_OK) == 0) {
		sprintf(files + strlen(files), " %s", fname);
		return 1;
	}
	return 0;
}


static void
prsec(drv)
driver_t *drv;
{
	FILE *fp;

	if (qflag && access(sfile, F_OK) == 0)
		return;

	fp = open1(sfile, "w", FULL);
	chmod(sfile, file_mode);
	fdep_prsec(fp, drv);
	fclose(fp);
}


void
compconf()
{
	if (new_cfile) {
		/* compile conf.c */
		do_chdir(output);
		if (compile(cfile, NULL, cfile) != 0)
			exit(1);
	}
}


/* routines for handling dynamic loadable modules */

static void
prmod(drv)
driver_t *drv;
{
	register FILE *fp;
	char *pfx, *intr_pfx, *mflags;
	unsigned short type;
	char caps[PFXSZ];
	char delay_tune[TUNESZ];
	struct interface_list *ilp;
	unsigned n, nver;
	int cpu;

	if (qflag && access(mfile, F_OK) == 0)
		return;

	fp = open1(mfile, "w", FULL);
	chmod(mfile, file_mode);

	type = 0;
	pfx = drv->mdev.prefix;
	mflags = drv->mdev.mflags;

	if ((cpu = drv->bind_cpu) == -1 &&
	    INSTRING(drv->mdev.mflags, HARDMOD))
		cpu = -2;

	if (INSTRING(mflags, BLOCK))
		type |= MODBDEV;

	if (INSTRING(mflags, CHAR))
		type |= MODCDEV;

	if (INSTRING(mflags, STREAM))
		type |= MODSTR;

	if (INSTRING(mflags, FILESYS))
		type |= MODFS;

	if (INSTRING(mflags, EXECSW))
		type |= MODEXEC;

	if (INSTRING(mflags, HARDMOD))
		type |= MODHARD;

	if ((type & MODSTR) && (!(type & MODCDEV) || INSTRING(mflags, MOD)))
		type |= MODMOD;

	fprintf(fp, "#include <config.h>\n");
	fprintf(fp, "#include <sys/types.h>\n");
	fprintf(fp, "#include <sys/conf.h>\n");

	if (type & MODINTR) {
		fprintf(fp, "#include <sys/param.h>\n");
		fprintf(fp, "#include <sys/moddrv.h>\n");
	}

	if (type & MODMOD) {
		fprintf(fp, "#include <sys/stream.h>\n");
		fprintf(fp, "#include <sys/conf.h>\n");
	}
		
	if (type & MODFS) {
		fprintf(fp, "#include <sys/vfs.h>\n");
		fprintf(fp, "#include <sys/modfs.h>\n");
	}

	if (type & MODEXEC) {
		fprintf(fp, "#include <sys/exec.h>\n");
		fprintf(fp, "#include <sys/modexec.h>\n");
	}

	print_mod_pragma(fp, "drv_");

	uppermap(pfx, caps);
	sprintf(delay_tune, "%s_UNLOAD_DELAY", caps);
	if (tfind(delay_tune) == NULL) {
		fprintf(fp, "#define %s_UNLOAD_DELAY\t%s\n",
			caps, no_def_delay ? "0" : "DEF_UNLOAD_DELAY");
	}

	fprintf(fp, "\n");

	if (type & (MODDRV | MODMOD))
		fprintf(fp, "extern int %sdevflag;\n", intr_pfx = pfx);
	else if (type & MODINTR) {
		fprintf(fp, "static int updevflag = 0;\n");
		intr_pfx = "up";
	}

	if (type & MODSTR)
		fprintf(fp, "extern struct streamtab %sinfo;\n", pfx);

	if (type & MODFS) {
		fprintf(fp, "extern vfsops_t %s_vfsops;\n", pfx);
		fprintf(fp, "extern unsigned long %s_fsflags;\n", pfx);
	}

	if (type & MODDRV) {
		fprintf(fp, "extern int nodev();\n");
		fprintf(fp, "extern int nulldev();\n");
		fprintf(fp, "extern int mod_enosys();\n");
	}

	if ((type & MODINTR) && drv->intr_decl) {
		if (drv_has_entry(drv, edef_intr))
			fprintf(fp, "extern void %sintr();\n", pfx);
		else
			fprintf(fp, "extern void intnull();\n");
	}

	if ((type & MODDRV) && !(type & MODSTR)) {
		if (drv_has_entry(drv, edef_open))
			fprintf(fp, "extern int %sopen();\n", pfx);
		else if (type & MODBDEV) {
			pfmt(stderr, MM_ERROR, OPRT, drv->mdev.name);
			error(0);
		}

		if (drv_has_entry(drv, edef_close))
			fprintf(fp, "extern int %sclose();\n", pfx);
		else if (type & MODBDEV) {
			pfmt(stderr, MM_ERROR, CLRT, drv->mdev.name);
			error(0);
		}

		if (type & MODBDEV) {
			if (!drv_has_entry(drv, edef_strategy)) {
				pfmt(stderr, MM_ERROR, STRAT, drv->mdev.name);
				error(0);
			}
			fprintf(fp, "extern int %sstrategy();\n", pfx);

			if (drv_has_entry(drv, edef_size))
				fprintf(fp, "extern int %ssize();\n", pfx);
		}
		if (drv_has_entry(drv, edef_devinfo))
			fprintf(fp, "extern int %sdevinfo();\n", pfx);
		if (drv_has_entry(drv, edef_read))
			fprintf(fp, "extern int %sread();\n", pfx);
		if (drv_has_entry(drv, edef_write))
			fprintf(fp, "extern int %swrite();\n", pfx);
		if (drv_has_entry(drv, edef_ioctl))
			fprintf(fp, "extern int %sioctl();\n", pfx);
		if (drv_has_entry(drv, edef_mmap))
			fprintf(fp, "extern int %smmap();\n", pfx);
		if (drv_has_entry(drv, edef_segmap))
			fprintf(fp, "extern int %ssegmap();\n", pfx);
		if (drv_has_entry(drv, edef_chpoll))
			fprintf(fp, "extern int %schpoll();\n", pfx);
	}

	if (type & MODDRV) {
		fprintf(fp, "\nstruct mod_drv_data %s_mod_drvdata = {\n", pfx);

		if (type & MODBDEV) {
			fprintf(fp, "\t{%sopen,", pfx);
			fprintf(fp, " %sclose,", pfx);
			fprintf(fp, " %sstrategy,", pfx);

			if (drv_has_entry(drv, edef_size))
				fprintf(fp, " %ssize,", pfx);
			else
				fprintf(fp, " nulldev,");

			if (drv_has_entry(drv, edef_devinfo))
				fprintf(fp, " %sdevinfo,", pfx);
			else
				fprintf(fp, " mod_enosys,");

			fprintf(fp, "\n\t\t");

			fprintf(fp, " \"%s\",", drv->mdev.extname);
			fprintf(fp, " NULL,");

			mdep_bdevsw(fp, drv);

			fprintf(fp, " &%sdevflag,", pfx);

			fprintf(fp, " %d },\n", cpu);

			fprintf(fp, "\t%s_BMAJOR_0, %s_BMAJORS,\n", caps, caps);
		} else {
			fprintf(fp, "\t{nodev, nodev, nodev, nodev, nodev,\n\t\tNULL, NULL, ");
			mdep_bdevsw(fp, drv);
			fprintf(fp, "NULL, -1 },\n");
			fprintf(fp, "\t0, 0,\n");
		}

		if ((type & (MODCDEV | MODSTR)) == MODCDEV) {
			if (drv_has_entry(drv, edef_open))
				fprintf(fp, "\t{%sopen,", pfx);
			else
				fprintf(fp, "\t{nulldev,");

			if (drv_has_entry(drv, edef_close))
				fprintf(fp, "%sclose,", pfx);
			else
				fprintf(fp, "nulldev,");

			if (drv_has_entry(drv, edef_read))
				fprintf(fp, "%sread,", pfx);
			else
				fprintf(fp, "nodev,");

			if (drv_has_entry(drv, edef_write))
				fprintf(fp, "%swrite,", pfx);
			else
				fprintf(fp, "nodev,");

			if (drv_has_entry(drv, edef_ioctl))
				fprintf(fp, "%sioctl,\n", pfx);
			else
				fprintf(fp, "nodev,\n");

			if (drv_has_entry(drv, edef_mmap))
				fprintf(fp, "\t\t%smmap,", pfx);
			else
				fprintf(fp, "\t\tnodev,");

			if (drv_has_entry(drv, edef_segmap))
				fprintf(fp, "%ssegmap,", pfx);
			else
				fprintf(fp, "nodev,");

			if (drv_has_entry(drv, edef_chpoll))
				fprintf(fp, "%schpoll,", pfx);
			else
				fprintf(fp, "nodev,");

			if (drv_has_entry(drv, edef_msgio))
				fprintf(fp, "%smsgio,", pfx);
			else
				fprintf(fp, "nodev,");

			if (drv_has_entry(drv, edef_devinfo))
				fprintf(fp, "%sdevinfo,", pfx);
			else
				fprintf(fp, "mod_enosys,");

			fprintf(fp, "NULL,\n");

			fprintf(fp, "NULL, \t\t\"%s\",", drv->mdev.extname);

			mdep_cdevsw(fp, drv);

			fprintf(fp, "&%sdevflag,", pfx);

			fprintf(fp, " %d },\n", cpu);

		} else {
			if (type & MODSTR)
				fprintf(fp, "\t{nulldev, nulldev,");
			else
				fprintf(fp, "\t{nodev, nodev,");

			fprintf(fp, " nodev, nodev, nodev, nodev, nodev,\n\t\tnodev, nodev, nodev,");

			if (type & MODSTR) 
				fprintf(fp, " NULL, &%sinfo, \"%s\", ", 
					pfx, drv->mdev.extname);
			else
				fprintf(fp, " NULL, NULL, NULL, ");

			mdep_cdevsw(fp, drv);

			if (type & MODSTR) {
				fprintf(fp, "&%sdevflag,", pfx);
				fprintf(fp, " %d },\n", cpu);
			} else {
				fprintf(fp, "NULL,");
				fprintf(fp, " -1 },\n");
			}
		}
		if (type & MODCDEV)
			fprintf(fp, "\t%s_CMAJOR_0, %s_CMAJORS };\n", caps, caps);
		else
			fprintf(fp, "\t0, 0 };\n");
	}

	if ((type & MODINTR) && drv->intr_decl && !drv->autoconf) {
		fprintf(fp, "\nstruct mod_drvintr %s_attach_info = {\n", pfx);
		fprintf(fp, "\tMOD_INTR_MAGIC, MOD_INTR_VER, \"%s\",",
			    drv->mdev.extname);
		fprintf(fp, " &%sdevflag,", intr_pfx);
		if (drv_has_entry(drv, edef_intr))
			fprintf(fp, " %sintr\n};\n", pfx);
		else
			fprintf(fp, " intnull\n};\n");
	}

	if (type & MODMOD) {
		fprintf(fp, "\nstruct fmodsw %s_mod_strdata = {\n", pfx);
		fprintf(fp, "\t\"%s\",\n\t&%sinfo,\n", drv->mdev.extname, pfx);
		fprintf(fp, "\t&%sdevflag\n};\n", pfx);
	}

	if (type & MODFS) {
		fprintf(fp, "\nstruct mod_fs_data %s_mod_fsdata = {\n", pfx);
		fprintf(fp, "\t\"%s\",\n\t&%s_vfsops,\n\t&%s_fsflags,\n};\n",
			drv->mdev.extname, pfx, pfx, pfx);
	}

	if (type & MODEXEC) {
		int has_core = 0;
		int has_textinfo = 0;

		if (!drv_has_entry(drv, edef_exec)) {
			pfmt(stderr, MM_ERROR, EXRT, drv->mdev.name);
			error(0);
		}
		fprintf(fp, "extern int %sexec();\n", pfx);

		if (drv_has_entry(drv, edef_core)) {
			fprintf(fp, "extern int %score();\n", pfx);
			has_core = 1;
		}
		if (drv_has_entry(drv, edef_textinfo)) {
			fprintf(fp, "extern int %stextinfo();\n", pfx);
			has_textinfo = 1;
		}
		fprintf(fp, "extern int mod_enosys();\n\n");

		fprintf(fp, "struct mod_exec_data %s_mod_execdata = {\n", pfx);
		fprintf(fp, "\t\"%s\",\n", drv->mdev.extname);
		fprintf(fp, "\t{ %sexec, ", pfx);

		if (has_core) {
			fprintf(fp, "%score, ", pfx);
		}
		else	{
			fprintf(fp, "mod_enosys, ");
		}
		if (has_textinfo) {
			fprintf(fp, "%stextinfo", pfx);
		}
		else	{
			fprintf(fp, "mod_enosys");
		}

		fprintf(fp, ", NULL },\n\tNULL\n};\n");
	}

	fprintf(fp, "struct mod_conf_data %s_conf_data = {\n", pfx);
	fprintf(fp, "\tMCD_VERSION,\n");
	fprintf(fp, "\t%s_UNLOAD_DELAY\n", caps);
	fprintf(fp, "};\n");

	fclose(fp);
}

static void
prmodreg(drv)
driver_t *drv;
{
	FILE *fp;
	char sym[128];
	char *mflags;
	int i;
	int entry;

	if (qflag && access(gfile, F_OK) == 0)
		return;

	fp = open1(gfile, "w", FULL);
	chmod(gfile, file_mode);

	entry = 0;
	mflags = drv->mdev.mflags;
	if (INSTRING(mflags, BLOCK)) {
		for (i = drv->mdev.blk_start; i <= drv->mdev.blk_end; i++)
			fprintf(fp, "%d:%d:%s:%d\n", MOD_TY_BDEV,
				MOD_C_MREG, drv->mdev.name, i);
		entry++;
	}

	if (INSTRING(mflags, CHAR) && !INSTRING(mflags, STREAM)) {
		for (i = drv->mdev.chr_start; i <= drv->mdev.chr_end; i++)
			fprintf(fp, "%d:%d:%s:%d\n", MOD_TY_CDEV,
				MOD_C_MREG, drv->mdev.name, i);
		entry++;
	}

	if (INSTRING(mflags, STREAM) && INSTRING(mflags, CHAR)) {
		for (i = drv->mdev.chr_start; i <= drv->mdev.chr_end; i++)
			fprintf(fp, "%d:%d:%s:%d\n", MOD_TY_SDEV,
				MOD_C_MREG, drv->mdev.name, i);
		entry++;
	}

	if (INSTRING(mflags, STREAM) && (!INSTRING(mflags, CHAR) ||
					INSTRING(mflags, MOD))) {
		fprintf(fp, "%d:%d:%s:%s\n", MOD_TY_STR, MOD_C_MREG, 
			drv->mdev.name, drv->mdev.extname);
		entry++;
	}

	if (INSTRING(mflags, FILESYS)) {
		fprintf(fp, "%d:%d:%s:%s\n", MOD_TY_FS, MOD_C_MREG, 
			drv->mdev.name, drv->mdev.extname);
		entry++;
	}

	if (INSTRING(mflags, EXECSW)) {
		int	n_magic;
		struct	magic_list *mlist;

		mlist = drv->mdev.magics;
		n_magic = mlist->nmagic;
		fprintf(fp, "%d:%d:%s:%d,%c",
			MOD_TY_EXEC,
			MOD_C_MREG, 
			drv->mdev.name,
			drv->mdev.order,
			(mlist->wildcard ? 'Y' : 'N'));
		for (i = 0; i < n_magic; )
			fprintf(fp, ",%d", mlist->magics[i++]);
		fprintf(fp, "\n");

		entry++;
	}

	if (!entry)
		fprintf(fp, "%d:%d:%s:%s\n", MOD_TY_MISC, MOD_C_MREG, 
			drv->mdev.name, drv->mdev.extname);
	fclose(fp);
}

#ifdef __STDC__
static int
qstrcmp(const void *s1, const void *s2)
{
	return strcmp(s1, s2);
}
#else
#define qstrcmp strcmp
#endif

static void
print_static(fp, ndrv)
FILE *fp;
int ndrv;
{
	driver_t *drv;
	char *static_modules;
	int j;
	char *buf;
	
	if ((static_modules = (char *)malloc(ndrv * NAMESZ)) == NULL) {
		pfmt(stderr, MM_ERROR, ":243:can't allocate memory for static_modules.\n");
		error(0);
	}
	
	for (drv = driver_info, buf = static_modules; drv != NULL; drv = drv->next) {
		/* not configured or loadable */
		if (drv->n_ctlr == 0 || drv->loadable)	
                        continue;
		strcpy(buf, drv->mdev.name);
		buf += NAMESZ;
	}

	qsort((void *)static_modules, ndrv, NAMESZ, qstrcmp);

	fprintf(fp, "\nchar *static_modules[] = {\n");
	for (j = 0, buf = static_modules; j < ndrv; j++, buf += NAMESZ)
		fprintf(fp, "\t\"%s\",\n", buf);
	fprintf(fp, "\tNULL };\n");
}

/* return the prefix of "idmkunix" */

static char *
getpref(cp)
char *cp;	/* how idmkunix was called */
{
	static char	tprefix[128];  /* enough room for prefix and \0 */
	int		cmdlen,
			preflen;
	char		*prefptr,
			*name;

	name = "idmkunix";
	if ((prefptr= strrchr(cp,'/')) == NULL)
		prefptr=cp;
	else
		prefptr++;
	cmdlen= strlen(prefptr);
	preflen= cmdlen - strlen(name);
	if ( (preflen < 0 )		/* if invoked with a name shorter
					   than name */
	    || (strcmp(prefptr + preflen, name) != 0)) {
		pfmt(stderr, MM_ERROR, ":248:command name must end in \"%s\"\n", name);
		exit(1);
		/*NOTREACHED*/
	} else {
		(void) strncpy(tprefix,prefptr,preflen);
		tprefix[preflen]='\0';
		return(tprefix);
	}
}


/* Add pragma at head of file to generate data into specific sections.
 * The function must be designed so that the original file is untouched
 * in case idmkunix is interrupted.
 */
static void
add_pragma(file, ofile, secpfx)
char *file, *ofile;
char *secpfx;
{
	FILE *fp;
	char buf[50];

	if (debug)
		fprintf(stderr, "Remap sections in %s\n", file);

	if ((fp = fopen(ofile, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, FOPEN, ofile, "w");
		fatal(0);
	}
	print_mod_pragma(fp, secpfx);
	fclose(fp);

	strcpy(buf, "cat ");
	strcat(buf, file);
	strcat(buf, " >> ");
	strcat(buf, ofile);
	(void) proc(buf);
}

static void
print_mod_pragma(fp, secpfx)
FILE *fp;
char *secpfx;
{
	fprintf(fp, "#pragma section_map text \".%stext\"\n", secpfx);
	fprintf(fp, "#pragma section_map data \".%sdata\"\n", secpfx);
	fprintf(fp, "#pragma section_map data1 \".%sdata1\"\n", secpfx);
	fprintf(fp, "#pragma section_map rodata \".%srodata\"\n", secpfx);
	fprintf(fp, "#pragma section_map rodata1 \".%srodata1\"\n", secpfx);
}

/*
 * Print out interface structures.
 */

static void
print_interfaces(fp)
FILE *fp;
{
	struct intfc *intp, *intp2;
	struct intfc_sym *symp;
	struct depend_list *dep;
	unsigned int n;

	fprintf(fp, "\n/* Interface tables */\n\n");

	/*
	 * First, assign numbers to all the interfaces.
	 */
	for (n = 0, intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver)
			intp2->count = n++;
	}

	/*
	 * Generate symbol arrays used by each interface.
	 */
	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			fprintf(fp, "static struct intfc_sym _sym%d[] = {\n",
				intp2->count);
			for (symp = intp2->symbols; symp; symp = symp->next) {
				fprintf(fp, "\t{ \"%s\", ", symp->symname);
				if (strcmp(symp->newname, "$dropped") == 0)
					fprintf(fp, "SYM_DROPPED },\n");
				else if (symp->newname[0] != '\0')
					fprintf(fp, "\"%s\" },\n",
						symp->newname);
				else
					fprintf(fp, "0 },\n");
			}
			fprintf(fp, "\t{ 0 }\n};\n");
		}
	}

	/*
	 * Now generate the interface structures themselves.
	 */
	fprintf(fp, "\nstruct interface interfaces[] = {\n");
	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			fprintf(fp, "\t{ \"%s\", \"%s\", _sym%d, \"",
				intp2->name, intp2->version, intp2->count);
			for (dep = intp2->depends; dep; dep = dep->next)
				fprintf(fp, " %s", dep->name);
			fprintf(fp, "\",\n");
			if (intp2->next_intfc)
				fprintf(fp, "\t  &interfaces[%d],",
					intp2->next_intfc->count);
			else
				fprintf(fp, "\t  0,");
			if (intp2->next_ver)
				fprintf(fp, " &interfaces[%d],",
					intp2->next_ver->count);
			else
				fprintf(fp, " 0,");
			if (intp2->rep_intfc)
				fprintf(fp, " &interfaces[%d],",
					intp2->rep_intfc->count);
			else
				fprintf(fp, " 0,");
			fprintf(fp, " %d },\n", intp2->order);
		}
	}
	fprintf(fp, "\t{ 0 }\n};\n");
}

/*
 * Compile a set of files, with appropriate options.
 */
static int
compile(files, outfile, name)
char *files, *outfile, *name;
{
	char cmd[PATH_MAX];
	char *cmdp, *filestart;
	char *filep, *endp;
	struct stat statb, statb2;
	int rc, chr;

	/* See if we can avoid rebuilding files which already exist. */
	if (qflag && outfile && stat(outfile, &statb) == 0) {
		for (filep = files; *filep != '\0';) {
			if ((endp = strchr(filep, ' ')) == NULL)
				endp = filep + strlen(filep);
			chr = *endp;
			rc = stat(filep, &statb2);
			*endp = chr;
			if (rc != 0)
				break;
			if (statb2.st_mtime >= statb.st_mtime)
				break;
			if (*endp == '\0') {
				/*
				 * All files are older than the output file.
				 * We can skip the compile/link.
				 */
				return 0;
			}
			filep = endp + 1;
		}
	}

	/* Construct cc command */
	if (strcmp(prefix, "") == 0) {
		/* native build uses special names */
		sprintf(cmd, "%s -Y0,%s -Ya,%s",
			cc_path,
			/* acomp dir */	IDLIB,
			/* as dir */	IDBIN);
	} else {
		/* cross environment just uses pfxcc */
		strcpy(cmd, cc_path);
	}
	if (access(dfile_path, F_OK) == 0)
		sprintf(cmd + strlen(cmd), " `cat %s` ", dfile_path);
	sprintf(cmd + strlen(cmd), " -c -dn %s %s ",
		deflist,
		inclist);
	filestart = cmd + strlen(cmd);

	/*
	 * Compile each file individually, to keep 'cc' from spitting out
	 * the filenames.  Sigh...
	 */
	filep = files;
	for (;;) {
		if ((endp = strchr(filep, ' ')) == NULL)
			endp = filep + strlen(filep);
		if (endp[-1] != 'o') {
			/* if not a .o file, compile it */

			memcpy(filestart, filep, endp - filep);
			filestart[endp - filep] = '\0';

			if ((rc = proc(cmd)) != 0) {
				pfmt(stderr, MM_ERROR, BADCOMP, files, name, rc);
				error(0);
				return rc;
			}
		}
		if (*endp == '\0')
			break;
		filep = endp + 1;
	}

	if (outfile == NULL)
		return 0;

	/* Construct ld command */
	sprintf(cmd, "%s -o %s -r -dn ", ld_path, outfile);
	cmdp = filestart = cmd + strlen(cmd);
	while (*files != '\0') {
		if ((*cmdp++ = *files++) == '.' &&
		    (files[1] == '\0' || files[1] == ' ')) {
			*cmdp++ = 'o';
			files++;
		}
	}
	*cmdp = '\0';

	if ((rc = proc(cmd)) != 0) {
		pfmt(stderr, MM_ERROR, BADCOMP, filestart, name, rc);
		error(0);
		return rc;
	}

	chmod(outfile, MOD_MODE);

	return 0;
}



/* link edit object modules and create 'unix' */

void
linkedit()
{
	char linkcmd[PATH_MAX];
	char ifile_path[PATH_MAX];

	if (mflag)
		return;

	if (ifile[0] == '/')
		strcpy(ifile_path, ifile);
	else
		sprintf(ifile_path, "%s/%s", output, ifile);

	sprintf(linkcmd, 
		"%s -dn -o %s -e start -M%s/kernmap `cat %s`",
		ld_path, outfile, input, ifile_path);

	do_chdir(pkdir);
	if (proc(linkcmd) != 0) {
		pfmt(stderr, MM_ERROR, LINK, outfile);
		fatal(0);
	}
}


/* Create a process and execute command.
 * Return 0 on success, non-zero on failure.
 */

int
proc(cmd)
char *cmd;
{
	if (debug)
		fprintf(stderr, "proc: %s\n", cmd);

	return system(cmd);
}


int must_conform;

static int
check_intfc_symbol(symname, arg)
char *symname;
void *arg;
{
	driver_t *drv = (driver_t *)arg;
	struct interface_list *ilistp;
	struct intfc *intp;
	struct intfc_sym *symp;
	int has_base = 0;

	for (ilistp = drv->mdev.interfaces; ilistp; ilistp = ilistp->next) {
		if ((intp = ilistp->intfc) == NULL) {
			has_base = 1;
			continue;
		}
		if ((symp = intfc_getsym(intp, symname)) == NULL)
			continue;
		if (symp->newname[0]) {
			rename_symbol(symp->newname);
			if (debug) {
				fprintf(stderr,
					"Symbol reference %s renamed to %s\n",
					symp->symname, symp->newname);
			}
		}
		return 0;
	}
	if (has_base)
		return 0;
	if (is_ftab(symname))
		return 0;
	if (must_conform) {
		pfmt(stderr, MM_ERROR,
		     ":278:Module %s uses non-conforming symbol, %s\n",
		     drv->mdev.name, symname);
		error(0);
	} else if (debug) {
		fprintf(stderr,
			"Potentially non-conforming symbol, %s, in %s\n",
			symname, drv->mdev.name);
	}
	return 0;
}

static void
check_interfaces(drv)
driver_t *drv;
{
	struct interface_list *ilistp;
	struct intfc *intp, *intp2;
	char **vp;

	/*
	 * Pick the version of interfaces for each $interface line.
	 */
	if ((ilistp = drv->mdev.interfaces) == NULL) {
		pfmt(stderr, MM_ERROR,
		     ":279:No $interface lines in Master file for %s\n",
		     drv->mdev.name);
		error(0);
		return;
	}
	for (; ilistp; ilistp = ilistp->next) {
		/* find matching interface name */
		if (strcmp(ilistp->name, "base") == 0 ||
		    strcmp(ilistp->name, "nonconforming") == 0) {
			ilistp->intfc = NULL;
			continue;
		}
		for (intp = interfaces; intp; intp = intp->next_intfc) {
			if (strcmp(ilistp->name, intp->name) == 0)
				break;
		}
		if (intp == NULL) {
			pfmt(stderr, MM_ERROR, ":245:No such interface supported: %s\n",
				ilistp->name);
			error(0);
			return;
		}
		/* find the best matching version */
		intp2 = NULL;
		do {
			for (vp = ilistp->versions; *vp != NULL; ++vp) {
				if (strcmp(*vp, intp->version) == 0) {
					if (intp2 == NULL ||
					    intp2->order < intp->order)
						intp2 = intp;
					break;
				}
			}
		} while ((intp = intp->next_ver) != NULL);
		if (intp2 == NULL) {
			pfmt(stderr, MM_ERROR,
				":246:No required interface versions supported for %s interface\n",
				ilistp->name);
			error(0);
			return;
		}
		if (debug)
			fprintf(stderr, "Choosing %s interface version %s\n",
					intp2->name, intp2->version);
		ilistp->intfc = intp2;
	}

	/*
	 * Can we enforce conformance?
	 * No, if "$interface base" or has "$depend" in Master file.
	 * The "$interface base" part will be handled in check_intfc_symbol.
	 * "$interface nonconforming" is treated the same as "$interface base".
	 */
	must_conform = (drv->mdev.depends == NULL);

	(void) scan_symbols(check_intfc_symbol, (void *)drv, SS_UNDEF);
}


/*
 *  find the maximum bdev and cdev major numbers in the res_major file
 */

static void
rd_res_major_max()
{
	FILE *fp;
	short start, end;
	int is_blk, is_chr, rtn, nfield;
	char type, range[RANGESZ], name[NAMESZ];
	char resfile[PATH_MAX];

	if (mflag)
		return;

	sprintf(resfile, "%s/res_major", cfdir);

	if ((fp = fopen(resfile, "r")) == NULL) {
		pfmt(stderr, MM_WARNING, ":36:%s: can not open for mode %s\n", resfile, "r");
		return;
	}

	while (fgets(linebuf, 80, fp) != NULL) {
		if (linebuf[0] == '#')
			continue;
		nfield = sscanf(linebuf, "%c %s %s", &type, range, name);

		if (nfield != 3) {
			pfmt(stderr, MM_WARNING, ":37:number of fields is incorrect\n");
		fprintf(stderr, "%s  %s", gettxt(":104", "FILE:"), resfile);
		fprintf(stderr, "%s  %s", gettxt(":21", "LINE:"), linebuf);
			fclose(fp);
			return;
		}

		rtn = getmajors(range, &start, &end);

		if (rtn != 0) {
			pfmt(stderr, MM_WARNING, ":39:illegal major number entry\n");
		fprintf(stderr, "%s  %s", gettxt(":104", "FILE:"), resfile);
		fprintf(stderr, "%s  %s", gettxt(":21", "LINE:"), linebuf);
			fclose(fp);
			return;
		}
		
		switch (type) {
		case 'b':
			if (end > res_bdev_max)
				res_bdev_max = end;
			break;

		case 'c':
			if (end > res_cdev_max)
				res_cdev_max = end;
			break;

		default:
			pfmt(stderr, MM_WARNING, ":38:unknown entry\n");
		fprintf(stderr, "%s  %s", gettxt(":104", "FILE:"), resfile);
		fprintf(stderr, "%s  %s", gettxt(":21", "LINE:"), linebuf);
			fclose(fp);
			return;
		}
	}

	fclose(fp);
}

