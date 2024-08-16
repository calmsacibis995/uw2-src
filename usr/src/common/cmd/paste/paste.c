/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)paste:paste.c	1.4.3.2"
#
/* paste: concatenate corresponding lines of each file in parallel. Release 1.4 */
/*	(-s option: serial concatenation like old (127's) paste command */
# include <stdio.h>	/* make :  cc paste.c  */
# include <sys/euc.h>
# include <getwidth.h>
# include <locale.h>
# include <pfmt.h>
# include <string.h>
# include <errno.h>
# include <stdlib.h>
# include <limits.h>

# ifndef LINE_MAX
# define LINE_MAX	2048
# endif

# define MAXOPNF 12  	/* maximal no. of open files (not with -s option) */
# define MAXLINE LINE_MAX  	/* maximal line length */
char del[MAXOPNF][MB_LEN_MAX+1] = {"\t"};

eucwidth_t	wp;
  
static const char badopen[] = ":3:Cannot open %s: %s\n";

int move();

void
main(argc, argv)
int argc;
char ** argv;
{
	int i, j, k, eofcount, nfiles, glue;
	int delcount = { 1 } ;
	int onefile  = { 0 } ;
	register int c ;
	int errflg = 0;
	FILE *inptr[MAXOPNF];
	char sepbuf[MB_LEN_MAX*MAXOPNF];
	extern int optind;
	extern char *optarg;
	void usage();
	void diag();
 	int delw;
	int mdelc;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:paste");

	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;

	while ((c = getopt(argc, argv, "d:s")) != EOF){
		switch (c) {
			case 's' :  onefile++;
				    break ;
			case 'd' :
				if((delcount = move(optarg,del)) == 0) {
					pfmt(stderr, MM_ERROR, ":51:no delimiters\n");
					errflg++;
				}
				break;
			default :
				errflg++;
		}
	} /* end options */
	if (errflg) usage();
 
	argv = &argv[optind];
	argc -= optind;
	if (argc < 1)		/* no files given */
		usage();
 
	if ( ! onefile) {	/* not -s option: parallel line merging */
		for (i = 0; argc >0 && i < MAXOPNF; i++) {
			if (strcmp(argv[i], "-") == 0) {
				inptr[i] = stdin;
			} else {
				inptr[i] = fopen(argv[i], "r");
				if (inptr[i] == NULL)
					diag(badopen, argv[i], strerror(errno));
			}
			argc--;
		}
		if (argc > 0) diag(":52:Too many files - limit %d\n", MAXOPNF);
		nfiles = i;
  
		do {
			sepbuf[0] = '\0';
			eofcount = 0;
			j = k = 0;
			for (i = 1; i <= nfiles; i++) {
				while((c = getc(inptr[i-1])) != '\n'
							&& c != EOF) {
					if (sepbuf[0]) {
					    (void)fputs(sepbuf, stdout);
					    sepbuf[0] = '\0';
					}
					(void)putchar(c);
				}
				if (c == EOF)
					eofcount++;
				else
					j = 1;
				if (i < nfiles)
					if ( j )
					    (void)fputs(del[k], stdout);
					else
					    (void)strcat(sepbuf, del[k]);
				k = (k + 1) % delcount;
			}
			if (eofcount < nfiles)	(void)putchar('\n');
		}while (eofcount < nfiles);
  
	} else {/* -s option: serial file pasting (old 127 paste command) */
		for (i = 0; i < argc; i++) {
			if (strcmp(argv[i], "-") == 0) {
				inptr[0] = stdin;
			} else {
				inptr[0] = fopen(argv[i], "r");
				if (inptr[0] == NULL) {
					diag(badopen, argv[i], strerror(errno));
				}
			}
	  
			glue = 0;
			j = 0;
			k = 0;
			while((c = getc(inptr[0])) != EOF)   {
				if (glue) {
					glue = 0;
					(void)fputs(del[k], stdout);
					k = (k + 1) % delcount;
				}
				if(c != '\n') {
					(void)putchar(c);
				} else glue++;
			}
			(void)putchar('\n');
			if (inptr[0] != stdin)
				(void)fclose(inptr[0]);
		}
	}
	exit(0);
	/* NOTREACHED */
} 

void diag(s,a1, a2)
char *s,*a1, *a2;
{
	pfmt(stderr, MM_ERROR, s, a1, a2);
	exit(1);
}
  
int
move(from, to)
char *from, to[MAXOPNF][MB_LEN_MAX+1];
{
	int c, i;
	char *cp;
	int l;


	for (i = 0; i < MAXOPNF; i++) {
		cp = to[i];
		if (!*from) break;
		c = *(unsigned char *)from++;
		if (!wp._multibyte || ISASCII(c)) {
			if (c != '\\') *cp++ = c;
			else { c = *(unsigned char *)from++;
				switch (c) {
					case '0' : 	break;
					case 't' : *cp++ = '\t';
							break;
					case 'n' : *cp++ = '\n';
							break;
					default  : *cp++ = c;
							break;
				}
			}
		} else {
			l = ISSET2(c) ? wp._eucw2 :
			    ISSET3(c) ? wp._eucw3 :
			    c < 0240  ? 1 : wp._eucw1;
			*cp++ = c;
			while ( --l ) *cp++ = *from++;
		}
		*cp = '\0';
	}
	return(i);
}
void usage()
{
	pfmt(stderr, MM_ACTION,
		":54:Usage: paste [-s] [-d<delimiterstring>] file1 file2 ...\n");
	exit(1);
}
