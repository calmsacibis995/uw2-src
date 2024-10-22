/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)compress:compress.c	1.4.5.16"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
*/
/*
 *	Copyright (c) 1986, 1987, 1988, 1989 The Santa Cruz Operation, Inc.
 *	All rights reserved.
 *
 *	Copyright (c) 1986 Regents of the University of California.
 *	All rights reserved.  The Berkeley software License Agreement
 *	specifies the terms and conditions for redistribution.
 *
 *	Copyright (c) 1986, 1987, 1988, Sun Microsystems, Inc.
 *	All Rights Reserved.
 */

/* 
 * Compress - data compression program 
 */
#define	min(a,b)	((a>b) ? b : a)

/*
 * machine variants which require cc -Dmachine:  pdp11, z8000, pcxt
 */

/*
 * Set USERMEM to the maximum amount of physical user memory available
 * in bytes.  USERMEM is used to determine the maximum BITS that can be used
 * for compression.
 *
 * SACREDMEM is the amount of physical memory saved for others; compress
 * will hog the rest.
 */
#ifndef SACREDMEM
#define SACREDMEM	0
#endif

#ifndef USERMEM
# define USERMEM 	450000	/* default user memory */
#endif

#ifdef USERMEM
# if USERMEM >= (433484+SACREDMEM)
#  define PBITS	16
# else
#  if USERMEM >= (229600+SACREDMEM)
#   define PBITS	15
#  else
#   if USERMEM >= (127536+SACREDMEM)
#    define PBITS	14
#   else
#    if USERMEM >= (73464+SACREDMEM)
#     define PBITS	13
#    else
#     define PBITS	12
#    endif
#   endif
#  endif
# endif
# undef USERMEM
#endif /* USERMEM */

#ifdef PBITS		/* Preferred BITS for this memory size */
# ifndef BITS
#  define BITS PBITS
# endif /* BITS */
#endif /* PBITS */

#if BITS == 16
# define HSIZE	69001		/* 95% occupancy */
#endif
#if BITS == 15
# define HSIZE	35023		/* 94% occupancy */
#endif
#if BITS == 14
# define HSIZE	18013		/* 91% occupancy */
#endif
#if BITS == 13
# define HSIZE	9001		/* 91% occupancy */
#endif
#if BITS <= 12
# define HSIZE	5003		/* 80% occupancy */
#endif


/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
#if BITS > 15
typedef long int	code_int;
#else
typedef int		code_int;
#endif

typedef long int	  count_int;

 typedef	unsigned char	char_type;

char_type magic_header[] = { "\037\235" };	/* 1F 9D */

/* Defines for third byte of header */
#define BIT_MASK	0x1f
#define BLOCK_MASK	0x80
/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
   a fourth header byte (for expansion).
*/
#define INIT_BITS 9			/* initial number of bits/code */

/*
 * compress.c - File compression ala IEEE Computer, June 1984.
 */
static char rcs_ident[] = "$Header: compress.c 1.2 91/09/09 $";

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <utime.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <libgen.h>
#include <pfmt.h>
#include <limits.h>
#include <langinfo.h>

/* Locally-defined void functions. */
void	compress(),
	output(),
	decompress(),
	writeerr(),
	copystat(),
	cl_block(),
	cl_hash(),
	prratio(),
	do_Pflag(),
	version();
     
int n_bits;				/* number of bits/code */
int maxbits = BITS;			/* user settable max # bits/code */
code_int maxcode;			/* maximum code, given n_bits */
code_int maxmaxcode = 1 << BITS;	/* should NEVER generate this code */
# define MAXCODE(n_bits)	((1 << (n_bits)) - 1)

count_int htab [HSIZE];
unsigned short codetab [HSIZE];

#define htabof(i)	htab[i]
#define codetabof(i)	codetab[i]
code_int hsize = HSIZE;			/* for dynamic table sizing */
count_int fsize;

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i)	codetabof(i)
# define tab_suffixof(i)	((char_type *)(htab))[i]
# define de_stack		((char_type *)&tab_suffixof(1<<BITS))

code_int free_ent = 0;			/* first unused entry */
int exit_stat = 0;			/* per-file status */
int perm_stat = 0;			/* permanent status */
int zflg = 0;				/* zcat called */
int uflg = 0;				/* uncompress called */
int cflg = 0;				/* compress called */

code_int getcode();

int nomagic = 0;	/* Use a 3-byte magic number header, unless old file */
int zcat_flg = 0;	/* Write output on stdout, suppress messages */
int precious = 1;	/* Don't unlink output file on interrupt */
int quiet = 1;		/* don't tell me about compression */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
int block_compress = BLOCK_MASK;
int clear_flg = 0;
long int ratio = 0;
#define CHECK_GAP 10000	/* ratio check interval */
count_int checkpoint = CHECK_GAP;
/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */ 
#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */

int force = 0;
char *ofname;
int rpipe, Pflag=0;		/* Read end of pipe and flag. */
char *ctmp="Comprtmp";		/* Tmp file used in do_Pflag. */
#ifdef DEBUG
int verbose = 0;
int debug = 0;
#endif /* DEBUG */
void (*oldint)();
int bgnd_flag;

int do_decomp = 0;
int save_errno = 0;
extern int opterr, optind;
extern char *optarg;

/*****************************************************************
 * TAG( main )
 *
 * Algorithm from "A Technique for High Performance Data Compression",
 * Terry A. Welch, IEEE Computer Vol 17, No 6 (June 1984), pp 8-19.
 *
 * Usage: compress [-dfvc] [-b bits] [file ...]
 * Inputs:
 *	-d:	    If given, decompression is done instead.
 *
 *      -c:         Write output on stdout, don't remove original.
 *
 *      -b:         Parameter limits the max number of bits/code.
 *
 *	-f:	    Forces output file to be generated, even if one already
 *		    exists, and even if no space is saved by compressing.
 *		    If -f is not used, the user will be prompted if stdin is
 *		    a tty, otherwise, the output file will not be overwritten.
 *
 *      -v:	    Write compression statistics
 *
 *      -P:         Parameter is the read end of a pipe. File names are read
 *                  from pipe until a Null is read or the pipe is closed.
 *                  Always overwrites the original file.
 *
 *      file ...:   Files to be compressed.  If none specified, and not
 *                  -P option then stdin is used.
 * Outputs:
 *	file.Z:	    Compressed form of file with same mode, owner, and utimes
 * 	or stdout   (if stdin used as input)
 *
 * Assumptions:
 *	When filenames are given, replaces with the compressed version
 *	(.Z suffix) only if the file decreases in size.
 * Algorithm:
 * 	Modified Lempel-Ziv method (LZW).  Basically finds common
 * substrings and replaces them with a variable size code.  This is
 * deterministic, and can be done on the fly.  Thus, the decompression
 * procedure needs no input table, but tracks the way the table was built.
 */

main( argc, argv )
register int argc; char **argv;
{
    int overwrite = 0;	/* Do not overwrite unless given -f flag */
    int c, errflg;
    char *tempname = (char *)NULL;
    char **filelist, **fileptr;
    char *tmp_p;
    char *cp;
    struct stat statbuf;
    struct stat savestat;
    extern void onintr(), oops();
    long name_max; /* limit on file name length */
    char *yesp;
    char *nop;
    char response[MAX_INPUT];
    int  i;

    (void)setlocale(LC_ALL,"");
    (void)setcat("uxcore");
    /* This bg check only works for sh. */
    if ( (oldint = signal ( SIGINT, SIG_IGN )) != SIG_IGN ) {
	signal ( SIGINT, onintr );
	signal ( SIGSEGV, oops );
    }
    bgnd_flag = oldint != SIG_DFL;
    errflg = 0;
    
    filelist = fileptr = (char **)(malloc(argc * sizeof(*argv)));
    *filelist = NULL;

    if ((cp = strrchr(argv[0], '/')) != (char *)NULL)
	cp++;
    else 
	cp = argv[0];

    if(strcmp(cp, "uncompress") == 0) {
	do_decomp = 1;
	uflg = 1;
	(void)setlabel("UX:uncompress");
    } else if(strcmp(cp, "zcat") == 0) {
	do_decomp = 1;
	zcat_flg = 1;
	zflg = 1;
	(void)setlabel("UX:zcat");
    } else {
	cflg = 1;
	(void)setlabel("UX:compress");
    }

    /* Argument Processing
     * All flags are optional.
     * -D => debug
     * -V => print Version; debug verbose
     * -d => do_decomp
     * -v => unquiet
     * -f => force overwrite of output file
     * -n => no header: useful to uncompress old files
     * -b maxbits => maxbits.  If -b is specified, then maxbits MUST be
     *	    given also.
     * -c => cat all output to stdout
     * -C => generate output compatible with compress 2.0.
     * -P pipe_desc => Pass file names through a pipe in <stdio.h> defined
     *       BUFSIZ data chunks whose read end is defined by pipe_desc.
     *       Always overwrites original file.
     * If a string is left, must be an input filename.
     */

     if (cflg) { 	/* compress called */
         while ((c = getopt(argc, argv, "b:cCdDfFnP:qvV")) != EOF)
		    switch (c) {
#ifdef DEBUG
		    case 'D':
			debug = 1;
			break;
		    case 'V':
			verbose = 1;
			version();
			break;
#else
		    case 'V':
			version();
			break;
#endif /* DEBUG */
		    case 'v':
			quiet = 0;
			break;
		    case 'd':
			do_decomp = 1;
			break;
		    case 'f':
		    case 'F':
			overwrite = 1;
			force = 1;
			break;
		    case 'n':
			nomagic = 1;
			break;
		    case 'C':
			block_compress = 0;
			break;
		    case 'b':
			maxbits = strtol(optarg, &tmp_p, 10);
			if (*tmp_p != (char)NULL) {
				pfmt(stderr, MM_ERROR, ":786:Invalid argument \"%s\" for -%c\n", optarg, c);
				errflg++;
				break;
			}
    			if(maxbits < INIT_BITS) maxbits = INIT_BITS;
    			if (maxbits > BITS) maxbits = BITS;
			break;
		    case 'c':
			zcat_flg = 1;
			break;
		    case 'q':
			quiet = 1;
			break;
                    case 'P':
			rpipe = strtol(optarg, &tmp_p, 10);
			if (*tmp_p != (char)NULL) {
				pfmt(stderr, MM_ERROR, ":786:Invalid argument \"%s\" for -%c\n", optarg, c);
				errflg++;
				break;
			}
			Pflag=1;
                        break;
		    case '?':
			errflg++;
			break;
		}
    	if (errflg) {
#ifdef DEBUG
	    fprintf(stderr,"Usage: compress [-cfvVdD] [-b maxbits] [file ...]\n");
#else
	    pfmt(stderr, MM_ACTION,":787:Usage: compress [-cfv] [-b maxbits] [file ...]\n");
#endif /* DEBUG */
	    exit(1);
    	}
    } else if (uflg) { 	/* uncompress */
         while ((c = getopt(argc, argv, "cfP:v")) != EOF)
		    switch (c) {
		    case 'c':
			zcat_flg = 1;
			break;
		    case 'f':
			overwrite = 1;
			break;
                    case 'P':
			rpipe = strtol(optarg, &tmp_p, 10);
			if (*tmp_p != (char)NULL) {
				pfmt(stderr, MM_ERROR, ":786:Invalid argument \"%s\" for -%c\n", optarg, c);
				errflg++;
				break;
			}
			Pflag=1;
                        break;
		    case 'v':
			quiet = 0;
			break;
		    case '?':
			errflg++;
			break;
		   }
         if (errflg) {
		pfmt(stderr, MM_ACTION,":788:Usage: uncompress [-cfv] [file ...]\n");
		exit(1);
    	 }
    } else { 	/* zcat */
         while ((c = getopt(argc, argv, "")) != EOF)
		    switch (c) {
		    case '?':
			errflg++;
			break;
		   }
	if (errflg) {
		pfmt(stderr, MM_ACTION,":789:Usage: zcat [file ...]\n");
		exit(1);
    	}
    }

    /* Build input file list */
    for ( ; optind < argc; optind++)
	*fileptr++ = argv[optind];
    *fileptr = NULL;

    maxmaxcode = 1 << maxbits;

    /* Read names from a pipe and compress/decompress overwriting */
    /* the original file.                                         */
    if (Pflag)  do_Pflag();

    if (*filelist != NULL) {
	for (fileptr = filelist; *fileptr; fileptr++) {
	    save_errno = 0;
	    exit_stat = 0;
	    if (do_decomp) {	/* DECOMPRESSION */
		/* Check for .Z suffix */
		if (strcmp(*fileptr + strlen(*fileptr) - 2, ".Z") != 0) {
		    /*
		     * No .Z, so tack one on.
		     * (For allocating temporary string, add 3 to 
		     * strlen: 1 for terminating NULL, 2 for ".Z".)
		     */
		    tempname = (char *)malloc(strlen(*fileptr) + 3);
		    if (tempname == (char *) NULL) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		    }
		    strcpy(tempname, *fileptr);
		    strcat(tempname, ".Z");
		    *fileptr = tempname;
		}
	    	if (lstat(*fileptr, &savestat)) {
		    save_errno = errno;
	    	}
		/* Open input file */
		if ((freopen(*fileptr, "r", stdin)) == NULL) {
		    pfmt(stderr, MM_ERROR,
			":37:%s: %s\n", *fileptr, strerror(errno));
		    perm_stat = 1;
		    continue;
		}
		/* Check the magic number */
		if (nomagic == 0) {
		    if ((getchar() != (magic_header[0] & 0xFF))
		     || (getchar() != (magic_header[1] & 0xFF))) {
			pfmt(stderr, MM_ERROR,
				":790:%s: not in compressed format\n", *fileptr);
			perm_stat = 1;
		        continue;
		    }
		    maxbits = getchar();	/* set -b from file */
		    block_compress = maxbits & BLOCK_MASK;
		    maxbits &= BIT_MASK;
		    maxmaxcode = 1 << maxbits;
		    if(maxbits > BITS) {
			pfmt(stderr, MM_ERROR,
	":791:%s: compressed with %1$d bits, can only handle %2$d bits\n",
			*fileptr, maxbits, BITS);
			continue;
		    }
		}
		/* Generate output filename */
		if ((ofname = strdup(*fileptr)) == (char *)NULL) {
		    fprintf(stderr, "%s\n", strerror(errno));
		    exit(1);
		}
		ofname[strlen(*fileptr) - 2] = '\0';  /* Strip off .Z */

	    } else {	/* COMPRESSION */
		if (strcmp(*fileptr + strlen(*fileptr) - 2, ".Z") == 0) {
	    	    exit_stat = 1;
		    pfmt(stderr, MM_ERROR,
			":792:%s: already has .Z suffix -- no change\n",
			*fileptr);
		    continue;
		}
	    	if (lstat(*fileptr, &savestat)) {
		    save_errno = errno;
	    	}
		/* Open input file */
		if ((freopen(*fileptr, "r", stdin)) == NULL) {
		    pfmt(stderr, MM_ERROR,
			":37:%s: %s\n", *fileptr, strerror(errno));
		    perm_stat = 1;
		    continue;
		}
		stat ( *fileptr, &statbuf );
		fsize = (long) statbuf.st_size;
		/*
		 * tune hash table size for small files -- ad hoc,
		 * but the sizes match earlier #defines, which
		 * serve as upper bounds on the number of output codes. 
		 */
		hsize = HSIZE;
		if ( fsize < (1 << 12) )
		    hsize = min ( 5003, HSIZE );
		else if ( fsize < (1 << 13) )
		    hsize = min ( 9001, HSIZE );
		else if ( fsize < (1 << 14) )
		    hsize = min ( 18013, HSIZE );
		else if ( fsize < (1 << 15) )
		    hsize = min ( 35023, HSIZE );
		else if ( fsize < 47000 )
		    hsize = min ( 50021, HSIZE );

		/* Generate output filename.
		 * (For allocating ofname string, add 3 to 
		 * strlen: 1 for terminating NULL, 2 for ".Z".)
		 */
		ofname = (char *)malloc(strlen(*fileptr) + 3);
		if (ofname == (char *)NULL) {
		    fprintf(stderr, "%s\n", strerror(errno));
		    exit(1);
		}
		strcpy(ofname, *fileptr);

		/* Get file name length limit - if possible */
		errno = 0;
		if (((name_max = pathconf(ofname, _PC_NAME_MAX)) == -1)  
		    && (errno != 0)) {
	    	        pfmt(stderr, MM_ERROR,
			    ":37:%s: %s\n", ofname, strerror(errno));
		        perm_stat = 1;
		        continue;
		}

		/* Short filenames */
		if ((cp = strrchr(ofname,'/')) != (char *)NULL)
		    cp++;
		else
		    cp = ofname;

		/* In cases where pathconf without errno set, we may still be
		 * able to create the file - try this second method.
 		 */
		if (name_max != -1) {
		    if ((long)strlen(cp) > (name_max - 2)) {
		    	pfmt(stderr, MM_ERROR,
			    ":793:%s: filename too long to tack on .Z\n",cp);
		    	continue;
		    }
		    strcat(ofname, ".Z");
		} else { 
		    strcat(ofname, ".Z");
		    if (creat_check(ofname) == 0) {
		   	pfmt(stderr, MM_ERROR,
			    ":793:%s: filename too long to tack on .Z\n",*fileptr);
			continue;
		    }
		}
	    }
	    /* Check for overwrite of existing file */
	    if (overwrite == 0 && zcat_flg == 0) {
		if (stat(ofname, &statbuf) == 0) {
		    yesp = nl_langinfo(YESSTR);
		    nop = nl_langinfo(NOSTR);
		    strcpy(response, nop);
		    pfmt(stderr, MM_WARNING, ":794:%s already exists\n", ofname);
		    if (bgnd_flag == 0 && isatty(2)) {
			pfmt(stderr, MM_NOSTD,
			 ":795:do you wish to overwrite %s (y or n)? ",ofname);
			fflush(stderr);
			if (read(2, response, MAX_INPUT) < 0)
				pfmt(stderr, MM_ERROR, ":796:stderr: %s\n",
					strerror(errno));
			for( i=0; response[i] != '\n'; ++i)
				;
			if ( i > 0)
				response[i] = '\0';
		    }
		    if (strncmp(response,yesp,strlen(response)) != 0) {
			pfmt(stderr, MM_NOSTD, ":797:\tnot overwritten\n");
			continue;
		    }
		}
	    }
	    if(zcat_flg == 0) {		/* Open output file */
		if (freopen(ofname, "w", stdout) == NULL) { 
			pfmt(stderr, MM_ERROR,
				":37:%s: %s\n", ofname, strerror(errno));
		        perm_stat = 1;
		        continue;
		 }
		precious = 0;
		if(!quiet)
			pfmt(stderr, MM_INFO, ":63:%s: ", *fileptr);
	    }

	    /* Actually do the compression/decompression */
	    if (do_decomp == 0)	
		compress();
#ifndef DEBUG
	    else			
		decompress();
#else
	    else if (debug == 0)	
		decompress();
	    else			
		printcodes();
	    if (verbose)		
		dump_tab();
#endif /* DEBUG */
	    if(zcat_flg == 0) {
		copystat(*fileptr, ofname, &savestat);	/* Copy stats */
		precious = 1;
		if((exit_stat != 0) || (!quiet))
			putc('\n', stderr);
	    }

    	    (void)free(ofname);
	    if (tempname != (char *)NULL) {
	    	    (void)free(tempname);
		    tempname = (char *)NULL;
	    }
	}  /* end for (fileptr=filelist; *fileptr; fileptr++) */

    } else {		/* Standard input */
	if (do_decomp == 0) {
		compress();
#ifdef DEBUG
		if(verbose)		dump_tab();
#endif /* DEBUG */
		if(!quiet)
			putc('\n', stderr);
	} else {
	    /* Check the magic number */
	    if (nomagic == 0) {
		if ((getchar()!=(magic_header[0] & 0xFF))
		 || (getchar()!=(magic_header[1] & 0xFF))) {
		    pfmt(stderr, MM_ERROR,
			":798:stdin: not in compressed format\n");
		    exit(1);
		}
		maxbits = getchar();	/* set -b from file */
		block_compress = maxbits & BLOCK_MASK;
		maxbits &= BIT_MASK;
		maxmaxcode = 1 << maxbits;
		fsize = 100000;		/* assume stdin large for USERMEM */
		if(maxbits > BITS) {
			pfmt(stderr, MM_ERROR,
	":799:stdin: compressed with %1$d bits, can only handle %2$d bits\n",
			maxbits, BITS);
			exit(1);
		}
	    }
#ifndef DEBUG
	    decompress();
#else
	    if (debug == 0)	decompress();
	    else		printcodes();
	    if (verbose)	dump_tab();
#endif /* DEBUG */
	}
    }
    exit(perm_stat ? perm_stat : exit_stat);
}

static int offset;
long int in_count = 1;			/* length of input */
long int bytes_out;			/* length of compressed output */
long int out_count = 0;			/* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the 
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

void
compress() {
    register long fcode;
    register code_int i = 0;
    register int c;
    register code_int ent;
    register int disp;
    register code_int hsize_reg;
    register int hshift;

    if (nomagic == 0) {
	putchar(magic_header[0]); putchar(magic_header[1]);
	putchar((char)(maxbits | block_compress));
	if(ferror(stdout))
		writeerr();
    }

    offset = 0;
    bytes_out = 3;		/* includes 3-byte header mojo */
    out_count = 0;
    clear_flg = 0;
    ratio = 0;
    in_count = 1;
    checkpoint = CHECK_GAP;
    maxcode = MAXCODE(n_bits = INIT_BITS);
    free_ent = ((block_compress) ? FIRST : 256 );

    ent = getchar ();

    hshift = 0;
    for ( fcode = (long) hsize;  fcode < 65536L; fcode *= 2L )
    	hshift++;
    hshift = 8 - hshift;		/* set hash code range bound */

    hsize_reg = hsize;
    cl_hash( (count_int) hsize_reg);		/* clear hash table */

    while ( (c = getchar()) != EOF ) {
	in_count++;
	fcode = (long) (((long) c << maxbits) + ent);
 	i = ((c << hshift) ^ ent);	/* xor hashing */

	if ( htabof (i) == fcode ) {
	    ent = codetabof (i);
	    continue;
	} else if ( (long)htabof (i) < 0 )	/* empty slot */
	    goto nomatch;
 	disp = hsize_reg - i;		/* secondary hash (after G. Knott) */
	if ( i == 0 )
	    disp = 1;
probe:
	if ( (i -= disp) < 0 )
	    i += hsize_reg;

	if ( htabof (i) == fcode ) {
	    ent = codetabof (i);
	    continue;
	}
	if ( (long)htabof (i) > 0 ) 
	    goto probe;
nomatch:
	output ( (code_int) ent );
	out_count++;
 	ent = c;
	if ( free_ent < maxmaxcode ) {
 	    codetabof (i) = free_ent++;	/* code -> hashtable */
	    htabof (i) = fcode;
	}
	else if ( (count_int)in_count >= checkpoint && block_compress )
	    cl_block ();
    }
    /*
     * Put out the final code.
     */
    output( (code_int)ent );
    out_count++;
    output( (code_int)-1 );

    /*
     * Print out stats on stderr
     */
    if(zcat_flg == 0 && !quiet) {
#ifdef DEBUG
	fprintf( stderr,
		"%ld chars in, %ld codes (%ld bytes) out, compression factor: ",in_count, out_count, bytes_out );
	prratio( stderr, in_count, bytes_out );
	fprintf( stderr, "\n");
	fprintf( stderr, "\tCompression as in compact: " );
	prratio( stderr, in_count-bytes_out, in_count );
	fprintf( stderr, "\n");
	fprintf( stderr, "\tLargest code (of last block) was %d (%d bits)\n",
		free_ent - 1, n_bits );
#else /* !DEBUG */
	pfmt( stderr, MM_NOSTD,":800:Compression: " );
	prratio( stderr, in_count-bytes_out, in_count );
#endif /* DEBUG */
    }
    if(bytes_out > in_count)	/* exit(2) if no savings */
	exit_stat = 2;
    return;
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 * 	code:	A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *		that n_bits =< (long)wordsize - 1.
 * Outputs:
 * 	Outputs code to the file.
 * Assumptions:
 *	Chars are 8 bits long.
 * Algorithm:
 * 	Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static char buf[BITS];

char_type lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};


void
output( code )
code_int  code;
{
#ifdef DEBUG
    static int col = 0;
#endif /* DEBUG */

    /*
     * On the VAX, it is important to have the register declarations
     * in exactly the order given, or the asm will break.
     */
    register int r_off = offset, bits= n_bits;
    register char * bp = buf;

#ifdef DEBUG
	if ( verbose )
	    fprintf( stderr, "%5d%c", code,
		    (col+=6) >= 74 ? (col = 0, '\n') : ' ' );
#endif /* DEBUG */
    if ( code >= 0 ) {
/* 
 * byte/bit numbering on the VAX is simulated by the following code
 */
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/*
	 * Since code is always >= 8 bits, only need to mask the first
	 * hunk on the left.
	 */
	*bp = (*bp & rmask[r_off]) | (code << r_off) & lmask[r_off];
	bp++;
	bits -= (8 - r_off);
	code >>= 8 - r_off;
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
	    *bp++ = code;
	    code >>= 8;
	    bits -= 8;
	}
	/* Last bits. */
	if(bits)
	    *bp = code;
	offset += n_bits;
	if ( offset == (n_bits << 3) ) {
	    bp = buf;
	    bits = n_bits;
	    bytes_out += bits;
	    do
		putchar(*bp++);
	    while(--bits);
	    offset = 0;
	}

	/*
	 * If the next entry is going to be too big for the code size,
	 * then increase it, if possible.
	 */
	if ( free_ent > maxcode || (clear_flg > 0))
	{
	    /*
	     * Write the whole buffer, because the input side won't
	     * discover the size increase until after it has read it.
	     */
	    if ( offset > 0 ) {
		if( fwrite( buf, 1, n_bits, stdout ) != n_bits)
			writeerr();
		bytes_out += n_bits;
	    }
	    offset = 0;

	    if ( clear_flg ) {
    	        maxcode = MAXCODE (n_bits = INIT_BITS);
	        clear_flg = 0;
	    }
	    else {
	    	n_bits++;
	    	if ( n_bits == maxbits )
		    maxcode = maxmaxcode;
	    	else
		    maxcode = MAXCODE(n_bits);
	    }
#ifdef DEBUG
	    if ( debug ) {
		fprintf( stderr, "\nChange to %d bits\n", n_bits );
		col = 0;
	    }
#endif /* DEBUG */
	}
    } else {
	/*
	 * At EOF, write the rest of the buffer.
	 */
	if ( offset > 0 )
	    fwrite( buf, 1, (offset + 7) / 8, stdout );
	bytes_out += (offset + 7) / 8;
	offset = 0;
	fflush( stdout );
#ifdef DEBUG
	if ( verbose )
	    fprintf( stderr, "\n" );
#endif /* DEBUG */
	if( ferror( stdout ) )
		writeerr();
    }
}

/*
 * Decompress stdin to stdout.  This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  The tables used herein are shared
 * with those of the compress() routine.  See the definitions above.
 */

void
decompress() {
    register char_type *stackp;
    register int finchar;
    register code_int code, oldcode, incode;

    /*
     * As above, initialize the first 256 entries in the table.
     */
    maxcode = MAXCODE(n_bits = INIT_BITS);
    for ( code = 255; code >= 0; code-- ) {
	tab_prefixof(code) = 0;
	tab_suffixof(code) = (char_type)code;
    }
    free_ent = ((block_compress) ? FIRST : 256 );

    finchar = oldcode = getcode();
    if(oldcode == -1)	/* EOF already? */
	return;			/* Get out of here */
    putchar( (char)finchar );	/* first code must be 8 bits = char */
    if(ferror(stdout))		/* Crash if can't write */
	writeerr();
    stackp = de_stack;

    while ( (code = getcode()) > -1 ) {

	if ( (code == CLEAR) && block_compress ) {
	    for ( code = 255; code >= 0; code-- )
		tab_prefixof(code) = 0;
	    clear_flg = 1;
	    free_ent = FIRST - 1;
	    if ( (code = getcode ()) == -1 )	/* O, untimely death! */
		break;
	}
	incode = code;
	/*
	 * Special case for KwKwK string.
	 */
	if ( code >= free_ent ) {
            *stackp++ = finchar;
	    code = oldcode;
	}

	/*
	 * Generate output characters in reverse order
	 */
	while ( code >= 256 ) {
	    *stackp++ = tab_suffixof(code);
	    code = tab_prefixof(code);
	}
	*stackp++ = finchar = tab_suffixof(code);

	/*
	 * And put them out in forward order
	 */
	do
	     putchar ( *--stackp ); 
	while ( stackp > de_stack );

	/*
	 * Generate the new entry.
	 */
	if ( (code=free_ent) < maxmaxcode ) {
	    tab_prefixof(code) = (unsigned short)oldcode;
	    tab_suffixof(code) = finchar;
	    free_ent = code+1;
	} 
	/*
	 * Remember previous code.
	 */
	oldcode = incode;
    }
    fflush( stdout );
    if(ferror(stdout))
	writeerr();
}

/*****************************************************************
 * TAG( getcode )
 *
 * Read one code from the standard input.  If EOF, return -1.
 * Inputs:
 * 	stdin
 * Outputs:
 * 	code or -1 is returned.
 */

code_int
getcode() {
    /*
     * On the VAX, it is important to have the register declarations
     * in exactly the order given, or the asm will break.
     */
    register code_int code;
    static int offset = 0, size = 0;
    static char_type buf[BITS];
    register int r_off, bits;
    register char_type *bp = buf;

    if ( clear_flg > 0 || offset >= size || free_ent > maxcode ) {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 */
	if ( free_ent > maxcode ) {
	    n_bits++;
	    if ( n_bits == maxbits )
		maxcode = maxmaxcode;	/* won't get any bigger now */
	    else
		maxcode = MAXCODE(n_bits);
	}
	if ( clear_flg > 0) {
    	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	}
	size = fread( buf, 1, n_bits, stdin );
	if ( size <= 0 )
	    return -1;			/* end of file */
	offset = 0;
	/* Round size down to integral number of codes */
	size = (size << 3) - (n_bits - 1);
    }
    r_off = offset;
    bits = n_bits;
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/* Get first part (low order bits) */
	code = (*bp++ >> r_off);
	bits -= (8 - r_off);
	r_off = 8 - r_off;		/* now, offset into code word */
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
	    code |= *bp++ << r_off;
	    r_off += 8;
	    bits -= 8;
	}
	/* high order bits. */
	code |= (*bp & rmask[bits]) << r_off;
    offset += n_bits;

    return code;
}

#ifdef DEBUG
printcodes()
{
    /*
     * Just print out codes from input file.  For debugging.
     */
    code_int code;
    int col = 0, bits;

    bits = n_bits = INIT_BITS;
    maxcode = MAXCODE(n_bits);
    free_ent = ((block_compress) ? FIRST : 256 );
    while ( ( code = getcode() ) >= 0 ) {
	if ( (code == CLEAR) && block_compress ) {
   	    free_ent = FIRST - 1;
   	    clear_flg = 1;
	}
	else if ( free_ent < maxmaxcode )
	    free_ent++;
	if ( bits != n_bits ) {
	    fprintf(stderr, "\nChange to %d bits\n", n_bits );
	    bits = n_bits;
	    col = 0;
	}
	fprintf(stderr, "%5d%c", code, (col+=6) >= 74 ? (col = 0, '\n') : ' ' );
    }
    putc( '\n', stderr );
    exit( 0 );
}

code_int sorttab[1<<BITS];	/* sorted pointers into htab */

dump_tab()	/* dump string table */
{
    register int i, first;
    register ent;
#define STACK_SIZE	15000
    int stack_top = STACK_SIZE;
    register c;

    if(do_decomp == 0) {	/* compressing */
	register int flag = 1;

	for(i=0; i<hsize; i++) {	/* build sort pointers */
		if((long)htabof(i) >= 0) {
			sorttab[codetabof(i)] = i;
		}
	}
	first = block_compress ? FIRST : 256;
	for(i = first; i < free_ent; i++) {
		fprintf(stderr, "%5d: \"", i);
		de_stack[--stack_top] = '\n';
		de_stack[--stack_top] = '"';
		stack_top = in_stack((htabof(sorttab[i])>>maxbits)&0xff, 
                                     stack_top);
		for(ent=htabof(sorttab[i]) & ((1<<maxbits)-1);
		    ent > 256;
		    ent=htabof(sorttab[ent]) & ((1<<maxbits)-1)) {
			stack_top = in_stack(htabof(sorttab[ent]) >> maxbits,
						stack_top);
		}
		stack_top = in_stack(ent, stack_top);
		fwrite( &de_stack[stack_top], 1, STACK_SIZE-stack_top, stderr);
	   	stack_top = STACK_SIZE;
	}
   } else if(!debug) {	/* decompressing */

       for ( i = 0; i < free_ent; i++ ) {
	   ent = i;
	   c = tab_suffixof(ent);
	   if ( isascii(c) && isprint(c) )
	       fprintf( stderr, "%5d: %5d/'%c'  \"",
			   ent, tab_prefixof(ent), c );
	   else
	       fprintf( stderr, "%5d: %5d/\\%03o \"",
			   ent, tab_prefixof(ent), c );
	   de_stack[--stack_top] = '\n';
	   de_stack[--stack_top] = '"';
	   for ( ; ent != NULL;
		   ent = (ent >= FIRST ? tab_prefixof(ent) : NULL) ) {
	       stack_top = in_stack(tab_suffixof(ent), stack_top);
	   }
	   fwrite( &de_stack[stack_top], 1, STACK_SIZE - stack_top, stderr );
	   stack_top = STACK_SIZE;
       }
    }
}

int
in_stack(c, stack_top)
	register c, stack_top;
{
	if ( (isascii(c) && isprint(c) && c != '\\') || c == ' ' ) {
	    de_stack[--stack_top] = c;
	} else {
	    switch( c ) {
	    case '\n': de_stack[--stack_top] = 'n'; break;
	    case '\t': de_stack[--stack_top] = 't'; break;
	    case '\b': de_stack[--stack_top] = 'b'; break;
	    case '\f': de_stack[--stack_top] = 'f'; break;
	    case '\r': de_stack[--stack_top] = 'r'; break;
	    case '\\': de_stack[--stack_top] = '\\'; break;
	    default:
	 	de_stack[--stack_top] = '0' + c % 8;
	 	de_stack[--stack_top] = '0' + (c / 8) % 8;
	 	de_stack[--stack_top] = '0' + c / 64;
	 	break;
	    }
	    de_stack[--stack_top] = '\\';
	}
	return stack_top;
}
#endif /* DEBUG */

void
writeerr()
{
    pfmt(stderr, MM_ERROR, ":37:%s: %s\n", ofname, strerror(errno));
    unlink ( ofname );
    exit ( 1 );
}

void
copystat(ifname, ofname, savestat)
char *ifname, *ofname;
struct stat *savestat;
{
    mode_t mode;
    struct utimbuf timep;

    fclose(stdout);
    if (save_errno) {
    	pfmt(stderr, MM_ERROR, ":37:%s: %s\n", ifname, strerror(save_errno));
	return;
    }
    if ((savestat->st_mode & S_IFMT/*0170000*/) != S_IFREG/*0100000*/) {
	if(quiet)
	    	pfmt(stderr, MM_INFO, ":63:%s: ", ifname);
	pfmt(stderr, MM_NOSTD, ":801: -- not a regular file: unchanged");
	exit_stat = 1;
	perm_stat = 1;
    } else if (savestat->st_nlink > 1) {
	if(quiet)
	    	pfmt(stderr, MM_INFO, ":63:%s: ", ifname);
	pfmt(stderr, MM_NOSTD, ":802: -- has %d other links: unchanged",
		savestat->st_nlink - 1);
	exit_stat = 1;
	perm_stat = 1;
    } else if (exit_stat == 2 && (!force)) { /* No compression: remove file.Z */
		pfmt(stderr, MM_NOSTD, ":803: -- file unchanged");
    } else {			/* ***** Successful Compression ***** */
	exit_stat = 0;
	mode = savestat->st_mode & 07777;
	timep.actime = savestat->st_atime;
	timep.modtime = savestat->st_mtime;
	utime(ofname, &timep);	/* Update last accessed and modified times */
	if (chmod(ofname, mode))		/* Copy modes */
    	    pfmt(stderr, MM_ERROR, ":37:%s: %s\n", ofname, strerror(errno));
	chown(ofname, savestat->st_uid, savestat->st_gid); /* Copy ownership */
	if (unlink(ifname))	/* Remove input file */
    	    pfmt(stderr, MM_ERROR, ":37:%s: %s\n", ifname, strerror(errno));
	if(!quiet)
		pfmt(stderr, MM_NOSTD, ":804: -- replaced with %s", ofname);
	return;		/* Successful return */
    }

    /* Unsuccessful return -- one of the tests failed */
    if (unlink(ofname))
    	pfmt(stderr, MM_ERROR, ":37:%s: %s\n", ofname, strerror(errno));
}

void
onintr ( )
{
    if (!precious)
	unlink ( ofname );
    if (Pflag)
	unlink ( ctmp );
    exit ( 1 );
}

void
oops ( )	/* wild pointer -- assume bad input */
{
    if ( do_decomp ) 
    	pfmt ( stderr, MM_ERROR, ":805:uncompress: corrupt input\n" );
    unlink ( ofname );
    if (Pflag)
	unlink ( ctmp );
    exit ( 1 );
}

void
cl_block ()		/* table clear for block compress */
{
    register long int rat;

    checkpoint = in_count + CHECK_GAP;
#ifdef DEBUG
	if ( debug ) {
    		fprintf ( stderr, "count: %ld, ratio: ", in_count );
     		prratio ( stderr, in_count, bytes_out );
		fprintf ( stderr, "\n");
	}
#endif /* DEBUG */

    if(in_count > 0x007fffff) {	/* shift will overflow */
	rat = bytes_out >> 8;
	if(rat == 0) {		/* Don't divide by zero */
	    rat = 0x7fffffff;
	} else {
	    rat = in_count / rat;
	}
    } else {
	rat = (in_count << 8) / bytes_out;	/* 8 fractional bits */
    }
    if ( rat > ratio ) {
	ratio = rat;
    } else {
	ratio = 0;
#ifdef DEBUG
	if(verbose)
		dump_tab();	/* dump string table */
#endif
 	cl_hash ( (count_int) hsize );
	free_ent = FIRST;
	clear_flg = 1;
	output ( (code_int) CLEAR );
#ifdef DEBUG
	if(debug)
    		fprintf ( stderr, "clear\n" );
#endif /* DEBUG */
    }
}

void
cl_hash(hsize)		/* reset code table */
	register count_int hsize;
{
	register count_int *htab_p = htab+hsize;
	register long i;
	register long m1 = -1;

	i = hsize - 16;
 	do {				/* might use Sys V memset(3) here */
		*(htab_p-16) = m1;
		*(htab_p-15) = m1;
		*(htab_p-14) = m1;
		*(htab_p-13) = m1;
		*(htab_p-12) = m1;
		*(htab_p-11) = m1;
		*(htab_p-10) = m1;
		*(htab_p-9) = m1;
		*(htab_p-8) = m1;
		*(htab_p-7) = m1;
		*(htab_p-6) = m1;
		*(htab_p-5) = m1;
		*(htab_p-4) = m1;
		*(htab_p-3) = m1;
		*(htab_p-2) = m1;
		*(htab_p-1) = m1;
		htab_p -= 16;
	} while ((i -= 16) >= 0);
    	for ( i += 16; i > 0; i-- )
		*--htab_p = m1;
}

void
prratio(stream, num, den)
FILE *stream;
long int num, den;
{
	register int q;			/* Doesn't need to be long */
	char	 *signstr;

	if(num > 214748L) {		/* 2147483647/10000 */
		q = num / (den / 10000L);
	} else {
		q = 10000L * num / den;		/* Long calculations, though */
	}
	if ( q < 0 ) {
		signstr = "-";
		q = -q;
	} else {
		signstr = "";
	}
	pfmt(stream, MM_NOSTD, ":806:%s%d.%02d%%", signstr, q / 100, q % 100);
}

/* do_Pflag                                                             */
/*      Read file names from the pipe and compress/decompress them   	*/
/*    	overwriting the original file. 					*/
/*      Note: <stdio.h> defined BUFSIZ chunks are read from the pipe.   */
/*      Programs using the -P option should write in BUFSIZ chunks.  	*/

void
do_Pflag() {
    	char namebuf[BUFSIZ];   /* Buffer for fnames passed through pipe.  */
    	struct utimbuf timep;
	char lng_ctmp[100], *dirname();
    	struct stat statbuf;

	/* A NULL, EOF or broken pipe will break loop. 	*/
    	while (read(rpipe, namebuf, sizeof(namebuf)) > 0) { 
	    if(namebuf[0] == '\0') break;
	    /* Open input file */
	    if ((freopen(namebuf, "r", stdin)) == NULL) {
	        pfmt(stderr, MM_ERROR,
			":37:%s: %s\n", namebuf, strerror(errno));
	        continue;
	    }
	    if (stat( namebuf, &statbuf ) < 0) {
	        pfmt(stderr, MM_ERROR,
			":37:%s: %s\n", namebuf, strerror(errno));
		continue;
	    }

	    if (do_decomp) {  /* DECOMPRESSION */
		/* Check the magic number */
		if (nomagic == 0) {
		    if ((getchar() != (magic_header[0] & 0xFF))
		     || (getchar() != (magic_header[1] & 0xFF))) {
			/* File not in compressed format. No problem. */
		    	continue;
		    }
		    maxbits = getchar();	/* set -b from file */
		    block_compress = maxbits & BLOCK_MASK;
		    maxbits &= BIT_MASK;
		    maxmaxcode = 1 << maxbits;
		    if(maxbits > BITS) {
		       pfmt(stderr, MM_ERROR,
		       ":807:%s: compressed with %d bits, can only handle %d bits\n",
		       namebuf, maxbits, BITS);
		       continue;
		    }
		}
	    } 
	    else {  /* COMPRESSION */
		fsize = (long) statbuf.st_size;
		/*
		 * tune hash table size for small files -- as below 
		 */
		hsize = HSIZE;
		if ( fsize < (1 << 12) )
		    hsize = min ( 5003, HSIZE );
		else if ( fsize < (1 << 13) )
		    hsize = min ( 9001, HSIZE );
		else if ( fsize < (1 << 14) )
		    hsize = min ( 18013, HSIZE );
		else if ( fsize < (1 << 15) )
		    hsize = min ( 35023, HSIZE );
		else if ( fsize < 47000 )
		    hsize = min ( 50021, HSIZE );
	   }
	   /* Add full path to ctmp. */
	   sprintf(lng_ctmp, "%s%s", dirname(namebuf), ctmp);
	   /* Set output filename to ctmp. */
	   if (freopen(lng_ctmp, "w", stdout) == NULL) {
		pfmt(stderr, MM_ERROR,
		    ":37:%s: %s\n", ctmp, strerror(errno));
	       continue;
	   }
	   /* Do actual compress/decompress. */
	   if (do_decomp) 
		decompress(); 
	   else
		compress();
   
	   fflush(stdout);
	   /* Remove input file. */
	   if (unlink(namebuf))		
		pfmt(stderr, MM_ERROR,
			":37:%s: %s\n", namebuf, strerror(errno));
	   /* Link tmp file to input filename. */
	   if( (link(lng_ctmp, namebuf)) < 0) {   
		pfmt(stderr, MM_ERROR,
			":37:%s: %s\n", namebuf, strerror(errno));
		pfmt(stderr, MM_ERROR,
			":808:Compress: Cannot link tmp to %s\n",namebuf);
	   }
	   /* Always unlink tmp file. */
	   unlink(lng_ctmp);	

	   /* Set time, mode and ownership for new file. */
	   timep.actime = time((long *) 0);
	   timep.modtime = statbuf.st_mtime;
	   utime(namebuf, &timep);
	   chmod(namebuf, statbuf.st_mode);
	   chown(namebuf, statbuf.st_uid, statbuf.st_gid);
	}
	/* Make sure tmp is gone. */
	unlink(lng_ctmp);			
	exit(0);
}


/* dirname					*/
/*      Return the directory portion of lname.	*/
/*						*/
char *
dirname(lname)
char *lname;
{
	char *cp, *end;
	static char buf[100];

	if (lname[0] == '/' || lname[1] == '/') 
		strcpy(buf, lname);
	else {
		strcpy(buf, "./");
		strcat(buf, lname);
	}
	end = &buf[strlen(buf)];
	for(cp = end; *cp != '/'; cp--); 
	*(cp+1)='\0';
	return(buf);
}

/* 
 * creat_check accounts for some file systems types (e.g. nfs) where a 
 * file name limit cannot be determined via pathconf.  In this case, we try
 * to create the file,if is doesn't exist, and then read the directory to
 * see if the file is there, or a truncated version (without the Z or without
 * .Z).  If the .Z file name is not there, we return 0.  If the file did
 * not exist before entering this function, it is removed.
 */
creat_check(filename)
char *filename;
{
	FILE *filed;
	DIR *dirp;
	struct dirent *direntp;
	char *basefile;
	int found = 0, exists = 0;

	if ((filed = fopen(filename,"r")) != NULL) 
		exists = 1;		/* file exists in some form */
	else if ((filed = fopen(filename,"a+")) == NULL) 
		return(0);		/* can't create file  	    */

	basefile = basename(filename);
	dirp = opendir(dirname(filename));
	while (((direntp = readdir(dirp)) != NULL) && !found)
		if (strcmp(direntp->d_name,basefile) == 0)
			found = 1;

	closedir(dirp);
	fclose(filed);
	if (exists == 0)
		unlink(filename);

	return(found);
}

void
version()
{
	fprintf(stderr, "%s, Berkeley 5.9 5/11/86\n", rcs_ident);
	pfmt(stderr, MM_NOSTD, ":809:Options: ");
#ifdef DEBUG
	fprintf(stderr, "DEBUG, ");
#endif
	pfmt(stderr, MM_NOSTD, ":810:BITS = %d\n", BITS);
}
