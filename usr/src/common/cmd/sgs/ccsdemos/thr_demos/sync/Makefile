#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ccsdemos:thr_demos/sync/Makefile	1.1"
all: demo_barrier demo_cond demo_rmutex demo_rwlock \
     demo_sbarrier demo_sema demo_spin
demo_barrier:	demo_barrier.c
	cc -lthread -ldl demo_barrier.c -o demo_barrier
demo_cond:	demo_cond.c
	cc -lthread -ldl demo_cond.c -o demo_cond
demo_rmutex:	demo_rmutex.c
	cc -lthread -ldl demo_rmutex.c -o demo_rmutex
demo_rwlock:	demo_rwlock.c
	cc -lthread -ldl demo_rwlock.c -o demo_rwlock
demo_sbarrier:	demo_sbarrier.c
	cc -lthread -ldl demo_sbarrier.c -o demo_sbarrier
demo_sema:	demo_sema.c
	cc -lthread -ldl demo_sema.c -o demo_sema
demo_spin:	demo_spin.c
	cc -lthread -ldl demo_spin.c -o demo_spin


