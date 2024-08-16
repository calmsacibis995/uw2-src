#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:sys/makefile	1.5.9.7"
#
# makefile for libc/port/sys
#
#

include $(LIBRULES)

.SUFFIXES: .p .P
PROF=
NONPROF=
PIC=
ROOT=
INCPORT=../inc
INCCPU=../../$(CPU)/inc
SDEFLIST=
INCLIST=-I$(INCCPU) -I$(INCPORT)

OBJECTS=\
execl.o       execle.o      execv.o	lockf.o    \
msgsys.o      nfssys.o      semsys.o    shmsys.o   ptrace.o

POBJECTS=$(OBJECTS:.o=.p)

PIC_OBJECTS=\
execl.P       execle.P      execv.P     lockf.P    \
msgsys.P      semsys.P      shmsys.P	ptrace.P

nonprof: $(OBJECTS)

prof: $(POBJECTS)

pic: $(PIC_OBJECTS)

msgsys.o msgsys.p semsys.o semsys.p \
	shmsys.o shmsys.p: $(INC)/sys/ipc.h
msgsys.o msgsys.p: $(INC)/sys/msg.h
semsys.o semsys.p: $(INC)/sys/sem.h
shmsys.o shmsys.p: $(INC)/sys/shm.h
msgsys.o msgsys.p semsys.o semsys.p \
	shmsys.o shmsys.p: $(INC)/sys/types.h
lockf.o lockf.p: $(INC)/errno.h 
lockf.o lockf.p: $(INC)/fcntl.h 
lockf.o lockf.p: $(INC)/unistd.h

.c.o .c.p .c.P:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(INCLIST) $(DEFLIST) $(SDEFLIST) $(CFLAGS) $(LOCALFLAGS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -p $*.c  && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -DDSHLIB -Kpic $*.c  && mv $(*F).o $*.P
