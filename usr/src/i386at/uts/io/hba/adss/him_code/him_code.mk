#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/adss/him_code/him_code.mk	1.2"

LD=ld -r
CC=cc
CFLAGS=-O -Xa -DUSL_UNIX -D_LTYPES -DSYSV -DSVR40 -D_SYSTEMENV=4 -D_KERNEL -Di386 -Di386a -DAT386 -DVPIX

all:		him6x60.o copy

him6x60.o:	him6x60.c aic6x60.h scb6x60.h him_scsi.h
		$(CC) $(CFLAGS) him6x60.c -c him6x60.o

copy:		him6x60.o
		./copyit
