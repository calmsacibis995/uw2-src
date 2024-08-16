/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/data.c	2.15.1.3"
/*LINTLIBRARY*/

#include "synonyms.h"

	/*
	* These definitions copied from <stdio.h> so that this file
	* can define its own _iob that has a distinctly different
	* type from that declared in <stdio.h>.
	*/
typedef struct	/* copied from <stdio.h> */
{
	int		_cnt;	/* number of available characters in buffer */
	unsigned char	*_ptr;	/* next character from/to here in buffer */
	unsigned char	*_base;	/* the buffer */
	unsigned char	_flag;	/* the state of the stream */
	unsigned char	_file;	/* file descriptor */
	unsigned char	_buf[2];/* micro buffer as a fall-back */
} FILE;

#define _IONBF		0004	/* copied from <stdio.h>: not buffered */
#define _IOREAD		0001	/* copied from <stdio.h>: currently reading */
#define _IOWRT		0002	/* copied from <stdio.h>: currently writing */

#include "stdiom.h"

#ifdef __STDC__
	#pragma weak _iob = __iob
#endif

	/*
	* For binary compatibility with older shared libaries, the
	* size of __iob[] must match that of previous implementations.
	*/
#ifdef i386
#   define _NFILE 60
#else
#   define _NFILE 20
#endif

#ifdef __STDC__
#   include <stddef.h>
#   define OFFSET	offsetof(struct t_preopen, bfile)
#else
#   define OFFSET	((Uint)&(((struct t_preopen *)0)->bfile[0]))
#endif
#define IOBSIZE	(sizeof(FILE) * _NFILE)
#define NBFILE	((IOBSIZE - OFFSET) / sizeof(BFILE))
#define PADSIZE	(IOBSIZE - OFFSET - NBFILE * sizeof(BFILE))

struct s	/* OUR shape of iob array */
{
	FILE	file[NSMALL];	/* advertised FILE stream objects */
	BFILE	bfile[NBFILE];	/* internal BFILE stream objects */
	Uchar	pad[PADSIZE];
};

#ifndef PIC

struct s _iob =
{
    /*
    * This initializes the advertised FILE stream objects.
    */
    {
	{0, 0, (Uchar *)&_iob.bfile[1], _IOREAD, 0},
	{0, 0, (Uchar *)&_iob.bfile[2], _IOWRT, 1},
#ifdef NO_FILE_OVERLAP
	{0, 0, (Uchar *)&_iob.bfile[0], _IOWRT | _IONBF, 2}
#endif
    },
    /*
    * This initializes the internal BFILE stream objects.
    */
    {
	/*
	* The first three are the preopened streams that correspond
	* (point to) the published three preopened FILE streams.
	*/
	{{0, 0, (Uchar *)&_iob.file[2], _IOWRT | _IONBF, 2},
					&_iob.bfile[3], 0, 0, IO_ACTIVE, 2},
	{{0, 0, (Uchar *)&_iob.file[0]}, &_iob.bfile[2], 0, 0, IO_ACTIVE, 0},
	{{0, 0, (Uchar *)&_iob.file[1]}, &_iob.bfile[0], 0, 0, IO_ACTIVE, 1},
	/*
	* The rest of the iob array of memory is used to hold ready-to-use
	* internal streams.  Unfortunately, the number of such is very
	* dependent on the sizes and alignments of the types FILE and BFILE,
	* as well as the value of _NFILE.  Since the initializations cannot
	* be parameterized (sizeof cannot be used in #if directives), the
	* values were computed for the six "normal" answers (+1, +3, +4, +11,
	* +19, +20), and that is as many as are initialized.  In general,
	* this should, at worst, leave some potential space unused.
	*/
#if _NFILE == 20 && defined(_REENTRANT)					/*+1*/
	{{0, 0, (Uchar *)&_iob.bfile[3]}}
#else
#if _NFILE == 20 && !defined(_REENTRANT) && defined(NO_FILE_OVERLAP)	/*+3*/
	{{0, 0, (Uchar *)&_iob.bfile[3]}, &_iob.bfile[4]},
	{{0, 0, (Uchar *)&_iob.bfile[4]}, &_iob.bfile[5]},
	{{0, 0, (Uchar *)&_iob.bfile[5]}}
#else
#if _NFILE == 20 && !defined(_REENTRANT) && !defined(NO_FILE_OVERLAP)	/*+4*/
	{{0, 0, (Uchar *)&_iob.bfile[3]}, &_iob.bfile[4]},
	{{0, 0, (Uchar *)&_iob.bfile[4]}, &_iob.bfile[5]},
	{{0, 0, (Uchar *)&_iob.bfile[5]}, &_iob.bfile[6]},
	{{0, 0, (Uchar *)&_iob.bfile[6]}}
#else
#if _NFILE != 20 && defined(_REENTRANT)					/*+11*/
	{{0, 0, (Uchar *)&_iob.bfile[3]}, &_iob.bfile[4]},
	{{0, 0, (Uchar *)&_iob.bfile[4]}, &_iob.bfile[5]},
	{{0, 0, (Uchar *)&_iob.bfile[5]}, &_iob.bfile[6]},
	{{0, 0, (Uchar *)&_iob.bfile[6]}, &_iob.bfile[7]},
	{{0, 0, (Uchar *)&_iob.bfile[7]}, &_iob.bfile[8]},
	{{0, 0, (Uchar *)&_iob.bfile[8]}, &_iob.bfile[9]},
	{{0, 0, (Uchar *)&_iob.bfile[9]}, &_iob.bfile[10]},
	{{0, 0, (Uchar *)&_iob.bfile[10]}, &_iob.bfile[11]},
	{{0, 0, (Uchar *)&_iob.bfile[11]}, &_iob.bfile[12]},
	{{0, 0, (Uchar *)&_iob.bfile[12]}, &_iob.bfile[13]},
	{{0, 0, (Uchar *)&_iob.bfile[13]}}
#else
#if _NFILE != 20 && !defined(_REENTRANT) && defined(NO_FILE_OVERLAP)	/*+19*/
	{{0, 0, (Uchar *)&_iob.bfile[3]}, &_iob.bfile[4]},
	{{0, 0, (Uchar *)&_iob.bfile[4]}, &_iob.bfile[5]},
	{{0, 0, (Uchar *)&_iob.bfile[5]}, &_iob.bfile[6]},
	{{0, 0, (Uchar *)&_iob.bfile[6]}, &_iob.bfile[7]},
	{{0, 0, (Uchar *)&_iob.bfile[7]}, &_iob.bfile[8]},
	{{0, 0, (Uchar *)&_iob.bfile[8]}, &_iob.bfile[9]},
	{{0, 0, (Uchar *)&_iob.bfile[9]}, &_iob.bfile[10]},
	{{0, 0, (Uchar *)&_iob.bfile[10]}, &_iob.bfile[11]},
	{{0, 0, (Uchar *)&_iob.bfile[11]}, &_iob.bfile[12]},
	{{0, 0, (Uchar *)&_iob.bfile[12]}, &_iob.bfile[13]},
	{{0, 0, (Uchar *)&_iob.bfile[13]}, &_iob.bfile[14]},
	{{0, 0, (Uchar *)&_iob.bfile[14]}, &_iob.bfile[15]},
	{{0, 0, (Uchar *)&_iob.bfile[15]}, &_iob.bfile[16]},
	{{0, 0, (Uchar *)&_iob.bfile[16]}, &_iob.bfile[17]},
	{{0, 0, (Uchar *)&_iob.bfile[17]}, &_iob.bfile[18]},
	{{0, 0, (Uchar *)&_iob.bfile[18]}, &_iob.bfile[19]},
	{{0, 0, (Uchar *)&_iob.bfile[19]}, &_iob.bfile[20]},
	{{0, 0, (Uchar *)&_iob.bfile[20]}, &_iob.bfile[21]},
	{{0, 0, (Uchar *)&_iob.bfile[21]}}
#else
#if _NFILE != 20 && !defined(_REENTRANT) && !defined(NO_FILE_OVERLAP)	/*+20*/
	{{0, 0, (Uchar *)&_iob.bfile[3]}, &_iob.bfile[4]},
	{{0, 0, (Uchar *)&_iob.bfile[4]}, &_iob.bfile[5]},
	{{0, 0, (Uchar *)&_iob.bfile[5]}, &_iob.bfile[6]},
	{{0, 0, (Uchar *)&_iob.bfile[6]}, &_iob.bfile[7]},
	{{0, 0, (Uchar *)&_iob.bfile[7]}, &_iob.bfile[8]},
	{{0, 0, (Uchar *)&_iob.bfile[8]}, &_iob.bfile[9]},
	{{0, 0, (Uchar *)&_iob.bfile[9]}, &_iob.bfile[10]},
	{{0, 0, (Uchar *)&_iob.bfile[10]}, &_iob.bfile[11]},
	{{0, 0, (Uchar *)&_iob.bfile[11]}, &_iob.bfile[12]},
	{{0, 0, (Uchar *)&_iob.bfile[12]}, &_iob.bfile[13]},
	{{0, 0, (Uchar *)&_iob.bfile[13]}, &_iob.bfile[14]},
	{{0, 0, (Uchar *)&_iob.bfile[14]}, &_iob.bfile[15]},
	{{0, 0, (Uchar *)&_iob.bfile[15]}, &_iob.bfile[16]},
	{{0, 0, (Uchar *)&_iob.bfile[16]}, &_iob.bfile[17]},
	{{0, 0, (Uchar *)&_iob.bfile[17]}, &_iob.bfile[18]},
	{{0, 0, (Uchar *)&_iob.bfile[18]}, &_iob.bfile[19]},
	{{0, 0, (Uchar *)&_iob.bfile[19]}, &_iob.bfile[20]},
	{{0, 0, (Uchar *)&_iob.bfile[20]}, &_iob.bfile[21]},
	{{0, 0, (Uchar *)&_iob.bfile[21]}, &_iob.bfile[22]},
	{{0, 0, (Uchar *)&_iob.bfile[22]}}
#else
 #error "Unmatched combination of _NFILE, _REENTRANT, and NO_FILE_OVERLAP"
#endif
#endif
#endif
#endif
#endif
#endif
    }
};

#else /*!PIC*/

struct s _iob = {0};

void
_init()	/* called by rtld because copy relocations for libc.so are broken */
{
	register BFILE *bp, **bpp;

	/*
	* The NSMALL preopened advertised FILEs.
	*/
	_iob.file[0]._base = (Uchar *)&_iob.bfile[1];
	_iob.file[0]._flag = _IOREAD;
	_iob.file[0]._file = 0;
	_iob.file[1]._base = (Uchar *)&_iob.bfile[2];
	_iob.file[1]._flag = _IOWRT;
	_iob.file[1]._file = 1;
#ifdef NO_FILE_OVERLAP
	_iob.file[2]._base = (Uchar *)&_iob.bfile[0];
	_iob.file[2]._flag = _IOWRT | _IONBF;
	_iob.file[2]._file = 2;
#endif
	/*
	* The three BFILEs for stderr, stdin, and stdout.
	*/
#ifdef NO_FILE_OVERLAP
	_iob.bfile[0].file._base = (Uchar *)&_iob.file[2];
#else
	_iob.bfile[0].file._base = (Uchar *)&_iob.bfile[0];
	_iob.bfile[0].file._flag = _IOWRT | _IONBF;
	_iob.bfile[0].file._file = 2;
#endif
	_iob.bfile[0].next = 0;		/* probably changed below */
	_iob.bfile[0].eflags = IO_ACTIVE;
	_iob.bfile[0].fd = 2;
	_iob.bfile[1].file._base = (Uchar *)&_iob.file[0];
	_iob.bfile[1].next = &_iob.bfile[2];
	_iob.bfile[1].eflags = IO_ACTIVE;
	_iob.bfile[1].fd = 0;
	_iob.bfile[2].file._base = (Uchar *)&_iob.file[1];
	_iob.bfile[2].next = &_iob.bfile[0];
	_iob.bfile[2].eflags = IO_ACTIVE;
	_iob.bfile[2].fd = 1;
	/*
	* The rest is used to hold NBFILE-3 more internal streams.
	*/
	bpp = &_iob.bfile[0].next;
	for (bp = &_iob.bfile[2]; ++bp < &_iob.bfile[NBFILE]; bpp = &bp->next)
	{
		*bpp = bp;
		bp->file._base = (Uchar *)bp;
	}
}

#endif /*PIC*/
