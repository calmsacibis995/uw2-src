/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:i386/ktool/unixsyms/parseargs.c	1.1"
/* In a cross-environment, make sure these headers are for the host system */
#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include "usym.h"

#ifdef __STDC__
void parse_args(int argc, char *argv[])
#else
void parse_args(argc, argv)
int argc;
char *argv[];
#endif
{
	long lseek();

	int c,i;
	char *namebuf;
	char		*prog = argv[0];
	char *tmpnam;

	while ((c = getopt(argc, argv, "?dpavl:s:i:e:")) != EOF) {
		switch (c) {
		case 'v':
			break;
		case 'i':
			if (ifd != -1)
				fatal(99,
				"Illegal to specify more than one \"-i\".\n");
			if ((ifd = open(optarg, O_RDONLY)) >=0) {
				if ((icmd_len = lseek(ifd, 0L, 2)) == -1L)
					fatal(7, "%s: seek error on %s\n",
					      prog, optarg);
				lseek(ifd, 0L, 0);
			}
			break;
		case 'e':
			if (efd != -1)
				fatal(99,
				"Illegal to specify more than one \"-e\".\n");
			if ((efd = open(optarg, O_RDONLY)) >= 0) {
				if ((ecmd_len = lseek(efd, 0L, 2)) == -1L)
					fatal(7, "%s: seek error on %s\n",
					      prog, optarg);
				lseek(efd, 0L, 0);
			}
			break;
		case 'd':
			debugonlyflg++;
			break;
		case 's':
			symsection = optarg;
			break;
		case 'a':
			addflg++;
			break;
		case 'l':
			if((fd = open(optarg, O_RDONLY)) < 0) 
				fatal(5,"%s: cannot open limit file %s\n",prog,optarg);
			else {
				if ((lim = lseek(fd, 0L, 2)) == -1L)
					fatal(6, "%s: seek error on %s\n",
					      prog, optarg);
				lseek(fd, 0L, 0);
				if((namebuf = (char *)calloc(lim+1, 1)) == NULL)
					fatal(8, "%s: cannot calloc memory for limit list\n",prog);
				if(read(fd,namebuf,lim) != lim)
					fatal(8, "%s: read error on %s\n",prog, optarg);
				
				i = 0;
				numlim = 0;
				while(i < lim) {
					if(*(namebuf + i) == '\n')
						numlim++;
					i++;
				}
				if((namelist = 
				     (char ** )malloc(numlim * sizeof(char *))) == NULL)
					fatal(8, "%s: cannot malloc memory for limit list\n",prog);
				i = 1;
				*(namelist) = strtok(namebuf,"\n");
				while((tmpnam = strtok(NULL,"\n")) != NULL) {
					*(namelist+i) = tmpnam;
					i++;
				}
				close(fd);
			}
			break;

				
		default:
			goto usage;
		}
	}

	if (argc - optind != 1) {
usage:
		fatal(99,
	"%s: usage: unixsyms [-v] [-s section-name] [-a] [-d] [[-i|-e] init-cmd-file] kernel-file\n",
			prog);
	}

	if ((fd = open(argv[optind], O_RDWR)) < 0)
		fatal(10, "%s: cannot open %s\n", prog, argv[optind]);
}
