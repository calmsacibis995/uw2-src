/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:common/arena.h	1.5"
#include "manifest.h"

#ifdef __STDC__
#define PROTO(x,y) x y
#else
#define PROTO(x,y) x()
#endif
typedef struct Arena_struct *Arena;

extern myVOID * PROTO(	arena_alloc,(Arena,int size));
extern void PROTO(	arena_term,(Arena));
extern Arena PROTO(	arena_init,(void));
#ifndef NODBG
extern void PROTO(	arena_init_stats,(void));
extern int  PROTO(	arena_max_mem,(void));
extern void PROTO(	arena_init_debug,(Arena, char *));
extern void PROTO(	arena_debug,(Arena,char *));
extern void PROTO(	arena_check,(void));
#endif

#define Arena_alloc(arena,cnt,type) \
	(type *) arena_alloc(arena,(cnt)*sizeof(type))
