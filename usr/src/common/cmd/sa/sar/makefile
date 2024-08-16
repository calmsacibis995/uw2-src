#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sa:common/cmd/sa/sar/makefile	1.6"
#ident  "$Header: $"

include $(CMDRULES)

LOCALDEF = -D_KMEMUSER

OBJECTS = 	sar.o\
		bufcache.o\
		cpu.o\
		disk.o\
		facc.o\
		flook.o\
		free.o\
		fs.o\
		gsched.o\
		init.o\
		inode.o\
 		ipc.o\
		kma.o\
		locsched.o\
		lwpres.o\
		names.o\
		output.o\
		procres.o\
		sarutil.o\
		skmem.o\
		start.o\
		syscall.o\
		tty.o\
		vm.o

sar: $(OBJECTS)
	$(CC) -o sar $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

lintit:
	$(LINT) $(LINTFLAGS) \
	 	sar.c\
		bufcache.c\
		cpu.c\
		disk.c\
		facc.c\
		flook.c\
		free.c\
		fs.c\
		gsched.c\
		init.c\
		inode.c\
 		ipc.c\
		kma.c\
		locsched.c\
		names.c\
		output.c\
		procres.c\
		sarutil.c\
		skmem.c\
		start.c\
		syscall.c\
		tty.c\
		vm.c

