/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)portmgmt:common/cmd/portmgmt/tty_settings/ttylist.c	1.2.7.2"
#ident  "$Header: ttylist.c 2.0 91/07/13 $"

/*
 * ttylist.c - a program to check and list the hunt sequences in
 *	       /etc/ttydefs.
 *
 * Usage: cat /etc/ttydefs | cut -d: -f1,5 | ttylist
 */

#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>

#define	MAXTABLE	200

#define	NOTSEQ		-1
#define	NOLABEL		-2
#define	COMPLETE	-3

struct	tbl {
	char	label[16];
	char	nextlabel[16];
	int	next;
	int	printed;
} tbl[MAXTABLE+1];

int	Nentry;

static	char *	getline();
static	int	findlabel();
static	void	printtbl();

main()
{
	int	i,j,ind;
	char	*sp, *bp, *tp;

	Nentry = 0;
	while ((bp = getline()) != NULL) {
		sp = bp;
		if (*bp == '#')
			continue;
		while (isspace(*bp)) bp++;
		if (strlen(bp) == 0)
			continue;
		if ((tp = strtok(bp,":")) == NULL) {
			printf("%s :(incorrect format, use <sttydefs -l> to check it)##true\n",sp);
			continue;
		}
		Nentry++;
		if (Nentry > MAXTABLE) {
			printf("WARNING: internal table overflow##true\n");
			Nentry--;
			break;
		}
		(void)strcpy(tbl[Nentry].label,tp);
		if ((tp = strtok(NULL,":")) == NULL) {
			printf("%s :(incorrect format, use <sttydefs -l> to check it)##true\n",sp);
			Nentry--;
			continue;
		}
		(void)strcpy(tbl[Nentry].nextlabel,tp);
		tbl[Nentry].next = 0;
		tbl[Nentry].printed = 0;
	}

	for (i=1; i<=Nentry; i++) {
		if (tbl[i].next == 0) {
			j = i;
			while ((ind=findlabel(j)) > 0) {
				if (ind == i) {
					tbl[j].next = COMPLETE;
					break;
				}
				if (tbl[ind].next > 0)
					break;
				tbl[j].next = ind;
				j = ind;
			}
			if (ind < 0)
				tbl[j].next = ind;
		}
	}
	printtbl();
}

static	int
findlabel(slot) 
int	slot;
{
	int	i;
	for (i=1; i<=Nentry; i++) {
		if (strcmp(tbl[slot].label,tbl[slot].nextlabel) == 0) {
			return(NOTSEQ);
		}
		if (strcmp(tbl[i].label,tbl[slot].nextlabel) == 0) {
			return(i);
		}
	}
	return(NOLABEL);
}

static	void
printtbl()
{
	int	i,j;
	for (i=1; i<=Nentry; i++) {
		if (!tbl[i].printed) {
			j = i;
			for (;;) {
				printf("%s:%s",tbl[j].label,tbl[j].nextlabel);
				tbl[j].printed = 1;
				if (tbl[j].next > 0) {
					printf("##false\n");
					j = tbl[j].next;
					if (tbl[j].printed)
						break;
				}
				else {
					if (tbl[j].next == NOTSEQ)
					    printf(" (does not sequence)##false\n");
					else if (tbl[j].next == NOLABEL)
					    printf(" (Nextlabel not found)##false\n");
					else 
						printf("##false\n");
					break;
				}
			}
		}
	}
}

static
char	*
getline()
{
	register	int	c;
	register	char	*cp;

	static		char	buf[BUFSIZ];

	cp = buf;
	while ((cp < (buf + BUFSIZ - 1)) && ((c = getchar()) != EOF)) {
		if (c == '\n')
			break;
		*cp++ = (char)c;
	}
	*cp = '\0';
	return((c == EOF && cp == buf) ? NULL : buf);
}
