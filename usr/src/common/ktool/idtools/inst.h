/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/inst.h	1.28"
#ident	"$Header:"

/* Header file for Installable Drivers commands */

/* Nested includes to resolve FILE and DIR */
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#define OMAXMIN		0xff	/* from util/mkdev.h */
#define	MAXOBJNAME	128	/* from mem/bootconf.h */
#define MAX_MAGIC	16	/* max number of magic numbers for
				 * each exec type */
#define MAX_VER		16	/* max number of versions per interface */
#define MAX_STR_TUNE	129	/* max length of string tunable */

/* macro to check if a given char is in a given string */
#define	INSTRING(str, c)	(strchr(str, c) != NULL)
/* string equality macro */
#define equal(s1, s2)		(strcmp(s1, s2) == 0)

/* path names of ID directories */
#define ROOT	"/etc/conf"		/* root of ID */
#define EROOT	"/etc"			/* directory for installing environment */
#define DROOT	"/dev"			/* directory for installing nodes */
#define	CFDIR	"cf.d"			/* Master & System files for building */
#define	PKDIR	"pack.d"
#define	CFPATH	"/etc/conf/cf.d"	/* ROOT/CFDIR */
#define SYSINC	"/usr/include"		/* path for include files */

/* Special "names" for getinst() */
#define FIRST	"0"
#define	NEXT	"1"
#define RESET	"2"

/* ID file type codes */
#define	MDEV   		0		/* Master device file */
#define MDEV_D		1		/* Files in mdevice.d directory */
#define	SDEV   		2		/* System device file */
#define SDEV_D		3		/* Files in sdevice.d directory */
#define	MTUN		4		/* Master tunable parameter file */
#define MTUN_D		5		/* Files in mtune.d directory */
#define	STUN		6		/* System tunable parameter file */
#define	STUN_D		7		/* Files in stune.d directory */
#define SASN		8		/* System device var assign file */
#define SASN_D		9		/* Files in sassign.d directory */
#define NODE		10		/* Node file */
#define NODE_D		11		/* Files in node.d directory */
#define FTAB		12		/* Ftab file (func table definition) */
#define FTAB_D		13		/* Files in ftab.d directory */
#define INTFC		14		/* Interface file */
#define INTFC_D		15		/* Files in interface.d directory */
#define ATUN		16		/* Autotune file */
#define ATUN_D		17		/* Files in autotune.d directory */
#define DRVTYPE		18		/* Driver type file */

/* Definition of ID file types used by getinst() */
typedef struct inst_ftype {
	char	*lname;		/* Logical name, used in error messages */
	char	*fname;		/* File or directory name */
	int	is_dir;		/* Flag: directory form instead of flat-file */
	int	basetype;	/* Base (flat-file) type */
	int	cur_ver;	/* Current syntax version number */
	int	ver;		/* Syntax version number of a file */
	FILE	*fp;		/* Open file pointer */
	DIR	*dp;		/* Open directory pointer */
} ftype_t;
extern ftype_t ftypes[];

/*
 * String sizes; these sizes include the null terminator character.
 */
#define NAMESZ	15	/* module name size, includes the null char */
#define TUNESZ	21	/* tunable parameter size, includes the null char */
#define MTYPESZ	41	/* module type size, includes the null char */
#define PFXSZ	9	/* handler prefix */
#define FLAGSZ	20	/* flag string */
#define RANGESZ	20	/* major number range */
#define SYMSZ	25	/* program symbol name */
#define TYPESZ	30	/* program type name */
#define FUNCSZ	33	/* function name size */
#define DRVTYPSZ 32	/* Driver.o type name size */

struct modlist {
	struct	modlist *next;
	char	name[NAMESZ];
};

struct entry_list {
	struct entry_def *edef;	/* entry_def struct for this entry point */
	struct entry_list *next;/* Next entry point for this module */
};

struct depend_list {
	struct	depend_list *next;
	char	name[NAMESZ];
};

struct magic_list {
	int	nmagic;
	int	wildcard;
	unsigned short	magics[MAX_MAGIC];
};

struct interface_list {
	struct interface_list *next;
	char	*name;			/* interface name */
	char	*versions[MAX_VER+1];	/* list of version strings */
	struct intfc *intfc;		/* chosen interface */
};

#define MDEV_VER	2
struct mdev {			/* Master file structure for devices */
	char	name[NAMESZ];	/* module name */
	char	extname[NAMESZ];	/* externally visible name */
	char	prefix[PFXSZ];	/* prefix for driver routines/structs */
	char	mflags[FLAGSZ];	/* letters indicating device flags */
	short	order;		/* order for init/start/execsw entries */
	short	blk_start;	/* start of multiple majors range - blk dev */
	short	blk_end;	/* end of multiple majors range - blk dev */
	short	chr_start;	/* start of multiple majors range - chr dev */
	short	chr_end;	/* end of multiple majors range - chr dev */
	struct entry_list *entries;
	char    modtype[MTYPESZ];       /* dynamic loadable module group */
	struct	depend_list *depends;	/* list of depend on modules */
	struct	magic_list *magics;	/* list of magic numbers */
	int	over;		/* original version of mdevice file */
	struct interface_list *interfaces;
};

#define SDEV_VER	2
struct sdev {			/* System file structure for devices */
	char	name[NAMESZ];	/* module name */
	char	conf;		/* Y/N - Configured in Kernel */
	char	conf_static;	/* Configured statically ($static) */
	long	unit;		/* unit field value */
	short	ipl;		/* ipl level for intr handler */
	short	itype;		/* type of interrupt scheme */
	short	vector;		/* interrupt vector number */
	long	sioa;		/* start I/O address */
	long	eioa;		/* end I/O address */
	unsigned long scma;	/* start controller memory address */
	unsigned long ecma;	/* end controller memory address */
	short	dmachan;	/* DMA channel */
	int	bind_cpu;	/* bind the module to a cpu number */
	int	over;		/* original version of sdevice file */
};

#define MTUNE_VER	0
struct mtune {			/* Master structure for tunable parameters */
	char	name[TUNESZ];	/* name of tunable parameter */
	long	def;		/* default value for numeric tunable */
	char	*str_def;	/* default value for the string tunable,
				 * NULL if numeric */
	long	min;		/* minimum value */
	long	max;		/* maximum value */
};

#define ATUNE_VER	0
struct atune {			/* Autotune structure */
	char	tv_name[20];
	int	tv_which;
	int	tv_linetype;
	int	tv_mempt;
	int	tv_tuneval;
	struct atune *next;
};

#define STUNE_VER	0
struct stune {			/* System structure for tunable parameters */
	char	name[TUNESZ];	/* name of tunable parameter */
	char	*value;		/* string value specified */
};

#define SASSIGN_VER	0
struct sassign {		/* System structure for dev variables */
	char	device[NAMESZ];	/* device variable name */
	char	major[NAMESZ];	/* major device name */
	long	minor;		/* minor device number */
	long	low;		/* lowest disk block in area */
	long	blocks;		/* number of disk blocks in area */
	char	objname[MAXOBJNAME];	/* pathname of object */
};

#define NODE_VER	0
struct node {			/* Structure for Node files */
	char	major[NAMESZ];	/* major device name */
	char	nodename[MAXOBJNAME];	/* pathname of node file in /dev */
	char	type;		/* BLOCK or CHAR */
	long	maj_off;	/* major number offset */
	long	minor;		/* minor number */
	char	majminor[NAMESZ];	/* minor = major of another device */
	long	uid, gid;	/* owner/group */
	unsigned short  mode;	/* file mode */
	unsigned long	level;	/* MAC-Level, should be level_t */
				/* but it's not defined in cross env */
};

#define FTAB_VER	0
struct ftab {
	char	entry[SYMSZ];	/* entry-point name */
	char	tabname[SYMSZ];	/* function table name */
	char	type[TYPESZ];	/* type returned by functions */
	char	fflags[FLAGSZ];	/* letters indicating ftab flags */
};

#define DRVMAP_VER	0

#define INTFC_VER	0
struct intfc_sym {
	struct intfc_sym *next;
	char	symname[FUNCSZ];	/* symbol name */
	char	newname[FUNCSZ];	/* remapped symbol name */
};
struct intfc {
	struct intfc *next_intfc;
	struct intfc *next_ver;
	struct intfc *rep_intfc;
	char	*name;		/* interface name */
	char	*version;	/* interface version */
	char	*repver;	/* version replaced, or NULL */
	int	order;		/* repver ordering */
	int	count;		/* temporary count */
	int	flags;		/* temporary flags */
	struct intfc_sym *symbols;
	struct depend_list *depends;	/* list of depend on modules */
};

#define DRVTYPE_VER	0
struct drvtype {
	char	type_name[DRVTYPSZ];	/* Driver.o type name */
};

/* variables for multiple major numbers */

struct multmaj {
	char	brange[RANGESZ];
        char	crange[RANGESZ];
};

/* Specific generic Ftab file flags */
#define DRV	'D'		/* Device driver related function table.*/

/* Specific generic Master file flags */
#define ONCE	'o'		/* allow only one spec. of device	*/
#define	BLOCK	'b'		/* block type device			*/
#define	CHAR	'c'		/* character type device		*/
#define CONSDEV	'C'		/* console capable device		*/
#define EXECSW	'e'		/* software exec module			*/
#define	MOD	'm'		/* STREAMS module type			*/
#define STREAM  'S'		/* STREAMS installable			*/
#define	DISP	'd'		/* dispatcher class			*/
#define UNIQ	'u'		/* assign identical blk and char majors	*/
#define FILESYS	'F'		/* filesystem module			*/
#define HARDMOD	'h'		/* hardware module			*/
#define KEEPMAJ	'k'		/* keep major numbers as is in Master	*/
#define KEEPNOD	'K'		/* keep device nodes			*/
#define FCOMPAT	'f'		/* v0 only: assume 4.0 devflag		*/
#define	STUBMOD	'l'		/* the module contains DLM stubs	*/
#define LOADMOD	'L'		/* the module can be loadable		*/


/* String buffers */
#define LINESZ	512
extern char linebuf[LINESZ];	/* current input line */

/* Flags */
extern int ignore_directives;	/* ignore special directives in Master file */


int rdinst(), getinst(), getinst_name();

void load_interfaces();
void dump_interfaces();
struct intfc_sym *intfc_getsym();
int intfc_replaces();
struct intfc *intfc_find();

/* Error codes from getinst()/rdinst()/getmajors() */

#define IERR_OPEN	-1
#define IERR_READ	-2
#define IERR_VER	-3
#define IERR_NFLDS	-4
#define IERR_FLAGS	-5
#define IERR_MAJOR	-6
#define IERR_MMRANGE	-7
#define IERR_BCTYPE	-8
#define IERR_ENTRY	-9
#define IERR_DEPEND	-10
#define IERR_AUTO	-11
#define IERR_TYPE	-12
#define IERR_MAGIC	-13
#define IERR_INTFC	-14
#define IERR_INTFC_DUP	-15
#define IERR_BADINTFC	-16
#define IERR_MISREP	-17
#define IERR_DUPREP	-18
#define IERR_REPLOOP	-19
#define IERR_NAME	-20

/* Additional status returns from rdinst() */

#define I_MORE		-99	/* Additional input lines must be processed */

/* Error messages corresponding to above error codes */

#define EMSG_OPEN	":46:Error opening %s file.\n"
#define EMSG_READ	":47:Error reading %s file.\n"
#define EMSG_VER	":48:Incorrect version number in %s file.\n"
#define EMSG_NFLDS	":49:Wrong number of fields in %s file.\n"
#define EMSG_FLAGS	":50:Illegal character in flags field in %s file for %s module.\n"
#define EMSG_MAJOR	":51:Syntax error in major number in %s file for %s module.\n"
#define EMSG_MMRANGE	":52:Invalid major range (start not less than end) in %s file for %s module.\n"
#define EMSG_BCTYPE	":53:Type character in %s file must be 'b' for block or 'c' for character.\n"
#define EMSG_ENTRY	":54:Unknown entry-point name in Master file.\n"
#define EMSG_DEPEND	":55:Unknown dependee module name in Master file.\n"
#define EMSG_INTFC	":56:Invalid $interface line in Master file for %s module.\n"
#define EMSG_TYPE	":57:Module type name too long.\n"
#define EMSG_MAGIC	":58:Empty magic line or more than one magic line.\n"
#define EMSG_AUTO	":59:Unrecognized field in Autotune file.\n"
#define EMSG_NAME	":250:$name line malformed or name too long.\n"
#define EMSG_REPLOOP	":252:Circular reference in interface.d $replace lines.\n"
#define EMSG_INTFC_DUP	":255:$interface line in %s module has duplicate name or version.\n"
#define EMSG_DUPREP	":256:Only one $replace line allowed.\n"
#define EMSG_BADINTFC	":257:Filenames in interface.d must be '<name>.<ver>'.\n"
#define EMSG_MISREP	":258:No interface.d file for $replace line in file %s.\n"
  
  
/* Definitions for autotuning */

/* Values for the autotune linetype field */
#define TV_STEP		0
#define TV_LINEAR	1

/* Values for the autotune "which" field */
#define TV_DEF	0	
#define TV_MIN	1
#define TV_MAX	2

#define NWHICH	3	/* How many possiblities for the autotune which field */
