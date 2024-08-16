/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prof:common/prof.c	1.12"

#include "hidelibc.h"		/* uses "_" to hide libc functions */
#include "hidelibelf.h"		/* uses "_" to hide libelf functions */

/*
 *	Program profiling report generator.
 *
 *	Usage:
 *
 *	prof [ -V ] [ -[ntca] ] [ -[ox] ] [ -g ] [ -l ] [ -z ] [ -s ] [ -j] [ -C] [ -m mdata ] [ prog ]
 *
 *	Where "prog" is the program that was profiled; "a.out" by default.
 *	Options are:
 *
 *	-n	Sort by symbol name.
 *	-t	Sort by decreasing time.
 *	-c	Sort by decreasing number of calls.
 *	-a	Sort by increasing symbol address.
 *
 *	The options that determine the type of sorting are mutually exclusive.
 *	Additional options are:
 *
 *	-o	Include symbol addresses in output (in octal).
 *	-x	Include symbol addresses in output (in hexadecimal).
 *	-g	Include non-global T-type symbols in output.
 *	-l	Do NOT inlcude local T-type symbols in output (default).
 *	-z	Include all symbols in profiling range, even if zero
 *			number of calls or time.
 *	-h	Suppress table header.
 *	-s	Follow report with additional statistical information.
 *	-m mdata Use file "mdata" instead of MON_OUT for profiling data.
 *	-V	Print version information for prof (and exit, if only V spec'd)
 *	-C	Decode C++ symbol names.
 *	-j      Join data for all profiled objects into one listing.
 */

#include <stdio.h>
#include <string.h>
#include "symint.h"
#include "sys/param.h"			/* for HZ */
#include "sgs.h"
#include "debug.h"
#include "machrel.h"
#include "dprof.h"


#define OLD_DEBUG(x)		


#define PROC				/* Mark procedure names. */
#define Print	(void)printf
#define Fprint	(void)fprintf

#ifndef CCADIFF 
#error "undefined CCADIFF"
#endif

#define SEC(ticks) ((double)(ticks)/HZ)		/* Convert clock ticks to seconds. */

#define FNPC    (shared_obj ? (unsigned long)((unsigned long)ccp->fnpc - SOlist->baseaddr):(unsigned long) ccp->fnpc)


/* Format for addresses in output */
static char 	aformat[] = "%11o ";
static SOentry *SOlist; 	/* List of all shared object entries in mon.out */
static Cnt 	*call_cnt_ptr; 	/* List of all functions call counters */
static int 	no_of_SOs = 0; 	/* total number of shared objects */
static int 	tot_fcn_cnt = 0; /*total number of function call counters used */
static int	 _out_cnt = 0; 	/* number of times the pc was outside the profiling range */
static int 	gflag = 0;	/*  replaces gmatch and gmask */
static int 	jflag = 0; 	/* indicates if data should be combined into one table  */
static int	Cflag = 0; 		/* decode C++ names */

static void 	print_report(); 	/* routine that prints out all the info. */
static void 	setup(void);		/* creates the interface between prof and the new format */

static void	eofon(FILE *, char *); 	/* read error has occurred */
static void	Perror(char *);		/* Print cmd name and call perror */
static int 	readnl(int); 		/* determines if given symbol satisfies
				           given conditions */
static char 	*getname(PROF_FILE *, PROF_SYMBOL); /* get name from symbol */
static Cnt 	*find_ccp(); 		/* finds the call counter list for a 
				           given shared object */
 

/*
*	TS1 insures that the symbols section is executable.
*/
#define TS1(s) (((s)>0) && (scnhdrp[(s)-1].sh_flags & SHF_EXECINSTR))
/*
*	TS2 insures that the symbol should be reported.  We want
*	to report only those symbols that are functions (STT_FUNC)
*	or "notype" (STT_NOTYPE... "printf", for example).  Also,
*	unless the gflag is set, the symbol must be global.
*/
#define TS2(i)	\
	((							\
		(ELF32_ST_TYPE(i) == STT_FUNC)			\
		|| (ELF32_ST_TYPE(i) == STT_NOTYPE)		\
	) && (							\
		(ELF32_ST_BIND(i) == STB_GLOBAL)		\
		|| (gflag && (ELF32_ST_BIND(i) == STB_LOCAL))	\
	))

#define TXTSYM(s,i)	(TS1(s) && TS2(i))


static PROF_FILE	*ldptr; 	/* For profiled loaded object file. */
static FILE	*mon_iop;		/* For profile (MON_OUT) file. */
static char	*sym_fn = "a.out";	/* Default program file name. */
static char	*mon_fn = MON_OUT;	/* Default profile file name.
					May be changed by "-m file". */

extern void *realloc();
extern char *strncpy(), *optarg;
extern int errno, optind;
extern long strtol();
extern void qsort(), exit();


/* For symbol table entries read from program file. */
static PROF_SYMBOL nl;

/* Compare routines called from qsort(). */

static int	c_ccaddr();	/* Compare fnpc fields of cnt structures. */

static int	c_sladdr();	/* Compare   sl_addr fields of slist structures. */

static int	c_time();	/*	"    sl_time    "    "   "	"      */


static int	c_name();	/*	"    sl_name    "    "   "	"      */

static int	c_ncalls();	/*	"    sl_count   "    "   "	"      */

static int	c_SOptr();	/*      "    SOptr      "    "  Cnt     "      */

static int	C_sladdr();	/* Compare   sl_addr fields of prnt_details structures. */

static int	C_time();	/*	"    sl_time    "    "   "	"      */

static int	C_name();	/*	"    sl_name    "    "   "	"      */

static int	C_ncalls();	/*	"    sl_count   "    "   "	"      */


	/* Other stuff. */


/* Memory allocation. Like malloc(), but no return if error. Defined in libprof.a */
extern void	*_lprof_Malloc();

/* Scan past path part (if any) in the ... */
extern char	*basename();

/* command name, for error messages. */
static char	*cmdname;

/* Structure of subroutine call counters (cnt) is defined in dprof.h. */

/* Structure for object entries in mon.out is defined in dprof.h. */

/* Local representation of symbols and call/time information. */
struct slist {
	char 	*sl_name;		/* Symbol name. */
	unsigned long 	sl_addr;	/* Address. */
	long 	sl_count;		/* Count of subroutine calls */
	float 	sl_time;		/* Count of clock ticks in this routine,
						converted to secs. */
};

/* Local representation of symbols and call/time information to be used 
 * if joining data for all loaded objects. */
typedef struct  {
        char 	**sl_name;         /* Symbol name. */
        unsigned long sl_addr;     /* Address. */
        long 	sl_count;          /* Count of subroutine calls */
        float 	sl_time;           /* Count of clock ticks in this routine,
                                                converted to secs. */
        char    *SOname;   		/* Shared Object with definition of symbol */
} Prt_details;



	/* local structure for tracking synonyms in our symbol list */
struct snymEntry
{
	unsigned long	sym_addr;	/* address which has a synonym */
	int	howMany;	/* # of synonyms for this symbol */
	int	snymReported;	/* 'was printed in a report line already' flag,
				 *   >0	report line printed for these syns.
				 *  ==0 not printed yet.
				 */
	long	tot_sl_count;	/* total subr calls for these snyms */
	float	tot_sl_time;	/* total clock ticks (a la sl_time) */
};

/* structure used to pass info to the print function print_report() */
typedef struct Prnt_info{
	char 	*SOpath; 	/* name of the shared object */
	unsigned long 	baseaddr; /* virtual address of shared object */
	struct slist 	*slist;	/* list of symbols with timing etc. */
	int 	symttl;		/* total num of symbols in the object */
	int 	n_syms;		/* num of symbols that qualify to be reported */
	struct 	Prnt_info *next;/* pointer to the next object */
}prnt_data;



static float t_tot = 0;	/* Total time: SEC(sum of all pcounts[i]) */
static Elf32_Shdr	*scnhdrp;	/* pointer to first section header */


static int	(*sort)() = NULL;	/* Compare routine for sorting output symbols.
						Set by "-[acnt]". */
static int 	shared_obj = 0;	/* indicates if the file being processed is a shared object or not */

static int	flags;		/* Various flag bits. */

static unsigned long adr_l;		/* From head.lpc. */
static unsigned long adr_h;		/*   "  head.hpc. */


/* Bit macro and flag bit definitions. */

#define FBIT(pos)	(01 << (pos))	/* Returns value with bit pos set. */
#define F_SORT		FBIT(0)		/* Set if "-[acnt]" seen. */
#define F_VERBOSE	FBIT(1)		/* Set if "-s" seen. */
#define F_ZSYMS		FBIT(2)		/* Set if "-z" seen. */
#define F_PADDR		FBIT(3)		/* Set if "-o" or "-x" seen. */
#define F_NHEAD		FBIT(4)		/* Set if "-h" seen. */


static struct snymEntry *snymList;	/* Pointer to allocated list of
				 * synonym entries.  */
static struct snymEntry *snymp; /* for scanning entries. */

static int 	n_snyms;		/* #used slots in snymList */

#define NAMELEN		100

        /* Demangle - interface to the  demangle routine
         *  to decode C++ names
         */

PROC
static char *
Demangle(ldpter, symbol)
PROF_FILE *ldpter;
PROF_SYMBOL symbol;
{
        size_t  len;
        char    *namebuf;
	char 	*name;
	int  	demang_len = NAMELEN;
	extern  int demangle();
	extern	void free(void *);

	name = &(ldpter->pf_symstr_p)[symbol.ps_sym.st_name];
	if (name == NULL)
		return "<< bad symbol name>>";
        namebuf =  (char *) _lprof_Malloc(demang_len, 1);

        if (((len = demangle(name, namebuf, (size_t)demang_len)) != (size_t) -1) 
	     && (len > demang_len)){
		free(namebuf);
		demang_len = len;
		namebuf =  (char *) _lprof_Malloc(demang_len, 1);
		len = demangle(name, namebuf,(size_t) demang_len);
	}
	if (len == (size_t) -1)
                (void)strcpy(namebuf,name);

	return(namebuf);
}


	/* printSnymNames - print a comma-separated list of snym names.
	 * This routine hunts down all the synonyms for the given
	 * symbol, and prints them as a comma-separated list.
	 * NB we assume that all the synonyms _Follow_ this one,
	 * since they are only printed when the First one
	 * is seen.
	 */
PROC
static void
printSnymNames( slp, snymp )
struct	slist       *slp;
struct	snymEntry   *snymp;
{
	 /* how many snyms for this addr, total, and their shared address */
	int i = snymp->howMany;
	unsigned long sharedaddr = snymp->sym_addr;

	 /* put out first name - it counts as one, so decr count */
	(void) fputs( slp->sl_name, stdout );
	i--;

	 /* for the others: find each, print each. */
	while( i-- > 0 ) {
		while( (++slp)->sl_addr != sharedaddr );
		Print(", %s", slp->sl_name);
	}
	 /* finally.. the trailing newline */
	(void)putchar('\n');	
}

	/* symbol_name - generate a list of synonym names to print out
	 * after processing data for all loaded objects (if joining
	 * data for all objects )
	 */

PROC
static char ** 
symbol_name(slp, snymp)
struct slist 		*slp;
struct snymEntry	*snymp;
{
	int i = 1;
	int index = 0;
	unsigned long sharedaddr;
	char **nameptr;
	extern void *calloc(size_t, size_t);
	
	if (snymp) {
		i = snymp->howMany;
		sharedaddr = snymp->sym_addr;
		if ((nameptr = (char **) calloc(i+1, sizeof(char *))) == NULL){
			(void)fprintf(stderr, "%s: calloc: Out of space\n", cmdname);
			exit(1);
		}
			
		nameptr[index++] = slp->sl_name;
		i--;

		while (i-- > 0) {
			while ((++slp)->sl_addr != sharedaddr );
			nameptr[index++] = slp->sl_name;
		}
	} else	{
		nameptr = ( char **) calloc(i+1, sizeof (char *));
		nameptr[0] = slp->sl_name;
	}
	return(nameptr);
}


	/* getSnymEntry - see if addr was noted as a aliased address
	 * (i.e. a synonym symbol) and return the address of the
	 * snym entry if it was.
	 */
PROC
static struct snymEntry
*getSnymEntry( sl_addr )
unsigned long sl_addr;
{
	struct snymEntry *p;
	int i;

	for( p=snymList, i=n_snyms; --i >= 0; p++ )
		if( sl_addr == p->sym_addr )
			return( p );

	return( (struct snymEntry *) 0 );
}





PROC
main(argc, argv)
int argc;
char **argv;
{

	WORD *pcounts;		/* Pointer to allocated area for
						pcounts: PC clock hit counts */
	short	VwasSpecified=0;/* 1 if -V was specified */

	register WORD *pcp;	/* For scanning pcounts. */


	register  Cnt *ccp;	/* For scanning ccounts. */

	struct slist *slist;	/* Pointer to allocated slist structures: symbol
						name/address/time/call counts */

	register struct slist *slp;	/* For scanning slist */

	prnt_data *print_data = NULL;	/* pointer to the list of objects */

	int vn_cc;	/* Number of cnt structures in profile data
						file (later # ones used). */

	int n_pc;	/* Number of pcounts in profile data file. */

	int n_syms;	/* Number of text symbols (of proper type)
					that fill in range of profiling. */
	int symttl;	/* Total # symbols in program file sym-table */
	long temp;
	int i;
	register int n, symct;
	Elf32_Phdr	*phdrp;
	int	LibC = 1;	/* Has libc.so.1 been seen */

	double sf;		/* Scale for index into pcounts:
 				         i(pc) = ((pc - pc_l) * sf) . */
	unsigned  long pc_m;	/* Range of PCs profiled: pc_m = pc_h - pc_l */
	unsigned  long pc_h;
	unsigned  long pc_l;

	float t = 0.0;
	float t0 = 0.0;		/* Time accumulator */
	float has_time;	/* indicate if any symbols log time for object */
	long  has_call;     /* indicate if any symbols were called for object */

	prnt_data *data, *tail_data; /* temp. variables used to build list for print_report() */

	DEBUG_LOC("main: top");
	cmdname = basename(*argv);	/* command name. */

	while ((n = getopt(argc, argv, "jcanthsglzoxT:m:VC")) != EOF) {

		switch (n) {
		int (*fcn)();	/* For function to sort results. */

		case 'm':	/* Specify data file:	-m file */
			mon_fn = optarg;
			break;

#ifdef ddt
		case 'T':	/* Set trace flags: -T(octnum) */
			debug_value = (int)strtol(optarg, 0, 8);
			break;
#endif

		case 'n':	/* Sort by symbol name. */
			if (jflag)
				fcn = C_name;
			else
				fcn = c_name;
			goto check;

		case 't':	/* Sort by decreasing time. */
			if (jflag)
				fcn = C_time;
			else
				fcn = c_time;
			goto check;

		case 'c':	/* Sort by decreasing # calls. */
			if (jflag)
				fcn = C_ncalls;
			else
				fcn = c_ncalls;
			goto check;

		case 'a':	/* Sort by increasing symbol address
						(don't have to -- it will be) */
			if (jflag)
				fcn = C_sladdr;
			else
				fcn = c_sladdr;
		check:		/* Here to check sort option conflicts. */
			if ((flags & F_SORT) && sort != fcn) {
				Fprint(stderr,
			   "%s: Warning: %c overrides previous specification\n",
				    cmdname, n);
			}
			sort = fcn;	/* Store sort routine */
			flags |= F_SORT; /* Note have done so */
			break;

		case 'o':	/* Include symbol addresses in output. */
		case 'x':	/* Include symbol addresses in output. */
			aformat[3] = (char) n;	/* 'o' or 'x' in format */
			flags |= F_PADDR;	/* Set flag. */
			break;

		case 'g':	/* Include local T symbols as well as global*/
			gflag = 1;
			break;

		case 'l':	/* Do NOT include local T symbols */
			gflag = 0;
			break;

		case 'z':	/* Print all symbols in profiling range,
				 		 even if no time or # calls. */
			flags |= F_ZSYMS;	/* Set flag. */
			break;

		case 'h':	/* Suppress table header. */
			flags |= F_NHEAD;
			break;

		case 's':	/* Follow normal output with extra summary. */
			flags |= F_VERBOSE;	/* Set flag (...) */
			break;

		case 'V':
			(void) Fprint(stderr, "prof: %s %s\n", CPPT_PKG, CPPT_REL);
			VwasSpecified=1;
			break;
		case 'C':
			Cflag = 1;
			break;
		case 'j':
			jflag = 1;  /* combine data for all objects into one table */
			break;
		case '?':	/* But no good. */
			Fprint(stderr,
			    "%s: Unrecognized option: %c\n", cmdname, n);
			exit(1);

		}	/* End switch (n) */
	}	/* End while (getopt) */

	DEBUG_LOC("main: following getopt");

	/* if -V the only argument, just exit. */
	if (VwasSpecified && argc==2 && !flags )
		exit(1);

	if (optind < argc)
		sym_fn = argv[optind];	/* name other than `a.out' */

	if (sort == NULL && !(flags & F_SORT))
				/* If have not specified sort mode ... */
		if (jflag)
			sort = C_time;		/* then sort by decreasing time. */
		else
			sort = c_time;	

	setup();
	while (no_of_SOs--) {
		DEBUG_LOC("main: before _symintOpen");
		if ((ldptr = _symintOpen(SOlist->SOpath)) == NULL) {
			Perror("_symintOpen failed");
		}
		DEBUG_LOC("main: after _symintOpen");

		scnhdrp = ldptr->pf_shdarr_p;
	
		{
			Elf_Kind k = elf_kind(ldptr->pf_elf_p);

			DEBUG(printf("elf_kind = %d\n",k));
			DEBUG(printf("elf_type = %d\n",ldptr->pf_elfhd_p->e_type));
			if ( ((k != ELF_K_ELF) && (k != ELF_K_COFF))
			      || ((ldptr->pf_elfhd_p->e_type != ET_EXEC)
			      && (ldptr->pf_elfhd_p->e_type != ET_DYN))) {
				Fprint(stderr, "%s: %s: improper format\n", cmdname, sym_fn);
				exit(1);
			}
			if (ldptr->pf_elfhd_p->e_type == ET_DYN) /* is it a shared object or not */
				shared_obj = 1;
			else
				shared_obj = 0;

			if (k == ELF_K_COFF) {
				Fprint(stderr,
				"%s: %s: Warning - internal conversion of COFF file to ELF\n",
				cmdname, sym_fn);
			}
		}

		/* Compute the file address of symbol table. Machine-dependent. */

		DEBUG(printf("number of symbols (pf_nsyms) = %d\n",ldptr->pf_nsyms));

		/* Number of symbols in file symbol table. */
		symttl = ldptr->pf_nsyms;
		if (symttl == 0) {		/* This is possible. */
			Fprint(stderr, "%s: %s: has no symbols\n", cmdname, sym_fn);
			continue;		/* Note zero exit code. */
		}

		n_pc = SOlist->size;	/* size of histogram buffer */
		/* Space for PC counts. */
		pcounts = ( WORD *)_lprof_Malloc(n_pc, sizeof(WORD));
		
		/* Read the PC counts from the rest of MON_OUT file. */
		if (fread((char *)pcounts, sizeof(WORD), n_pc, mon_iop) != n_pc)
			eofon(mon_iop, mon_fn);		/* Probably junk file. */



	/*
	Having gotten preliminaries out of the way, get down to business.
	The range pc_m of addresses over which profiling was done is
	computed from the low (pc_l) and high (pc_h) addresses, gotten
	from the MON_OUT header.  From this and the number of clock
	tick counters, n_pc, is computed the so-called "scale", sf, used
	in the mapping of addresses to indices, as follows:

		i(pc) = (pc - pc_l) * sf

	Following this, the symbol table is scanned, and those symbols
	that qualify are counted.  These  are T-type symbols, excluding
	local (nonglobal) unless the "-g" option was given. Having thus
	determined the space requirements, space for symbols/times etc.
	is allocated, and the symbol table re-read, this time keeping
	qualified symbols.

	*/
	

		if (shared_obj) {
		/* In shared objects addresses are offsets from base 0 */
			adr_l = (unsigned long) 0 ;	
			adr_h = (unsigned long) (SOlist->endaddr - SOlist->baseaddr) ;
			pc_l = (unsigned long)(SOlist->textstart - SOlist->baseaddr);
			pc_h = (unsigned long)(SOlist->endaddr - SOlist->textstart);
		} else {
			adr_l = pc_l = SOlist->textstart;	/* Low PC of range that was profiled. */
			adr_h = pc_h = SOlist->endaddr;	/* First address past range of profiling */
		}
		pc_m = pc_h - pc_l;

		OLD_DEBUG(if (debug_value) Fprint(stderr,
			"low pc = %#o, high pc = %#o, range = %#o = %u\n\
			call counts: %u,  pc counters: %u\n",
			pc_l, pc_h, pc_m, pc_m, tot_fcn_cnt,  n_pc));

		sf = (double)n_pc/pc_m;

		OLD_DEBUG( if (debug_value) Fprint( stderr, "sf = %f\n", sf); );

		/* Prepare to read symbols from "a.out" (or whatever). */
		n_syms = 0;			/* Init count of qualified symbols. */
		n = symttl;			/* Total symbols. */
		while (--n >= 0)			/* Scan symbol table. */
			if (readnl(n))	/* Read and examine symbol, count qualifiers */
				n_syms++;

		OLD_DEBUG( if (debug_value) {
			Fprint(stderr, "%u symbols, %u qualify\n", symttl, n_syms);
		}
		);

		/* Allocate space for qualified symbols. */

		slist = slp =
			(struct slist *)_lprof_Malloc(n_syms, sizeof(struct slist));

		/* Allocate space for synonym symbols
		 * (i.e. symbols that refer to the same address).
		 * NB there can be no more than n_syms/2 addresses
		 * with symbols, That Have Aliases, that refer to them!
		 */



	/* Loop on number of qualified symbols. */
		for (symct = 0; symct < symttl; symct++) {
			if (readnl(symct)) {	/* Get one. Check again. */
				/* Is qualified. Move name ... */
				if (Cflag)
					slp->sl_name = Demangle(ldptr, nl);
				else
					slp->sl_name = getname(ldptr, nl);

				/* and address into slist structure. */
				slp->sl_addr = (unsigned long) nl.ps_sym.st_value;

				/* set other slist fields to zero. */
				slp->sl_time = 0.0;
				slp->sl_count = 0;
				OLD_DEBUG( if (debug_value & 02)
					Fprint(stderr, "%-8.8s: %#8o\n", slp->sl_name, slp->sl_addr));

				slp++;
			}
		}
	/*
	Now attempt to match call counts with symbols.  To do this, it
	helps to first sort both the symbols and the call address/count
	pairs by ascending address, since they are generally not, to
	begin with.  The addresses associated with the counts are not,
	of course, the subroutine addresses associated with the symbols,
	but some address slightly past these. Therefore a given count
	address (in the fnpc field) is matched with the closest symbol
	address (sl_addr) that is:
		(1) less than the fnpc value but,
		(2) not more than CCADIFF bytes less than it.
	The value of CCADIFF is roughly the size of the code between
	the subroutine entry and that following the call to the mcount
	routine.  In other words, unreasonable matchups are avoided.
	Situations such as this could arise when static procedures are
	counted but the "-g" option was not given to this program,
	causing the symbol to fail to qualify.  Without this limitation,
	unmatched counts could be erroneously charged.
	*/
		slp = slist;			/*   "		"   "   symbol. */
		/* symbols by increasing address. */
		qsort((char *)slp, (unsigned)n_syms, sizeof(struct slist), c_sladdr);

		if ((ccp = find_ccp()) != NULL) {
			/* Sort call counters and ... */
			qsort((char *)ccp, (unsigned)SOlist->ccnt, sizeof(Cnt), c_ccaddr);
			vn_cc = SOlist->ccnt;			/* save this for verbose option */
			/* Loop to match up call counts & symbols. */
			for (n = n_syms; n > 0 && vn_cc > 0; ) {
				if ((slp->sl_addr <= FNPC) &&
				    FNPC <= slp->sl_addr+CCADIFF) {
					/* got a candidate: find Closest. */
					struct slist *closest_symp;
					do {
						closest_symp = slp;
						slp++;
						--n;
					} while( n>0 && slp->sl_addr < FNPC );

					OLD_DEBUG(
						if (debug_value & 04) {
						    Fprint(stderr,
						    "Routine %-8.8s @ %#8x+%-2d matches count address %#8x\n",
						    closest_symp->sl_name,
						    closest_symp->sl_addr,
						    ccp->fnpc-slp->sl_addr,
						    ccp->fnpc);
					}
					);
					closest_symp->sl_count = ccp->mcnt;  /* Copy count. */
					++ccp;
					--vn_cc;
				} else if (FNPC < slp->sl_addr) {
					++ccp;
					--vn_cc;
				} else {
					++slp;
					--n;
				}
			}
		}
	/*
	The distribution of times to addresses is done on a proportional
	basis as follows: The t counts in pcounts[i] correspond to clock
	ticks for values of pc in the range pc, pc+1, ..., pc+s_inv-1
	(odd addresses excluded for PDP11s).  Without more detailed information,
	it must be assumed that there is no greater probability
	of the clock ticking for any particular pc in this range than for
	any other.  Thus the t counts are considered to be equally distributed
	over the addresses in the range, and that the time for any given
	address in the range is pcounts[i]/s_inv.

	The values of the symbols that qualify, bounded below and above
	by pc_l and pc_h, respectively, partition the profiling range into
	regions to which are assigned the total times associated with the
	addresses they contain in the following way:

	The sum of all pcounts[i] for which the corresponding addresses are
	wholly within the partition are charged to the partition (the
	subroutine whose address is the lower bound of the partition).

	If the range of addresses corresponding to a given t = pcounts[i]
	lies astraddle the boundary of a partition, e.g., for some k such
	that 0 < k < s_inv-1, the addresses pc, pc+1, ..., pc+k-1 are in
	the lower partition, and the addresses pc+k, pc+k+1, ..., pc+s_inv-1
	are in the next partition, then k*pcounts[i]/s_inv time is charged
	to the lower partition, and (s_inv-k) * pcounts[i]/s_inv time to the
	upper.  It is conceivable, in cases of large granularity or small
	subroutines, for a range corresponding to a given pcounts[i] to
	overlap three regions, completely containing the (small) middle one.
	The algorithm is adjusted appropriately in this case.
	*/


		pcp = pcounts;				/* Reset to base. */
		slp = slist;				
		has_time = 0.0;
		has_call = 0;
		for (n = 0; n < n_syms; n++) {		
			/* Start addr of region, low addr of overlap. */
			unsigned long pc0, pc00;
			/* Start addr of next region, low addr of overlap. */
			unsigned long pc1, pc10;
			 /* First index into pcounts for this region and next region. */
			register int i0, i1;
			long ticks;

			/* Address of symbol (subroutine). */
			if (shared_obj)
				pc0 = slp[n].sl_addr - pc_l;
			else
				pc0 = slp[n].sl_addr;

			/* Address of next symbol, if any or top of profile
								range, if not */
			pc1 = (n < n_syms - 1) ? (shared_obj ? slp[n+1].sl_addr - pc_l: slp[n+1].sl_addr) : pc_h;

			/* Lower bound of indices into pcounts for this range */

			i0 = ((unsigned) pc0 - (unsigned) pc_l) * sf;

			/* Upper bound (least or least + 1) of indices. */
			i1 = ((unsigned) pc1 - (unsigned) pc_l)  * sf;

			if (i1 >= n_pc)				/* If past top, */
				i1 = n_pc - 1;				/* adjust. */

			/* Lowest addr for which count maps to pcounts[i0]; */
			pc00 =  pc_l + (unsigned long)i0/sf;

			/* Lowest addr for which count maps to pcounts[i1]. */
			pc10 =  pc_l + (unsigned long)i1/sf;

			OLD_DEBUG(if (debug_value & 010) Fprint(stderr,
			"%-8.8s\ti0 = %4d, pc00 = 0x%x, pc0 = 0x%x\n\
			\t\ti1 = %4d, pc10 = 0x%x, pc1 = 0x%x\n\t\t",
			slp[n].sl_name, i0, pc00, pc0, i1, pc10, pc1));

			t = 0;			/* Init time for this symbol. */
			if (i0 == i1) {
			/* Counter overlaps two areas? (unlikely unless large
								granularity). */
				ticks = pcp[i0];	/* # Times (clock ticks). */
				OLD_DEBUG(if (debug_value & 010) 
				Fprint(stderr,"ticks = %d\n",ticks));

			    /* Time less that which overlaps adjacent areas */
				t +=  SEC(ticks * (double) (pc1 - pc0) * sf );

				OLD_DEBUG(if (debug_value & 010)
				Fprint(stderr, "%ld", (pc1 - pc0) * ticks));

			} else {
				/* Overlap with previous region? */
				if (pc00 < pc0) {
					ticks = pcp[i0];
					OLD_DEBUG(if (debug_value & 010) Fprint(stderr,"pc00 < pc0 ticks = %d\n",ticks));

				/* Get time of overlapping area and subtract
						proportion for lower region. */
					t += SEC(
						ticks*(1-(double) (pc0-pc00) * sf));

				/* Do not count this time when summing times
						wholly within the region. */
					i0++;

					OLD_DEBUG(if (debug_value & 010)
					Fprint(stderr, "%ld + ", (pc0 - pc00) * ticks));
				}


			/* Init sum of counts for PCs not shared w/other
								routines. */
				ticks = 0;

				/* Stop at first count that overlaps following
								routine. */
				for (i = i0; i < i1; i++)
					ticks += pcp[i];

				t += SEC(ticks);  /* Convert to secs & add to total. */

				OLD_DEBUG(if (debug_value & 010) Fprint(stderr, "%ld", ticks));

			/* Some overlap with low addresses of next routine? */
				if (pc10 < pc1) {
					/* Yes. Get total count ... */
					ticks = pcp[i1];

					/* and accumulate proportion for addresses in
							range of this routine */

					t += SEC((double) ticks * (pc1 - pc10) * sf);

					OLD_DEBUG(if (debug_value & 010) 
					Fprint(stderr,"ticks = %d\n",ticks));

					OLD_DEBUG(if (debug_value & 010)
					Fprint(stderr, " + %ld", (pc1 - pc10) * ticks));
				}
			}		/* End if (i0 == i1) ... else ... */

			slp[n].sl_time = t;	/* Store time for this routine. */
			t0 += t;		/* Accumulate total time. */

			OLD_DEBUG(if (debug_value & 010) 
			Fprint(stderr, " ticks = %.2f msec\n", t));

			/* Convert offsets in shared objects to addresses. 
			    Keep track of which object has symbol definition. */
			if (shared_obj) 
				slp[n].sl_addr += SOlist->baseaddr; 
			has_time += slp[n].sl_time;
			has_call += slp[n].sl_count;

		}	/* End for (n = 0; n < n_syms; n++) */

		/* Final pass to total up time. */
		/* Sum ticks, then convert to seconds. */

		for (n = n_pc, temp = 0; --n >= 0; temp += *(pcp++) );

		t_tot += SEC(temp);
 
		/* generate info for print routines */
		data = (prnt_data *) _lprof_Malloc(1,sizeof(prnt_data)); 
		if (print_data == NULL)
			print_data = data; /* create list for print_report(). */
		else
			tail_data->next = data; /* append to list */
		tail_data = data;
		if (shared_obj) 
			data->baseaddr = SOlist->baseaddr;
		else 
			data->baseaddr = 0;

		data->SOpath = SOlist->SOpath; /*init info for print routines */
		if (has_time || has_call) 
			data->slist = slist;
		else
			
			data->slist = NULL;
		data->symttl = symttl;
		data->n_syms = n_syms;
		data->next = NULL;
		_symintClose(ldptr);
		SOlist++;
	}
/* ...and finally call the print routine */
	print_report(print_data);
	exit(0);

}


/* Read symbol entry. Return TRUE if satisfies conditions. */
/*  readnl() entirely new for COFF */

PROC
static
readnl(symindex)
int symindex;
{
	nl = ldptr->pf_symarr_p[symindex];

	OLD_DEBUG( if (debug_value & 020) {
		Fprint(
			stderr,
			"`%-8.8s'\tst_info=%#4o, value=%#8.6o\n",
			ldptr->pf_symstr_p[nl.ps_sym.st_name],
			(unsigned char) nl.ps_sym.st_info,
			nl.ps_sym.st_value
		);
	}
);
	/*
	TXTSYM accepts global (and local, if "-g" given) T-type symbols.
	Only those in the profiling range are useful.
	*/
#if DEBUG	
	printf("symbol value = ox%x\n",nl.ps_sym.st_value);
#endif
	return(
		nl.ps_sym.st_shndx < SHN_LORESERVE
		&& TXTSYM(
			nl.ps_sym.st_shndx,
			nl.ps_sym.st_info
		)
		&& (adr_l <= (unsigned long )nl.ps_sym.st_value)
		&& ((unsigned long)nl.ps_sym.st_value < adr_h)
	);

}




/*
	Given the quotient Q = N/D, where entier(N) == N and D > 0, an
	approximation of the "best" number of fractional digits to use
	in printing Q is f = entier(log10(D)), which is crudely produced
	by the following routine.
*/

PROC 
static int
fprecision(count)
long count;
{
	return (count < 10 ? 0 : count < 100 ? 1 : count < 1000 ? 2 :
	    count < 10000 ? 3 : 4);
}


/* Here if unexpected read problem. */

PROC
static void
eofon(iop, fn)
register FILE *iop;
register char *fn;
{
	if (ferror(iop))		/* Real error? */
		Perror(fn);		/* Yes. */
	Fprint(stderr, "%s: %s: Premature EOF\n", cmdname, fn);
	exit(1);
}

/* Version of perror() that prints cmdname first. */

PROC
static void
Perror(s)
char *s;
{				/* Print system error message & exit. */
	register int err = errno;	/* Save current errno in case */

	Fprint(stderr, "%s: ", cmdname);
	errno = err;			/* Put real error back. */
	perror(s);			/* Print message. */
	_symintClose(ldptr);		/* cleanup symbol information */
	exit(1);			/* Exit w/nonzero status. */
}

/*
	Various comparison routines for qsort. Uses:

	c_ccaddr	- Compare fnpc fields of cnt structs to put
				call counters in increasing address order.
	c_sladdr	- Sort slist structures on increasing address.
	c_time		-  "	 "	  "      " decreasing time.
	c_ncalls	-  "	 "	  "      " decreasing # calls.
	c_name		-  "	 "	  "      " increasing symbol name
*/

#define CMP2(v1,v2)	((v1) < (v2) ? -1 : (v1) == (v2) ? 0 : 1)
#define CMP1(v)		CMP2(v, 0)

PROC
c_ccaddr(p1, p2)
register struct cnt *p1, *p2;
{
	return (CMP2(p1->fnpc, p2->fnpc));
}

PROC
c_sladdr(p1, p2)
register struct slist *p1, *p2;
{
	return (CMP2(p1->sl_addr, p2->sl_addr));
}

PROC
C_sladdr(p1, p2)
register Prt_details *p1, *p2;
{
	return (CMP2(p1->sl_addr, p2->sl_addr));
}

PROC
c_time(p1, p2)
register struct slist *p1, *p2;
{
	register float dtime = p2->sl_time - p1->sl_time; /* Decreasing time. */

	return (CMP1(dtime));
}

PROC
C_time(p1, p2)
register Prt_details *p1, *p2;
{
	register float dtime = p2->sl_time - p1->sl_time; /* Decreasing time. */

	return (CMP1(dtime));
}


PROC
c_ncalls(p1, p2)
register struct slist *p1, *p2;
{
	register int diff = p2->sl_count - p1->sl_count; /* Decreasing # calls. */

	return (CMP1(diff));
}

PROC
C_ncalls(p1, p2)
register Prt_details *p1, *p2;
{
	register int diff = p2->sl_count - p1->sl_count; /* Decreasing # calls. */

	return (CMP1(diff));
}

PROC
c_name(p1, p2)
register struct slist *p1, *p2;
{
	register int diff;

		/* flex names has variable length strings for names */
	diff = strcmp(p1->sl_name, p2->sl_name);
	return (CMP1(diff));
}

PROC
C_name(p1, p2)
register Prt_details *p1, *p2;
{
	register int diff;

		/* flex names has variable length strings for names */
	diff = strcmp(*p1->sl_name, *p2->sl_name);
	return (CMP1(diff));
}

PROC
static
c_SOptr(p1, p2)
register Cnt *p1, *p2;
{
	return( CMP2(p1->_SOptr, p2->_SOptr));
}

#define STRSPACE 2400		/* guess at amount of string space */
/*	getname - get the name of a symbol in a permanent fashion
*/
PROC
static char *
getname(ldpter, symbol)
PROF_FILE *ldpter;
PROF_SYMBOL symbol;
{
	static char *strtable = NULL;	/* space for names */
	static int sp_used = 0;		/* space used so far */
	static int size = 0;		/* size of string table */
	char *name;			/* name to store */
	int lth;			/* space needed for name */
	int get;			/* amount of space to get */

	name = &(ldpter->pf_symstr_p)[symbol.ps_sym.st_name];
	if (name == NULL)  {
		return "<<bad symbol name>>";
	}
	lth = strlen(name) + 1;
	if ((sp_used + lth) > size)  {	 /* if need more space */
		/* just in case very long name */
		get = lth > STRSPACE ? lth : STRSPACE;
		strtable = _lprof_Malloc(1, get);
		size = get;
		sp_used = 0;
	}
	(void)strcpy(&(strtable[sp_used]), name);
	name = &(strtable[sp_used]);
	sp_used += lth;
	return name;
}





/* Initialization of SOlist, call_cnt_ptr using the new format of mon.out */
static void
setup()
{

	SOentry head;
	SOentry	*tmpSOlist;
	int	tmp_cnt; 
	int 	aout = 1;
	int	tblesize;
	char    *nametbl;

	
	/* Open monitor data file (has counts). */
	if ((mon_iop = fopen(mon_fn, "r")) == NULL)
		Perror(mon_fn);

	/* read the header */
	if (fread((char *)&head, sizeof(SOentry), 1, mon_iop) != 1)
                eofon(mon_iop, mon_fn);         /* Probably junk file. */

	no_of_SOs = head.size;	/* no. of SO entries in mon.out */
	_out_cnt = (int) head.baseaddr;	/* no. of times the pc did not fall within the profiling region */
	tblesize = head.ccnt;
	SOlist = (SOentry *)_lprof_Malloc(no_of_SOs, sizeof(SOentry));

	/* read in all SOentries */
	if (fread(SOlist, sizeof(SOentry), no_of_SOs, mon_iop) != no_of_SOs)
                eofon(mon_iop, mon_fn);         /* Probably junk file. */
	
	nametbl = (char *) _lprof_Malloc(tblesize, sizeof(char));
	if (fread(nametbl, sizeof(char), tblesize, mon_iop) != sizeof(char) * tblesize)
                eofon(mon_iop, mon_fn);         /* Probably junk file. */

	tmp_cnt = no_of_SOs;
	for (tmpSOlist = SOlist; tmp_cnt; tmp_cnt--)
	{
		tmpSOlist->SOpath = nametbl + (int) tmpSOlist->SOpath;

		if ((strlen(tmpSOlist->SOpath) == 0) && aout)
		{
			aout = 0;
			tmpSOlist->SOpath = sym_fn;
		}
		/* count the total function cnt structures to be read in */
		tot_fcn_cnt += tmpSOlist->ccnt;
		tmpSOlist++;
	}

	call_cnt_ptr = (Cnt *) _lprof_Malloc(tot_fcn_cnt, sizeof(Cnt));
	/* read the function call counters */
	if (fread(call_cnt_ptr, sizeof(Cnt), tot_fcn_cnt, mon_iop) != tot_fcn_cnt)
		eofon(mon_iop, mon_fn);         /* Probably junk file. */
	/* sort the list of Cnt structures so that all functions belonging to the same SO are together */
	qsort(call_cnt_ptr, (unsigned) tot_fcn_cnt, sizeof(Cnt), c_SOptr);
}

/* This routine finds the first call counter that belongs to the given SOentry.
*  Since the call counts are sorted according to the shared objects this
*  marks the beginning of the call counters list for the shared object
*/

static Cnt *find_ccp()
{
	int	tot_cnt = tot_fcn_cnt;
	Cnt	*callptr = call_cnt_ptr;

	while (tot_cnt)
	{
		if (SOlist->next_SO == callptr->_SOptr) 
		/* if the SOentry addresses are the same we've found it */
			break;
		else
		{ 
			callptr++;
			tot_cnt--;
		}
	}
	return(tot_cnt ? callptr: NULL);
}

PROC
static void
print_hdr()
{ 	
	if (!(flags & F_NHEAD)) { 
		if (flags & F_PADDR) 
			Print(" Address     ");   
		(void)puts(" %Time Seconds Cumsecs  #Calls   msec/call  Name"); 
		}
}

PROC
static void
print_sum(int n_nonzero, int n_syms, int  symttl)
{
	Fprint(stderr, "%5d/%d symbols qualified", n_syms, symttl);
	if (n_nonzero < n_syms)
		Fprint(stderr, ", %d had zero time and zero call-counts\n",
			    n_syms - n_nonzero);
	else
		(void)putc('\n', stderr);

}

PROC
static void
print_nodata(prnt_data *pd_ptr)
{
	Print ("%d/%d qualified  symbols: ", pd_ptr->n_syms, pd_ptr->symttl);
	Print("no time or call count data collected.\n\n");

}

PROC
static void
print_report(prnt_data *print_data)
{
	int 	n_syms, tot_n_syms = 0;
	int  	ccnt = 1;
	int	n_nonzero, tot_n_zero = 0; /* Number of symbols printed because 
				              of nonzero time & call counts */
	int     fdigits; 	/* # of digits of precision for print msecs/call */
	int 	tot_symttl = 0;
	int 	n;
	struct 	slist *slist, *slp;
	float 	cum_t = 0.0;
	Prt_details 	*tmp_details = NULL; 
	Prt_details 	*prt_details = NULL;
	int	n_print = 0;
	int 	snymCapacity;		/* #slots in snymList */


        /* Now, whilst we still have the symbols sorted
         * in address order..
         * Loop to record duplicates, so we can display
         * synonym symbols correctly.
         * Synonym symbols, or symbols with the same address,
         * are to be displayed by prof on the same line, with
         * one statistics line, as below:
         *                      ... 255  ldaopen, ldaopen
         * The way this will be implemented, is as follows:
         *
         * Pass 1 - while the symbols are in address order, we
         *  do a pre-pass through them, to determine for which
         *  addresses there are more than one symbol (i.e. synonyms).
         *  During this prepass we collect summary statistics in
         *  the synonym entry, for all the synonyms.
         *
         * 'Pass' 2 - while printing a report,  for each report line,
         *  if the current symbol is a synonym symbol (i.e. in the
         *  snymList) then we scan forward and pick up all the names
         *  which map to this address, and print them too.
         *  If the address' synonyms have already been printed, then
         *  we just skip this symbol and go on to process the next.
         *
         */
        if (t_tot != 0.0)               /* Convert to percent. */
                t_tot = 100.0/t_tot;    /* Prevent divide-by-zero fault */
	for (;print_data; print_data = print_data->next) {
		if (!jflag) {
			Print("\nObject: %s\n\n",print_data->SOpath);
			if (print_data->slist != NULL)	
				print_hdr();
			else  /* no data gathered for this object */
				print_nodata(print_data);
		}
		if (print_data->slist != NULL)
			slist = print_data->slist;
		else
			continue;
		n_syms = print_data->n_syms;
		n_snyms = 0;
		snymCapacity = n_syms/2;
	        snymList = snymp = (struct snymEntry *)
			_lprof_Malloc(snymCapacity, sizeof(struct snymEntry));
		{        /* pass 1 */
			unsigned long thisaddr;
			unsigned long lastaddr = slist->sl_addr;/* use 1st sym as 'last/prior symbol' */
			int lastWasSnym=0;              /* 1st can't be snym yet-no aliases seen */
			int thisIsSnym;

			OLD_DEBUG(int totsnyms=0; int totseries=0; struct slist *lastslp= slist; ) ;

			/* NB loop starts with 2nd symbol, loops over n_syms-1 symbols!
	*/

			for( n=n_syms-1, slp=slist+1; --n >=0; slp++ ) {
				thisaddr = slp->sl_addr;
				thisIsSnym = (thisaddr == lastaddr);

				if ( thisIsSnym ) {
				/* gotta synonym */
					if( !lastWasSnym ) {

						OLD_DEBUG(
						if (debug_value)  {
						Fprint(
							stderr,
							"Synonym series:\n1st->\t%s at address %x, ct=%ld, time=%f\n",
							lastslp->sl_name,lastaddr,lastslp->sl_count,lastslp->sl_time
							);
						totseries++;
						totsnyms++;
					}
					);
					 /* this is the Second! of a series */
						snymp = ( n_snyms++ == 0 ? snymList : snymp + 1 );
						snymp->howMany=1;   /*gotta count 1st one!! */
						snymp->sym_addr = slp->sl_addr;
						 /*zero summary statistics */
						snymp->tot_sl_count=0;
						snymp->tot_sl_time=0.0;
						/*turn off the reported flag */
						snymp->snymReported=0;
					} /* !lastWasSnym */

					OLD_DEBUG( if (debug_value)  {
						Fprint( stderr,
						"\t%s at address %x, ct=%ld, time=%f\n",
						slp->sl_name, thisaddr, slp->sl_count, slp->sl_time
						);
						totsnyms++;
					}
					);
				 /* ok - bump count for snym, and note its Finding */
					snymp->howMany++;
					 /* and update the summary statistics */
					snymp->tot_sl_count += slp->sl_count;
					 snymp->tot_sl_time += slp->sl_time;
				} /* thisIsSnym */

				lastaddr = thisaddr;
				lastWasSnym = thisIsSnym ;

				OLD_DEBUG( if (debug_value) lastslp = slp; );

			} /* for */
			OLD_DEBUG( if (debug_value)  {
				Fprint(stderr,"Total #series %d, #synonyms %d\n", totseries, totsnyms);
			}
			);
		}  /* pass 1 */

        /*
        Most of the heavy work is done now.  Only minor stuff remains.
        The symbols are currently in address order and must be re-sorted
        if desired in a different order.  Report generating options
        include "-o" or "-x": Include symbol address, which causes another colum
n
        in the output; and "-z": Include symbols in report even if zero
        time and call count.  Symbols not in profiling range are excluded
        in any case.  Following the main body of the report, the "-s"
        option causes certain additional information to be printed.  */

		if (sort && !jflag)       
		/* If comparison routine given  and not printing separate table
		   for each shared object, then use it. */
			qsort((char *)slist, (unsigned)n_syms, sizeof(struct slist), sort);


		n_nonzero = 0;
		for (n = n_syms, slp = slist; --n >= 0; slp++) {

			 /* if a snym symbol, use summarized stats, else use indiv. */
			if( (snymp=getSnymEntry( slp->sl_addr )) != 0 ) {
				slp->sl_count = snymp->tot_sl_count;
				slp->sl_time = snymp->tot_sl_time;

			}
			 /* if a snym and already reported, skip this entry */
			if ( snymp && snymp->snymReported )
				continue;
			 /* Don't do entries with no action. */
			if (slp->sl_time == 0.0 && slp->sl_count == 0 && !(flags & F_ZSYMS))
				continue;

			 /* count number of entries (i.e. symbols) printed */
			if ( snymp )
				n_nonzero += snymp->howMany; /* add for each snym */
			else
				n_nonzero++;
			cum_t += slp->sl_time; 

			if (!jflag) {
				if (flags & F_PADDR)    /* Printing address of symbol? */
					Print(aformat, slp->sl_addr);
				Print("%6.1f%8.2f%8.2f", slp->sl_time * t_tot, slp->sl_time, cum_t);
				fdigits = 0;
				if (slp->sl_count) {            /* Any calls recorded? */
				/* Get reasonable number of fractional digits to print. */
					fdigits = fprecision(slp->sl_count);
					Print("%8ld%#*.*f", slp->sl_count, fdigits+8, fdigits,
					    1000.0*slp->sl_time/slp->sl_count);
					Print("%*s", 6-fdigits, " ");
				} else 
					Print("%22s", " ");

			 /* now print the name (or comma-separated list of names,
			  * for synonym symbols).
			  */
				if (snymp) {
					printSnymNames(slp, snymp);
					snymp->snymReported = 1;	/* Mark it done */
				} else 
					(void)puts(slp->sl_name);       /* print the one name */
			} else {
				if (prt_details == NULL)
					tmp_details = prt_details = (Prt_details *)_lprof_Malloc(n_syms, (sizeof(Prt_details)));
				else {
					prt_details = (Prt_details *) realloc(prt_details, (n_syms + n_print) * sizeof(Prt_details));
					tmp_details= prt_details + n_print;
				}

				tmp_details->sl_time = slp->sl_time;
				tmp_details->sl_count = slp->sl_count;
				tmp_details->sl_addr = slp->sl_addr;
				tmp_details->sl_name = symbol_name(slp,snymp);
				if (snymp) 
					snymp->snymReported = 1;	/* Mark it done */
				tmp_details->SOname = print_data->SOpath;
				tmp_details++;
				n_print++;

			}
		}
		if (flags & F_VERBOSE) {                /* Extra info? */
			int tot = tot_fcn_cnt;

			while ((tot -= FCNTOT) > 0)
				ccnt++;
			if (jflag) {
					/* total data */
				tot_n_zero += n_nonzero;
				tot_n_syms += n_syms;
				tot_symttl += print_data->symttl;
			} else {
			

				print_sum(n_nonzero, n_syms, print_data->symttl);
				tot_n_zero = 0;
				tot_n_syms = 0;
				tot_symttl = 0;
			}  
		}
		Print("\n");
	}
	if (jflag) {
		print_hdr();
		cum_t = 0.0;
	        if (sort)
			qsort(prt_details, (unsigned) n_print, sizeof(Prt_details), sort);
		while (n_print --) {
			cum_t += prt_details->sl_time;
			if (flags & F_PADDR)    /* Printing address of symbol? */
				Print(aformat, prt_details->sl_addr);
			Print("%6.1f%8.2f%8.2f", prt_details->sl_time * t_tot, prt_details->sl_time, cum_t);
			fdigits = 0;
			if (prt_details->sl_count) {
			   /* Any calls recorded? */
                /* Get reasonable number of fractional digits to print. */
				fdigits = fprecision(prt_details->sl_count);
				Print("%8ld%#*.*f", prt_details->sl_count, fdigits+8, fdigits, 1000.0* prt_details->sl_time/prt_details->sl_count);
				Print("%*s", 6-fdigits, " ");
			} else 
				Print("%22s", " ");
 
			Print("%s:",prt_details->SOname);

	
                 /* now print the name (or comma-separated list of names,
                  * for synonym symbols).
                  */

			{

				char **names = prt_details->sl_name;
				int i = 0;
				(void) fputs(names[i++], stdout);
				while (* names[i] != NULL) {
					(void) fputs(", ", stdout);
					(void) fputs(names[i++],stdout); /* print the name */
				}
				(void)putchar('\n');
			}
			prt_details++;
		}
		if (flags & F_VERBOSE)               /* Extra info? */
			print_sum(tot_n_zero, tot_n_syms, tot_symttl);
	} /* if jflag */
	if (flags & F_VERBOSE)                 /* Extra info? */
		Fprint(stderr, "\n Total of %5d/%6-d call counts used\n", tot_fcn_cnt, ccnt*FCNTOT);
}
