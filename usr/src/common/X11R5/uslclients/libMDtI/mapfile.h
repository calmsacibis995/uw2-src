/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libMDtI:mapfile.h	1.4"
#endif

#ifndef __mapfile_h__
#define __mapfile_h__

#include <sys/types.h>
#include <sys/mman.h>

typedef struct {
	size_t		filesize;	/* file size */
	int		line;		/* current line count */
	char		*mapptr;	/* ptr to mapped space */
	char		*curptr;	/* ptr to current loc */
	char		*endptr;	/* ptr to the byte after the */
					/* end of mapped space */
} DmMapfileRec, *DmMapfilePtr;

#define MF_EOFPTR(MP)	((MP)->endptr)
#define MF_CURPTR(MP)	((MP)->curptr)
#define MF_EOF(MP)	((MP)->curptr == (MP)->endptr)
#define MF_NOT_EOF(MP)	((MP)->curptr != (MP)->endptr)
#define MF_NEWLINE(MP)	((*((MP)->curptr) == '\n') ? (MP)->line++ : 0)
#define MF_PEEKC(MP)	(MF_NOT_EOF(MP) ? *((MP)->curptr) : EOF)
#define MF_GETC(MP)	(MF_NOT_EOF(MP) ? (MF_NEWLINE(MP), *(((MP)->curptr)++))\
			 : EOF)
#define MF_GETPTR(MP)	(MF_NOT_EOF(MP) ? ((MP)->curptr) : NULL)
#define MF_NEXTC(MP)	(MF_NOT_EOF(MP) ? (MF_NEWLINE(MP), (MP)->curptr++) : 0)
#define MF_LINENO(MP)	((MP)->line)

/* function prototypes */
extern DmMapfilePtr Dm__mapfile(char *filename, int prot, int flags);
extern void Dm__unmapfile(DmMapfilePtr mp);
extern char *Dm__findchar(DmMapfilePtr, int ch);
extern char *Dm__strchr(DmMapfilePtr mp, char *str);
extern char *Dm__strstr(DmMapfilePtr mp, char *str);

#endif /* __mapfile_h__ */
