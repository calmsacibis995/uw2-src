/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dump:common/dump.h	1.3"
#define DATESIZE 60

typedef struct scntab {
	char             *scn_name;
	Elf32_Shdr       *p_shdr;
	Elf_Scn          *p_sd;
} SCNTAB;

#ifdef __STDC__
#define VOID_P void *
#else
#define VOID_P char *
#endif

#define UCHAR_P unsigned char *

#define FAILURE 1
#define SUCCESS 0
