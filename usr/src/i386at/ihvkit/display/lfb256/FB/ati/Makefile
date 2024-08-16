#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ihvkit:display/lfb256/FB/ati/Makefile	1.1"
OBJS = ati.o
FILES = bin/Driver.o bin/Master bin/Node bin/System

all: $(FILES)

bin/Driver.o: $(OBJS)
	ld -r -o $@ $(OBJS)
