/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:util/makestrs.c	1.2"
/* $XConsortium: makestrs.c,v 1.4 91/05/04 18:25:06 rws Exp $ */
/*
Copyright 1991 by the Massachusetts Institute of Technology

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

/* Constructs string definitions */

#include <stdio.h>
#include <X11/Xos.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
char *malloc();
#endif
#if defined(macII) && !defined(__STDC__)  /* stdlib.h fails to define these */
char *malloc();
#endif /* macII */

typedef struct _Rec {
    struct _Rec *next;
    char *left;
    char *right;
    int offset;
} Rec;

char buf[1024];
char global[64];
char header_name[64];
Rec *recs;
Rec **tail = &recs;
int offset;

AddRec(left, right)
    char *left;
    char *right;
{
    Rec *rec;
    int llen;
    int rlen;
    int len;

    llen = len = strlen(left) + 1;
    rlen = strlen(right) + 1;
    if (right != left + 1)
	len += rlen;
    rec = (Rec *)malloc(sizeof(Rec) + len);
    if (!rec)
	exit(1);
    rec->offset = offset;
    offset += rlen;
    rec->left = (char *)(rec + 1);
    strcpy(rec->left, left);
    if (llen != len) {
	rec->right = rec->left + llen;
	strcpy(rec->right, right);
    } else {
	rec->right = rec->left + 1;
    }
    *tail = rec;
    tail = &rec->next;
}

FlushRecs(header)
    FILE *header;
{
    Rec *rec;

    *tail = NULL;
    fprintf(header, "/* This file is automatically generated. */\n");
    fprintf(header, "/* Do not edit. */\n");
    fprintf(header, "\n");
    fprintf(header, "#ifndef _Xt%s_h_\n", header_name);
    fprintf(header, "#define _Xt%s_h_\n", header_name);
    fprintf(header, "#ifdef XTSTRINGDEFINES\n");
    for (rec = recs; rec; rec = rec->next) {
	fprintf(header, "#define Xt%s \"%s\"\n", rec->left, rec->right);
    }
    fprintf(header, "#else\n");
    fprintf(header, "#if __STDC__\n");
    fprintf(header, "#define _XtConst_ const\n");
    fprintf(header, "#else\n");
    fprintf(header, "#define _XtConst_ /**/\n");
    fprintf(header, "#endif\n");
    fprintf(header, "extern _XtConst_ char %s[];\n", global);
    for (rec = recs; rec; rec = rec->next) {
	fprintf(header, "#ifndef Xt%s\n", rec->left);
#ifdef ARRAYPERSTR
	fprintf(header, "extern char Xt%s[];\n", rec->left);
#else
	fprintf(header, "#define Xt%s ((char*)&%s[%d])\n",
		rec->left, global, rec->offset);
#endif
	fprintf(header, "#endif\n");
    }
    fprintf(header, "#undef _XtConst_\n");
    fprintf(header, "#endif\n");
    fprintf(header, "#endif\n");
    while (rec = recs) {
	recs = rec->next;
	free((char *)rec);
    }
    tail = &recs;
    offset = 0;
}

main()
{
    FILE *header = NULL;
    char *right;
#ifndef ARRAYPERSTR
    int first;
#endif

    printf("/* This file is automatically generated. */\n");
    printf("/* Do not edit. */\n");
    printf("\n");
    printf("#if __STDC__\n");
    printf("#define Const const\n");
    printf("#else\n");
    printf("#define Const /**/\n");
    printf("#endif\n");
    while (gets(buf)) {
	if (!buf[0] || (buf[0] == '!'))
	    continue;
	if (buf[0] == ':') {
	    if (header) {
#ifndef ARRAYPERSTR
		printf("\n");
		printf("};\n");
#endif
		FlushRecs(header);
		fclose(header);
	    }
	    right = index(buf, ' ');
	    if (!right)
		exit(1);
	    *right = 0;
	    header = fopen(buf+1, "w");
	    if (!header) {
		perror(buf+1);
		exit(1);
	    }
#ifndef ARRAYPERSTR
	    first = 1;
	    strcpy(global, right+1);
	    printf("\n");
	    printf("Const char %s[] = {\n", global);
#endif
	    strcpy(header_name, buf+1);
	    right = index(header_name, '.');
	    if (right)
		*right = 0;
	    continue;
	}
	if (right = index(buf, ' '))
	    *right++ = 0;
	else
	    right = buf + 1;
	AddRec(buf, right);
#ifdef ARRAYPERSTR
	printf("Const char Xt%s[] = \"%s\";\n", buf, right);
#else
	if (!first)
	    printf(",\n");
	first = 0;
	while (*right) {
	    printf("'%c',", *right);
	    right++;
	}
	printf("0");
#endif
    }
    if (header) {
	FlushRecs(header);
	fclose(header);
#ifndef ARRAYPERSTR
	printf("\n");
	printf("};\n");
#endif
    }
    exit(0);
}
