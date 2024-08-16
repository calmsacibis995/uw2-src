/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idreadauto.c	1.9"
#ident	"$Header:"

#include <stdio.h>
#include "inst.h"
#include <malloc.h>
#include <stdlib.h>

#ifndef CROSS
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ksym.h>
#endif

#include <locale.h>
#include <pfmt.h>

#define ONEMEG 1048576
#define EMULTISPEC 2
#define EUSAGE 3
#define ENULLCURVE 4

int debug;
int idra_error = 0;

struct ahash {
	struct atune *head;
	struct atune *tail;
} hash[NWHICH];

extern char pathinst[];
extern char *optarg;

main(argc, argv)
int argc;
char ** argv;
{
int nfields, j, rc, fd;
int def, min, max; 
int approxmem=0, memsz=0;
struct atune *aptr;
char opt, tunable[20], which[14], junk3[14], linetype[14],root[128];
char name[20];

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idreadauto");

	while ((opt = getopt(argc, argv, "scr:t:")) != EOF) {
		switch(opt) {
			case 'r':
				strcpy(root,optarg);
				break;
			case 't':
				approxmem=atol(optarg);
				break;
			case 's':
				flatbuild();
				exit(0);
				break;
			case 'c':
				curvebuild();
				exit(0);
				break;
			default:
				pfmt(stderr, MM_ACTION, ":76:Usage: %s [-r root] [ -c | -s | -t tunemem ]\n",argv[0]);
				exit(EUSAGE);
		}
	}

        /* Tell getinst() about directory names */
	strcat(root,pathinst);
 	strcpy(pathinst, root);
	
	if ((rc=rdatune()) != 0)
		exit(rc);

	if (approxmem != 0) {
		memsz = approxmem;
	}
#ifndef CROSS
	else {
		ulong_t addr = 0, type = 0;
		getksym("tunemem", &addr, &type);
		fd = open("/dev/kmem", O_RDONLY);
		if (fd != -1) {
			lseek(fd, addr, 0);
			read(fd, &memsz, sizeof(int));
		}
	}
#endif
	if (memsz == 0) {
		memsz = 8;
	}

	aptr=hash[TV_DEF].head;
	for (; aptr != NULL; aptr=aptr->next) {
		if ( (aptr->next == NULL ) || (strcmp(aptr->next->tv_name, aptr->tv_name) != 0) ) {

			min = tune(hash[TV_MIN].head, memsz, aptr->tv_name);
			max = tune(hash[TV_MAX].head, memsz, aptr->tv_name);
			def = tune(hash[TV_DEF].head, memsz, aptr->tv_name);

			if (max < min) {
				pfmt(stderr, MM_WARNING,
":77:For %s, maximum is less than minimum.  This indicates\nan inconsistent autotune file.\n",
				aptr->tv_name);
			} else if (def < min) {
				pfmt(stderr, MM_WARNING,
":78:For %s, default is less than minimum.  This indicates\nan inconsistent autotune file.  Default will be increased to minimum.\n",
					aptr->tv_name);
				def = min;
			} else if (def > max) {
				pfmt(stderr, MM_WARNING,
":79:For %s, default is greater than maximum.  This indicates\nan inconsistent autotune file.  Default will be decreased to maximum.\n",
					aptr->tv_name);
				def = max;
			}

			printf("%s\t%d\t%d\t%d\t%%%%AUTO%%%%\n", aptr->tv_name, def, min, max);
	
		}
	}

	return(idra_error);

}

tune(curve, ms, name)
struct atune *curve;
int ms;
char *name;
{
	int i, val, num, den, extrapolate;
	struct atune *aptr, *prev, *bptr;

	if (curve == NULL) {
		idra_error = ENULLCURVE;
		return(0);
	}

	for (aptr=curve; aptr->next != NULL ; aptr=aptr->next){
		if ((strcmp(aptr->tv_name,aptr->next->tv_name) == 0) && 
		    (aptr->tv_mempt == aptr->next->tv_mempt)) { 
			idra_error = EMULTISPEC;
			return(0);
		}
	}

	extrapolate = 0;
	for (aptr=curve; aptr->next != NULL ; aptr=aptr->next){
		if ((strcmp(aptr->tv_name, name) == 0) && 
		   (ms < aptr->tv_mempt))
			break;
		if (strcmp(aptr->tv_name, name) > 0) {
			extrapolate = 1; 
			break;
		}
		prev=aptr;
	}
	if (strcmp(aptr->tv_name, name) > 0) {
		extrapolate = 1; 
	}

	if (aptr==curve) {		
		/* First tunable and below smallest mempt */
		return(curve->tv_tuneval);
	}

	if (strcmp(prev->tv_name, name) == 0) {
		if (prev->tv_linetype == TV_STEP) {
			return(prev->tv_tuneval);
		} 
	} else {	
		/* Below smallest mempt for this tunable */
		return(aptr->tv_tuneval);	
	}


	/* tv_linetype == TV_LINEAR */

	if ((extrapolate == 1)) { 
		/* Back up aptr and prev to the last two for this tunable */
		aptr = prev; 	
		for (bptr=curve; bptr->next != aptr; bptr=bptr->next);
		prev = bptr;
		if (strcmp(prev->tv_name, name) != 0) {
			/* Single line */
			return(aptr->tv_tuneval);	
		}

	}

	num = ms - prev->tv_mempt;
	den = aptr->tv_mempt - prev->tv_mempt;
	val = prev->tv_tuneval + (num*(aptr->tv_tuneval - prev->tv_tuneval))/den;
	return(val);
}


rdatune()
{
	struct atune *btune, *whichlist, *whichtail, *ptr;
	int ctune, stat, alph;
	char done;

	getinst(ATUN, RESET, NULL);

	if ((btune = (struct atune *)malloc(sizeof(struct atune))) == NULL){
		pfmt(stderr, MM_ERROR, ":80:Out of space during autotune\n");
		exit(1);
	}

	while ((stat = getinst(ATUN, NEXT, btune)) == 1) {
		whichlist = hash[btune->tv_which].head; 
		whichtail = hash[btune->tv_which].tail; 
		if (whichlist == NULL) {
			hash[btune->tv_which].head = btune;
			hash[btune->tv_which].tail = btune;
		} else if (((alph=strcmp(btune->tv_name,whichtail->tv_name)) > 0)
			|| ((alph==0) &&
				(btune->tv_mempt > whichtail->tv_mempt))) {
			/* Putting at tail */
			whichtail->next = btune;
			hash[btune->tv_which].tail = btune;
		} else if (((alph=strcmp(btune->tv_name,whichlist->tv_name)) < 0)
			|| ((alph==0) &&
				(btune->tv_mempt < whichlist->tv_mempt))) {
			/* Putting at head */
			btune->next = whichlist;
			hash[btune->tv_which].head = btune;
		} else {
			for (ptr=whichlist, done=0; ptr->next!=NULL && !done ; ptr=ptr->next){
				if (((alph=strcmp(ptr->next->tv_name,btune->tv_name)) > 0)
				    || ((alph==0) &&
				    (ptr->next->tv_mempt > btune->tv_mempt))) {
					btune->next = ptr->next;
					ptr->next = btune;
					done = 1;
				}
			}
		}

		if ((btune = (struct atune *)malloc(sizeof(struct atune))) == 0){
			pfmt(stderr, MM_ERROR, ":80:Out of space during autotune\n");
			exit(1);
	        }
	}
	if (stat != 0) {
		insterror(stat, ATUN, "");
		exit(1);
	}
}

flatbuild()
{
	char name[20];
	int tuneval;

	scanf("%s %d",name,&tuneval);
	printf("struct tune_point %scurve[] = {\n",name);
	printf("\t{4, %d, TV_STEP}\n",tuneval);
	printf("};\n");
	printf("int %scurvesz = 1;\n",name);
	return(0);
}

curvebuild() 
{
	char name[20], which[5], linetype[16];
	int opt, mempt, tuneval, linecount=0;

	while (scanf("%s %s %s %d %d",name,which,linetype,&mempt,&tuneval) != -1) {
		if (strcmp(which,"DEF") == 0) {
			if (linecount == 0) {
				printf("struct tune_point %scurve[] = {\n",name);
			} else {
				printf(",\n");
			}
			printf("\t{%d, %d, TV_%s}",mempt,tuneval,linetype);
			linecount++;
		}
	}
	printf(" };\n");
	printf("int %scurvesz = %d;\n",name, linecount);
}
