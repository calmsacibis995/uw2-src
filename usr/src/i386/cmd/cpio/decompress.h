/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cpio:i386/cmd/cpio/decompress.h	1.1"

#define	MBUFFERS	32
#define	ZREAD_SIZE	(32*1024)
#define	ZBUFFER_SIZE	(ZREAD_SIZE*MBUFFERS)

typedef struct {
	unsigned long ucmp_size;
	unsigned long cmp_size;
	unsigned long daddr;
} HEADER_INFO ;

/* Shared memory structure */
typedef struct {
	int count[MBUFFERS];
	int sequence[MBUFFERS];
} SB;

extern void (*mem_decompress)(char *, char *, unsigned long, unsigned long);
