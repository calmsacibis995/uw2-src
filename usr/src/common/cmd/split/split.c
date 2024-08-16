/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved. 					*/

#ident	"@(#)split:split.c	1.6.1.10"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/split/split.c,v 1.1 91/02/28 20:10:37 ccs Exp $"

#include <stdio.h>
#include <sys/types.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>

#define	NUM_ALPHA	26	/* number of alphabetic characters to use */
unsigned count = 1000;
unsigned bytes;
int	suf_len = 2;
int	fnumber = 0;
int	max_outf;
char	fname[PATH_MAX + 1];
char	*ifil;
char	*ofil;
char	*tail;
char	*last;
FILE	*is;
FILE	*os;

void split_by_bytes();
void split_by_lines();
void usage();
FILE *open_file();
void close_file();

main(argc, argv)
char *argv[];
{
	register i, c, f;
	int iflg = 0;
	int ch;
	int aflg = 0;
	int bflg = 0;
	int lflg = 0;
	int numflg = 0;
	char 	*strptr;
	int	numargs;
	long	name_max;
	char	*tmphead, *tmptail;
	char	*head, *tail;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:split");

	ch = '\0';
	while(ch != EOF) {
		if (optind >= argc)
			break;
		ch = argv[optind][0];
		if ((ch == '-') && (isdigit(argv[optind][1]))) {
			if (bflg || lflg) {
				usage();
				/* NOTREACHED */
			}
			numflg = 1;
			count = atoi(argv[optind] + 1);
			optind++;
		} else if ((ch = getopt(argc, argv, "a:b:l:")) != EOF) {
			switch(ch) {
				case 'a':
					aflg = 1;
					suf_len = atoi(optarg);
					break;
				case 'b':
					if (lflg || numflg) {
						usage();
						/* NOTREACHED */
					}
					bflg = 1;
					bytes = (size_t)strtoul(optarg,
								&strptr, 10);
					if (*strptr == 'k') {
						bytes *= 1024;
					} else if (*strptr == 'm') {
						bytes *= 1048576;
					} else if (*strptr != '\0') {
						usage();
						/* NOTREACHED */
					}
					break;
				case 'l':
					if (bflg || numflg) {
						usage();
						/* NOTREACHED */
					}
					lflg = 1;
					count = atoi(optarg);
					break;
				default:
					usage();
					/* NOTREACHED */
					break;
			}
		}
	}
	if ((lflg || numflg) && (count < 1)) {
		usage();
		/* NOTREACHED */
	} else if (bflg && (bytes < 1)) {
		usage();
		/* NOTREACHED */
	}
	numargs = argc - optind;
	if ((numargs < 0) || (numargs > 2)) {
		usage();
		/* NOTREACHED */
	}

	/*
	 * At this point we may have zero, one, or two more arguments.
	 * If there are no more arguments, the input file is
	 * "stdin", and the output file prefix is "x".
	 * If there are one or two arguments and the first is "-",
	 * then the input file is "stdin", otherwise the first is
	 * the name of the input file.  If there is a second argument,
	 * it is the prefix for the output file.
	 */
	if (numargs == 0) {
		ifil = NULL;
		ofil = "x";
	} else if (numargs >= 1) {
		if(strcmp(argv[optind], "-") == 0) {
			ifil = NULL;
		} else {
			ifil = strdup(argv[optind]);
		}
		if (numargs == 2) {
			optind++;
			ofil = strdup(argv[optind]);
		} else {
			ofil = "x";
		}
	}
	if(ifil == NULL)
		is = stdin;
	else {
		if((is=fopen(ifil,"r")) == NULL) {
			exit(1);
		}
	}
	tmphead = strdup(ofil);
	tmptail = strdup(ofil);
	head = dirname(tmphead);
	tail = basename(tmptail);

	if (((name_max = pathconf(head, _PC_NAME_MAX)) == -1) &&
		(errno != 0)) {
		pfmt(stderr, MM_ERROR, ":56:%s: %s\n", head, strerror(errno));
		exit(1);
	} else if (name_max == -1) {
		char 	*tmpstr;
		int	len = strlen(ofil);

		tmpstr = (char *)malloc(len + suf_len + 1);
		strcpy(tmpstr, ofil);
		for (i = 0; i < suf_len; i++)
			*(tmpstr + len + i) = '0';
		*(tmpstr + len + i) = '\0';
		if (creat_check(tmpstr, head) == 0) {
			pfmt(stderr, MM_ERROR,
			    ":143:Output file name %s too long or -a argument %d too large\n",
			    ofil, suf_len);
			exit(1);
		}
	} else if (((long)strlen(tail)) > (name_max - suf_len)) {
		pfmt(stderr, MM_ERROR,
			":143:Output file name %s too long or -a argument %d too large\n",
			ofil, suf_len);
		exit(1);
	}


	for (i = 0, max_outf = 1; i < suf_len; i++) {
		/*
		 * Of course, this test is ludicrous, since the file 
		 * system may barf long before we create INT_MAX files 
		 * in a directory due to directory size limitations.
		 */
		if (max_outf > (INT_MAX / NUM_ALPHA)) {
			max_outf = INT_MAX;
			break;
		}
		max_outf *= NUM_ALPHA;
	}

	if (bflg)
		split_by_bytes();
	else
		split_by_lines();
	fclose(is);
	exit(0);
} 	/* main */

void
split_by_bytes()
{
	int done, fnumber;
	size_t i, rcount, numread;
	char buffer[BUFSIZ];

	for (done = 0, fnumber = 0; !done; fnumber++)
	{
		os = NULL;
		for(i = 0; i < bytes; i += numread)	/* break file */
		{
			rcount = ((bytes - i) < BUFSIZ) ? bytes - i : BUFSIZ;
			if ((numread = fread(buffer, 1, rcount, is)) <= 0) {
				done = 1;
				break; /* break out of inner loop */
			}
			/* open file only after we have read something */
			if (os == NULL) {
				os = open_file(fnumber);
			}
			if (fwrite(buffer, 1, numread, os) != numread) {
				perror(gettxt(":145", "split"));
				exit(1);
			}
		}
		close_file(os);
	}
	return;
} /* split_by_bytes */


void
split_by_lines(void)
{
	int done, fnumber;
	size_t i;
	int c;

	for (done = 0, fnumber = 0; !done; fnumber++)
	{
		os = NULL;
		for(i = 0; i < count && !done; i++)	/* break file */
		{
			do {
				if ((c = getc(is)) == EOF)
					done = 1;
				else {
					/* open file only after 
			  	 	* we have read something */
					if (os == NULL)
						os = open_file(fnumber);

					if (putc(c,os) == -1) {
						perror(gettxt(":145", "split"));
						exit(1);
					}
				}
			} while (c != '\n' && c != EOF);
		}
		close_file(os);
	}
	return;
} /* split_by_lines */


FILE *
open_file(int n)
{
	int i, j;
	FILE *f;

	if(n >= max_outf) {
		pfmt(stderr, MM_ERROR,
		     ":144:More than %d output files needed, aborting split\n",
		     max_outf);
		exit(1);
	}
	for(i = 0; ofil[i]; i++)
		fname[i] = ofil[i];
	for(j = suf_len - 1; j >= 0; j--) {
		fname[i+j] = n % NUM_ALPHA + 'a';
		n /= NUM_ALPHA;
	}
	i += suf_len;
	fname[i] = '\0';
	if((f = fopen(fname,"w")) == NULL) {
		perror(gettxt(":145", "split"));
		exit(1);
	}
	return f;
} /* open_file */


void
close_file(FILE *f)
{
	if ((f != NULL) && (fclose(f) == EOF)) {
		perror(gettxt(":145", "split"));
		exit(1);
	}
	return;
} /* close_file */

/* 
 * creat_check accounts for some file systems types (e.g. nfs) where a 
 * file name limit cannot be determined via pathconf.  In this case, we try
 * to create the file,if is doesn't exist, and then read the directory to
 * see if the file is there, or a truncated version (without the Z or without
 * .Z).  If the .Z file name is not there, we return 0.  If the file did
 * not exist before entering this function, it is removed.
 */
creat_check(filename, dir_name)
char *filename;
char *dir_name;
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
	fflush(filed);

	basefile = basename(filename);
	dirp = opendir(dir_name);
	while (((direntp = readdir(dirp)) != NULL) && !found)
		if (strcmp(direntp->d_name, basefile) == 0)
			found = 1;

	closedir(dirp);
	fclose(filed);
	if (exists == 0)
		unlink(filename);

	return(found);
}

void
usage()
{
	pfmt(stderr, MM_ERROR, ":2:Incorrect usage\n");
	pfmt(stderr, MM_ACTION,
		":140:Usage: split [-l line_count] [-a suf_length] [file [name]]\n");
	pfmt(stderr, MM_NOSTD,
		":141:\tsplit -b n[k|m] [-a suf_length] [file [name]]\n");
	pfmt(stderr, MM_NOSTD,
		":142:\tsplit [-line_count] [-a suffix_length] [file [name]]\n");
	exit(1);
}

