/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)ktool:common/ktool/idtools/defines.h	1.15"
#ident	"$Header:"

/* Check if two line segments, (a,b) and (x,y),  overlap. */
#define	OVERLAP(a,b,x,y)	(!(b < x || a > y))
#define MAX(a, b)		(a > b ? a : b)
#define	MIN(a, b)		(a < b ? a : b)

#define BHIGH	16383		/* max block device major number */
#define CHIGH	16383		/* max character device major number */
#define BLOW	0		/* min block device major number */
#define CLOW	0		/* min character device major number */

/* default switch table sizes */
#define DEF_BDEV_RESERVE	20
#define DEF_CDEV_RESERVE	50
#define DEF_VFS_RESERVE		10
#define DEF_FMOD_RESERVE	50

/* loadable module types flag */
#define MODBDEV		0x1
#define MODCDEV		0x2
#define MODSTR		0x4
#define MODMOD		0x8
#define MODFS		0x10
#define MODMISC		0x20
#define MODHARD		0x40
#define MODEXEC		0x80
#define MODSD		(MODSTR | MODCDEV)
#define MODSM		(MODSTR | MODMOD)
#define MODDRV		(MODBDEV | MODCDEV)
#define MODINTR		(MODDRV | MODHARD)

/* location of file */
#define	IN	0		/* file in input directory		*/
#define OUT	1		/* file in output directory		*/
#define FULL	2		/* file has full pathname		*/

/* entry-point type definition structure */
struct entry_def {
	struct entry_def *next;		/* next entry-point definition */
	int		has_sym;	/* true if module has this symbol */
	int		is_var;		/* true if this symbol is a variable */
	char		*sname;		/* space for symbol name temp str */
	char		suffix[4];	/* suffix for this type (var length) */
};

extern struct entry_def *define_entry();
extern int lookup_entry();
extern int drv_has_entry();
extern void lookup_entries();

/* Function table definition structure */
struct ftab_def {
	struct ftab_def	*next;
	struct entry_def *entry;	/* Entry point to collect in table */
	char		*ret_type;	/* Function return type (e.g. "int") */
	char		fflags[FLAGSZ];	/* flags */
	char		tabname[4];	/* Table name; variable length */
};

extern struct ftab_def *define_ftab();

/* Symbol-table related functions */

extern int load_symtab();
extern void close_symtab();
extern int get_funcs();
extern int scan_symbols();
extern void rename_symbol();
extern void *get_valptr();

/* Symbol type values for scan_symbols(): */
#define SS_UNDEF	1		/* Undefined symbols only */
#define SS_GLOBAL	2		/* Global, defined symbols only */

/* error messages */
#define OIOA	":132:Start I/O Address, %lx, greater than End I/O Address, %lx\n"
#define OCMA	":133:Start Memory Address, %lx, greater than End Memory Address, %lx\n"
#define RIOA	":134:I/O Address range, %lx and %lx, must be within (%lx, %lx)\n"
#define RCMA	":135:Controller Memory Address, %lx, must be greater than %lx\n"
#define RIVN	":136:Interrupt vector number, %hd, must be within (%d, %d)\n"
#define RIPL	":137:Interrupt priority level, %hd, must be within (%d, %d)\n"
#define RITYP	":138:Interrupt type, %hd, must be within (%d, %d)\n"
#define RDMA	":139:DMA channel, %hd, must not be greater than %hd\n"
#define CIOA	":140:I/O Address ranges overlap for devices '%s' and '%s'\n"
#define CCMA	":141:Memory Address ranges overlap for devices '%s' and '%s'\n"
#define CDMA	":142:DMA channel conflict between devices '%s' and '%s'\n"
#define CID	":143:Id '%c' shared by devices '%s' and '%s'\n"
#define IBDM	":144:Block device major number range, %hd-%hd, must be within (%d, %d)\n"
#define DBDM	":145:Identical block device major number range, %hd-%hd, for '%s' and '%s'\n"
#define ICDM	":146:Character device major number range, %hd-%hd, must be within (%d, %d)\n"
#define DCDM	":147:Identical character device major number range, %hd-%hd, for '%s' and '%s'\n"
#define UNIT	":148:Unit, %hd, must be within (%hd, %hd)\n"

#define ONESPEC	":150:Only one specification of device '%s' allowed\n"
#define TUNE	":151:Unknown tunable parameter '%s'\n"
#define RESPEC	":152:Tunable parameter '%s' respecified\n"
#define PARM	":153:The value of parameter '%s', %ld, must be within (%ld, %ld)\n"
#define UNK	":154:Unknown device '%s'\n"
#define DEVREQ	":155:'%s' must be a block or character device\n"
#define MINOR	":156:Minor device number must be within (%d, %d)\n"
#define VECDIFF	":157:Conflicting use of interrupt vector; already used as type %d, ipl %d\n"
#define CVEC	":158:Interrupt vector conflict between devices '%s' and '%s'\n"
#define	OPRT	":159:Block device '%s' must have an 'open' function\n"
#define CLRT	":160:Block device '%s' must have a 'close' function\n"
#define STRAT	":161:Block device '%s' must have a 'strategy' function\n"
#define PRTRT	":162:Block device '%s' must have a 'print' function\n"
#define STRTAB	":163:Streams module/driver '%s' must have an 'info' structure\n"
#define CONSTAB	":164:Console driver '%s' must have a 'conssw' structure\n"
#define TTYVAR	":165:Terminal driver '%s' must have an '_tty' array\n"
#define	EXRT	":166:Execsw module '%s' must have an 'exec' function\n"
#define	DINITRT	":167:Dispatcher module '%s' must have an '_init' function\n"
#define	FINITRT	":168:Filesystem module '%s' must have an 'init' function\n"
#define NOTCONS	":169:Selected default console '%s' is not a console driver\n"
#define	EXIST	":170:Directory '%s' does not exist\n"
#define MPAR	":171:Missing value for tunable parameter '%s'\n"
#define FOPEN	":172:Cannot open '%s' for mode '%s'\n"
#define WRONG	":173:Wrong number of fields in %s line\n"
#define SUCH	":174:No such device '%s' in mdevice\n"
#define TABMEM	":175:Not enough memory for %s table\n"
#define	CONFD	":176:Configured field for device '%s' must contain 'Y' or 'N'\n"
#define NOMAXMINOR ":177:Cannot find value for MAXMINOR tunable, using %lu\n"
#define RDSYM	":178:Error reading symbol table from %s\n"
#define TOONEW	":179:%s: version is too new\n"
#define ENTRYNP	":180:%s: specified entry-point '%s' not present\n"
#define NOCONF	":181:missing intermediate file for %s module\n"
#define MAJOF	":182:%s: %s major number used is over the switch table size\n"
#define CPUDIFF ":183:modules %s and %s share interrupt but bind to different cpu\n"
#define NOMAGIC ":184:No magic number configured for exec type %s.\n"
#define INVMAGIC ":185:Module '%s' is wrong type for magic numbers.\n"
#define	DEV_NOTBLK	":186:%sdev must be a block device.\n"
#define	DEV_NOTLOAD	":187:%sdev may not be a loadable driver.\n"
#define HWREQ	":249:'%s' requires an 'h' flag\n"
#define NOFCOMPAT ":251:Version 0 driver '%s' without 'f' flag not supported.\n"
#define BADCOMP ":253:Compile/link of '%s' for %s module failed.\n\tExit code = %d\n"
#define LINK	":254:Cannot link-edit %s\n"
